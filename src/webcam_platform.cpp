// Webcam Platform Control - Phase 8
// Dual-axis servo platform for webcam positioning
// Manual control via joystick + web interface

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include "wifi_credentials.h"

// Web server
WebServer server(80);

// Servo
Servo platformServo;
#define SERVO_PIN 25
int currentAngle = 90;

// Joystick
#define VRX_PIN 35    // X-axis (speed control in auto mode)
#define VRY_PIN 32    // Y-axis (manual positioning)
#define SW_PIN  33    // Mode switch button

// LED indicator
#define LED_PIN 2

// System modes
enum Mode {
  STANDBY,      // LED off, joystick inactive
  AUTO_SCAN,    // LED blinking, automatic scanning
  MANUAL_PAN    // LED on, manual positioning via VRy
};

Mode currentMode = STANDBY;

// Auto scan parameters
int scanSpeed = 300;      // Delay between movements (100-500ms)
const int MIN_SPEED = 100;
const int MAX_SPEED = 500;
const int SPEED_STEP = 50;

// Manual pan parameters
int targetAngle = 90;
const int ANGLE_DEADZONE = 50;

// Button handling
bool lastSwState = HIGH;
unsigned long lastClickTime = 0;
int clickCount = 0;
const unsigned long DOUBLE_CLICK_TIMEOUT = 500;

// Auto scan state
bool isScanning = false;
unsigned long lastScanMove = 0;
bool scanDirection = true;  // true = forward, false = backward

// LED blinking
unsigned long lastLedToggle = 0;
bool ledState = false;

// Statistics
unsigned long startTime = 0;
int commandCount = 0;

// ===== API ENDPOINTS =====

void handleApiStatus() {
  JsonDocument doc;
  
  doc["status"] = "ok";
  doc["mode"] = currentMode == STANDBY ? "standby" : (currentMode == AUTO_SCAN ? "auto" : "manual");
  doc["angle"] = currentAngle;
  doc["scan_speed"] = scanSpeed;
  doc["uptime"] = (millis() - startTime) / 1000;
  doc["rssi"] = WiFi.RSSI();
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleApiSetAngle() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }
  
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));
  
  if (error) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }
  
  int angle = doc["angle"] | 90;
  angle = constrain(angle, 0, 180);
  
  platformServo.write(angle);
  currentAngle = angle;
  
  commandCount++;
  
  JsonDocument response;
  response["status"] = "ok";
  response["angle"] = currentAngle;
  
  String responseStr;
  serializeJson(response, responseStr);
  server.send(200, "application/json", responseStr);
}

void handleApiScan() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }
  
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));
  
  if (error) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }
  
  int speed = doc["speed"] | 300;
  scanSpeed = constrain(speed, MIN_SPEED, MAX_SPEED);
  
  currentMode = AUTO_SCAN;
  isScanning = true;
  
  commandCount++;
  
  JsonDocument response;
  response["status"] = "scanning";
  response["speed"] = scanSpeed;
  
  String responseStr;
  serializeJson(response, responseStr);
  server.send(200, "application/json", responseStr);
}

void handleApiStop() {
  currentMode = STANDBY;
  isScanning = false;
  platformServo.write(90);
  currentAngle = 90;
  digitalWrite(LED_PIN, LOW);
  
  server.send(200, "application/json", "{\"status\":\"standby\"}");
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'><title>Webcam Platform</title><style>body{font-family:Arial;max-width:600px;margin:50px auto;padding:20px;background:#1a1a1a;color:#e0e0e0}button{padding:15px 30px;margin:10px;font-size:18px;cursor:pointer;border:none;border-radius:5px}.scan{background:#4CAF50;color:white}.stop{background:#f44336;color:white}.manual{background:#2196F3;color:white}.status{padding:20px;background:#2a2a2a;border-radius:5px;margin:20px 0;border:1px solid #444}.slider-container{display:flex;align-items:center;gap:10px;margin:20px 0}.btn-adjust{background:#555;color:white;border:none;padding:10px 15px;font-size:20px;cursor:pointer;border-radius:5px;width:50px}.btn-adjust:active{background:#777}input[type=range]{flex:1;height:40px}h1{color:#4CAF50}</style></head><body><h1>Webcam Platform Control</h1><div class='status'><p><strong>Mode:</strong> <span id='mode'>-</span></p><p><strong>Angle:</strong> <span id='angle'>-</span>&deg;</p><p><strong>Speed:</strong> <span id='speed'>-</span> ms</p></div><div><h3>Manual Positioning</h3><label>Pan Angle (0-180&deg;): <span id='angleValue'>90</span></label><div class='slider-container'><button class='btn-adjust' onclick='adjustAngle(-10)'>-</button><input type='range' id='angleSlider' min='0' max='180' step='10' value='90' oninput='onAngleChange()'><button class='btn-adjust' onclick='adjustAngle(10)'>+</button></div><button class='manual' onclick='setAngle()'>Set Position</button></div><div><h3>Auto Scan Mode</h3><label>Scan Speed (100-500 ms): <span id='speedValue'>300</span></label><div class='slider-container'><button class='btn-adjust' onclick='adjustSpeed(-50)'>-</button><input type='range' id='speedSlider' min='100' max='500' step='50' value='300' oninput='onSpeedChange()'><button class='btn-adjust' onclick='adjustSpeed(50)'>+</button></div><button class='scan' onclick='startScan()'>Start Scan</button><button class='stop' onclick='stop()'>Stop</button></div><script>const angleSlider=document.getElementById('angleSlider');const angleValue=document.getElementById('angleValue');const speedSlider=document.getElementById('speedSlider');const speedValue=document.getElementById('speedValue');function onAngleChange(){angleValue.textContent=angleSlider.value}function adjustAngle(delta){let newValue=parseInt(angleSlider.value)+delta;newValue=Math.max(0,Math.min(180,newValue));angleSlider.value=newValue;angleValue.textContent=newValue}function onSpeedChange(){speedValue.textContent=speedSlider.value}function adjustSpeed(delta){let newValue=parseInt(speedSlider.value)+delta;newValue=Math.max(100,Math.min(500,newValue));speedSlider.value=newValue;speedValue.textContent=newValue}function setAngle(){const angle=parseInt(angleSlider.value);fetch('/api/angle',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({angle:angle})}).then(()=>updateStatus())}function startScan(){const speed=parseInt(speedSlider.value);fetch('/api/scan',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({speed:speed})}).then(()=>updateStatus())}function stop(){fetch('/api/stop',{method:'POST'}).then(()=>updateStatus())}function updateStatus(){fetch('/api/status').then(r=>r.json()).then(data=>{document.getElementById('mode').textContent=data.mode;document.getElementById('angle').textContent=data.angle;document.getElementById('speed').textContent=data.scan_speed})}setInterval(updateStatus,1000);updateStatus()</script></body></html>";
  
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  startTime = millis();
  
  // Setup servo
  platformServo.attach(SERVO_PIN);
  platformServo.write(90);
  
  // Setup LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Setup joystick
  pinMode(SW_PIN, INPUT_PULLUP);
  
  // Connect to WiFi
  Serial.println("\n=== Webcam Platform Control ===");
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\n✓ WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // Setup API endpoints
  server.on("/", handleRoot);
  server.on("/api/status", handleApiStatus);
  server.on("/api/angle", handleApiSetAngle);
  server.on("/api/scan", handleApiScan);
  server.on("/api/stop", handleApiStop);
  
  server.begin();
  Serial.println("✓ HTTP server started");
  Serial.println("================================\n");
}

