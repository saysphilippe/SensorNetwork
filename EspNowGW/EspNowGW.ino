/*
   ESP-NOW gateway for SENSORNODE.
   Compiles on ESP8266 or ESP32.
   Sends push message over IFTTT.

   How to set up IFTTT webhook:
    1) Sign up at ifttt.com (free for up to 5 applets) and connect it to your phone.
    2) Create webhook applet:
      a) Click "Create"
      b) Click If This "Add"
      c) Select "Webhooks"
      d) Select "Receive a web request"
      e) Define "Event Name" --> Use name in configuration parameters below
      f) Click "Create trigger"
      g) Click on "That" and select "Notifications"
      h) Select "Send a notification from the IFTTT app"
      i) Write the notification message to be sent
      j) Click "Create action"
      k) Click "Continue"
      l) Edit Applet title if you want to make it more intuitive
      m) Click "Finish"

      Up to 3 values can be sent to IFTTT webhook using:  event.setValue(1, String(parameter_1));
                                                          event.setValue(2, String(parameter_2));
                                                          event.setValue(3, String(parameter_3));

*/

#ifdef ESP32
#include <esp_now.h>
#include <WiFi.h>
#elif defined(ESP8266)
#include <espnow.h>
#include <ESP8266WiFi.h>
#else
#error "Du må bruke enten ESP8266 eller ESP32"
#endif

#include <WiFiClientSecure.h>
#include <Adafruit_NeoPixel.h> //To use Neopixel on ESP-32-S2-Saola development board connected to GPIO18
#include "DataToIftttMaker.h"
#include "mycredentials.h"    // Defines MYSSID and MYPASSWOR

///////////////////////////////////////////////////////////////////////
// GW configuration parameters
///////////////////////////////////////////////////////////////////////
const char* ssid = MYSSID;
const char* password = MYPASSWORD;
IPAddress local_IP(10, 0, 0, 250);   // Set your Static IP address (oppkobling går raskere med statisk IP)
IPAddress gateway(10, 0, 0, 3);      // Set your Gateway IP address
IPAddress subnet(255, 255, 255, 0);     // Subnet
IPAddress primaryDNS(1, 1, 1, 1);
IPAddress secondaryDNS(8, 8, 8, 8);
const char* myKey = "bFcjG5fIGPEeUiRJ284zi4";     // Your IFTTT maker key: https://maker.ifttt.com/ -> Documentation
DataToMaker event(myKey, "ESP_NOW_GW");           // Must correspond to Event Name in IFTTT Webhook
#define NEOPIXELPIN 18                          // Neopixel connected on GPIO18
///////////////////////////////////////////////////////////////////////

// Neopixel
#define BLINKDELAY 50
#define LEDINTENSITY 100
// setup for one NeoPixel attached to GPIO "NEOPIXELPIN"
Adafruit_NeoPixel pixels(1, NEOPIXELPIN, NEO_GRB + NEO_KHZ800);

// Declare data structure for incoming data over ESP-NOW (Sender and Receiver must have identical structure!)
struct payloadStruct {
  char nodeName[20];
  char message[100];
} payload;

unsigned long timer;
boolean payload_arrived = false;

void BlinkGreenNeopixel()
{
  pixels.setPixelColor(0, pixels.Color(0, LEDINTENSITY, 0));
  pixels.show();   // Send the updated pixel colors to the hardware.
  timer = millis();
  while (millis() < timer + BLINKDELAY) { // Non blocking delay
  }
  pixels.clear(); // Set all pixel colors to 'off'
  pixels.show();   // Send the updated pixel colors to the hardware.
  timer = millis();
  while (millis() < timer + BLINKDELAY) { // Non blocking delay
  }
}

void BlinkBlueNeopixel()
{
  pixels.setPixelColor(0, pixels.Color(0, 0, LEDINTENSITY));
  pixels.show();   // Send the updated pixel colors to the hardware.
  timer = millis();
  while (millis() < timer + BLINKDELAY) { // Non blocking delay
  }
  pixels.clear(); // Set all pixel colors to 'off'
  pixels.show();   // Send the updated pixel colors to the hardware.
  timer = millis();
  while (millis() < timer + BLINKDELAY) { // Non blocking delay
  }
}


