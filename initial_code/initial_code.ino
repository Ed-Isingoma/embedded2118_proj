#include <DHT11.h>
#include <EEPROM.h>
const int redLed = 3;       // Pin 2 for red LED
const int yellowLed = 4;    // Pin 1 for yellow LED
const int buzzer = 2; 

const int dhtPin = 7;  // Pin connected to DHT11 data pin
const int MQ2_PIN=A0;

unsigned long lastWriteTime = 0;  // Variable to track last write time
unsigned long lastReadTime = 0;   // Variable to track last read time
const long writeInterval = 5000;  // 5 seconds for writing
const long readInterval = 10000; // 10 seconds for reading

DHT11 dht11(dhtPin);

struct WeatherData {
    unsigned int temperature : 6; // 6 bits for temperature 
    unsigned int humidity : 6;    // 6 bits for humidity 
    unsigned int smokeLevel : 6;  // 6  bits for smoke level 
} ;

int readTempHumidity(WeatherData &data){
  int temp, hum;
  
    // Attempt to read the temperature and humidity values from the DHT11 sensor.
    int result = dht11.readTemperatureHumidity(temp,hum);
    
    if (result == 0) {    
      data.temperature = constrain(temp,0,63);
      Serial.print("Temperature: ");
      Serial.print(data.temperature);
      Serial.println(" °C");
    }
    else {
        // Print error message based on the error code.
        Serial.println(DHT11::getErrorString(result));
    }
  return data.temperature;
}

int readSmokeLevel(WeatherData &data) {
    int rawSmokeLevel = analogRead(MQ2_PIN);  // Read analog value (0-1023)
    data.smokeLevel=map(rawSmokeLevel, 0, 1023, 0, 63); // Scale to fit 6-bit range (0-63)
    Serial.print("Smoke Level: ");
    Serial.println(data.smokeLevel);
    return data.smokeLevel;
}

void HighTemp(){
  PORTB |= (1 << redLed);  // Set red LED pin high
  PORTB &= ~(1 << yellowLed);    // Set yellow LED pin low
  PORTB |= (1 << buzzer);        // Turn on the buzzer
  Serial.println("Red led on,Buzzer on");
}

void LowTemp(){
  PORTB |= (1 << yellowLed);  // Set yellow LED pin high
  PORTB &= ~(1 << redLed);    // Set red LED pin low
   PORTB &= ~(1 << buzzer);       // Turn off the buzzer
  Serial.println("Yellow led on, Buzzer off");
}

//*portDDRB &= ~(1 << 1); // Set Pin 1 as input

void writeDataToEEPROM(WeatherData &data) {
  EEPROM.put(0, data.temperature);  // Store temperature at address 0
  EEPROM.put(2, data.smokeLevel);   // Store smoke level at address 2 (make sure not to overwrite)
}
// Function to read temperature from EEPROM
int readTempFromEEPROM(int address) {
  int value;
  EEPROM.get(address, value);  // Read the stored integer value from EEPROM
  return value;
}

// Function to read smoke level from EEPROM
int readSmokeFromEEPROM(int address) {
  int value;
  EEPROM.get(address, value);  // Read the stored integer value from EEPROM
  return value;
}


void setup() {
  Serial.begin(9600);
  DDRB |= (1 << redLed);     // Set red LED pin as output
  DDRB |= (1 << yellowLed);  // Set yellow LED pin as output
  DDRB |= (1 << buzzer);     //Set buzzer as output
}

void loop() {
  WeatherData data;
  
  // Get current time (in milliseconds)
  unsigned long currentMillis = millis();
  
  // Write to EEPROM every 5 seconds
  if (currentMillis - lastWriteTime >= writeInterval) {
    lastWriteTime = currentMillis;  // Update the last write time
    data.temperature = readTempHumidity(data);  // Read temperature
    data.smokeLevel = readSmokeLevel(data);    // Read smoke level
    
    // Store data in EEPROM
    writeDataToEEPROM(data);
    Serial.println("Data written to EEPROM");
  }
  if (currentMillis - lastReadTime >= readInterval) {
    lastReadTime = currentMillis;  // Update the last read time
    
    // Read the stored data from EEPROM
    int storedTemp = readTempFromEEPROM(0);  // Read stored temperature from address 0
    int storedSmokeLevel = readSmokeFromEEPROM(2);  // Read stored smoke level from address 2
    
    // Print the stored data to verify
    Serial.print("Stored Temp: ");
    Serial.print(storedTemp);
    Serial.print(" °C\tStored Smoke Level: ");
    Serial.println(storedSmokeLevel);
  }
  
  // Add a small delay to avoid flooding the serial monitor
  delay(100);  // You can adjust this delay as needed
}
