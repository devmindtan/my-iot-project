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

const int DEADZONE = 400; // Vùng an toàn chống tự trôi con trỏ

// STATE MANAGEMENT
float playerX = 108.0;
float playerY = 108.0;
const float MAX_SPEED = 8.0;
const int PLAYER_SIZE = 24;

uint32_t lastDebounceTime = 0;
const uint32_t DEBOUNCE_DELAY = 200;
int centerX, centerY;

// ============== GAME STATE ==============
#define MAX_OBSTACLES 6
struct Obstacle {
  lv_obj_t * obj;
  float x, y;
  float speed;
  int size;
  bool active;
};
Obstacle obstacles[MAX_OBSTACLES];

bool gameOver = false;
unsigned long gameStartTime = 0;
unsigned long survivedTime = 0;
unsigned long lastSpawnTime = 0;
int spawnInterval = 1200; // ms, giảm dần theo thời gian -> khó hơn

// UI COMPONENTS
lv_obj_t * player_cursor;
lv_obj_t * score_label;
lv_obj_t * gameover_label;
lv_obj_t * restart_label;

// HÀM CALLBACK BƠM PIXEL CHUẨN LVGL V9
void my_disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)px_map, w * h, true);
  tft.endWrite();

  lv_display_flush_ready(disp);
}

// ============== TẠO / RESET OBSTACLE ==============
void spawn_obstacle() {
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (!obstacles[i].active) {
      int size = random(14, 28);
      obstacles[i].size = size;
      obstacles[i].x = random(0, 240 - size);
      obstacles[i].y = -size;
      // Tốc độ tăng dần theo thời gian sống sót
      float speedBonus = (millis() - gameStartTime) / 8000.0; // +1.0 mỗi 8s
      obstacles[i].speed = 3.0 + (random(0, 30) / 10.0) + speedBonus; // base cao hơn + bonus
      obstacles[i].active = true;

      lv_obj_set_size(obstacles[i].obj, size, size);
      lv_obj_set_pos(obstacles[i].obj, (int)obstacles[i].x, (int)obstacles[i].y);
      lv_obj_clear_flag(obstacles[i].obj, LV_OBJ_FLAG_HIDDEN);
      return;
    }
  }
}

// ============== KHỞI TẠO UI ==============
void create_game_ui() {
  // Nền màu Slate-900
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_make(15, 23, 42), 0);

  // Label điểm số / thời gian sống sót
  score_label = lv_label_create(lv_scr_act());
  lv_obj_align(score_label, LV_ALIGN_TOP_LEFT, 8, 6);
  lv_obj_set_style_text_color(score_label, lv_color_make(148, 163, 184), 0);
  lv_label_set_text(score_label, "Time: 0.0s");

  // Label Game Over (ẩn ban đầu)
  gameover_label = lv_label_create(lv_scr_act());
  lv_obj_align(gameover_label, LV_ALIGN_CENTER, 0, -20);
  lv_obj_set_style_text_color(gameover_label, lv_color_make(239, 68, 68), 0);
  lv_obj_set_style_text_font(gameover_label, &lv_font_montserrat_14, 0);
  lv_label_set_text(gameover_label, "GAME OVER");
  lv_obj_add_flag(gameover_label, LV_OBJ_FLAG_HIDDEN);

  // Label hướng dẫn restart (ẩn ban đầu)
  restart_label = lv_label_create(lv_scr_act());
  lv_obj_align(restart_label, LV_ALIGN_CENTER, 0, 20);
  lv_obj_set_style_text_color(restart_label, lv_color_make(148, 163, 184), 0);
  lv_label_set_text(restart_label, "Nhan nut de choi lai");
  lv_obj_add_flag(restart_label, LV_OBJ_FLAG_HIDDEN);

  // Player (chấm đỏ)
  player_cursor = lv_obj_create(lv_scr_act());
  lv_obj_set_size(player_cursor, PLAYER_SIZE, PLAYER_SIZE);
  lv_obj_set_style_bg_color(player_cursor, lv_color_make(239, 68, 68), 0); // Red-500
  lv_obj_set_style_radius(player_cursor, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(player_cursor, 0, 0);
  lv_obj_set_style_shadow_width(player_cursor, 10, 0);
  lv_obj_set_style_shadow_color(player_cursor, lv_color_make(0, 0, 0), 0);
  lv_obj_set_style_shadow_opa(player_cursor, LV_OPA_40, 0);
  lv_obj_set_pos(player_cursor, (int)playerX, (int)playerY);

  // Khởi tạo các obstacle (ẩn sẵn, dùng lại)
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    obstacles[i].obj = lv_obj_create(lv_scr_act());
    lv_obj_set_style_bg_color(obstacles[i].obj, lv_color_make(250, 204, 21), 0); // Yellow-400
    lv_obj_set_style_radius(obstacles[i].obj, 4, 0);
    lv_obj_set_style_border_width(obstacles[i].obj, 0, 0);
    lv_obj_add_flag(obstacles[i].obj, LV_OBJ_FLAG_HIDDEN);
    obstacles[i].active = false;
  }

  lv_refr_now(NULL);
}

