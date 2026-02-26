const express = require('express');
const router = express.Router();
const storeController = require('../controllers/storeController');
const { authenticate } = require('../middleware/auth');

router.get('/characters', authenticate, storeController.getCharacters);
router.get('/packages', storeController.getPackages);
router.post('/buy-character', authenticate, storeController.buyCharacter);
router.post('/buy-remnants', authenticate, storeController.buyRemnants);

module.exports = router;
