
/*********************************************************************

1. 2024.4.30일 aws-iot와 연결이 성공 버전......

  nodemcu: esp8266 과 컴파일 다운로드 성공

  참조 사이트 : https://how2electronics.com/connecting-esp8266-to-amazon-aws-iot-core-using-mqtt/

  참조 사이트 : https://github.com/camsaway/aws-iot-esp8266-mqtt-example/blob/main/README.md

  참조 사이트 : https://www.youtube.com/watch?v=xZoeJ-osS3g

  The purpose of this repo is to provide ESP8266 compatible code to connect to IoT Core MQTT service.

  It is based largely on the ESP32 example provided by AWS at this location. https://github.com/aws-samples/aws-iot-esp32-arduino-examples 
  
  아마존 사이트: 
  https://aws.amazon.com/blogs/compute/building-an-aws-iot-core-device-using-aws-serverless-and-an-esp32/

2. serial 인터페이스  참조 사이트 :
     https://www.jobtoy.com/project/arduino_view?idx=85

3. shadow 참조 사이트:
  https://github.com/jigneshk5/AWS-IoT-Core-with-ESP8266

**********************************************************************/

// #include <Arduino.h>

#include "secrets.h"            // Currently a file for Keys, Certs, etc
#include <WiFiClientSecure.h>   // From the core ESP library - Don't need to add this
// #include <MQTTClient.h>         // Need to add library 256dpi/MQTT
#include <ESP8266WiFi.h>        // From the core ESP library - Don't need to add this.

#include "serial_communication.h"
#include "timer.h"
#include <time.h>

#include <PubSubClient.h>
#include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson (use v6.xx)

void connectAWS();
void connectToMqtt(bool nonBlocking);
void messageReceived(char *topic, byte *payload, unsigned int length);

// The MQTT topics that this device should publish/subscribe to
#define AWS_IOT_PUBLISH_TOPIC   "ozs/test"

// 2024-05-05 : 디바이스 ID에 따른 topic을 지정했다.
// #define AWS_IOT_SUBSCRIBE_TOPIC "ozs/test/00001"
// #define AWS_IOT_SUBSCRIBE_TOPIC "ozs/test/00002"
#define AWS_IOT_SUBSCRIBE_TOPIC "ozs/test/00004"

const int MQTT_PORT = 8883;
const char* MQTT_SUB_TOPIC =  "$aws/things/ozs_0406/shadow/update/delta";
const char* MQTT_SUB_TOPIC_ACCEPTED =  "$aws/things/ozs_0406/shadow/update/accepted";
const char* MQTT_SUB_TOPIC_GET =  "$aws/things/ozs_0406/shadow/get/accepted";
const char* MQTT_PUB_TOPIC = "$aws/things/ozs_0406/shadow/update";
const char* MQTT_PUB_GET = "$aws/things/ozs_0406/shadow/get";


WiFiClientSecure net;
// WiFiClientSecure net = WiFiClientSecure();
// MQTTClient client = MQTTClient(256);

PubSubClient client(net);

// port(5,6) ozs serial 통신
SoftwareSerial SerialPort(D5,D6);

String receivedString = ""; // 수신된 문자열을 저장할 변수
bool insideBrackets = false; // '['와 ']' 사이의 문자열인지 여부를 나타내는 플래그

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
  struct tm timeinfo;
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

  // Configure WiFiClientSecure to use the AWS IoT device credentials
    // net.setCACert(AWS_CERT_CA);
    // net.setCertificate(AWS_CERT_CRT);
    // net.setPrivateKey(AWS_CERT_PRIVATE);

  // client.begin(AWS_IOT_ENDPOINT, 8883, net);


  // while (!client.connect(THINGNAME)) {
  //   Serial.print(".");
  //   delay(100);
  // }
  // if(!client.connected()){
  //   Serial.println("AWS IoT Timeout!");
  //   return;
  // } else {
  //   Serial.println("AWS IoT Connected!");
  // }

  // // Subscribe to a topic
  // client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
  // Serial.println("Topic Subscribed");

  client.setServer(AWS_IOT_ENDPOINT, MQTT_PORT);
  client.setCallback(messageReceived);


  connectToMqtt(false);
}