void handleButtonClick() {
  bool sw = digitalRead(SW_PIN);
  
  // Detect button press (HIGH -> LOW transition)
  if (sw == LOW && lastSwState == HIGH) {
    // Simple mode cycling: Standby -> Manual Pan -> Auto Scan -> Standby
    if (currentMode == STANDBY) {
      currentMode = MANUAL_PAN;
      Serial.println("Mode: MANUAL_PAN");
    } else if (currentMode == MANUAL_PAN) {
      currentMode = AUTO_SCAN;
      isScanning = true;
      Serial.println("Mode: AUTO_SCAN");
    } else if (currentMode == AUTO_SCAN) {
      currentMode = STANDBY;
      isScanning = false;
      platformServo.write(90);
      currentAngle = 90;
      digitalWrite(LED_PIN, LOW);
      Serial.println("Mode: STANDBY (returned to center)");
    }
    
    delay(300);  // Debounce
  }
  
  lastSwState = sw;
}

void handleAutoScan() {
  if (!isScanning) return;
  
  unsigned long now = millis();
  
  // LED blinking
  if (now - lastLedToggle >= 500) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    lastLedToggle = now;
  }
  
  // Read VRx for speed adjustment
  int vrx = analogRead(VRX_PIN);
  if (vrx < 1800) {
    scanSpeed = max(MIN_SPEED, scanSpeed - SPEED_STEP);
  } else if (vrx > 1900) {
    scanSpeed = min(MAX_SPEED, scanSpeed + SPEED_STEP);
  }
  
  // Servo movement: 0 -> 180 -> 0 -> 180 (like Phase 7)
  if (now - lastScanMove >= scanSpeed) {
    if (scanDirection) {
      platformServo.write(180);
      currentAngle = 180;
      scanDirection = false;
    } else {
      platformServo.write(0);
      currentAngle = 0;
      scanDirection = true;
    }
    lastScanMove = now;
  }
}

void handleManualPan() {
  // LED on solid
  digitalWrite(LED_PIN, HIGH);
  
  // Read VRy for manual positioning
  int vry = analogRead(VRY_PIN);
  
  // Map VRy to direction (with deadzone in center)
  int centerValue = 2048;
  int offset = vry - centerValue;
  const int DEADZONE = 300;  // Deadzone for center position
  
  if (abs(offset) > DEADZONE) {
    // Joystick moved from center - determine direction
    if (offset < 0) {
      // Joystick left - move to 0
      if (currentAngle > 0) {
        currentAngle--;
        platformServo.write(currentAngle);
        delay(15);  // Smooth movement
      }
    } else {
      // Joystick right - move to 180
      if (currentAngle < 180) {
        currentAngle++;
        platformServo.write(currentAngle);
        delay(15);  // Smooth movement
      }
    }
  }
  // If joystick in deadzone - stop (hold current position, no movement)
}

void loop() {
  server.handleClient();
  
  // Handle button clicks
  handleButtonClick();
  
  // Handle current mode
  switch (currentMode) {
    case STANDBY:
      digitalWrite(LED_PIN, LOW);
      break;
      
    case AUTO_SCAN:
      handleAutoScan();
      break;
      
    case MANUAL_PAN:
      handleManualPan();
      break;
  }
  
  delay(10);
}
