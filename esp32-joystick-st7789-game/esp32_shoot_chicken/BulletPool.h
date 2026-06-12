#pragma once
#include <lvgl.h>
#include "Config.h"

// ── BulletPool ────────────────────────────────────────────────────
// Chịu trách nhiệm:
//   - Pool cố định MAX_BULLETS slot
//   - Spawn đạn (fire) với offset cho multishot/shotgun
//   - Update vị trí mỗi tick (dirty flag trên Y)
//   - Ẩn khi ra ngoài màn hình
// Debug tip: Serial.printf trong fire() để xem số đạn spawn được
struct BulletData {
  lv_obj_t* obj    = nullptr;
  int x = 0, y = 0;
  int prevX = -999, prevY = -999;  // dirty flag
  int damage  = 1;
  bool active = false;
};

class BulletPool {
public:
  BulletData bullets[MAX_BULLETS];

  // Tạo LVGL objects — gọi 1 lần sau display init
  void createObjects() {
    for (int i = 0; i < MAX_BULLETS; i++) {
      bullets[i].obj = lv_obj_create(lv_scr_act());
      lv_obj_set_size(bullets[i].obj, BULLET_W, BULLET_H);
      lv_obj_set_style_bg_color(bullets[i].obj, lv_color_make(250,204,21), 0);
      lv_obj_set_style_radius(bullets[i].obj, 0, 0);
      lv_obj_set_style_border_width(bullets[i].obj, 0, 0);
      lv_obj_add_flag(bullets[i].obj, LV_OBJ_FLAG_HIDDEN);
      bullets[i].active = false;
      bullets[i].prevX  = -999;
      bullets[i].prevY  = -999;
    }
  }

  // Reset tất cả đạn (khi start game)
  void resetAll() {
    for (int i = 0; i < MAX_BULLETS; i++) {
      bullets[i].active = false;
      bullets[i].prevX = -999; bullets[i].prevY = -999;
      lv_obj_add_flag(bullets[i].obj, LV_OBJ_FLAG_HIDDEN);
    }
  }

  // Bắn đạn — gunX: vị trí gun, g: gun stats, multishot: đang bật?
  void fire(float gunX, const GunStats& g, int selectedGun, bool multishotActive) {
    static const int off3[3] = {-10, 0, 10};
    static const int off5[5] = {-20, -10, 0, 10, 20};
    int n = multishotActive ? 5 : g.numBullets;

    // Màu + kích thước theo loại súng (tính 1 lần)
    lv_color_t bc;
    int bw = BULLET_W, bh = BULLET_H;
    if (multishotActive) {
      bc = lv_color_make(192,132,252);
    } else {
      switch (selectedGun) {
        case GUN_FAST:    bc = lv_color_make(56,189,248);  break;
        case GUN_HEAVY:   bc = lv_color_make(239,68,68); bw=6; bh=12; break;
        case GUN_SHOTGUN: bc = lv_color_make(168,85,247); break;
        default:          bc = lv_color_make(250,204,21); break;
      }
    }

    int fired = 0;
    for (int i = 0; i < MAX_BULLETS && fired < n; i++) {
      if (bullets[i].active) continue;
      int off = multishotActive ? off5[fired] : (n == 3 ? off3[fired] : 0);
      bullets[i].x    = (int)gunX + GUN_WIDTH/2 - BULLET_W/2 + off;
      bullets[i].y    = GUN_Y - BULLET_H;
      bullets[i].prevX = bullets[i].x;
      bullets[i].prevY = bullets[i].y;
      bullets[i].damage  = g.damage;
      bullets[i].active  = true;
      lv_obj_set_size(bullets[i].obj, bw, bh);
      lv_obj_set_style_bg_color(bullets[i].obj, bc, 0);
      lv_obj_set_pos(bullets[i].obj, bullets[i].x, bullets[i].y);
      lv_obj_clear_flag(bullets[i].obj, LV_OBJ_FLAG_HIDDEN);
      fired++;
    }
    Serial.printf("[BulletPool] Fired %d/%d bullets\n", fired, n);
  }

  // Update mỗi tick: di chuyển + dirty flag + ẩn khi ra ngoài
  void update() {
    for (int i = 0; i < MAX_BULLETS; i++) {
      if (!bullets[i].active) continue;
      bullets[i].y -= BULLET_SPEED;
      if (bullets[i].y < -BULLET_H) {
        bullets[i].active = false;
        lv_obj_add_flag(bullets[i].obj, LV_OBJ_FLAG_HIDDEN);
      } else if (bullets[i].y != bullets[i].prevY) {
        bullets[i].prevY = bullets[i].y;
        lv_obj_set_pos(bullets[i].obj, bullets[i].x, bullets[i].y);
      }
    }
  }

  void hideAll() {
    for (int i = 0; i < MAX_BULLETS; i++)
      lv_obj_add_flag(bullets[i].obj, LV_OBJ_FLAG_HIDDEN);
  }
};
