#pragma once
#include "Config.h"
#include "DisplayManager.h"
#include "UIManager.h"
#include "Player.h"
#include "BulletPool.h"
#include "EnemyPool.h"
#include "DropPool.h"
#include "ParticlePool.h"
#include "FlashPool.h"

// ── GameManager ───────────────────────────────────────────────────
// Chịu trách nhiệm:
//   - State machine: MENU → PLAYING → GAME OVER → MENU
//   - Điều phối tất cả pool và player mỗi tick
//   - Spawn timing enemy, auto-shoot timing
//   - Xử lý drop pickup (apply_drop logic)
//   - Bomb và Laser effect
//
// QUAN TRỌNG: init() chỉ nhận references — KHÔNG tạo LVGL objects.
//   Tất cả createObjects() phải gọi trong setup() trước khi gọi init().
// Debug tip: đặt Serial.printf trong tick() để trace state transitions
class GameManager {
public:
  unsigned long lastTickTime = 0;

private:
  // ── Pointers đến các hệ thống ─────────────────────────────────
  DisplayManager* _disp      = nullptr;
  UIManager*      _ui        = nullptr;
  Player*         _player    = nullptr;
  BulletPool*     _bullets   = nullptr;
  EnemyPool*      _enemies   = nullptr;
  DropPool*       _drops     = nullptr;
  ParticlePool*   _particles = nullptr;
  FlashPool*      _flashes   = nullptr;

  // ── Game state ────────────────────────────────────────────────
  GameState _state      = STATE_MENU;
  int       _score      = 0;
  int       _finalScore = 0;

  // ── Timing ────────────────────────────────────────────────────
  unsigned long _gameStartTime = 0;
  unsigned long _lastSpawnTime = 0;
  unsigned long _lastShotTime  = 0;
  int           _spawnInterval = 1400;

  // ── Debounce button ───────────────────────────────────────────
  uint32_t       _lastDebounceTime = 0;
  const uint32_t DEBOUNCE_DELAY   = 200;

public:
  // Gọi trong setup() SAU khi tất cả createObjects() đã xong
  // KHÔNG gọi createAll() hay createObjects() trong này
  void init(DisplayManager* d, UIManager* ui,
            Player* p, BulletPool* b, EnemyPool* e,
            DropPool* dr, ParticlePool* pa, FlashPool* fl) {
    _disp = d; _ui = ui; _player = p;
    _bullets = b; _enemies = e; _drops = dr;
    _particles = pa; _flashes = fl;
  }

