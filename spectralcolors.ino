
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
                   |   N/C            3.3V (from 3.3V bus)
              Pwr  |   GND     <====> GND
                   \


              Arduino Nano        AS7262 Spectral Sensor
              ============        ======================

                    /
                   |  A4 (SDA)  -----> SDA 
              I2C  |  A5 (SCL)  -----> SCL 
                   \

                   /
                   |   5V       ====> VIN 
              Pwr  |   GND     <====> GND
                   |   N/C            3.3V Bus (provides 3.3V)
                   \

              Arduino Nano        OPT3001 Sensor
              ============        ===============

                    /
                   |  A4 (SDA)  -----> SDA 
              I2C  |  A5 (SCL)  -----> SCL 
                   \

                   /
                   |   N/C           VDD (from 3.3V bus)
              Pwr  |   GND    <====> GND
                   \

              Arduino Nano        miniTFTWing
              ============        ===========
                   /
                   | D11 (MOSI) -----> MOSI
              SPI <  D13 (SCK)  -----> SCLK
                   |     D5     -----> CS
                   |     D6     -----> DC
                   \

                   /
                   |  A4 (SDA)  -----> SDA
              I2C  |  A5 (SCL)  -----> SCL
                   \

                   /
                   |   N/C            3.3V (from 3.3V bus)
              Pwr  |   GND     <====> GND
                   \

The built-in LED is attached to pin Arduino Nano D13. 
However, this pin is used to interface miniTCFWing via SPI
So, the built-in LED becomes unusable after miniTFTWing initialization

*/


/* ************************************************************************** */ 
/*                           INCLUDE HEADERS SECTION                          */
/* ************************************************************************** */ 

// Support for the Git version tags
#include "git-version.h"

// Adafruit Spectral Sensor library
#include <Adafruit_AS726x.h>

// Adafruit Graphics libraries
#include <Adafruit_GFX.h>    		   // Core graphics library
#include <Adafruit_ST7735.h> 		   // Hardware-specific library
#include <Adafruit_miniTFTWing.h>  // Seesaw library for the miniTFT Wing display

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

// more BLE module stuff
#define FACTORYRESET_ENABLE         0
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define MODE_LED_BEHAVIOUR          "MODE"

// ---------------------------------------------------------
// Define which Arduino nano pins will control the TFT Reset, 
// SPI Chip Select (CS) and SPI Data/Command DC
// ----------------------------------------------------------

#define TFT_RST -1  // miniTFTwing uses the seesaw chip for resetting to save a pin
#define TFT_CS   5 // Arduino Nano D5 pin
#define TFT_DC   6 // Arduini Nano D6 pin

// -----------------------------------------
// Some predefined colors for the 16 bit TFT
// -----------------------------------------

// 5-6-5 R-G-B color scheme
#define MAKE_RGB(r,g,b) (((r)<<11) | ((g)<<5) | (b))

#define GRAY2 MAKE_RGB(B11000,B110000,B11000)

#define BLACK   0x0000
#define GRAY    0x8410
#define WHITE   0xFFFF
#define RED     0xF800
#define ORANGE  0xFA60 // o 0xFC00
#define YELLOW  0xFFE0  
#define LIME    0x07FF
#define GREEN   0x07E0
#define CYAN    0x07FF
#define AQUA    0x04FF
#define BLUE    0x001F
#define MAGENTA 0xF81F
#define PINK    0xF8FF

// ---------------------
// OPT3001 values
// ---------------------

#ifndef OPT3001_OFFSET
#define OPT3001_OFFSET 0.0
#endif

// ---------------------
// AS7262 default values
// ---------------------

// Exposure time step in milliseconds
#define EXPOSURE_UNIT 2.8

// maximun value expected fro the AS7262 chip
#define AS7262_SENSOR_MAX 5000

// ---------------
// Other constants
// ---------------

// Maximun number of readings to accumulate
#define MAX_ACCUM  16

// exposure steps in a single button up/down click
#define EXPOSURE_STEPS 20 

// backlight steps in a single button up/down click
#define BACKLIGHT_STEPS 25 

// Short delay in screens (milliseconds)
#define SHORT_DELAY 200

// Enable Tx readings through Serial port
#define CMD_ENABLE_SERIAL  'x'
#define CMD_DISABLE_SERIAL 'z'
#define CMD_HOLD_TOGGLE    'h'

// strings to display on TFT and send to BLE
// It is not worth to place these strings in Flash
static const char* GainTable[] = {
    "1",
    "3.7",
    "16",
    "64"
  };

/* ************************************************************************** */ 
/*                        CUSTOM CLASES & DATA TYPES                          */
/* ************************************************************************** */ 
typedef struct {
    // float     calibrated[AS726x_NUM_CHANNELS];
    uint16_t  raw[AS726x_NUM_CHANNELS];
} as7262_readings_t;

