#ifndef _CONFIG_H
#define _CONFIG_H
const int sendInterval = 5000;
unsigned long int si = 0;
const int ntpInterval = 1000;
unsigned long int ntpi = 0;

// Replace with your network credentials
const char *ssid = "BT-T6A2PN";
const char *password = "DKuAv9LaJtDda3";

const char* host = "esp8266-webupdate";
const char* update_path = "/ota";
const char* update_username = "admin";
const char* update_password = "admin";

// REPLACE with your Domain name and URL path or IP address with path
const char *serverName = "http://some-domain.co.uk";

// // API Key should we need it.
String apiKeyValue = "please-changeme-if-you-need-me";

String sensorName = "SI1702";
String sensorLocation = "Home";

String sensorId = "S1";

const long utcOffsetInSeconds = 3600;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};


// remove this define to not compile BMP280 sensor
#define _COMPILE_BMP_280
// remove this define to not compile the display
#define _COMPILE_DISPLAY

#endif
