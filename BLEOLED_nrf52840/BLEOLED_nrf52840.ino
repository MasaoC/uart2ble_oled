//For XIAO nrf52840

#include <Wire.h> 
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 32 
#define OLED_RESET -1 
#define BAUD_RATE 38400

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
SoftwareSerial D10serial(D10, D7);//RX D10 is used only (D8 is not being used)

//Bluetooth機能を有効にする場合、#define BLE する。無効にする場合コメントアウトする。
#define BLE

#ifdef BLE
  #include <bluefruit.h>
  #define MAX_PRPH_CONNECTION   3 //多すぎるとメモリ不足で起動時フリーズする.....
  uint8_t connection_count = 0;
  // BLE Service
  BLEDfu  bledfu;  // OTA DFU service
  BLEDis  bledis;  // device information
  BLEUart bleuart; // uart over ble
  //BLEBas  blebas;  // battery残量の通知機能　測定機能を作ってないので、今はコメントアウトしとく。
#endif

//定数
const int BUFFER_SIZE = 90;//受信バッファー.  86文字+\r+\n と少し余裕持たせてみた。
const int TIMEOUT_UART = 3000;// If no signal for TIMEOUT_UART ms, then NO SIGNAL... will be displayed.
const char NEXT_LINE_CHAR = '~';//改行とする文字
const char ENDOFDATA = '\n';//データ終了(区切り)文字
const char BLE_BEGIN_TOKEN = '#';
const char BIGFONT_TOKEN = '!';
const unsigned long LED_TIME = 500;// LEDの点滅周期/2 ms

//変数
uint8_t buf[BUFFER_SIZE];//受信バッファー(char型でもいいのだが、BLEライブラリとの親和性のため uint8_tとする。)
bool ledMode = true;//LED ON/OFF
bool RXisUsed = false;// D7ハードシリアル受信中。2ポートのうち、今どっちが使われいるのか判定するために使用する。
bool D10isUsed = false;// ソフトシリアル受信中。

unsigned long last_led_fliptime = 0;//最後のled点灯・消灯時刻 ms
unsigned long last_message_time = 0;//最後にUART受信した時刻 ms

void printOLED(const char* chars){
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextSize(1);
    display.println(chars);
    display.display();
}

//使用状況確認
void checkUsage(int delaytime){
  delay(delaytime);
  if(Serial1.available() > 0){
    RXisUsed = true;
    printOLED("RX UART RECEIVED");
    last_message_time = millis();
    return;
  }
  delay(delaytime);
  if(D10serial.available() > 0){
    D10isUsed = true;
    printOLED("D10 UART RECEIVED");
    last_message_time = millis();
  }
}

// the setup function runs once when you press reset or power the board
void setup() {
  #ifdef BLE
    //BLE LED自動制御OFF
    Bluefruit.autoConnLed(false);
    // Config the peripheral connection with maximum bandwidth  more SRAM required by SoftDevice  Note: All config***() function must be called before begin()
    Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);
    
    // Initialize Bluefruit with max concurrent connections as Peripheral = 2, Central = 0
    Bluefruit.begin(MAX_PRPH_CONNECTION, 0);
    Bluefruit.setTxPower(8);    // Check bluefruit.h for supported values
    
    Bluefruit.setName("ALBA TAIYO OLED"); // useful testing with multiple central connections
    Bluefruit.Periph.setConnectCallback(connect_callback);
    Bluefruit.Periph.setDisconnectCallback(disconnect_callback);
    // To be consistent OTA DFU should be added first if it exists
    bledfu.begin();

    // Configure and Start Device Information Service
    bledis.setManufacturer("Adafruit Industries");
    bledis.setModel("Bluefruit Feather52");
    bledis.begin();

    // Configure and Start BLE Uart Service
    bleuart.begin();
    // Start BLE Battery Service
    //電圧図る機能（ハードウェア）つけたら、これで転送できる。
    //blebas.begin();
    //blebas.write(100);
    // Set up and start advertising
    startAdv();
  #endif

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  digitalWrite(LED_RED,HIGH);
  digitalWrite(LED_GREEN,HIGH);
  digitalWrite(LED_BLUE,HIGH);

  display.begin(SSD1306_SWITCHCAPVCC,0x3C); 
  display.setTextColor(WHITE);  
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(2);
  display.println("HELLO!");
  display.setCursor(0,16);
  display.println("ALBATROSS!");
  display.display(); 

  pinMode(D7, INPUT_PULLDOWN);//D7=RX
  pinMode(D9, INPUT_PULLDOWN);//D9=D7=RX
  pinMode(D10, INPUT_PULLDOWN);

  Serial.begin(BAUD_RATE);
  Serial1.begin(BAUD_RATE);
  D10serial.begin(BAUD_RATE);

  //readBytesUntil のタイムアウト時間を設定する。最初のbitから、データを最後まで受信しきるまでの時間は少なくとも待ちたい。
  //なので最小値は、約90bit/38400bps*1000=2.34 [ms] なので、最大100msも待てば最後まで受信できるはず。 
  Serial.setTimeout(100);
  Serial1.setTimeout(100);
  D10serial.setTimeout(100);

  checkUsage(1000);//1000ms wait * 2 for signal (SHOW HELLO ALBATROSS)
}

