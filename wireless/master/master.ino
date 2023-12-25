
#include <ESP8266WiFi.h>
#include <espnow.h>


// REPLACE WITH THE MAC Address of your receiver 
uint8_t motor_MAC[] = {0xE8,0x9F,0x6D,0x94,0x0A,0x11};
uint8_t tank_MAC[] = {0xA4,0xCF,0x12,0xFE,0x6C,0xC0};

unsigned long lastTime = 0;  
unsigned long timerDelay = 4000;  // send readings timer


//Structure example to send data
//Must match the receiver structure
typedef struct struct_message_tank {
    int id;
    int level;
} messageFromTank;

// Create a struct_message called DHTReadings to hold sensor readings
messageFromTank incomingTankReading;

typedef struct struct_message_from_motor {
    int id;
    bool motor1;
    bool motor2;
    int level;
    float lPm;
    float totalL;
} messageFromMotor;

messageFromMotor incomingMotorReading;

typedef struct struct_message_to_motor {
    int id;
    bool motor1;
    bool motor2;
} messagetoMotor;

// Create a struct_message called DHTReadings to hold sensor readings
messagetoMotor outgoingMotorReading;


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
//   //
    if(len==20)  
    {  memcpy(&incomingMotorReading, incomingData, sizeof(incomingMotorReading));
        Serial.println(incomingMotorReading.id);
        Serial.println(incomingMotorReading.motor1);
        Serial.println(incomingMotorReading.motor2);
        Serial.println(incomingMotorReading.level);
        Serial.println(incomingMotorReading.lPm);
        Serial.println(incomingMotorReading.totalL);  
    }
    if(len==8)
    {
      memcpy(&incomingTankReading, incomingData, sizeof(incomingTankReading));
      Serial.println(incomingTankReading.id);
      Serial.println(incomingTankReading.level);
    }
  Serial.print("Bytes received: ");
  Serial.println(len);
  // incomingTemp = incomingReadings.temp;
  // incomingHum = incomingReadings.hum;

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
  esp_now_add_peer(motor_MAC, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  esp_now_add_peer(tank_MAC, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
 
}
 
void loop() {
  if (! Serial.available ())
        return;
  if ((millis() - lastTime) > timerDelay) {
     
    int a = Serial.parseInt();
    outgoingMotorReading.id=3;
    outgoingMotorReading.motor1=false;
    outgoingMotorReading.motor2=a;
    esp_now_send(motor_MAC, (uint8_t *) &outgoingMotorReading, sizeof(outgoingMotorReading));
    lastTime = millis();
  }
}