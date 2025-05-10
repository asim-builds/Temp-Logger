💡 Code Breakdown – Arduino Temperature & Humidity Monitor
🛠️ Initialization and Setup
The pins of HW-479 RGB LED module, buzzer, push button, DHT sensor, and OLED are initialized inside the setup() function.

Push button pins are configured as INPUT_PULLUP, which keeps them at HIGH when unpressed by enabling the internal pull-up resistor.

The RGB LED and buzzer are set as OUTPUT to control their states based on system logic.

OLED display uses I2C protocol, initialized using Wire.begin() and oled.begin() commands.

On boot, the OLED briefly shows a welcome message ("Temp & Humidity Monitor") for 500ms to give the user a visual confirmation that the system is starting.

🌡️ Sensor Data Reading and Conversion
Temperature and humidity values are continuously read from the DHT sensor.

If the user prefers Fahrenheit, temperature is converted using the standard formula:
F = (C × 9/5) + 32.

These values are used both for display and condition checking (e.g., LED colors, buzzer alerts).

🎨 RGB LED Color Control
The updateLED() function maps specific temperature ranges to colors:

Below 8°C: Icy Blue

8–12°C: Cool Blue

12–20°C: Light Blue

20–25°C: Green

25–33°C: Orange

Above 33°C: Red

Temperature is always converted back to Celsius internally for consistent logic, even if display is in Fahrenheit.

🔊 Buzzer Alert for Abnormal Temperatures
The checkTemp() function checks if temperature is abnormally low (<5°C) or too high (≥33°C).

If a transition is detected from normal to abnormal, a brief buzzer beep (100ms) is triggered.

A boolean wasNormal flag is used to avoid repeated alerts until the temperature goes back to normal.

📺 OLED Display Output
The OLED display is updated every loop with:

Temperature reading (in selected unit with trend arrow ↑ ↓)

Humidity value

LED status (ON/OFF depending on logic)

Rotating motivational quote or message on the bottom line

Temperature trend (rising, falling, or stable) is shown using ASCII characters 24 (↑) and 25 (↓).

🔁 Display Logic and Centering
Display strings (e.g., temperature, humidity) are formatted with snprintf() and dtostrf() to control number formatting.

The text is centered using oled.getStrWidth() and simple math.

The oled.firstPage() and oled.nextPage() loop ensures proper screen refresh using the U8g2 library.

🧠 Quote System
A quote is fetched with getQuote() and indexed using currentQuote.

After each loop, currentQuote is incremented and wrapped around using modulo to rotate quotes cyclically.

🧷 Safety: Watchdog Monitoring
The checkWatchdog() function is called at the end of the loop to reset or check system health, ensuring the microcontroller doesn't hang or freeze.

🕓 Timing
A delay(1000) at the end of each loop ensures a 1-second refresh rate for display and sensor reads.