  // ── Public: điều hướng state ──────────────────────────────────
  void showMenu() {
    _state = STATE_MENU;

    lv_obj_clear_flag(_ui->menuTitle,   LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(_ui->menuGunname, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(_ui->menuDesc,    LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(_ui->menuHint,    LV_OBJ_FLAG_HIDDEN);

    // Gun obj đã được tạo trong setup() → an toàn
    lv_obj_add_flag(_player->gunObj,    LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(_ui->scoreLabel,    LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(_ui->shieldIcon,    LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(_ui->rapidIcon,     LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(_ui->multishotIcon, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(_ui->laserObj,      LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(_ui->powerupLabel,  LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(_ui->gameoverLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(_ui->restartLabel,  LV_OBJ_FLAG_HIDDEN);

    for (int i = 0; i < MAX_HEARTS;    i++) lv_obj_add_flag(_ui->heartIcons[i], LV_OBJ_FLAG_HIDDEN);
    _bullets->hideAll();
    _enemies->hideAll();
    _drops->hideAll();
    _particles->hideAll();
    _flashes->hideAll();

    _ui->setMenuGun(Player::gunNames[_player->selectedGun],
                    Player::gunDescription(_player->selectedGun));
    Serial.println("[GameManager] STATE_MENU");
  }

  void startGame() {
    _state = STATE_PLAYING;
    Serial.println("[GameManager] STATE_PLAYING");

    lv_obj_add_flag(_ui->menuTitle,   LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(_ui->menuGunname, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(_ui->menuDesc,    LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(_ui->menuHint,    LV_OBJ_FLAG_HIDDEN);

    lv_obj_clear_flag(_player->gunObj,  LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(_ui->scoreLabel,  LV_OBJ_FLAG_HIDDEN);
    for (int i = 0; i < MAX_HEARTS; i++)
      lv_obj_clear_flag(_ui->heartIcons[i], LV_OBJ_FLAG_HIDDEN);

    _player->reset();
    _bullets->resetAll();
    _enemies->resetAll();
    _drops->resetAll();
    _particles->resetAll();
    _flashes->resetAll();

    _score = 0; _finalScore = 0;
    _ui->resetScoreDirty();
    _ui->updateHearts(_player->hp);
    _spawnInterval = 1400;
    _player->shotInterval = Player::gunStats[_player->selectedGun].interval;
    _gameStartTime = _lastSpawnTime = _lastShotTime = millis();
    _ui->updateScore(_score);
  }

  // ── Tick chính — gọi mỗi GAME_TICK_MS ────────────────────────
  void tick() {
    if (_state == STATE_MENU) {
      _handleMenuNavigation();
    } else if (_state == STATE_PLAYING) {
      _player->updateMovement();
      _updateAutoShoot();
      _bullets->update();
      _updateEnemies();
      _updateDropsWithPickup();
      _particles->update();
      _flashes->update();
      _player->updatePowerupTimers(_ui);

      if (_state == STATE_OVER) _triggerGameOver();
    }
  }

  // ── Button (gọi mỗi loop — trước tick) ───────────────────────
  void handleButtonClick() {
    bool pressed = (digitalRead(JOYSTICK_SW_PIN) == LOW);
    if (pressed && (millis() - _lastDebounceTime) > DEBOUNCE_DELAY) {
      _lastDebounceTime = millis();
      if      (_state == STATE_MENU) startGame();
      else if (_state == STATE_OVER) showMenu();
    }
  }

private:
  // ── Menu navigation ───────────────────────────────────────────
  void _handleMenuNavigation() {
    static unsigned long lastNavTime = 0;
    if (millis() - lastNavTime < 300) return;
    int rawX = analogRead(JOYSTICK_X_PIN);
    bool moved = false;
    if      (rawX - _player->centerX > DEADZONE) {
      _player->selectedGun = (_player->selectedGun + 1) % GUN_COUNT;
      moved = true;
    } else if (_player->centerX - rawX > DEADZONE) {
      _player->selectedGun = (_player->selectedGun - 1 + GUN_COUNT) % GUN_COUNT;
      moved = true;
    }
    if (!moved) return;
    lastNavTime = millis();
    _ui->setMenuGun(Player::gunNames[_player->selectedGun],
                    Player::gunDescription(_player->selectedGun));
  }

  // ── Auto shoot ────────────────────────────────────────────────
  void _updateAutoShoot() {
    unsigned long now = millis();
    if (now - _lastShotTime > (unsigned long)_player->effectiveShotInterval()) {
      _lastShotTime = now;
      _bullets->fire(_player->gunX,
                     Player::gunStats[_player->selectedGun],
                     _player->selectedGun,
                     _player->multishotActive);
    }
  }

  // ── Enemy update ──────────────────────────────────────────────
  void _updateEnemies() {
    unsigned long now     = millis();
    unsigned long elapsed = now - _gameStartTime;

    _spawnInterval = constrain(1400 - (int)(elapsed/90) - _score*8, 500, 1400);
    int si = Player::gunStats[_player->selectedGun].interval - _score*4;
    _player->shotInterval = constrain(si,
      Player::gunStats[_player->selectedGun].interval / 2, 9999);

    if (now - _lastSpawnTime > (unsigned long)_spawnInterval) {
      _lastSpawnTime = now;
      _enemies->spawn(_score);
    }

    for (int i = 0; i < MAX_ENEMIES; i++) {
      EnemyData& e = _enemies->enemies[i];
      if (!e.active) continue;

      e.y += e.speed;
      if (e.type == ENEMY_ZIGZAG) {
        e.x += e.zigDir * 1.4f;
        if (e.x < 0)           { e.x = 0;              e.zigDir =  1.0f; }
        if (e.x > 240-e.size)  { e.x = 240 - e.size;   e.zigDir = -1.0f; }
      }
      _enemies->updateFlash(i);

      int ex=(int)e.x, ey=(int)e.y, es=e.size;

      // Va chạm gun
      if (checkCollision((int)_player->gunX, GUN_Y, GUN_WIDTH, GUN_HEIGHT, ex,ey,es,es)) {
        int dmg = (e.type == ENEMY_BOSS) ? 2 : 1;
        _particles->spawn(ex+es/2, ey+es/2, lv_color_make(239,68,68), 4);
        _enemies->hide(i);
        for (int d = 0; d < dmg && _player->hp > 0; d++) {
          if (_player->takeDamage(_ui)) {
            _state = STATE_OVER; _finalScore = _score; return;
          }
        }
        continue;
      }

      // Rơi đáy
      if (e.y > 240) {
        _particles->spawn(ex+es/2, 235, lv_color_make(100,100,200), 3);
        _enemies->hide(i);
        if (_player->takeDamage(_ui)) {
          _state = STATE_OVER; _finalScore = _score; return;
        }
        continue;
      }

      // Va chạm đạn
      bool killed = false;
      for (int b = 0; b < MAX_BULLETS; b++) {
        BulletData& bul = _bullets->bullets[b];
        if (!bul.active) continue;
        if (!checkCollision(bul.x, bul.y, BULLET_W, BULLET_H, ex,ey,es,es)) continue;

        int hitX = ex+es/2, hitY = ey+es/2;
        _flashes->spawn(hitX, hitY);
        e.hp -= bul.damage;
        bul.active = false;
        lv_obj_add_flag(bul.obj, LV_OBJ_FLAG_HIDDEN);

        if (e.hp <= 0) {
          lv_color_t pc; int numP;
          switch (e.type) {
            case ENEMY_BOSS:   pc=lv_color_make(255,80,80);  numP=6; break;
            case ENEMY_TANK:   pc=lv_color_make(52,211,153); numP=5; break;
            case ENEMY_ZIGZAG: pc=lv_color_make(168,85,247); numP=4; break;
            default:           pc=lv_color_make(251,191,36); numP=3; break;
          }
          _particles->spawn(hitX, hitY, pc, numP);

          DropType dt = _rollDrop(e.type);
          if (dt != DROP_NONE) _drops->spawn(hitX - DROP_SIZE/2, hitY, dt);

          _score += (e.type==ENEMY_BOSS)?3:(e.type==ENEMY_TANK)?2:1;
          _enemies->hide(i);
          killed = true;
          _ui->updateScore(_score);
          Serial.printf("[GameManager] Score: %d\n", _score);
          break;
        } else {
          _enemies->triggerFlash(i);
          _enemies->updateHpBar(i);
          break;
        }
      }

      if (!killed && e.active) {
        lv_obj_set_pos(e.obj, ex, ey);
        _enemies->moveHpBar(i);
      }
    }
  }

  // ── Drop pickup ───────────────────────────────────────────────
  void _updateDropsWithPickup() {
    int hit = _drops->update(_player->gunX);
    if (hit < 0) return;
    _applyDrop(_drops->drops[hit].type);
  }

  void _applyDrop(DropType type) {
    Serial.printf("[GameManager] Apply drop: %d\n", (int)type);
    switch (type) {
      case DROP_HP:
        if (_player->hp < PLAYER_MAX_HP) { _player->hp++; _ui->updateHearts(_player->hp); }
        _ui->showPowerupMsg("+1 HP!");
        break;
      case DROP_RAPID:
        _player->applyRapid();
        _ui->showPowerupMsg("RAPID 5s!");
        break;
      case DROP_SHIELD:
        _player->applyShield();
        _ui->showPowerupMsg("SHIELD!");
        break;
      case DROP_BOMB:
        _activateBomb();
        _ui->showPowerupMsg("BOOM!");
        break;
      case DROP_LASER:
        _activateLaser();
        _ui->showPowerupMsg("LASER!");
        break;
      case DROP_MULTISHOT:
        _player->applyMultishot();
        _ui->showPowerupMsg("5-SHOT 8s!");
        break;
      case DROP_MAGNET:
        _drops->magnetPull();
        _ui->showPowerupMsg("MAGNET!");
        break;
      default: break;
    }
  }

  void _activateBomb() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
      EnemyData& e = _enemies->enemies[i];
      if (!e.active) continue;
      lv_color_t pc;
      switch (e.type) {
        case ENEMY_BOSS:   pc=lv_color_make(255,80,80);  break;
        case ENEMY_TANK:   pc=lv_color_make(52,211,153); break;
        case ENEMY_ZIGZAG: pc=lv_color_make(168,85,247); break;
        default:           pc=lv_color_make(251,191,36); break;
      }
      _particles->spawn((int)e.x+e.size/2, (int)e.y+e.size/2, pc, 4);
      _enemies->hide(i);
    }
    lv_obj_t* laser = _ui->laserObj;
    lv_obj_set_size(laser, 240, 240);
    lv_obj_set_pos(laser, 0, 0);
    lv_obj_set_style_bg_color(laser, lv_color_make(255,200,50), 0);
    lv_obj_set_style_bg_opa(laser, LV_OPA_40, 0);
    lv_obj_clear_flag(laser, LV_OBJ_FLAG_HIDDEN);
    _player->showLaser(6);
  }

  void _activateLaser() {
    int lx = (int)_player->gunX + GUN_WIDTH/2 - 3;
    lv_obj_t* laser = _ui->laserObj;
    lv_obj_set_size(laser, 6, GUN_Y + GUN_HEIGHT);
    lv_obj_set_pos(laser, lx, 0);
    lv_obj_set_style_bg_color(laser, lv_color_make(255,255,100), 0);
    lv_obj_set_style_bg_opa(laser, LV_OPA_COVER, 0);
    lv_obj_clear_flag(laser, LV_OBJ_FLAG_HIDDEN);
    _player->showLaser(8);
    bool scored = false;
    for (int i = 0; i < MAX_ENEMIES; i++) {
      EnemyData& e = _enemies->enemies[i];
      if (!e.active) continue;
      int ex=(int)e.x, es=e.size;
      if (ex < lx+26 && ex+es > lx-20) {
        _particles->spawn(ex+es/2, (int)e.y+es/2, lv_color_make(255,255,100), 3);
        _score += (e.type==ENEMY_BOSS)?3:(e.type==ENEMY_TANK)?2:1;
        _enemies->hide(i);
        scored = true;
      }
    }
    if (scored) _ui->updateScore(_score);
  }

  void _triggerGameOver() {
    Serial.printf("[GameManager] GAME OVER — Score: %d\n", _finalScore);
    _state = STATE_OVER;
    _ui->showGameOverScreen();
    lv_obj_set_style_bg_color(_player->gunObj, lv_color_make(100,100,100), 0);
    _score = _finalScore;
    _ui->updateScore(_score);
  }

  DropType _rollDrop(EnemyType type) {
    int r = random(0, 100);
    switch (type) {
      case ENEMY_BOSS:
        if (r<20) return DROP_HP;      if (r<35) return DROP_SHIELD;
        if (r<50) return DROP_BOMB;    if (r<65) return DROP_LASER;
        if (r<80) return DROP_MULTISHOT; if (r<90) return DROP_RAPID;
        return DROP_MAGNET;
      case ENEMY_TANK:
        if (r<12) return DROP_HP;      if (r<22) return DROP_RAPID;
        if (r<32) return DROP_MULTISHOT; if (r<40) return DROP_SHIELD;
        return DROP_NONE;
      case ENEMY_ZIGZAG:
        if (r<10) return DROP_RAPID;   if (r<18) return DROP_LASER;
        if (r<24) return DROP_SHIELD;  if (r<30) return DROP_MAGNET;
        return DROP_NONE;
      default:
        if (r<10) return DROP_HP;      if (r<15) return DROP_BOMB;
        if (r<20) return DROP_MULTISHOT;
        return DROP_NONE;
    }
  }
};
