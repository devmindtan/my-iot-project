import json
import os
import random
import socket
import threading
import time
from datetime import datetime, timedelta, timezone
from typing import Any, Dict, List, Optional

from dotenv import load_dotenv
from flask import Flask, jsonify, request
from supabase import Client, create_client

load_dotenv()

app = Flask(__name__)

SUPABASE_URL = os.environ.get("SUPABASE_URL")
# Prefer service role key on backend to bypass RLS for internal services.
SUPABASE_KEY = os.environ.get("SUPABASE_SERVICE_ROLE_KEY") or os.environ.get("SUPABASE_KEY")

if not SUPABASE_URL or not SUPABASE_KEY:
    raise RuntimeError("Missing SUPABASE_URL or SUPABASE_SERVICE_ROLE_KEY/SUPABASE_KEY")

# Fixed family user ID — used as owner for all DB records (no auth needed).
# Override via .env: FAMILY_USER_ID=<uuid>
FAMILY_USER_ID: str = os.environ.get(
    "FAMILY_USER_ID", "00000000-0000-0000-0000-000000000001"
)

supabase: Client = create_client(SUPABASE_URL, SUPABASE_KEY)


def _now_iso() -> str:
    return datetime.now(timezone.utc).isoformat()


def _bad_request(message: str, status_code: int = 400):
    return jsonify({"ok": False, "error": message}), status_code


def _find_all_pots() -> List[Dict[str, Any]]:
    res = (
        supabase.table("plant_pots")
        .select("id,name,plant_type,emoji,location,auto_water_enabled,water_threshold,device_id,created_at")
        .order("created_at", desc=True)
        .execute()
    )
    return res.data or []


def _find_pot_by_id(pot_id: str) -> Optional[Dict[str, Any]]:
    res = (
        supabase.table("plant_pots")
        .select("id,user_id,name,device_id")
        .eq("id", pot_id)
        .limit(1)
        .execute()
    )
    rows = res.data or []
    return rows[0] if rows else None


def _build_status(moisture: Optional[float]) -> str:
    if moisture is None:
        return "unknown"
    if moisture < 20:
        return "critical"
    if moisture < 35:
        return "dry"
    if moisture <= 70:
        return "optimal"
    return "wet"


def _resolve_pot_for_message(payload: Dict[str, Any]) -> Optional[str]:
    pot_id = payload.get("pot_id")
    if pot_id:
        return str(pot_id)

    device_mac = payload.get("device_mac")
    if not device_mac:
        return None

    device_res = (
        supabase.table("devices")
        .select("id")
        .eq("device_mac", str(device_mac))
        .limit(1)
        .execute()
    )
    devices = device_res.data or []
    if not devices:
        return None

    device_id = devices[0]["id"]
    pot_res = (
        supabase.table("plant_pots")
        .select("id")
        .eq("device_id", device_id)
        .limit(1)
        .execute()
    )
    pots = pot_res.data or []
    return pots[0]["id"] if pots else None


def _insert_sensor_reading(payload: Dict[str, Any]) -> Dict[str, Any]:
    pot_id = _resolve_pot_for_message(payload)
    if not pot_id:
        raise ValueError("Cannot resolve pot_id from payload (need pot_id or device_mac)")

    moisture = payload.get("moisture")
    temperature = payload.get("temperature")
    light_level = payload.get("light_level")
    recorded_at = payload.get("recorded_at") or _now_iso()

    insert_payload = {
        "pot_id": pot_id,
        "moisture": moisture,
        "temperature": temperature,
        "light_level": light_level,
        "recorded_at": recorded_at,
    }

    res = supabase.table("sensor_readings").insert(insert_payload).execute()
    reading = (res.data or [{}])[0]

    pot = _find_pot_by_id(pot_id)
    if pot and moisture is not None:
        status = _build_status(float(moisture))
        if status in {"dry", "critical"}:
            supabase.table("notifications").insert(
                {
                    "user_id": pot["user_id"],
                    "pot_id": pot_id,
                    "type": status,
                    "title": f"{pot['name']} cần chú ý",
                    "body": f"Độ ẩm hiện tại {float(moisture):.1f}% ({status}).",
                }
            ).execute()

    return reading


