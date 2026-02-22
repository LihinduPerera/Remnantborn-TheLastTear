const express = require('express');
const router = express.Router();
const multer = require('multer');
const profileController = require('../controllers/profileController');
const { authenticate } = require('../middleware/auth');
const { avatarStorage } = require('../config/cloudinary');

const upload = multer({ storage: avatarStorage });

// Protected routes
router.get('/:userId', authenticate, profileController.getProfile);
router.get('/me', authenticate, profileController.getMyProfile);
router.put('/:userId', authenticate, profileController.updateProfile);
router.patch('/:userId/game-stats', authenticate, profileController.updateGameStats);
router.get('/:userId/status', authenticate, profileController.getOnlineStatus);

// Upload avatar
router.post('/upload-avatar', authenticate, upload.single('avatar'), profileController.uploadAvatar);

// Public route (search)
router.get('/search/users', profileController.searchUsers);

module.exports = router;