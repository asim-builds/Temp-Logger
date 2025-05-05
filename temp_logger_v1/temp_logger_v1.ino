#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <DHT.h>

#define DHTPIN 2
#define DHTTYPE DHT11

U8G2_SSD1306_128X64_NONAME_1_HW_I2C oled(U8G2_R0);
DHT dht(DHTPIN, DHTTYPE);

const char* quotes[] = {
  "Seize the day!",
  "Dream big!",
  "Never give up.",
  "Be the change.",
  "Stay positive.",
  "There is no tomorrow.",
};
const int quoteCount = sizeof(quotes) / sizeof(quotes[0]);
int currentQuote = 0;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  if (!oled.begin()) {
    Serial.println("Display initialization failed");
    while (1);
  }
  dht.begin();
}

void loop() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  char tempStr[10];
  char humStr[10];
  dtostrf(temperature, 0, 1, tempStr);
  dtostrf(humidity, 0, 1, humStr);

  oled.firstPage();
  do {
    // Set larger font for Temp & Humidity
    oled.setFont(u8g2_font_7x14_tf);

    // Uncomment this code to put the temperature and humidity values to the left.
    // Temp line (adjusted x-coordinates to remove visual gap)
    // oled.drawStr(0, 18, "Temp:");
    // oled.drawStr(40, 18, tempStr);
    // oled.drawStr(75, 18, "C");

    // // Humidity line
    // oled.drawStr(0, 36, "Hum:");
    // oled.drawStr(35, 36, humStr);
    // oled.drawStr(65, 36, "%");

    // Build full strings for temp and humidity
    char tempLine[25];
    char humLine[25];
    sprintf(tempLine, "Temp: %s C", tempStr);
    sprintf(humLine, "Hum: %s %%", humStr);

    // Calculate X for centering
    int tempX = (128 - oled.getStrWidth(tempLine)) / 2;
    int humX = (128 - oled.getStrWidth(humLine)) / 2;

    // Draw centered strings
    oled.drawStr(tempX, 18, tempLine);
    oled.drawStr(humX, 36, humLine);

    // Set slightly larger font for quote (but still smaller than Temp/Hum)
    oled.setFont(u8g2_font_6x13_tr);
    oled.drawStr(0, 56, quotes[currentQuote]);

  } while (oled.nextPage());

  currentQuote = (currentQuote + 1) % quoteCount;
  delay(3000);
}
