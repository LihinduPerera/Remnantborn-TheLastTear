const express = require('express');
const router = express.Router();
const purchaseController = require('../controllers/purchaseController');
const { authenticate } = require('../middleware/auth');

// Protected routes
router.get('/:userId', authenticate, purchaseController.getPurchases);
router.post('/', authenticate, purchaseController.createPurchase);
router.post('/check-ownership/:userId', authenticate, purchaseController.checkOwnership);
router.get('/history/:userId', authenticate, purchaseController.getPurchaseHistory);

module.exports = router;