//代码大部分参考B站大佬2345vor的博客https://blog.csdn.net/vor234/article/details/138620142，感谢！
//基于他的功能上添加了1，Smartconfig智能配网.2，ST7789V2屏幕显示文本内容.3,由原本2秒改成了5秒录音。
#include <Arduino.h>
#include "base64.h"
#include "WiFi.h"
#include "HTTPClient.h"
#include "cJSON.h"
#include <ArduinoJson.h>
#include "Audio.h"
#include <TFT_eSPI.h>
#include <SPI.h>
#include <U8g2_for_TFT_eSPI.h>
#include <Preferences.h>
#include <esp_wifi.h>
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
TFT_eSPI tft = TFT_eSPI();  
U8g2_for_TFT_eSPI u8g2;
// #按键和max9814麦克风
#define key 7
#define ADC 2
//max98357扬声器引脚
#define I2S_DOUT 6  // DIN connection
#define I2S_BCLK 5  // Bit clock
#define I2S_LRC 4   // Left Right Clock
Preferences preferences;
Audio audio;
HTTPClient http, http1, http2;
String voice_id = "female-tianmei-jingpin";  //青年大学生音色：male-qn-daxuesheng;甜美女性音色：female-tianmei;男性主持人：presenter_male;女性主持人：presenter_female
const int ledPin = 47;  // 指示灯的PIN
hw_timer_t *timer = NULL;
const int adc_data_len = 16000 * 5;
const int data_json_len = adc_data_len * 2 * 1.4;
uint16_t *adc_data;
char *data_json;
uint8_t adc_start_flag = 0;     //开始标志
uint8_t adc_complete_flag = 0;  //完成标志
// 1.++++++++++++++++++++++你需要更改成自己的minimaxapikey+++++++++++++++++++
const char *mini_apiKey = "yJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJHcm91cE5hbWUiOiLpqazkuJwiLCJVc2VyTmFtZSI6IumprOS4nCIsIkFjY291bnQiOiIiLCJTdWJqZWN0SUQiOiIxNzkzMjYyMTIwMDA4MTcyMjM5IiwiUGhvbmUiOiIxNTMzNzI3NTA1MiIsIkdyb3VwSUQiOiIxNzkzMjYyMTE5OTk5NzgzNjMxIiwiUGFnZU5hbWUiOiIiLCJNYWlsIjoiIiwiQ3JlYXRlVGltZSI6IjIwMjQtMDYtMTUgMTg6MDI6NTQiLCJpc3MiOiJtaW5pbWF4In0.ApuuctSBObfF0TAre9fXomonv_heh-u6e3U1zMr4oQIvc2-CCXTxnp201esRXceXZi_E4evVLS9In2o96BSxa4id75QqQBwrPj8ez0vd16K_aREK88os1aV1-rIG3l58InkkRy8kA2X9OIxVJYfJQ-gSUQs0zaqQCCYbcN2nB76P6Yix7wNQbmlL7839Qg-hIGR9LYydfeypyEu28v__yZaMKR_-vUWI71_lEz39D1481HdSO_4xmI85XgT0b1-8YKXK9wvEldSZyxnSt4z_X747QaZn1l7hcjymIbI_41TZbEcGg_7T3aFIRu-LpIA2nPbZbjdDC7IOWHVD-oDMwA";
const char *tts_url = "https://api.minimax.chat/v1/t2a_pro?GroupId=793262119999783631";//2.++++++++++++++++++++++你需要更改成自己的MiniMax的groupid+++++++++++++++++++
const char *chat_url = "https://api.minimax.chat/v1/text/chatcompletion_v2";
const char *stt_url = "http://vop.baidu.com/server_api";
String baidu_token = "24.acb6966eef1d7be3994e78cb4bc4.2592000.1723549112.282335-78661498";//3.++++++++++++++++++++++你需要更改成自己的百度access_token+++++++++++++++++++
String mini_token_key = String("Bearer ") + mini_apiKey;
// Send request to MiniMax API
String inputText = "你好，minimax！";
String response, question, aduiourl;
String answer = "我是鹏鹏的小助手，你好鸭";
DynamicJsonDocument jsonDoc(1024);
uint32_t num = 0;
uint32_t time1, time2;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
// 函数声明
void IRAM_ATTR onTimer();
String sendToSTT();
String getGPTAnswer(String inputText);
String getvAnswer(String ouputText);
//4.++++++++++++++++++++++你需要适配自己的屏幕而修改TFT_eSpi库里面的User_Setup设置+++++++++++++++++++
void displayText(const String &text) {
  tft.fillScreen(TFT_BLACK); // 清除屏幕
  //tft.setTextDatum(MC_DATUM);  // Middle center datum for text positioning
  // 设置字体、前景色、模式和方向
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);  // Ensure the correct font is set
  u8g2.setForegroundColor(WHITE);          //字色
                                          // u8g2.setBackgroundColor(BLACK);//字背景色
  u8g2.setFontMode(1);                     //透明
  u8g2.setFontDirection(0);                //字方向
  int16_t x = 0;                           // 文本框的 X 坐标
  int16_t y = 14;                          // 文本框的 Y 坐标
  int16_t w = tft.width();                 // Width of the screen
  int16_t h = tft.height();                // Height of the screen
   // 定义行高和最大行宽
  int lineHeight = 14;
  int maxLineWidth = w;  // 填充
  int currentLineWidth = 0;
  String currentLine = "";
  // 循环遍历文本字符串，逐字符处理
  for (uint16_t i = 0; i < text.length(); i++) {
    String c = text.substring(i, i + 1);
    int charWidth = 4;
  // 当前行宽度超过最大宽度或遇到换行符时，打印当前行并移动到下一行
    if (currentLineWidth + charWidth > maxLineWidth || c == "\n") {
      u8g2.setCursor(x, y);
      u8g2.print(currentLine);
      y += lineHeight;
      currentLine = "";
      currentLineWidth = 0;
    }
     // 如果不是换行符，将字符添加到当前行
    if (c != "\n") {
      currentLine += c;
      currentLineWidth += charWidth;
    }
  }
  // 打印最后一行文本（如果存在）
  if (currentLine.length() > 0) {
    u8g2.setCursor(x, y);
    u8g2.print(currentLine);
  }
}
void saveWiFiCredentials(const char* ssid, const char* password);
bool loadWiFiCredentials(String &ssid, String &password);
void startSmartConfig();
void setupWiFi();
void saveWiFiCredentials(const char* ssid, const char* password) {
  preferences.begin("wifi", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.end();
}
bool loadWiFiCredentials(String &ssid, String &password) {
  preferences.begin("wifi", true);
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  preferences.end();
  return (ssid.length() > 0 && password.length() > 0);
}
void startSmartConfig() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.beginSmartConfig();
  Serial.println("Waiting for SmartConfig.");
  displayText("Waiting for SmartConfig.");
  while (!WiFi.smartConfigDone()) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("SmartConfig received.");
  Serial.println("Waiting for WiFi");
  displayText("SmartConfig received.Waiting for WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi Connected.");  
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  displayText("WIFI已连接!");
  // Save the credentials
  saveWiFiCredentials(WiFi.SSID().c_str(), WiFi.psk().c_str());
}

