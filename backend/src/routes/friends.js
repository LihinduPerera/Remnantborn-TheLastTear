const express = require('express');
const router = express.Router();
const friendController = require('../controllers/friendController');
const { authenticate } = require('../middleware/auth');

router.post('/request', authenticate, friendController.sendFriendRequest);
router.post('/respond', authenticate, friendController.respondToRequest);
router.get('/', authenticate, friendController.getFriends);
router.get('/pending', authenticate, friendController.getPendingRequests);
router.delete('/:friendId', authenticate, friendController.removeFriend);

module.exports = router;