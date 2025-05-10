# 🌡️ Arduino Temperature & Humidity Logger with OLED and RGB Feedback

A simple environment monitor built with Arduino. This project reads temperature and humidity data from a sensor, displays it on an OLED screen, and provides visual (RGB LED) and audible (buzzer) alerts based on customizable thresholds.

## 🔧 Features

- Reads room **temperature** and **humidity**
- Logs values to the **Serial Monitor** every few seconds
- Displays real-time **temperature** and **humidity** on an **OLED screen**
- **Buzzer alert** triggers when:
  - Temperature > set threshold (e.g., 30°C)
  - Humidity > set threshold (e.g., 70%)
- Includes a **push button** to:
  - Switch between °C and °F display modes
  - Turn the RGB LED on/off
- Uses an **RGB LED** to provide visual feedback on environmental conditions. Example:
  - 🔴 Red: High temperature
  - 🔵 Blue: Low temperature
  - 🟢 Green: Normal range
  - 🟡 Yellow: Warning zone

## 📦 Components Used

- Arduino Uno (or compatible board)
- DHT11 or DHT22 sensor
- 0.96" I2C OLED display (SSD1306)
- RGB LED (common cathode or anode)
- Buzzer
- Push button
- Breadboard or perfboard

## 🧠 How It Works

### 🌡️ Temperature Handling

- Reads temperature in °C by default (can display in °F)
- Converts temperature for display if needed
- Stores a base temperature every 5 minutes
- Compares new readings to the base to determine the trend:
  - `↑` if increase ≥ 0.5°C
  - `↓` if decrease ≤ -0.5°C
  - `stable` otherwise

### 💡 RGB LED Logic

| Temperature (°C) | LED Color    |
|------------------|--------------|
| < 8              | Icy Blue     |
| 8 - 12           | Cool Blue    |
| 12 - 20          | Light Blue   |
| 20 - 25          | Green        |
| 25 - 33          | Orange       |
| > 33             | Red          |

### 🔊 Buzzer Alert

- Temperature is abnormal if:
  - **Below 5°C**
  - **Above 33°C**
- Buzzer beeps once when temp crosses into abnormal range.

### 🖥️ OLED Display

- Shows temp with 1 decimal, units, and trend
- Humidity with 1 decimal
- Quote (rotates each loop)

## 🧰 Libraries Used

- `DHT` (Adafruit DHT sensor library)
- `U8g2` (for OLED display)

## 🚀 Getting Started

1. Connect all components as per your circuit design.
2. Upload the code using the Arduino IDE.
3. Press the button to toggle display modes or LED feedback.
4. Monitor real-time data on OLED and through the Serial Monitor.

## 🛠️ Future Improvements (Ideas)
- Add real-time clock (RTC) for timestamping logs
- Save readings to an SD card or external storage
- Graphical trends on OLED
- Bluetooth/ESP upgrade to send data to phone
- Display graphs on PC using SerialPlot or a Python script

## 🤝 Contributing
Feel free to fork this repo and submit pull requests for improvements or feature suggestions!

## 📝 License
This project is open-source and available under the MIT License.

