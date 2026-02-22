const express = require('express');
const router = express.Router();
const eventController = require('../controllers/eventController');
const { authenticate } = require('../middleware/auth');

// Public routes
router.get('/', eventController.getEvents);
router.get('/active', eventController.getActiveEvents);
router.get('/:eventId', eventController.getEvent);

// Protected routes
router.post('/', authenticate, eventController.createEvent);
router.post('/:eventId/join', authenticate, eventController.joinEvent);

module.exports = router;