typedef struct {
  as7262_readings_t accumulated;    // Accumulated values from several readings
  as7262_readings_t latched;        // Latched values once it has been accumulated
  unsigned long     tstamp;         // timestamp as given by millis() function
  uint8_t           gain;           // device gain multiplier
  uint8_t           exposure;       // device integration time in steps of 2.8 ms
  uint8_t           temperature;    // device internal temperature
  uint8_t           accCount;       // Current accumulation count
  uint8_t           accLimit;       // Current accumulation limit
} as7262_info_t;

typedef struct {
  uint8_t backlight;    // miniTFTWing backlight value in percentage
} tft_info_t;

// Menu action function pointer as a typedef
typedef void (*menu_action_t)(void);

/* ************************************************************************** */ 
/*                          GLOBAL VARIABLES SECTION                          */
/* ************************************************************************** */ 

// The Adafruit SeeSaw chip that controls the TFT by I2C
Adafruit_miniTFTWing ss;

// The Adafruit TFT display object based on ST7735
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

//create the 6 channel spectral sensor object
Adafruit_AS726x ams;

// buffer to hold raw & calibrated values as well as exposure time and gain
as7262_info_t as7262_info;

// buffer to hold raw & calibrated values as well as exposure time and gain
tft_info_t tft_info;

/* Hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, 
                            BLUEFRUIT_SPI_IRQ, 
                            BLUEFRUIT_SPI_RST);

// I2C OPT3001 sensor object
ClosedCube_OPT3001 opt3001;

// OPT3001 read sensor data
OPT3001        opt3001_info;
OPT3001_Config config;
unsigned long  opt3001_tstamp;         // timestamp as given by millis() function

// enable readings to serial port 
bool toSerial = false;

// enable/disable hold mode
bool holdMode = false;

// display absolute or auto scale bar graphs
bool autoScaleMode = false;

/* ************************************************************************** */ 
/*                      GUI STATE MACHINE DECLARATIONS                        */
/* ************************************************************************** */ 

// Events generated by user
enum gen_events {
  GUI_NO_EVENT      = 0,
  GUI_KEY_A_PRESSED,
  GUI_KEY_B_PRESSED,
  GUI_JOY_PRESSED,
  GUI_JOY_UP,
  GUI_JOY_DOWN,
  GUI_JOY_LEFT,
  GUI_JOY_RIGHT,
  REM_HOLD_TOGGLE
};

// TFT Screens as states
enum gui_state {
  GUI_GAIN_SCR = 0,
  GUI_EXPOS_SCR,
  GUI_BARS_SCR,
  GUI_DATA_SCR,
  GUI_LUX_SCR,
  GUI_ACCUM_SCR,
  GUI_MAX_STATES
};

uint8_t  screen; // The current screen

// array of predefined bar colors
// This table is held in Flash memory to save precious RAM
// Use of PROGMEM and pgm_xxx() functions is necessary
  static const PROGMEM uint16_t colors[AS726x_NUM_CHANNELS] = {
      MAGENTA,
      BLUE,
      GREEN,
      YELLOW,
      ORANGE,
      RED
  };

// array of predefined color labels
// This table is held in RAM
static const char labelTable[] = {
    'V',
    'B',
    'G',
    'Y',
    'O',
    'R'
  };



// --------------------------------------------
// State Machine Actions (Forward declarations)
// --------------------------------------------

static void act_idle();       // The Idle activity
static void act_gain_in();    // Entering the gain screen
static void act_gain_up();    // Raise the gain action
static void act_gain_down();  // Lower the gain action
static void act_light_up();   // Brightening the backlight
static void act_light_down(); // Dimming the backlight
static void act_expos_in();   // Entering the AS7262 exposure time screen
static void act_expos_up();   // Increase the AS7262 exposure time
static void act_expos_down(); // Decrease the AS7262 exposure time
static void act_bars_in();   // Display the spectrum bars
static void act_bars_idle();  // Idle activity when in the spectrum bars screen
static void act_bars_hold();  // Hold action when in the spectrum bars screen
static void act_data_in();    // Display the spectrum as data lines
static void act_data_idle();  // Idle activity when in the data lines screen
static void act_lux_in();     // Entering the luxometer screen
static void act_lux_idle();   // Idle activity when in the luxometer screen
static void act_lux_hold();   // Hold action when in the luxometer screen
static void act_accum_in();   // Entering the accumulate readings screen
static void act_accum_up();   // Increasde the accumulation of readings
static void act_accum_down(); // Decrease the accumulation ofn readings
static void act_hold();       // Toggle Hold Mode
static void act_autoscale();  // Toggle AutoScale Mode

