
/* ************************************************************************** */ 
/*                           ORIGINAL COPYRIGHT TEXT                          */ 
/* ************************************************************************** */ 

/*
  This sketch reads the sensor and creates a color bar graph on a tiny TFT

  Designed specifically to work with the Adafruit AS7262 breakout and 160x18 tft
  ----> http://www.adafruit.com/products/3779
  ----> http://www.adafruit.com/product/3533
  
  These sensors use I2C to communicate. The device's I2C address is 0x49
  Adafruit invests time and resources providing this open source code,
  please support Adafruit andopen-source hardware by purchasing products
  from Adafruit!
  
  Written by Dean Miller for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
  */

/* ************************************************************************** */ 
/*                             HARDWARE CONNECTIONS                           */ 
/* ************************************************************************** */ 

/*
              Arduino Nano        BLE SPI Friend Module
              ============        ======================

                   /
                   | D11 (MOSI) -----> MOSI
                   | D12 (MISO) -----> MISO
              SPI <  D13 (SCK)  -----> SCLK
                   |     D8     -----> CS
                   \
                   /
                   |     D7     -----> IRQ
                   |     D4     -----> RESET
                   \

                   /
                   |   3.3V    <====> 3.3V 
              Pwr  |   GND     <====> GND
                   \


              Arduino Nano        AS7262 Spectral Sensor
              ============        ======================

                    /
                   |  A4 (SDA)  -----> SDA 
              I2C  |  A5 (SCL)  -----> SCL 
                   \

                   /
                   |   3.3V    <====> VIN 
              Pwr  |   GND     <====> GND
                   \

*/


/* ************************************************************************** */ 
/*                           INCLUDE HEADERS SECTION                          */
/* ************************************************************************** */ 

// Support for the Git version tags
#include "git-version.h"

// Adafruit Spectral Sensor library
#include <Adafruit_AS726x.h>

// Adafruit Bluetooth libraries
#include <Adafruit_BLE.h>
#include <Adafruit_BluefruitLE_SPI.h>

/* ************************************************************************** */ 
/*                                DEFINEs SECTION                             */
/* ************************************************************************** */ 

#ifndef GIT_VERSION
#define GIT_VERSION "0.1.0" // For downloads without git
#endif

// BLUETOOTH SHARED SPI SETTINGS
// -----------------------------------------------------------------------------
// The following macros declare the pins to use for HW and SW SPI communication.
// SCK, MISO and MOSI should be connected to the HW SPI pins on the Uno when
// using HW SPI.  This should be used with nRF51822 based Bluefruit LE modules
// that use SPI (Bluefruit LE SPI Friend).
// ------------------------------------------------------------------------------

#define BLUEFRUIT_SPI_CS               8
#define BLUEFRUIT_SPI_IRQ              7
#define BLUEFRUIT_SPI_RST              4    // Optional but recommended, set to -1 if unused
#define VERBOSE_MODE                   false  // If set to 'true' enables debug output

// BLE module stuff
#define FACTORYRESET_ENABLE         0
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define MODE_LED_BEHAVIOUR          "MODE"


// Exposure time step in milliseconds
#define EXPOSURE_UNIT 2.8

// steps in a single button up/down click
#define EXPOSURE_STEPS 20 

// maximun value expected fro the AS7262 chip
#define SENSOR_MAX 5000

// Short delay in screens (milliseconds)
#define SHORT_DELAY 200

// strings to display on TFT and send to BLE
// It is not worth to place these strings in Flash
const char* GainTable[] = {
    "1",
    "3.7",
    "16",
    "64"
  };

/* ************************************************************************** */ 
/*                        CUSTOM CLASES & DATA TYPES                          */
/* ************************************************************************** */ 

typedef struct {
  float    calibratedValues[AS726x_NUM_CHANNELS];
  uint16_t rawValues[AS726x_NUM_CHANNELS];
  uint8_t  gain;           // device gain multiplier
  uint8_t  exposure;       // device integration time in steps of 2.8 ms
  uint8_t  temperature;    // device internal temperature
} as7262_info_t;


/* ************************************************************************** */ 
/*                          GLOBAL VARIABLES SECTION                          */
/* ************************************************************************** */ 


//create the 6 channel spectral sensor object
Adafruit_AS726x ams;

