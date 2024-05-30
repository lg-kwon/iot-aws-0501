
/*********************************************************************

1. 2024.4.30일 aws-iot와 연결이 성공 버전......

  nodemcu: esp8266 과 컴파일 다운로드 성공

  참조 사이트 : https://how2electronics.com/connecting-esp8266-to-amazon-aws-iot-core-using-mqtt/

  참조 사이트 : https://github.com/camsaway/aws-iot-esp8266-mqtt-example/blob/main/README.md

  The purpose of this repo is to provide ESP8266 compatible code to connect to IoT Core MQTT service.

  It is based largely on the ESP32 example provided by AWS at this location. https://github.com/aws-samples/aws-iot-esp32-arduino-examples https://aws.amazon.com/blogs/compute/building-an-aws-iot-core-device-using-aws-serverless-and-an-esp32/

2. serial 인터페이스  참조 사이트 :
     https://www.jobtoy.com/project/arduino_view?idx=85

**********************************************************************/

#include <Arduino.h>


#define DEBUG
// #define OZS_SERIAL_NUMBER_00002
#define OZS_SERIAL_NUMBER_00004

// 2024-05-29 : 디바이스 key 포함
#ifdef OZS_SERIAL_NUMBER_00002
#include "secrets_7da964b5019e7ef01fb29fbe3c1afc754a0385d9443b399105966575bfc3d4e4.h"            
#endif

#ifdef OZS_SERIAL_NUMBER_00004
#include "secrets_e73bd4c5ee.h"
#endif


#include <WiFiClientSecure.h>   // From the core ESP library - Don't need to add this
#include <MQTTClient.h>         // Need to add library 256dpi/MQTT
#include <ArduinoJson.h>        // Need to add library bblanchon/ArduinoJSON
#include "ESP8266WiFi.h"        // From the core ESP library - Don't need to add this.

#include "serial_communication.h"
#include "timer.h"


WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);


// port(5,6) ozs serial 통신
SoftwareSerial SerialPort(D5,D6);

String receivedString = ""; // 수신된 문자열을 저장할 변수
bool insideBrackets = false; // '['와 ']' 사이의 문자열인지 여부를 나타내는 플래그

struct tm timeinfo;

unsigned long start_time;
unsigned long end_time;



void connectAWS()
{
  // Connect to the WiFi network
  Serial.print("Connecting to Wi-Fi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println(""); 
  Serial.print("Connected! IP Address: "); 
  Serial.println(WiFi.localIP());
  Serial.print("Mask: "); 
  Serial.print(WiFi.subnetMask()); 
  Serial.print(" Gateway: "); Serial.println(WiFi.gatewayIP());
  Serial.print("SSID: "); 
  Serial.print(WiFi.SSID()); 
  Serial.print(" RSSI: "); 
  Serial.println(WiFi.RSSI());

  Serial.print("AWS_IOT_SUBSCRIBE_TOPIC :");
  Serial.println(AWS_IOT_SUBSCRIBE_TOPIC);


  // Set the real time as this is needed for X509 certificate verification
  Serial.println(""); 
  Serial.print("Waiting for NTP time sync: ");
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  
  gmtime_r(&now, &timeinfo);
  Serial.println(""); 
  Serial.print("Current time: "); 
  convertToKoreanTime(&timeinfo);
  Serial.print(asctime(&timeinfo));

  // Connect to AWS MQTT
  Serial.println(""); 
  Serial.println("Connecting to AWS IOT");
  // Configure WiFiClientSecure to use the AWS IoT device credentials
    BearSSL::X509List cert(AWS_CERT_CA);
    net.setTrustAnchors(&cert);
    BearSSL::X509List client_crt(AWS_CERT_CRT);
    BearSSL::PrivateKey key(AWS_CERT_PRIVATE);
    net.setClientRSACert(&client_crt, &key);
  // Connect to the MQTT broker on the AWS endpoint we defined earlier

  String clientId = "ESP8266Client-" + String(ESP.getChipId());

  Serial.println("client id:");
  Serial.println(clientId);

  client.begin(AWS_IOT_ENDPOINT, 8883, net);


  while (!client.connect(clientId.c_str())) {
    Serial.print(".");
    delay(100);
  }
  if(!client.connected()){
    Serial.println("AWS IoT Timeout!");
    return;
  } else {
    Serial.println("AWS IoT Connected!");
  }

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
  Serial.println("Topic Subscribed");
}



void messageHandler(String &topic, String &payload) {


  start_time = millis();


  payload.replace("\\","");

  #ifdef DEBUG
  Serial.println("incoming: " + topic + " - " + payload);
  #endif

  on_message_received(topic, payload );

//  StaticJsonDocument<200> doc;
//  deserializeJson(doc, payload);
//  const char* message = doc["message"];
}

void setup() {
  SerialPort.begin(115200); //(5,6) port 
  Serial.begin(115200);
  Serial.println(""); Serial.println(""); 
  Serial.println("SETUP");
  connectAWS();
  Serial.println(""); Serial.println(""); 
  client.onMessage(messageHandler);

  // 2024-05-05 : 네트웍 준비 송부
  send_wifi_ready();
  // delay(200);
  // publishMessage();
}

void publishMessage()
{
  Serial.println("publish Message.....");
  StaticJsonDocument<200> doc;
  time_t now = time(nullptr);
   gmtime_r(&now, &timeinfo);
   convertToKoreanTime(&timeinfo);

  doc["time"] = asctime(&timeinfo);
  doc["sensor_a0"] = analogRead(0);
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);

  end_time = millis();

  unsigned long elapsedTime = end_time - start_time;
  Serial.print("Message handling and publishing took: ");
  Serial.print(elapsedTime);
  Serial.println(" ms");

}

