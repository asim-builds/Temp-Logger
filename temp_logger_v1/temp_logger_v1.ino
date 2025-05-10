#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <DHT.h>
#include <avr/wdt.h>

// Pin definitions
#define DHTPIN 2
#define DHTTYPE DHT11
#define RED_PIN 5
#define GREEN_PIN 6
#define BLUE_PIN 9
#define BUZZER_PIN 8
#define BUTTON_PIN 3
#define LED_BUTTON_PIN 4  // New button pin for toggling LED

// Simple watchdog timer
unsigned long lastActivityTime = 0;
const unsigned long WATCHDOG_TIMEOUT = 10000; // 10 seconds

// Global variables (minimized)
bool displayInCelsius = true;
bool lastButtonState = HIGH;
bool lastLedButtonState = HIGH;
bool wasNormal = true;
uint8_t currentQuote = 0;
bool ledEnabled = true;  // New variable to track LED state

// Temperature tracking
float lastTemp = 0;
uint8_t trendSymbol = 0; // 0=stable, 1=up, 2=down

// For trend monitoring over time
const unsigned long TREND_INTERVAL = 300000; // 5 minutes in milliseconds
unsigned long lastTrendTime = 0;
float trendBaseTemp = 0;

// Quote timing
unsigned long lastQuoteChangeTime = 0;
const unsigned long QUOTE_CHANGE_INTERVAL = 15000; // 15 seconds between quote changes

// Debugging
const bool DEBUG = false;

// Device instances
U8G2_SSD1306_128X64_NONAME_1_HW_I2C oled(U8G2_R0);
DHT dht(DHTPIN, DHTTYPE);

// Quotes array (stored in program memory to save RAM)
const char quote0[] PROGMEM = "5, 4, 3, 2, 1....GO";
const char quote1[] PROGMEM = "There is no tomorrow.";
const char quote2[] PROGMEM = "Keep moving forward.";
const char quote3[] PROGMEM = "Progress over Results.";
const char quote4[] PROGMEM = "Veni Vici Vidi";
// const char quote5[] PROGMEM = "I failed I broke I rose.";

const char* const quotes[] PROGMEM = {
  quote0, quote1, quote2, quote3, quote4
};
const uint8_t quoteCount = sizeof(quotes) / sizeof(quotes[0]);

void setup() {
  Serial.begin(9600);
  Serial.println(F("Starting..."));
  
  // Initialize pins
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_BUTTON_PIN, INPUT_PULLUP);  // Initialize new button pin
  
  // Initialize devices
  Wire.begin();
  oled.begin();
  
  // Show startup message
  oled.firstPage();
  do {
    oled.setFont(u8g2_font_7x14_tf);
    oled.drawStr(10, 30, "Starting...");
  } while (oled.nextPage());
  delay(500);
  
  // Initialize DHT sensor
  dht.begin();
  delay(1000); // Give DHT time to stabilize
  
  // Initialize watchdog
  lastActivityTime = millis();
  lastQuoteChangeTime = millis(); // Initialize quote timer
  
  // Setup a hardware watchdog
  wdt_enable(WDTO_2S);
  
  Serial.println(F("Setup complete"));
}

// Get a quote from program memory
void getQuote(char* buffer, uint8_t index) {
  strcpy_P(buffer, (char*)pgm_read_word(&(quotes[index])));
}

// Minimal watchdog reset function
void kickWatchdog() {
  lastActivityTime = millis();
}

// Check if system is responsive
void checkWatchdog() {
  if (millis() - lastActivityTime > WATCHDOG_TIMEOUT) {
    // System hung - reset core components
    Wire.end();
    Wire.begin();
    oled.begin();
    dht.begin();
    kickWatchdog();
  }
}

// Set RGB LED color
inline void setColor(uint8_t r, uint8_t g, uint8_t b) {
  if (ledEnabled) {
    analogWrite(RED_PIN, r);
    analogWrite(GREEN_PIN, g);
    analogWrite(BLUE_PIN, b);
  } else {
    // Turn off LED completely when disabled
    analogWrite(RED_PIN, 0);
    analogWrite(GREEN_PIN, 0);
    analogWrite(BLUE_PIN, 0);
  }
}