class FiwareSocketIngestor:
    def __init__(self):
        self.host = os.environ.get("FIWARE_SOCKET_HOST", "0.0.0.0")
        self.port = int(os.environ.get("FIWARE_SOCKET_PORT", "5055"))
        self._thread: Optional[threading.Thread] = None
        self._stop_event = threading.Event()
        self._lock = threading.Lock()
        self.running = False
        self.last_error: Optional[str] = None
        self.last_message_at: Optional[str] = None
        self.received_count = 0

    def start(self) -> bool:
        with self._lock:
            if self.running:
                return False
            self._stop_event.clear()
            self._thread = threading.Thread(target=self._run, daemon=True)
            self._thread.start()
            self.running = True
            return True

    def stop(self) -> bool:
        with self._lock:
            if not self.running:
                return False
            self._stop_event.set()
            self.running = False
            return True

    def status(self) -> Dict[str, Any]:
        return {
            "running": self.running,
            "host": self.host,
            "port": self.port,
            "received_count": self.received_count,
            "last_message_at": self.last_message_at,
            "last_error": self.last_error,
        }

    def _run(self):
        server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        try:
            server.bind((self.host, self.port))
            server.listen(5)
            server.settimeout(1.0)
            while not self._stop_event.is_set():
                try:
                    conn, _ = server.accept()
                except socket.timeout:
                    continue
                with conn:
                    conn.settimeout(1.0)
                    buffer = ""
                    while not self._stop_event.is_set():
                        try:
                            chunk = conn.recv(4096)
                        except socket.timeout:
                            continue
                        if not chunk:
                            break
                        buffer += chunk.decode("utf-8", errors="ignore")
                        while "\n" in buffer:
                            raw_line, buffer = buffer.split("\n", 1)
                            line = raw_line.strip()
                            if not line:
                                continue
                            self._handle_line(line)
        except Exception as exc:
            self.last_error = str(exc)
        finally:
            try:
                server.close()
            except Exception:
                pass
            with self._lock:
                self.running = False

    def _handle_line(self, line: str):
        try:
            payload = json.loads(line)
            _insert_sensor_reading(payload)
            self.received_count += 1
            self.last_message_at = _now_iso()
        except Exception as exc:
            self.last_error = str(exc)


fiware_ingestor = FiwareSocketIngestor()


@app.get("/")
def index():
    return jsonify(
        {
            "ok": True,
            "service": "plant-sentry-api",
            "time": _now_iso(),
            "routes": [
                "GET /api/health",
                "GET /api/dashboard",
                "GET /api/pots",
                "GET /api/pots/<pot_id>/readings?limit=30",
                "GET /api/notifications?limit=50",
                "PATCH /api/notifications/<id>/read",
                "PATCH /api/notifications/read-all",
                "GET /api/watering-logs?limit=50",
                "POST /api/ingest",
                "POST /api/seed",
                "GET /api/fiware/socket/status",
                "POST /api/fiware/socket/start",
                "POST /api/fiware/socket/stop",
            ],
        }
    )


@app.get("/api/health")
def health_check():
    try:
        res = supabase.table("plant_pots").select("id", count="exact").limit(1).execute()
        return jsonify(
            {
                "ok": True,
                "db": "connected",
                "pots_count_hint": res.count,
                "fiware_socket": fiware_ingestor.status(),
            }
        )
    except Exception as exc:
        return jsonify({"ok": False, "error": str(exc)}), 500


@app.get("/api/pots")
def get_pots():
    pots = _find_all_pots()
    return jsonify({"ok": True, "items": pots, "count": len(pots)})


