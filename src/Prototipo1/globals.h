/* Debug prints verbose debug details */
#define DEBUG
uint8_t sdCardReady = 0;

#define STATUS_GRAY   0
#define STATUS_GREEN  1
#define STATUS_YELLOW 2
#define STATUS_RED    3

// XBee buffer size
#define XBEE_BUFFER 100

// UI Button BEHAVIOUR
#define BTN_TIMEOUT 5000 // Timeout before sending message

// Custom non-blocking xBee library
XBee900HP * myXBee;
uint8_t xBeeData[XBEE_BUFFER];
char xBeeBuffPos = 0;

// Filesystem libs
SdFat sd;
// Log file.
SdFile file;
bool file_ready = false;

// UI variables
long timeoutA = 0;
long timeoutB = 0;
long timeoutC = 0;
long timeoutD = 0;
long lastStatus = 0;
static gps_fix rmc_data;
long last_batt_read = 0;

boolean gpsrunning = false;
