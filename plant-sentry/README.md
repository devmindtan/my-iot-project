# 🌱 PlantSentry — Smart Plant Monitoring System

PlantSentry là hệ thống IoT giám sát và chăm sóc cây trồng thông minh. Kết hợp giữa phần cứng ESP32, server Python và ứng dụng Flutter để theo dõi độ ẩm, nhiệt độ, ánh sáng và điều khiển tưới nước từ xa.

---

## 🏗️ Kiến trúc hệ thống

```
┌─────────────────┐        HTTP/JSON         ┌─────────────────┐
│   ESP32 + Sensor│  ──────────────────────▶  │  Python Server  │
│  (Firmware C++)  │                          │  (Flask API)    │
└─────────────────┘                          └────────┬────────┘
                                                      │
                                              Dữ liệu cảm biến
                                                      │
                                             ┌────────▼────────┐
                                             │  Flutter App    │
                                             │  (iOS/Android)  │
                                             └─────────────────┘
```

---

## 📁 Cấu trúc dự án

```
plant-sentry/
├── app/                        # Flutter mobile app
│   └── lib/
│       ├── main.dart           # Entry point
│       ├── app.dart            # MaterialApp + theme config
│       ├── theme/
│       │   └── app_colors.dart # Bảng màu chung
│       ├── models/
│       │   └── plant_pot.dart  # Model PlantPot, MoistureStatus
│       ├── data/
│       │   └── sample_data.dart # Dữ liệu mẫu
│       ├── screens/
│       │   ├── home_screen.dart          # Shell điều hướng chính
│       │   ├── dashboard_screen.dart     # Tổng quan — filter, tìm kiếm, grid/list
│       │   ├── add_plant_screen.dart     # Thêm chậu cây mới
│       │   ├── plant_detail_screen.dart  # Chi tiết từng chậu cây
│       │   ├── notifications_screen.dart # Màn hình thông báo
│       │   └── settings_screen.dart      # Màn hình cài đặt
│       └── widgets/
│           ├── plant_pot_card.dart        # Card dạng list
│           ├── plant_pot_grid_card.dart   # Card compact dạng grid (20+ chậu)
│           ├── summary_row.dart           # Thống kê tổng quan
│           ├── alert_banner.dart          # Banner cảnh báo
│           ├── moisture_meter.dart        # Đồng hồ đo độ ẩm (arc)
│           ├── moisture_chart.dart        # Biểu đồ lịch sử 7 ngày
│           └── future_features_banner.dart # Tính năng sắp ra mắt
├── firmware/
│   └── event_esp32.ino         # Code Arduino cho ESP32
├── server/
│   └── emit_esp32.py           # Flask server nhận dữ liệu từ ESP32
├── deployments/                # Cấu hình triển khai
└── docs/                       # Tài liệu bổ sung
```

---

## 📱 Flutter App

### Tính năng hiện tại

| Màn hình          | Mô tả                                                                     |
| ----------------- | ------------------------------------------------------------------------- |
| **Tổng quan**     | Filter theo trạng thái, tìm kiếm, chế độ lưới/danh sách, nhóm theo vị trí |
| **Thêm chậu**     | Form thêm chậu mới — emoji picker, loại cây, vị trí, tưới tự động         |
| **Chi tiết chậu** | Đồng hồ độ ẩm, biểu đồ 7 ngày, điều khiển tưới thủ công/tự động           |
| **Thông báo**     | Lịch sử cảnh báo, đánh dấu đã đọc                                         |
| **Cài đặt**       | Ngưỡng tưới, thông báo đẩy, kết nối thiết bị IoT                          |

### Tính năng sắp ra mắt

- 📹 **Camera theo dõi 24/7** — Xem trực tiếp + AI nhận diện bệnh cây
- 🧠 **AI chẩn đoán** — Phân tích ảnh và đưa lời khuyên tức thì
- 🔔 **Cảnh báo thông minh** — Dựa theo thời tiết và mùa vụ
- 🌐 **Quản lý đa chậu** — Lên đến 50 thiết bị cùng lúc

### Chạy ứng dụng

```bash
cd app
flutter pub get
flutter run
```

---

## 🔧 Firmware (ESP32)

**File:** `firmware/event_esp32.ino`

ESP32 đọc nhiệt độ từ cảm biến DS18B20 và gửi dữ liệu lên server qua HTTP POST mỗi 3 giây.

### Cần thay đổi trước khi nạp

```cpp
const char* ssid = "TEN_WIFI";
const char* password = "MAT_KHAU_WIFI";
const char* serverName = "http://<IP_SERVER>:5000/data";
```

### Thư viện Arduino cần cài

- `WiFi.h` (built-in ESP32)
- `HTTPClient.h` (built-in ESP32)
- `OneWire` by Paul Stoffregen
- `DallasTemperature` by Miles Burton

### Sơ đồ kết nối phần cứng

| ESP32 Pin | Thiết bị                                |
| --------- | --------------------------------------- |
| GPIO 4    | DS18B20 Data (+ 4.7kΩ pull-up lên 3.3V) |
| 3.3V      | VCC cảm biến                            |
| GND       | GND cảm biến                            |

---

## 🖥️ Server (Python Flask)

**File:** `server/emit_esp32.py`

Server nhận dữ liệu JSON từ ESP32 và log ra console. Đây là điểm trung gian để sau này mở rộng lưu vào database.

### Cài đặt và chạy

```bash
pip install flask
python server/emit_esp32.py
```

Server lắng nghe tại `http://0.0.0.0:5000`.

### API Endpoint

**POST** `/data`

```json
{
  "device_id": "AABBCCDD1122",
  "temperature": 27.5
}
```

**Response:**

```json
{ "status": "ok" }
```

---

## 🚀 Lộ trình phát triển

- [x] Phần cứng ESP32 + cảm biến nhiệt độ
- [x] Server Flask nhận dữ liệu
- [x] Flutter app — giao diện tổng quan
- [x] Chi tiết chậu cây, biểu đồ độ ẩm
- [x] Màn hình thông báo & cài đặt
- [x] Dashboard hỗ trợ 20+ chậu — filter, tìm kiếm, grid/list, nhóm vị trí
- [x] Màn hình thêm chậu cây mới
- [x] Schema Supabase (profiles, devices, plant_pots, sensor_readings, watering_logs, automation_rules, notifications)
- [ ] Tích hợp cảm biến độ ẩm đất thực tế
- [ ] Kết nối Supabase — thay sample data bằng dữ liệu thực
- [ ] Kết nối real-time Flutter ↔ Server (WebSocket / MQTT)
- [ ] Module điều khiển van tưới tự động
- [ ] Camera AI nhận diện sức khỏe cây
- [ ] Triển khai production (Docker + cloud)

---

## 🛠️ Yêu cầu kỹ thuật

| Thành phần | Công nghệ                                   |
| ---------- | ------------------------------------------- |
| Mobile App | Flutter 3.x, Dart 3.x, Material 3           |
| Firmware   | Arduino C++ (ESP32)                         |
| Backend    | Python 3.x, Flask                           |
| Hardware   | ESP32 DevKit, DS18B20, Soil Moisture Sensor |

---

## 📄 License

MIT License — Xem file [LICENSE](LICENSE) để biết thêm chi tiết.
