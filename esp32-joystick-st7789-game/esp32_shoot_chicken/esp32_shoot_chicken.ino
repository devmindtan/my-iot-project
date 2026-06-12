/*
 * ESP32 Shoot Chicken - OOP Refactored
 *
 * CẤU TRÚC FILE:
 *  ├── Config.h          — Hằng số, enums, structs cơ bản
 *  ├── DisplayManager.h  — Khởi tạo TFT + LVGL flush
 *  ├── Player.h          — Gun, HP, powerup, joystick
 *  ├── BulletPool.h      — Pool 16 đạn
 *  ├── EnemyPool.h       — Pool 5 enemy + HP bar
 *  ├── DropPool.h        — Pool 5 drop item + ParticlePool + FlashPool
 *  ├── UIManager.h       — Score, hearts, menu labels, game over
 *  ├── GameManager.h     — State machine, vòng lặp, spawn logic
 *  └── esp32_shoot_chicken.ino  ← file này
 */

#include "Config.h"
#include "DisplayManager.h"
#include "UIManager.h"
#include "Player.h"
#include "BulletPool.h"
#include "EnemyPool.h"
#include "DropPool.h"
#include "ParticlePool.h"
#include "FlashPool.h"
#include "GameManager.h"

// ── Singleton instances ──────────────────────────────────────────
DisplayManager  displayMgr;
UIManager       uiMgr;
ParticlePool    particlePool;
FlashPool       flashPool;
BulletPool      bulletPool;
DropPool        dropPool;
EnemyPool       enemyPool;
Player          player;
GameManager     gameMgr;

// ── Setup ────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  // 1. pinMode TRƯỚC calibrate joystick
  pinMode(JOYSTICK_X_PIN,  INPUT);
  pinMode(JOYSTICK_Y_PIN,  INPUT);
  pinMode(JOYSTICK_SW_PIN, INPUT_PULLUP);

  // 2. Display + LVGL init
  displayMgr.init();

  // 3. Tạo tất cả LVGL objects — MỘT LẦN duy nhất theo đúng thứ tự
  uiMgr.createAll();          // labels, hearts, icons
  player.createGunObj();      // gun_obj (phải tạo TRƯỚC showMenu)

  // 4. Link UI icon pointers sang Player (để Player tự ẩn/hiện khi nhận damage/pickup)
  player.shieldIcon    = uiMgr.shieldIcon;
  player.rapidIcon     = uiMgr.rapidIcon;
  player.multishotIcon = uiMgr.multishotIcon;
  player.laserObj      = uiMgr.laserObj;

  // 5. Tạo pool objects (bullets, enemies, drops, particles, flashes)
  bulletPool.createObjects();
  enemyPool.createObjects();
  dropPool.createObjects();
  particlePool.createObjects();
  flashPool.createObjects();

  // 6. Calibrate joystick center (sau pinMode)
  player.calibrateJoystick();

  // 7. Init GameManager — chỉ nhận references, KHÔNG tạo objects lần nữa
  gameMgr.init(&displayMgr, &uiMgr,
               &player, &bulletPool, &enemyPool,
               &dropPool, &particlePool, &flashPool);

  // 8. Hiện menu (gun_obj đã tồn tại → an toàn)
  gameMgr.showMenu();
}

// ── Loop ─────────────────────────────────────────────────────────
void loop() {
  gameMgr.handleButtonClick();

  unsigned long now = millis();
  if (now - gameMgr.lastTickTime >= GAME_TICK_MS) {
    gameMgr.lastTickTime = now;
    gameMgr.tick();
  }

  lv_timer_handler();
  lv_refr_now(NULL);
  delay(10);
}