// Action to execute as a function of current state and event
// This table is held in Flash memory to save precious RAM
// Use of PROGMEM and pgm_xxx() functions is necessary
static menu_action_t get_action(uint8_t state, uint8_t event)
{
  static const menu_action_t menu_action[][GUI_MAX_STATES] PROGMEM = {
    //----------------+---------------+----------------+--------------+---------------+---------------+
    //  GAIN SCREEN   | EXPOSURE SCR  |    BARS SCR    |   DATA_SCR   |  LUX SCREEN   | ACCUM SCREEN  |
    //----------------+---------------+----------------+--------------+---------------+---------------+
    { act_idle,       act_idle,        act_bars_idle,   act_data_idle,  act_lux_idle,   act_idle       }, // GUI_NO_EVENT
    { act_light_up,   act_light_up,    act_light_up,    act_light_up,   act_light_up,   act_light_up   }, // GUI_KEY_A_PRESSED
    { act_light_down, act_light_down,  act_light_down,  act_light_down, act_light_down, act_light_down }, // GUI_KEY_B_PRESSED
    { act_bars_in,    act_bars_in,     act_bars_hold,   act_hold,       act_lux_hold,   act_bars_in    }, // GUI_JOY_PRESSED
    { act_gain_up,    act_expos_up,    act_data_in,     act_data_idle,  act_idle,       act_accum_up   }, // GUI_JOY_UP
    { act_gain_down,  act_expos_down,  act_autoscale,   act_bars_in,    act_idle,       act_accum_down }, // GUI_JOY_DOWN
    { act_accum_in,   act_gain_in,     act_expos_in,    act_expos_in,   act_bars_in,    act_lux_in     }, // GUI_JOY_LEFT
    { act_expos_in,   act_bars_in,     act_lux_in,      act_lux_in,     act_accum_in,   act_gain_in    }, // GUI_JOY_RIGHT
    { act_hold,       act_hold,        act_bars_hold,   act_hold,       act_lux_hold,   act_hold       }  // REM_HOLD_TOGGLE
  };
  return (menu_action_t) pgm_read_ptr(&menu_action[event][state]);
}

/* -------------------------------------------------------------------------- */ 

// Next state to proceed as a function of current state and event
// This table is held in Flash memory to save precious RAM
// Use of PROGMEM and pgm_xxx() functions is necessary
static uint8_t get_next_screen(uint8_t state, uint8_t event)
{
  static const PROGMEM uint8_t next_screen[][GUI_MAX_STATES] = {
    //----------------+---------------+----------------+--------------+---------------+---------------+
    //  GAIN SCREEN   | EXPOSURE SCR  |    BARS SCR    |   DATA_SCR   |  LUX SCREEN   | ACCUM SCREEN  |
    //----------------+---------------+----------------+--------------+---------------+---------------+
      { GUI_GAIN_SCR,   GUI_EXPOS_SCR,   GUI_BARS_SCR,  GUI_DATA_SCR,  GUI_LUX_SCR,    GUI_ACCUM_SCR }, // GUI_NO_EVENT
      { GUI_GAIN_SCR,   GUI_EXPOS_SCR,   GUI_BARS_SCR,  GUI_DATA_SCR,  GUI_LUX_SCR,    GUI_ACCUM_SCR }, // GUI_KEY_A_PRESSED
      { GUI_GAIN_SCR,   GUI_EXPOS_SCR,   GUI_BARS_SCR,  GUI_DATA_SCR,  GUI_LUX_SCR,    GUI_ACCUM_SCR }, // GUI_KEY_B_PRESSED
      { GUI_BARS_SCR,   GUI_BARS_SCR,    GUI_BARS_SCR,  GUI_DATA_SCR,  GUI_LUX_SCR,    GUI_BARS_SCR  }, // GUI_JOY_PRESSED
      { GUI_GAIN_SCR,   GUI_EXPOS_SCR,   GUI_DATA_SCR,  GUI_BARS_SCR,  GUI_LUX_SCR,    GUI_ACCUM_SCR }, // GUI_JOY_UP
      { GUI_GAIN_SCR,   GUI_EXPOS_SCR,   GUI_BARS_SCR,  GUI_BARS_SCR,  GUI_LUX_SCR,    GUI_ACCUM_SCR }, // GUI_JOY_DOWN
      { GUI_ACCUM_SCR,  GUI_GAIN_SCR,    GUI_EXPOS_SCR, GUI_EXPOS_SCR, GUI_BARS_SCR,   GUI_LUX_SCR   }, // GUI_JOY_LEFT
      { GUI_EXPOS_SCR,  GUI_BARS_SCR,    GUI_LUX_SCR,   GUI_LUX_SCR,   GUI_ACCUM_SCR,  GUI_GAIN_SCR  }, // GUI_JOY_RIGHT
      { GUI_EXPOS_SCR,  GUI_BARS_SCR,    GUI_BARS_SCR,  GUI_DATA_SCR,  GUI_LUX_SCR,    GUI_ACCUM_SCR }  // REM_HOLD_TOGGLE
    };
  return pgm_read_byte(&next_screen[event][state]);
}

