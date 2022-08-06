//   __  __     _    _                __  __  ___ _____ _____
//  |  \/  |___(_)__| |_ _  _ _ _ ___|  \/  |/ _ \_   _|_   _|
//  | |\/| / _ \ (_-<  _| || | '_/ -_) |\/| | (_) || |   | |
//  |_|_ |_\___/_/__/\__|\_,_|_| \___|_|  |_|\__\_\|_|   |_|
//  | __/ __| _ \/  \/ |
//  | _|\__ \  _/ () | |
//  |___|___/_|  \__/|_|
//
// Cybernetic 06/2022
// Running on ESP01

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "credentials.h"

//// VARIABLES ////
// MQTT
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;
const char *mqtt_server = MQTT_SERVER_URL;
const char *HostName = "Moisture_Sensor_02";
const char *topic_UP = "DATA1";
WiFiClient espClient;
PubSubClient client(espClient);
// Time
unsigned long previous_millis = 0;
// Serial com
const byte numChars = 32;
boolean newData = false;
char receivedChars[numChars];
char *statusMsg = "";

void setup() {
    Serial.begin(9600);
    setup_wifi();
    client.setServer(mqtt_server, 1883);
    statusMsg = "<Ready>";
}

void loop() {
    client.loop();
    if (client.connected()) {
        unsigned long now = millis();
        recvWithStartEndMarkers();
        if (now - previous_millis >= 200) {
            Serial.println(statusMsg);
            previous_millis = now;
        }
        if (newData) {
            newData = false;
            client.publish(topic_UP, receivedChars);
            statusMsg = "<Sent>";
        }
    } else {
        reconnect();
    }
}

void setup_wifi() {
    delay(10);
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(HostName);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
}

void reconnect() {
    while (!client.connected()) {
        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);
        if (!client.connect(clientId.c_str())) {
            delay(5000);
        }
    }
}

void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();
        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            } else {
                receivedChars[ndx] = '\0';  // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        } else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}
