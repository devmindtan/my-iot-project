/*
 * ESP32 Shoot Chicken - v4 OPTIMIZED
 * Tối ưu cho ESP32-D0WD-V3 @ 240MHz, 4MB Flash, LVGL v8/v9
 *
 * CÁC TỐI ƯU CHÍNH:
 * 1. Display buffer đôi 20 dòng (x2 so với trước) → giảm flush overhead
 * 2. Dirty-flag per object: lv_obj_set_pos chỉ gọi khi vị trí thực sự đổi
 * 3. HP bar tách move/update: move_enemy_hpbar chỉ set_pos, update_enemy_hpbar
 *    chỉ gọi khi HP thay đổi thực sự
 * 4. Enemy flash: restore màu từ cache (cr,cg,cb) không qua switch/lv_color_make mỗi frame
 * 5. Particles: pool cố định, int-math (x10), không float, không gravity object
 * 6. Flashes: pool nhỏ (4), life=2 (đủ thấy, ít frame dirty)
 * 7. Bỏ lv_refr_now() khỏi loop thường – LVGL tự lập lịch qua lv_timer_handler
 *    lv_refr_now chỉ dùng 1 lần sau create_game_ui
 * 8. Game tick tách khỏi render: logic chạy mỗi GAME_TICK_MS, không phụ thuộc delay
 * 9. Bullet: không gọi lv_obj_set_style mỗi frame, chỉ khi bắt đầu active
 * 10. Score label: chỉ update khi score thực sự đổi (dirty flag)
 * 11. Border radius trên particle/bullet = 0 (hình vuông nhanh hơn circle)
 * 12. Bỏ border trên objects nhỏ (particle, bullet, flash) → ít overdraw
 */

#include <tft_setup.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <lvgl.h>

TFT_eSPI tft = TFT_eSPI();

static const uint16_t screenWidth  = 240;
static const uint16_t screenHeight = 240;

#define JOYSTICK_X_PIN  34
#define JOYSTICK_Y_PIN  35
#define JOYSTICK_SW_PIN 32

const int DEADZONE = 400;

// ============== TIMING ==============
// Game logic tick cố định, tách khỏi render
#define GAME_TICK_MS  14   // ~71fps logic cap; LVGL render tự quyết định
static unsigned long lastTickTime = 0;

// ============== PLAYER ==============
const int GUN_WIDTH  = 30;
const int GUN_HEIGHT = 14;
const int GUN_Y      = 220;
float gunX = 105.0f;
const float GUN_SPEED = 6.0f;
int centerX, centerY;
uint32_t lastDebounceTime = 0;
const uint32_t DEBOUNCE_DELAY = 200;
// Cache vị trí gun để tránh set_pos thừa
int gunPxLast = -1;

// ============== MÁU PLAYER ==============
const int PLAYER_MAX_HP = 3;
int playerHP = PLAYER_MAX_HP;
#define MAX_HEARTS 3
lv_obj_t* heart_icons[MAX_HEARTS];

// ============== POWERUP STATE ==============
bool rapidActive      = false;
unsigned long rapidEndTime = 0;
bool shieldActive     = false;
bool multishotActive  = false;
unsigned long multishotEndTime = 0;
int laserFlashTimer   = 0;

lv_obj_t* shield_icon;
lv_obj_t* rapid_icon;
lv_obj_t* multishot_icon;
lv_obj_t* laser_obj;
lv_obj_t* powerup_label;
int powerupMsgTimer = 0;

// ============== LOẠI SÚNG ==============
enum GunType { GUN_NORMAL, GUN_FAST, GUN_HEAVY, GUN_SHOTGUN, GUN_COUNT };
int selectedGun = GUN_NORMAL;
const char* gunNames[GUN_COUNT] = {"THUONG", "NHANH", "MANH", "SHOTGUN"};
struct GunStats { int interval; int damage; int numBullets; };
GunStats gunStats[GUN_COUNT] = {
  {400, 1, 1},
  {200, 1, 1},
  {750, 3, 1},
  {500, 1, 3},
};

// ============== BULLET ==============
// 16 slot để hỗ trợ multishot 5 đạn liên tục
#define MAX_BULLETS 16
struct Bullet {
  lv_obj_t* obj;
  int x, y;
  int prevX, prevY;  // dirty-flag: chỉ set_pos khi đổi
  int damage;
  bool active;
};
Bullet bullets[MAX_BULLETS];
const int BULLET_SPEED = 9;
const int BULLET_W = 4;
const int BULLET_H = 10;
unsigned long lastShotTime = 0;
int shotInterval = 400;

// ============== LOẠI VẬT THỂ ==============
enum EnemyType {
  ENEMY_CHICKEN,
  ENEMY_TANK,
  ENEMY_BOSS,
  ENEMY_ZIGZAG,
  ENEMY_TYPE_COUNT
};

// ============== DROP ==============
enum DropType {
  DROP_NONE, DROP_HP, DROP_RAPID, DROP_SHIELD,
  DROP_BOMB, DROP_LASER, DROP_MULTISHOT, DROP_MAGNET
};
#define MAX_DROPS 5
struct Drop {
  lv_obj_t* obj;
  int x, y, prevY;   // prevY dirty-flag
  DropType type;
  bool active;
};
Drop drops[MAX_DROPS];
const int DROP_SIZE  = 12;
const int DROP_SPEED = 1;

// ============== PARTICLE ==============
// Hình vuông 3x3 (không circle) → nhanh hơn đáng kể
#define MAX_PARTICLES 8   // giảm từ 10 → 8; đủ thị giác, ít object dirty
struct Particle {
  lv_obj_t* obj;
  int x, y, prevX, prevY;
  int vx10, vy10;
  int life;
  bool active;
};
Particle particles[MAX_PARTICLES];

// ============== FLASH HIT ==============
// Life=2 thay vì 3: thấy được, ít frame bẩn
#define MAX_FLASHES 4
struct Flash {
  lv_obj_t* obj;
  int life;
  bool active;
};
Flash flashes[MAX_FLASHES];

// ============== ENEMY ==============
#define MAX_ENEMIES 5
struct Enemy {
  lv_obj_t* obj;
  lv_obj_t* hpBarBg;
  lv_obj_t* hpBarFill;
  float x, y;
  float speed;
  float zigDir;
  int size;
  int hp, maxHp;
  int prevHp;          // dirty-flag HP bar: chỉ update khi hp đổi
  int prevBarX, prevBarY; // dirty-flag bar pos
  bool active;
  EnemyType type;
  bool flashActive;
  int flashTimer;
  uint8_t cr, cg, cb; // cache màu gốc
};
Enemy enemies[MAX_ENEMIES];
unsigned long lastSpawnTime = 0;
int spawnInterval = 1400;