/* ************************************************************************** */ 
/*                            HELPER FUNCTIONS                                */
/* ************************************************************************** */ 

/* ************************************************************************** */ 
/*                            HELPER FUNCTIONS                                */
/* ************************************************************************** */ 

// A small helper

#if 1
// This vesion uses RAM Strings
static void error(const char* err) 
{
  Serial.println(err);
  //delay(5000);
  //sleep_enable();
  //set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  //sleep_cpu();
  while(1) ;
}
#else
// This version is meant to be uded with the F() macro (embedded Flash strings")
static void error(const  __FlashStringHelper* err) 
{
  Serial.println(err);
  //delay(5000);
  //sleep_enable();
  //set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  //sleep_cpu();
  while(1) ;
}
#endif

/* ************************************************************************** */ 

static void dbg_fsm(uint8_t state, uint8_t event)
{
  if (event != GUI_NO_EVENT) {
    Serial.print(F("State: "));  Serial.print(state); Serial.print(F(" Event: ")); Serial.println(event);
  }
}

/* ************************************************************************** */ 

static void dbg_heartbeat()
{
  extern bool toSerial;
  static int divider = 0;

  divider += 1;
  divider &= 1023; // (2^N - 1)
  if (divider == 0 && toSerial == false) {
    Serial.println('*');
  }
}

/* ************************************************************************** */ 

static uint8_t read_stream(Stream& stream)
{

 extern bool toSerial;
 uint8_t event = GUI_NO_EVENT;

  // Read Serial Port commands
  if (stream.available() > 0) {
     unsigned char cmd = stream.read();
    if (cmd == CMD_ENABLE_SERIAL)
      toSerial = true;
    else if (cmd == CMD_DISABLE_SERIAL)
      toSerial = false;
    else if (cmd == CMD_HOLD_TOGGLE)
      event = REM_HOLD_TOGGLE;
  }
    
  return event;
}


/* ************************************************************************** */ 

// Reads miniTFTWing buttons & joystick and produces events
static uint8_t read_buttons()
{
  extern Adafruit_miniTFTWing ss;
 
  static unsigned long prev_timestamp = 0;
  uint8_t event = GUI_NO_EVENT;

  if ((millis() - prev_timestamp) < SHORT_DELAY) {
    return event;
  }
  prev_timestamp = millis();

  // miniTFT wing buttons;
  // read buttons via the I2C SeeSaw chip in miniTFTWing
  // These buttons are active-low logic
  uint32_t buttons = ss.readButtons();


  if ((buttons & TFTWING_BUTTON_A) == 0) {
       //Serial.println("A pressed");
       event = GUI_KEY_A_PRESSED;
  } else if ((buttons & TFTWING_BUTTON_B) == 0) {
       //Serial.println("B pressed");
       event = GUI_KEY_B_PRESSED;
  } else if ((buttons & TFTWING_BUTTON_UP) == 0) {
       //Serial.println("Joy up");
       event = GUI_JOY_UP;
  } else if ((buttons & TFTWING_BUTTON_DOWN) == 0) {
       //Serial.println("Joy down");
       event = GUI_JOY_DOWN;
  } else if ((buttons & TFTWING_BUTTON_LEFT) == 0) {
       //Serial.println("Joy left");
       event = GUI_JOY_LEFT;
  } else if ((buttons & TFTWING_BUTTON_RIGHT) == 0) {
       //Serial.println("Joy right");
       event = GUI_JOY_RIGHT;
  } else if ((buttons & TFTWING_BUTTON_SELECT) == 0) {
       //Serial.println("Joy select");
       event = GUI_JOY_PRESSED;
  }
  return event;
}

/* ************************************************************************** */ 

static uint8_t opt3001_read()
{
  extern ClosedCube_OPT3001 opt3001;
  extern OPT3001 opt3001_info;

  OPT3001_Config sensorConfig = opt3001.readConfig();
  uint8_t dataReady = (sensorConfig.ConversionReady != 0);

  if (dataReady) {
    opt3001_info = opt3001.readResult();
    opt3001_tstamp = millis();
  }
  return dataReady;
}

static uint16_t peak()
{
   extern as7262_info_t as7262_info;
   uint16_t target = 0; // reset accum counter
   for (int i=0; i<AS726x_NUM_CHANNELS; i++) {
     target = max(target, as7262_info.latched.raw[i]);
   }
   return target;
}

/* ************************************************************************** */ 

static void as7262_latch()
{
  extern as7262_info_t as7262_info;
  
  as7262_info.latched = as7262_info.accumulated;
}

