#include <tft_setup.h>
#include <SPI.h>
#include <TFT_eSPI.h> 
#include <lvgl.h> 

TFT_eSPI tft = TFT_eSPI();

// Cấu hình thông số màn hình
static const uint16_t screenWidth  = 240;
static const uint16_t screenHeight = 240;

// Định nghĩa chân phần cứng Joystick
#define JOYSTICK_X_PIN  34
#define JOYSTICK_Y_PIN  35
#define JOYSTICK_SW_PIN 32

const int DEADZONE = 400;

// ============== PLAYER (SÚNG) ==============
const int GUN_WIDTH  = 30;
const int GUN_HEIGHT = 14;
const int GUN_Y      = 240 - 20; // súng đặt sát đáy
float gunX = 105.0;              // toạ độ X của súng (top-left)
const float GUN_SPEED = 6.0;

int centerX, centerY;
uint32_t lastDebounceTime = 0;
const uint32_t DEBOUNCE_DELAY = 200;

// ============== BULLET (ĐẠN TỰ BẮN) ==============
#define MAX_BULLETS 4
struct Bullet {
  lv_obj_t * obj;
  float x, y;
  bool active;
};
Bullet bullets[MAX_BULLETS];
const float BULLET_SPEED = 9.0;
const int BULLET_W = 4;
const int BULLET_H = 10;
unsigned long lastShotTime = 0;
int shotInterval = 350; // ms giữa các lần bắn, giảm dần theo điểm

// ============== CHICKEN (GÀ RƠI) ==============
#define MAX_CHICKENS 4
struct Chicken {
  lv_obj_t * obj;
  float x, y;
  float speed;
  int size;
  bool active;
};
Chicken chickens[MAX_CHICKENS];
unsigned long lastSpawnTime = 0;
int spawnInterval = 1100;

// ============== GAME STATE ==============
bool gameOver = false;
unsigned long gameStartTime = 0;
int score = 0;
int finalScore = 0;

// UI COMPONENTS
lv_obj_t * gun_obj;
lv_obj_t * score_label;
lv_obj_t * gameover_label;
lv_obj_t * restart_label;

// ============== LVGL FLUSH CALLBACK ==============
void my_disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)px_map, w * h, true);
  tft.endWrite();

  lv_display_flush_ready(disp);
}

// ============== AABB COLLISION ==============
bool check_collision(float ax, float ay, int aw, int ah, float bx, float by, int bw, int bh) {
  return (ax < bx + bw && ax + aw > bx &&
          ay < by + bh && ay + ah > by);
}

// ============== SPAWN GÀ ==============
void spawn_chicken() {
  for (int i = 0; i < MAX_CHICKENS; i++) {
    if (!chickens[i].active) {
      int size = random(20, 34);
      chickens[i].size = size;
      chickens[i].x = random(0, 240 - size);
      chickens[i].y = -size;

      // tốc độ tăng dần theo điểm
      float speedBonus = score / 10.0;
      chickens[i].speed = 1.5 + (random(0, 20) / 10.0) + speedBonus;
      chickens[i].active = true;

      lv_obj_set_size(chickens[i].obj, size, size);
      lv_obj_set_pos(chickens[i].obj, (int)chickens[i].x, (int)chickens[i].y);
      lv_obj_clear_flag(chickens[i].obj, LV_OBJ_FLAG_HIDDEN);
      return;
    }
  }
}

// ============== BẮN ĐẠN (TỰ ĐỘNG) ==============
void fire_bullet() {
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!bullets[i].active) {
      bullets[i].x = gunX + (GUN_WIDTH / 2) - (BULLET_W / 2);
      bullets[i].y = GUN_Y - BULLET_H;
      bullets[i].active = true;
      lv_obj_set_pos(bullets[i].obj, (int)bullets[i].x, (int)bullets[i].y);
      lv_obj_clear_flag(bullets[i].obj, LV_OBJ_FLAG_HIDDEN);
      return;
    }
  }
}

