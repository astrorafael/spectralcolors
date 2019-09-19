
/* ************************************************************************** */ 
/*                           ORIGINAL COPYRIGHT TEXT                          */ 
/* ************************************************************************** */ 

/*
  This sketch reads sends a welcome string through the Arduino serial port an Bluetooth LE module.
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
                   |   3.3V    <====> VIN
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


/* ************************************************************************** */ 
/*                                DEFINEs SECTION                             */
/* ************************************************************************** */ 

#ifndef GIT_VERSION
#define GIT_VERSION "0.1.0"
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

// ---------------------------------------------------------
// Define which Arduino nano pins will control the TFT Reset, 
// SPI Chip Select (CS) and SPI Data/Command DC
// ----------------------------------------------------------

// BLE module stuff
#define FACTORYRESET_ENABLE         0
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define MODE_LED_BEHAVIOUR          "MODE"


/* ************************************************************************** */ 
/*                          GLOBAL VARIABLES SECTION                          */
/* ************************************************************************** */ 


/* Hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, 
                            BLUEFRUIT_SPI_IRQ, 
                            BLUEFRUIT_SPI_RST);


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

static void send_bluetooth()
{
  extern Adafruit_BluefruitLE_SPI ble;

  String line("Hello world\n");
  ble.print(line.c_str());  // send to BLE
}


/* ------------------------------------------------------------------------- */ 


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
  ble.info();
  //ble.verbose(false);  // debug info is a little annoying after this point!
}

/* ************************************************************************** */ 

/* ************************************************************************** */ 


/* ************************************************************************** */ 
/*                                MAIN SECTION                               */
/* ************************************************************************** */ 


void setup() 
{
  Serial.begin(115200);
  while(!Serial);
  Serial.println(F("Sketch version: " GIT_VERSION));
  setup_ble();
}


void loop() 
{
  if (ble.isConnected()) {
      send_bluetooth();
  }
  delay(1000);
  Serial.println("Hello world");
}
