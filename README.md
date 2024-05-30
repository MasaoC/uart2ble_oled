# uart2ble_oled

- UARTを受信し、OLEDおよびBLEに送信するArduinoプログラムです。
- ESP32C3 XIAO用に作成されています。
- UART -> XIAO(ESP32C3) -> SSD1306 and BLE.
- 受信のUARTは38400Baud. OLED(SSD1306)とはI2Cで通信する。
- UARTで受信したデータのうち、#で始まる文字列のみを、BLE Nordic UARTで送信する。
- 詳細情報が欲しい方はメールからどうぞ。
