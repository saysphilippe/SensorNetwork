/*
  Code to run on @SaysPhilippe SensorNode.
  @opsahle, 9-apr-2022
*/

#include <ESP8266WiFi.h>
#include <espnow.h>
#include <OneWire.h>
#include <DallasTemperature.h>

/////////////////////////////////////////////////////////////////////
// Node configuration parameters
/////////////////////////////////////////////////////////////////////
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};  // Can be replaced by ESP-NOW receiver MAC Address
//uint8_t broadcastAddress[] = {0x7C, 0xDF, 0xA1, 0x00, 0x9F, 0x88};  // Can be replaced by ESP-NOW receiver MAC Address  **NOT WORKING!**
#define NODENAME  "Postkassa"                                        // Maximum 15 characters
int32_t wifi_channel = 12;                                           // Run receiver code to obtain this

///////////////////////////////////////////////////////////////////////////////////////////
// Procedure for Vcc read calibration (adjusting for individual ADC gain in your ESP8266)
///////////////////////////////////////////////////////////////////////////////////////////
// 1) Start with: adc_cal = 1.0;
// 2) Run code
// 3) Note voltage measured by firmware: Vcc_f
// 4) Measure actual voltage on the board close to ESP8266: Vcc_m
// 5) Calculate: adc_cal = Vcc_m / Vcc_f
// 6) Insert value here:
float adc_cal = 0.962;

// ESP8266 ADC setup: Read internal Vcc
ADC_MODE(ADC_VCC);


// Temp sensor DS18B20 is connected to GPIO5
#define ONE_WIRE_BUS 5

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// Declare data structure for sending over ESP-NOW (Sender and Receiver must have identical structure!)
struct payloadStruct {
  char nodeName[20];
  char message[100];
} payload;

int sendResult;

// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0) {
    Serial.println("Delivery success");
  }
  else {
    Serial.println("Delivery fail");
  }
}

void setup() {
  // Initialize Serial
  Serial.begin(115200);
  Serial.println();
  Serial.println("***********************");
  Serial.println("*     ESP-NOW node    *");
  Serial.println("***********************");

  sensors.begin();  // Initiate reading from temp sensor

  // ESP LED on while code is active
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Set wifi channel to same as receiver
  wifi_promiscuous_enable(1);
  wifi_set_channel(wifi_channel);
  wifi_promiscuous_enable(0);

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);

  ///////////////////////////////////////////////////////////////////////////////
  // esp_now_register_send_cb(OnDataSent);
  ///////////////////////////////////////////////////////////////////////////////

  // Register peer
  int addStatus = esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, wifi_channel, NULL, 0);
  if (addStatus == 0) {
    // Pair success
    Serial.println("ESP-NOW pair success");
  } else {
    Serial.println("ESP-NOW pair failed");
  }
  Serial.print("ESP-NOW node WiFi channel: ");
  Serial.println(WiFi.channel());

  //Read temp sensor
  sensors.requestTemperatures(); // Read data from temp sensor

  // Set nodename in payload
  strcpy(payload.nodeName, NODENAME);

  // Copy Vcc and temp measurements into nodeData structure and send payload
  strcpy(payload.message, "Voltage/temp: ");
  char textBuffer[10];
  dtostrf((float)ESP.getVcc() / 1024 * adc_cal, 4, 2, textBuffer); // Voltage with two decimals in 4 characters
  strcat(payload.message, textBuffer);
  strcat(payload.message, " V / ");
  dtostrf(sensors.getTempCByIndex(0), 4, 1, textBuffer); // Temp with one decimals in 4 characters
  strcat(payload.message, textBuffer);
  strcat(payload.message, " degC");

  // Send payload
  esp_now_send(broadcastAddress, (uint8_t *) &payload, sizeof(payload));

  // Turn off LED and go to deepsleep
  digitalWrite(LED_BUILTIN, HIGH);
  //For continuous testing using code in loop(): Comment out next line
  ESP.deepSleep(0); //Eternal deep sleep
}

void loop() {

  //Execution will never reach loop() if the last line in setup() is uncommented.

  //Read temp sensor
  sensors.requestTemperatures(); // Read data from temp sensor

  // Copy Vcc and temp measurements into nodeData structure and send payload
  strcpy(payload.message, "Voltage/temp: ");
  char textBuffer[10];
  dtostrf((float)ESP.getVcc() / 1024 * adc_cal, 4, 2, textBuffer); // Voltage with two decimals in 4 characters
  strcat(payload.message, textBuffer);
  strcat(payload.message, " V / ");
  dtostrf(sensors.getTempCByIndex(0), 4, 1, textBuffer); // Temp with one decimals in 4 characters
  strcat(payload.message, textBuffer);
  strcat(payload.message, " degC");

  // Send payload
  esp_now_send(broadcastAddress, (uint8_t *) &payload, sizeof(payload));
  delay(1000);
  
}
