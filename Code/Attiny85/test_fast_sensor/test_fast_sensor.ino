
//   __  __     _    _                __  __  ___ _____ _____
//  |  \/  |___(_)__| |_ _  _ _ _ ___|  \/  |/ _ \_   _|_   _|
//  | |\/| / _ \ (_-<  _| || | '_/ -_) |\/| | (_) || |   | |
//  |_|_ |_\___/_/__/\__|\_,_|_| \___|_|  |_|\__\_\|_|   |_|
//    /_\| |_| |_(_)_ _ _  _( _ ) __|
//   / _ \  _|  _| | ' \ || / _ \__ \                         
//  /_/ \_\__|\__|_|_||_\_, \___/___/
//                      |__/
//
// Cybernetic 06/2022
// Running on Attiny85
// Description:
//      1/ Turn On the ESP01
//      2/ If ESP01 ready, send battery voltage and moisture value
//      3/ If ESP01 return message sent, turn off the ESP01
//      4/ Go to sleep mode during 30minutes
//      5/ Repeat
//
//
//               ATTINY85
//               _________
//              |         |
// -Reset-/A0/5-|         |-VCC
//    CLKI/A3/3-|         |-2/SCK/SCL
//    CLKO/A2,4-|         |-1/MISO
//          GND-|         |-0/MOSI/SDA
//              |_________|

#include <SoftwareSerial.h>

#define MOISTURE 1
#define BATTERY 2
#define TEMPERATURE 3


// Serial
SoftwareSerial mySerial(1, 0);  // rx,tx

// Pins
const int batteryPin = A3;
const int moisturePin = A2;
const int ESPPin = 2;

// Variables
float moisture_value = 0.0;  // 0-1000
float battery_value = 0.0;
float temperature_value = 0.0;

void setup() {
    OSCCAL = 65;  // Tuning Attiny85 for Serial Garbages Fix, see https://jloh02.github.io/projects/tuning-attiny85/ and Attiny85Tuning.ino
    mySerial.begin(9600);
    pinMode(moisturePin, INPUT);
    pinMode(batteryPin, INPUT);
    pinMode(ESPPin, OUTPUT);
    digitalWrite(ESPPin, LOW);
}

void loop() {
    moisture_value = get(MOISTURE);
    battery_value = get(BATTERY);
    temperature_value = get(TEMPERATURE);
    digitalWrite(ESPPin, HIGH);
    mySerial.print("moisture:");
    mySerial.print(moisture_value);
    mySerial.print(",battery:");
    mySerial.print(battery_value);
    mySerial.print(",temperature:");
    mySerial.println(temperature_value);
    delay(1000);
}

float get(int data) {
    float value = 0.0;
    switch (data) {
        case MOISTURE:
            analogRead(moisturePin); //discard
            value = analogRead(moisturePin);
            delay(10);  // See https://www.quora.com/Why-is-a-little-delay-needed-after-analogRead-in-Arduino
            break;
        case BATTERY:
            analogRead(batteryPin); //discard
            value = analogRead(batteryPin);
            delay(10);  // same
            break;
        case TEMPERATURE:
            ADMUX = 0xaf & (0x8f | ADMUX);  // Set ADMUX to 0x8F in order to choose ADC4 and Set VREF to 1.1V (Without changing ADLAR p134 in datasheet)
            ADCSRA |= 0x40;                  // run conversion
            while (bitRead(ADCSRA, 6) == 1) {
                delayMicroseconds(10);
            }
            value = (ADCH << 8) | ADCL;
            delay(10);  // same
            break;
        default:
            break;
    }
    return value;
}