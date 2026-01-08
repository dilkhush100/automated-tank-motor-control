#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "images.h" // Ensure this file exists with your bitmap

// ==========================================
//       USER CONFIGURATION SECTION
// ==========================================
struct Config {
  // Pin Definitions
  const uint8_t PIN_FLOW_SENSOR = D5;
  const uint8_t PIN_RELAY       = D6;
  
  // Flow Sensor Settings
  const float   CALIBRATION_FACTOR = 7.5;  // Standard for YF-S201
  const float   MIN_FLOW_THRESHOLD = 1.0;  // Litres/Minute to consider "Flowing"
  const uint8_t NOISE_FILTER_MS    = 2;    // Ignore pulses faster than 2ms (Debounce)
  
  // Timers (Milliseconds)
  const unsigned long MOTOR_START_DELAY = 3000; // 30 Seconds required flow
  const unsigned long CONNECTION_TIMEOUT = 3000; // 3 Seconds to wait for heartbeat
  
  // Tank Logic
  const int TANK_LEVEL_STOP = 91; // Stop if level is > 90%
} config;

// ==========================================
//          SYSTEM VARIABLES
// ==========================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Data Packet Structure
typedef struct struct_message {
    int id;
    float level;
} struct_message;
struct_message remoteData;

// Flow Calculation Globals
volatile unsigned long pulseCount = 0;
volatile unsigned long lastPulseTime = 0;
float currentFlowRate = 0.0;
float totalVolume = 0.0;

// System State Globals
unsigned long lastFlowCalcTime = 0;
unsigned long lastPacketTime = 0;
unsigned long flowStabilityTimer = 0; // Tracks how long flow has been stable
bool isRemoteConnected = false;
bool isFlowStable = false;            // True if flow > threshold for 30s

// ==========================================
//       INTERRUPT SERVICE ROUTINE
// ==========================================
void ICACHE_RAM_ATTR onPulse() {
  unsigned long now = millis();
  // Hardware Debounce: Ignore noise spikes faster than 2ms
  if (now - lastPulseTime > config.NOISE_FILTER_MS) {
    pulseCount++;
    lastPulseTime = now;
  }
}

// ==========================================
//        ESP-NOW CALLBACK
// ==========================================
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  if (len == sizeof(remoteData)) {
    memcpy(&remoteData, incomingData, sizeof(remoteData));
    lastPacketTime = millis(); // Reset heartbeat timer
  }
}

// ==========================================
//           CORE LOGIC FUNCTIONS
// ==========================================

void updateFlowRate() {
  // Disable interrupt briefly to read variable safely
  detachInterrupt(digitalPinToInterrupt(config.PIN_FLOW_SENSOR));
  unsigned long pulses = pulseCount;
  pulseCount = 0;
  attachInterrupt(digitalPinToInterrupt(config.PIN_FLOW_SENSOR), onPulse, FALLING);

  unsigned long timeDiff = millis() - lastFlowCalcTime;
  if (timeDiff > 0) {
    // formula: (PulseFreq / CalibrationFactor) = L/min
    float freq = (1000.0 / timeDiff) * pulses;
    currentFlowRate = freq / config.CALIBRATION_FACTOR;
    
    // Noise floor clamp
    if (pulses < 2) currentFlowRate = 0.0;
    
    // Integrate total volume: (L/min) / 60 = L/sec
    totalVolume += (currentFlowRate / 60.0) * (timeDiff / 1000.0);
  }
  lastFlowCalcTime = millis();
}

void manageMotor() {
  unsigned long now = millis();

  // 1. Check Connection Heartbeat
  if (now - lastPacketTime < config.CONNECTION_TIMEOUT && lastPacketTime != 0) {
    isRemoteConnected = true;
  } else {
    isRemoteConnected = false;
  }

  // 2. Check Flow Stability Timer
  if (currentFlowRate > config.MIN_FLOW_THRESHOLD) {
    if (flowStabilityTimer == 0) flowStabilityTimer = now; // Start timer
    
    if (now - flowStabilityTimer >= config.MOTOR_START_DELAY) {
      isFlowStable = true;
    }
  } else {
    flowStabilityTimer = 0; // Reset timer if flow stops
    isFlowStable = false;
  }

  // 3. MASTER DECISION MATRIX
  // Motor ON only if: Connected AND Level Safe AND Flow Stable
  bool levelSafe = (remoteData.level <= config.TANK_LEVEL_STOP);
  
  if (isRemoteConnected && levelSafe && isFlowStable) {
    digitalWrite(config.PIN_RELAY, HIGH);
  } else {
    digitalWrite(config.PIN_RELAY, LOW);
  }
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // --- CASE 1: OFFLINE ---
  if (!isRemoteConnected) {
    display.setTextSize(2);
    display.setCursor(20, 25);
    display.print("OFFLINE");
    display.setTextSize(1);
    display.setCursor(15, 45);
    display.print("Check Remote Node");
    display.display();
    return;
  }

  // --- CASE 2: NORMAL OPERATION ---
  
  // TOP BAR: Level and Status
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("L:"); display.print((int)remoteData.level); display.print("%");
  
  // Status Indicator logic
  display.setCursor(80, 0);
  if (remoteData.level > config.TANK_LEVEL_STOP) {
    display.print("FULL");
  } else if (digitalRead(config.PIN_RELAY)) {
    display.print(" ON ");
  } else {
    display.print("OFF ");
  }

  // MIDDLE: Flow Rate (The Hero Metric)
  display.setTextSize(3);
  display.setCursor(0, 20);
  display.print(currentFlowRate, 1);
  display.setTextSize(1);
  display.print(" L/m");

  // BOTTOM: Total Volume
  display.setTextSize(2);
  display.setCursor(0, 50);
  display.print("SUM:"); display.print((int)totalVolume); display.print("L");

  display.display();
}

// ==========================================
//              SETUP & LOOP
// ==========================================
void setup() {
  Serial.begin(115200);

  // Hardware Init
  pinMode(config.PIN_FLOW_SENSOR, INPUT_PULLUP);
  pinMode(config.PIN_RELAY, OUTPUT);
  digitalWrite(config.PIN_RELAY, LOW); // Fail-safe OFF

  // Display Init
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for(;;); // Halt if display fails
  }
  display.clearDisplay();
  display.drawBitmap(0, 0, my_image, 128, 64, SSD1306_WHITE);
  display.display();
  delay(2000);

  // WiFi Init
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  if (esp_now_init() != 0) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("ESP-NOW FAIL");
    display.display();
    return;
  }
  
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);

  // Start Interrupts
  attachInterrupt(digitalPinToInterrupt(config.PIN_FLOW_SENSOR), onPulse, FALLING);
}

void loop() {
  // Non-blocking 1-second interval
  if (millis() - lastFlowCalcTime > 1000) {
    updateFlowRate();
    manageMotor();
    updateDisplay();
    
    // Debug info
    Serial.printf("Flow: %.1f L/m | Lvl: %d | Motor: %d\n", 
                  currentFlowRate, remoteData.level, digitalRead(config.PIN_RELAY));
  }
}