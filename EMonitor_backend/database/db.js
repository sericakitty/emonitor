// db.js
// This file contains the database connection function. You can change to make it work with your own database.

const mongoose = require('mongoose');

const { MONGODB_URI } = process.env;

// Function to connect to MongoDB
function connectDB() {
  mongoose.connect(MONGODB_URI)
    .then(() => console.log('Connected to MongoDB'))
    .catch((err) => console.error('Error connecting to MongoDB:', err));
}

module.exports = connectDB;