// buffer to hold raw & calibrated values as well as exposure time and gain
as7262_info_t as7262_info;

/* Hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, 
                            BLUEFRUIT_SPI_IRQ, 
                            BLUEFRUIT_SPI_RST);

 unsigned long seq = 0; // Tx sequence number

/* ************************************************************************** */ 
/*                            HELPER FUNCTIONS                                */
/* ************************************************************************** */ 


// A small helper
static void error(const __FlashStringHelper* err) 
{
  Serial.println(err);
  //delay(5000);
  //sleep_enable();
  //set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  //sleep_cpu();
  while(1) ;
}

/* ************************************************************************** */ 



/* ************************************************************************** */ 

static uint8_t read_as7262_sensor()
{
  extern Adafruit_AS726x ams;
  extern as7262_info_t   as7262_info;

  uint8_t dataReady = ams.dataReady();
  if(dataReady) {
    ams.readCalibratedValues(as7262_info.calibratedValues);
    ams.readRawValues(as7262_info.rawValues);
    as7262_info.temperature = ams.readTemperature();
  }
  return dataReady;
}

/* ************************************************************************** */ 

static void format_message(String& line)
{
  extern as7262_info_t as7262_info;
  extern const char*   GainTable[];
  extern unsigned long seq;
 
   // Start JSON sequence
  line += String("['A',");
  // Sequence number
  line += String(seq++);  line += String(',');
  // Relative timestamp
  line += String(millis()); line += String(',');
  // AS7262 Exposure time in milliseconds
  line += String(as7262_info.exposure*EXPOSURE_UNIT,1); line += String(',');
  // AS7262 Gain
  line += String(GainTable[as7262_info.gain]); line += String(',');
  // AS7262 Temperature
  line += String(as7262_info.temperature); line += String(',');
  // AS7262 calibrated values
  for (int i=0; i< 5; i++) {
      line += String(as7262_info.calibratedValues[i], 4); 
      line += String(',');
  }
  line += String(as7262_info.calibratedValues[5], 4); 
  // End JSON sequence
  line += String("]\n"); 
}


/* ************************************************************************** */ 
/*                              SETUP FUNCTIONS                              */
/* ************************************************************************** */ 

static void setup_ble()
{
  extern Adafruit_BluefruitLE_SPI ble;

  Serial.print(F("Bluefruit SPI... "));
  
  if ( !ble.begin(VERBOSE_MODE) ) {
    error(F("Couldn't find Bluefruit!"));
  }

  if ( FACTORYRESET_ENABLE && ! ble.factoryReset() ) {
    /* Perform a factory reset to make sure everything is in a known state */
    error(F("Couldn't factory reset Bluefruit!"));
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);
  // LED Activity command is only supported from Fiormware version 0.6.6
  ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);

  // Set module to DATA mode
  ble.setMode(BLUEFRUIT_MODE_DATA);
  Serial.println(F("ok"));
  //ble.info();
  //ble.verbose(false);  // debug info is a little annoying after this point!
}

/* ************************************************************************** */ 

static void setup_as7262()
{
  extern as7262_info_t as7262_info;
  extern Adafruit_AS726x ams;
 
  Serial.print(F("AS7262... "));
  // finds the 6 channel chip
  if(!ams.begin()){
    error(F("could not connect to AS7262!"));
  }
  // as initialized by the AS7262 library
  // Note that in MODE 2, the exposure time is actually doubled
  as7262_info.gain     = GAIN_64X;
  as7262_info.exposure = 50;
  // continuous conversion time is already done by default in the ams driver
  //ams.setConversionType(MODE_2);
  Serial.println(F("ok"));
}


/* ************************************************************************** */ 
/*                                MAIN SECTION                               */
/* ************************************************************************** */ 


void setup() 
{
  Serial.begin(115200);
  while(!Serial);
  Serial.println(F("Sketch version: " GIT_VERSION));
  setup_ble();
  setup_as7262();
}


void loop() 
{
  extern Adafruit_BluefruitLE_SPI ble;

  if (read_as7262_sensor()) {
     String line;
     format_message(line);
    if (ble.isConnected()) {
      ble.print(line.c_str());    // send to BLE
    }
    Serial.print(line);
  }
}