// ============== KHỞI TẠO UI ==============
void create_game_ui() {
  // Nền tối
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_make(30, 41, 59), 0);

  // Label điểm số
  score_label = lv_label_create(lv_scr_act());
  lv_obj_align(score_label, LV_ALIGN_TOP_LEFT, 8, 6);
  lv_obj_set_style_text_color(score_label, lv_color_make(226, 232, 240), 0);
  lv_label_set_text(score_label, "Score: 0");

  // Label Game Over (ẩn ban đầu)
  gameover_label = lv_label_create(lv_scr_act());
  lv_obj_align(gameover_label, LV_ALIGN_CENTER, 0, -20);
  lv_obj_set_style_text_color(gameover_label, lv_color_make(239, 68, 68), 0);
  lv_obj_set_style_text_font(gameover_label, &lv_font_montserrat_14, 0);
  lv_label_set_text(gameover_label, "GAME OVER");
  lv_obj_add_flag(gameover_label, LV_OBJ_FLAG_HIDDEN);

  // Label hướng dẫn restart (ẩn ban đầu)
  restart_label = lv_label_create(lv_scr_act());
  lv_obj_align(restart_label, LV_ALIGN_CENTER, 0, 10);
  lv_obj_set_style_text_color(restart_label, lv_color_make(148, 163, 184), 0);
  lv_label_set_text(restart_label, "Nhấn nút để chơi lại");
  lv_obj_add_flag(restart_label, LV_OBJ_FLAG_HIDDEN);

  // Súng (hình chữ nhật xanh lá ở đáy)
  gun_obj = lv_obj_create(lv_scr_act());
  lv_obj_set_size(gun_obj, GUN_WIDTH, GUN_HEIGHT);
  lv_obj_set_style_bg_color(gun_obj, lv_color_make(34, 197, 94), 0); // Green-500
  lv_obj_set_style_radius(gun_obj, 4, 0);
  lv_obj_set_style_border_width(gun_obj, 0, 0);
  lv_obj_set_pos(gun_obj, (int)gunX, GUN_Y);

  // Khởi tạo đạn (tái sử dụng)
  for (int i = 0; i < MAX_BULLETS; i++) {
    bullets[i].obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bullets[i].obj, BULLET_W, BULLET_H);
    lv_obj_set_style_bg_color(bullets[i].obj, lv_color_make(250, 204, 21), 0); // Yellow-400
    lv_obj_set_style_radius(bullets[i].obj, 2, 0);
    lv_obj_set_style_border_width(bullets[i].obj, 0, 0);
    lv_obj_add_flag(bullets[i].obj, LV_OBJ_FLAG_HIDDEN);
    bullets[i].active = false;
  }

  // Khởi tạo gà (tái sử dụng)
  for (int i = 0; i < MAX_CHICKENS; i++) {
    chickens[i].obj = lv_obj_create(lv_scr_act());
    lv_obj_set_style_bg_color(chickens[i].obj, lv_color_make(251, 191, 36), 0); // Amber-400
    lv_obj_set_style_radius(chickens[i].obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(chickens[i].obj, 2, 0);
    lv_obj_set_style_border_color(chickens[i].obj, lv_color_make(255, 255, 255), 0);
    lv_obj_add_flag(chickens[i].obj, LV_OBJ_FLAG_HIDDEN);
    chickens[i].active = false;
  }

  lv_refr_now(NULL);
}

// ============== RESET GAME ==============
void reset_game() {
  gunX = 105.0;
  lv_obj_set_pos(gun_obj, (int)gunX, GUN_Y);
  lv_obj_set_style_bg_color(gun_obj, lv_color_make(34, 197, 94), 0);

  for (int i = 0; i < MAX_BULLETS; i++) {
    bullets[i].active = false;
    lv_obj_add_flag(bullets[i].obj, LV_OBJ_FLAG_HIDDEN);
  }
  for (int i = 0; i < MAX_CHICKENS; i++) {
    chickens[i].active = false;
    lv_obj_add_flag(chickens[i].obj, LV_OBJ_FLAG_HIDDEN);
  }

  score = 0;
  spawnInterval = 1100;
  shotInterval = 350;
  lastSpawnTime = millis();
  lastShotTime = millis();
  gameStartTime = millis();
  gameOver = false;

  lv_obj_add_flag(gameover_label, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(restart_label, LV_OBJ_FLAG_HIDDEN);

  char buf[24];
  sprintf(buf, "Score: %d", score);
  lv_label_set_text(score_label, buf);
}

// ============== NÚT NHẤN ==============
void handle_button_click() {
  if (digitalRead(JOYSTICK_SW_PIN) == LOW) {
    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
      lastDebounceTime = millis();
      if (gameOver) {
        reset_game();
      }
    }
  }
}

// ============== DI CHUYỂN SÚNG (CHỈ TRỤC X) ==============
void update_gun_movement() {
  int rawX = analogRead(JOYSTICK_X_PIN);
  float stepX = -((float)(rawX - centerX) / 2048.0) * GUN_SPEED;
  if (abs(rawX - centerX) < DEADZONE) stepX = 0;

  gunX += stepX;

  if (gunX < 0) gunX = 0;
  if (gunX > (240 - GUN_WIDTH)) gunX = (240 - GUN_WIDTH);

  lv_obj_set_pos(gun_obj, (int)gunX, GUN_Y);
}

// ============== TỰ ĐỘNG BẮN ĐẠN ==============
void update_auto_shoot() {
  unsigned long now = millis();
  if (now - lastShotTime > (unsigned long)shotInterval) {
    lastShotTime = now;
    fire_bullet();
  }
}

