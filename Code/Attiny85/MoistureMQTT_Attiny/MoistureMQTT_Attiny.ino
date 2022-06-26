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

// Serial
SoftwareSerial mySerial(1, 0);  // rx,tx

// Pins
const int batteryPin = A3;
const int moisturePin = A2;
const int ESPPin = 2;

// Variables
float moistureValue = 0;  // 0-1000
float batteryValue = 0;

// Variables for the Sleep/power down modes:
volatile boolean f_wdt = 1;

void setup() {
    OSCCAL = 64;  // Tuning Attiny85 for Serial Garbages Fix, see sketch Attiny85Tuning.ino (take the min value with hieroglyph and max and the average is the good one)
    mySerial.begin(9600);
    setup_watchdog(9);
    pinMode(moisturePin, INPUT);
    pinMode(batteryPin, INPUT);
    pinMode(ESPPin, OUTPUT);
    digitalWrite(ESPPin, LOW);
    mySerial.println("Hello");
}

void loop() {
    if (f_wdt == 1) {  // wait for timed out watchdog / flag is set when a watchdog timeout occurs
        f_wdt = 0;     // reset flag
        moistureValue = analogRead(moisturePin);
        batteryValue = analogRead(batteryPin);
        digitalWrite(ESPPin, HIGH);
        while (mySerial.available() == 0) {
        }
        if (mySerial.available() > 0) {
            String msg = mySerial.readStringUntil('\n');
            if (msg == "Ready") {
                mySerial.print("moisture:");
                mySerial.print(moistureValue);
                mySerial.print(",battery:");
                mySerial.println(batteryValue);
                while (mySerial.available() == 0) {
                }
            }
            if (msg == "Sent") {
                digitalWrite(ESPPin, LOW);
                // Set the ports to be inputs - saves more power
                pinMode(ESPPin, INPUT);
                for (int i = 0; i < 255; i++) {  // 225*8s=1800s=30min
                    system_sleep();              // Send the unit to sleep
                }
                // Set the ports to be output again
                pinMode(ESPPin, OUTPUT);
            }
        }
    }
}

// set system into the sleep state
// system wakes up when wtchdog is timed out
void system_sleep() {
    cbi(ADCSRA, ADEN);  // switch Analog to Digitalconverter OFF
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);  // sleep mode is set here
    sleep_enable();
    sleep_mode();  // System actually sleeps here
    sleep_disable();  // System continues execution here when watchdog timed out
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