//buff に読み込んだ、シリアル受信char文字列をデコードしOLEDに表示する。
//NEXT_LINE_CHAR=[~]文字を使用することで強制改行 ができる。ただしOLED側都合で、4行まで。
//一文字目が[!]であるとフォントサイズが大きくなる。
//1文字目が[#]であると、BLEに転送する。
void decode_print_buff(int rlen){
  if(rlen <= 0){
    return;
  }
  last_message_time = millis();

  #ifdef BLE
    //BLE開始文字だったら、
    if(buf[0] == BLE_BEGIN_TOKEN){
        buf[rlen] = ENDOFDATA;//plotter用
        // Send to all connected centrals
        for (uint8_t conn_hdl=0; conn_hdl < MAX_PRPH_CONNECTION; conn_hdl++)
        {
          //BLE_BEGIN_TOKENの一文字を飛ばして、送信文字列にENDOFDATAまで含める。（あえて+1-1は残した）
          bleuart.write(conn_hdl,buf+1,rlen-1+1);
        }
        //以上
        return;
    }
  #endif

  char firstline[rlen+1];//終了文字('\0')のために+1
  char secondline[rlen+1];//まだ2行目の長さは不明のため、同じ長さ確保。
  char thirdline[rlen+1];//まだ3行目の長さは不明のため、同じ長さ確保。
  char forthline[rlen+1];//まだ4行目の長さは不明のため、同じ長さ確保。
  
  int index_second_line = -1;
  int index_third_line = -1;
  int index_forth_line = -1;
  int firstline_len = 0;

  int font_size = buf[0]==BIGFONT_TOKEN?2:1;

  //1文字づつ処理
  for(int i = 0; i < rlen; i++){
    //改行文字であるならば、
    if(buf[i] == NEXT_LINE_CHAR){
      if(index_second_line == -1){
        //１行目終了。
        firstline[i] = '\0';//終了文字・いれないと文字化けする。
        index_second_line = 0;
      }else if(index_third_line == -1){
        //２行目終了
        secondline[index_second_line] =  '\0';
        index_third_line = 0;
      }else if(index_forth_line == -1){
        //3行目終了
        thirdline[index_third_line] =  '\0';
        index_forth_line = 0;
      }
      continue;
    }
    if(index_second_line < 0){//1行目
      firstline[i] = buf[i];
    }else if(index_third_line < 0){//2行目
      secondline[index_second_line++] = buf[i];
    }else if(index_forth_line < 0){//2行目
      thirdline[index_third_line++] = buf[i];
    }else{//3行目
      forthline[index_forth_line++] = buf[i];
    }
  }
  firstline[rlen] = '\0';//改行なかった時のため。終了文字・いれないと文字化けする。
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(font_size);
  display.print(firstline);

  //2行目
  if(index_second_line >= 0){
    secondline[index_second_line] ='\0';
    display.setCursor(0,8);
    display.print(secondline);
  }
  //３行目
  if(index_third_line >= 0){
    thirdline[index_third_line] ='\0';
    display.setCursor(0,16);
    display.print(thirdline);
  }
  //4行目
  if(index_forth_line >= 0){
    forthline[index_forth_line] ='\0';
    display.setCursor(0,24);
    display.print(forthline);
  }
  display.display();
}