// callback function that will be executed when data is received
// The function declarations are slightly different ESP32 vs ESP8266, so...
#ifdef ESP32
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
#include <WiFi.h>
#elif defined(ESP8266)
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
#else
#error "Du må bruke enten ESP8266 eller ESP32"
#endif
  memcpy(&payload, incomingData, sizeof(payload));
  Serial.print("Bytes received: ");
  Serial.print(len);
  Serial.print("\t"); //Tabulation
  Serial.print(payload.nodeName);
  Serial.print("\t"); //Tabulation
  Serial.println(payload.message);

  // Prepare payload to be sent to IFTTT webhook
  // Three parameters are sent:
  //  1) Node name from incoming message
  //  2) Parameter name and unit, example: "Voltage [V]"
  //  3) Measurement value with number of decimals as specified in incoming message
  char writeBuffer[20];
  event.setValue(1, String(payload.nodeName));
  event.setValue(2, String(payload.message));

  payload_arrived = true;
  BlinkGreenNeopixel(); //Green blink on a Neopixel connected to GPIO18
}

void wifi_connect() {
  pixels.begin(); // INITIALIZE NeoPixel
  pixels.clear(); // Set all pixel colors to 'off'
  pixels.show();  // Send the updated pixel colors to the hardware.

  WiFi.mode(WIFI_AP_STA);
  WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);

  //Connect to Wifi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  long int reboottimer = millis();
  while (WiFi.status() != WL_CONNECTED) {
    timer = millis();
    while (millis() < timer + 500) { // Non blocking delay 500ms
    }
    Serial.print(".");
    BlinkBlueNeopixel();
  }
  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.print("WiFi channel: ");
  Serial.println(WiFi.channel());
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
}

void setup() {
  // Initialize Serial
  Serial.begin(115200);
  Serial.println("***********************");
  Serial.println("*   ESP-NOW Gateway   *");
  Serial.println("***********************");

  //Connect to Wifi
  wifi_connect();
  Serial.println("ESP-NOW nodes must be set to the same WiFi channel!");

#ifdef ESP32
  // Init ESP-NOW
  ESP_ERROR_CHECK(esp_now_init());
  // ...and register the callback function
  ESP_ERROR_CHECK(esp_now_register_recv_cb(OnDataRecv));
#elif defined(ESP8266)
  esp_now_init();
  esp_now_register_recv_cb(OnDataRecv);
#else
#error "Du må bruke enten ESP8266 eller ESP32"
#endif

}

void loop() {
  if (payload_arrived) {
#ifdef ESP32
    // Stop ESP-NOW
    ESP_ERROR_CHECK(esp_now_deinit()); //De-initialize ESP-NOW while using Wifi
#elif defined(ESP8266)
    esp_now_deinit(); //De-initialize ESP-NOW while using Wifi
#else
#error "Du må bruke enten ESP8266 eller ESP32"
#endif

    //wifi_connect();
    WiFi.reconnect(); //Ser ut for å fungere bedre enn å gjenta en vanlig "connect".

    //Send to IFTT
    boolean connectedToIfttt = false;
    while (connectedToIfttt == false) {
      if (event.connect())
      {
        Serial.println("Connected To IFTTT, pushing message");
        event.post();
        connectedToIfttt = true;
        BlinkGreenNeopixel();
        BlinkGreenNeopixel();
      }
      else {
        Serial.println("Failed To Connect To IFTTT!");
        // Wait 3 seconds before retry
        timer = millis();
        while (millis() < timer + 3000) { // Non blocking delay 3000 ms
        }
      }
    }

    //Wait for transmission to finish (can maybe be reduced)
    timer = millis();
    while (millis() < timer + 1000) { // Non blocking delay 1000 ms
    }

    //Re-initialize ESP-NOW
    timer = millis();
    while (millis() < timer + 100) { // Non blocking delay 100 ms
    }
    WiFi.mode(WIFI_AP_STA);

#ifdef ESP32
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(OnDataRecv));
#elif defined(ESP8266)
    esp_now_init();
    esp_now_register_recv_cb(OnDataRecv);
#else
#error "Du må bruke enten ESP8266 eller ESP32"
#endif
    delay(100);

    payload_arrived = false;
  }

}