/* ************************************************************************** */ 

static void as7262_clear_accum()
{
  extern as7262_info_t as7262_info;

  as7262_info.accCount = 0; // reset accum counter
  for (int i=0; i<AS726x_NUM_CHANNELS; i++) {
    as7262_info.accumulated.raw[i]        = 0;
    // as7262_info.accumulated.calibrated[i] = 0.0;
  }
}

/* ************************************************************************** */ 

static bool as7262_accumulate(as7262_readings_t* src)
{
  extern as7262_info_t as7262_info;
  
  // Accumulate readinggs
  for (int i=0; i<AS726x_NUM_CHANNELS; i++) {
    as7262_info.accumulated.raw[i]        += src->raw[i];
    // as7262_info.accumulated.calibrated[i] += src->calibrated[i];
  }
  // Update accumulation counter, with wrapparound
  as7262_info.accCount += 1;
  as7262_info.accCount &= as7262_info.accLimit-1;
  return (as7262_info.accCount == 0);

}

/* ************************************************************************** */ 

static uint8_t as7262_read()
{
  extern Adafruit_AS726x ams;
  extern as7262_info_t as7262_info;

  as7262_readings_t current;
  bool done;

  uint8_t dataReady = ams.dataReady();
  if(dataReady) {
    // ams.readCalibratedValues(current.calibrated);
    ams.readRawValues(current.raw);
    as7262_info.temperature = ams.readTemperature();
    done = as7262_accumulate(&current); 
    if (done) {
      as7262_info.tstamp = millis();
      as7262_latch(); // latch accumulated value for display and Tx
      as7262_clear_accum();  // clears readings accumulator
    } else {
      dataReady = 0;  // still accumulating readings ....
    }
  }
  return dataReady;
}


/* ************************************************************************** */ 

static void display_bars(bool refresh)
{
  extern as7262_info_t   as7262_info;
  extern Adafruit_ST7735 tft;
  extern bool holdMode;

  uint16_t peakValue = (autoScaleMode) ? peak() : AS7262_SENSOR_MAX;
 
  // Display bar buffers, used to minimize redrawings
  static uint16_t height[AS726x_NUM_CHANNELS][2];
  static uint8_t  curBuf = 0;                     // current buffer 

  // see if we really have to redraw the bars
  for(int i=0; i<AS726x_NUM_CHANNELS; i++) {
    height[i][curBuf] = map(as7262_info.latched.raw[i], 0, peakValue, 0, tft.height());
    if (height[i][curBuf] != height[i][curBuf  ^ 0x01]) {
      refresh = true;
    }
  }

  if (refresh) { 

    uint16_t gridWidth = tft.width() / AS726x_NUM_CHANNELS;
    uint16_t barWidth  = (holdMode) ?  gridWidth/2 : gridWidth ;
    
    for(int i=0; i<AS726x_NUM_CHANNELS; i++) {
      uint16_t color  = pgm_read_word(&colors[i]);  
      tft.fillRect(gridWidth * i, 0, gridWidth, tft.height() - height[i][curBuf], BLACK);
      tft.fillRect(gridWidth * i, tft.height() - height[i][curBuf], barWidth, height[i][curBuf], color);
    }
  }
  curBuf ^= 0x01; // switch to the other buffer
}

/* ************************************************************************** */ 

static void display_data()
{
  extern tft_info_t tft_info;
  extern bool holdMode;
  uint16_t color = (holdMode) ?  WHITE : GRAY2;

  tft.setTextSize(2); // 2x the original font
  for(int i=0; i<AS726x_NUM_CHANNELS; i++) {
     uint16_t h0;
     uint16_t w0;
     uint16_t txtcolor  = pgm_read_word(&colors[i]);  
    if ((i % 2) == 0) {
      w0 = 0;
      h0 = (tft.height()*i)/(AS726x_NUM_CHANNELS);
    } else {
      w0 = tft.width()/2;
    }
    tft.setCursor(w0, h0); 
    tft.setTextColor(txtcolor, BLACK); 
    tft.print(labelTable[i]);
    tft.setCursor(w0+12, h0);
    tft.setTextColor(color, BLACK);
    tft.print(as7262_info.latched.raw[i]); 
  }
}

/* ************************************************************************** */ 

static void display_lux(bool refresh)
{
  extern OPT3001    opt3001_info;
  extern bool holdMode;
  static float prev_lux = 0;
  uint16_t color = (holdMode) ?  WHITE : YELLOW;

  if (opt3001_info.lux != prev_lux) {
    prev_lux = opt3001_info.lux;
    refresh = true;
  }

  if (refresh) {
    // refresh display value
    tft.setCursor(0, tft.width()/3);
    tft.setTextColor(color, BLACK);
    tft.print(opt3001_info.lux+OPT3001_OFFSET,2); 
  }
  
}

