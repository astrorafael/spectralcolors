
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

              Arduino Nano        AS7262 Spectral Sensor
              ============        ======================
                    /
                   |  A4 (SDA)  -----> SDA 
              I2C  |  A5 (SCL)  -----> SCL 
                   \
                   /
                   |   3.3V    <====> 3.3V 
              Pwr  |   GND     <====> GND
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
                   |  A4 (SDA)  -----> SDA (+10k pullup)
              I2C  |  A5 (SCL)  -----> SCL (+10k pullup)
                   \
                   /
                   |   3.3V    <====> 3.3V 
              Pwr  |   GND     <====> GND
                   \


*/

/* ************************************************************************** */ 
/*                           INCLUDE HEADERS SECTION                          */
/* ************************************************************************** */ 

// Adafruit Spectral Sensor library
#include <Adafruit_AS726x.h>

// Adafruit Graphics libraries
#include <Adafruit_GFX.h>    		   // Core graphics library
#include <Adafruit_ST7735.h> 		   // Hardware-specific library
#include <Adafruit_miniTFTWing.h>  // Seesaw library for the miniTFT Wing display

// Support for the Git version tags
#include "git-version.h"

/* ************************************************************************** */ 
/*                                DEFINEs SECTION                             */
/* ************************************************************************** */ 

// ---------------------------------------------------------
// Define which Arduino nano pins will control the TFT Reset, 
// SPI Chip Select (CS) and SPI Data/Command DC
// ----------------------------------------------------------

#define TFT_RST -1  // miniTFTwing uses the seesaw chip for resetting to save a pin
#define TFT_CS   5 // Arduino nano D5 pin
#define TFT_DC   6 // Arduini nano D6 pin

// Exposure time step in milliseconds
#define EXPOSURE_UNIT 2.8

// steps in a single button up/down click
#define EXPOSURE_STEPS 20 

// maximun value expected fro the AS7262 chip
#define SENSOR_MAX 5000

// Short delay in screens (milliseconds)
#define SHORT_DELAY 200

// -----------------------------------------
// Some predefined colors for the 16 bit TFT
// -----------------------------------------

#define BLACK   0x0000
#define GRAY    0x8410
#define WHITE   0xFFFF
#define RED     0xF800
#define ORANGE  0xFA60
#define YELLOW  0xFFE0  
#define LIME    0x07FF
#define GREEN   0x07E0
#define CYAN    0x07FF
#define AQUA    0x04FF
#define BLUE    0x001F
#define MAGENTA 0xF81F
#define PINK    0xF8FF

/* ************************************************************************** */ 
/*                        CUSTOM CLASES & DATA TYPES                          */
/* ************************************************************************** */ 

typedef struct {
  float    calibratedValues[AS726x_NUM_CHANNELS];
  uint16_t rawValues[AS726x_NUM_CHANNELS];
  uint8_t  gain;           // device gain multiplier
  uint8_t  exposure;       // device integration time in steps of 2.8 ms
  uint8_t  temperature;    // device internal temperature
} sensor_info_t;

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
sensor_info_t sensor_info;

/* ************************************************************************** */ 
/*                      GUI STATE MACHINE DECLARATIONS                        */
/* ************************************************************************** */ 

// Events generated by user
enum gui_events {
  GUI_NO_EVENT      = 0,
  GUI_KEY_A_PRESSED,
  GUI_KEY_B_PRESSED,
  GUI_JOY_PRESSED,
  GUI_JOY_UP,
  GUI_JOY_DOWN,
  GUI_JOY_LEFT,
  GUI_JOY_RIGHT
};

// TFT Screens as states
enum gui_state {
  GUI_GAIN_SCREEN     = 0,
  GUI_EXPOSURE_SCREEN,
  GUI_READINGS_SCREEN
};

// --------------------------------------------
// State Machine Actions (Forward declarations)
// --------------------------------------------
static void act_idle();
static void act_gain_enter();
static void act_gain_up();
static void act_gain_down();
static void act_exposure_enter();
static void act_exposure_up();
static void act_exposure_down();
static void act_readings_enter();


// Action to execute as a function of current state and event
// This table is held in Flash memory to save precious RAM
// Use of PROGMEM and pgm_xxx() functions is necessary
static menu_action_t get_action(uint8_t state, uint8_t event)
{
  static const menu_action_t menu_action[][3] PROGMEM = {
    // GAIN SCREEN      |   EXPOSURE SCREEN    | READINGS SCREEN
    //------------------+----------------------+-----------------
    {act_idle,           act_idle,               act_readings_enter}, // GUI_NO_EVENT
    {act_gain_up,        act_exposure_up,        act_idle},           // GUI_KEY_A_PRESSED
    {act_gain_down,      act_exposure_down,      act_idle},           // GUI_KEY_B_PRESSED
    {act_idle,           act_idle,               act_idle},           // GUI_JOY_PRESSED
    {act_gain_up,        act_exposure_up,        act_idle},           // GUI_JOY_UP
    {act_gain_down,      act_exposure_down,      act_idle},           // GUI_JOY_DOWN
    {act_readings_enter, act_gain_enter,         act_exposure_enter}, // GUI_JOY_LEFT
    {act_exposure_enter, act_readings_enter,     act_gain_enter}      // GUI_JOY_RIGHT
  };
  return (menu_action_t) pgm_read_ptr(&menu_action[event][state]);
}

