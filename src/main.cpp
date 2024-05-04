
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

#include "secrets.h"            // Currently a file for Keys, Certs, etc
#include <WiFiClientSecure.h>   // From the core ESP library - Don't need to add this
#include <MQTTClient.h>         // Need to add library 256dpi/MQTT
#include <ArduinoJson.h>        // Need to add library bblanchon/ArduinoJSON
#include "ESP8266WiFi.h"        // From the core ESP library - Don't need to add this.

#include "serial_communication.h"



// The MQTT topics that this device should publish/subscribe to
#define AWS_IOT_PUBLISH_TOPIC   "ozs/test"
#define AWS_IOT_SUBSCRIBE_TOPIC "ozs/test"
WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);


// port(5,6) ozs serial 통신
SoftwareSerial SerialPort(D5,D6);

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
  client.begin(AWS_IOT_ENDPOINT, 8883, net);


  while (!client.connect(THINGNAME)) {
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

void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["time"] = millis();
  doc["sensor_a0"] = analogRead(0);
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

void messageHandler(String &topic, String &payload) {
  payload.replace("\\","");
  Serial.println("incoming: " + topic + " - " + payload);

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
}

void loop() {
  // Serial.printf(".");
  // publishMessage();
  client.loop();
  delay(1000);
}