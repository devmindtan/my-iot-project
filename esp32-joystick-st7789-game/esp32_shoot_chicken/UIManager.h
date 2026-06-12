#pragma once
#include <lvgl.h>
#include "Config.h"

// ── UIManager ─────────────────────────────────────────────────────
// Chịu trách nhiệm:
//   - Tạo tất cả LVGL objects (gọi 1 lần sau display init)
//   - Hiện/ẩn theo GameState
//   - Cập nhật score label (dirty flag) và hearts
//   - Hiện thông báo powerup
// Debug tip: nếu UI sai vị trí → kiểm tra createAll(); nếu không update → dirty flag
class UIManager {
public:
  // ── LVGL objects ───────────────────────────────────────────────
  lv_obj_t* scoreLabel    = nullptr;
  lv_obj_t* heartIcons[MAX_HEARTS];
  lv_obj_t* gameoverLabel = nullptr;
  lv_obj_t* restartLabel  = nullptr;
  lv_obj_t* menuTitle     = nullptr;
  lv_obj_t* menuGunname   = nullptr;
  lv_obj_t* menuDesc      = nullptr;
  lv_obj_t* menuHint      = nullptr;
  lv_obj_t* powerupLabel  = nullptr;
  // Powerup HUD icons (được Player cũng dùng)
  lv_obj_t* shieldIcon    = nullptr;
  lv_obj_t* rapidIcon     = nullptr;
  lv_obj_t* multishotIcon = nullptr;
  lv_obj_t* laserObj      = nullptr;

  // ── Dirty flag score ───────────────────────────────────────────
  int scoreDisplayed = -1;

  // Tạo tất cả LVGL objects — gọi 1 lần trong setup()
  void createAll() {
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_make(30,41,59), 0);

    // Score
    scoreLabel = lv_label_create(lv_scr_act());
    lv_obj_align(scoreLabel, LV_ALIGN_TOP_LEFT, 8, 6);
    lv_obj_set_style_text_color(scoreLabel, lv_color_make(226,232,240), 0);
    lv_label_set_text(scoreLabel, "Score: 0");

    // Hearts
    for (int i = 0; i < MAX_HEARTS; i++) {
      heartIcons[i] = lv_obj_create(lv_scr_act());
      lv_obj_set_size(heartIcons[i], 14, 14);
      lv_obj_set_style_radius(heartIcons[i], LV_RADIUS_CIRCLE, 0);
      lv_obj_set_style_border_width(heartIcons[i], 0, 0);
      lv_obj_set_style_bg_color(heartIcons[i], lv_color_make(239,68,68), 0);
      lv_obj_set_pos(heartIcons[i], 240 - 20 - (i*18), 6);
    }