void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["time"] = millis();
  doc["sensor_a0"] = analogRead(0);
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

// void messageHandler(String &topic, String &payload) {
//   payload.replace("\\","");
//   Serial.println("incoming: " + topic + " - " + payload);

//   on_message_received(topic, payload );

// //  StaticJsonDocument<200> doc;
// //  deserializeJson(doc, payload);
// //  const char* message = doc["message"];
// }

void messageReceived(char *topic, byte *payload, unsigned int length)
{

  Serial.println("messageReceived");

  String msgIN = "";
  for (int i = 0; i < length; i++)
  {
    msgIN += (char)payload[i];
  }
  String msgString = msgIN;
  Serial.println("Recieved [" + String(topic) + "]: " + msgString);

  StaticJsonDocument<1000> doc;
  // StaticJsonDocument<64> filter;

  // DeserializationError error = deserializeJson(doc, msgString, DeserializationOption::Filter(filter));
  DeserializationError error = deserializeJson(doc, msgString);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  // Serialize JSON document to a string
    char jsonBuffer[200];
    serializeJsonPretty(doc, jsonBuffer, sizeof(jsonBuffer));

// Print JSON string
    Serial.println("Parsed JSON document:");
    Serial.println(jsonBuffer);

  if(String(topic) == "$aws/things/ozs_0406/shadow/update/delta"){
    JsonObject stateObj = doc["state"];
    if(stateObj.containsKey("power_1")){
      String power_1 = stateObj["power_1"];
      Serial.println("power_1 = " + power_1);

      if(power_1 == "on"){
        Serial.println("power_ 1 == on");
        // power를 on 하고  reported를 on으로 한다. 
        // delta가 발생했고, 'on'을 원하지만, reported는 'off' 이다. 즉 디바이스는 'off'이다.
        // 따라서  
        Serial.println("activate device on and change power_1 to on ");
        String str= "{\"state\" : {\"reported\" : {\"power_1\" : \"on\"}}}";
        client.publish(MQTT_PUB_TOPIC, str.c_str());
      }
    }

  }
  
  else if(String(topic) == "$aws/things/ozs_0406/shadow/get/accepted"){
      // String redstr = doc["state"]["desired"]["red"];
      // String greenstr = doc["state"]["desired"]["green"];
      Serial.println("get/accepted: ");
      
      // if (redstr == "1") {
      //   digitalWrite(red, HIGH);              
      // } else {
      //   digitalWrite(red, LOW);
      // }

      // if (greenstr == "1") {
      //   digitalWrite(green, HIGH);              
      // } else {
      //   digitalWrite(green, LOW);
      // }
    // String str = "{ \"state\": { \"reported\": { \"red\":"+redstr+", \"green\":"+greenstr+"} } }";
    // client.publish("$aws/things/ozs_0406/shadow/update",str.c_str ());
  }
  Serial.println();
}

void pubSubErr(int8_t MQTTErr)
{
  if (MQTTErr == MQTT_CONNECTION_TIMEOUT)
    Serial.print("Connection tiemout");
  else if (MQTTErr == MQTT_CONNECTION_LOST)
    Serial.print("Connection lost");
  else if (MQTTErr == MQTT_CONNECT_FAILED)
    Serial.print("Connect failed");
  else if (MQTTErr == MQTT_DISCONNECTED)
    Serial.print("Disconnected");
  else if (MQTTErr == MQTT_CONNECTED)
    Serial.print("Connected");
  else if (MQTTErr == MQTT_CONNECT_BAD_PROTOCOL)
    Serial.print("Connect bad protocol");
  else if (MQTTErr == MQTT_CONNECT_BAD_CLIENT_ID)
    Serial.print("Connect bad Client-ID");
  else if (MQTTErr == MQTT_CONNECT_UNAVAILABLE)
    Serial.print("Connect unavailable");
  else if (MQTTErr == MQTT_CONNECT_BAD_CREDENTIALS)
    Serial.print("Connect bad credentials");
  else if (MQTTErr == MQTT_CONNECT_UNAUTHORIZED)
    Serial.print("Connect unauthorized");
}