@app.get("/api/pots/<pot_id>/readings")
def get_pot_readings(pot_id: str):
    limit = request.args.get("limit", "30")
    try:
        limit_num = max(1, min(500, int(limit)))
    except ValueError:
        return _bad_request("Invalid limit, must be integer")

    res = (
        supabase.table("sensor_readings")
        .select("id,pot_id,moisture,temperature,light_level,recorded_at")
        .eq("pot_id", pot_id)
        .order("recorded_at", desc=True)
        .limit(limit_num)
        .execute()
    )
    return jsonify({"ok": True, "items": res.data or [], "count": len(res.data or [])})


@app.get("/api/dashboard")
def get_dashboard_data():
    pots = _find_all_pots()
    pot_ids = [p["id"] for p in pots]

    latest_map: Dict[str, Dict[str, Any]] = {}
    if pot_ids:
        latest_res = (
            supabase.table("latest_sensor_readings")
            .select("pot_id,moisture,temperature,light_level,recorded_at")
            .in_("pot_id", pot_ids)
            .execute()
        )
        latest_map = {row["pot_id"]: row for row in (latest_res.data or [])}

    merged = []
    for pot in pots:
        latest = latest_map.get(pot["id"], {})
        moisture_val = latest.get("moisture")
        merged.append(
            {
                **pot,
                "latest": latest,
                "status": _build_status(float(moisture_val)) if moisture_val is not None else "unknown",
            }
        )

    unread_res = (
        supabase.table("notifications")
        .select("id", count="exact")
        .eq("is_read", False)
        .limit(1)
        .execute()
    )

    return jsonify(
        {
            "ok": True,
            "summary": {
                "total_pots": len(merged),
                "critical": len([p for p in merged if p["status"] == "critical"]),
                "dry": len([p for p in merged if p["status"] == "dry"]),
                "optimal": len([p for p in merged if p["status"] == "optimal"]),
                "unread_notifications": unread_res.count or 0,
            },
            "pots": merged,
        }
    )


@app.get("/api/notifications")
def get_notifications():
    # Family/home scope: public read API with optional filters.
    user_id = request.args.get("user_id")
    pot_id = request.args.get("pot_id")
    is_read_param = request.args.get("is_read")

    limit = request.args.get("limit", "50")
    try:
        limit_num = max(1, min(500, int(limit)))
    except ValueError:
        return _bad_request("Invalid limit, must be integer")

    query = (
        supabase.table("notifications")
        .select("id,user_id,pot_id,type,title,body,is_read,created_at", count="exact")
        .order("created_at", desc=True)
        .limit(limit_num)
    )

    if user_id:
        query = query.eq("user_id", user_id)
    if pot_id:
        query = query.eq("pot_id", pot_id)
    if is_read_param is not None:
        normalized = is_read_param.strip().lower()
        if normalized not in {"true", "false"}:
            return _bad_request("is_read must be true or false")
        query = query.eq("is_read", normalized == "true")

    res = query.execute()
    items = res.data or []

    unread_query = supabase.table("notifications").select("id", count="exact").eq("is_read", False).limit(1)
    if user_id:
        unread_query = unread_query.eq("user_id", user_id)
    if pot_id:
        unread_query = unread_query.eq("pot_id", pot_id)
    unread_count = unread_query.execute().count or 0

    return jsonify(
        {
            "ok": True,
            "count": len(items),
            "total": res.count or len(items),
            "unread": unread_count,
            "items": items,
        }
    )


@app.patch("/api/notifications/<int:notification_id>/read")
def mark_notification_read(notification_id: int):
    body = request.get_json(silent=True) or {}
    is_read = body.get("is_read", True)
    if not isinstance(is_read, bool):
        return _bad_request("is_read must be boolean")

    res = (
        supabase.table("notifications")
        .update({"is_read": is_read})
        .eq("id", notification_id)
        .execute()
    )
    items = res.data or []
    if not items:
        return jsonify({"ok": False, "error": "Notification not found"}), 404

    return jsonify({"ok": True, "item": items[0]})


