// 1. Ép thư viện tự tạo cấu hình riêng, không thèm đọc file User_Setup.h nữa
#define USER_SETUP_LOADED 1 

// 2. Định nghĩa cấu hình màn hình ST7789
#define ST7789_DRIVER
#define TFT_WIDTH  240
#define TFT_HEIGHT 240
#define TFT_INVERSION_ON

// 3. Khai báo đúng các chân đã chạy thành công của bạn
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   -1
#define TFT_DC   15
#define TFT_RST  17

// 4. Các font chữ tiêu chuẩn
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT
#define SPI_FREQUENCY 27000000