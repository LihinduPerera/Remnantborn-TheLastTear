// src/controllers/postController.js (fixed version with dynamic select and removed !inner)
const { supabase } = require('../config/database');
const ApiResponse = require('../utils/response');

class PostController {
  // Create a new post
  async createPost(req, res) {
    try {
      const { content, attachments } = req.body;
      const author_id = req.user.id;
    
      if (!content || content.trim().length === 0) {
        return ApiResponse.error(res, 'Post content cannot be empty', 400);
      }
    
      if (content.length > 5000) {
        return ApiResponse.error(res, 'Post too long (max 5000 characters)', 400);
      }
    
      const postData = {
        author_id,
        content: content.trim(),
        attachments: attachments || [],
        created_at: new Date().toISOString(),
        likes: 0
      };
    
      const { data: post, error } = await supabase
        .from('posts')
        .insert([postData])
        .select(`
          *,
          author:profiles!posts_author_id_fkey(username, avatar_url)
        `)
        .single();
    
      if (error) {
        return ApiResponse.error(res, `Failed to create post: ${error.message}`, 400);
      }
    
      // Format response
      const response = {
        id: post.id,
        author_id: post.author_id,
        author_username: post.author?.username,
        author_avatar: post.author?.avatar_url,
        content: post.content,
        attachments: post.attachments,
        likes: post.likes,
        created_at: post.created_at
      };
    
      return ApiResponse.created(res, response, 'Post created successfully');
    
    } catch (error) {
      console.error('Create post error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }

  // Get posts with pagination
  async getPosts(req, res) {
    try {
      const { limit = 20, offset = 0, author_id, sort_by = 'newest' } = req.query;
    
      let selectStr = `
        id,
        author_id,
        content,
        attachments,
        likes,
        created_at,
        author:profiles!posts_author_id_fkey(username, avatar_url, level),
        comments:comments(count)
      `;
    
      if (req.user) {
        selectStr += `,
        user_like:post_likes(user_id)`;
      }
    
      let query = supabase
        .from('posts')
        .select(selectStr, { count: 'exact' });
    
      if (req.user) {
        query = query.eq('post_likes.user_id', req.user.id);
      }
    
      // Filter by author if specified
      if (author_id) {
        query = query.eq('author_id', author_id);
      }
    
      // Apply sorting
      if (sort_by === 'newest') {
        query = query.order('created_at', { ascending: false });
      } else if (sort_by === 'popular') {
        query = query.order('likes', { ascending: false });
      } else {
        query = query.order('created_at', { ascending: false });
      }
    
      const { data: posts, error, count } = await query
        .range(offset, offset + limit - 1);
    
      if (error) {
        return ApiResponse.error(res, `Failed to fetch posts: ${error.message}`, 400);
      }
    
      // Format posts
      const formattedPosts = posts?.map(post => ({
        id: post.id,
        author_id: post.author_id,
        author_username: post.author?.username,
        author_avatar: post.author?.avatar_url,
        author_level: post.author?.level,
        content: post.content,
        attachments: post.attachments,
        likes: post.likes,
        comments_count: post.comments?.[0]?.count || 0,
        user_liked: req.user ? (post.user_like?.length > 0) : false,
        created_at: post.created_at
      })) || [];
    
      return ApiResponse.success(res, {
        posts: formattedPosts,
        pagination: {
          total: count || 0,
          limit: parseInt(limit),
          offset: parseInt(offset),
          has_more: (offset + posts?.length) < (count || 0)
        }
      }, 'Posts retrieved successfully');
    
    } catch (error) {
      console.error('Get posts error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }

  // Like a post
  async likePost(req, res) {
    try {
      const { postId } = req.params;
      const userId = req.user.id;
    
      if (!postId) {
        return ApiResponse.error(res, 'Post ID is required', 400);
      }
    
      // Check if post exists
      const { data: post } = await supabase
        .from('posts')
        .select('id, likes')
        .eq('id', postId)
        .single();
    
      if (!post) {
        return ApiResponse.notFound(res, 'Post not found');
      }
    
      // Check if already liked
      const { data: existingLike } = await supabase
        .from('post_likes')
        .select('id')
        .eq('post_id', postId)
        .eq('user_id', userId)
        .single();
    
      if (existingLike) {
        // Unlike
        const { error: unlikeError } = await supabase
          .from('post_likes')
          .delete()
          .eq('post_id', postId)
          .eq('user_id', userId);
      
        if (unlikeError) {
          return ApiResponse.error(res, 'Failed to unlike post', 500);
        }
      
        // Decrement likes count
        await supabase
          .from('posts')
          .update({ likes: post.likes - 1 })
          .eq('id', postId);
      
        return ApiResponse.success(res, { liked: false, likes: post.likes - 1 }, 'Post unliked successfully');
      } else {
        // Like
        const { error: likeError } = await supabase
          .from('post_likes')
          .insert({
            post_id: postId,
            user_id: userId,
            created_at: new Date().toISOString()
          });
      
        if (likeError) {
          return ApiResponse.error(res, 'Failed to like post', 500);
        }
      
        // Increment likes count
        await supabase
          .from('posts')
          .update({ likes: post.likes + 1 })
          .eq('id', postId);
      
        return ApiResponse.success(res, { liked: true, likes: post.likes + 1 }, 'Post liked successfully');
      }
    
    } catch (error) {
      console.error('Like post error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }

  // Delete a post
  async deletePost(req, res) {
    try {
      const { postId } = req.params;
      const userId = req.user.id;
    
      if (!postId) {
        return ApiResponse.error(res, 'Post ID is required', 400);
      }
    
      // Check if post exists and user is author
      const { data: post } = await supabase
        .from('posts')
        .select('author_id')
        .eq('id', postId)
        .single();
    
      if (!post) {
        return ApiResponse.notFound(res, 'Post not found');
      }
    
      if (post.author_id !== userId) {
        return ApiResponse.forbidden(res, 'You can only delete your own posts');
      }
    
      // Delete post
      const { error } = await supabase
        .from('posts')
        .delete()
        .eq('id', postId);
    
      if (error) {
        return ApiResponse.error(res, 'Failed to delete post', 500);
      }
    
      return ApiResponse.success(res, { success: true }, 'Post deleted successfully');
    
    } catch (error) {
      console.error('Delete post error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }

  // Get single post
  async getPost(req, res) {
    try {
      const { postId } = req.params;
    
      if (!postId) {
        return ApiResponse.error(res, 'Post ID is required', 400);
      }
    
      let selectStr = `
        id,
        author_id,
        content,
        attachments,
        likes,
        created_at,
        author:profiles!posts_author_id_fkey(username, avatar_url, level),
        comments:comments(count),
        post_likes:post_likes(count)
      `;
    
      let query = supabase
        .from('posts')
        .select(selectStr)
        .eq('id', postId)
        .single();
    
      const { data: post, error } = await query;
    
      if (error || !post) {
        return ApiResponse.notFound(res, 'Post not found');
      }
    
      const response = {
        id: post.id,
        author_id: post.author_id,
        author_username: post.author?.username,
        author_avatar: post.author?.avatar_url,
        author_level: post.author?.level,
        content: post.content,
        attachments: post.attachments,
        likes: post.likes,
        comments_count: post.comments?.[0]?.count || 0,
        total_likes: post.post_likes?.[0]?.count || 0,
        created_at: post.created_at
      };
    
      return ApiResponse.success(res, response, 'Post retrieved successfully');
    
    } catch (error) {
      console.error('Get post error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }
}

module.exports = new PostController();