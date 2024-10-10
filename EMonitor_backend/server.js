// server.js

// Load environment variables from the .env file
require('dotenv').config();

// Import the express library to create a web server
const express = require('express');

// Import the body-parser library to parse request data
const bodyParser = require('body-parser');

// Import the http module to create an HTTP server
const http = require('http');

// Import the socket.io library for real-time communication, which helps to update the live view
const { Server } = require('socket.io');

// Import database connection and models
const connectDB = require('./database/db');
const { SensorData } = require('./database/models');

// Create an express app
const app = express();
const server = http.createServer(app);
const io = new Server(server);

// Middleware to parse request data
app.use(bodyParser.urlencoded({ extended: true }));
app.use(bodyParser.json());

// Set EJS as the view engine
app.set('view engine', 'ejs');

// Connect to MongoDB
connectDB();

// POST route to add sensor data to the database
app.post('/add-sensor-data', async (req, res) => {
  try {
    if (req.body.EMONITOR_API_KEY !== process.env.SECRET_KEY) {
      return res.status(401).send('Unauthorized');
    }

    const newSensorData = new SensorData({
      temperature: parseFloat(req.body.temperature) || 0,
      co2: parseFloat(req.body.co2) || 0,
      tvoc: parseFloat(req.body.tvoc) || 0,
      lightLevel: parseFloat(req.body.lightLevel) || 0,
      timestamp: new Date()
    });

    await newSensorData.save();

    // Emit the latest data to all connected clients
    io.emit('sensorData', newSensorData);

    res.status(200).send('Sensor data saved successfully!');
  } catch (err) {
    res.status(500).send('Failed to save sensor data: ' + err.message);
  }
});

// Route to get all dates with available sensor data
app.get('/data-dates', async (req, res) => {
  try {
    const dates = await SensorData.aggregate([
      {
        $group: {
          _id: {
            $dateToString: { format: "%Y-%m-%d", date: "$timestamp" }
          }
        }
      },
      { $sort: { _id: 1 } }
    ]);

    // Return the array of dates in YYYY-MM-DD format
    res.json(dates.map(date => date._id));
  } catch (err) {
    res.status(500).json({ message: 'Error fetching dates with data' });
  }
});

// Route to display the live view
app.get('/', (req, res) => {
  res.render('live'); // Render the live view
});

// Route to provide the latest sensor data in JSON format (for testing or fallback)
app.get('/latest-sensor-data', async (req, res) => {
  try {
    const latestSensorData = await SensorData.findOne().sort({ timestamp: -1 });

    if (latestSensorData) {
      // Return the data in JSON format
      res.json({
        temperature: latestSensorData.temperature,
        co2: latestSensorData.co2,
        tvoc: latestSensorData.tvoc,
        lightLevel: latestSensorData.lightLevel,
        timestamp: latestSensorData.timestamp
      });
    } else {
      res.json({ message: 'No sensor data available' });
    }
  } catch (err) {
    res.status(500).json({ message: 'Error fetching latest sensor data' });
  }
});

// Route to display sensor data history
app.get('/history', async (req, res) => {
  try {
    const selectedDate = req.query.date || new Date().toISOString().slice(0, 10); //
    const startOfDay = new Date(selectedDate);
    const endOfDay = new Date(selectedDate);
    endOfDay.setDate(startOfDay.getDate() + 1);

    const sensorData = await SensorData.find({
      timestamp: { $gte: startOfDay, $lt: endOfDay }
    }).sort({ timestamp: 1 });

    res.render('history', { sensorData, selectedDate });
  } catch (err) {
    res.status(500).send('Error retrieving sensor data: ' + err.message);
  }
});

// Start the server on the specified port
const PORT = process.env.PORT || 3000;
server.listen(PORT, () => {
  console.log(`Server is running on port ${PORT}`);
});
