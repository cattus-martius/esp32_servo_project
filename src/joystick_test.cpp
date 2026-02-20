// Тестування джойстика - перевірка розпіновки
// Показує значення VRx, VRy, SW в Serial Monitor

#include <Arduino.h>

// Попередні значення для відстеження змін
int lastVrx = -1;
int lastVry = -1;
bool lastSw = true;

// Deadzone для аналогових входів
const int DEADZONE = 20;

// Дебаунсинг для кнопки
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;

#define VRX_PIN 35  // GPIO35 (зелений дріт)
#define VRY_PIN 32  // GPIO32 (жовтий дріт)
#define SW_PIN  34  // GPIO34 (оранжевий дріт)

void setup() {
  Serial.begin(115200);
  
  // Налаштування пінів
  pinMode(SW_PIN, INPUT_PULLUP);
  
  Serial.println("\n=== JOYSTICK TEST ===");
  Serial.println("VRx (зелений): D35 (GPIO35)");
  Serial.println("VRy (жовтий):  D32 (GPIO32)");
  Serial.println("SW (оранжевий): D34 (GPIO34)");
  Serial.println("====================\n");
  
  delay(1000);
}

void loop() {
  // Читання аналогових значень (0-4095)
  int vrx = analogRead(VRX_PIN);
  int vry = analogRead(VRY_PIN);
  
  // Читання кнопки з простою фільтрацією
  bool sw1 = digitalRead(SW_PIN);
  delay(5);
  bool sw2 = digitalRead(SW_PIN);
  bool sw = (sw1 == sw2) ? sw1 : lastSw;  // Якщо не співпадають - беремо попереднє
  
  // Перевірка змін з deadzone для аналогових
  bool vrxChanged = abs(vrx - lastVrx) > DEADZONE;
  bool vryChanged = abs(vry - lastVry) > DEADZONE;
  bool swChanged = sw != lastSw;
  
  // Логувати тільки якщо щось змінилося
  if (vrxChanged || vryChanged || swChanged) {
    Serial.print("VRx: ");
    Serial.print(vrx);
    Serial.print("\t VRy: ");
    Serial.print(vry);
    Serial.print("\t SW: ");
    Serial.println(sw ? "Released" : "PRESSED");
    
    if (vrxChanged) lastVrx = vrx;
    if (vryChanged) lastVry = vry;
    if (swChanged) lastSw = sw;
  }
  
  delay(10);
}
