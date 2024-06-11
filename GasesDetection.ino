#include <LiquidCrystal_I2C.h> // Include the LiquidCrystal library for I2C
#include <Wire.h>              // Include the Wire library for I2C

#define MQ2_PIN A0             // Define the analog pin for MQ-2 sensor
#define MQ135_PIN A1           // Define the analog pin for MQ-135 sensor
#define RELAY_PIN_1 2          // Define the digital pin for relay 1
#define RELAY_PIN_2 3          // Define the digital pin for relay 2
#define LED_ADDRESS 0x27       // Address of your 16x2 LED display

LiquidCrystal_I2C lcd(LED_ADDRESS, 16, 2); // Initialize the LCD object

// Baseline values for MQ-2 sensor
int baselineLPG = 0;
int baselineSmoke = 0;
int baselineCO2 = 0;
const int NUM_CALIBRATION_READINGS = 10;

// Noise threshold
const int NOISE_THRESHOLD = 5;

unsigned long previousMillis = 0;
const long interval = 1000; // Update interval in milliseconds

int scrollPosition = 0; // Position for scrolling
String firstRowText;    // Text to scroll in the first row

void setup() {
  lcd.init();               // Initialize the LCD
  lcd.backlight();          // Turn on the backlight
  lcd.clear();              // Clear the LCD screen
  Serial.begin(9600);       // Initialize serial communication
  
  pinMode(MQ2_PIN, INPUT);    // Set MQ-2 pin as input
  pinMode(MQ135_PIN, INPUT);  // Set MQ-135 pin as input
  pinMode(RELAY_PIN_1, OUTPUT); // Set relay pin 1 as output
  pinMode(RELAY_PIN_2, OUTPUT); // Set relay pin 2 as output

  // Allow time for sensors to stabilize
  delay(10000);

  // Calibrate MQ-2 sensor
  calibrateMQ2Sensor();
}

void calibrateMQ2Sensor() {
  long sumLPG = 0;
  long sumSmoke = 0;
  long sumCO2 = 0;

  // Take multiple readings and calculate the average
  for (int i = 0; i < NUM_CALIBRATION_READINGS; ++i) {
    int mq2Reading = analogRead(MQ2_PIN);
    sumLPG += mq2Reading;
    sumSmoke += mq2Reading;
    sumCO2 += mq2Reading;
    delay(1000); // Delay between readings
  }

  baselineLPG = sumLPG / NUM_CALIBRATION_READINGS;
  baselineSmoke = sumSmoke / NUM_CALIBRATION_READINGS;
  baselineCO2 = sumCO2 / NUM_CALIBRATION_READINGS;
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Read the raw sensor values of LPG, Smoke, and CO2 using MQ-2 sensor
    int mq2Reading = analogRead(MQ2_PIN);
    int lpgValue = max(0, mq2Reading - baselineLPG);    // Calculate LPG value
    int smokeValue = max(0, mq2Reading - baselineSmoke); // Calculate Smoke value
    int co2Value = max(0, mq2Reading - baselineCO2);    // Calculate CO2 value

    // Check for noise and ignore values below the threshold
    if (lpgValue < NOISE_THRESHOLD) lpgValue = 0;
    if (smokeValue < NOISE_THRESHOLD) smokeValue = 0;
    if (co2Value < NOISE_THRESHOLD) co2Value = 0;

    // Read the air quality value using MQ-135 sensor
    int airQualityValue = analogRead(MQ135_PIN);

    // Create the text to scroll
    firstRowText = "LPG: " + String(lpgValue) + " Smoke: " + String(smokeValue) + " CO2: " + String(co2Value);

    // Calculate the length of the scrolling text
    int textLength = firstRowText.length();
    
    // Ensure scroll position is within range
    scrollPosition++;
    if (scrollPosition >= textLength + 16) {
      scrollPosition = 0;
    }

    // Clear only the first row for updated values, keeping the second row intact
    lcd.setCursor(0, 0); // Set cursor to the first row
    if (scrollPosition < textLength) {
      lcd.print(firstRowText.substring(scrollPosition));
      if (scrollPosition + 16 > textLength) {
        lcd.print(firstRowText.substring(0, (scrollPosition + 16) - textLength));
      }
    } else {
      lcd.print(firstRowText.substring(scrollPosition - textLength));
    }
    lcd.print("   "); // Clear any leftover characters

    Serial.print("LPG: ");
    Serial.print(lpgValue);
    Serial.print(" Smoke: ");
    Serial.print(smokeValue);
    Serial.print(" CO2: ");
    Serial.print(co2Value);
    Serial.print(" Air Quality: ");
    Serial.println(airQualityValue);

    // Print the air quality value on the second row
    lcd.setCursor(0, 1); // Set cursor to the second row
    lcd.print("Air Quality: ");
    lcd.print(airQualityValue);
    lcd.print("    "); // Clear any leftover characters

    // Control the relay based on sensor readings
    if (lpgValue > NOISE_THRESHOLD || smokeValue > NOISE_THRESHOLD || co2Value > NOISE_THRESHOLD) {
      digitalWrite(RELAY_PIN_1, LOW); // Turn on relay 1
      digitalWrite(RELAY_PIN_2, LOW); // Turn on relay 2
    } else {
      digitalWrite(RELAY_PIN_1, HIGH); // Turn off relay 1
      digitalWrite(RELAY_PIN_2, HIGH); // Turn off relay 2
    }
  }
}