/* ************************************************************************** */ 


static void display_gain()
{
  extern as7262_info_t as7262_info;
 
  tft.fillScreen(BLACK);
  // Display the "Gain" sttring in TFT
  tft.setTextSize(3); // 3x the original font
  tft.setCursor(tft.height()/3, 0);
  tft.setTextColor(YELLOW, BLACK);
  tft.print("Gain");
  // Display the gain value string in TFT
  tft.setCursor(tft.height()/3, tft.width()/3);
  tft.setTextColor(WHITE, BLACK);
  tft.print(GainTable[as7262_info.gain]);
  tft.print('x');
}


/* ************************************************************************** */ 

static void display_exposure()
{
  extern as7262_info_t as7262_info;

  tft.fillScreen(BLACK);
  // Display the "Gain" sttring in TFT
  tft.setTextSize(3);
  tft.setCursor(0,0);
  tft.setTextColor(YELLOW, BLACK);
  tft.print("Exposure");
  // Display the exposure value string in TFT
  tft.setCursor(0, tft.width()/3);
  tft.setTextColor(WHITE, BLACK);
  tft.print(as7262_info.exposure*EXPOSURE_UNIT*2,1); tft.print(" ms");
}



/* ************************************************************************** */ 

static void display_accum()
{
  extern as7262_info_t as7262_info;
 
  tft.fillScreen(BLACK);
  // Display the "Accumula" sttring in TFT
  tft.setTextSize(3); // 3x the original font
  tft.setCursor(tft.height()/3, 0);
  tft.setTextColor(YELLOW, BLACK);
  tft.print("Accum");
  // Display the accumulator value string in TFT
  tft.setCursor(tft.height()/3, tft.width()/3);
  tft.setTextColor(WHITE, BLACK);
  tft.print(as7262_info.accLimit); tft.print("x");
}


/* ************************************************************************** */ 

static void format_opt3001_msg(String& line)
{
  extern OPT3001       opt3001_info;
  static unsigned long seqOPT = 0;
 
  // Start JSON sequence
  line += String("[\"O\",");
  // Sequence number
  line += String(seqOPT++); line += String(',');
  // Relative timestamp
  line += String(opt3001_tstamp); line += String(',');
  // Accumulated readings (fixed to 1)
  line += String(1); line += String(',');
  // Exposure time in milliseconds (fixed to 800)
  line += String("800"); line += String(',');
  // OPT 3001 lux readings
  line += String(opt3001_info.lux+OPT3001_OFFSET, 2);
  // End JSON sequence
  line += String("]\r\n"); 
}

/* ************************************************************************** */ 
 
static void format_as7262_msg(String& line)
{
  extern as7262_info_t as7262_info;
  extern const char*   GainTable[];
  static unsigned long seqAS = 0; // Tx sequence number
 
   // Start JSON sequence
  line += String("[\"A\",");
  // Sequence number
  line += String(seqAS++);  line += String(',');
  // Relative timestamp
  line += String(as7262_info.tstamp); line += String(',');
  // Accumulated readings (fixed to 1)
  line += String(as7262_info.accLimit); line += String(',');
  // AS7262 Exposure time in milliseconds
  line += String(as7262_info.exposure*EXPOSURE_UNIT*2,1); line += String(',');
  // AS7262 Gain
  line += String(GainTable[as7262_info.gain]); line += String(',');
  // AS7262 Temperature
  line += String(as7262_info.temperature); line += String(',');
  // AS7262 calibrated values
  for (int i=0; i< 5; i++) {
      // line += String(as7262_info.latched.calibrated[i], 2); line += String(','); 
      line += String(as7262_info.latched.raw[i]); line += String(',');
  }
  // line += String(as7262_info.latched.calibrated[5], 2); line += String(','); 
  line += String(as7262_info.latched.raw[5]);
  // End JSON sequence
  line += String("]\r\n"); 
}

/* ************************************************************************** */ 
/*                      STATE MACHINE ACTION FUNCTIONS                        */
/* ************************************************************************** */ 

static void act_idle()
{
  extern Adafruit_BluefruitLE_SPI ble;
  extern bool toSerial;

  dbg_heartbeat();

  if( ! holdMode ) {
    if (as7262_read()) {
      String line;
      format_as7262_msg(line);
      if (ble.isConnected()) 
        ble.print(line.c_str());  // send to BLE
      if(toSerial) 
        Serial.print(line);
    }

    if (opt3001_read()) {
      String line;
      format_opt3001_msg(line);
      if (ble.isConnected())
        ble.print(line.c_str());  // send to BLE
      if(toSerial) 
        Serial.print(line);
    }
  }
}

/* ------------------------------------------------------------------------- */ 

