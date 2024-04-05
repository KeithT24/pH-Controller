#define WIFI_CHANNEL 1

char ssid[] = ;
char pass[] = ;                   
byte homeassistant[] = { 192,168,1,185 };
const char* mqtt_server = "192.168.1.185";


unsigned long lastConnectionTime = 0;
const unsigned long postingInterval = 5 * 1000;


//distance sensor pins
const int trigPin = 5;
const int echoPin = 18;
//pump pins
const int phUpPin = 15;
const int topPin = 4;
const int phDownPin = 2;

#define ONE_WIRE_BUS 14
#define TEMPERATURE_PRECISION 9


//Distance Sensor stuff
//define sound speed in cm/uS
#define SOUND_SPEED 0.034
#define CM_TO_INCH 0.393701
float dist_offset = 10;
long duration;
float distanceCm;
float distanceInch;

//pH and Dosing Monitor variables
float phReading = 0;
bool upPump = 0;
bool downPump = 0;
bool topPump = 0;
bool phDoser = 0;
bool topOff = 0;
float phTarget = 6.00;
float phTolerance = 0.20;
float waterTemp = 0;

//timer values
unsigned long doser_delay = 1 * 60000; 
unsigned int doser_time = 1;

unsigned long top_delay = 1 * 60000; 
unsigned int top_time = 5;


unsigned short doser_intervalId;
unsigned short top_intervalId;

const unsigned long readInterval = 2 * 1000; //2 second sensor read interval
unsigned long lastRead = 0;

enum system_mode {
  ON,
  CAL7,
  CAL4,
  CAL10,
  CALCLEAR
};

static const char * const mode_names[] = {
  [ON] = "On",
  [CAL7] = "Cal7",
  [CAL4] = "Cal4",
  [CAL10] = "Cal10"
};