//LEDを点滅させる。
void flash_led(){
  if(millis() - last_led_fliptime > LED_TIME){
    ledMode = !ledMode;
    if(!RXisUsed && !D10isUsed){
      digitalWrite(LED_RED, ledMode);
      digitalWrite(LED_GREEN,HIGH);
      digitalWrite(LED_BLUE,HIGH);
    }else if(RXisUsed){
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN,ledMode);
      digitalWrite(LED_BLUE,HIGH);
    }else if(D10isUsed){
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN,HIGH);
      digitalWrite(LED_BLUE,ledMode);
    }
    last_led_fliptime = millis();
  }
}
// the loop function runs over and over again forever
void loop() {
  //LEDの点滅
  flash_led();

  //NO SIGNAL ...
  if(!RXisUsed && !D10isUsed){
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextSize(2);
    display.println("NO SIGNAL");
    //アニメーション
    display.setCursor((millis()/70)%110,16);
    if(ledMode){
      display.println(".");
    }else{
      display.println("..");
    }
    display.display();
    //RXかD10にUARTが来ていないか確認する。
    checkUsage(random(30,100));//送信周期と完全一致する可能性を避けるため、Random使用。
  }
  //RX(D9)受信モードであるなら
  else if(RXisUsed){
    //受信していれば
    if (Serial1.available() > 0) {
      digitalWrite(LED_RED,LOW);
      int bytes = Serial1.available();
      int rlen = Serial1.readBytesUntil(ENDOFDATA, buf, BUFFER_SIZE);
      decode_print_buff(rlen);
      digitalWrite(LED_RED,HIGH);
    }
  // D10受信モードであるなら
  }else if(D10isUsed){
    D10serial.listen();
    //データがあれば受信
    if (D10serial.available() > 0) {
      digitalWrite(LED_RED,LOW);
      int bytes = D10serial.available();
      int rlen = D10serial.readBytesUntil(ENDOFDATA, buf, BUFFER_SIZE);
      decode_print_buff(rlen);
      digitalWrite(LED_RED,HIGH);
    }
  }

  #ifdef BLE
    //スマホ(BLE)から来たデータは全て、Serial(USB-C)に返す。特に使ってない。
    while ( bleuart.available() )
    {
      uint8_t ch;
      ch = (uint8_t) bleuart.read();
      Serial.write(ch);
    }
  #endif

  //Debug用 USB-C UARTを受信した場合。
  if (Serial.available() > 0) {
    RXisUsed = true;//内部処理としては、USB-CもD7ピンのRXも同じことにしておく。（緑LED点滅）
    printOLED("USB-C UART RECEIVED");
    last_message_time = millis();
    digitalWrite(LED_RED,LOW);
    int rlen = Serial.readBytesUntil(ENDOFDATA, buf, BUFFER_SIZE);
    decode_print_buff(rlen);
    digitalWrite(LED_RED,HIGH);
  }

  //RX,D10いずれか使用中で、タイムアウト時間経過していれば isUsed を falseにする。
  if(millis()-last_message_time > TIMEOUT_UART){
    D10isUsed = false;
    RXisUsed = false;
  }
}



#ifdef BLE
  //サンプルコードコピペ。
  void startAdv(void)
  {
    // Advertising packet
    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();
    // Include bleuart 128-bit uuid
    Bluefruit.Advertising.addService(bleuart);
    // Secondary Scan Response packet (optional)
    // Since there is no room for 'Name' in Advertising packet
    Bluefruit.ScanResponse.addName();

    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms. fast mode = 20 ms, slow mode = 152.5 ms
    Bluefruit.Advertising.setFastTimeout(30);      // Timeout for fast mode is 30 seconds
    Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds.  Start(timeout) with timeout = 0 will advertise forever (until connected)
  }


  //サンプルコード、コピペ： callback invoked when central connects
  void connect_callback(uint16_t conn_handle)
  {
    // Get the reference to current connection
    BLEConnection* connection = Bluefruit.Connection(conn_handle);

    char central_name[32] = { 0 };
    connection->getPeerName(central_name, sizeof(central_name));

    Serial.print("Connected to ");
    Serial.println(central_name);

    connection_count++;
    Serial.print("Connection count: ");
    Serial.println(connection_count);
    
    // Keep advertising if not reaching max
    if (connection_count < MAX_PRPH_CONNECTION){
      Serial.println("Keep advertising");
      Bluefruit.Advertising.start(0);
    }
  }

  // サンプルコード、コピペ：Callback invoked when a connection is dropped
  void disconnect_callback(uint16_t conn_handle, uint8_t reason)
  {
    (void) conn_handle;
    (void) reason;
    Serial.println();
    Serial.print("Disconnected, reason = 0x"); Serial.println(reason, HEX);
    connection_count--;
  }
#endif
