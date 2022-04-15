/*
   ESP-NOW gateway for SENSORNODE
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

#include <ESP8266WiFi.h> // ESP8266
#include <espnow.h>   // ESP8266
#include <WiFiClientSecure.h>
#include "DataToIftttMaker.h" // !!Remember to change the lines over in the DataToIftttMaker.h file as well.

///////////////////////////////////////////////////////////////////////
// GW configuration parameters
///////////////////////////////////////////////////////////////////////
const char* ssid = "Your network";
const char* password = "password";
const char* myKey = "YOUR KEY";     // Your IFTTT maker key: https://maker.ifttt.com/ -> Documentation
DataToMaker event(myKey, "ESP_NOW_GW");           // Must correspond to Event Name in IFTTT Webhook
///////////////////////////////////////////////////////////////////////

// Declare data structure for incoming data over ESP-NOW (Sender and Receiver must have identical structure!)
struct payloadStruct {
  char nodeName[20];
  char message[100];
} payload;

boolean payload_arrived = false;

void mbus_hexdump(uint8_t *buf, int len)
{
  Serial.printf("DUMP (%db) [ ", len);
  for (uint8_t *p = buf; p - buf < len; ++p)
    Serial.printf("%02X ", *p);
  Serial.print("]\n");
}

void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
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
}

void wifi_connect() {
  WiFi.mode(WIFI_AP_STA);

  //Connect to Wifi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
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

  // Init ESP-NOW
 esp_now_init();
  // ...and register the callback function
 esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  if (payload_arrived) {
    // Stop ESP-NOW
   esp_now_deinit();
    wifi_connect();
    //Send to IFTT
    boolean connectedToIfttt = false;
    while (connectedToIfttt == false) {
      if (event.connect())
      {
        Serial.println("Connected To IFTTT, pushing message");
        event.post();
        connectedToIfttt = true;
      }
      else {
        Serial.println("Failed To Connect To IFTTT!");
        delay(3000); // Wait 3 seconds before retry
      }
    }

    delay(1000);  //Wait for transmission to finish (can maybe be reduced)
  
    //Re-initialize ESP-NOW
    delay(100);
    WiFi.mode(WIFI_AP_STA);
  esp_now_init();
  esp_now_register_recv_cb(OnDataRecv);
    delay(100);

    payload_arrived = false;
  }

}
