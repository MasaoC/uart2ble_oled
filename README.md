# uart2ble_oled

![無線ロガー電装システム図](https://github.com/MasaoC/uart2ble_oled/assets/6983713/60b3bb58-795b-453c-8975-e27294d7edc4)


- UARTを受信し、OLEDおよびBLEに送信するArduinoプログラムです。
- ESP32C3 XIAO用に作成されています。nrf52840用もありますが飛距離が最大15m程度なのでTFでは使えません。
- リレー以外にはOLEDに表示する機能もついています。UART -> XIAO(ESP32C3) -> SSD1306 and BLE.
- 受信のUARTは38400Baud. OLED(SSD1306)とはI2Cで通信する。
- UARTで受信したデータのうち、#で始まる文字列のみを、BLE Nordic UARTで送信する。
- 詳細情報が欲しい方はメールからどうぞ。