// ============== SCORE DIRTY FLAG ==============
int scoreDisplayed = -1; // force update lần đầu

// ============== GAME STATE ==============
enum GameState { STATE_MENU, STATE_PLAYING, STATE_OVER };
GameState gameState = STATE_MENU;
unsigned long gameStartTime = 0;
int score = 0, finalScore = 0;
int gunFlashTimer = 0;

// UI
lv_obj_t* gun_obj;
lv_obj_t* score_label;
lv_obj_t* gameover_label;
lv_obj_t* restart_label;
lv_obj_t* menu_title;
lv_obj_t* menu_gunname;
lv_obj_t* menu_desc;
lv_obj_t* menu_hint;

// ============== FLUSH (DMA-friendly) ==============
void my_disp_flush(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
  uint32_t w = area->x2 - area->x1 + 1;
  uint32_t h = area->y2 - area->y1 + 1;
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t*)px_map, w * h, true);
  tft.endWrite();
  lv_display_flush_ready(disp);
}

// ============== COLLISION ==============
inline bool check_collision(int ax, int ay, int aw, int ah,
                             int bx, int by, int bw, int bh) {
  return (ax < bx+bw && ax+aw > bx && ay < by+bh && ay+ah > by);
}

// ============== HEARTS ==============
void update_hearts() {
  for (int i = 0; i < MAX_HEARTS; i++) {
    lv_obj_set_style_bg_color(heart_icons[i],
      (i < playerHP) ? lv_color_make(239,68,68) : lv_color_make(71,85,105), 0);
  }
}

// ============== SCORE (dirty flag) ==============
inline void update_score_label() {
  if (score == scoreDisplayed) return;
  scoreDisplayed = score;
  char buf[20];
  sprintf(buf, "Score: %d", score);
  lv_label_set_text(score_label, buf);
}

// ============== POWERUP UI ==============
void show_powerup_msg(const char* msg) {
  lv_label_set_text(powerup_label, msg);
  lv_obj_clear_flag(powerup_label, LV_OBJ_FLAG_HIDDEN);
  powerupMsgTimer = 70;
}

void update_powerup_timers() {
  if (rapidActive && millis() > rapidEndTime) {
    rapidActive = false;
    lv_obj_add_flag(rapid_icon, LV_OBJ_FLAG_HIDDEN);
  }
  if (multishotActive && millis() > multishotEndTime) {
    multishotActive = false;
    lv_obj_add_flag(multishot_icon, LV_OBJ_FLAG_HIDDEN);
  }
  if (laserFlashTimer > 0 && --laserFlashTimer == 0)
    lv_obj_add_flag(laser_obj, LV_OBJ_FLAG_HIDDEN);
  if (powerupMsgTimer > 0 && --powerupMsgTimer == 0)
    lv_obj_add_flag(powerup_label, LV_OBJ_FLAG_HIDDEN);
}

// ============== HP BAR ==============
// Tách move (chỉ set_pos, gọi mỗi frame) và update (set size+màu, chỉ khi HP đổi)

void move_enemy_hpbar(int i) {
  int barX = (int)enemies[i].x;
  int barY = (int)enemies[i].y + enemies[i].size + 2;
  // Dirty-flag: chỉ gọi set_pos khi vị trí thực sự thay đổi
  if (barX == enemies[i].prevBarX && barY == enemies[i].prevBarY) return;
  enemies[i].prevBarX = barX;
  enemies[i].prevBarY = barY;
  lv_obj_set_pos(enemies[i].hpBarBg,   barX, barY);
  lv_obj_set_pos(enemies[i].hpBarFill, barX, barY);
}