void connectToMqtt(bool nonBlocking = false)
{
  Serial.println("MQTT connecting ");
  while (!client.connected())
  {
    if (client.connect(THINGNAME))
    {
      Serial.println("connected!");
      if (!client.subscribe(MQTT_SUB_TOPIC))
        pubSubErr(client.state());
      if (!client.subscribe(MQTT_SUB_TOPIC_ACCEPTED))
        pubSubErr(client.state());
      if (!client.subscribe(MQTT_SUB_TOPIC_GET))
        pubSubErr(client.state());
      // if (!client.subscribe(MQTT_PUB_GET)) // 추가
      //   pubSubErr(client.state()); 
           
      // client.publish("$aws/things/ozs_0406/shadow/get","");  //For getting last saved state
    }
    else
    {
      Serial.print("failed, reason -> ");
      pubSubErr(client.state());
      if (!nonBlocking)
      {
        Serial.println(" < try again in 5 seconds");
        delay(5000);
      }
      else
      {
        Serial.println(" <");
      }
    }
    if (nonBlocking)
      break;
  }
}

void setup() {
  // SerialPort.begin(115200); //(5,6) port 
  Serial.begin(115200);
  Serial.println(""); Serial.println(""); 
  Serial.println("SETUP");
  connectAWS();
  Serial.println(""); 
  Serial.println(""); 
  // client.onMessage(messageHandler);

  // 2024-05-05 : 네트웍 준비 송부
  // send_wifi_ready();
}

void loop() {
  // Serial.printf(".");
  // publishMessage();
  client.loop();

  

 // 2024-05-06 :SerialPort로부터 데이터를 읽어옴
  // OZS 보드에서 MQTT가 연결이 되었는지 확인 메세지를 처리한다. 
  // 확인 메세지 "hello 8226"
  while (Serial.available() > 0) {
     // 시리얼 데이터 읽기
    char incomingChar = Serial.read();

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

      // // 2024-05-06 : "8266" 문자열이 포함되어 있는지 확인하여 처리
      // if (receivedString.indexOf("8266") != -1) {
      //   // "8266"이 포함되어 있으면 send_wifi_ready() 함수 호출
      //   send_wifi_ready();
      // }

        if(receivedString == "on"){

          Serial.println("incomingChar [on] input");
          String str= "{\"state\" : {\"desired\" : {\"power_1\" : \"on\"}}}";
          client.publish(MQTT_PUB_TOPIC, str.c_str ());

        }
        else if(receivedString == "off"){

          Serial.println("incomingChar [off] input");
          String str= "{\"state\" : {\"desired\" : {\"power_1\" : \"off\"}}}";
          client.publish(MQTT_PUB_TOPIC, str.c_str ());

        }
        else if(receivedString == "reported"){

          Serial.println("incomingChar [reported] input");
          String str= "{\"state\" : {\"reported\" : {\"power_1\" : \"off\"}}}";
          client.publish(MQTT_PUB_TOPIC, str.c_str ());

        }
        else if(receivedString == "get"){

          Serial.println("incomingChar [get] input");
          String str= "{\"state\" : {\"reported\" : {\"power_1\" : \"off\"}}}";
          client.publish("$aws/things/ozs_0406/shadow/get", str.c_str());  //For getting last saved state

        }
        else{
          Serial.println("wrong input");
        }

    }
    // '['와 ']' 사이의 문자열인 경우 receivedString에 문자 추가
    else if (insideBrackets) {
      receivedString += incomingChar;
    }
  }


  

  // delay(10000);
}