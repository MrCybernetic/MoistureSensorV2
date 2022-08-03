
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
//               ATTINY85
//               _________
//              |         |
// -Reset-/A0/5-|         |-VCC
//    CLKI/A3/3-|         |-2/SCK/SCL
//    CLKO/A2,4-|         |-1/MISO
//          GND-|         |-0/MOSI/SDA
//              |_________|

#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <avr/interrupt.h>
#include <avr/io.h>  // Adds useful constants
#include <avr/sleep.h>
#include <stdlib.h>

// Routines to set and clear bits (used in the sleep code)
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#define MOISTURE 1
#define BATTERY 2
#define TEMPERATURE 3

// Serial
SoftwareSerial mySerial(1, 0);  // rx,tx

// Pins
const int batteryPin = A3;
const int moisturePin = A2;
const int ESPPin = 2;

// Variables for the Sleep/power down modes:
volatile boolean f_wdt = 1;

// Variables for Serial com
const byte numChars = 32;
char receivedChars[numChars];  // an array to store the received data
boolean newData = false;
#define NONE 1
#define READY_RECEIVED 2
#define SENT_RECEIVED 3
int status = 0;
long previous_millis = 0;

void setup() {
    OSCCAL = 65;  // Tuning Attiny85 for Serial Garbages Fix, see https://jloh02.github.io/projects/tuning-attiny85/ and Attiny85Tuning.ino
    mySerial.begin(9600);
    pinMode(moisturePin, INPUT);
    pinMode(batteryPin, INPUT);
    pinMode(ESPPin, OUTPUT);
    digitalWrite(ESPPin, HIGH);
}

void loop() {
    unsigned long now = millis();
    recvWithStartEndMarkers();
    status = showNewData();
    String msg_data;
    switch (status) {
        case READY_RECEIVED:
            if (now - previous_millis >= 200) {
                msg_data += F("m:");
                msg_data += String(get_value(MOISTURE));
                msg_data += F(",b:");
                msg_data += String(get_value(BATTERY));
                msg_data += F(",t:");
                msg_data += String(get_value(TEMPERATURE));
                mySerial.print("<");
                mySerial.print(msg_data);
                mySerial.println(">");
                previous_millis = now;
            }
            break;
        case SENT_RECEIVED:
            setup_watchdog(9);
            digitalWrite(ESPPin, LOW);
            pinMode(ESPPin, INPUT);          // Set the ports to be inputs - saves more power
            for (int i = 0; i < 100; i++) {  // 675*8s=5400s=1h30min
                system_sleep();              // Send the unit to sleep
            }
            // Set the ports to be output again
            pinMode(ESPPin, OUTPUT);
            digitalWrite(ESPPin, HIGH);
            break;
        default:
            break;
    }
}

int get_value(int data) {
    int value = 0;
    switch (data) {
        case MOISTURE:
            analogRead(moisturePin);  // discard
            value = analogRead(moisturePin);
            delay(1);  // See https://www.quora.com/Why-is-a-little-delay-needed-after-analogRead-in-Arduino
            break;
        case BATTERY:
            analogRead(batteryPin);
            value = analogRead(batteryPin);
            delay(1);  // same
            break;
        case TEMPERATURE:
            get_temperature();
            value = get_temperature();
            delay(1);  // same
            break;
        default:
            break;
    }
    return value;
}

int get_temperature() {
    ADMUX = 0xaf & (0x8f | ADMUX);  // Set ADMUX to 0x8F in order to choose ADC4 and Set VREF to 1.1V (Without changing ADLAR p134 in datasheet)
    ADCSRA |= _BV(ADSC);            // Start conversion
    while ((ADCSRA & _BV(ADSC)));  // Wait until conversion is finished
    return (ADCH << 8) | ADCL;
}

// set system into the sleep state
// system wakes up when wtchdog is timed out
void system_sleep() {
    cbi(ADCSRA, ADEN);                    // switch Analog to Digitalconverter OFF
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);  // sleep mode is set here
    sleep_enable();
    sleep_mode();       // System actually sleeps here
    sleep_disable();    // System continues execution here when watchdog timed out
    sbi(ADCSRA, ADEN);  // switch Analog to Digitalconverter ON
}

// 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec
void setup_watchdog(int ii) {
    byte bb;
    int ww;
    if (ii > 9) ii = 9;
    bb = ii & 7;
    if (ii > 7) bb |= (1 << 5);
    bb |= (1 << WDCE);
    ww = bb;
    MCUSR &= ~(1 << WDRF);
    // start timed sequence
    WDTCR |= (1 << WDCE) | (1 << WDE);
    // set new watchdog timeout value
    WDTCR = bb;
    WDTCR |= _BV(WDIE);
}

// Watchdog Interrupt Service / is executed when watchdog timed out
ISR(WDT_vect) {
    f_wdt = 1;  // set global flag
}

void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;
    while (mySerial.available() > 0 && newData == false) {
        rc = mySerial.read();
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

int showNewData() {
    if (newData == true) {
        newData = false;
        if (strcmp(receivedChars, "Ready") == 0) {
            return READY_RECEIVED;
        } else if (strcmp(receivedChars, "Sent") == 0) {
            return SENT_RECEIVED;
        } else {
            return NONE;
        }
    }
}