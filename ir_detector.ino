#include <Wire.h>  // Required for I2C communication with ADS1115
#include <Adafruit_ADS1015.h>  // Required for ADS1115 ADC
#include <SoftwareSerial.h>
#include <DFMiniMp3.h>  //Required for DFPlayer Mini MP3Player player
#include <avr/sleep.h> //sleep/low power functions

// Pin definitions
#define PHOTO_FRONT_LEFT_PIN 0
#define PHOTO_FRONT_RIGHT_PIN 1
#define PHOTO_REAR_LEFT_PIN 2
#define PHOTO_REAR_RIGHT_PIN 3
#define PHOTO_MIDDLE_LEFT_PIN 4
#define PHOTO_MIDDLE_RIGHT_PIN 5
#define POWER_SWITCH_PIN 6 
#define SPEAKER_PIN 7
#define PLAY_SWITCH_PIN 8
#define BATTERY_READ_PIN A0
#define VOLUME_PIN A1

// audio file definitions
#define SPEECH_001 1  // Speech file number for middle right photodiode
#define SPEECH_002 2  // Speech file number for middle left photodiode
#define SPEECH_003 3  // Speech file number for rear left/right photodiodes
#define SPEECH_004 4  // Speech file number for front left/right photodiodes
#define SPEECH_005 5  // Speech file number for low battery
#define SPEECH_006 6  // Speech file number for on message
#define SPEECH_007 7  // Speech file number for off message

// ADC setup
Adafruit_ADS1115 ads1;
Adafruit_ADS1115 ads2;

// MP3 player setup
SoftwareSerial MP3Serial(0, 1);
DFMiniMp3 MP3Player;

// Battery threshold and debounce settings
#define LOW_BATTERY_PERCENT 10
#define TRIP_ADC_THRESHOLD 16383 //-32767 to 32767 for input voltages between -6.144V to +6.144V, depending on the gain setting.
#define TRIP_DEBOUNCE_MS 250 //time an IR sensor must be above threshold to trigger playing an MP3 file

//battery vars
int battery_percent = 0; // or some other initial value

// debounce timer for all trips
static unsigned long tripTime = 0;

void enable_peripherals()
{
    //mp3 player 
    MP3Serial.begin(9600); // init hardware serial for MP3Player
    MP3Player.begin(MP3Serial); //init serial line to mp3 player

    //ADC
    ads1.begin(); //Init front photodiode ADC
    ads2.begin(); //Init rear and middle photodiode ADC
}

void disable_peripherals()
{
    //mp3 player
    MP3Serial.end(); // stop MP3Player player communication

    //ADC
    ads1.disableADC(); //Disable front photodiode ADC
    ads2.disableADC(); //Disable rear and middle photodiode ADC
}

void exit_sleep_mode()
{
    pinMode(POWER_SWITCH_PIN, INPUT);  // init on/off switch pin to input
    detachInterrupt(digitalPinToInterrupt(POWER_SWITCH_PIN)); 

    ADCSRA = 1; // ADC on
    TWCR = 1;   // TWI on

    enable_peripherals();

    MP3Player.playMp3Folder(SPEECH_006); //on message
}

void enter_sleep_mode()
{
    MP3Player.playMp3Folder(SPEECH_007); //sleep message
    
    disable_peripherals();

    ADCSRA = 0; // ADC off
    TWCR = 0;   // TWI off

    pinMode(POWER_SWITCH_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(POWER_SWITCH_PIN), exit_sleep_mode, FALLING);

    // Put the microcontroller to sleep
    sleep_enable();
    sleep_mode();

    //waiting for interrupt to trigger here...

    sleep_disable();
}

void setup() 
{
    pinMode(POWER_SWITCH_PIN, INPUT);  // init on/off switch pin to input
    pinMode(PLAY_SWITCH_PIN, INPUT);  // init play switch pin to input
    pinMode(SPEAKER_PIN, OUTPUT);  // init speaker pin to output

    set_sleep_mode(SLEEP_MODE_PWR_DOWN); //init setting for sleep mode
    
    enable_peripherals();

    //mp3 player
    MP3Player.volume(15); //initial volume
    MP3Player.EQ(DFPLAYER_EQ_NORMAL);  // Set EQ to normal mode

    //ADC
    ads1.setGain(GAIN_ONE);  // Front photodiode ADC gain
    ads2.setGain(GAIN_ONE);  // Rear and middle photodiode ADC gain
}

void loop() 
{
    unsigned long current_time = millis();

    // Read sensor values
    int adc_front_left = ads1.readADC_SingleEnded(PHOTO_FRONT_LEFT_PIN);
    int adc_front_right = ads1.readADC_SingleEnded(PHOTO_FRONT_RIGHT_PIN);
    int adc_rear_left = ads1.readADC_SingleEnded(PHOTO_REAR_LEFT_PIN);
    int adc_rear_right = ads1.readADC_SingleEnded(PHOTO_REAR_RIGHT_PIN);
    int adc_middle_left = ads2.readADC_SingleEnded(PHOTO_MIDDLE_LEFT_PIN);
    int adc_middle_right = ads2.readADC_SingleEnded(PHOTO_MIDDLE_RIGHT_PIN);

    // Check battery level
    if (battery_percent < LOW_BATTERY_PERCENT) 
    {
        MP3Player.playMp3Folder(SPEECH_005);
    }

    // Check for all trips and debounce
    if (adc_front_left > TRIP_ADC_THRESHOLD || adc_front_right > TRIP_ADC_THRESHOLD ||
        adc_rear_left > TRIP_ADC_THRESHOLD || adc_rear_right > TRIP_ADC_THRESHOLD ||
        adc_middle_left > TRIP_ADC_THRESHOLD || adc_middle_right > TRIP_ADC_THRESHOLD) 
    {
        if (current_time - tripTime >= TRIP_DEBOUNCE_MS) 
        {
            if (adc_front_left > TRIP_ADC_THRESHOLD || adc_front_right > TRIP_ADC_THRESHOLD) 
            {
                MP3Player.playMp3Folder(SPEECH_004);
            }
            else if (adc_rear_left > TRIP_ADC_THRESHOLD || adc_rear_right > TRIP_ADC_THRESHOLD) 
            {
                MP3Player.playMp3Folder(SPEECH_003);
            } 
            else if (adc_middle_left > TRIP_ADC_THRESHOLD) 
            {
                MP3Player.playMp3Folder(SPEECH_002);
            } 
            else if (adc_middle_right > TRIP_ADC_THRESHOLD) 
            {
                MP3Player.playMp3Folder(SPEECH_001);
            }
            tripTime = current_time;
        }
    }

    // Check if circuit switch is open
    if (digitalRead(POWER_SWITCH_PIN) == LOW) 
    {
        enter_sleep_mode();
    } 
}