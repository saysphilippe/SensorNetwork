/*
  This code is to run on the SensorNode. This code will run every time the sensor wakes up, or if you have soldered the VIN pad, to bypass the switch and the mosfet.
  @SaysPhilippe 2022
*/

#include <ESP8266WiFi.h>
#include <espnow.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// REPLACE WITH RECEIVER MAC Address
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
float calibration = -0.08; //Check Battery voltage using multimeter & add/subtract the value
int bat_percentage;
ADC_MODE(ADC_VCC);
#define ONE_WIRE_BUS 5
// Setup a oneWire instance to communicate with any OneWire devices

typedef struct struct_message {
  char deviceid[32];
  float voltage;
  float temperature;  
} struct_message;

// Create a struct_message called sensorData
struct_message sensorData;

unsigned long lastTime = 0;  
unsigned long timerDelay = 2000;  // send readings timer

// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
  }
  else{
    Serial.println("Delivery fail");
  }
}
 
void setup() {
  OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature sensor 
  DallasTemperature sensors(&oneWire);
  sensors.begin();
  sensors.requestTemperatures(); 
    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);   

  // Init Serial Monitor
  Serial.begin(115200);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 0, NULL, 0);
  
    // Set values to send
    strcpy(sensorData.deviceid, "SENSORNODE");
    sensorData.voltage = ESP.getVcc()/1024.+0.25;
    sensorData.temperature = sensors.getTempCByIndex(0);
    Serial.println();
    Serial.print("Voltage: ");
    Serial.println(ESP.getVcc()/1024.+0.25);
    Serial.print("Temperature: ");
    Serial.println(sensors.getTempCByIndex(0));
    // Send message via ESP-NOW
    esp_now_send(broadcastAddress, (uint8_t *) &sensorData, sizeof(sensorData));
  digitalWrite(2, LOW);
  ESP.deepSleep(30e6); //30e6 - This code will only run if you have soldered PIN 16 and RESET together.
  ESP.restart();
}
 
void loop() {
 
}
