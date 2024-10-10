#include "config.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <time.h>  // Include the time library to get timestamps

// LCD display configuration
int mySDA = 21;  // SDA pin for I2C communication (used by LCD)
int mySCL = 22;  // SCL pin for I2C communication (used by LCD)
int lcdColumns = 20;  // Number of columns on the LCD
int lcdRows = 4;  // Number of rows on the LCD
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);  // Create LCD object with I2C address 0x27

// Gas sensor (CCS811) configuration
#include "Adafruit_CCS811.h"
Adafruit_CCS811 ccs;  // Create CCS811 gas sensor object
float co2ppm, tvocppm;  // Variables to store CO2 and TVOC readings
float airQualityIndex;  // Variable to store combined air quality index

// Pressure and temperature sensor (BMP280) configuration
#include <Adafruit_BMP280.h>
Adafruit_BMP280 bmp;  // Create BMP280 sensor object for temperature and pressure measurement
float temperature;  // Variable to store temperature reading
float pressure;  // Variable to store pressure reading
float altitude;  // Variable to store altitude calculation
float humidity;  // Variable to store humidity reading (from another sensor if available)

// Light sensor configuration
const int lightSensorPin = 34;  // Light sensor analog pin (e.g., LDR connected to GPIO 34)
int lightValue;  // Variable to store light sensor readings
int smoothedLightValue = 0;  // Smoothed value for light sensor
String lightLevelDescription;  // Variable to store light level description

// Timing variables
unsigned long currentMillis;  // Store the current time in milliseconds
unsigned long startMillis;  // Store the start time in milliseconds
const unsigned long period = 300000;  // Interval period for sending data (in milliseconds)
bool firstTime = true;  // Variable to check if it's the first data send

// Function to get the ESP32 unique ID
String getESP32ID() {
  uint64_t chipid = ESP.getEfuseMac();  // Get ESP32 chip ID (essentially its MAC address)
  String id = String((uint16_t)(chipid >> 32), HEX) + String((uint32_t)chipid, HEX);
  id.toUpperCase();  // Convert to uppercase for better readability
  return id;
}

// Function to get the current timestamp
String getCurrentTimestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Failed to get time";
  }
  char timeStringBuff[25];  // Buffer to hold formatted timestamp
  strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d %H:%M", &timeinfo);
  return String(timeStringBuff);
}

void setup() {
  // Start serial communication
  Serial.begin(115200);
  Serial.println("In setup");

  // Initialize the LCD
  lcd.init();
  lcd.backlight();

  // Initialize the gas sensor (CCS811)
  if (!ccs.begin()) {
    Serial.println("Failed to start CCS811 sensor! Please check your wiring.");
    lcd.setCursor(0, 0);
    lcd.print("CCS811 init failed");
    while (1);  // Stop program if sensor initialization fails
  }

  // Wait for the sensor to be ready
  while (!ccs.available());

  // Initialize Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Configure time using NTP
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");  // Set timezone to UTC
  Serial.println("Time configured");

  // Start BMP280 setup
  Serial.print("Booting BMP280.");
  lcd.setCursor(0, 1);
  lcd.print("Booting BMP280...");

  // Initialize the BMP280 sensor (temperature and pressure)
  if (!bmp.begin(0x76)) {  // Check if the BMP280 sensor is connected
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    lcd.setCursor(0, 1);
    lcd.print("BMP280 init failed");
    while (1);  // Stop program if sensor initialization fails
  }

  delay(3000);

  // BMP280 started successfully
  Serial.print("BMP280 started.");

  // Show startup message on the LCD
  lcd.setCursor(0, 0);
  lcd.print("Started.");
  delay(1000);
  lcd.clear();

  startMillis = millis();  // Record the start time
}

void readGasSensor() {
  lcd.setCursor(0, 0);  // Set cursor to the first line on LCD
  if (ccs.available()) {
    if (!ccs.readData()) {
      co2ppm = ccs.geteCO2();  // Get CO2 concentration
      tvocppm = ccs.getTVOC();  // Get TVOC concentration

      // Normalize CO2 and TVOC values to a common scale (0-100)
      float normalizedCO2 = map(co2ppm, 400, 2000, 0, 100);
      float normalizedTVOC = map(tvocppm, 0, 1000, 0, 100);

      // Calculate a combined air quality index
      airQualityIndex = (normalizedCO2 + normalizedTVOC) / 2;

      // Display air quality on LCD based on the index
      if (airQualityIndex < 25) {
        lcd.print("Air Quality: Good.  ");
      } else if (airQualityIndex >= 25 && airQualityIndex < 50) {
        lcd.print("Air Quality: Mod.   ");
      } else if (airQualityIndex >= 50 && airQualityIndex < 75) {
        lcd.print("Air Quality: Poor   ");
      } else {
        lcd.print("Air Quality: Hazard ");
      }

      // Print air quality data to Serial Monitor
      Serial.print("CO2: ");
      Serial.print(co2ppm);
      Serial.print(" ppm, TVOC: ");
      Serial.print(tvocppm);
      Serial.print(" ppb, Air Quality Index: ");
      Serial.println(airQualityIndex);
    } else {
      Serial.println("ERROR reading CO2 and TVOC!");
      lcd.print("Error reading sensors");
    }
  } else {
    Serial.println("No data available from CCS811 sensor");
  }
}

