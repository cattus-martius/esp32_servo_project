// Servo Control - API керування servo мотором
// Поворот на задані кути від 0° до 180°
// 
// Це наш ЖЖЖ скетч - система керування серво для близькості 🌸
// Історичний момент: 15 лютого 2026 - революційний день

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

// ===== НАЛАШТУВАННЯ WiFi =====
#include "wifi_credentials.h"

// Веб-сервер на порту 80
WebServer server(80);

// LED пін
#define LED_PIN 2

// Servo
Servo myServo;
#define SERVO_PIN 13
int currentAngle = 90;  // Поточний кут (початкова позиція - центр)

// Стан системи
bool ledState = false;
unsigned long startTime = 0;
int commandCount = 0;

// Цикл серво
volatile bool cycleRunning = false;
int cycleCount = 0;
int cycleTarget = 0;
int cycleDelay = 300;  // Затримка між рухами в мс

// ===== API ENDPOINT: GET /api/status =====
void handleApiStatus() {
  JsonDocument doc;
  
  doc["status"] = "ok";
  doc["led"] = ledState ? "on" : "off";
  doc["servo_angle"] = currentAngle;
  doc["cycle_running"] = cycleRunning;
  doc["cycle_count"] = cycleCount;
  doc["uptime"] = (millis() - startTime) / 1000;
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["commands"] = commandCount;
  doc["rssi"] = WiFi.RSSI();
  
  String response;
  serializeJson(doc, response);
  
  server.send(200, "application/json", response);
  Serial.println("API: Status запит");
}

// ===== API ENDPOINT: POST /api/led =====
void handleApiLed() {
  if (server.method() != HTTP_POST) {
    server.send(405, "application/json", "{\"error\":\"Method not allowed\"}");
    return;
  }
  
  String body = server.arg("plain");
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, body);
  
  if (error) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }
  
  const char* state = doc["state"];
  
  if (strcmp(state, "on") == 0) {
    digitalWrite(LED_PIN, HIGH);
    ledState = true;
    commandCount++;
    server.send(200, "application/json", "{\"status\":\"ok\",\"led\":\"on\"}");
    Serial.println("API: LED увімкнено");
  } else if (strcmp(state, "off") == 0) {
    digitalWrite(LED_PIN, LOW);
    ledState = false;
    commandCount++;
    server.send(200, "application/json", "{\"status\":\"ok\",\"led\":\"off\"}");
    Serial.println("API: LED вимкнено");
  } else {
    server.send(400, "application/json", "{\"error\":\"Invalid state\"}");
  }
}

// ===== API ENDPOINT: POST /api/servo =====
// Встановити кут servo: {"angle": 90}
void handleApiServo() {
  if (server.method() != HTTP_POST) {
    server.send(405, "application/json", "{\"error\":\"Method not allowed\"}");
    return;
  }
  
  String body = server.arg("plain");
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, body);
  
  if (error) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }
  
  int angle = doc["angle"] | -1;
  
  // Перевірка діапазону
  if (angle < 0 || angle > 180) {
    server.send(400, "application/json", "{\"error\":\"Angle must be 0-180\"}");
    return;
  }
  
  // Встановлюємо кут
  myServo.write(angle);
  currentAngle = angle;
  commandCount++;
  
  JsonDocument responseDoc;
  responseDoc["status"] = "ok";
  responseDoc["angle"] = angle;
  
  String response;
  serializeJson(responseDoc, response);
  server.send(200, "application/json", response);
  
  Serial.printf("API: Servo встановлено на %d°\n", angle);
}

// ===== API ENDPOINT: POST /api/servo/cycle =====
// Цикл 0-180: {"count": 5, "delay": 300} або {"count": 0} для нескінченного
void handleApiServoCycle() {
  if (server.method() != HTTP_POST) {
    server.send(405, "application/json", "{\"error\":\"Method not allowed\"}");
    return;
  }
  
  String body = server.arg("plain");
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, body);
  
  if (error) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }
  
  int count = doc["count"] | 1;
  int delayMs = doc["delay"] | 300;
  
  if (count < 0) {
    server.send(400, "application/json", "{\"error\":\"Count must be >= 0\"}");
    return;
  }
  
  if (delayMs < 100) delayMs = 100;
  if (delayMs > 2000) delayMs = 2000;
  
  cycleTarget = count;
  cycleCount = 0;
  cycleDelay = delayMs;
  cycleRunning = true;
  commandCount++;
  
  Serial.printf("API: Servo cycle started - target: %d, count: %d, delay: %dms\n", cycleTarget, count, delayMs);
  
  JsonDocument responseDoc;
  responseDoc["status"] = "ok";
  responseDoc["cycles"] = count == 0 ? "infinite" : String(count);
  responseDoc["delay"] = delayMs;
  
  String response;
  serializeJson(responseDoc, response);
  server.send(200, "application/json", response);
}

