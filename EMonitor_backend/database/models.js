// models.js
// Here you can modify this file to make your own database models

const mongoose = require('mongoose');

// User Schema
const UserSchema = new mongoose.Schema({
  username: { type: String, required: true, unique: true },
  password: { type: String, required: true },  // Password should be hashed in production
});

// Sensor Data Schema
const SensorDataSchema = new mongoose.Schema({
  temperature: Number,
  co2: Number,
  tvoc: Number,
  lightLevel: String,
  timestamp: { type: Date, default: Date.now }
});

// Create models from schemas
const User = mongoose.model('User', UserSchema);
const SensorData = mongoose.model('SensorData', SensorDataSchema);

module.exports = { User, SensorData };