void readTemperatureSensor() {
  lcd.setCursor(0, 1);  // Set cursor to the second line
  temperature = bmp.readTemperature();  // Read temperature
  if (!isnan(temperature)) {  // Check if temperature reading is valid
    // Display temperature on LCD
    lcd.print("Temp: ");
    lcd.print(temperature, 1);
    lcd.print(" *C");

    // Print temperature reading to Serial Monitor
    Serial.print("Temperature = ");
    Serial.print(temperature);
    Serial.println(" *C");
  } else {
    Serial.println("Failed to read temperature from BMP280 sensor");
    lcd.print("Error reading Temp");
  }
}

void readPressureSensor() {
  lcd.setCursor(0, 3);  // Set cursor to the fourth line
  pressure = bmp.readPressure() / 100;  // Read pressure
  humidity = (pressure / 1000.0F) * 10;

  if (!isnan(pressure)) {  // Check if pressure reading is valid
    altitude = bmp.readAltitude(1013.25);  // Calculate altitude

    // Display altitude data on LCD
    lcd.print("Altitude: ");
    lcd.print(altitude, 1);
    lcd.print(" m ");

    // Print pressure and altitude readings to Serial Monitor
    Serial.print("Pressure = ");
    Serial.print(pressure);
    Serial.println(" hPa");
    Serial.print("Altitude = ");
    Serial.print(altitude);
    Serial.println(" meters");
  } else {
    Serial.println("Failed to read pressure from BMP280 sensor");
    lcd.print("Error reading Press");
  }
}

void readLightSensor() {
  lcd.setCursor(0, 2);  // Set cursor to the third line
  int rawLightValue = analogRead(lightSensorPin);  // Read light level

  if (rawLightValue >= 0) {  // Check if light sensor value is valid
    // Apply a simple moving average for smoothing
    smoothedLightValue = (smoothedLightValue * 0.9) + (rawLightValue * 0.1);

    // Determine light level category based on the sensor value
    if (smoothedLightValue < 300) {
      lightLevelDescription = "Dark";
    } else if (smoothedLightValue >= 300 && smoothedLightValue < 600) {
      lightLevelDescription = "Dim";
    } else if (smoothedLightValue >= 600 && smoothedLightValue < 900) {
      lightLevelDescription = "Bright";
    } else {
      lightLevelDescription = "V. Bright";
    }

    // Display light level on LCD
    lcd.print("Light: ");
    lcd.print(lightLevelDescription);

    // Print light level to Serial Monitor
    Serial.print("Light level: ");
    Serial.print(smoothedLightValue);
    Serial.print(" - ");
    Serial.println(lightLevelDescription);
  } else {
    Serial.println("Failed to read light level");
    lcd.print("Error reading Light");
  }
}

void displayDataSendStatus(int httpResponseCode) {
  lcd.clear();  // Clear the LCD screen
  lcd.setCursor(0, 0);
  lcd.print("Device ID: ");
  lcd.print(getESP32ID());  // Display device ID on the first line
  lcd.setCursor(0, 1);
  lcd.print("ID: ");
  lcd.print(getESP32ID());  // Display same device ID as an example (second line)
  lcd.setCursor(0, 2);
  lcd.print(getCurrentTimestamp());  // Display current timestamp on third line
  lcd.setCursor(0, 3);
  lcd.print("Status: ");
  lcd.print(httpResponseCode);  // Display HTTP response code on fourth line
}

void sendDataToServer() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Specify the URL
    http.begin(SERVER_URL);

    // Specify the content type and make the request
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    // Create the HTTP POST data string with sensor values
    String httpRequestData = "temperature=" + String(temperature, 1) +
                             "&humidity=" + String(humidity, 1) +
                             "&co2=" + String(co2ppm) +
                             "&tvoc=" + String(tvocppm) +
                             "&lightLevel=" + String(smoothedLightValue) +
                             "&API_KEY=" + String(API_KEY);

    // Send the POST request
    int httpResponseCode = http.POST(httpRequestData);

    // Print HTTP response code to the console
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);  // Print the HTTP response code to the console

    displayDataSendStatus(httpResponseCode);

    // Check the HTTP response code and display success message if 200
    if (httpResponseCode > 0) {
      if (httpResponseCode == 200) {

        // Print success message to Serial
        Serial.println("Data sent successfully.");
      } else {
        // Print error message to Serial
        Serial.print("Error on sending POST: ");
        Serial.println(httpResponseCode);
      }
    } else {
      // Print error message to Serial
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }

    // Wait for 3 seconds to show the send status on the screen
    delay(3000);

    // Display sensor data again after showing send status
    lcd.clear();
    readGasSensor();
    readTemperatureSensor();
    readLightSensor();
    readPressureSensor();

    // Close the connection
    http.end();
  } else {
    // If Wi-Fi is disconnected, display the message on Serial and LCD
    Serial.println("WiFi Disconnected");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Disconnected");
  }
}

void loop() {
  currentMillis = millis();  // Get the current time

  // Read data from all sensors
  readGasSensor();
  readTemperatureSensor();
  readLightSensor();
  readPressureSensor();

  // Send data to server immediately upon startup
  if (firstTime) {
    sendDataToServer();  // Send data immediately upon startup
    startMillis = currentMillis;
    firstTime = false;  // Set firstTime to false
  } else if ((currentMillis - startMillis) > period) {
    // Send data to server every 'period' milliseconds
    sendDataToServer();
    startMillis = currentMillis;
  }

  delay(period);  // Wait 5 minutes before updating sensor readings again
  lcd.clear();
}




