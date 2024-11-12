#include <DHT.h>
#include <EEPROM.h>

#define DHTPIN 2       // DHT22 data pin connected to digital pin 2
#define DHTTYPE DHT22  // DHT22 sensor type

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);
  dht.begin();
}

void loop() {
  // Read temperature and humidity from the DHT22
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Check if any reads failed
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Convert temperature and humidity to integers for storage in EEPROM
  uint8_t tempValue = (uint8_t)temperature; // Assuming 0-100°C range
  uint8_t humValue = (uint8_t)humidity;     // Assuming 0-100% range

  // Write to EEPROM (address 0 for temperature, 1 for humidity)
  EEPROM.update(0, tempValue);
  EEPROM.update(1, humValue);

  // Read values back from EEPROM for verification
  uint8_t storedTemp = EEPROM.read(0);
  uint8_t storedHum = EEPROM.read(1);

  // Print EEPROM-stored values to Serial Monitor
  Serial.print("Stored Temperature: ");
  Serial.print(storedTemp);
  Serial.println(" °C");
  Serial.print("Stored Humidity: ");
  Serial.print(storedHum);
  Serial.println(" %");

  delay(10000); // Wait another 10 seconds before the next loop
}
