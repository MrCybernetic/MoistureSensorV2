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
// Description:
//      1/ Connect to wifi
//      2/ Tell to the Attiny, "I am Ready"
//      3/ Wait to receive any data separate by \n
//      4/ Publish them via MQTT on topic_UP topic
//

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "credentials.h"

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;
const char *mqtt_server = MQTT_SERVER_URL;
const char *HostName = "Moisture_Sensor_01";
const char *topic_UP = "DATA0";
char *statusMsg = "";

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
    delay(10);
    /*Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);*/
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(HostName);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        // Serial.print(".");
        delay(500);
    }
    /*Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());*/
}
void reconnect() {
    while (!client.connected()) {
        // Serial.print("Attempting MQTT connection...");
        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);
        if (client.connect(clientId.c_str())) {
            // Serial.println("connected");
        } else {
            /*Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");*/
            delay(5000);
        }
    }
}
void setup() {
    Serial.begin(9600);
    setup_wifi();
    client.setServer(mqtt_server, 1883);
    statusMsg = "Ready";
}

void loop() {
    client.loop();
    if (client.connected()) {
        Serial.println(statusMsg);
        if (Serial.available() > 0) {
            String msg = Serial.readStringUntil('\n');
            char Buf[msg.length() + 1];
            msg.toCharArray(Buf, msg.length() + 1);
            client.publish(topic_UP, Buf);
            statusMsg = "Sent";
        }
    } else {
        reconnect();
    }
}