@app.patch("/api/notifications/read-all")
def mark_all_notifications_read():
    pot_id = request.args.get("pot_id")

    query = supabase.table("notifications").update({"is_read": True})
    if pot_id:
        query = query.eq("pot_id", pot_id)

    res = query.eq("is_read", False).execute()
    updated = res.data or []

    return jsonify({"ok": True, "updated": len(updated), "items": updated})


@app.get("/api/watering-logs")
def get_watering_logs():
    # Family/home scope: public read API with optional filters.
    user_id = request.args.get("user_id")
    pot_id = request.args.get("pot_id")

    limit = request.args.get("limit", "50")
    try:
        limit_num = max(1, min(500, int(limit)))
    except ValueError:
        return _bad_request("Invalid limit, must be integer")

    if user_id and not pot_id:
        user_pots_res = supabase.table("plant_pots").select("id").eq("user_id", user_id).execute()
        user_pot_ids = [p["id"] for p in (user_pots_res.data or [])]
        if not user_pot_ids:
            return jsonify({"ok": True, "count": 0, "total": 0, "items": []})

        logs_res = (
            supabase.table("watering_logs")
            .select(
                "id,pot_id,triggered_by,duration_seconds,moisture_before,moisture_after,created_at",
                count="exact",
            )
            .in_("pot_id", user_pot_ids)
            .order("created_at", desc=True)
            .limit(limit_num)
            .execute()
        )
        logs = logs_res.data or []
    else:
        query = (
            supabase.table("watering_logs")
            .select(
                "id,pot_id,triggered_by,duration_seconds,moisture_before,moisture_after,created_at",
                count="exact",
            )
            .order("created_at", desc=True)
            .limit(limit_num)
        )
        if pot_id:
            query = query.eq("pot_id", pot_id)
        logs_res = query.execute()
        logs = logs_res.data or []

    pot_ids = list({row["pot_id"] for row in logs if row.get("pot_id")})
    pot_meta: Dict[str, Dict[str, Any]] = {}
    if pot_ids:
        pots_res = (
            supabase.table("plant_pots")
            .select("id,name,plant_type,location")
            .in_("id", pot_ids)
            .execute()
        )
        pot_meta = {p["id"]: p for p in (pots_res.data or [])}

    items = [{**row, "pot": pot_meta.get(row.get("pot_id"))} for row in logs]

    return jsonify(
        {
            "ok": True,
            "count": len(items),
            "total": logs_res.count or len(items),
            "items": items,
        }
    )


@app.post("/api/ingest")
def ingest_http():
    payload = request.get_json(silent=True) or {}
    try:
        reading = _insert_sensor_reading(payload)
        return jsonify({"ok": True, "reading": reading})
    except ValueError as exc:
        return _bad_request(str(exc))
    except Exception as exc:
        return jsonify({"ok": False, "error": str(exc)}), 500