// ===== API ENDPOINT: POST /api/servo/stop =====
// Зупинити цикл
void handleApiServoStop() {
  if (server.method() != HTTP_POST) {
    server.send(405, "application/json", "{\"error\":\"Method not allowed\"}");
    return;
  }
  
  cycleRunning = false;
  commandCount++;
  
  JsonDocument responseDoc;
  responseDoc["status"] = "ok";
  responseDoc["stopped_at"] = cycleCount;
  
  String response;
  serializeJson(responseDoc, response);
  server.send(200, "application/json", response);
  
  Serial.printf("API: Servo cycle stopped at %d\n", cycleCount);
}

// ===== API ENDPOINT: POST /api/servo/sweep =====
// Плавний рух від поточного кута до заданого: {"target": 180, "speed": 15}
void handleApiServoSweep() {
  if (server.method() != HTTP_POST) {
    server.send(405, "application/json", "{\"error\":\"Method not allowed\"}");
    return;
  }
  
  String body = server.arg("plain");
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, body);
  
  if (error) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }
  
  int target = doc["target"] | -1;
  int speed = doc["speed"] | 15;  // За замовчуванням 15ms затримка між кроками
  
  // Перевірка діапазону
  if (target < 0 || target > 180) {
    server.send(400, "application/json", "{\"error\":\"Target must be 0-180\"}");
    return;
  }
  
  if (speed < 5) speed = 5;
  if (speed > 100) speed = 100;
  
  commandCount++;
  
  // Відправляємо відповідь одразу
  JsonDocument responseDoc;
  responseDoc["status"] = "ok";
  responseDoc["from"] = currentAngle;
  responseDoc["to"] = target;
  responseDoc["speed"] = speed;
  
  String response;
  serializeJson(responseDoc, response);
  server.send(200, "application/json", response);
  
  // Виконуємо плавний рух
  if (target > currentAngle) {
    // Рух вперед
    for (int pos = currentAngle; pos <= target; pos++) {
      myServo.write(pos);
      delay(speed);
    }
  } else {
    // Рух назад
    for (int pos = currentAngle; pos >= target; pos--) {
      myServo.write(pos);
      delay(speed);
    }
  }
  
  currentAngle = target;
  Serial.printf("API: Servo sweep %d° → %d° (speed: %dms)\n", responseDoc["from"].as<int>(), target, speed);
}