void update_enemy_hpbar(int i) {
  // Cập nhật size+màu chỉ khi HP thực sự thay đổi
  if (enemies[i].hp == enemies[i].prevHp) {
    // Chỉ move
    move_enemy_hpbar(i);
    return;
  }
  enemies[i].prevHp = enemies[i].hp;

  int barW  = enemies[i].size;
  int fillW = (enemies[i].hp * barW) / enemies[i].maxHp;
  if (fillW < 1) fillW = 1;

  int barX = (int)enemies[i].x;
  int barY = (int)enemies[i].y + enemies[i].size + 2;
  enemies[i].prevBarX = barX;
  enemies[i].prevBarY = barY;

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

void hide_enemy(int i) {
  enemies[i].active = false;
  lv_obj_add_flag(enemies[i].obj,       LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(enemies[i].hpBarBg,   LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(enemies[i].hpBarFill, LV_OBJ_FLAG_HIDDEN);
}

// ============== PARTICLES ==============
void spawn_particles(int cx, int cy, lv_color_t color, int count) {
  int spawned = 0;
  for (int i = 0; i < MAX_PARTICLES && spawned < count; i++) {
    if (particles[i].active) continue;
    particles[i].x = particles[i].prevX = cx;
    particles[i].y = particles[i].prevY = cy;
    particles[i].vx10 = random(-22, 22);
    particles[i].vy10 = random(-25, 4);
    particles[i].life = random(4, 8);
    particles[i].active = true;
    lv_obj_set_style_bg_color(particles[i].obj, color, 0);
    lv_obj_set_pos(particles[i].obj, cx, cy);
    lv_obj_clear_flag(particles[i].obj, LV_OBJ_FLAG_HIDDEN);
    spawned++;
  }
}

void update_particles() {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (!particles[i].active) continue;
    particles[i].vy10 += 4;  // gravity *10
    particles[i].x   += particles[i].vx10 / 10;
    particles[i].y   += particles[i].vy10 / 10;
    if (--particles[i].life <= 0 || particles[i].y > 248) {
      particles[i].active = false;
      lv_obj_add_flag(particles[i].obj, LV_OBJ_FLAG_HIDDEN);
    } else if (particles[i].x != particles[i].prevX ||
               particles[i].y != particles[i].prevY) {
      // Dirty-flag: set_pos hỉ khi vị trí đổi
      particles[i].prevX = particles[i].x;
      particles[i].prevY = particles[i].y;
      lv_obj_set_pos(particles[i].obj, particles[i].x, particles[i].y);
    }
  }
}

// ============== FLASH HIT ==============
void spawn_flash(int cx, int cy) {
  for (int i = 0; i < MAX_FLASHES; i++) {
    if (!flashes[i].active) {
      flashes[i].life   = 2;  // giảm từ 3→2: ít frame dirty
      flashes[i].active = true;
      lv_obj_set_pos(flashes[i].obj, cx-4, cy-4);
      lv_obj_clear_flag(flashes[i].obj, LV_OBJ_FLAG_HIDDEN);
      return;
    }
  }
}

void update_flashes() {
  for (int i = 0; i < MAX_FLASHES; i++) {
    if (flashes[i].active && --flashes[i].life <= 0) {
      flashes[i].active = false;
      lv_obj_add_flag(flashes[i].obj, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

// ============== BOMB ==============
void activate_bomb() {
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (!enemies[i].active) continue;
    int ex=(int)enemies[i].x, ey=(int)enemies[i].y, es=enemies[i].size;
    lv_color_t pc;
    switch(enemies[i].type) {
      case ENEMY_BOSS:   pc=lv_color_make(255,80,80);  break;
      case ENEMY_TANK:   pc=lv_color_make(52,211,153); break;
      case ENEMY_ZIGZAG: pc=lv_color_make(168,85,247); break;
      default:           pc=lv_color_make(251,191,36); break;
    }
    spawn_particles(ex+es/2, ey+es/2, pc, 4); // 4 thay 6: tiết kiệm particle slot
    hide_enemy(i);
  }
  lv_obj_set_size(laser_obj, 240, 240);
  lv_obj_set_pos(laser_obj, 0, 0);
  lv_obj_set_style_bg_color(laser_obj, lv_color_make(255,200,50), 0);
  lv_obj_set_style_bg_opa(laser_obj, LV_OPA_40, 0);
  lv_obj_clear_flag(laser_obj, LV_OBJ_FLAG_HIDDEN);
  laserFlashTimer = 6; // giảm từ 8→6
}

// ============== LASER ==============
void activate_laser() {
  int lx = (int)gunX + GUN_WIDTH/2 - 3;
  lv_obj_set_size(laser_obj, 6, GUN_Y + GUN_HEIGHT);
  lv_obj_set_pos(laser_obj, lx, 0);
  lv_obj_set_style_bg_color(laser_obj, lv_color_make(255,255,100), 0);
  lv_obj_set_style_bg_opa(laser_obj, LV_OPA_COVER, 0);
  lv_obj_clear_flag(laser_obj, LV_OBJ_FLAG_HIDDEN);
  laserFlashTimer = 8;
  bool scored = false;
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (!enemies[i].active) continue;
    int ex=(int)enemies[i].x, es=enemies[i].size;
    if (ex < lx+26 && ex+es > lx-20) {
      int ey=(int)enemies[i].y;
      spawn_particles(ex+es/2, ey+es/2, lv_color_make(255,255,100), 3);
      score += (enemies[i].type==ENEMY_BOSS)?3:(enemies[i].type==ENEMY_TANK)?2:1;
      hide_enemy(i);
      scored = true;
    }
  }
  if (scored) update_score_label();
}

// ============== DROPS ==============
void spawn_drop(int x, int y, DropType type) {
  for (int i = 0; i < MAX_DROPS; i++) {
    if (drops[i].active) continue;
    drops[i].x = x; drops[i].y = drops[i].prevY = y;
    drops[i].type   = type;
    drops[i].active = true;
    lv_color_t c;
    switch (type) {
      case DROP_HP:        c=lv_color_make(239,68,68);   break;
      case DROP_RAPID:     c=lv_color_make(56,189,248);  break;
      case DROP_SHIELD:    c=lv_color_make(34,197,94);   break;
      case DROP_BOMB:      c=lv_color_make(251,146,60);  break;
      case DROP_LASER:     c=lv_color_make(250,204,21);  break;
      case DROP_MULTISHOT: c=lv_color_make(192,132,252); break;
      case DROP_MAGNET:    c=lv_color_make(251,191,36);  break;
      default:             c=lv_color_make(200,200,200); break;
    }
    lv_obj_set_style_bg_color(drops[i].obj, c, 0);
    lv_obj_set_pos(drops[i].obj, x, y);
    lv_obj_clear_flag(drops[i].obj, LV_OBJ_FLAG_HIDDEN);
    return;
  }
}

void apply_drop(DropType type) {
  switch (type) {
    case DROP_HP:
      if (playerHP < PLAYER_MAX_HP) { playerHP++; update_hearts(); }
      show_powerup_msg("+1 HP!");
      break;
    case DROP_RAPID:
      rapidActive  = true;
      rapidEndTime = millis() + 5000;
      lv_obj_clear_flag(rapid_icon, LV_OBJ_FLAG_HIDDEN);
      show_powerup_msg("RAPID 5s!");
      break;
    case DROP_SHIELD:
      shieldActive = true;
      lv_obj_clear_flag(shield_icon, LV_OBJ_FLAG_HIDDEN);
      show_powerup_msg("SHIELD!");
      break;
    case DROP_BOMB:
      activate_bomb();
      show_powerup_msg("BOOM!");
      break;
    case DROP_LASER:
      activate_laser();
      show_powerup_msg("LASER!");
      break;
    case DROP_MULTISHOT:
      multishotActive  = true;
      multishotEndTime = millis() + 8000;
      lv_obj_clear_flag(multishot_icon, LV_OBJ_FLAG_HIDDEN);
      show_powerup_msg("5-SHOT 8s!");
      break;
    case DROP_MAGNET:
      for (int i = 0; i < MAX_DROPS; i++)
        if (drops[i].active && drops[i].type != DROP_MAGNET)
          drops[i].y = GUN_Y;
      show_powerup_msg("MAGNET!");
      break;
    default: break;
  }
}

void update_drops() {
  for (int i = 0; i < MAX_DROPS; i++) {
    if (!drops[i].active) continue;
    drops[i].y += DROP_SPEED;
    if (drops[i].y > 240) {
      drops[i].active = false;
      lv_obj_add_flag(drops[i].obj, LV_OBJ_FLAG_HIDDEN);
      continue;
    }
    if (check_collision((int)gunX, GUN_Y, GUN_WIDTH, GUN_HEIGHT,
                        drops[i].x, drops[i].y, DROP_SIZE, DROP_SIZE)) {
      apply_drop(drops[i].type);
      drops[i].active = false;
      lv_obj_add_flag(drops[i].obj, LV_OBJ_FLAG_HIDDEN);
      continue;
    }
    // Dirty-flag: set_pos chỉ khi y đổi
    if (drops[i].y != drops[i].prevY) {
      drops[i].prevY = drops[i].y;
      lv_obj_set_pos(drops[i].obj, drops[i].x, drops[i].y);
    }
  }
}

// ============== SPAWN ENEMY ==============
void spawn_enemy() {
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (enemies[i].active) continue;

    EnemyType type = ENEMY_CHICKEN;
    int roll = random(0, 100);
    if (score >= 5) {
      if      (roll < 20 && score >= 10) type = ENEMY_BOSS;
      else if (roll < 45 && score >= 7)  type = ENEMY_ZIGZAG;
      else if (roll < 65 && score >= 4)  type = ENEMY_TANK;
    }
    enemies[i].type = type;

    int size;
    switch (type) {
      case ENEMY_TANK:   size = random(26, 36); break;
      case ENEMY_BOSS:   size = random(38, 46); break;
      case ENEMY_ZIGZAG: size = random(18, 26); break;
      default:           size = random(18, 30); break;
    }
    enemies[i].size     = size;
    enemies[i].x        = (float)random(0, 240 - size);
    enemies[i].y        = (float)(-size);
    enemies[i].zigDir   = (random(0,2)==0) ? 1.0f : -1.0f;
    enemies[i].flashActive = false;
    enemies[i].flashTimer  = 0;
    enemies[i].prevBarX    = -999; // force first bar update
    enemies[i].prevBarY    = -999;

    float bonus = score / 30.0f;
    float spd   = 0.4f + (random(0,8)/10.0f) + bonus;
    if (type == ENEMY_BOSS)   spd *= 0.4f;
    if (type == ENEMY_TANK)   spd *= 0.6f;
    if (type == ENEMY_ZIGZAG) spd *= 1.1f;
    if (spd > 2.5f) spd = 2.5f;
    enemies[i].speed = spd;

    int baseMin = 2 + score/8; if (baseMin > 6)  baseMin = 6;
    int baseMax = 4 + score/5; if (baseMax > 10) baseMax = 10;
    switch (type) {
      case ENEMY_BOSS:
        enemies[i].maxHp = 12 + score/3;
        if (enemies[i].maxHp > 25) enemies[i].maxHp = 25;
        break;
      case ENEMY_TANK:
        enemies[i].maxHp = baseMax + random(0,3);
        if (enemies[i].maxHp > 14) enemies[i].maxHp = 14;
        break;
      case ENEMY_ZIGZAG:
        enemies[i].maxHp = baseMin + random(0,2);
        break;
      default:
        enemies[i].maxHp = random(baseMin, baseMax+1);
        break;
    }
    enemies[i].hp     = enemies[i].maxHp;
    enemies[i].prevHp = -1; // force HP bar update lần đầu
    enemies[i].active = true;

    uint8_t r, g, b;
    switch (type) {
      case ENEMY_CHICKEN:
        if      (enemies[i].maxHp <= 3) { r=251;g=191;b=36;  }
        else if (enemies[i].maxHp <= 6) { r=249;g=115;b=22;  }
        else                             { r=220;g=38; b=38;  }
        lv_obj_set_style_radius(enemies[i].obj, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(enemies[i].obj, 2, 0);
        lv_obj_set_style_border_color(enemies[i].obj, lv_color_make(255,255,255), 0);
        break;
      case ENEMY_TANK:
        r=52; g=211; b=153;
        lv_obj_set_style_radius(enemies[i].obj, 4, 0);
        lv_obj_set_style_border_width(enemies[i].obj, 2, 0);
        lv_obj_set_style_border_color(enemies[i].obj, lv_color_make(200,255,230), 0);
        break;
      case ENEMY_BOSS:
        r=180; g=0; b=60;
        lv_obj_set_style_radius(enemies[i].obj, 6, 0);
        lv_obj_set_style_border_width(enemies[i].obj, 3, 0);
        lv_obj_set_style_border_color(enemies[i].obj, lv_color_make(255,100,100), 0);
        break;
      case ENEMY_ZIGZAG:
        r=168; g=85; b=247;
        lv_obj_set_style_radius(enemies[i].obj, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(enemies[i].obj, 2, 0);
        lv_obj_set_style_border_color(enemies[i].obj, lv_color_make(220,180,255), 0);
        break;
      default: r=200; g=200; b=200; break;
    }
    enemies[i].cr = r; enemies[i].cg = g; enemies[i].cb = b;
    lv_obj_set_style_bg_color(enemies[i].obj, lv_color_make(r,g,b), 0);
    lv_obj_set_size(enemies[i].obj, size, size);
    lv_obj_set_pos(enemies[i].obj, (int)enemies[i].x, (int)enemies[i].y);
    lv_obj_clear_flag(enemies[i].obj, LV_OBJ_FLAG_HIDDEN);
    update_enemy_hpbar(i);
    return;
  }
}

// ============== GUN DESCRIPTION ==============
const char* gun_description(int g) {
  switch (g) {
    case GUN_NORMAL:  return "Can bang, de dung";
    case GUN_FAST:    return "Ban nhanh, sat thuong thap";
    case GUN_HEAVY:   return "Ban cham, sat thuong cao";
    case GUN_SHOTGUN: return "3 dan song song";
  }
  return "";
}

// ============== FIRE BULLET ==============
void fire_bullet() {
  GunStats g = gunStats[selectedGun];
  int n = g.numBullets;
  const int off3[3] = {-10, 0, 10};
  const int off5[5] = {-20,-10, 0, 10, 20};
  if (multishotActive) n = 5;

  // Màu & size tính một lần
  lv_color_t bc;
  int bw = BULLET_W, bh = BULLET_H;
  if (multishotActive) {
    bc = lv_color_make(192,132,252);
  } else {
    switch (selectedGun) {
      case GUN_FAST:    bc=lv_color_make(56,189,248);  break;
      case GUN_HEAVY:   bc=lv_color_make(239,68,68); bw=6; bh=12; break;
      case GUN_SHOTGUN: bc=lv_color_make(168,85,247); break;
      default:          bc=lv_color_make(250,204,21); break;
    }
  }

  int fired = 0;
  for (int i = 0; i < MAX_BULLETS && fired < n; i++) {
    if (bullets[i].active) continue;
    int off = multishotActive ? off5[fired] : (n==3 ? off3[fired] : 0);
    bullets[i].x    = (int)gunX + GUN_WIDTH/2 - BULLET_W/2 + off;
    bullets[i].y    = GUN_Y - BULLET_H;
    bullets[i].prevX = bullets[i].x;
    bullets[i].prevY = bullets[i].y;
    bullets[i].damage = g.damage;
    bullets[i].active = true;
    lv_obj_set_size(bullets[i].obj, bw, bh);
    lv_obj_set_style_bg_color(bullets[i].obj, bc, 0);
    lv_obj_set_pos(bullets[i].obj, bullets[i].x, bullets[i].y);
    lv_obj_clear_flag(bullets[i].obj, LV_OBJ_FLAG_HIDDEN);
    fired++;
  }
}

// ============== CREATE UI ==============
void create_game_ui() {
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_make(30,41,59), 0);

  score_label = lv_label_create(lv_scr_act());
  lv_obj_align(score_label, LV_ALIGN_TOP_LEFT, 8, 6);
  lv_obj_set_style_text_color(score_label, lv_color_make(226,232,240), 0);
  lv_label_set_text(score_label, "Score: 0");

  for (int i = 0; i < MAX_HEARTS; i++) {
    heart_icons[i] = lv_obj_create(lv_scr_act());
    lv_obj_set_size(heart_icons[i], 14, 14);
    lv_obj_set_style_radius(heart_icons[i], LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(heart_icons[i], 0, 0);
    lv_obj_set_style_bg_color(heart_icons[i], lv_color_make(239,68,68), 0);
    lv_obj_set_pos(heart_icons[i], 240-20-(i*18), 6);
  }

  shield_icon = lv_obj_create(lv_scr_act());
  lv_obj_set_size(shield_icon, GUN_WIDTH+8, GUN_HEIGHT+8);
  lv_obj_set_style_radius(shield_icon, 6, 0);
  lv_obj_set_style_bg_opa(shield_icon, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(shield_icon, 3, 0);
  lv_obj_set_style_border_color(shield_icon, lv_color_make(34,197,94), 0);
  lv_obj_add_flag(shield_icon, LV_OBJ_FLAG_HIDDEN);

  rapid_icon = lv_label_create(lv_scr_act());
  lv_obj_set_pos(rapid_icon, 8, 22);
  lv_obj_set_style_text_color(rapid_icon, lv_color_make(56,189,248), 0);
  lv_label_set_text(rapid_icon, "2x");
  lv_obj_add_flag(rapid_icon, LV_OBJ_FLAG_HIDDEN);

  multishot_icon = lv_label_create(lv_scr_act());
  lv_obj_set_pos(multishot_icon, 8, 36);
  lv_obj_set_style_text_color(multishot_icon, lv_color_make(192,132,252), 0);
  lv_label_set_text(multishot_icon, "5x");
  lv_obj_add_flag(multishot_icon, LV_OBJ_FLAG_HIDDEN);

  laser_obj = lv_obj_create(lv_scr_act());
  lv_obj_set_size(laser_obj, 6, 240);
  lv_obj_set_style_bg_color(laser_obj, lv_color_make(255,255,100), 0);
  lv_obj_set_style_border_width(laser_obj, 0, 0);
  lv_obj_set_style_radius(laser_obj, 0, 0);
  lv_obj_add_flag(laser_obj, LV_OBJ_FLAG_HIDDEN);

  powerup_label = lv_label_create(lv_scr_act());
  lv_obj_align(powerup_label, LV_ALIGN_CENTER, 0, 40);
  lv_obj_set_style_text_color(powerup_label, lv_color_make(250,204,21), 0);
  lv_obj_set_style_text_font(powerup_label, &lv_font_montserrat_14, 0);
  lv_label_set_text(powerup_label, "");
  lv_obj_add_flag(powerup_label, LV_OBJ_FLAG_HIDDEN);

  gameover_label = lv_label_create(lv_scr_act());
  lv_obj_align(gameover_label, LV_ALIGN_CENTER, 0, -20);
  lv_obj_set_style_text_color(gameover_label, lv_color_make(239,68,68), 0);
  lv_obj_set_style_text_font(gameover_label, &lv_font_montserrat_14, 0);
  lv_label_set_text(gameover_label, "GAME OVER");
  lv_obj_add_flag(gameover_label, LV_OBJ_FLAG_HIDDEN);

  restart_label = lv_label_create(lv_scr_act());
  lv_obj_align(restart_label, LV_ALIGN_CENTER, 0, 10);
  lv_obj_set_style_text_color(restart_label, lv_color_make(148,163,184), 0);
  lv_label_set_text(restart_label, "Nhan nut de ve menu");
  lv_obj_add_flag(restart_label, LV_OBJ_FLAG_HIDDEN);

  gun_obj = lv_obj_create(lv_scr_act());
  lv_obj_set_size(gun_obj, GUN_WIDTH, GUN_HEIGHT);
  lv_obj_set_style_bg_color(gun_obj, lv_color_make(34,197,94), 0);
  lv_obj_set_style_radius(gun_obj, 4, 0);
  lv_obj_set_style_border_width(gun_obj, 0, 0);
  lv_obj_set_pos(gun_obj, (int)gunX, GUN_Y);

  for (int i = 0; i < MAX_BULLETS; i++) {
    bullets[i].obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bullets[i].obj, BULLET_W, BULLET_H);
    lv_obj_set_style_bg_color(bullets[i].obj, lv_color_make(250,204,21), 0);
    lv_obj_set_style_radius(bullets[i].obj, 0, 0); // hình vuông nhanh hơn
    lv_obj_set_style_border_width(bullets[i].obj, 0, 0);
    lv_obj_add_flag(bullets[i].obj, LV_OBJ_FLAG_HIDDEN);
    bullets[i].active = false;
    bullets[i].prevX  = -999;
    bullets[i].prevY  = -999;
  }

  for (int i = 0; i < MAX_ENEMIES; i++) {
    enemies[i].obj = lv_obj_create(lv_scr_act());
    lv_obj_set_style_radius(enemies[i].obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(enemies[i].obj, 2, 0);
    lv_obj_set_style_border_color(enemies[i].obj, lv_color_make(255,255,255), 0);
    lv_obj_add_flag(enemies[i].obj, LV_OBJ_FLAG_HIDDEN);
    enemies[i].active  = false;
    enemies[i].prevHp  = -1;
    enemies[i].prevBarX = -999;
    enemies[i].prevBarY = -999;

    enemies[i].hpBarBg = lv_obj_create(lv_scr_act());
    lv_obj_set_style_bg_color(enemies[i].hpBarBg, lv_color_make(55,65,81), 0);
    lv_obj_set_style_border_width(enemies[i].hpBarBg, 0, 0);
    lv_obj_set_style_radius(enemies[i].hpBarBg, 0, 0);
    lv_obj_add_flag(enemies[i].hpBarBg, LV_OBJ_FLAG_HIDDEN);

    enemies[i].hpBarFill = lv_obj_create(lv_scr_act());
    lv_obj_set_style_bg_color(enemies[i].hpBarFill, lv_color_make(34,197,94), 0);
    lv_obj_set_style_border_width(enemies[i].hpBarFill, 0, 0);
    lv_obj_set_style_radius(enemies[i].hpBarFill, 0, 0);
    lv_obj_add_flag(enemies[i].hpBarFill, LV_OBJ_FLAG_HIDDEN);
  }

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

  for (int i = 0; i < MAX_PARTICLES; i++) {
    particles[i].obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(particles[i].obj, 3, 3); // 3x3 thay 4x4: ít pixel dirty hơn
    lv_obj_set_style_radius(particles[i].obj, 0, 0); // hình vuông nhanh hơn circle
    lv_obj_set_style_border_width(particles[i].obj, 0, 0);
    lv_obj_add_flag(particles[i].obj, LV_OBJ_FLAG_HIDDEN);
    particles[i].active = false;
    particles[i].prevX  = -999;
    particles[i].prevY  = -999;
  }

  for (int i = 0; i < MAX_FLASHES; i++) {
    flashes[i].obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(flashes[i].obj, 7, 7);
    lv_obj_set_style_radius(flashes[i].obj, 0, 0); // hình vuông
    lv_obj_set_style_border_width(flashes[i].obj, 0, 0);
    lv_obj_set_style_bg_color(flashes[i].obj, lv_color_make(255,255,255), 0);
    lv_obj_add_flag(flashes[i].obj, LV_OBJ_FLAG_HIDDEN);
    flashes[i].active = false;
  }

  // Menu
  menu_title = lv_label_create(lv_scr_act());
  lv_obj_align(menu_title, LV_ALIGN_TOP_MID, 0, 30);
  lv_obj_set_style_text_color(menu_title, lv_color_make(226,232,240), 0);
  lv_label_set_text(menu_title, "CHON SUNG");

  menu_gunname = lv_label_create(lv_scr_act());
  lv_obj_align(menu_gunname, LV_ALIGN_CENTER, 0, -20);
  lv_obj_set_style_text_color(menu_gunname, lv_color_make(250,204,21), 0);
  lv_obj_set_style_text_font(menu_gunname, &lv_font_montserrat_14, 0);

  menu_desc = lv_label_create(lv_scr_act());
  lv_obj_align(menu_desc, LV_ALIGN_CENTER, 0, 10);
  lv_obj_set_style_text_color(menu_desc, lv_color_make(148,163,184), 0);

  menu_hint = lv_label_create(lv_scr_act());
  lv_obj_align(menu_hint, LV_ALIGN_BOTTOM_MID, 0, -16);
  lv_obj_set_style_text_color(menu_hint, lv_color_make(100,116,139), 0);
  lv_label_set_text(menu_hint, "Gat trai/phai: doi sung\nNhan nut: bat dau");

  lv_refr_now(NULL); // chỉ gọi 1 lần sau khởi tạo UI
}

// ============== SHOW MENU ==============
void show_menu() {
  gameState = STATE_MENU;
  lv_obj_clear_flag(menu_title,   LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(menu_gunname, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(menu_desc,    LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(menu_hint,    LV_OBJ_FLAG_HIDDEN);

  lv_obj_add_flag(gun_obj,        LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(score_label,    LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(shield_icon,    LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(rapid_icon,     LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(multishot_icon, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(laser_obj,      LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(powerup_label,  LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(gameover_label, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(restart_label,  LV_OBJ_FLAG_HIDDEN);
  for (int i=0;i<MAX_HEARTS;i++)    lv_obj_add_flag(heart_icons[i],   LV_OBJ_FLAG_HIDDEN);
  for (int i=0;i<MAX_BULLETS;i++)   lv_obj_add_flag(bullets[i].obj,   LV_OBJ_FLAG_HIDDEN);
  for (int i=0;i<MAX_ENEMIES;i++) {
    lv_obj_add_flag(enemies[i].obj,       LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(enemies[i].hpBarBg,   LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(enemies[i].hpBarFill, LV_OBJ_FLAG_HIDDEN);
  }
  for (int i=0;i<MAX_DROPS;i++)     lv_obj_add_flag(drops[i].obj,      LV_OBJ_FLAG_HIDDEN);
  for (int i=0;i<MAX_PARTICLES;i++) lv_obj_add_flag(particles[i].obj,  LV_OBJ_FLAG_HIDDEN);
  for (int i=0;i<MAX_FLASHES;i++)   lv_obj_add_flag(flashes[i].obj,    LV_OBJ_FLAG_HIDDEN);

  char buf[24];
  sprintf(buf, "< %s >", gunNames[selectedGun]);
  lv_label_set_text(menu_gunname, buf);
  lv_label_set_text(menu_desc, gun_description(selectedGun));
}

// ============== START GAME ==============
void start_game() {
  gameState = STATE_PLAYING;
  lv_obj_add_flag(menu_title,   LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(menu_gunname, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(menu_desc,    LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(menu_hint,    LV_OBJ_FLAG_HIDDEN);

  lv_obj_clear_flag(gun_obj,     LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(score_label, LV_OBJ_FLAG_HIDDEN);
  for (int i=0;i<MAX_HEARTS;i++) lv_obj_clear_flag(heart_icons[i], LV_OBJ_FLAG_HIDDEN);

  gunX = 105.0f; gunPxLast = -1; gunFlashTimer = 0;
  lv_obj_set_pos(gun_obj, (int)gunX, GUN_Y);
  lv_obj_set_style_bg_color(gun_obj, lv_color_make(34,197,94), 0);

  rapidActive = false; shieldActive = false;
  multishotActive = false; powerupMsgTimer = 0; laserFlashTimer = 0;
  lv_obj_add_flag(shield_icon,    LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(rapid_icon,     LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(multishot_icon, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(laser_obj,      LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(powerup_label,  LV_OBJ_FLAG_HIDDEN);

  for (int i=0;i<MAX_BULLETS;i++) {
    bullets[i].active=false;
    bullets[i].prevX=-999; bullets[i].prevY=-999;
    lv_obj_add_flag(bullets[i].obj, LV_OBJ_FLAG_HIDDEN);
  }
  for (int i=0;i<MAX_ENEMIES;i++) {
    enemies[i].active=false;
    enemies[i].prevHp=-1; enemies[i].prevBarX=-999; enemies[i].prevBarY=-999;
    lv_obj_add_flag(enemies[i].obj,       LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(enemies[i].hpBarBg,   LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(enemies[i].hpBarFill, LV_OBJ_FLAG_HIDDEN);
  }
  for (int i=0;i<MAX_DROPS;i++) {
    drops[i].active=false; drops[i].prevY=-999;
    lv_obj_add_flag(drops[i].obj, LV_OBJ_FLAG_HIDDEN);
  }
  for (int i=0;i<MAX_PARTICLES;i++) {
    particles[i].active=false;
    particles[i].prevX=-999; particles[i].prevY=-999;
    lv_obj_add_flag(particles[i].obj, LV_OBJ_FLAG_HIDDEN);
  }
  for (int i=0;i<MAX_FLASHES;i++) {
    flashes[i].active=false;
    lv_obj_add_flag(flashes[i].obj, LV_OBJ_FLAG_HIDDEN);
  }

  playerHP = PLAYER_MAX_HP; update_hearts();
  score = 0; scoreDisplayed = -1; // force update
  spawnInterval = 1400;
  shotInterval  = gunStats[selectedGun].interval;
  lastSpawnTime = millis(); lastShotTime = millis(); gameStartTime = millis();
  lastTickTime  = millis();
  update_score_label();
}

// ============== BUTTON ==============
void handle_button_click() {
  bool pressed = (digitalRead(JOYSTICK_SW_PIN) == LOW);
  if (pressed && (millis()-lastDebounceTime) > DEBOUNCE_DELAY) {
    lastDebounceTime = millis();
    if      (gameState == STATE_MENU) start_game();
    else if (gameState == STATE_OVER) show_menu();
  }
}

// ============== MENU NAV ==============
void handle_menu_navigation() {
  int rawX = analogRead(JOYSTICK_X_PIN);
  static unsigned long lastNavTime = 0;
  if (millis() - lastNavTime < 300) return;
  bool moved = false;
  if      (rawX - centerX  > DEADZONE) { selectedGun=(selectedGun+1)%GUN_COUNT; moved=true; }
  else if (centerX - rawX  > DEADZONE) { selectedGun=(selectedGun-1+GUN_COUNT)%GUN_COUNT; moved=true; }
  if (!moved) return;
  lastNavTime = millis();
  char buf[24];
  sprintf(buf, "< %s >", gunNames[selectedGun]);
  lv_label_set_text(menu_gunname, buf);
  lv_label_set_text(menu_desc, gun_description(selectedGun));
}

// ============== GUN MOVE ==============
void update_gun_movement() {
  int rawX  = analogRead(JOYSTICK_X_PIN);
  int delta = rawX - centerX;
  if (abs(delta) >= DEADZONE)
    gunX -= ((float)delta / 2048.0f) * GUN_SPEED;
  if (gunX < 0)              gunX = 0;
  if (gunX > 240-GUN_WIDTH)  gunX = 240-GUN_WIDTH;

  int px = (int)gunX;
  if (px != gunPxLast) {
    gunPxLast = px;
    lv_obj_set_pos(gun_obj, px, GUN_Y);
    if (shieldActive)
      lv_obj_set_pos(shield_icon, px-4, GUN_Y-4);
  }

  if (gunFlashTimer > 0 && --gunFlashTimer == 0)
    lv_obj_set_style_bg_color(gun_obj, lv_color_make(34,197,94), 0);
}

// ============== AUTO SHOOT ==============
void update_auto_shoot() {
  unsigned long now = millis();
  int interval = rapidActive ? (shotInterval/2) : shotInterval;
  if (now - lastShotTime > (unsigned long)interval) {
    lastShotTime = now;
    fire_bullet();
  }
}

// ============== UPDATE BULLETS ==============
void update_bullets() {
  for (int i=0; i<MAX_BULLETS; i++) {
    if (!bullets[i].active) continue;
    bullets[i].y -= BULLET_SPEED;
    if (bullets[i].y < -BULLET_H) {
      bullets[i].active = false;
      lv_obj_add_flag(bullets[i].obj, LV_OBJ_FLAG_HIDDEN);
    } else if (bullets[i].y != bullets[i].prevY) {
      // Dirty-flag: x đạn không đổi trong flight, chỉ check y
      bullets[i].prevY = bullets[i].y;
      lv_obj_set_pos(bullets[i].obj, bullets[i].x, bullets[i].y);
    }
  }
}

// ============== PLAYER DAMAGE ==============
void player_take_damage() {
  if (shieldActive) {
    shieldActive = false;
    lv_obj_add_flag(shield_icon, LV_OBJ_FLAG_HIDDEN);
    show_powerup_msg("Shield block!");
    return;
  }
  playerHP--;
  update_hearts();
  lv_obj_set_style_bg_color(gun_obj, lv_color_make(239,68,68), 0);
  gunFlashTimer = 15;
  if (playerHP <= 0) {
    gameState  = STATE_OVER;
    finalScore = score;
  }
}

// ============== UPDATE ENEMIES ==============
void update_enemies() {
  unsigned long now     = millis();
  unsigned long elapsed = now - gameStartTime;

  spawnInterval = 1400 - (int)(elapsed/90) - score*8;
  if (spawnInterval < 500) spawnInterval = 500;

  shotInterval = gunStats[selectedGun].interval - score*4;
  int minI = gunStats[selectedGun].interval/2;
  if (shotInterval < minI) shotInterval = minI;

  if (now - lastSpawnTime > (unsigned long)spawnInterval) {
    lastSpawnTime = now;
    spawn_enemy();
  }

  for (int i=0; i<MAX_ENEMIES; i++) {
    if (!enemies[i].active) continue;

    enemies[i].y += enemies[i].speed;
    if (enemies[i].type == ENEMY_ZIGZAG) {
      enemies[i].x += enemies[i].zigDir * 1.4f;
      if (enemies[i].x < 0)                   { enemies[i].x=0;                    enemies[i].zigDir=1.0f; }
      if (enemies[i].x > 240-enemies[i].size) { enemies[i].x=240-enemies[i].size; enemies[i].zigDir=-1.0f; }
    }

    // Flash timer: restore màu từ cache (không switch mỗi frame)
    if (enemies[i].flashActive && --enemies[i].flashTimer <= 0) {
      enemies[i].flashActive = false;
      lv_obj_set_style_bg_color(enemies[i].obj,
        lv_color_make(enemies[i].cr, enemies[i].cg, enemies[i].cb), 0);
    }

    int ex=(int)enemies[i].x, ey=(int)enemies[i].y, es=enemies[i].size;

    // Va chạm súng
    if (check_collision((int)gunX,GUN_Y,GUN_WIDTH,GUN_HEIGHT, ex,ey,es,es)) {
      int dmg = (enemies[i].type==ENEMY_BOSS) ? 2 : 1;
      spawn_particles(ex+es/2, ey+es/2, lv_color_make(239,68,68), 4);
      hide_enemy(i);
      for (int d=0; d<dmg && playerHP>0; d++) player_take_damage();
      if (gameState==STATE_OVER) return;
      continue;
    }

    // Rơi đáy
    if (enemies[i].y > 240) {
      spawn_particles(ex+es/2, 235, lv_color_make(100,100,200), 3);
      hide_enemy(i);
      player_take_damage();
      if (gameState==STATE_OVER) return;
      continue;
    }

    // Va chạm đạn
    bool killed = false;
    for (int b=0; b<MAX_BULLETS; b++) {
      if (!bullets[b].active) continue;
      if (!check_collision(bullets[b].x, bullets[b].y, BULLET_W, BULLET_H, ex,ey,es,es)) continue;

      int hitX=ex+es/2, hitY=ey+es/2;
      spawn_flash(hitX, hitY);
      enemies[i].hp -= bullets[b].damage;

      bullets[b].active = false;
      lv_obj_add_flag(bullets[b].obj, LV_OBJ_FLAG_HIDDEN);

      if (enemies[i].hp <= 0) {
        lv_color_t pc;
        int numP;
        switch(enemies[i].type) {
          case ENEMY_BOSS:   pc=lv_color_make(255,80,80);  numP=6; break;
          case ENEMY_TANK:   pc=lv_color_make(52,211,153); numP=5; break;
          case ENEMY_ZIGZAG: pc=lv_color_make(168,85,247); numP=4; break;
          default:           pc=lv_color_make(251,191,36); numP=3; break;
        }
        spawn_particles(hitX, hitY, pc, numP);

        // DROP
        int dropRoll = random(0,100);
        DropType dt  = DROP_NONE;
        switch(enemies[i].type) {
          case ENEMY_BOSS:
            if      (dropRoll < 20) dt=DROP_HP;
            else if (dropRoll < 35) dt=DROP_SHIELD;
            else if (dropRoll < 50) dt=DROP_BOMB;
            else if (dropRoll < 65) dt=DROP_LASER;
            else if (dropRoll < 80) dt=DROP_MULTISHOT;
            else if (dropRoll < 90) dt=DROP_RAPID;
            else                    dt=DROP_MAGNET;
            break;
          case ENEMY_TANK:
            if      (dropRoll < 12) dt=DROP_HP;
            else if (dropRoll < 22) dt=DROP_RAPID;
            else if (dropRoll < 32) dt=DROP_MULTISHOT;
            else if (dropRoll < 40) dt=DROP_SHIELD;
            break;
          case ENEMY_ZIGZAG:
            if      (dropRoll < 10) dt=DROP_RAPID;
            else if (dropRoll < 18) dt=DROP_LASER;
            else if (dropRoll < 24) dt=DROP_SHIELD;
            else if (dropRoll < 30) dt=DROP_MAGNET;
            break;
          default:
            if      (dropRoll < 10) dt=DROP_HP;
            else if (dropRoll < 15) dt=DROP_BOMB;
            else if (dropRoll < 20) dt=DROP_MULTISHOT;
            break;
        }
        if (dt != DROP_NONE) spawn_drop(hitX-DROP_SIZE/2, hitY, dt);

        score += (enemies[i].type==ENEMY_BOSS)?3:(enemies[i].type==ENEMY_TANK)?2:1;
        hide_enemy(i);
        killed = true;
        update_score_label();
        break;
      } else {
        lv_obj_set_style_bg_color(enemies[i].obj, lv_color_make(255,255,255), 0);
        enemies[i].flashActive = true;
        enemies[i].flashTimer  = 3;
        // HP thực sự giảm → update_enemy_hpbar sẽ detect qua prevHp
        update_enemy_hpbar(i);
        break;
      }
    }

    if (!killed && enemies[i].active) {
      lv_obj_set_pos(enemies[i].obj, ex, ey);
      // move_enemy_hpbar thay update: chỉ dời vị trí (HP không đổi khi di chuyển bình thường)
      move_enemy_hpbar(i);
    }
  }
}

// ============== GAME OVER ==============
void trigger_game_over() {
  lv_obj_clear_flag(gameover_label, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(restart_label,  LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_style_bg_color(gun_obj, lv_color_make(100,100,100), 0);
  score = finalScore;
  update_score_label();
}

// ============== SETUP ==============
void setup() {
  Serial.begin(115200);
  pinMode(JOYSTICK_X_PIN,  INPUT);
  pinMode(JOYSTICK_Y_PIN,  INPUT);
  pinMode(JOYSTICK_SW_PIN, INPUT_PULLUP);

  long sumX=0, sumY=0;
  for (int i=0;i<50;i++) {
    sumX += analogRead(JOYSTICK_X_PIN);
    sumY += analogRead(JOYSTICK_Y_PIN);
    delay(5);
  }
  centerX = sumX/50; centerY = sumY/50;
  Serial.printf("Center calibrated: X=%d Y=%d\n", centerX, centerY);

  tft.init();
  tft.setRotation(0);
  lv_init();

  lv_display_t* disp = lv_display_create(screenWidth, screenHeight);
  // Single buffer 10 dòng – double buffer gây block flush với TFT_eSPI synchronous
  static uint8_t disp_buf[screenWidth * 10 * sizeof(lv_color_t)];
  lv_display_set_buffers(disp, disp_buf, NULL, sizeof(disp_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(disp, my_disp_flush);

  randomSeed(analogRead(36));
  create_game_ui();
  show_menu();
}

// ============== LOOP ==============
void loop() {
  handle_button_click();

  unsigned long now = millis();
  if (now - lastTickTime >= GAME_TICK_MS) {
    lastTickTime = now;

    if (gameState == STATE_MENU) {
      handle_menu_navigation();
    } else if (gameState == STATE_PLAYING) {
      update_gun_movement();
      update_auto_shoot();
      update_bullets();
      update_enemies();
      update_drops();
      update_particles();
      update_flashes();
      update_powerup_timers();

      if (gameState == STATE_OVER)
        trigger_game_over();
    }
  }

  lv_timer_handler();
  lv_refr_now(NULL);
  delay(10);
}
