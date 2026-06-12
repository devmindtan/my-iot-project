void setup() {
  // Khởi tạo Serial với tốc độ 115200
  Serial.begin(115200);
  
  // Chờ 2 giây để cổng Serial ổn định sau khi reset
  delay(2000); 

  Serial.println("\n==========================================");
  Serial.println("      KIỂM TRA THÔNG TIN PHẦN CỨNG ESP32   ");
  Serial.println("==========================================");

  // 1. Kiểm tra chính xác dòng Chip
  Serial.print("- Dòng Chip (Model):        ");
  Serial.println(ESP.getChipModel());

  // 2. Kiểm tra số nhân CPU
  Serial.print("- Số nhân CPU (Cores):      ");
  Serial.print(ESP.getChipCores());
  Serial.println(" nhân");

  // 3. Kiểm tra tần số CPU (Xung nhịp)
  Serial.print("- Tần số CPU Clock:         ");
  Serial.print(ESP.getCpuFreqMHz());
  Serial.println(" MHz");

  // 4. Kiểm tra phiên bản Revision của Chip từ nhà sản xuất
  Serial.print("- Chip Revision (Phiên bản): ");
  Serial.println(ESP.getChipRevision());

  // 5. Kiểm tra dung lượng bộ nhớ Flash tích hợp
  Serial.print("- Dung lượng Flash:         ");
  Serial.print(ESP.getFlashChipSize() / (1024 * 1024));
  Serial.println(" MB");

  // 6. Kiểm tra phiên bản của SDK (Espressif IoT Development Framework)
  Serial.print("- Phiên bản ESP-IDF SDK:    ");
  Serial.println(ESP.getSdkVersion());
  
  Serial.println("==========================================");
}

void loop() {
  // Không cần chạy gì trong loop
}