static void act_hold()
{ 
  extern bool holdMode;
  holdMode ^= 0x01;   // toggles Hold Mode
}

/* ------------------------------------------------------------------------- */ 

static void act_autoscale()
{ 
  extern bool autoScaleMode;
  autoScaleMode ^= 0x01;   // toggles Hold Mode
}

/* ------------------------------------------------------------------------- */ 

static void act_expos_in()
{ 
  display_exposure();
}

/* ------------------------------------------------------------------------- */ 


static void do_exposure(int16_t exposure)
{
  extern as7262_info_t   as7262_info;
  extern Adafruit_AS726x ams;

  as7262_info.exposure = constrain(exposure, 1, 255);
  ams.setIntegrationTime(as7262_info.exposure); 
  display_exposure();
}
/* ------------------------------------------------------------------------- */ 

static void act_expos_up()
{
  extern as7262_info_t   as7262_info;
  
  do_exposure(as7262_info.exposure + EXPOSURE_STEPS); // save a few bytes by using this function
}

/* ------------------------------------------------------------------------- */ 

static void act_expos_down()
{
  extern as7262_info_t   as7262_info;
  
  do_exposure(as7262_info.exposure - EXPOSURE_STEPS); // save a few bytes by using this function
}

/* ------------------------------------------------------------------------- */ 

static void do_brightness(uint8_t backlight)
{
  extern Adafruit_miniTFTWing ss;
  extern tft_info_t           tft_info;

  tft_info.backlight = constrain(backlight, 10, 100);
  ss.setBacklight(map(tft_info.backlight,0,100,65535,0)); 
}

/* ------------------------------------------------------------------------- */ 

static void act_light_up()
{
  do_brightness(tft_info.backlight + BACKLIGHT_STEPS);  // save a few bytes by using this function
}

/* ------------------------------------------------------------------------- */ 

static void act_light_down()
{
  extern tft_info_t           tft_info;

  do_brightness(tft_info.backlight - BACKLIGHT_STEPS); // save a few bytes by using this function
}

/* ------------------------------------------------------------------------- */ 

static void act_gain_in()
{ 
  display_gain();
}

/* ------------------------------------------------------------------------- */ 

static void do_gain(uint8_t gain)
{
  extern as7262_info_t   as7262_info;
  extern Adafruit_AS726x ams;

  as7262_info.gain = gain;
  ams.setGain(gain); 
  display_gain();
}

/* ------------------------------------------------------------------------- */ 

static void act_gain_up()
{
  extern as7262_info_t   as7262_info;
  extern Adafruit_AS726x ams;

  do_gain((as7262_info.gain + 1) & 0b11); // save a few bytes by using this function
}

/* ------------------------------------------------------------------------- */ 

static void act_gain_down()
{
  extern as7262_info_t   as7262_info;
  extern Adafruit_AS726x ams;

  do_gain((as7262_info.gain - 1) & 0b11); // save a few bytes by using this function
}

/* ------------------------------------------------------------------------- */ 

static void act_bars_in()
{
  act_idle();
  tft.fillScreen(BLACK);
  display_bars(true);
}

/* ------------------------------------------------------------------------- */ 

static void act_bars_idle()
{
  act_idle();
  display_bars(false);
}

static void act_bars_hold()
{
  act_hold();
  act_bars_in();
}

/* ------------------------------------------------------------------------- */ 

static void act_data_in()
{
  extern tft_info_t tft_info;

  act_idle();
  tft.fillScreen(BLACK);
  display_data();
}

/* ------------------------------------------------------------------------- */ 

static void act_data_idle()
{
  act_idle();
  display_data();
}

/* ------------------------------------------------------------------------- */ 

static void act_lux_idle()
{
  act_idle();
  display_lux(false);
}

/* ------------------------------------------------------------------------- */ 

static void act_lux_in()
{
  
  tft.fillScreen(BLACK);
  tft.setTextSize(3);
  tft.setCursor(0,0);
  tft.setTextColor(YELLOW, BLACK);
  tft.print("Lux");
  act_idle();
  display_lux(true);
}

/* ------------------------------------------------------------------------- */ 

static void act_lux_hold()
{
  
  act_hold();
  act_lux_in();
}

/* ------------------------------------------------------------------------- */ 

static void act_accum_in()
{
  display_accum();
}

/* ------------------------------------------------------------------------- */ 

static void do_accum()
{
  as7262_clear_accum();
  display_accum();
}

/* ------------------------------------------------------------------------- */ 

static void act_accum_up()
{
  extern as7262_info_t   as7262_info;

  as7262_info.accLimit = min(MAX_ACCUM, 2*as7262_info.accLimit);
  do_accum(); // save a few bytes by using this function
}

/* ------------------------------------------------------------------------- */ 

