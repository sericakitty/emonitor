### IoT Experimental Project – Final Course Project

## Overview

This project is designed to monitor environmental conditions using a variety of sensors connected to an ESP32 microcontroller. The data collected by the sensors is displayed on an LCD screen and sent to a backend server for further analysis. The backend is built using modern web technologies, providing a robust platform to view and analyze the collected data.

## Hardware Components

1. **ESP32 Microcontroller:**
   - Serves as the primary controller for reading data from sensors and sending it to the backend server.
   - Provides Wi-Fi connectivity for remote data transmission.

2. **CCS811 Air Quality Sensor:**
   - Measures CO2 and Total Volatile Organic Compounds (TVOC) levels in the environment.
   - Outputs CO2 in ppm and TVOC in ppb.

3. **BMP280 Temperature and Pressure Sensor:**
   - Measures ambient temperature and atmospheric pressure.
   - Calculates altitude based on pressure readings.

4. **LDR Light Sensor:**
   - Measures ambient light intensity.
   - Outputs light levels that are used to categorize brightness.

5. **I2C LCD Display:**
   - Displays real-time sensor readings such as air quality, temperature, light level, and altitude.
   - Provides visual feedback on the status of data transmission to the server.

## Backend Technologies

1. **Backend Server:**
   - The server is built using **Node.js** with an **Express** framework for handling HTTP requests.
   - **MongoDB** is used as the database for storing and managing sensor data.

2. **API Endpoint:**
   - Data is sent to a RESTful API endpoint hosted on an **Azure Web App**
   - The server accepts POST requests with sensor data formatted as JSON.
   - **Endpoint needs `apiKey` to verify** the authenticity of the data before storing it in the database.

3. **Data Analysis and Visualization:**
   - The backend uses **D3.js** for data visualization and **Chart.js** to create dynamic charts for analyzing sensor data over time.

## How the System Works

1. The ESP32 reads environmental data from the connected sensors and displays it on the LCD screen.
2. Every 5 minutes, the ESP32 sends the sensor data as a POST request to the backend server.
3. The server processes the incoming data, stores it in the database, and provides it to the frontend for visualization and analysis.

## How to Use

1. Connect the ESP32 and sensors as described in the wiring diagram.
2. Configure the `config.h` file with your Wi-Fi credentials and server API key.
3. Upload the code to the ESP32 using the Arduino IDE or PlatformIO.
4. Once powered on, the device will automatically connect to the Wi-Fi network, collect data from sensors, and send it to the backend server.

## Note

Ensure that the backend server is properly set up and the API endpoint is accessible before running the device. If you encounter any issues, check the Serial Monitor for debugging information and the status of data transmission.

## Future Feature

Currently, the ESP32 device already prints its unique device ID and timestamp to the LCD display and the Serial Monitor when data is sent to the backend. This information can be leveraged to implement a future feature that allows users to register devices under their accounts. The planned enhancement will enable users to view and manage only the devices they have registered, providing a more personalized experience on the web platform.

### Planned Workflow:

1. **Device ID and Timestamp:**
   - The ESP32 device prints its unique device ID and the current timestamp whenever data is sent to the backend.
   - This device ID will be stored in the backend database along with other sensor data, providing a unique identifier for each device.

2. **User Registration and Device Association:**
   - The backend will be updated to include a device registration system where users can enter the ESP32 device ID to register it under their account.
   - Once registered, the device will be associated with the user’s profile in the database.

3. **User Authentication and Device Filtering:**
   - When a user logs into the web application, the frontend will only display devices registered under that user’s account.
   - This ensures that users see only their own devices and associated data, even if multiple devices are connected to the same backend.

4. **Implementation Plan:**
   - Extend the backend API to include an endpoint for device registration using the device ID printed by the ESP32.
   - Implement user authentication and device filtering on the backend and frontend.
   - Update the frontend to fetch and display only the devices associated with the authenticated user.

This feature will allow multiple users to connect their own ESP32 devices to a shared backend while ensuring that each user can view and manage only their registered devices. It enhances security and personalization, making the system more versatile for different use cases.