// ============== CẬP NHẬT ĐẠN ==============
void update_bullets() {
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (bullets[i].active) {
      bullets[i].y -= BULLET_SPEED;

      if (bullets[i].y < -BULLET_H) {
        bullets[i].active = false;
        lv_obj_add_flag(bullets[i].obj, LV_OBJ_FLAG_HIDDEN);
        continue;
      }
      lv_obj_set_pos(bullets[i].obj, (int)bullets[i].x, (int)bullets[i].y);
    }
  }
}

// ============== CẬP NHẬT GÀ ==============
void update_chickens() {
  unsigned long now = millis();
  unsigned long elapsed = now - gameStartTime;

  // tăng độ khó: spawn nhanh hơn theo thời gian + theo điểm
  spawnInterval = 1100 - (elapsed / 60) - (score * 15);
  if (spawnInterval < 350) spawnInterval = 350;

  // bắn nhanh hơn khi điểm cao
  shotInterval = 350 - (score * 8);
  if (shotInterval < 120) shotInterval = 120;

  if (now - lastSpawnTime > (unsigned long)spawnInterval) {
    lastSpawnTime = now;
    spawn_chicken();
  }

  for (int i = 0; i < MAX_CHICKENS; i++) {
    if (chickens[i].active) {
      chickens[i].y += chickens[i].speed;

      // Va chạm với súng -> thua
      if (check_collision(gunX, GUN_Y, GUN_WIDTH, GUN_HEIGHT,
                           chickens[i].x, chickens[i].y, chickens[i].size, chickens[i].size)) {
        gameOver = true;
        finalScore = score;
        return;
      }

      // Rơi quá đáy mà chưa bị bắn -> coi như thua (gà chạm đất)
      if (chickens[i].y > 240) {
        gameOver = true;
        finalScore = score;
        return;
      }

      // Kiểm tra va chạm với từng đạn
      for (int b = 0; b < MAX_BULLETS; b++) {
        if (bullets[b].active &&
            check_collision(bullets[b].x, bullets[b].y, BULLET_W, BULLET_H,
                             chickens[i].x, chickens[i].y, chickens[i].size, chickens[i].size)) {
          // Trúng gà -> tăng điểm, ẩn gà và đạn
          score++;
          chickens[i].active = false;
          lv_obj_add_flag(chickens[i].obj, LV_OBJ_FLAG_HIDDEN);

          bullets[b].active = false;
          lv_obj_add_flag(bullets[b].obj, LV_OBJ_FLAG_HIDDEN);

          char buf[24];
          sprintf(buf, "Score: %d", score);
          lv_label_set_text(score_label, buf);
          break;
        }
      }

      if (chickens[i].active) {
        lv_obj_set_pos(chickens[i].obj, (int)chickens[i].x, (int)chickens[i].y);
      }
    }
  }
}

// ============== GAME OVER ==============
void trigger_game_over() {
  lv_obj_clear_flag(gameover_label, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(restart_label, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_style_bg_color(gun_obj, lv_color_make(100, 100, 100), 0);

  char buf[32];
  sprintf(buf, "Score: %d", finalScore);
  lv_label_set_text(score_label, buf);
}

// ============== SETUP ==============
void setup() {
  Serial.begin(115200);
  pinMode(JOYSTICK_X_PIN, INPUT);
  pinMode(JOYSTICK_Y_PIN, INPUT);
  pinMode(JOYSTICK_SW_PIN, INPUT_PULLUP);

  // Calibrate tâm joystick
  long sumX = 0, sumY = 0;
  for (int i = 0; i < 50; i++) {
    sumX += analogRead(JOYSTICK_X_PIN);
    sumY += analogRead(JOYSTICK_Y_PIN);
    delay(5);
  }
  centerX = sumX / 50;
  centerY = sumY / 50;
  Serial.printf("Center calibrated: X=%d Y=%d\n", centerX, centerY);

  tft.init();
  tft.setRotation(0);

  lv_init();

  lv_display_t * disp = lv_display_create(screenWidth, screenHeight);
  static uint8_t buf[screenWidth * 10 * sizeof(lv_color_t)];
  lv_display_set_buffers(disp, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(disp, my_disp_flush);

  randomSeed(analogRead(36));

  create_game_ui();
  reset_game();
}

// ============== LOOP ==============
void loop() {
  handle_button_click();

  if (!gameOver) {
    update_gun_movement();
    update_auto_shoot();
    update_bullets();
    update_chickens();

    if (gameOver) {
      trigger_game_over();
    }
  }

  lv_timer_handler();
  lv_refr_now(NULL);

  delay(10);
}
