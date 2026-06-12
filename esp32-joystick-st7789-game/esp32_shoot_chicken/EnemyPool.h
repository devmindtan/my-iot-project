#pragma once
#include <lvgl.h>
#include "Config.h"

// ── EnemyData ─────────────────────────────────────────────────────
struct EnemyData {
  lv_obj_t* obj      = nullptr;
  lv_obj_t* hpBarBg  = nullptr;
  lv_obj_t* hpBarFill = nullptr;

  float x = 0, y = 0;
  float speed  = 1.0f;
  float zigDir = 1.0f;
  int   size   = 20;
  int   hp = 1, maxHp = 1;

  int prevHp   = -1;    // dirty flag HP bar update
  int prevBarX = -999;  // dirty flag bar position
  int prevBarY = -999;

  bool      active      = false;
  EnemyType type        = ENEMY_CHICKEN;
  bool      flashActive = false;
  int       flashTimer  = 0;
  uint8_t   cr = 200, cg = 200, cb = 200;  // cache màu gốc
};

// ── EnemyPool ─────────────────────────────────────────────────────
// Chịu trách nhiệm:
//   - Pool cố định MAX_ENEMIES slot
//   - Spawn enemy theo score/roll
//   - HP bar (tách move/update để tối ưu dirty flag)
//   - Flash khi bị bắn
//   - Ẩn khi chết hoặc rơi đáy
// Debug tip: in enemy[i].hp và type trong spawn_/hide_ để trace lifecycle
class EnemyPool {
public:
  EnemyData enemies[MAX_ENEMIES];

  void createObjects() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
      // Body
      enemies[i].obj = lv_obj_create(lv_scr_act());
      lv_obj_set_style_radius(enemies[i].obj, LV_RADIUS_CIRCLE, 0);
      lv_obj_set_style_border_width(enemies[i].obj, 2, 0);
      lv_obj_set_style_border_color(enemies[i].obj, lv_color_make(255,255,255), 0);
      lv_obj_add_flag(enemies[i].obj, LV_OBJ_FLAG_HIDDEN);
      enemies[i].active  = false;
      enemies[i].prevHp  = -1;
      enemies[i].prevBarX = -999;
      enemies[i].prevBarY = -999;

      // HP bar background
      enemies[i].hpBarBg = lv_obj_create(lv_scr_act());
      lv_obj_set_style_bg_color(enemies[i].hpBarBg, lv_color_make(55,65,81), 0);
      lv_obj_set_style_border_width(enemies[i].hpBarBg, 0, 0);
      lv_obj_set_style_radius(enemies[i].hpBarBg, 0, 0);
      lv_obj_add_flag(enemies[i].hpBarBg, LV_OBJ_FLAG_HIDDEN);

