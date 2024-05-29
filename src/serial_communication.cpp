#include "serial_communication.h"
#include <ArduinoJson.h>        // Need to add library bblanchon/ArduinoJSON

 #define DEBUG

void send_serial_data(const char *data) {
    // Implementation of serial write function

    String newData = "[" + String(data) + "]";

#ifdef DEBUG
    Serial.printf("json data = %s\n", newData.c_str());
#endif

    // Your serial write logic goes here
    // Example: serial_write_function(data);
    // For demonstration, I'm just printing the data here
    // Serial write logic for all characters in newData
    for (int i = 0; i < newData.length(); i++) {
        char currentChar = newData.charAt(i);
        
        // Serial.printf("Sending character: %c \n", currentChar);
        
        SerialPort.write(currentChar);
        
        //2024-05-29 : 시리얼로 전달할 때 delay가 필요하다. 이것이 없으면 통신이 안되는 경우 발생.
        delay(2);

        // Your serial write logic goes here
        // Example: serial_write_function(currentChar);
        // For demonstration, I'm just printing the character here
    }
}

// wifi ready send
void send_wifi_ready() {
    Serial.println("send wifi ready to ozs");

    send_serial_data("{wifi:on:}");

}

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

    // Serialize JSON document to a string
    char jsonBuffer[200];
    serializeJsonPretty(doc, jsonBuffer, sizeof(jsonBuffer));

#ifdef DEBUG  
    // Print JSON string
    Serial.println("send_power_ozs Parsed JSON document:");
    Serial.println(jsonBuffer);
#endif


    // Handle different commands
    if (doc["power"] == "on") {
         Serial.print("send_power_ozs power on");
         Serial.println();
        send_serial_data("{power:on:}");

    } else if (doc["power"] == "off") {
        Serial.print("send_power_ozs power off");
         Serial.println();
          send_serial_data("{power:off:}");
    } else {
        Serial.println("Key not found in data");
    }

    // start ozs
    send_serial_data("{start:act:}"); // Additional data transmission
}

void send_stop_ozs() {
    Serial.println("send stop cmd");

    send_serial_data("{stop:on:}");
    send_serial_data("{start:act:}");
}

void send_check_ozs_status() {
    Serial.println("send check ozs status");

    send_serial_data("{status:status:}");
    send_serial_data("{start:act:}");
}

void send_check_system_info() {
    Serial.println("check system info");

    send_serial_data("{status:sysinfo:}");
    send_serial_data("{start:act:}");
}

void send_start_ozs(int action, int duration, int wind_speed) {

    Serial.println("send start cmd :");
    Serial.printf("action = %d\n", action);
    Serial.printf("time = %d\n", duration);
    Serial.printf("wind = %d\n", wind_speed);

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
        Serial.printf("Invalid action command: %d\n", action);
    }

    // Wind strength transmission
    if (wind_speed == 1) {
        send_serial_data("{wind:1:}");
    } else if (wind_speed == 2) {
        send_serial_data("{wind:2:}");
    } else if (wind_speed == 3) {
        send_serial_data("{wind:3:}");
    } else {
        Serial.printf("Invalid wind speed command: %d\n", wind_speed);
    }

    // Time duration
    if (duration == 1) {
        send_serial_data("{duration:30:}");
    } else if (duration == 2) {
        send_serial_data("{duration:60:}");
    } else if (duration == 3) {
        send_serial_data("{duration:90:}");
    } else {
        Serial.printf("Invalid duration command: %d\n", duration);
    }

    // Start OZS
    send_serial_data("{start:act:}");
}

void on_message_received(String &topic, String &payload) {

#ifdef DEBUG
    Serial.print("Received message from topic '");
    Serial.print(topic);
    Serial.print("': ");
    Serial.println(payload);
#endif

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

#ifdef DEBUG
    // Print JSON string
    Serial.println("Parsed JSON document:");
    Serial.println(jsonBuffer);
#endif

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
#ifdef DEBUG
            Serial.print("Action: ");
            Serial.println(action);
            Serial.print("Time: ");
            Serial.println(time);
            Serial.print("Wind: ");
            Serial.println(wind);
#endif
            // Now you can use the extracted values as needed
            send_start_ozs(action, time, wind);

        } else {
            Serial.println("One or more keys (action, time, wind) not found within 'start' object");
        }
    } 
    else if(doc.containsKey("stop")){
        send_stop_ozs();
    }
    else if(doc.containsKey("power")){
        send_power_ozs(payload);
    }

    else if(doc.containsKey("status")){
       // Print the value of "status"
        const char* statusValue = doc["status"].as<const char*>();
        // Serial.print("Status: ");
        // Serial.println(statusValue);

        // Additional logic based on the value of "status"
        if (strcmp(statusValue, "status") == 0) {
            #ifdef DEBUG
            Serial.println("Received status check...");
            #endif
            send_check_ozs_status();
        } else if (strcmp(statusValue, "sysinfo") == 0) {
            #ifdef DEBUG
            Serial.println("Received system info inquiry");
            #endif
            send_check_system_info();
        } else {
            Serial.println("Error: unrecognized status value");
        }
        //  publishMessage();

    }
    
    
    else {
        Serial.printf("No 'start' key found in data");
    }

    
}