void setupWiFi() {
  String ssid, password;
  if (loadWiFiCredentials(ssid, password)) {
    Serial.println("找到已保存的凭据。正在尝试连接...");
    displayText("找到已保存的凭据。正在尝试连接...");
    WiFi.begin(ssid.c_str(), password.c_str());
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected to WiFi!");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      displayText("WIFI已连接!");
    } else {
      Serial.println("\nFailed to connect with saved credentials.");
      Serial.print("WiFi status: ");
      Serial.println(WiFi.status());
      Serial.println("Starting SmartConfig...");
      displayText("开始SmartConfig...");
      startSmartConfig();
    }
  } else {
    Serial.println("No saved credentials found. Starting SmartConfig...");
    startSmartConfig();
  }
}
void setup() {
  Serial.begin(115200);
  adc_data = (uint16_t *)ps_malloc(adc_data_len * sizeof(uint16_t));  //ps_malloc 指使用片外PSRAM内存
  if (!adc_data) {
    Serial.println("无法为 adc_data 分配内存");
  }
  data_json = (char *)ps_malloc(data_json_len * sizeof(char));  // 根据需要调整大小
  if (!data_json) {
    Serial.println("无法为 data_json 分配内存");
  }
  pinMode(ADC, ANALOG);
  pinMode(key, INPUT_PULLUP);
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(21);  // 0...21
  pinMode(ledPin, OUTPUT);
  uint8_t count = 0;
  tft.init();
  tft.setRotation(0);           // 将旋转设置为纵向或横向
  tft.fillScreen(TFT_MAGENTA);  // 品红色背景
  u8g2.begin(tft);
  setupWiFi();
  timer = timerBegin(0, 80, true);    //  80M的时钟 40分频 2M
  timerAlarmWrite(timer, 1000000 / 16000, true);  //  2M  计125个数进中断  16K
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmEnable(timer);
  timerStop(timer);  //先暂停
}
unsigned long lastReconnectAttempt = 0;
const unsigned long reconnectInterval = 30000; 
void loop() {
  unsigned long currentMillis = millis();
  if (WiFi.status() != WL_CONNECTED) {
    if (currentMillis - lastReconnectAttempt >= reconnectInterval) {
      Serial.println("WiFi 连接丢失。正在尝试重新连接...");
      displayText("WiFi 连接丢失。正在尝试重新连接...");
      setupWiFi();
      lastReconnectAttempt = currentMillis;
    }
  }
  audio.loop();
  if (digitalRead(key) == 0) {
    delay(10);
    if (digitalRead(key) == 0) {
      Serial.printf("Start recognition\r\n");
      displayText("开始提问");
      digitalWrite(ledPin, LOW);
      adc_start_flag = 1;
      timerStart(timer);
      while (!adc_complete_flag)  //等待采集完成
      {
        ets_delay_us(10);
      }
      timerStop(timer);
      adc_complete_flag = 0;  //清标志
      digitalWrite(ledPin, HIGH);
      question = sendToSTT();
      if (question != "error") {
        Serial.println("输入:" + question);       
        // u8g2.setCursor(8, 15);
        displayText("输入:" + question);
        answer = getGPTAnswer(question);
        if (answer != "error") {
          Serial.println("回答: " + answer);
          // u8g2.setCursor(8, 30);
          delay(500);
          displayText("回答: " + answer);
          aduiourl = getvAnswer(answer);
          if (aduiourl != "error") {
            audio.stopSong();
            audio.connecttohost(aduiourl.c_str());  //  128k mp3
          }
        }
      }
     // Serial.println("Recognition complete\r\n");
    }
  }
  while (Serial.available() > 0) {
    char voice = Serial.read();
    // Serial.println(voice);
    switch (voice) {
      case '1':
        voice_id = "male-qn-daxuesheng";
        break;
      case '2':
        voice_id = "female-tianmei";
        break;
      case '3':
        voice_id = "presenter_male";
        break;
      case '4':
        voice_id = "presenter_female";
        break;
      case '5':
        //5.replace your clone voice_id
        voice_id = "vor_test";
        break;
    }
  //  Serial.println(voice_id);
  }
  vTaskDelay(5);
}
//录音函数
void IRAM_ATTR onTimer() {
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  if (adc_start_flag == 1) {
    //Serial.println("");
    adc_data[num] = analogRead(ADC);
    num++;
    if (num >= adc_data_len) {
      adc_complete_flag = 1;
      adc_start_flag = 0;
      num = 0;
      //Serial.println(Complete_flag);
    }
  }
  portEXIT_CRITICAL_ISR(&timerMux);
}
//stt语音识别
String sendToSTT() {
  memset(data_json, '\0', data_json_len * sizeof(char));
  strcat(data_json, "{");
  strcat(data_json, "\"format\":\"pcm\",");
  strcat(data_json, "\"rate\":16000,");
  strcat(data_json, "\"dev_pid\":1537,");
  strcat(data_json, "\"channel\":1,");
  strcat(data_json, "\"cuid\":\"57722200\",");
  strcat(data_json, "\"token\":\"");
  strcat(data_json, baidu_token.c_str());
  strcat(data_json, "\",");
  sprintf(data_json + strlen(data_json), "\"len\":%d,", adc_data_len * 2);
  strcat(data_json, "\"speech\":\"");
  strcat(data_json, base64::encode((uint8_t *)adc_data, adc_data_len * sizeof(uint16_t)).c_str());
  strcat(data_json, "\"");
  strcat(data_json, "}");
  http.begin(stt_url);  //https://vop.baidu.com/pro_api
  http.addHeader("Content-Type", "application/json");
  // Serial.print(data_json);
  int httpResponseCode = http.POST(data_json);
  if (httpResponseCode == 200) {
    response = http.getString();
    http.end();
    Serial.print(response);
    deserializeJson(jsonDoc, response);
    String question = jsonDoc["result"][0];
    // 访问"result"数组，并获取其第一个元
    return question;
  } else {
    http.end();
    Serial.printf("stt_error: %s\n", http.errorToString(httpResponseCode).c_str());
    return "error";
  }
}
//chatgpt对话
String getGPTAnswer(String inputText) {
  http1.begin(chat_url);
  http1.addHeader("Content-Type", "application/json");
  http1.addHeader("Authorization", mini_token_key);
  String payload = "{\"model\":\"abab5.5s-chat\",\"messages\":[{\"role\": \"system\",\"content\": \"你是悠悠的生活助手机器人，要求下面的回答严格控制在256字符以内。\"},{\"role\": \"user\",\"content\": \"" + inputText + "\"}]}";
  int httpResponseCode = http1.POST(payload);
  if (httpResponseCode == 200) {
    response = http1.getString();
    http1.end();
    Serial.println(response);
    deserializeJson(jsonDoc, response);
    String answer = jsonDoc["choices"][0]["message"]["content"];
    return answer;
  } else {
    // http1.end();
    response = http1.getString();
    http1.end();
    Serial.println(response);
    Serial.printf("chatError %i \n", httpResponseCode);
    return "error";
  }
}
//tts语音播报
String getvAnswer(String ouputText) {
  http2.begin(tts_url);
  http2.addHeader("Content-Type", "application/json");
  http2.addHeader("Authorization", mini_token_key);
  // 创建一个StaticJsonDocument对象，足够大以存储JSON数据
  StaticJsonDocument<200> doc;
  // 填充数据
  doc["text"] = ouputText;
  doc["model"] = "speech-01";
  doc["audio_sample_rate"] = 32000;
  doc["bitrate"] = 128000;
  doc["voice_id"] = voice_id;
  // 创建一个String对象来存储序列化后的JSON字符串
  String jsonString;
  // 序列化JSON到String对象
  serializeJson(doc, jsonString);
  int httpResponseCode = http2.POST(jsonString);
  if (httpResponseCode == 200) {
    response = http2.getString();
    Serial.println(response);
    http2.end();
    deserializeJson(jsonDoc, response);
    String aduiourl = jsonDoc["audio_file"];
    return aduiourl;
  } else {
    Serial.printf("tts %i \n", httpResponseCode);
    http2.end();
    return "error";
  }
}