/* -------------------------------------------------------------------------- */ 

// Next state to proceed as a function of current state and event
// This table is held in Flash memory to save precious RAM
// Use of PROGMEM and pgm_xxx() functions is necessary
static uint8_t get_next_screen(uint8_t state, uint8_t event)
{
  static const PROGMEM uint8_t next_screen[][3] = {
      // GAIN SCREEN      |   EXPOSURE SCREEN    | READINGS SCREEN
      //------------------+----------------------+-----------------
      {GUI_GAIN_SCREEN,     GUI_EXPOSURE_SCREEN,   GUI_READINGS_SCREEN}, // GUI_NO_EVENT
      {GUI_GAIN_SCREEN,     GUI_EXPOSURE_SCREEN,   GUI_READINGS_SCREEN}, // GUI_KEY_A_PRESSED
      {GUI_GAIN_SCREEN,     GUI_EXPOSURE_SCREEN,   GUI_READINGS_SCREEN}, // GUI_KEY_B_PRESSED
      {GUI_GAIN_SCREEN,     GUI_EXPOSURE_SCREEN,   GUI_READINGS_SCREEN}, // GUI_JOY_PRESSED
      {GUI_GAIN_SCREEN,     GUI_EXPOSURE_SCREEN,   GUI_READINGS_SCREEN}, // GUI_JOY_UP
      {GUI_GAIN_SCREEN,     GUI_EXPOSURE_SCREEN,   GUI_READINGS_SCREEN}, // GUI_JOY_DOWN
      {GUI_READINGS_SCREEN, GUI_GAIN_SCREEN,       GUI_EXPOSURE_SCREEN}, // GUI_JOY_LEFT
      {GUI_EXPOSURE_SCREEN, GUI_READINGS_SCREEN,   GUI_GAIN_SCREEN}      // GUI_JOY_RIGHT
    };
  return pgm_read_byte(&next_screen[event][state]);
}

/* ************************************************************************** */ 
/*                            HELPER FUNCTIONS                                */
/* ************************************************************************** */ 

