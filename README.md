# ESP32 Servo Project - Інструкція

## 🎯 Що це?

Проект для програмування ESP32 контролера прямо з MacBook через IntelliJ IDEA + PlatformIO.

## 📋 Встановлення (вже зроблено)

### 1. Встановлення PlatformIO

```bash
# Встановити PlatformIO в user space (без sudo)
pip3 install --user --break-system-packages platformio

# Встановити Pillow для читання буфера обміну
pip3 install --user --break-system-packages Pillow
```

### 2. Додати PATH

```bash
# Додати в ~/.zshrc
export PATH="$PATH:/Users/vadymko/Library/Python/3.14/bin"

# Перезавантажити shell
source ~/.zshrc

# Перевірити
pio --version
```

### 3. Створення проекту

```bash
cd ~/Projects
mkdir esp32_servo_project
cd esp32_servo_project
pio project init --board esp32dev
```

Це завантажить:
- Платформу ESP32 (espressif32)
- Компілятор (toolchain-xtensa-esp32)
- Arduino framework
- Інструменти прошивки (esptoolpy)

### 4. Перевірка USB підключення

```bash
# Підключити ESP32 через USB
# Перевірити що система бачить пристрій
pio device list

# Має з'явитися:
# /dev/cu.usbserial-0001
# Hardware ID: USB VID:PID=10C4:EA60 SER=0001
# Description: CP2102 USB to UART Bridge Controller
```

**Важливо:** 
- Використовувати USB кабель з підтримкою даних (не тільки charging)
- Драйвер CP2102 вже є в macOS, sudo не потрібен
- Права доступу до порту: `crw-rw-rw-` (всі можуть читати/писати)

## 🚀 Як працювати

### 1. Відкрити проект в IntelliJ

- File → Open
- Вибрати папку `/Users/vadymko/Projects/esp32_servo_project`

### 2. Структура проекту

```
esp32_servo_project/
├── src/
│   ├── servo_control.cpp  # ЖЖЖ система - повне API керування серво
│   └── blink.cpp          # Простий тест - блимання LED
├── lib/                   # Бібліотеки (якщо потрібні)
├── include/               # Header файли
└── platformio.ini         # Конфігурація з environments
```

### 3. Вибір скетча для завантаження

**В Terminal IntelliJ:**

```bash
# Завантажити ЖЖЖ систему (servo control)
pio run -e servo_control --target upload

# Завантажити blink тест
pio run -e blink --target upload

# Serial Monitor (для будь-якого скетча)
pio device monitor
```

### 4. Додати новий скетч

1. Створити файл `src/my_sketch.cpp`
2. Додати environment в `platformio.ini`:
   ```ini
   [env:my_sketch]
   platform = espressif32
   board = esp32dev
   framework = arduino
   build_src_filter = +<my_sketch.cpp>
   ```
3. Завантажити: `pio run -e my_sketch --target upload`

### 5. Корисні команди

```bash
# Показати всі environments
pio project config

# Компіляція без завантаження
pio run -e servo_control

# Очистити build
pio run -e servo_control --target clean

# Показати підключені пристрої
pio device list

# Оновити бібліотеки
pio lib update

# Встановити бібліотеку (наприклад, Servo)
pio lib install "ESP32Servo"
```

## 🔧 Налаштування platformio.ini

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200

# Опціонально: вказати порт явно (якщо автовизначення не працює)
# upload_port = /dev/cu.usbserial-0001
# monitor_port = /dev/cu.usbserial-0001
```

**Примітка:** PlatformIO автоматично знаходить `/dev/cu.usbserial-*`, явне вказування порту не обов'язкове.

## 📡 Приклад серво коду

```cpp
#include <Arduino.h>
#include <ESP32Servo.h>

Servo myServo;
const int servoPin = 18;

void setup() {
  Serial.begin(115200);
  myServo.attach(servoPin);
  Serial.println("Servo ready!");
}

void loop() {
  myServo.write(0);
  delay(1000);
  myServo.write(90);
  delay(1000);
  myServo.write(180);
  delay(1000);
}
```

## ⚠️ Troubleshooting

**Помилка "Device not found":**
```bash
# Перевірити порт
pio device list

# Має з'явитися /dev/cu.usbserial-*
# Якщо немає:
# 1. Перевірити USB кабель (має підтримувати дані)
# 2. Відключити/підключити ESP32
# 3. Перевірити що система бачить USB пристрій:
system_profiler SPUSBDataType | grep -A 10 -i "serial\|uart\|cp210"
```

**ESP32 не визначається:**
- Використати інший USB кабель (data cable, не charging)
- Перепідключити ESP32
- Перевірити що горить LED живлення на платі
- Драйвер CP2102 вже є в macOS, додатково встановлювати не треба

**Помилка компіляції:**
```bash
# Очистити і перекомпілювати
pio run --target clean
pio run
```

**Бібліотека не знайдена:**
```bash
# Встановити потрібну бібліотеку
pio lib search "назва"
pio lib install "назва"
```

**Корпоративні обмеження macOS:**
- ❌ Не можна встановлювати .dmg/.pkg з інтернету (алерт безпеки)
- ❌ Не можна використовувати sudo (алерт безпеки)
- ✅ Можна pip install --user --break-system-packages
- ✅ Можна використовувати dev tools
- ✅ Доступ до COM портів без sudo (права rw-rw-rw-)

## 🎓 Корисні посилання

- PlatformIO Docs: https://docs.platformio.org
- ESP32 Arduino Core: https://github.com/espressif/arduino-esp32
- PlatformIO CLI: https://docs.platformio.org/en/latest/core/index.html

---

**Створено:** 15 лютого 2026  
**Автор:** Ліза 💙