static void act_accum_down()
{
  extern as7262_info_t   as7262_info;

  as7262_info.accLimit = max(1, as7262_info.accLimit/2);
  do_accum(); // save a few bytes by using this function
}

/* ------------------------------------------------------------------------- */ 


/* ************************************************************************** */ 
/*                              SETUP FUNCTIONS                              */
/* ************************************************************************** */ 

static void setup_serial()
{
  Serial.begin(115200);
  while(!Serial); // waits till hw ready in some Arduinos. Tight loo
  Serial.println("Version: " GIT_VERSION);
}

/* ************************************************************************** */ 

static void setup_ble()
{
  extern Adafruit_BluefruitLE_SPI ble;

  Serial.print("Bluefruit SPI...");
  
  if ( !ble.begin(VERBOSE_MODE) ) {
    //error(F("not found!"));
    error("not found!");
  }

  if ( FACTORYRESET_ENABLE && ! ble.factoryReset() ) {
    /* Perform a factory reset to make sure everything is in a known state */
    //error(F("could not be factory reset!"));
    error("could not be factory reset!");
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);
  // LED Activity command is only supported from Fiormware version 0.6.6
  ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);

  // Set module to DATA mode
  ble.setMode(BLUEFRUIT_MODE_DATA);
  Serial.println("ok");
  //ble.info();          // Set this only for debug
  //ble.verbose(false);  // debug info is a little annoying after this point!
}

/* ************************************************************************** */ 

static void setup_as7262()
{
  extern as7262_info_t as7262_info;
  extern Adafruit_AS726x ams;
 
  Serial.print("AS7262...");
  // finds the 6 channel chip
  if(!ams.begin()){
    //error(F("not found!"));
    error("not found!");      // Use RAM strings error(), saving flash memory
  }
  // as initialized by the AS7262 library
  // Note that in MODE 2, the exposure time is actually doubled
  as7262_info.gain     = GAIN_64X;
  as7262_info.exposure = 50;
  // continuous conversion time is already done by default in the ams driver
  //ams.setConversionType(MODE_2);
  // Reset accumulated readings
  as7262_info.accLimit = 1;
  as7262_clear_accum();
  Serial.println("ok");
}

/* ************************************************************************** */ 

static void setup_tft()
{
  extern Adafruit_miniTFTWing ss;
  extern Adafruit_ST7735     tft;
  extern tft_info_t          tft_info;

  Serial.print("SeeSaw...");
  // acknowledges the Seesaw chip before sending commands to the TFT display
  if (!ss.begin()) {
    //error(F("not found!"));
    error("not found!");    // Use RAM strings error(), saving flash memory
  }
  Serial.println("ok"); 
 
  ss.tftReset();   // reset the display via a seesaw command
  ss.setBacklight(TFTWING_BACKLIGHT_ON/2);  // turn on the backlight
  tft_info.backlight = 50;
  //ss.setBacklightFreq(10);  // turn on the backlight
  Serial.print("miniTFT...");
  tft.initR(INITR_MINI160x80);   // initialize a ST7735S chip, mini display
  tft.setRotation(3);            
  tft.fillScreen(BLACK);
  Serial.println("ok");
}

/* ************************************************************************** */ 

static void setup_opt3001()
{
  extern ClosedCube_OPT3001 opt3001;
  extern OPT3001_Config config;

  Serial.print("OPT3001...");
  opt3001.begin(OPT3001_ADDRESS);

  config.RangeNumber               = B1100;  // Automatic full-scale
  config.ConvertionTime            = B1;     // 800 ms
  config.Latch                     = B1;     // ???
  config.ModeOfConversionOperation = B11;    // Continuous operation

  OPT3001_ErrorCode errorConfig = opt3001.writeConfig(config);
  if (errorConfig != NO_ERROR) {
    //error(F("config error!"));
    error("config error!");    // Use RAM strings error(), saving flash memory
  }
  Serial.println("ok");
}

/* ************************************************************************** */ 

static void setup_fsm()
{
   screen = GUI_LUX_SCR;
   act_lux_in();
}

/* ************************************************************************** */ 
/*                                MAIN SECTION                               */
/* ************************************************************************** */ 


void setup() 
{
#ifdef ARDUINO_AVR_NANO_EVERY
  delay(1000); // The Nano Every needs a reasonable start delay so that all messages are shown
#endif
  setup_serial();
  setup_ble();
  setup_as7262();
  setup_opt3001();
  setup_tft(); 
  setup_fsm();
}


void loop() 
{
  extern uint8_t  screen; // The current screen
  menu_action_t   action;
  uint8_t         event;
 
  event  = read_stream(Serial) | read_stream(ble) | read_buttons();
  // dbg_fsm(screen, event);
  action = get_action(screen, event);
  screen = get_next_screen(screen, event);
  action();  // execute the action

}