@app.post("/api/seed")
def seed_fake_data():
    body = request.get_json(silent=True) or {}
    user_id = FAMILY_USER_ID

    # Ensure family profile row exists (requires FK to auth.users to be dropped
    # in Supabase: ALTER TABLE public.profiles DROP CONSTRAINT profiles_id_fkey;)
    supabase.table("profiles").upsert(
        {"id": user_id, "display_name": "Gia dinh"},
        on_conflict="id",
    ).execute()

    pot_count = int(body.get("pot_count", 5))
    readings_per_pot = int(body.get("readings_per_pot", 24))
    pot_count = max(1, min(30, pot_count))
    readings_per_pot = max(1, min(240, readings_per_pot))

    locations = ["Ban cong", "Phong khach", "Bep", "San thuong"]
    plant_types = ["Monstera", "Snake Plant", "Pothos", "Basil", "Rosemary"]
    emojis = ["🌿", "🪴", "🌱", "🍀", "🌵"]

    devices_payload = []
    for i in range(max(1, pot_count // 2)):
        devices_payload.append(
            {
                "user_id": user_id,
                "device_mac": f"DE:MO:00:00:00:{i+1:02d}",
                "name": f"Demo Sensor {i+1}",
                "location": random.choice(locations),
                "firmware_version": "1.2.0",
                "is_online": True,
                "last_seen": _now_iso(),
            }
        )

    device_rows = supabase.table("devices").upsert(devices_payload).execute().data or []
    device_ids = [d["id"] for d in device_rows]

    pots_payload = []
    for i in range(pot_count):
        pots_payload.append(
            {
                "user_id": user_id,
                "device_id": device_ids[i % len(device_ids)] if device_ids else None,
                "name": f"Chau demo {i+1}",
                "plant_type": random.choice(plant_types),
                "emoji": random.choice(emojis),
                "location": random.choice(locations),
                "auto_water_enabled": random.choice([True, False]),
                "water_threshold": random.choice([25, 30, 35]),
            }
        )

    pot_rows = supabase.table("plant_pots").insert(pots_payload).execute().data or []

    reading_rows: List[Dict[str, Any]] = []
    now = datetime.now(timezone.utc)
    for pot in pot_rows:
        for j in range(readings_per_pot):
            ts = now - timedelta(minutes=10 * j)
            moisture = round(random.uniform(18, 80), 2)
            reading_rows.append(
                {
                    "pot_id": pot["id"],
                    "moisture": moisture,
                    "temperature": round(random.uniform(22, 34), 2),
                    "light_level": round(random.uniform(20, 95), 2),
                    "recorded_at": ts.isoformat(),
                }
            )

    if reading_rows:
        supabase.table("sensor_readings").insert(reading_rows).execute()

    rule_rows = []
    for pot in pot_rows:
        rule_rows.append(
            {
                "pot_id": pot["id"],
                "condition_type": "moisture_below",
                "threshold_value": random.choice([25, 30, 35]),
                "action": random.choice(["water", "water_and_notify", "notify"]),
                "is_active": True,
            }
        )
    if rule_rows:
        supabase.table("automation_rules").insert(rule_rows).execute()

    watering_rows: List[Dict[str, Any]] = []
    for pot in pot_rows:
        moisture_before = round(random.uniform(15, 35), 2)
        moisture_after = min(100.0, round(moisture_before + random.uniform(8, 24), 2))
        watering_rows.append(
            {
                "pot_id": pot["id"],
                "triggered_by": random.choice(["manual", "auto", "scheduled"]),
                "duration_seconds": random.randint(8, 35),
                "moisture_before": moisture_before,
                "moisture_after": moisture_after,
                "created_at": _now_iso(),
            }
        )
    if watering_rows:
        supabase.table("watering_logs").insert(watering_rows).execute()

    return jsonify(
        {
            "ok": True,
            "seeded": {
                "devices": len(device_rows),
                "pots": len(pot_rows),
                "sensor_readings": len(reading_rows),
                "automation_rules": len(rule_rows),
                "watering_logs": len(watering_rows),
            },
        }
    )


@app.get("/api/fiware/socket/status")
def fiware_socket_status():
    return jsonify({"ok": True, "socket": fiware_ingestor.status()})


@app.post("/api/fiware/socket/start")
def fiware_socket_start():
    started = fiware_ingestor.start()
    return jsonify({"ok": True, "started": started, "socket": fiware_ingestor.status()})


@app.post("/api/fiware/socket/stop")
def fiware_socket_stop():
    stopped = fiware_ingestor.stop()
    return jsonify({"ok": True, "stopped": stopped, "socket": fiware_ingestor.status()})


if __name__ == "__main__":
    auto_start_socket = os.environ.get("FIWARE_SOCKET_AUTOSTART", "false").lower() == "true"
    if auto_start_socket:
        fiware_ingestor.start()

    app.run(host="0.0.0.0", port=int(os.environ.get("PORT", "5000")), debug=True)