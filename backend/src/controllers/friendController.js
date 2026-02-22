const { supabase } = require('../config/database');
const ApiResponse = require('../utils/response');

class FriendController {
  async sendFriendRequest(req, res) {
    try {
      const { friendId } = req.body;
      const userId = req.user.id;

      if (!friendId || friendId === userId) {
        return ApiResponse.error(res, 'Valid friend ID required (cannot be self)', 400);
      }

      // Check if user exists
      const { data: friend } = await supabase.from('profiles').select('user_id').eq('user_id', friendId).single();
      if (!friend) {
        return ApiResponse.notFound(res, 'User not found');
      }

      // Check if request already exists
      const { data: existing } = await supabase
        .from('friends')
        .select('*')
        .or(`and(user_id.eq.${userId},friend_id.eq.${friendId}),and(user_id.eq.${friendId},friend_id.eq.${userId})`)
        .single();

      if (existing) {
        return ApiResponse.conflict(res, 'Friend request or relationship already exists');
      }

      const requestData = {
        user_id: userId,
        friend_id: friendId,
        status: 'pending',
        created_at: new Date().toISOString()
      };

      const { data: request, error } = await supabase
        .from('friends')
        .insert([requestData])
        .select()
        .single();

      if (error) {
        return ApiResponse.error(res, `Failed to send request: ${error.message}`, 400);
      }

      return ApiResponse.created(res, request, 'Friend request sent');
    } catch (error) {
      console.error('Send friend request error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }

  async respondToRequest(req, res) {
    try {
      const { requestId, action } = req.body; // action: 'accept' | 'reject' | 'block'
      const userId = req.user.id;

      if (!requestId || !['accept', 'reject', 'block'].includes(action)) {
        return ApiResponse.error(res, 'Valid request ID and action (accept/reject/block) required', 400);
      }

      const { data: request } = await supabase
        .from('friends')
        .select('*')
        .eq('id', requestId)
        .single();

      if (!request) {
        return ApiResponse.notFound(res, 'Request not found');
      }

      if (request.friend_id !== userId) {
        return ApiResponse.forbidden(res, 'You can only respond to requests sent to you');
      }

      if (request.status !== 'pending') {
        return ApiResponse.error(res, 'Request already processed', 400);
      }

      let updateData;
      if (action === 'accept') {
        updateData = { status: 'accepted' };
      } else if (action === 'reject') {
        updateData = { status: 'rejected' };
      } else if (action === 'block') {
        updateData = { status: 'blocked' };
      }

      const { error } = await supabase
        .from('friends')
        .update({ ...updateData, updated_at: new Date().toISOString() })
        .eq('id', requestId);

      if (error) {
        return ApiResponse.error(res, 'Failed to update request', 500);
      }

      return ApiResponse.success(res, { success: true, status: action === 'accept' ? 'accepted' : action === 'reject' ? 'rejected' : 'blocked' }, 'Request processed');
    } catch (error) {
      console.error('Respond to request error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }

  async getFriends(req, res) {
    try {
      const userId = req.user.id;
      const { limit = 20, offset = 0 } = req.query;

      const { data: friends, error, count } = await supabase
        .from('friends')
        .select(`
          *,
          friend_profile:profiles!friends_friend_id_fkey(username, avatar_url, level)
        `, { count: 'exact' })
        .eq('status', 'accepted')
        .or(`user_id.eq.${userId},friend_id.eq.${userId}`)
        .range(offset, offset + limit - 1);

      if (error) {
        return ApiResponse.error(res, `Failed to fetch friends: ${error.message}`, 400);
      }

      const formattedFriends = friends?.map(f => {
        const isSender = f.user_id === userId;
        return {
          friend_id: isSender ? f.friend_id : f.user_id,
          username: f.friend_profile?.username,
          avatar_url: f.friend_profile?.avatar_url,
          level: f.friend_profile?.level,
          status: f.status
        };
      }) || [];

      return ApiResponse.success(res, {
        friends: formattedFriends,
        pagination: {
          total: count || 0,
          limit: parseInt(limit),
          offset: parseInt(offset),
          has_more: (offset + friends?.length) < (count || 0)
        }
      }, 'Friends retrieved successfully');
    } catch (error) {
      console.error('Get friends error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }

  async getPendingRequests(req, res) {
    try {
      const userId = req.user.id;

      const { data: requests, error } = await supabase
        .from('friends')
        .select(`
          *,
          sender_profile:profiles!friends_user_id_fkey(username, avatar_url)
        `)
        .eq('friend_id', userId)
        .eq('status', 'pending');

      if (error) {
        return ApiResponse.error(res, `Failed to fetch requests: ${error.message}`, 400);
      }

      const formattedRequests = requests?.map(r => ({
        request_id: r.id,
        sender_id: r.user_id,
        sender_username: r.sender_profile?.username,
        sender_avatar: r.sender_profile?.avatar_url,
        status: r.status
      })) || [];

      return ApiResponse.success(res, formattedRequests, 'Pending requests retrieved');
    } catch (error) {
      console.error('Get pending requests error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }

  async removeFriend(req, res) {
    try {
      const { friendId } = req.params;
      const userId = req.user.id;

      if (!friendId) {
        return ApiResponse.error(res, 'Friend ID is required', 400);
      }

      const { error } = await supabase
        .from('friends')
        .delete()
        .or(`and(user_id.eq.${userId},friend_id.eq.${friendId}),and(user_id.eq.${friendId},friend_id.eq.${userId})`);

      if (error) {
        return ApiResponse.error(res, 'Failed to remove friend', 500);
      }

      return ApiResponse.success(res, { success: true }, 'Friend removed successfully');
    } catch (error) {
      console.error('Remove friend error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }
}

module.exports = new FriendController();