// ST7789 240 x 240 display with no chip select line
#define USER_SETUP_ID 24

#define ST7789_DRIVER     // Kích hoạt driver cho ST7789

#define TFT_WIDTH  240
#define TFT_HEIGHT 240

// Đôi khi màn hình ST7789 bị ngược màu, nếu bị thì bật dòng này lên (on/off)   
#define TFT_INVERSION_ON 

// --- CẤU HÌNH CHÂN CHO ESP32 (Hết sức lưu ý phần này) ---
#define TFT_MOSI 23  // Chân SDA/MOSI trên màn hình -> Cắm vào GPIO 23 của ESP32
#define TFT_SCLK 18  // Chân SCL/SCK trên màn hình  -> Cắm vào GPIO 18 của ESP32
#define TFT_CS   -1  // Màn hình không có chân CS -> Để là -1
#define TFT_DC   15  // Chân DC (hoặc RS/A0)        -> Cắm vào GPIO 15 của ESP32
#define TFT_RST  17  // Chân RES (hoặc RST)       -> Cắm vào GPIO 17 của ESP32

// --- CÁC FONT CHỮ ĐƯỢC TẢI ---
#define LOAD_GLCD   
#define LOAD_FONT2  
#define LOAD_FONT4  
#define LOAD_FONT6  
#define LOAD_FONT7  
#define LOAD_FONT8  
#define LOAD_GFXFF  
#define SMOOTH_FONT

// --- CẤU HÌNH TỐC ĐỘ SPI (Hạ xuống để đảm bảo chạy ổn định trước) ---
#define SPI_FREQUENCY       27000000  // Hạ từ 40MHz xuống 27MHz để tránh nhiễu tín hiệu
#define SPI_READ_FREQUENCY  20000000