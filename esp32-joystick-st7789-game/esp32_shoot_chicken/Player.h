#pragma once
#include <lvgl.h>
#include "Config.h"
#include "UIManager.h"

// ── Player ────────────────────────────────────────────────────────
// Chịu trách nhiệm:
//   - Di chuyển gun theo joystick
//   - Quản lý HP, shield, rapid, multishot
//   - Nhận sát thương
//   - Fire bullet (BulletPool xử lý object, Player quyết định khi nào bắn)
// Debug tip: gunX/gunPxLast = vị trí; rapidActive/shieldActive = powerup state
class Player {
public:
  // ── Thuộc tính vị trí ──────────────────────────────────────────
  float gunX      = 105.0f;
  int   gunPxLast = -1;       // dirty flag: tránh set_pos thừa

  // ── Joystick calibration ───────────────────────────────────────
  int centerX = 0, centerY = 0;

  // ── HP ─────────────────────────────────────────────────────────
  int hp = PLAYER_MAX_HP;

  // ── Powerup state ──────────────────────────────────────────────
  bool          rapidActive      = false;
  unsigned long rapidEndTime     = 0;
  bool          shieldActive     = false;
  bool          multishotActive  = false;
  unsigned long multishotEndTime = 0;
  int           laserFlashTimer  = 0;

  // ── Gun stats ──────────────────────────────────────────────────
  int selectedGun = GUN_NORMAL;

  static const char* gunNames[GUN_COUNT];
  static GunStats    gunStats[GUN_COUNT];

  // ── Flash effect ───────────────────────────────────────────────
  int gunFlashTimer = 0;

  // ── Debounce bắn ──────────────────────────────────────────────
  unsigned long lastShotTime = 0;
  int           shotInterval = 400;

  // ── LVGL object (gán từ UIManager) ────────────────────────────
  lv_obj_t* gunObj      = nullptr;
  lv_obj_t* shieldIcon  = nullptr;  // từ UIManager
  lv_obj_t* rapidIcon   = nullptr;
  lv_obj_t* multishotIcon = nullptr;
  lv_obj_t* laserObj    = nullptr;

  // Calibrate joystick center — gọi trong setup()
  void calibrateJoystick() {
    long sumX = 0, sumY = 0;
    for (int i = 0; i < 50; i++) {
      sumX += analogRead(JOYSTICK_X_PIN);
      sumY += analogRead(JOYSTICK_Y_PIN);
      delay(5);
    }
    centerX = sumX / 50;
    centerY = sumY / 50;
    Serial.printf("[Player] Joystick center: X=%d Y=%d\n", centerX, centerY);
  }

  // Tạo LVGL gun object — gọi sau UIManager::createAll()
  void createGunObj() {
    gunObj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(gunObj, GUN_WIDTH, GUN_HEIGHT);
    lv_obj_set_style_bg_color(gunObj, lv_color_make(34,197,94), 0);
    lv_obj_set_style_radius(gunObj, 4, 0);
    lv_obj_set_style_border_width(gunObj, 0, 0);
    lv_obj_set_pos(gunObj, (int)gunX, GUN_Y);
  }

  // Reset về trạng thái đầu game
  void reset() {
    gunX = 105.0f; gunPxLast = -1; gunFlashTimer = 0;
    lv_obj_set_pos(gunObj, (int)gunX, GUN_Y);
    lv_obj_set_style_bg_color(gunObj, lv_color_make(34,197,94), 0);

    hp = PLAYER_MAX_HP;
    rapidActive = false; shieldActive = false;
    multishotActive = false;
    laserFlashTimer = 0;

    lv_obj_add_flag(shieldIcon,    LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(rapidIcon,     LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(multishotIcon, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(laserObj,      LV_OBJ_FLAG_HIDDEN);
  }

  // Di chuyển gun — gọi mỗi tick
  void updateMovement() {
    int rawX  = analogRead(JOYSTICK_X_PIN);
    int delta = rawX - centerX;
    if (abs(delta) >= DEADZONE)
      gunX -= ((float)delta / 2048.0f) * GUN_SPEED;
    gunX = constrain(gunX, 0.0f, (float)(240 - GUN_WIDTH));

    int px = (int)gunX;
    if (px != gunPxLast) {
      gunPxLast = px;
      lv_obj_set_pos(gunObj, px, GUN_Y);
      if (shieldActive)
        lv_obj_set_pos(shieldIcon, px - 4, GUN_Y - 4);
    }

    if (gunFlashTimer > 0 && --gunFlashTimer == 0)
      lv_obj_set_style_bg_color(gunObj, lv_color_make(34,197,94), 0);
  }

  // Nhận sát thương — trả về true nếu chết
  // Debug tip: nếu player mất máu sai → đặt Serial.printf ở đây
  bool takeDamage(UIManager* ui) {
    if (shieldActive) {
      shieldActive = false;
      lv_obj_add_flag(shieldIcon, LV_OBJ_FLAG_HIDDEN);
      ui->showPowerupMsg("Shield block!");
      return false;
    }
    hp--;
    ui->updateHearts(hp);
    lv_obj_set_style_bg_color(gunObj, lv_color_make(239,68,68), 0);
    gunFlashTimer = 15;
    Serial.printf("[Player] HP: %d\n", hp);
    return (hp <= 0);
  }

  // Tick powerup timers — gọi mỗi game tick
  void updatePowerupTimers(UIManager* ui) {
    if (rapidActive && millis() > rapidEndTime) {
      rapidActive = false;
      lv_obj_add_flag(rapidIcon, LV_OBJ_FLAG_HIDDEN);
    }
    if (multishotActive && millis() > multishotEndTime) {
      multishotActive = false;
      lv_obj_add_flag(multishotIcon, LV_OBJ_FLAG_HIDDEN);
    }
    if (laserFlashTimer > 0 && --laserFlashTimer == 0)
      lv_obj_add_flag(laserObj, LV_OBJ_FLAG_HIDDEN);
    ui->tickPowerupMsg();
  }

  // Áp dụng pickup drop
  void applyRapid()      { rapidActive = true; rapidEndTime = millis() + 5000;
                           lv_obj_clear_flag(rapidIcon, LV_OBJ_FLAG_HIDDEN); }
  void applyShield()     { shieldActive = true;
                           lv_obj_clear_flag(shieldIcon, LV_OBJ_FLAG_HIDDEN); }
  void applyMultishot()  { multishotActive = true; multishotEndTime = millis() + 8000;
                           lv_obj_clear_flag(multishotIcon, LV_OBJ_FLAG_HIDDEN); }
  void showLaser(int timerVal) { laserFlashTimer = timerVal;
                                 lv_obj_clear_flag(laserObj, LV_OBJ_FLAG_HIDDEN); }

  // Helpers
  int  effectiveShotInterval() const { return rapidActive ? (shotInterval/2) : shotInterval; }
  bool isAlive()               const { return hp > 0; }

  static const char* gunDescription(int g) {
    switch (g) {
      case GUN_NORMAL:  return "Can bang, de dung";
      case GUN_FAST:    return "Ban nhanh, sat thuong thap";
      case GUN_HEAVY:   return "Ban cham, sat thuong cao";
      case GUN_SHOTGUN: return "3 dan song song";
    }
    return "";
  }
};

// ── Static member definitions ─────────────────────────────────────
const char* Player::gunNames[GUN_COUNT] = {"THUONG", "NHANH", "MANH", "SHOTGUN"};
GunStats    Player::gunStats[GUN_COUNT] = {
  {400, 1, 1},
  {200, 1, 1},
  {750, 3, 1},
  {500, 1, 3},
};
