#pragma once
#include <tft_setup.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include "Config.h"

// ── DisplayManager ────────────────────────────────────────────────
// Chịu trách nhiệm:
//   - Khởi tạo TFT và LVGL
//   - Cung cấp flush callback (DMA-friendly)
// Debug tip: nếu màn hình trắng/đen → kiểm tra init() và flush callback
class DisplayManager {
public:
  TFT_eSPI tft;

  // Gọi 1 lần trong setup()
  void init() {
    tft.init();
    tft.setRotation(0);
    lv_init();

    lv_display_t* disp = lv_display_create(SCREEN_W, SCREEN_H);

    // Single buffer 10 dòng (double buffer gây block với TFT_eSPI synchronous)
    static uint8_t dispBuf[SCREEN_W * 10 * sizeof(lv_color_t)];
    lv_display_set_buffers(disp, dispBuf, NULL, sizeof(dispBuf),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, _flushCb);

    // Lưu con trỏ tft để dùng trong static callback
    lv_display_set_user_data(disp, this);

    randomSeed(analogRead(36));
  }

private:
  // LVGL flush callback — static vì LVGL không hỗ trợ member function pointer
  static void _flushCb(lv_display_t* disp, const lv_area_t* area, uint8_t* pxMap) {
    auto* self = (DisplayManager*)lv_display_get_user_data(disp);
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    self->tft.startWrite();
    self->tft.setAddrWindow(area->x1, area->y1, w, h);
    self->tft.pushColors((uint16_t*)pxMap, w * h, true);
    self->tft.endWrite();
    lv_display_flush_ready(disp);
  }
};
