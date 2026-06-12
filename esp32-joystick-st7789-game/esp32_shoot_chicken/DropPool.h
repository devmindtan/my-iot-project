#pragma once
#include <lvgl.h>
#include "Config.h"

// ═══════════════════════════════════════════════════════════════════
// ── DropPool ──────────────────────────────────────────────────────
// Chịu trách nhiệm: spawn drop từ enemy chết, rơi xuống, va chạm gun
// Debug tip: apply_drop log ở apply(); nếu drop không xuất hiện → hết slot
// ═══════════════════════════════════════════════════════════════════
struct DropData {
  lv_obj_t* obj = nullptr;
  int x = 0, y = 0, prevY = -999;
  DropType type   = DROP_NONE;
  bool     active = false;
};

class DropPool {
public:
  DropData drops[MAX_DROPS];

  void createObjects() {
    for (int i = 0; i < MAX_DROPS; i++) {
      drops[i].obj = lv_obj_create(lv_scr_act());
      lv_obj_set_size(drops[i].obj, DROP_SIZE, DROP_SIZE);
      lv_obj_set_style_radius(drops[i].obj, LV_RADIUS_CIRCLE, 0);
      lv_obj_set_style_border_width(drops[i].obj, 2, 0);
      lv_obj_set_style_border_color(drops[i].obj, lv_color_make(255,255,255), 0);
      lv_obj_add_flag(drops[i].obj, LV_OBJ_FLAG_HIDDEN);
      drops[i].active = false;
      drops[i].prevY  = -999;
    }
  }

  void resetAll() {
    for (int i = 0; i < MAX_DROPS; i++) {
      drops[i].active = false; drops[i].prevY = -999;
      lv_obj_add_flag(drops[i].obj, LV_OBJ_FLAG_HIDDEN);
    }
  }

  void spawn(int x, int y, DropType type) {
    for (int i = 0; i < MAX_DROPS; i++) {
      if (drops[i].active) continue;
      drops[i].x = x; drops[i].y = drops[i].prevY = y;
      drops[i].type   = type;
      drops[i].active = true;

      lv_color_t c;
      switch (type) {
        case DROP_HP:        c = lv_color_make(239,68,68);   break;
        case DROP_RAPID:     c = lv_color_make(56,189,248);  break;
        case DROP_SHIELD:    c = lv_color_make(34,197,94);   break;
        case DROP_BOMB:      c = lv_color_make(251,146,60);  break;
        case DROP_LASER:     c = lv_color_make(250,204,21);  break;
        case DROP_MULTISHOT: c = lv_color_make(192,132,252); break;
        case DROP_MAGNET:    c = lv_color_make(251,191,36);  break;
        default:             c = lv_color_make(200,200,200); break;
      }
      lv_obj_set_style_bg_color(drops[i].obj, c, 0);
      lv_obj_set_pos(drops[i].obj, x, y);
      lv_obj_clear_flag(drops[i].obj, LV_OBJ_FLAG_HIDDEN);
      return;
    }
    Serial.println("[DropPool] No free slot!");
  }

  // Update + trả về index drop va chạm gun (-1 nếu không có)
  int update(float gunX) {
    for (int i = 0; i < MAX_DROPS; i++) {
      if (!drops[i].active) continue;
      drops[i].y += DROP_SPEED;
      if (drops[i].y > 240) {
        drops[i].active = false;
        lv_obj_add_flag(drops[i].obj, LV_OBJ_FLAG_HIDDEN);
        continue;
      }
      if (checkCollision((int)gunX, GUN_Y, GUN_WIDTH, GUN_HEIGHT,
                          drops[i].x, drops[i].y, DROP_SIZE, DROP_SIZE)) {
        drops[i].active = false;
        lv_obj_add_flag(drops[i].obj, LV_OBJ_FLAG_HIDDEN);
        return i;   // caller dùng drops[i].type để xử lý
      }
      if (drops[i].y != drops[i].prevY) {
        drops[i].prevY = drops[i].y;
        lv_obj_set_pos(drops[i].obj, drops[i].x, drops[i].y);
      }
    }
    return -1;
  }

  // Magnet: kéo tất cả drop về phía gun
  void magnetPull() {
    for (int i = 0; i < MAX_DROPS; i++)
      if (drops[i].active && drops[i].type != DROP_MAGNET)
        drops[i].y = GUN_Y;
  }

  void hideAll() {
    for (int i = 0; i < MAX_DROPS; i++)
      lv_obj_add_flag(drops[i].obj, LV_OBJ_FLAG_HIDDEN);
  }
};


// ═══════════════════════════════════════════════════════════════════
// ── ParticlePool ──────────────────────────────────────────────────
// Hình vuông 3x3, int-math (x10), không float, không gravity object
// Debug tip: nếu particle không xuất hiện → hết slot (tăng MAX_PARTICLES)
// ═══════════════════════════════════════════════════════════════════
struct ParticleData {
  lv_obj_t* obj = nullptr;
  int x=0, y=0, prevX=-999, prevY=-999;
  int vx10=0, vy10=0;
  int  life   = 0;
  bool active = false;
};