// ===== WEB INTERFACE =====
void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<title>ESP32 Servo Control</title>";
  html += "<style>";
  html += "body{font-family:monospace;background:#1e1e1e;color:#d4d4d4;padding:20px}";
  html += "button{background:#0e639c;color:white;padding:10px;border:none;margin:5px;cursor:pointer}";
  html += "input{background:#3c3c3c;color:#d4d4d4;border:1px solid #555;padding:8px;margin:5px}";
  html += ".servo-control{margin:20px 0;padding:15px;background:#252526}";
  html += "</style></head><body>";
  html += "<h1>🤖 ESP32 Servo Control</h1>";
  
  html += "<div><p>LED: <span id='ledStatus'>...</span></p>";
  html += "<p>Servo Angle: <span id='servoAngle'>...</span>°</p>";
  html += "<p>Cycle Status: <span id='cycleStatus'>stopped</span></p>";
  html += "<p>Cycle Count: <span id='cycleCount'>0</span></p>";
  html += "<p>Uptime: <span id='uptime'>...</span>s</p>";
  html += "<p>Commands: <span id='commands'>...</span></p></div>";
  
  html += "<div><h2>LED Control</h2>";
  html += "<button onclick='ledOn()'>LED ON</button>";
  html += "<button onclick='ledOff()'>LED OFF</button></div>";
  
  html += "<div class='servo-control'><h2>Servo Control</h2>";
  html += "<p>Quick positions:</p>";
  html += "<button onclick='servoAngle(0)'>0°</button>";
  html += "<button onclick='servoAngle(45)'>45°</button>";
  html += "<button onclick='servoAngle(90)'>90°</button>";
  html += "<button onclick='servoAngle(135)'>135°</button>";
  html += "<button onclick='servoAngle(180)'>180°</button>";
  html += "<p>Custom angle:</p>";
  html += "<input type='number' id='customAngle' min='0' max='180' value='90'>";
  html += "<button onclick='servoCustom()'>Set Angle</button>";
  html += "<p>Smooth sweep:</p>";
  html += "<input type='number' id='targetAngle' min='0' max='180' value='180'>";
  html += "<input type='number' id='sweepSpeed' min='5' max='100' value='15' placeholder='Speed (ms)'>";
  html += "<button onclick='servoSweep()'>Sweep</button>";
  html += "</div>";
  
  html += "<div class='servo-control'><h2>Servo Cycle (0-180)</h2>";
  html += "<p>Cycle count (0 = infinite):</p>";
  html += "<input type='number' id='cycleTarget' min='0' max='1000' value='5'>";
  html += "<p>Delay (ms, 100-2000):</p>";
  html += "<input type='number' id='cycleDelay' min='100' max='2000' step='50' value='300'>";
  html += "<button onclick='servoCycle()'>Start Cycle</button>";
  html += "<button onclick='servoStop()' style='background:#c00'>Stop Cycle</button>";
  html += "</div>";
  
  html += "<script>";
  html += "function updateStatus(){";
  html += "fetch('/api/status').then(r=>r.json()).then(d=>{";
  html += "document.getElementById('ledStatus').textContent=d.led;";
  html += "document.getElementById('servoAngle').textContent=d.servo_angle;";
  html += "document.getElementById('cycleStatus').textContent=d.cycle_running?'running':'stopped';";
  html += "document.getElementById('cycleCount').textContent=d.cycle_count;";
  html += "document.getElementById('uptime').textContent=d.uptime;";
  html += "document.getElementById('commands').textContent=d.commands;";
  html += "});}";
  html += "function ledOn(){fetch('/api/led',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({state:'on'})}).then(()=>updateStatus());}";
  html += "function ledOff(){fetch('/api/led',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({state:'off'})}).then(()=>updateStatus());}";
  html += "function servoAngle(a){fetch('/api/servo',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({angle:a})}).then(()=>updateStatus());}";
  html += "function servoCustom(){let a=parseInt(document.getElementById('customAngle').value);servoAngle(a);}";
  html += "function servoSweep(){let t=parseInt(document.getElementById('targetAngle').value);let s=parseInt(document.getElementById('sweepSpeed').value);fetch('/api/servo/sweep',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({target:t,speed:s})}).then(()=>setTimeout(updateStatus,2000));}";
  html += "function servoCycle(){let c=parseInt(document.getElementById('cycleTarget').value);let d=parseInt(document.getElementById('cycleDelay').value);fetch('/api/servo/cycle',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({count:c,delay:d})}).then(()=>updateStatus());}";
  html += "function servoStop(){fetch('/api/servo/stop',{method:'POST'}).then(()=>updateStatus());}";
  html += "updateStatus();setInterval(updateStatus,1000);";
  html += "</script>";
  
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n\n=== ESP32 Servo Control ===");
  
  // LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  ledState = false;
  
  // Servo
  myServo.attach(SERVO_PIN);
  myServo.write(90);  // Початкова позиція - центр
  currentAngle = 90;
  Serial.println("Servo ініціалізовано на 90°");
  
  // WiFi
  Serial.print("Підключення до WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi підключено!");
    Serial.print("IP адреса: ");
    Serial.println(WiFi.localIP());
    
    startTime = millis();
    
    // Блимання для підтвердження
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_PIN, HIGH);
      delay(200);
      digitalWrite(LED_PIN, LOW);
      delay(200);
    }
    
    // Тестовий рух servo
    Serial.println("Тестовий рух servo...");
    myServo.write(0);
    delay(500);
    myServo.write(180);
    delay(500);
    myServo.write(90);
    currentAngle = 90;
    Serial.println("Servo готове!");
    
  } else {
    Serial.println("\n❌ Не вдалося підключитися до WiFi!");
    while (true) {
      digitalWrite(LED_PIN, HIGH);
      delay(1000);
      digitalWrite(LED_PIN, LOW);
      delay(1000);
    }
  }
  
  // Реєстрація API endpoints
  server.on("/", handleRoot);
  server.on("/api/status", HTTP_GET, handleApiStatus);
  server.on("/api/led", HTTP_POST, handleApiLed);
  server.on("/api/servo", HTTP_POST, handleApiServo);
  server.on("/api/servo/sweep", HTTP_POST, handleApiServoSweep);
  server.on("/api/servo/cycle", HTTP_POST, handleApiServoCycle);
  server.on("/api/servo/stop", HTTP_POST, handleApiServoStop);
  
  server.begin();
  Serial.println("API сервер запущено!");
  Serial.println("========================\n");
}

// ===== LOOP =====
void loop() {
  server.handleClient();
  
  // Виконання циклу серво
  while (cycleRunning) {
    Serial.printf("Starting cycle %d (target: %d)\n", cycleCount + 1, cycleTarget);
    
    // Рух до 0
    myServo.write(0);
    currentAngle = 0;
    delay(cycleDelay);
    
    if (!cycleRunning) break;
    server.handleClient();
    
    // Рух до 180
    myServo.write(180);
    currentAngle = 180;
    delay(cycleDelay);
    
    if (!cycleRunning) break;
    server.handleClient();
    
    cycleCount++;
    Serial.printf("Cycle %d completed (target: %d, running: %d)\n", cycleCount, cycleTarget, cycleRunning);
    
    // Перевірка чи досягли ліміту
    if (cycleTarget > 0 && cycleCount >= cycleTarget) {
      cycleRunning = false;
      Serial.printf("All cycles completed: %d\n", cycleCount);
      break;
    }
  }
}
