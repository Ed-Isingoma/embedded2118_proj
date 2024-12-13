#include <DHT11.h>
#include <EEPROM.h>

#include <Sim800L.h>
#include <SoftwareSerial.h>

// Define pin connections 
#define RX 10
#define TX 11
//#define RST 12

// Create Sim800L instance
Sim800L sim800l(RX, TX);
const char phoneNumber[] = "+256702439337";
const char message[] = "The temperature and smoke levels are high";


const int redLed = 3;       // Pin 3 for red LED
const int greenLed = 4;    // Pin 4 for green LED
const int buzzer = 2; 
const int button  = 3;

unsigned char *portDDRB = (unsigned char *) 0x24;  // Data Direction Register B
unsigned char *portDataB = (unsigned char *) 0x25; // Data Register B

unsigned char *portDDRD = (unsigned char *) 0x2A;  // Data Direction Register D
unsigned char *portDataD = (unsigned char *) 0x2B; // Data Register D
unsigned char *pinDataD = (unsigned char *) 0x29;  // Pin Input Register D

const int dhtPin = 7;  // Pin connected to DHT11 data pin
const int MQ2_PIN=A0;

unsigned long lastDebounceTime = 0;      // Last debounce time
unsigned long debounceDelay = 50; 

bool buttonState = false;    // Current button state
bool lastButtonState = false;

unsigned long lastWriteTime = 0;  // Variable to track last write time
unsigned long lastReadTime = 0;   // Variable to track last read time
const long writeInterval = 5000;  // 5 seconds for writing
const long readInterval = 10000; // 10 seconds for reading

const int maxtemp=26;
const int maxsmoke=24;

DHT11 dht11(dhtPin);

volatile bool alertTriggered = false;

// To communicate with the GSM module Arduino
volatile bool sendCommandToGSM = false;


struct WeatherData {
    unsigned int temperature : 6; // 6 bits for temperature 
    unsigned int humidity : 6;    // 6 bits for humidity 
    unsigned int smokeLevel : 6;  // 6  bits for smoke level 
} data;

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
void ISR_alert() {
  alertTriggered = true; // Set the flag when the interrupt is triggered
  sendCommandToGSM = true; // Signal the GSM Arduino
}

void HighTemp(){
  *portDataB|= (1 << redLed);  // Set red LED pin high
  *portDataB= ~(1 << greenLed);    // Set green LED pin low
  *portDataB |= (1 << buzzer);        // Turn on the buzzer
  Serial.println("Red led on,Buzzer on");

  if (sendCommandToGSM) {
    Serial.println("Notifying GSM Arduino...");
    Serial.println("Calling...");
    sim800l.callNumber(phoneNumber); // No return value to check
    Serial.println("Call command sent.");
    sim800l.sendSms(phoneNumber, message);
    Serial.println("SMS sent successfully!");
    sendCommandToGSM = false; // Reset the flag after notifying
  }
}

void LowTemp(){
  *portDataB |= (1 << greenLed);  // Set green LED pin high
  *portDataB &= ~(1 << redLed);    // Set red LED pin low
  *portDataB &= ~(1 << buzzer);       // Turn off the buzzer
  Serial.println("Green led on, Buzzer off");
}



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

void debounceButton() {
    static bool currentState = false;
    static unsigned long lastDebounceTime = 0;

    bool reading = (*pinDataD & (1 << button));  // Read the button state
    if (reading != lastButtonState) {
        lastDebounceTime = millis(); // Reset the debounce timer
    }
    if ((millis() - lastDebounceTime) > debounceDelay) {
        if (reading != currentState) {
            currentState = reading;
            if (currentState) { // Return true only on button press
              *portDataB |= (1 << redLed);
              Serial.println("Button Pressed: Red LED ON");
            }
            else {
                // Turn red LED off when button is released
                *portDataB &= ~(1 << redLed);
                Serial.println("Button Released: Red LED OFF");
            }
        }
    }
    lastButtonState = reading;
}


void setup() {
  Serial.begin(9600);
  *portDDRB |= (1 << redLed);     // Set red LED pin as output
  *portDDRB |= (1 << greenLed);  // Set green LED pin as output
  *portDDRB |= (1 << buzzer);     //Set buzzer as output
  *portDDRD &= ~(1 << button);
  *portDataD &= ~(1 << button);

   // Configure external interrupt on INT0 (pin 2)
  EICRA |= (1 << ISC01) | (1 << ISC00); // Trigger on rising edge
  EIMSK |= (1 << INT0);                 // Enable INT0 interrupt

  sei(); // Enable global interrupts
  Serial.println("Initializing SIM800L..."); // Initialize the SIM800L
  sim800l.begin();

  Serial.println("SIM800L initialized.");
}

void loop() {
  debounceButton();
  WeatherData data;
  
  // Get current time (in milliseconds)
  unsigned long currentMillis = millis();
  int temp =readTempHumidity(data);
  int smoke=readSmokeLevel(data);

   // Check if interrupt flag is set
  if (alertTriggered) {
    alertTriggered = false; // Reset the flag
    if (temp >= maxtemp && smoke >= maxsmoke) {
      HighTemp();
    }
  } else {
    if (temp < maxtemp || smoke < maxsmoke) {
      LowTemp();
    }
  }

  delay(100); // Small delay to prevent flooding



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