class ParticlePool {
public:
  ParticleData particles[MAX_PARTICLES];

  void createObjects() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
      particles[i].obj = lv_obj_create(lv_scr_act());
      lv_obj_set_size(particles[i].obj, 3, 3);
      lv_obj_set_style_radius(particles[i].obj, 0, 0);
      lv_obj_set_style_border_width(particles[i].obj, 0, 0);
      lv_obj_add_flag(particles[i].obj, LV_OBJ_FLAG_HIDDEN);
      particles[i].active = false;
      particles[i].prevX  = -999;
      particles[i].prevY  = -999;
    }
  }

  void resetAll() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
      particles[i].active = false;
      particles[i].prevX = -999; particles[i].prevY = -999;
      lv_obj_add_flag(particles[i].obj, LV_OBJ_FLAG_HIDDEN);
    }
  }

  void spawn(int cx, int cy, lv_color_t color, int count) {
    int spawned = 0;
    for (int i = 0; i < MAX_PARTICLES && spawned < count; i++) {
      if (particles[i].active) continue;
      particles[i].x = particles[i].prevX = cx;
      particles[i].y = particles[i].prevY = cy;
      particles[i].vx10   = random(-22, 22);
      particles[i].vy10   = random(-25, 4);
      particles[i].life   = random(4, 8);
      particles[i].active = true;
      lv_obj_set_style_bg_color(particles[i].obj, color, 0);
      lv_obj_set_pos(particles[i].obj, cx, cy);
      lv_obj_clear_flag(particles[i].obj, LV_OBJ_FLAG_HIDDEN);
      spawned++;
    }
  }

  void update() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
      if (!particles[i].active) continue;
      particles[i].vy10 += 4;   // gravity *10
      particles[i].x    += particles[i].vx10 / 10;
      particles[i].y    += particles[i].vy10 / 10;
      if (--particles[i].life <= 0 || particles[i].y > 248) {
        particles[i].active = false;
        lv_obj_add_flag(particles[i].obj, LV_OBJ_FLAG_HIDDEN);
      } else if (particles[i].x != particles[i].prevX ||
                 particles[i].y != particles[i].prevY) {
        particles[i].prevX = particles[i].x;
        particles[i].prevY = particles[i].y;
        lv_obj_set_pos(particles[i].obj, particles[i].x, particles[i].y);
      }
    }
  }

  void hideAll() {
    for (int i = 0; i < MAX_PARTICLES; i++)
      lv_obj_add_flag(particles[i].obj, LV_OBJ_FLAG_HIDDEN);
  }
};


// ═══════════════════════════════════════════════════════════════════
// ── FlashPool ─────────────────────────────────────════════════════
// Flash trắng 7x7, life=2 (ít frame dirty)
// Debug tip: nếu flash không hiện → hết slot (tăng MAX_FLASHES)
// ═══════════════════════════════════════════════════════════════════
struct FlashData {
  lv_obj_t* obj = nullptr;
  int  life   = 0;
  bool active = false;
};

class FlashPool {
public:
  FlashData flashes[MAX_FLASHES];

  void createObjects() {
    for (int i = 0; i < MAX_FLASHES; i++) {
      flashes[i].obj = lv_obj_create(lv_scr_act());
      lv_obj_set_size(flashes[i].obj, 7, 7);
      lv_obj_set_style_radius(flashes[i].obj, 0, 0);
      lv_obj_set_style_border_width(flashes[i].obj, 0, 0);
      lv_obj_set_style_bg_color(flashes[i].obj, lv_color_make(255,255,255), 0);
      lv_obj_add_flag(flashes[i].obj, LV_OBJ_FLAG_HIDDEN);
      flashes[i].active = false;
    }
  }

  void resetAll() {
    for (int i = 0; i < MAX_FLASHES; i++) {
      flashes[i].active = false;
      lv_obj_add_flag(flashes[i].obj, LV_OBJ_FLAG_HIDDEN);
    }
  }

  void spawn(int cx, int cy) {
    for (int i = 0; i < MAX_FLASHES; i++) {
      if (!flashes[i].active) {
        flashes[i].life   = 2;
        flashes[i].active = true;
        lv_obj_set_pos(flashes[i].obj, cx-4, cy-4);
        lv_obj_clear_flag(flashes[i].obj, LV_OBJ_FLAG_HIDDEN);
        return;
      }
    }
  }

  void update() {
    for (int i = 0; i < MAX_FLASHES; i++) {
      if (flashes[i].active && --flashes[i].life <= 0) {
        flashes[i].active = false;
        lv_obj_add_flag(flashes[i].obj, LV_OBJ_FLAG_HIDDEN);
      }
    }
  }

  void hideAll() {
    for (int i = 0; i < MAX_FLASHES; i++)
      lv_obj_add_flag(flashes[i].obj, LV_OBJ_FLAG_HIDDEN);
  }
};
