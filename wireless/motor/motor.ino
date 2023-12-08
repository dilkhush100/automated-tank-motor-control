
#include <ESP8266WiFi.h>
#include <espnow.h>


// REPLACE WITH THE MAC Address of your receiver 
uint8_t master_MAC[] = {0xA8,0x48,0xFA,0XFF,0xEF,0x77};
uint8_t tank_MAC[] = {0xE8,0x9F,0x6D,0x94,0x0A,0x11};

unsigned long lastTime = 0;  
unsigned long timerDelay = 2000;  // send readings timer

// Variable to store if sending data was successful
String success;

//Structure example to send data
//Must match the receiver structure
typedef struct struct_message_tank {
    int id;
    float level;
} messageFromTank;

// Create a struct_message called DHTReadings to hold sensor readings
messageFromTank incomingTankReading;

typedef struct struct_message_to_master {
    int id;
    bool motor1;
    bool motor2;
    int level;
    float lPm;
    float totalL;
} messagetoMaster;

messagetoMaster outgoingMotorReading;

typedef struct struct_message_from_master {
    int id;
    bool motor1;
    bool motor2;
} messageFromMaster;

// Create a struct_message called DHTReadings to hold sensor readings
messageFromMaster incomingMasterReading;


// Create a struct_message to hold

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

// Callback when data is received
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  //memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  //8 byte length for tank 
  //8 bytes for master to motor
  //20 bytes for motor
 if(*incomingData==3)
 {
   memcpy(&incomingMasterReading, incomingData, sizeof(incomingMasterReading));
   Serial.println(incomingMasterReading.id);
   Serial.println(incomingMasterReading.motor1);
   Serial.println(incomingMasterReading.motor2);
 }
 if(*incomingData==1)
  {
   memcpy(&incomingTankReading, incomingData, sizeof(incomingTankReading));
   Serial.println(incomingTankReading.id);
   Serial.println(incomingTankReading.level);
   
 }

}

void setup() {
  // Init Serial Monitor
  Serial.begin(9600);
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Set ESP-NOW Role
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  esp_now_add_peer(master_MAC, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  esp_now_add_peer(tank_MAC, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
}
 
void loop() {
  if ((millis() - lastTime) > timerDelay) {
    // save the last time you updated the DHT values
    outgoingMotorReading.motor1=true;
    outgoingMotorReading.motor2=false;
    outgoingMotorReading.id=2;
    outgoingMotorReading.level=88;
    outgoingMotorReading.lPm=10.1;
    outgoingMotorReading.totalL=101.0;    
    esp_now_send(master_MAC, (uint8_t *) &outgoingMotorReading, sizeof(outgoingMotorReading));
    lastTime = millis();
  }
}