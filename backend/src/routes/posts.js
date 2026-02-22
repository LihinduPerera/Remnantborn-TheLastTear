const express = require('express');
const router = express.Router();
const postController = require('../controllers/postController');
const { authenticate, optionalAuthenticate } = require('../middleware/auth');

// Public routes (optional auth for like status)
router.get('/', optionalAuthenticate, postController.getPosts);
router.get('/:postId', optionalAuthenticate, postController.getPost);

// Protected routes
router.post('/', authenticate, postController.createPost);
router.post('/:postId/like', authenticate, postController.likePost);
router.delete('/:postId', authenticate, postController.deletePost);

module.exports = router;