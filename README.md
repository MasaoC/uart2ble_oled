# uart2ble_oled

![無線ロガー電装システム図](https://github.com/MasaoC/uart2ble_oled/assets/6983713/60b3bb58-795b-453c-8975-e27294d7edc4)


- UARTを受信し、OLEDおよびBLEに送信するArduinoプログラムです。
- ESP32C3 XIAO用に作成されています。nrf52840用もありますが飛距離が最大15m程度なのでTFでは使えません。
- リレー以外にはOLEDに表示する機能もついています。UART -> XIAO(ESP32C3) -> SSD1306 and BLE.
- 受信のUARTは38400Baud. OLED(SSD1306)とはI2Cで通信する。
- UARTで受信したデータのうち、#で始まる文字列のみを、BLE Nordic UARTで送信する。
- 詳細情報が欲しい方はメールからどうぞ。

## 仕様書
- [BLEOLED_nrf52840](https://github.com/MasaoC/uart2ble_oled/blob/main/doc/UARTtoOLED%E5%9F%BA%E6%9D%BFv1%20%E4%BB%95%E6%A7%98%E6%9B%B8.pdf)
- [BLEOLEDv2_ESP32C3](https://github.com/MasaoC/uart2ble_oled/blob/main/doc/UARTtoOLED%E5%9F%BA%E6%9D%BFv2%20%E4%BB%95%E6%A7%98%E6%9B%B8.pdf)
