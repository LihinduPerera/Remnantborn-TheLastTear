const express = require('express');
const router = express.Router();
const matchRewardController = require('../controllers/matchRewardController');
const { authenticate } = require('../middleware/auth');

router.post('/reward', authenticate, matchRewardController.submitReward);
router.post('/complete', authenticate, matchRewardController.submitMatchComplete);

module.exports = router;
