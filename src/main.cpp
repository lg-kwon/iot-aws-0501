
/*********************************************************************

1. 2024.4.30일 aws-iot와 연결이 성공 버전......

  nodemcu: esp8266 과 컴파일 다운로드 성공

  참조 사이트 : https://how2electronics.com/connecting-esp8266-to-amazon-aws-iot-core-using-mqtt/

  참조 사이트 : https://github.com/camsaway/aws-iot-esp8266-mqtt-example/blob/main/README.md

  The purpose of this repo is to provide ESP8266 compatible code to connect to IoT Core MQTT service.

  It is based largely on the ESP32 example provided by AWS at this location. https://github.com/aws-samples/aws-iot-esp32-arduino-examples https://aws.amazon.com/blogs/compute/building-an-aws-iot-core-device-using-aws-serverless-and-an-esp32/

**********************************************************************/

#include <Arduino.h>

#include "secrets.h"            // Currently a file for Keys, Certs, etc
#include <WiFiClientSecure.h>   // From the core ESP library - Don't need to add this
#include <MQTTClient.h>         // Need to add library 256dpi/MQTT
#include <ArduinoJson.h>        // Need to add library bblanchon/ArduinoJSON
#include "ESP8266WiFi.h"        // From the core ESP library - Don't need to add this.

// #include "serial_communication.h"


void on_message_received(String &topic, String &payload);



// The MQTT topics that this device should publish/subscribe to
#define AWS_IOT_PUBLISH_TOPIC   "ozs/test"
#define AWS_IOT_SUBSCRIBE_TOPIC "ozs/test"
WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);


// Define your serial write function here
void send_serial_data(const char *data) {
    // Implementation of serial write function
    printf("json data = %s\n", data);
    // Your serial write logic goes here
    // Example: serial_write_function(data);
    // For demonstration, I'm just printing the data here
}

// Function to send power command to OZS board
void send_power_ozs(String &power) {
    Serial.println("send power cmd");

    // Parse JSON payload
    StaticJsonDocument<200> doc; // Adjust the size as needed
    DeserializationError error = deserializeJson(doc, power);
    
    if (error) {
        Serial.print("send_power_ozs deserializeJson() failed: ");
        Serial.println(error.f_str());
        return;
    }
    // if (power == "on") {
    //     json["power"] = "on";
    // } else if (power == "off") {
    //     json["power"] = "off";
    // }

    // Handle different commands
    if (doc["power"] == "on") {
         Serial.print("send_power_ozs power on");
         Serial.println();
    } else if (doc["power"] == "off") {
        Serial.print("send_power_ozs power off");
         Serial.println();
    } else {
        Serial.println("Key not found in data");
    }

    // String output;
    // serializeJson(json, output); // JSON 객체를 직렬화하여 문자열로 변환합니다.

    // send_serial_data(output); // 직렬 데이터를 전송합니다.
    send_serial_data("{start:act:}"); // 추가적인 데이터를 전송합니다.
}
// Function to send stop command to OZS board
void send_stop_ozs() {
    printf("send stop cmd\n");

    send_serial_data("{stop:on:}");
    send_serial_data("{start:act:}");
}

// Function to send start command to OZS with data
void send_start_ozs(int action, int duration, int wind_speed) {
    printf("start :\n");
    printf("action = %d\n", action);
    printf("time = %d\n", duration);
    printf("wind = %d\n", wind_speed);

    // Power on transmission
    send_serial_data("{power:on:}");

    // Mode transmission
    if (action == 1) {
        send_serial_data("{mode:1:}");
    } else if (action == 2) {
        send_serial_data("{mode:2:}");
    } else if (action == 3) {
        send_serial_data("{mode:3:}");
    } else {
        printf("Invalid action command: %d\n", action);
    }

    // Wind strength transmission
    if (wind_speed == 1) {
        send_serial_data("{wind:1:}");
    } else if (wind_speed == 2) {
        send_serial_data("{wind:2:}");
    } else if (wind_speed == 3) {
        send_serial_data("{wind:3:}");
    } else {
        printf("Invalid wind speed command: %d\n", wind_speed);
    }

    // Time duration
    if (duration == 1) {
        send_serial_data("{duration:30:}");
    } else if (duration == 2) {
        send_serial_data("{duration:60:}");
    } else if (duration == 3) {
        send_serial_data("{duration:90:}");
    } else {
        printf("Invalid duration command: %d\n", duration);
    }

    // Start OZS
    send_serial_data("{start:act:}");
}


// Callback when the subscribed topic receives a message
void on_message_received(String &topic, String &payload) {
    Serial.print("Received message from topic '");
    Serial.print(topic);
    Serial.print("': ");
    Serial.println(payload);

    // Parse JSON payload
    StaticJsonDocument<200> doc; // Adjust the size as needed
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.f_str());
        return;
    }

    // Serialize JSON document to a string
    char jsonBuffer[200];
    serializeJsonPretty(doc, jsonBuffer, sizeof(jsonBuffer));

    // Print JSON string
    Serial.println("Parsed JSON document:");
    Serial.println(jsonBuffer);

    // Check if "start" key exists
    if (doc.containsKey("start")) {
        // Get the "start" object
        JsonObject startObj = doc["start"];
        // Check if "action", "time", "wind" keys exist within "start" object
        if (startObj.containsKey("action") && startObj.containsKey("time") && startObj.containsKey("wind")) {
            // Get the values of "action", "time", "wind"
            int action = startObj["action"];
            int time = startObj["time"];
            int wind = startObj["wind"];
            Serial.print("Action: ");
            Serial.println(action);
            Serial.print("Time: ");
            Serial.println(time);
            Serial.print("Wind: ");
            Serial.println(wind);
            // Now you can use the extracted values as needed
        } else {
            Serial.println("One or more keys (action, time, wind) not found within 'start' object");
        }
    } else {
        Serial.println("No 'start' key found in data");
    }

    // // Handle different commands
    // if (doc.containsKey("power")) {
    //     send_power_ozs(payload);
    // } else if (doc.containsKey("stop")) {
    //     send_stop_ozs();
    // } else if (doc.containsKey("start")) {
    //     send_start_ozs(action, duration, wind_speed);
    // } else {
    //     Serial.println("Key not found in data");
    // }
}



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
  Serial.begin(115200);
  Serial.println(""); Serial.println(""); 
  Serial.println("SETUP");
  connectAWS();
  Serial.println(""); Serial.println(""); 
  client.onMessage(messageHandler);
}

void loop() {
  Serial.printf(".");
  // publishMessage();
  client.loop();
  delay(10000);
}