void publish_ozs_status(String &message){

  #ifdef DEBUG
  Serial.println("report ozs board status......." + message);
  #endif

  // ":" delimiter로 message를 분리
  int delimiterIndex = message.indexOf(':');
  String firstToken = message.substring(0, delimiterIndex);
  firstToken.trim();

  Serial.println("firstToke = " + firstToken);
  String secondToken = message.substring(delimiterIndex + 1);

  delimiterIndex = secondToken.indexOf(':');
  String status = secondToken.substring(0,delimiterIndex);
  String time = secondToken.substring(delimiterIndex+1);
  
  #ifdef DEBUG
  Serial.println("status = " + status);
  Serial.println("time = " + time);
  #endif


  StaticJsonDocument<200> doc;

  doc["status"] = status;
  doc["time"] = time;

  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);


  end_time = millis();

  unsigned long elapsedTime = end_time - start_time;
  Serial.print("Message handling and publishing took: ");
  Serial.print(elapsedTime);
  Serial.println(" ms");


}

void publish_ozs_system_info(String &message){
  // Serial.println("report ozs system info = " + message);

  // ":" delimiter로 message를 분리
  int delimiterIndex = message.indexOf(':');
  String firstToken = message.substring(0, delimiterIndex);

#ifdef DEBUG
  Serial.println("firstToke = " + firstToken);
#endif
  String info = message.substring(delimiterIndex + 1);

  // delimiterIndex = secondToken.indexOf(':');
  // String filter = secondToken.substring(0,delimiterIndex);
  // String time = secondToken.substring(delimiterIndex+1);
  
  Serial.println("rx info = " + info);
  // Serial.println("time = " + time);


  StaticJsonDocument<200> doc;

  doc["info"] = info;
  doc["serial"] = OZS_SERIAL_NUMBER;
  

  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);

 
  end_time = millis();

  unsigned long elapsedTime = end_time - start_time;
  Serial.print("Message handling and publishing took: ");
  Serial.print(elapsedTime);
  Serial.println(" ms");


}

void loop() {
  // Serial.printf(".");
  // publishMessage();
  client.loop();

  // 2024-05-06 :SerialPort로부터 데이터를 읽어옴
  // OZS 보드에서 MQTT가 연결이 되었는지 확인 메세지를 처리한다. 
  // 확인 메세지 "hello 8226"
  while (SerialPort.available() > 0) {
     // 시리얼 데이터 읽기
    char incomingChar = SerialPort.read();

    // '['가 들어오면 내부 문자열 시작
    if (incomingChar == '[') {
      insideBrackets = true;
      receivedString = ""; // 새로운 문자열 시작
    }
    // ']'가 들어오면 내부 문자열 끝
    else if (incomingChar == ']') {
      insideBrackets = false;
      
      // 내부 문자열을 처리 (예: 시리얼 모니터로 출력)
      Serial.println(" Rx=" + receivedString);

      // 2024-05-06 : "8266" 문자열이 포함되어 있는지 확인하여 처리
      if (receivedString.indexOf("8266") != -1) {
        // "8266"이 포함되어 있으면 send_wifi_ready() 함수 호출
        send_wifi_ready();
      }
      else if(receivedString.indexOf("OZS") != -1){
        publish_ozs_status(receivedString);
      }
      else if(receivedString.indexOf("SYSINFO") != -1){
        publish_ozs_system_info(receivedString);
      }
      else{
        Serial.println("loop: critical error:");
      }


    }
    // '['와 ']' 사이의 문자열인 경우 receivedString에 문자 추가
    else if (insideBrackets) {
      receivedString += incomingChar;
    }
  }

  //2024-05-29 : 10ms 마다 한번씩 체크한다. 시간을 획기적으로 줄였다.
  delay(10);
}