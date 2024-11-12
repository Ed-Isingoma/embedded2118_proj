#include <DHT11.h>
const int redLed = 3;       // Pin 2 for red LED
const int yellowLed = 4;    // Pin 1 for yellow LED
const int buzzer = 2; 

const int dhtPin = 7;  // Pin connected to DHT11 data pin
const int MQ2_PIN=A0;

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
      data.humidity = constrain(hum,0,63);
      Serial.print("Temperature: ");
      Serial.print(data.temperature);
      Serial.print(" °C\tHumidity: ");
      Serial.print(data.humidity);
      Serial.println(" %");
      
    }
    else {
        // Print error message based on the error code.
        Serial.println(DHT11::getErrorString(result));
    }
  //return result;
}

int readSmokeLevel(WeatherData &data) {
    int rawSmokeLevel = analogRead(MQ2_PIN);  // Read analog value (0-1023)
    data.smokeLevel=map(rawSmokeLevel, 0, 1023, 0, 63); // Scale to fit 6-bit range (0-63)
    Serial.print("Smoke Level: ");
    Serial.println(data.smokeLevel);
    //return data.smokeLevel;
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


void setup() {
  Serial.begin(9600);
  DDRB |= (1 << redLed);     // Set red LED pin as output
  DDRB |= (1 << yellowLed);  // Set yellow LED pin as output
  DDRB |= (1 << buzzer);     //Set buzzer as output
}

void loop() {
  WeatherData data;
  data.temperature=readTempHumidity(data);
  data.smokeLevel=readSmokeLevel(data);
  if (data.temperature > 27 && data.smokeLevel > 60) {  // Modify thresholds as needed
        HighTemp();  // Call HighTemp() if temperature and smoke level are high
    } else {
        LowTemp();  // Call LowTemp() if either is not high
    }

    delay(2000);

}
