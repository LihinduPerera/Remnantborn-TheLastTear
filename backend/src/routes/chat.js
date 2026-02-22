const express = require('express');
const router = express.Router();
const chatController = require('../controllers/chatController');
const { authenticate } = require('../middleware/auth');

// Protected routes
router.post('/send', authenticate, chatController.sendMessage);
router.get('/', authenticate, chatController.getMessages);
router.get('/conversations', authenticate, chatController.getConversations);
router.get('/channel/:channel', authenticate, chatController.getChannelMessages);

module.exports = router;