// ============== RESET GAME ==============
void reset_game() {
  playerX = 108.0;
  playerY = 108.0;
  lv_obj_set_pos(player_cursor, (int)playerX, (int)playerY);
  lv_obj_set_style_bg_color(player_cursor, lv_color_make(239, 68, 68), 0);

  for (int i = 0; i < MAX_OBSTACLES; i++) {
    obstacles[i].active = false;
    lv_obj_add_flag(obstacles[i].obj, LV_OBJ_FLAG_HIDDEN);
  }

  spawnInterval = 900;
  lastSpawnTime = millis();
  gameStartTime = millis();
  gameOver = false;

  lv_obj_add_flag(gameover_label, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(restart_label, LV_OBJ_FLAG_HIDDEN);
}

// ============== XỬ LÝ NÚT NHẤN ==============
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

// ============== DI CHUYỂN PLAYER ==============
void update_player_movement() {
  int rawX = analogRead(JOYSTICK_X_PIN);
  int rawY = analogRead(JOYSTICK_Y_PIN);

  float stepX = -((float)(rawX - centerX) / 2048.0) * MAX_SPEED;
  float stepY = -((float)(rawY - centerY) / 2048.0) * MAX_SPEED;

  if (abs(rawX - centerX) < DEADZONE) stepX = 0;
  if (abs(rawY - centerY) < DEADZONE) stepY = 0;

  playerX += stepX;
  playerY += stepY;

  if (playerX < 0) playerX = 0;
  if (playerX > (240 - PLAYER_SIZE)) playerX = (240 - PLAYER_SIZE);
  if (playerY < 0) playerY = 0;
  if (playerY > (240 - PLAYER_SIZE)) playerY = (240 - PLAYER_SIZE);

  lv_obj_set_pos(player_cursor, (int)playerX, (int)playerY);
}

// ============== KIỂM TRA VA CHẠM (AABB) ==============
bool check_collision(float ax, float ay, int asize, float bx, float by, int bsize) {
  return (ax < bx + bsize && ax + asize > bx &&
          ay < by + bsize && ay + asize > by);
}

// ============== CẬP NHẬT OBSTACLES ==============
void update_obstacles() {
  unsigned long now = millis();

  // Tăng độ khó theo thời gian, giảm dần khoảng cách spawn
  unsigned long elapsed = now - gameStartTime;
  spawnInterval = 900 - (elapsed / 80); // giảm nhanh hơn
  if (spawnInterval < 200) spawnInterval = 200;

  if (now - lastSpawnTime > (unsigned long)spawnInterval) {
    lastSpawnTime = now;
    spawn_obstacle();
  }

  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (obstacles[i].active) {
      obstacles[i].y += obstacles[i].speed;

      if (obstacles[i].y > 240) {
        // rơi hết màn hình -> tái sử dụng
        obstacles[i].active = false;
        lv_obj_add_flag(obstacles[i].obj, LV_OBJ_FLAG_HIDDEN);
        continue;
      }

      lv_obj_set_pos(obstacles[i].obj, (int)obstacles[i].x, (int)obstacles[i].y);

      // Kiểm tra va chạm với player
      if (check_collision(playerX, playerY, PLAYER_SIZE,
                           obstacles[i].x, obstacles[i].y, obstacles[i].size)) {
        gameOver = true;
        survivedTime = now - gameStartTime;
      }
    }
  }
}

// ============== CẬP NHẬT LABEL ĐIỂM ==============
void update_score_label() {
  char buf[32];
  if (!gameOver) {
    float t = (millis() - gameStartTime) / 1000.0;
    sprintf(buf, "Time: %.1fs", t);
  } else {
    sprintf(buf, "Survived: %.1fs", survivedTime / 1000.0);
  }
  lv_label_set_text(score_label, buf);
}

// ============== XỬ LÝ GAME OVER ==============
void trigger_game_over() {
  lv_obj_clear_flag(gameover_label, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(restart_label, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_style_bg_color(player_cursor, lv_color_make(100, 100, 100), 0); // xám khi chết
  update_score_label();
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
    update_player_movement();
    update_obstacles();
    update_score_label();

    if (gameOver) {
      trigger_game_over();
    }
  }

  lv_timer_handler();
  lv_refr_now(NULL);

  delay(10);
}