// Check temperature unit button state
bool checkButton() {
  bool btnState = digitalRead(BUTTON_PIN);
  bool unitChanged = false;
  
  if (btnState == LOW && lastButtonState == HIGH) {
    // Button pressed - debounce
    delay(50);
    
    // Toggle temperature unit
    displayInCelsius = !displayInCelsius;
    unitChanged = true;
    if (DEBUG) Serial.println(F("Toggled temperature unit"));
  }
  
  lastButtonState = btnState;
  return unitChanged;
}

// Check LED toggle button state
void checkLedButton() {
  bool btnState = digitalRead(LED_BUTTON_PIN);
  
  if (btnState == LOW && lastLedButtonState == HIGH) {
    // Button pressed - debounce
    delay(50);
    
    // Toggle LED state
    ledEnabled = !ledEnabled;
    if (DEBUG) {
      Serial.print(F("LED toggled: "));
      Serial.println(ledEnabled ? F("ON") : F("OFF"));
    }
  }
  
  lastLedButtonState = btnState;
}

// Update temperature trend indicator over a longer time period
void updateTrend(float currentTemp) {
  unsigned long currentTime = millis();
  
  // Initialize base temperature if it's the first reading
  if (trendBaseTemp == 0) {
    trendBaseTemp = currentTemp;
    lastTrendTime = currentTime;
    if (DEBUG) Serial.println(F("First trend reading - initializing baseline"));
    return;
  }
  
  // Store current value
  lastTemp = currentTemp;
  
  // Only update trend indicator every TREND_INTERVAL (e.g., 5 minutes)
  if (currentTime - lastTrendTime >= TREND_INTERVAL) {
    // Calculate difference from the base temperature from 5 minutes ago
    float diff = currentTemp - trendBaseTemp;
    
    // Threshold of 0.5 degrees over 5 minutes for meaningful change
    if (diff >= 0.5) {
      trendSymbol = 1; // Up arrow
      if (DEBUG) {
        Serial.print(F("5min Trend: UP (diff: "));
        Serial.print(diff);
        Serial.println(F("°C)"));
      }
    } else if (diff <= -0.5) {
      trendSymbol = 2; // Down arrow
      if (DEBUG) {
        Serial.print(F("5min Trend: DOWN (diff: "));
        Serial.print(diff);
        Serial.println(F("°C)"));
      }
    } else {
      trendSymbol = 0; // Stable
      if (DEBUG) {
        Serial.print(F("5min Trend: STABLE (diff: "));
        Serial.print(diff);
        Serial.println(F("°C)"));
      }
    }
    
    // Reset the base temperature and time for next interval
    trendBaseTemp = currentTemp;
    lastTrendTime = currentTime;
  }
}

// Update LED based on temperature - with correction for F scale
void updateLED(float temp, bool isCelsius) {
  // Convert to Celsius for consistent LED color mapping if in F mode
  float tempForLED = isCelsius ? temp : (temp - 32.0) * 5.0 / 9.0;
  
  if (tempForLED < 8) {
    setColor(0, 100, 255);      // Icy Blue
  } else if (tempForLED < 12) {
    setColor(0, 120, 255);      // Cool Blue
  } else if (tempForLED < 20) {
    setColor(0, 180, 255);      // Light Blue
  } else if (tempForLED < 25) {
    setColor(0, 255, 0);        // Green
  } else if (tempForLED < 33) {
    setColor(255, 100, 0);      // Orange
  } else {
    setColor(255, 0, 0);        // Red
  }
}

// Check temperature and trigger buzzer if needed
void checkTemp(float temp, bool isCelsius) {
  // Convert to Celsius for consistent evaluation if in F mode
  float tempForCheck = isCelsius ? temp : (temp - 32.0) * 5.0 / 9.0;
  
  bool isAbnormal = (tempForCheck >= 40 || tempForCheck < 5);
  
  if (isAbnormal && wasNormal) {
    // Warning beep
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    wasNormal = false;
  } else if (!isAbnormal && !wasNormal) {
    wasNormal = true;
  }
}

