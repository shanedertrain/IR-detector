#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

// Pin definitions
#define SWITCH_PIN 7
#define FRONT_LEFT A0
#define FRONT_RIGHT A1
#define REAR_LEFT A2
#define REAR_RIGHT A3
#define MIDDLE_LEFT A4
#define MIDDLE_RIGHT A5
#define BATTERY_READ_PIN A7

// ADC setup
Adafruit_ADS1115 ads1;
Adafruit_ADS1115 ads2;

// MP3 player setup
SoftwareSerial mp3Serial(0, 1);
DFRobotDFPlayerMini mp3Player;

// Battery threshold and debounce settings
#define LOW_BATTERY_THRESHOLD 10 //percent
#define TRIP_THRESHOLD 16383 //0 to 32767 for input voltages between -6.144V to +6.144V, depending on the gain setting.
#define TRIP_DEBOUNCE_MS 250 //time an IR sensor must be above threshold to trigger playing an MP3 file

// debounce timer for all trips
static unsigned long tripTime = 0;

void setup() {
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  mp3Serial.begin(9600);
  mp3Player.begin(mp3Serial);
  mp3Player.setTimeOut(500);
  mp3Player.volume(15);
  ads1.begin();
  ads2.begin();
}

void loop() {
    // Declare variables
    unsigned long currentTime = millis();

    // Read sensor values
    int frontLeft = ads1.readADC_SingleEnded(FRONT_LEFT);
    int frontRight = ads1.readADC_SingleEnded(FRONT_RIGHT);
    int rearLeft = ads1.readADC_SingleEnded(REAR_LEFT);
    int rearRight = ads1.readADC_SingleEnded(REAR_RIGHT);
    int middleLeft = ads2.readADC_SingleEnded(MIDDLE_LEFT);
    int middleRight = ads2.readADC_SingleEnded(MIDDLE_RIGHT);

    // Check if circuit switch is on or off
    if (digitalRead(SWITCH_PIN) == LOW) 
    {
        mp3Player.playMp3Folder(6);
    } 
    else 
    {
        mp3Player.playMp3Folder(7);
    }

    // Check battery level
    if (analogRead(BATTERY_READ_PIN) < LOW_BATTERY_THRESHOLD) 
    {
        mp3Player.playMp3Folder(5);
    }
  
    if (digitalRead(SWITCH_PIN) == LOW) {
        mp3Player.playMp3Folder(6);
    } else {
        // Check for all trips and debounce
        if (frontLeft > TRIP_THRESHOLD || frontRight > TRIP_THRESHOLD ||
            rearLeft > TRIP_THRESHOLD || rearRight > TRIP_THRESHOLD ||
            middleLeft > TRIP_THRESHOLD || middleRight > TRIP_THRESHOLD) {
            if (currentTime - tripTime >= TRIP_DEBOUNCE_MS) {
                if (frontLeft > TRIP_THRESHOLD || frontRight > TRIP_THRESHOLD) {
                    mp3Player.playMp3Folder(4);
                } else if (rearLeft > TRIP_THRESHOLD || rearRight > TRIP_THRESHOLD) {
                    mp3Player.playMp3Folder(3);
                } else if (middleLeft > TRIP_THRESHOLD) {
                    mp3Player.playMp3Folder(2);
                } else if (middleRight > TRIP_THRESHOLD) {
                    mp3Player.playMp3Folder(1);
                }
                tripTime = currentTime;
            }
        }
    }
}