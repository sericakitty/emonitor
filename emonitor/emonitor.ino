// Description: This file contains the main code for the ESP32-based environmental monitoring system.

// WIFI_SSID
// WIFI_PASSWORD
// EMONITOR_API_KEY
// SERVER_URL
// OPENWEATHERMAP_API_KEY
// CITY_NAME
// UNITS
#include "config.h" // create a config.h file with the above content

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

float externalTemperature = 0.0;  // Variable to store external temperature
float seaLevelPressure = 1013.25;  // Default sea level pressure in hPa

long gmtOffset_sec = 2 * 3600;  // GMT offset for Finland (UTC+2 hours) // change to your timezone
long daylightOffset_sec = 3600; // Daylight saving time offset (1 hour) // change to your timezone

void setup() {
  // Start serial communication
  Serial.begin(115200);
  Serial.println("In setup");

  // Initialize the LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Starting...");
  Serial.println("LCD initialized.");

  delay(1000);  // Delay to allow the message to be read

  // Initialize the gas sensor (CCS811)
  lcd.clear();
  lcd.print("Init CCS811...");
  Serial.println("Initializing CCS811 sensor...");
  if (!ccs.begin()) {
    Serial.println("Failed to start CCS811 sensor! Please check your wiring.");
    lcd.print("CCS811 init failed");
    while (1);  // Stop program if sensor initialization fails
  }
  lcd.clear();
  lcd.print("CCS811 init OK");
  Serial.println("CCS811 initialized successfully.");

  delay(1000);  // Delay to allow the message to be read

  // Wait for the CCS811 sensor to be ready
  lcd.clear();
  lcd.print("Waiting CCS811...");
  Serial.println("Waiting for CCS811 sensor to be ready...");
  while (!ccs.available()) {
    delay(100);
  }
  lcd.clear();
  lcd.print("CCS811 ready");
  Serial.println("CCS811 sensor is ready.");

  delay(1000);  // Delay to allow the message to be read

  // Initialize Wi-Fi
  lcd.clear();
  lcd.print("Connecting WiFi");
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int wifiAttempt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    lcd.setCursor(0, 1);
    lcd.print("Attempt ");
    lcd.print(++wifiAttempt);
    Serial.print(".");
  }
  lcd.clear();
  lcd.print("WiFi connected");
  Serial.println("\nConnected to WiFi");

  delay(1000);  // Delay to allow the message to be read

  // Configure time using NTP
  lcd.clear();
  lcd.print("Configuring time");
  Serial.println("Configuring time...");
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov"); // Set the time via NTP

  delay(1000);  // Delay to allow the message to be read

  Serial.println("Time configured");
  lcd.clear();
  lcd.print("Time configured");

  delay(1000);  // Delay to allow the message to be read

  // Start BMP280 setup
  lcd.clear();
  lcd.print("Init BMP280...");
  Serial.println("Initializing BMP280 sensor...");
  // Initialize the BMP280 sensor (temperature and pressure)
  if (!bmp.begin(0x76)) {  // Check if the BMP280 sensor is connected
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    lcd.clear();
    lcd.print("BMP280 init failed");
    while (1);  // Stop program if sensor initialization fails
  }
  lcd.clear();
  lcd.print("BMP280 init OK");
  Serial.println("BMP280 initialized successfully.");

  delay(1000);  // Delay to allow the message to be read

  // Fetch weather data from OpenWeatherMap
  lcd.clear();
  lcd.print("Fetching weather");
  Serial.println("Fetching weather data...");
  fetchWeatherData();  // Fetch weather data from OpenWeatherMap
  lcd.clear();
  lcd.print("Weather data OK");
  Serial.println("Weather data fetched.");

  delay(1000);  // Delay to allow the message to be read

  // Setup complete
  lcd.clear();
  lcd.print("Setup complete");
  Serial.println("Setup complete.");

  Serial.println();

  delay(1000);  // Delay to allow the message to be read

  lcd.clear();

  startMillis = millis();  // Record the start time
}

// Function to update LCD with sensor data using descriptive text
void updateDisplayWithSensorData() {
  lcd.clear();
  lcd.setCursor(0, 0);

  // Display CO2 concentration and air quality description
  if (airQualityIndex < 25) {
    lcd.print("Air Quality: Good");
  } else if (airQualityIndex >= 25 && airQualityIndex < 50) {
    lcd.print("Air Quality: Mod.");
  } else if (airQualityIndex >= 50 && airQualityIndex < 75) {
    lcd.print("Air Quality: Poor");
  } else {
    lcd.print("Air Quality: Hazard");
  }

  // Display temperature in Celsius
  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  lcd.print(temperature, 1);
  lcd.print(" *C");

  // Display light level description
  lcd.setCursor(0, 2);
  lcd.print("Light: ");
  lcd.print(lightLevelDescription);

  // Display altitude information
  lcd.setCursor(0, 3);
  lcd.print("Altitude: ");
  lcd.print(altitude, 1);
  lcd.print(" m");
}


