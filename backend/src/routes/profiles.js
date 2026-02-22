const express = require('express');
const router = express.Router();
const profileController = require('../controllers/profileController');
const { authenticate } = require('../middleware/auth');

// Protected routes
router.get('/:userId', authenticate, profileController.getProfile);
router.put('/:userId', authenticate, profileController.updateProfile);
router.patch('/:userId/game-stats', authenticate, profileController.updateGameStats);
router.get('/:userId/status', authenticate, profileController.getOnlineStatus);

// Public route (search)
router.get('/search/users', profileController.searchUsers);

module.exports = router;