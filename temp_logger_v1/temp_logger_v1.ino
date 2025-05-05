
#include "DHT.h"

#define DHTPIN 2
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  dht.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(2000);

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Check if readings failed
  if (isnan(humidity)|| isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor...");
    return;
  }

  Serial.print("Humidity: ");
  Serial.println(humidity);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");
}
