# Cấu hình phần cứng ESP32-D0WD-V3 + ST7789

## 1. Thông tin chip (đo bằng check_hardware_info.ino)

```bash
==========================================
      KIỂM TRA THÔNG TIN PHẦN CỨNG ESP32
==========================================
- Dòng Chip (Model):        ESP32-D0WD-V3
- Số nhân CPU (Cores):      2 nhân
- Tần số CPU Clock:         240 MHz
- Chip Revision (Phiên bản): 301
- Dung lượng Flash:         4 MB
- Phiên bản ESP-IDF SDK:    v5.5.4
==========================================
```

## 2. Cấu hình màn hình ST7789 (Setup24_ST7789.h)

| Thông số           | Giá trị       | Ghi chú                       |
| ------------------ | ------------- | ----------------------------- |
| Driver             | ST7789_DRIVER |                               |
| Độ phân giải       | 240 x 240     | Không có chân CS              |
| TFT_INVERSION_ON   | bật           | Bật nếu màn hình bị ngược màu |
| TFT_MOSI           | GPIO 23       |                               |
| TFT_SCLK           | GPIO 18       |                               |
| TFT_CS             | -1            | Không dùng                    |
| TFT_DC             | GPIO 15       |                               |
| TFT_RST            | GPIO 17       |                               |
| SPI_FREQUENCY      | 27 MHz        | Hạ từ 40MHz để chạy ổn định   |
| SPI_READ_FREQUENCY | 20 MHz        |                               |

Font đã tải: GLCD, FONT2, FONT4, FONT6, FONT7, FONT8, GFXFF, SMOOTH_FONT

## 3. File liên quan

- `check_hardware_info.ino`: code đọc thông tin chip ESP32
- `Setup24_ST7789.h`: file cấu hình chân và driver cho thư viện TFT_eSPI