void loop() {
  // Reset the hardware watchdog
  wdt_reset();

  // Reset watchdog at start of loop
  kickWatchdog();
  
  // Check both buttons
  bool unitChanged = checkButton();
  checkLedButton();
  
  // Read sensor data - add delay before reading to stabilize
  delay(50);
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  
  // Verify sensor readings
  if (isnan(humidity) || isnan(temperature)) {
    // Failed reading, show error message and try again next loop
    oled.firstPage();
    do {
      oled.setFont(u8g2_font_7x14_tf);
      oled.drawStr(5, 30, "Sensor Error!");
    } while (oled.nextPage());
    
    delay(1000);
    return;
  }
  
  // Update trend indicator
  updateTrend(temperature);
  
  // Print values to serial for debugging
  if (DEBUG) {
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print("%, Temp: ");
    Serial.print(temperature);
    Serial.print("°C, LED: ");
    Serial.println(ledEnabled ? "ON" : "OFF");
  }
  
  // Convert temperature if needed
  float displayTemp = temperature;
  if (!displayInCelsius) {
    displayTemp = (temperature * 9.0 / 5.0) + 32.0;
  }
  
  // Update outputs - pass original temp in Celsius for proper LED behavior
  updateLED(displayTemp, displayInCelsius);
  checkTemp(displayTemp, displayInCelsius);
  
  // Buffer for storing formatted text
  char buffer[30];
  
  // Format values to fixed decimal places
  char tempStr[6]; // Buffer for temperature string
  char humStr[6];  // Buffer for humidity string
  
  // Convert floating point to strings with 1 decimal place
  dtostrf(displayTemp, 4, 1, tempStr);
  dtostrf(humidity, 4, 1, humStr);
  
  // Update display
  oled.firstPage();
  do {
    // Temperature display with trend indicator
    oled.setFont(u8g2_font_7x14_tf);
    
    // Format the temperature with trend
    if (trendSymbol == 1) {
      // Up arrow
      snprintf(buffer, sizeof(buffer), "Temp: %s %c %c", tempStr, displayInCelsius ? 'C' : 'F', 24); // ASCII 24 is ↑
    } else if (trendSymbol == 2) {
      // Down arrow
      snprintf(buffer, sizeof(buffer), "Temp: %s %c %c", tempStr, displayInCelsius ? 'C' : 'F', 25); // ASCII 25 is ↓
    } else {
      // No trend indicator
      snprintf(buffer, sizeof(buffer), "Temp: %s %c", tempStr, displayInCelsius ? 'C' : 'F');
    }
    
    uint8_t tempX = (128 - oled.getStrWidth(buffer)) / 2;
    oled.drawStr(tempX, 18, buffer);
    
    // Humidity display
    snprintf(buffer, sizeof(buffer), "Hum: %s %%", humStr);
    uint8_t humX = (128 - oled.getStrWidth(buffer)) / 2;
    oled.drawStr(humX, 36, buffer);
    
    // Quote display (moved up to where LED status was)
    oled.setFont(u8g2_font_6x12_tf);
    getQuote(buffer, currentQuote);
    uint8_t quoteX = (128 - oled.getStrWidth(buffer)) / 2;
    oled.drawStr(quoteX, 54, buffer);
  } while (oled.nextPage());
  
  // Check if it's time to change the quote (every QUOTE_CHANGE_INTERVAL milliseconds)
  unsigned long currentTime = millis();
  if (currentTime - lastQuoteChangeTime >= QUOTE_CHANGE_INTERVAL) {
    // Update quote
    currentQuote = (currentQuote + 1) % quoteCount;
    lastQuoteChangeTime = currentTime;
    
    // Log quote change
    if (DEBUG) {
      Serial.print(F("Quote changed to #"));
      Serial.println(currentQuote);
    }
  }
  
  // Check if system is hanging
  checkWatchdog();
  
  // Wait before next update
  delay(1000);
}