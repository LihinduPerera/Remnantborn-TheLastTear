const { supabase } = require('../config/database');
const ApiResponse = require('../utils/response');

class CommentController {
  async createComment(req, res) {
    try {
      const { postId, content } = req.body;
      const userId = req.user.id;

      if (!postId || !content || content.trim().length === 0) {
        return ApiResponse.error(res, 'Post ID and content are required', 400);
      }

      if (content.length > 2000) {
        return ApiResponse.error(res, 'Comment too long (max 2000 characters)', 400);
      }

      // Check if post exists
      const { data: post } = await supabase.from('posts').select('id').eq('id', postId).single();
      if (!post) {
        return ApiResponse.notFound(res, 'Post not found');
      }

      const commentData = {
        post_id: postId,
        user_id: userId,
        content: content.trim(),
        created_at: new Date().toISOString()
      };

      const { data: comment, error } = await supabase
        .from('comments')
        .insert([commentData])
        .select(`
          *,
          author:profiles!comments_user_id_fkey(username, avatar_url)
        `)
        .single();

      if (error) {
        return ApiResponse.error(res, `Failed to create comment: ${error.message}`, 400);
      }

      const response = {
        id: comment.id,
        post_id: comment.post_id,
        user_id: comment.user_id,
        author_username: comment.author?.username,
        author_avatar: comment.author?.avatar_url,
        content: comment.content,
        created_at: comment.created_at
      };

      return ApiResponse.created(res, response, 'Comment created successfully');
    } catch (error) {
      console.error('Create comment error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }

  async getComments(req, res) {
    try {
      const { postId } = req.params;
      const { limit = 20, offset = 0 } = req.query;

      if (!postId) {
        return ApiResponse.error(res, 'Post ID is required', 400);
      }

      const { data: comments, error, count } = await supabase
        .from('comments')
        .select(`
          *,
          author:profiles!comments_user_id_fkey(username, avatar_url)
        `, { count: 'exact' })
        .eq('post_id', postId)
        .order('created_at', { ascending: true })
        .range(offset, offset + limit - 1);

      if (error) {
        return ApiResponse.error(res, `Failed to fetch comments: ${error.message}`, 400);
      }

      const formattedComments = comments?.map(c => ({
        id: c.id,
        post_id: c.post_id,
        user_id: c.user_id,
        author_username: c.author?.username,
        author_avatar: c.author?.avatar_url,
        content: c.content,
        created_at: c.created_at
      })) || [];

      return ApiResponse.success(res, {
        comments: formattedComments,
        pagination: {
          total: count || 0,
          limit: parseInt(limit),
          offset: parseInt(offset),
          has_more: (offset + comments?.length) < (count || 0)
        }
      }, 'Comments retrieved successfully');
    } catch (error) {
      console.error('Get comments error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }

  async deleteComment(req, res) {
    try {
      const { commentId } = req.params;
      const userId = req.user.id;

      if (!commentId) {
        return ApiResponse.error(res, 'Comment ID is required', 400);
      }

      const { data: comment } = await supabase
        .from('comments')
        .select('user_id')
        .eq('id', commentId)
        .single();

      if (!comment) {
        return ApiResponse.notFound(res, 'Comment not found');
      }

      if (comment.user_id !== userId) {
        return ApiResponse.forbidden(res, 'You can only delete your own comments');
      }

      const { error } = await supabase.from('comments').delete().eq('id', commentId);

      if (error) {
        return ApiResponse.error(res, 'Failed to delete comment', 500);
      }

      return ApiResponse.success(res, { success: true }, 'Comment deleted successfully');
    } catch (error) {
      console.error('Delete comment error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }
}

module.exports = new CommentController();