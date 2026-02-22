const express = require('express');
const router = express.Router();
const authController = require('../controllers/authController');
const { authenticate } = require('../middleware/auth');

// Public routes
router.post('/signup', authController.signup);
router.post('/login', authController.login);
router.post('/dev-login', authController.devLogin);

// Protected route
router.post('/verify-token', authenticate, authController.verifyToken);

module.exports = router;