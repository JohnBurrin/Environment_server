#include "_config.h"
#include "calibration.h"

#include "SSD1306Wire.h" 
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <SI7021.h>
#ifdef _COMPILE_DISPLAY 
  #include "SSD1306Wire.h"
#endif

#define SDA 4
#define SCL 5

SI7021 sensor;

#ifdef  _COMPILE_BMP_280
  #include <Adafruit_BMP280.h>
  Adafruit_BMP280 bmp; // use I2C interface
  Adafruit_Sensor *bmp_temp = bmp.getTemperatureSensor();
  Adafruit_Sensor *bmp_pressure = bmp.getPressureSensor();
#endif

WiFiClient wifiClient;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

#ifdef _COMPILE_DISPLAY 
SSD1306Wire display(0x3c, SDA, SCL);   // ADDRESS, SDA, SCL  -  SDA and SCL usually populate automatically based on your board's pins_arduino.h
#endif

int counter = 0;
float pressure;
float temperature;
int humidity;

String wifiMacString;
String wifiIPString;
String timeString = "00:00:00";
String sensorString = "Bare with..";

void setSensorValues() {
      #ifdef _COMPILE_BMP_280
        sensors_event_t pressure_event; 
        bmp_pressure->getEvent(&pressure_event);
        pressure = pressure_event.pressure + pressure_offset;
        //temp_event.temperature;
    #endif
    temperature = (((float)sensor.getCelsiusHundredths()) /100) + temperature_offset;
    humidity  = sensor.getHumidityPercent() + humidity_offset;
}

void handleRoot() {
  char page[800];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;
  snprintf(page, 800,
           "<html>\
              <head>\
                <meta http-equiv='refresh' content='5'/>\
                <title>Environment Hub</title>\
                <style>\
                  body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
                </style>\
              </head>\
              <body>\
                <h1>Temperature and Humidity</h1>\
                <p>Temperature: %f</p>\
                <p>Humidity: %f</p>\
                <p>Time: %s</p>\
                <p>Uptime: %02d:%02d:%02d</p>\
              </body>\
            </html>",temperature, humidity, timeString, hr, min % 60, sec % 60
          );
  httpServer.send(200, "text/html", page);
}

void setup()
{
    Serial.begin(115200);
    timeClient.begin();
    #ifdef _COMPILE_DISPLAY 
      // Initialising the UI will init the display too.
      display.init();
    
      display.flipScreenVertically();
      display.setFont(ArialMT_Plain_10);
      display.clear();
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.setFont(ArialMT_Plain_24);
      display.drawString(0, 0, "Booting.");
      display.display();
      delay(500);
    #endif
    pinMode(LED_BUILTIN, OUTPUT);
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(ssid, password);

    Serial.println("Connecting");
    while (WiFi.status() != WL_CONNECTED)
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
        Serial.print(".");
        #ifdef _COMPILE_DISPLAY 
        display.print(".");
        #endif
    }
    Serial.println("");
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());
    MDNS.begin(host);

  httpServer.on("/", handleRoot);

  httpUpdater.setup(&httpServer, update_path, update_username, update_password);
  httpServer.begin();

  MDNS.addService("http", "tcp", 80);
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local%s in your browser and login with username '%s' and password '%s'\n", host, update_path, update_username, update_password);
    wifiMacString = WiFi.macAddress();
    wifiIPString = WiFi.localIP().toString();

    httpServer.on("/", handleRoot);
    sensor.begin(SDA, SCL);
    
    #ifdef _COMPILE_BMP_280
      bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID);
    #endif
}

void loop()
{   
    httpServer.handleClient();
    MDNS.update();

    setSensorValues();
    
    //Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED)
    {
        if (millis() > (ntpInterval + ntpi)) {
          display.clear();
          timeClient.update();
          timeString = timeClient.getFormattedTime();
          #ifdef _COMPILE_DISPLAY 
            display.setTextAlignment(TEXT_ALIGN_LEFT);
            display.drawString(0, 0, timeString);
            display.drawString(0, 30, sensorString);
            display.display();
          #endif
          //Serial.println(timeClient.getFormattedTime());
          ntpi = millis();
        }
        
        HTTPClient http;

        // Your Domain name with URL path or IP address with path
        http.begin(wifiClient, serverName);

        // Specify content-type header
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");

        String httpRequestData = "api_key=" + apiKeyValue + "&sensor=" + sensorName + "&location=" + sensorLocation;
        httpRequestData = httpRequestData + "&temperature=" + temperature + "&humidity=" + humidity;
        httpRequestData = httpRequestData + "&mac_address=" + String(wifiMacString) + "&ip_address=" + String(wifiIPString);
        
        #ifdef _COMPILE_BMP_280
          httpRequestData = httpRequestData + "&pressure=" + pressure;
        #endif
        
        httpRequestData = httpRequestData + "&sensor_id=" + sensorId + "";

        if (millis() > (sendInterval + si)) {
          digitalWrite(LED_BUILTIN, LOW);
          // uncomment to see the post data in the monitor
          Serial.println(httpRequestData);
          
          // Send HTTP POST request
          int httpResponseCode = http.POST(httpRequestData);
#ifdef _COMPILE_DISPLAY
          switch (counter % 3) {
            case 0:
              sensorString = String(pressure) + "mb";
            break;
            case 1:
              sensorString = String(temperature) + (char)247 + "°C";
            break;
            case 2:
              sensorString = String(humidity) + "%";
            break;
          }
          counter ++;
          if (counter > 2){
            counter = 0;
          }
#endif
          if (httpResponseCode > 0)
          {
              Serial.print("HTTP Response code: ");
              Serial.println(httpResponseCode);
          }
          else
          {
              Serial.print("Error code: ");
              Serial.println(httpResponseCode);
          }
          si = millis();
           digitalWrite(LED_BUILTIN, HIGH);
        }
        // Free resoclearurces
        http.end();
    }
    else
    {
        Serial.println("WiFi Disconnected");
    }
}
