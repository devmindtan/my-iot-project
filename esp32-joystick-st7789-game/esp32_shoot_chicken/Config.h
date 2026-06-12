#pragma once
#include <lvgl.h>

// ── Màn hình ─────────────────────────────────────────────────────
static const uint16_t SCREEN_W = 240;
static const uint16_t SCREEN_H = 240;

// ── Joystick pins ─────────────────────────────────────────────────
#define JOYSTICK_X_PIN   34
#define JOYSTICK_Y_PIN   35
#define JOYSTICK_SW_PIN  32
const int DEADZONE = 400;

// ── Timing ───────────────────────────────────────────────────────
#define GAME_TICK_MS  14   // ~71fps logic

// ── Player / Gun ─────────────────────────────────────────────────
const int   GUN_WIDTH     = 30;
const int   GUN_HEIGHT    = 14;
const int   GUN_Y         = 220;
const float GUN_SPEED     = 6.0f;
const int   PLAYER_MAX_HP = 3;
#define     MAX_HEARTS    3

// ── Bullet ───────────────────────────────────────────────────────
#define MAX_BULLETS  16
const int BULLET_SPEED = 9;
const int BULLET_W     = 4;
const int BULLET_H     = 10;

// ── Enemy ─────────────────────────────────────────────────────────
#define MAX_ENEMIES 5

// ── Drop ─────────────────────────────────────────────────────────
#define MAX_DROPS  5
const int DROP_SIZE  = 12;
const int DROP_SPEED = 1;

// ── Particle ──────────────────────────────────────────────────────
#define MAX_PARTICLES 8

// ── Flash ─────────────────────────────────────────────────────────
#define MAX_FLASHES 4

// ── Enums ─────────────────────────────────────────────────────────
enum GameState  { STATE_MENU, STATE_PLAYING, STATE_OVER };
enum EnemyType  { ENEMY_CHICKEN, ENEMY_TANK, ENEMY_BOSS, ENEMY_ZIGZAG, ENEMY_TYPE_COUNT };
enum DropType   { DROP_NONE, DROP_HP, DROP_RAPID, DROP_SHIELD,
                  DROP_BOMB, DROP_LASER, DROP_MULTISHOT, DROP_MAGNET };
enum GunType    { GUN_NORMAL, GUN_FAST, GUN_HEAVY, GUN_SHOTGUN, GUN_COUNT };

// ── Structs ───────────────────────────────────────────────────────
struct GunStats {
  int interval;   // ms giữa 2 phát
  int damage;
  int numBullets;
};

// Inline collision helper
inline bool checkCollision(int ax, int ay, int aw, int ah,
                            int bx, int by, int bw, int bh) {
  return (ax < bx+bw && ax+aw > bx && ay < by+bh && ay+ah > by);
}