      // HP bar fill
      enemies[i].hpBarFill = lv_obj_create(lv_scr_act());
      lv_obj_set_style_bg_color(enemies[i].hpBarFill, lv_color_make(34,197,94), 0);
      lv_obj_set_style_border_width(enemies[i].hpBarFill, 0, 0);
      lv_obj_set_style_radius(enemies[i].hpBarFill, 0, 0);
      lv_obj_add_flag(enemies[i].hpBarFill, LV_OBJ_FLAG_HIDDEN);
    }
  }

  void resetAll() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
      enemies[i].active = false;
      enemies[i].prevHp = -1;
      enemies[i].prevBarX = -999; enemies[i].prevBarY = -999;
      lv_obj_add_flag(enemies[i].obj,       LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(enemies[i].hpBarBg,   LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(enemies[i].hpBarFill, LV_OBJ_FLAG_HIDDEN);
    }
  }

  // Spawn 1 enemy vào slot trống
  // Debug tip: nếu không spawn được → tất cả slot đang active
  bool spawn(int score) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
      if (enemies[i].active) continue;
      _initEnemy(i, score);
      return true;
    }
    Serial.println("[EnemyPool] No free slot!");
    return false;
  }

  // Ẩn enemy (khi chết hoặc rơi đáy)
  void hide(int i) {
    enemies[i].active = false;
    lv_obj_add_flag(enemies[i].obj,       LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(enemies[i].hpBarBg,   LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(enemies[i].hpBarFill, LV_OBJ_FLAG_HIDDEN);
    Serial.printf("[EnemyPool] Enemy %d hidden\n", i);
  }

  // Cập nhật flash timer (restore màu gốc từ cache)
  void updateFlash(int i) {
    if (enemies[i].flashActive && --enemies[i].flashTimer <= 0) {
      enemies[i].flashActive = false;
      lv_obj_set_style_bg_color(enemies[i].obj,
        lv_color_make(enemies[i].cr, enemies[i].cg, enemies[i].cb), 0);
    }
  }

  // Trigger hit flash (trắng)
  void triggerFlash(int i) {
    lv_obj_set_style_bg_color(enemies[i].obj, lv_color_make(255,255,255), 0);
    enemies[i].flashActive = true;
    enemies[i].flashTimer  = 3;
  }

  // Di chuyển HP bar (chỉ set_pos — không cập nhật size/màu)
  void moveHpBar(int i) {
    int barX = (int)enemies[i].x;
    int barY = (int)enemies[i].y + enemies[i].size + 2;
    if (barX == enemies[i].prevBarX && barY == enemies[i].prevBarY) return;
    enemies[i].prevBarX = barX;
    enemies[i].prevBarY = barY;
    lv_obj_set_pos(enemies[i].hpBarBg,   barX, barY);
    lv_obj_set_pos(enemies[i].hpBarFill, barX, barY);
  }

  // Cập nhật HP bar đầy đủ (size + màu + pos) — chỉ khi HP thực sự đổi
  void updateHpBar(int i) {
    if (enemies[i].hp == enemies[i].prevHp) { moveHpBar(i); return; }
    enemies[i].prevHp = enemies[i].hp;

    int barW  = enemies[i].size;
    int fillW = (enemies[i].hp * barW) / enemies[i].maxHp;
    if (fillW < 1) fillW = 1;

    int barX = (int)enemies[i].x;
    int barY = (int)enemies[i].y + enemies[i].size + 2;
    enemies[i].prevBarX = barX; enemies[i].prevBarY = barY;

    lv_obj_set_size(enemies[i].hpBarBg,   barW, 3);
    lv_obj_set_pos(enemies[i].hpBarBg,    barX, barY);
    lv_obj_set_size(enemies[i].hpBarFill, fillW, 3);
    lv_obj_set_pos(enemies[i].hpBarFill,  barX, barY);

    int ratio = (enemies[i].hp * 100) / enemies[i].maxHp;
    lv_color_t c = (ratio > 60) ? lv_color_make(34,197,94) :
                   (ratio > 30) ? lv_color_make(250,204,21) :
                                  lv_color_make(239,68,68);
    lv_obj_set_style_bg_color(enemies[i].hpBarFill, c, 0);
    lv_obj_clear_flag(enemies[i].hpBarBg,   LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(enemies[i].hpBarFill, LV_OBJ_FLAG_HIDDEN);
  }

  void hideAll() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
      lv_obj_add_flag(enemies[i].obj,       LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(enemies[i].hpBarBg,   LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(enemies[i].hpBarFill, LV_OBJ_FLAG_HIDDEN);
    }
  }

private:
  void _initEnemy(int i, int score) {
    EnemyData& e = enemies[i];

    // Xác định loại theo score + random
    int roll = random(0, 100);
    e.type = ENEMY_CHICKEN;
    if (score >= 5) {
      if      (roll < 20 && score >= 10) e.type = ENEMY_BOSS;
      else if (roll < 45 && score >= 7)  e.type = ENEMY_ZIGZAG;
      else if (roll < 65 && score >= 4)  e.type = ENEMY_TANK;
    }

    // Kích thước
    switch (e.type) {
      case ENEMY_TANK:   e.size = random(26, 36); break;
      case ENEMY_BOSS:   e.size = random(38, 46); break;
      case ENEMY_ZIGZAG: e.size = random(18, 26); break;
      default:           e.size = random(18, 30); break;
    }

    e.x = (float)random(0, 240 - e.size);
    e.y = (float)(-e.size);
    e.zigDir = (random(0,2) == 0) ? 1.0f : -1.0f;
    e.flashActive = false; e.flashTimer = 0;
    e.prevBarX = -999; e.prevBarY = -999;

    // Tốc độ
    float bonus = score / 30.0f;
    float spd   = 0.4f + (random(0,8) / 10.0f) + bonus;
    if (e.type == ENEMY_BOSS)   spd *= 0.4f;
    if (e.type == ENEMY_TANK)   spd *= 0.6f;
    if (e.type == ENEMY_ZIGZAG) spd *= 1.1f;
    e.speed = constrain(spd, 0.1f, 2.5f);

    // HP
    int baseMin = constrain(2 + score/8, 2, 6);
    int baseMax = constrain(4 + score/5, 4, 10);
    switch (e.type) {
      case ENEMY_BOSS:
        e.maxHp = constrain(12 + score/3, 12, 25); break;
      case ENEMY_TANK:
        e.maxHp = constrain(baseMax + random(0,3), 0, 14); break;
      case ENEMY_ZIGZAG:
        e.maxHp = baseMin + random(0,2); break;
      default:
        e.maxHp = random(baseMin, baseMax+1); break;
    }
    e.hp = e.maxHp; e.prevHp = -1;

    // Màu theo loại
    uint8_t r, g, b;
    switch (e.type) {
      case ENEMY_CHICKEN:
        if      (e.maxHp <= 3) { r=251; g=191; b=36;  }
        else if (e.maxHp <= 6) { r=249; g=115; b=22;  }
        else                   { r=220; g=38;  b=38;  }
        lv_obj_set_style_radius(e.obj, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(e.obj, 2, 0);
        lv_obj_set_style_border_color(e.obj, lv_color_make(255,255,255), 0);
        break;
      case ENEMY_TANK:
        r=52; g=211; b=153;
        lv_obj_set_style_radius(e.obj, 4, 0);
        lv_obj_set_style_border_width(e.obj, 2, 0);
        lv_obj_set_style_border_color(e.obj, lv_color_make(200,255,230), 0);
        break;
      case ENEMY_BOSS:
        r=180; g=0; b=60;
        lv_obj_set_style_radius(e.obj, 6, 0);
        lv_obj_set_style_border_width(e.obj, 3, 0);
        lv_obj_set_style_border_color(e.obj, lv_color_make(255,100,100), 0);
        break;
      case ENEMY_ZIGZAG:
        r=168; g=85; b=247;
        lv_obj_set_style_radius(e.obj, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(e.obj, 2, 0);
        lv_obj_set_style_border_color(e.obj, lv_color_make(220,180,255), 0);
        break;
      default: r=200; g=200; b=200; break;
    }
    e.cr = r; e.cg = g; e.cb = b;
    lv_obj_set_style_bg_color(e.obj, lv_color_make(r,g,b), 0);
    lv_obj_set_size(e.obj, e.size, e.size);
    lv_obj_set_pos(e.obj, (int)e.x, (int)e.y);
    lv_obj_clear_flag(e.obj, LV_OBJ_FLAG_HIDDEN);
    e.active = true;

    Serial.printf("[EnemyPool] Spawn type=%d size=%d hp=%d\n", e.type, e.size, e.maxHp);
    updateHpBar(i);
  }
};
