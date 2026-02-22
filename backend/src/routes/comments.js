const express = require('express');
const router = express.Router();
const commentController = require('../controllers/commentController');
const { authenticate } = require('../middleware/auth');

router.post('/', authenticate, commentController.createComment);
router.get('/post/:postId', commentController.getComments); // Public
router.delete('/:commentId', authenticate, commentController.deleteComment);

module.exports = router;