// Function to get the ESP32 unique ID
String getESP32ID() {
  uint64_t chipid = ESP.getEfuseMac();  // Get ESP32 chip ID (essentially its MAC address)
  String id = String((uint16_t)(chipid >> 32), HEX) + String((uint32_t)chipid, HEX);
  id.toUpperCase();  // Convert to uppercase for better readability
  return id;
}

// Function to get the current timestamp with Finnish timezone
String getCurrentTimestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Failed to get time";  // Return an error message if time is not available
  }

  char timeStringBuff[30];  // Buffer to hold formatted timestamp
  // Format the timestamp to include the timezone (%Z prints timezone abbreviation)
  strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d %H:%M", &timeinfo);

  return String(timeStringBuff);
}


void fetchWeatherData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Construct the OpenWeatherMap API URL
    String weatherApiUrl = "http://api.openweathermap.org/data/2.5/weather?q=" + String(CITY_NAME) +
                           "&units=" + String(UNITS) + "&appid=" + String(OPENWEATHERMAP_API_KEY);

    http.begin(weatherApiUrl);  // Initialize HTTPClient with the API URL

    int httpResponseCode = http.GET();  // Send the GET request

    if (httpResponseCode > 0) {
      String payload = http.getString();  // Get the response payload

      // Manually parse the JSON response to extract temperature and pressure
      int tempIndex = payload.indexOf("\"temp\":");
      int pressureIndex = payload.indexOf("\"pressure\":");

      if (tempIndex != -1 && pressureIndex != -1) {
        // Extract temperature
        int tempStart = tempIndex + 7;  // Length of "\"temp\":"
        int tempEnd = payload.indexOf(",", tempStart);
        String tempString = payload.substring(tempStart, tempEnd);
        externalTemperature = tempString.toFloat();

        // Extract pressure
        int pressureStart = pressureIndex + 11;  // Length of "\"pressure\":"
        int pressureEnd = payload.indexOf(",", pressureStart);
        if (pressureEnd == -1) {
          pressureEnd = payload.indexOf("}", pressureStart);  // Handle case when pressure is at the end
        }
        String pressureString = payload.substring(pressureStart, pressureEnd);
        seaLevelPressure = pressureString.toFloat();

        // Debug output
        Serial.print("External Temperature: ");
        Serial.print(externalTemperature);
        Serial.println(" °C");

        Serial.print("Sea Level Pressure: ");
        Serial.print(seaLevelPressure);
        Serial.println(" hPa");
      } else {
        Serial.println("Failed to parse weather data.");
      }
    } else {
      Serial.print("Error fetching weather data: ");
      Serial.println(httpResponseCode);
    }

    http.end();  // Close the HTTP connection
  } else {
    Serial.println("WiFi Disconnected");
  }
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

float calculateAltitude(float localPressure, float seaLevelPressure, float temp) {
  // Constants
  const float R = 287.05;  // Specific gas constant for dry air [J/(kg·K)]
  const float g = 9.80665;  // Acceleration due to gravity [m/s²]
  const float L = 0.0065;  // Temperature lapse rate [K/m]

  // Convert temperature to Kelvin
  float tempK = temp + 273.15;

  // Calculate altitude
  float exponent = (R * L) / g;
  float ratio = localPressure / seaLevelPressure;
  float altitude = (tempK / L) * (1 - pow(ratio, exponent));

  return altitude;
}

void readPressureSensor() {
  lcd.setCursor(0, 3);  // Set cursor to the fourth line
  pressure = bmp.readPressure() / 100;  // Read local pressure in hPa

  if (!isnan(pressure)) {  // Check if pressure reading is valid
    // Calculate altitude using external temperature and sea level pressure
    altitude = calculateAltitude(pressure, seaLevelPressure, externalTemperature);

    // Display altitude data on LCD
    lcd.print("Altitude: ");
    lcd.print(altitude, 1);
    lcd.print(" m ");

    // Print pressure and altitude readings to Serial Monitor
    Serial.print("Local Pressure = ");
    Serial.print(pressure);
    Serial.println(" hPa");
    Serial.print("External Temperature = ");
    Serial.print(externalTemperature);
    Serial.println(" °C");
    Serial.print("Sea Level Pressure = ");
    Serial.print(seaLevelPressure);
    Serial.println(" hPa");
    Serial.print("Calculated Altitude = ");
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
  lcd.print("Device ID:");
  lcd.setCursor(0, 1);
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
                             "&co2=" + String(co2ppm) +
                             "&tvoc=" + String(tvocppm) +
                             "&lightLevel=" + String(smoothedLightValue) +
                             "&API_KEY=" + String(EMONITOR_API_KEY);

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

    // Update LCD with sensor data after sending
    updateDisplayWithSensorData();

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
  Serial.print("Current time: ");
  Serial.println(getCurrentTimestamp());  // Print the current timestamp to Serial Monitor

  // Fetch weather data periodically (e.g., every hour)
  static unsigned long weatherDataTimer = 0;
  if (currentMillis - weatherDataTimer > 3600000 || weatherDataTimer == 0) {  // Every 1 hour
    fetchWeatherData();
    weatherDataTimer = currentMillis;
  }

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

  Serial.println();
  delay(period);  // Wait 5 minutes before updating sensor readings again
  lcd.clear();
}




