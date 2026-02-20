# Webcam Platform Control

Dual-axis servo platform for precise webcam positioning with manual and automatic control modes.

## Features

- **Manual Positioning**: Precise angle control (0-180°) via joystick or web interface
- **Auto Scan Mode**: Automatic platform scanning with adjustable speed
- **Web Interface**: Remote control via browser
- **LED Indicators**: Visual feedback for current mode

## Hardware Setup

### Components
- ESP32 DevKit
- SG90 Servo Motor
- KY-023 Joystick Module
- Built-in LED (GPIO 2)

### Wiring
- Servo Signal → GPIO 25
- Joystick VRx → GPIO 35 (speed control in auto mode)
- Joystick VRy → GPIO 32 (manual positioning)
- Joystick SW → GPIO 33 (mode switch)
- LED → GPIO 2 (built-in)

## Operating Modes

### 1. Standby Mode
- **LED**: Off
- **Joystick**: Inactive
- **Entry**: Power on or exit from Manual Pan mode
- **Exit**: Double-click button to enter Auto Scan

### 2. Auto Scan Mode
- **LED**: Blinking (500ms interval)
- **Function**: Platform automatically scans 0-180° range
- **Speed Control**: Use VRx axis to adjust scan speed (100-500ms)
- **Entry**: Double-click button from Standby
- **Exit**: Single-click to enter Manual Pan

### 3. Manual Pan Mode
- **LED**: Solid on
- **Function**: Direct angle control via VRy axis
- **Control**: Move joystick Y-axis to position platform
- **Behavior**: Release joystick - platform holds position
- **Entry**: Single-click from Auto Scan
- **Exit**: Single-click to return to Standby (platform returns to 90° center)

## Button Controls

- **Double-click** (< 500ms between clicks): Standby → Auto Scan
- **Single-click**: 
  - Auto Scan → Manual Pan
  - Manual Pan → Standby (returns to center)

## Web Interface

Access via browser: `http://[ESP32_IP]`

### Manual Positioning
- Angle slider (0-180°)
- +/- buttons for fine adjustment
- "Set Position" button to apply

### Auto Scan Mode
- Speed slider (100-500ms)
- +/- buttons for speed adjustment
- "Start Scan" button
- "Stop" button (returns to standby)

### Status Display
- Current mode
- Platform angle
- Scan speed

## API Endpoints

### GET /api/status
Returns current system status:
```json
{
  "status": "ok",
  "mode": "standby|auto|manual",
  "angle": 90,
  "scan_speed": 300,
  "uptime": 1234,
  "rssi": -45
}
```

### POST /api/angle
Set platform angle:
```json
{
  "angle": 90
}
```

### POST /api/scan
Start auto scan mode:
```json
{
  "speed": 300
}
```

### POST /api/stop
Stop all operations and return to standby.

## Configuration

WiFi credentials are stored in `include/wifi_credentials.h`:
```cpp
const char* WIFI_SSID = "your_ssid";
const char* WIFI_PASSWORD = "your_password";
```

## Use Cases

- **Webcam positioning**: Precise camera angle adjustment
- **Surveillance**: Automatic area scanning
- **Time-lapse photography**: Smooth panning shots
- **Video conferencing**: Quick position presets

## Technical Specifications

- **Angle Range**: 0-180°
- **Scan Speed**: 100-500ms per step
- **Position Accuracy**: ±2°
- **Response Time**: < 50ms
- **WiFi**: 2.4GHz 802.11 b/g/n

## Safety Features

- Automatic center return on mode exit
- Smooth movement transitions
- Deadzone in joystick center (prevents drift)
- Debounced button inputs

## Troubleshooting

**Platform doesn't move:**
- Check servo power supply
- Verify GPIO 25 connection
- Check Serial Monitor for errors

**Joystick not responding:**
- Verify mode (LED indicator)
- Check joystick wiring
- Ensure 3.3V power supply

**Web interface not accessible:**
- Check WiFi connection
- Verify IP address in Serial Monitor
- Check router firewall settings

## Development

Built with:
- PlatformIO
- Arduino Framework for ESP32
- ESP32Servo library
- ArduinoJson 7.x

Compile and upload:
```bash
pio run -e webcam_platform -t upload
```

Monitor serial output:
```bash
pio device monitor -e webcam_platform
```