    // Powerup icons
    shieldIcon = lv_obj_create(lv_scr_act());
    lv_obj_set_size(shieldIcon, GUN_WIDTH+8, GUN_HEIGHT+8);
    lv_obj_set_style_radius(shieldIcon, 6, 0);
    lv_obj_set_style_bg_opa(shieldIcon, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(shieldIcon, 3, 0);
    lv_obj_set_style_border_color(shieldIcon, lv_color_make(34,197,94), 0);
    lv_obj_add_flag(shieldIcon, LV_OBJ_FLAG_HIDDEN);

    rapidIcon = lv_label_create(lv_scr_act());
    lv_obj_set_pos(rapidIcon, 8, 22);
    lv_obj_set_style_text_color(rapidIcon, lv_color_make(56,189,248), 0);
    lv_label_set_text(rapidIcon, "2x");
    lv_obj_add_flag(rapidIcon, LV_OBJ_FLAG_HIDDEN);

    multishotIcon = lv_label_create(lv_scr_act());
    lv_obj_set_pos(multishotIcon, 8, 36);
    lv_obj_set_style_text_color(multishotIcon, lv_color_make(192,132,252), 0);
    lv_label_set_text(multishotIcon, "5x");
    lv_obj_add_flag(multishotIcon, LV_OBJ_FLAG_HIDDEN);

    laserObj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(laserObj, 6, 240);
    lv_obj_set_style_bg_color(laserObj, lv_color_make(255,255,100), 0);
    lv_obj_set_style_border_width(laserObj, 0, 0);
    lv_obj_set_style_radius(laserObj, 0, 0);
    lv_obj_add_flag(laserObj, LV_OBJ_FLAG_HIDDEN);

    powerupLabel = lv_label_create(lv_scr_act());
    lv_obj_align(powerupLabel, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_text_color(powerupLabel, lv_color_make(250,204,21), 0);
    lv_obj_set_style_text_font(powerupLabel, &lv_font_montserrat_14, 0);
    lv_label_set_text(powerupLabel, "");
    lv_obj_add_flag(powerupLabel, LV_OBJ_FLAG_HIDDEN);

    // Game Over
    gameoverLabel = lv_label_create(lv_scr_act());
    lv_obj_align(gameoverLabel, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_text_color(gameoverLabel, lv_color_make(239,68,68), 0);
    lv_obj_set_style_text_font(gameoverLabel, &lv_font_montserrat_14, 0);
    lv_label_set_text(gameoverLabel, "GAME OVER");
    lv_obj_add_flag(gameoverLabel, LV_OBJ_FLAG_HIDDEN);

    restartLabel = lv_label_create(lv_scr_act());
    lv_obj_align(restartLabel, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_text_color(restartLabel, lv_color_make(148,163,184), 0);
    lv_label_set_text(restartLabel, "Nhan nut de ve menu");
    lv_obj_add_flag(restartLabel, LV_OBJ_FLAG_HIDDEN);

    // Menu
    menuTitle = lv_label_create(lv_scr_act());
    lv_obj_align(menuTitle, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_set_style_text_color(menuTitle, lv_color_make(226,232,240), 0);
    lv_label_set_text(menuTitle, "CHON SUNG");

    menuGunname = lv_label_create(lv_scr_act());
    lv_obj_align(menuGunname, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_text_color(menuGunname, lv_color_make(250,204,21), 0);
    lv_obj_set_style_text_font(menuGunname, &lv_font_montserrat_14, 0);

    menuDesc = lv_label_create(lv_scr_act());
    lv_obj_align(menuDesc, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_text_color(menuDesc, lv_color_make(148,163,184), 0);

    menuHint = lv_label_create(lv_scr_act());
    lv_obj_align(menuHint, LV_ALIGN_BOTTOM_MID, 0, -16);
    lv_obj_set_style_text_color(menuHint, lv_color_make(100,116,139), 0);
    lv_label_set_text(menuHint, "Gat trai/phai: doi sung\nNhan nut: bat dau");

    lv_refr_now(NULL);
  }

  // ── Score (dirty flag) ─────────────────────────────────────────
  void updateScore(int score) {
    if (score == scoreDisplayed) return;
    scoreDisplayed = score;
    char buf[20];
    sprintf(buf, "Score: %d", score);
    lv_label_set_text(scoreLabel, buf);
  }

  void resetScoreDirty() { scoreDisplayed = -1; }

  // ── Hearts ─────────────────────────────────────────────────────
  void updateHearts(int playerHP) {
    for (int i = 0; i < MAX_HEARTS; i++) {
      lv_obj_set_style_bg_color(heartIcons[i],
        (i < playerHP) ? lv_color_make(239,68,68)
                       : lv_color_make(71,85,105), 0);
    }
  }

  // ── Powerup message ────────────────────────────────────────────
  int powerupMsgTimer = 0;

  void showPowerupMsg(const char* msg) {
    lv_label_set_text(powerupLabel, msg);
    lv_obj_clear_flag(powerupLabel, LV_OBJ_FLAG_HIDDEN);
    powerupMsgTimer = 70;
  }

  void tickPowerupMsg() {
    if (powerupMsgTimer > 0 && --powerupMsgTimer == 0)
      lv_obj_add_flag(powerupLabel, LV_OBJ_FLAG_HIDDEN);
  }

  // ── Menu helpers ───────────────────────────────────────────────
  void setMenuGun(const char* name, const char* desc) {
    char buf[24];
    sprintf(buf, "< %s >", name);
    lv_label_set_text(menuGunname, buf);
    lv_label_set_text(menuDesc, desc);
  }

  void showGameOverScreen() {
    lv_obj_clear_flag(gameoverLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(restartLabel,  LV_OBJ_FLAG_HIDDEN);
  }
};
