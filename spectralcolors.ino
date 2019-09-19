
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


              Arduino Nano        OPT3001 Sensor
              ============        ===============

                    /
                   |  A4 (SDA)  -----> SDA 
              I2C  |  A5 (SCL)  -----> SCL 
                   \
    
                   /
                   |   3.3V    <====> VDD 
              Pwr  |   GND     <====> GND
                   \

*/


/* ************************************************************************** */ 
/*                           INCLUDE HEADERS SECTION                          */
/* ************************************************************************** */ 

// Support for the Git version tags
#include "git-version.h"

// Adafruit Bluetooth libraries
#include <Adafruit_BLE.h>
#include <Adafruit_BluefruitLE_SPI.h>

// ClosedCube OPT3001 library
#include <ClosedCube_OPT3001.h>

/* ************************************************************************** */ 
/*                                DEFINEs SECTION                             */
/* ************************************************************************** */ 

#ifndef GIT_VERSION
#define GIT_VERSION "0.1.0" // For downloads without git
#endif

// OPT 3001 Address (0x45 by default)
#define OPT3001_ADDRESS 0x45

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



/* ************************************************************************** */ 
/*                          GLOBAL VARIABLES SECTION                          */
/* ************************************************************************** */ 

/* Hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, 
                            BLUEFRUIT_SPI_IRQ, 
                            BLUEFRUIT_SPI_RST);

// I2C OPT3001 sensor object
ClosedCube_OPT3001 opt3001;

// OPT3001 read sensor data
OPT3001 opt3001_info;

// Tx sequence number
unsigned long seq = 0; 

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

static uint8_t read_opt3001_sensor()
{
  extern ClosedCube_OPT3001 opt3001;
  extern OPT3001 opt3001_info;

  OPT3001_Config sensorConfig = opt3001.readConfig();
  uint8_t dataReady = (sensorConfig.ConversionReady != 0);

  if (dataReady) {
    opt3001_info = opt3001.readResult();
  }
  return dataReady;
}


/* ************************************************************************** */ 

static void format_message(String& line)
{
  extern unsigned long   seq;
  extern OPT3001 opt3001_info;

  // Start JSON sequence
  line += String("['O',");
  // Sequence number
  line += String(seq++);  line += String(',');
  // Relative timestamp
  line += String(millis()); line += String(',');
  // OPT 3001 lux readings
  line += String(opt3001_info.lux, 2);
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

static void setup_opt3001()
{
  extern ClosedCube_OPT3001 opt3001;

  Serial.print(F("OPT3001... "));
  opt3001.begin(OPT3001_ADDRESS);

  OPT3001_Config config;
  
  config.RangeNumber               = B1100;  // Automatic full-scale
  config.ConvertionTime            = B1;     // 800 ms
  config.Latch                     = B1;     // ???
  config.ModeOfConversionOperation = B11;    // Continuou operation

  OPT3001_ErrorCode errorConfig = opt3001.writeConfig(config);
  if (errorConfig != NO_ERROR) {
    error(F("OPT3001 config error!"));
  }
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
  setup_opt3001();
}



void loop() 
{
  extern Adafruit_BluefruitLE_SPI ble;

  if (read_opt3001_sensor()) {
     String line;
     format_message(line);
    if (ble.isConnected()) {
      ble.print(line.c_str());    // send to BLE
    }
    Serial.print(line);
  }
}