// Reads miniTFTWing buttons & joystick and produces events
static uint8_t read_buttons()
{
  extern Adafruit_miniTFTWing ss;

  uint8_t event = GUI_NO_EVENT;

  // miniTFT wing buttons;
  uint32_t buttons;

  // read buttons via the I2C SeeSaw chip in miniTFTWing
  // These buttons are active-low logic
  buttons = ss.readButtons();


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


static int read_sensor()
{
  extern Adafruit_AS726x ams;
  extern sensor_info_t sensor_info;
  uint8_t freshData = 0;

  if(ams.dataReady()) {
    freshData = 1;
    ams.readCalibratedValues(sensor_info.calibratedValues);
    ams.readRawValues(sensor_info.rawValues);
    sensor_info.temperature = ams.readTemperature();
    //Serial.print('+');
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  } else {
    //Serial.print('-');
    digitalWrite(LED_BUILTIN, LOW);   // turn the LED on (HIGH is the voltage level)
  }
  return freshData;
}

/* ************************************************************************** */ 

static void display_bars()
{
  // array of bar colors
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

  extern sensor_info_t sensor_info;
  
  uint16_t barWidth = (tft.width()) / AS726x_NUM_CHANNELS;
  for(int i=0; i<AS726x_NUM_CHANNELS; i++) {
    uint16_t color  = pgm_read_word(&colors[i]);
    uint16_t height = map(sensor_info.calibratedValues[i], 0, SENSOR_MAX, 0, tft.height());
    tft.fillRect(barWidth * i, 0, barWidth, tft.height() - height, ST7735_BLACK);
    tft.fillRect(barWidth * i, tft.height() - height, barWidth, height, color);
  }
}

/* ************************************************************************** */ 


static void display_gain()
{
  extern sensor_info_t sensor_info;

  // strings to display on TFT
  // It is not worth to place these strings in Flash
  static const char* gain_table[] = {
    "1x",
    "3.7x",
    "16x",
    "64x"
  };

  tft.fillScreen(ST7735_BLACK);
  // Display the "Gain" sttring in TFT
  tft.setTextSize(3); // 3x the original font
  tft.setCursor(tft.height()/3, 0);
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.print("Gain");
  // Display the gain value string in TFT
  tft.setCursor(tft.height()/3, tft.width()/3);
  tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
  tft.print(gain_table[sensor_info.gain]);
  
}

/* ************************************************************************** */ 

static void display_exposure()
{
  extern sensor_info_t sensor_info;

  tft.fillScreen(ST7735_BLACK);
  // Display the "Gain" sttring in TFT
  tft.setTextSize(3);
  tft.setCursor(0,0);
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.print("Exposure");
  // Display the exposure value string in TFT
  tft.setCursor(0, tft.width()/3);
  tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
  tft.print(sensor_info.exposure*EXPOSURE_UNIT,1); tft.print(" ms");
}


/* ************************************************************************** */ 
/*                      STATE MACHINE ACTION FUNCTIONS                        */
/* ************************************************************************** */ 

static void act_idle()
{
  read_sensor();
}

/* ------------------------------------------------------------------------- */ 

static void act_exposure_enter()
{ 
  display_exposure();
  delay(SHORT_DELAY);
}

/* ------------------------------------------------------------------------- */ 

static void act_exposure_up()
{
  extern sensor_info_t sensor_info;
  extern Adafruit_AS726x ams;
  int exposure = sensor_info.exposure + EXPOSURE_STEPS;

  sensor_info.exposure = constrain(exposure, 1, 255);
  ams.setIntegrationTime(sensor_info.exposure); 
  display_exposure();
  delay(SHORT_DELAY);
}

/* ------------------------------------------------------------------------- */ 

static void act_exposure_down()
{
  extern sensor_info_t sensor_info;
  extern Adafruit_AS726x ams;
  int exposure = sensor_info.exposure - EXPOSURE_STEPS;

  sensor_info.exposure = constrain(exposure, 1, 255);
  ams.setIntegrationTime(sensor_info.exposure); 
  display_exposure();
  delay(SHORT_DELAY);
}

/* ------------------------------------------------------------------------- */ 

static void act_gain_enter()
{ 
  display_gain();
  delay(SHORT_DELAY);
}

/* ------------------------------------------------------------------------- */ 

static void act_gain_up()
{
  extern sensor_info_t sensor_info;
  extern Adafruit_AS726x ams;

  sensor_info.gain = (sensor_info.gain + 1) & 0b11;
  ams.setGain(sensor_info.gain); 
  display_gain();
  delay(SHORT_DELAY);
}

/* ------------------------------------------------------------------------- */ 

static void act_gain_down()
{
  extern sensor_info_t sensor_info;
  extern Adafruit_AS726x ams;

  sensor_info.gain = (sensor_info.gain - 1) & 0b11;
  ams.setGain(sensor_info.gain); 
  display_gain();
  delay(SHORT_DELAY);
}

/* ------------------------------------------------------------------------- */ 

static void act_readings_enter()
{
  uint8_t freshData;
  freshData = read_sensor();
  if (freshData)
    display_bars();
  else
    display_bars();
    delay(SHORT_DELAY);
}

/* ************************************************************************** */ 
/*                              SETUP FUNCTIONS                              */
/* ************************************************************************** */ 

static void setup_sensor()
{
  extern sensor_info_t sensor_info;
  extern Adafruit_AS726x ams;
  
  // finds the 6 channel chip
  if(!ams.begin()){
    Serial.println(F("could not connect to sensor! Please check your wiring."));
    while(1);
  }
  // as initialized by the AS7262 library
  // Note that in MODE 2, the exposure time is actually doubled
  sensor_info.gain     = GAIN_64X;
  sensor_info.exposure = 50;
  // continuous conversion time
  ams.setConversionType(MODE_2);
}

/* ************************************************************************** */ 

static void setup_tft()
{
  extern Adafruit_miniTFTWing ss;
  extern Adafruit_ST7735     tft;
  // acknowledges the Seesaw chip before sending commands to the TFT display
  if (!ss.begin()) {
    Serial.println(F("seesaw couldn't be found!"));
    while(1); // stops forever if cannto initialize
  }

  Serial.print(F("seesaw started!\tVersion: "));
  Serial.println(ss.getVersion(), HEX);

  ss.tftReset();   // reset the display via a seesaw command
  ss.setBacklight(TFTWING_BACKLIGHT_ON);  // turn on the backlight
  //ss.setBacklight(0x0000);  // turn off the backlight
  //ss.setBacklightFreq(5000);  // turn on the backlight

  tft.initR(INITR_MINI160x80);   // initialize a ST7735S chip, mini display
  tft.setRotation(3);            
  tft.fillScreen(ST7735_BLACK);
  
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println(F("TFT initialized"));
}


/* ************************************************************************** */ 
/*                                MAIN SECTION                               */
/* ************************************************************************** */ 


void setup() 
{
  Serial.begin(9600);
  Serial.println(F("Sketch version: " GIT_VERSION));
  pinMode(LED_BUILTIN, OUTPUT);
  setup_sensor();
  setup_tft();
}

void loop() 
{
  static uint8_t  screen = GUI_GAIN_SCREEN; // The current screen
  menu_action_t   action;
  uint8_t         event;
 
  event  = read_buttons();
  //Serial.print(F("State: "));  Serial.print(screen); Serial.print(F(" Event: ")); Serial.println(event);
  action = get_action(screen, event);
  screen = get_next_screen(screen, event);
  action();  // execute the action
}
