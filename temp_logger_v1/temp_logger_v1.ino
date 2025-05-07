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

// Simple watchdog timer
unsigned long lastActivityTime = 0;
const unsigned long WATCHDOG_TIMEOUT = 10000; // 10 seconds

// Global variables (minimized)
bool displayInCelsius = true;
bool lastButtonState = HIGH;
bool wasNormal = true;
uint8_t currentQuote = 0;

// Device instances
U8G2_SSD1306_128X64_NONAME_1_HW_I2C oled(U8G2_R0);
DHT dht(DHTPIN, DHTTYPE);

// Quotes array (stored in program memory to save RAM)
const char quote0[] PROGMEM = "Seize the day!";
const char quote1[] PROGMEM = "Dream big!";
const char quote2[] PROGMEM = "Never give up.";
const char quote3[] PROGMEM = "5, 4, 3, 2, 1... GO";
const char quote4[] PROGMEM = "I came I saw I conquered.";
const char quote5[] PROGMEM = "There is no tomorrow.";

const char* const quotes[] PROGMEM = {
  quote0, quote1, quote2, quote3, quote4, quote5
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
  analogWrite(RED_PIN, r);
  analogWrite(GREEN_PIN, g);
  analogWrite(BLUE_PIN, b);
}

// Check button state
bool checkButton() {
  bool btnState = digitalRead(BUTTON_PIN);
  bool changed = false;
  
  if (btnState == LOW && lastButtonState == HIGH) {
    // Button pressed
    displayInCelsius = !displayInCelsius;
    changed = true;
    delay(50); // Simple debounce
  }
  
  lastButtonState = btnState;
  return changed;
}

// Update LED based on temperature
void updateLED(float temp) {
  if (temp < 8) {
    setColor(0, 100, 255);      // Icy Blue
  } else if (temp < 12) {
    setColor(0, 120, 255);      // Cool Blue
  } else if (temp < 20) {
    setColor(0, 180, 255);      // Light Blue
  } else if (temp < 25) {
    setColor(0, 255, 0);        // Green
  } else if (temp < 33) {
    setColor(255, 100, 0);      // Orange
  } else {
    setColor(255, 0, 0);       // Red
  }
}

// Check temperature and trigger buzzer if needed
void checkTemp(float temp) {
  bool isAbnormal = (temp <= 23 || temp < 5);
  
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
  
  // Check for button press
  bool unitChanged = checkButton();
  
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
  
  // Print values to serial for debugging
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print("%, Temp: ");
  Serial.print(temperature);
  Serial.println("°C");
  
  // Convert temperature if needed
  if (!displayInCelsius) {
    temperature = (temperature * 9.0 / 5.0) + 32.0;
  }
  
  // Update outputs
  updateLED(temperature);
  checkTemp(temperature);
  
  // Buffer for storing formatted text
  char buffer[30];
  
  // Format values to fixed decimal places
  char tempStr[6]; // Buffer for temperature string
  char humStr[6];  // Buffer for humidity string
  
  // Convert floating point to strings with 1 decimal place
  dtostrf(temperature, 4, 1, tempStr);
  dtostrf(humidity, 4, 1, humStr);
  
  // Update display
  oled.firstPage();
  do {
    // Temperature display
    oled.setFont(u8g2_font_7x14_tf);
    snprintf(buffer, sizeof(buffer), "Temp: %s %c", tempStr, displayInCelsius ? 'C' : 'F');
    uint8_t tempX = (128 - oled.getStrWidth(buffer)) / 2;
    oled.drawStr(tempX, 18, buffer);
    
    // Humidity display
    snprintf(buffer, sizeof(buffer), "Hum: %s %%", humStr);
    uint8_t humX = (128 - oled.getStrWidth(buffer)) / 2;
    oled.drawStr(humX, 36, buffer);
    
    // Quote display
    oled.setFont(u8g2_font_6x13_tr);
    getQuote(buffer, currentQuote);
    oled.drawStr(0, 56, buffer);
    
  } while (oled.nextPage());
  
  // Update quote for next display
  currentQuote = (currentQuote + 1) % quoteCount;
  
  // Check if system is hanging
  checkWatchdog();
  
  // Wait before next update
  delay(1000);
}