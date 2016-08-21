/* Debug prints verbose debug details */
#define DEBUG
uint8_t sdCardReady = 0;

#define STATUS_GRAY   0
#define STATUS_GREEN  1
#define STATUS_YELLOW 2
#define STATUS_RED    3

// XBee buffer size
#define XBEE_BUFFER 150

#define MAX_BL 30000

uint32_t MY_ID=0;

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
long blTimeout = 0;
static gps_fix rmc_data;
long last_batt_read = 0;

boolean gpsrunning = false;
boolean buzzer_enabled = false;

#define RX_FRAME_OFFSET 12

#define PROTOCOL_0 0x00
#define PROTOCOL_1 0x01
#define PROTOCOL PROTOCOL_1

#define EEPROM_MSG_COUNT_OFFSET 0
#define EEPROM_SHARED_KEY_OFFSET EEPROM_MSG_COUNT_OFFSET+3
#define SHARED_KEY_SIZE 20

#define REACCION_VERSION "0.9.1"
