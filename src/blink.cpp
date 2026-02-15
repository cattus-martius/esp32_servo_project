// Blink - простий тестовий скетч
// Блимання вбудованим LED для перевірки що все працює

#include <Arduino.h>

#define LED_PIN 2

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  Serial.println("Blink test started!");
}

void loop() {
  digitalWrite(LED_PIN, HIGH);
  Serial.println("LED ON");
  delay(1000);
  
  digitalWrite(LED_PIN, LOW);
  Serial.println("LED OFF");
  delay(1000);
}
