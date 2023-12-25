#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <espnow.h>

// REPLACE WITH RECEIVER MAC Address
uint8_t receiver_master[] = {0xA8,0x48,0xFA,0XFF,0xEF,0x77};
uint8_t receiver_motor[] = {0xE8,0x9F,0x6D,0x94,0x0A,0x11};
// Structure example to send data
// Must match the sender structure
typedef struct struct_message {
   //1 for tank esp
   //2 for supply esp
    int id;
    float level;
} struct_message;

// Create a struct_message called myData
struct_message myData;

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
  // Init Serial Monitor
  Serial.begin(9600);
 
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
  esp_now_add_peer(receiver_master, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
  esp_now_add_peer(receiver_motor, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
}
 
void loop() {
  if ((millis() - lastTime) > timerDelay) {
    // Set values to send
    //1 for tank esp
    //2 for supply esp
    //strcpy(myData.a, "THIS IS A CHAR");
    myData.id = 1;
    myData.level = 88.4;


    // Send message via ESP-NOW
    esp_now_send(receiver_master, (uint8_t *) &myData, sizeof(myData));
    esp_now_send(receiver_motor, (uint8_t *) &myData, sizeof(myData));
    lastTime = millis();
  }
}