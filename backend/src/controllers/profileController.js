const { supabase } = require('../config/database');
const { cloudinary } = require('../config/cloudinary');
const ApiResponse = require('../utils/response');

const TABLE_NOT_FOUND_CODE = '42P01';

async function getRecentMatchHistory(userId, limit = 10) {
  try {
    const { data: participants, error: participantError } = await supabase
      .from('match_participants')
      .select('match_result_id, placement, elimination_order, survival_time_seconds, is_winner, character_id, created_at')
      .eq('user_id', userId)
      .order('created_at', { ascending: false })
      .limit(limit);

    if (participantError) {
      if (participantError.code === TABLE_NOT_FOUND_CODE) {
        return [];
      }
      throw participantError;
    }

    if (!participants || participants.length === 0) {
      return [];
    }

    const matchResultIds = [...new Set(participants.map((entry) => entry.match_result_id).filter(Boolean))];
    if (matchResultIds.length === 0) {
      return [];
    }

    const { data: matches, error: matchError } = await supabase
      .from('match_results')
      .select('id, match_id, map_name, game_mode, ended_at, duration_seconds, is_draw, winner_user_id')
      .in('id', matchResultIds);

    if (matchError) {
      if (matchError.code === TABLE_NOT_FOUND_CODE) {
        return [];
      }
      throw matchError;
    }

    const matchById = new Map((matches || []).map((match) => [match.id, match]));

    return participants.map((participant) => {
      const match = matchById.get(participant.match_result_id) || {};
      const isWinner = Boolean(participant.is_winner);
      return {
        match_id: match.match_id || null,
        map_name: match.map_name || 'UnknownMap',
        game_mode: match.game_mode || 'unknown',
        ended_at: match.ended_at || participant.created_at || null,
        duration_seconds: Number(match.duration_seconds || 0),
        is_draw: Boolean(match.is_draw),
        placement: Number(participant.placement || 0),
        elimination_order: Number(participant.elimination_order || 0),
        survival_time_seconds: Number(participant.survival_time_seconds || 0),
        is_winner: isWinner,
        character_id: participant.character_id || null,
        reward_amount: isWinner ? 15 : 5,
      };
    });
  } catch (error) {
    console.error('Get recent match history error:', error);
    return [];
  }
}

class ProfileController {
  // Get user profile
  async getProfile(req, res) {
    try {
      const { userId } = req.params;
      
      if (!userId) {
        return ApiResponse.error(res, 'User ID is required', 400);
      }
      
      // Get profile with purchases
      const { data: profile, error: profileError } = await supabase
        .from('profiles')
        .select(`
          *,
          purchases (
            item_type,
            item_id,
            purchased_at
          )
        `)
        .eq('user_id', userId)
        .single();
      
      if (profileError || !profile) {
        return ApiResponse.notFound(res, 'Profile not found');
      }
      
      // Format purchased items
      const purchasedItems = profile.purchases?.map(purchase => purchase.item_id) || [];
      const matchHistory = await getRecentMatchHistory(userId, 10);
      
      const response = {
        userId: profile.user_id,
        username: profile.username,
        level: profile.level,
        remnant_count: profile.remnant_count,
        avatar_url: profile.avatar_url,
        bio: profile.bio,
        created_at: profile.created_at,
        last_active: profile.last_active,
        updated_at: profile.updated_at,
        purchased_items: purchasedItems,
        match_history: matchHistory
      };
      
      return ApiResponse.success(res, response, 'Profile retrieved successfully');
      
    } catch (error) {
      console.error('Get profile error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }
  
  // Get current user's profile
  async getMyProfile(req, res) {
    try {
      const userId = req.user.id;
      
      const { data: profile, error: profileError } = await supabase
        .from('profiles')
        .select(`
          *,
          purchases (
            item_type,
            item_id,
            purchased_at
          )
        `)
        .eq('user_id', userId)
        .single();
      
      if (profileError || !profile) {
        return ApiResponse.notFound(res, 'Profile not found');
      }
      
      // Format purchased items
      const purchasedItems = profile.purchases?.map(purchase => purchase.item_id) || [];
      const matchHistory = await getRecentMatchHistory(userId, 10);
      
      const response = {
        userId: profile.user_id,
        username: profile.username,
        level: profile.level,
        remnant_count: profile.remnant_count,
        avatar_url: profile.avatar_url,
        bio: profile.bio,
        created_at: profile.created_at,
        last_active: profile.last_active,
        updated_at: profile.updated_at,
        purchased_items: purchasedItems,
        match_history: matchHistory
      };
      
      return ApiResponse.success(res, response, 'Profile retrieved successfully');
      
    } catch (error) {
      console.error('Get my profile error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }
  
  // Upload avatar
  async uploadAvatar(req, res) {
    try {
      const userId = req.user.id;
      
      if (!req.file) {
        return ApiResponse.error(res, 'No file uploaded', 400);
      }
      
      // Get current profile to check if there's an old avatar
      const { data: profile } = await supabase
        .from('profiles')
        .select('avatar_url')
        .eq('user_id', userId)
        .single();
      
      // Delete old avatar from Cloudinary if it exists and is a Cloudinary URL
      if (profile?.avatar_url && profile.avatar_url.includes('cloudinary')) {
        try {
          const publicId = profile.avatar_url.split('/').pop().split('.')[0];
          await cloudinary.uploader.destroy(`remnantborn/avatars/${publicId}`);
        } catch (deleteError) {
          console.error('Error deleting old avatar:', deleteError);
        }
      }
      
      // Update profile with new avatar URL
      const { error: updateError } = await supabase
        .from('profiles')
        .update({ 
          avatar_url: req.file.path,
          updated_at: new Date().toISOString()
        })
        .eq('user_id', userId);
      
      if (updateError) {
        return ApiResponse.error(res, `Failed to update avatar: ${updateError.message}`, 400);
      }
      
      return ApiResponse.success(res, {
        avatar_url: req.file.path
      }, 'Avatar uploaded successfully');
      
    } catch (error) {
      console.error('Upload avatar error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }
  
  // Update user profile
  async updateProfile(req, res) {
    try {
      const { userId } = req.params;
      const updates = req.body;
      
      if (!userId) {
        return ApiResponse.error(res, 'User ID is required', 400);
      }
      
      // Check if user is updating their own profile
      if (req.user.id !== userId) {
        return ApiResponse.forbidden(res, 'You can only update your own profile');
      }
      
      // Validate update fields
      const allowedUpdates = ['username', 'avatar_url', 'bio'];
      const updateData = {};
      
      for (const key in updates) {
        if (allowedUpdates.includes(key)) {
          updateData[key] = updates[key];
        }
      }
      
      if (Object.keys(updateData).length === 0) {
        return ApiResponse.error(res, 'No valid fields to update', 400);
      }
      
      // Check username uniqueness if updating username
      if (updateData.username) {
        const { data: existing } = await supabase
          .from('profiles')
          .select('user_id')
          .eq('username', updateData.username)
          .neq('user_id', userId)
          .single();
        
        if (existing) {
          return ApiResponse.conflict(res, 'Username already taken');
        }
      }
      
      // Update profile
      updateData.updated_at = new Date().toISOString();
      
      const { error } = await supabase
        .from('profiles')
        .update(updateData)
        .eq('user_id', userId);
      
      if (error) {
        return ApiResponse.error(res, `Failed to update profile: ${error.message}`, 400);
      }
      
      // Get updated profile to return
      const { data: updatedProfile } = await supabase
        .from('profiles')
        .select('username, avatar_url, bio, updated_at')
        .eq('user_id', userId)
        .single();
      
      return ApiResponse.success(res, updatedProfile, 'Profile updated successfully');
      
    } catch (error) {
      console.error('Update profile error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }
  
  // Update game stats (level, currency)
  async updateGameStats(req, res) {
    try {
      const { userId } = req.params;
      const { level, remnant_count, operation = 'set' } = req.body;
      
      if (!userId) {
        return ApiResponse.error(res, 'User ID is required', 400);
      }
      
      const updateData = {};
      
      if (level !== undefined) {
        if (operation === 'increment') {
          // Get current level and increment
          const { data: current } = await supabase
            .from('profiles')
            .select('level')
            .eq('user_id', userId)
            .single();
          
          updateData.level = (current?.level || 0) + level;
        } else {
          updateData.level = level;
        }
      }
      
      if (remnant_count !== undefined) {
        if (operation === 'increment') {
          // Get current remnant count and increment
          const { data: current } = await supabase
            .from('profiles')
            .select('remnant_count')
            .eq('user_id', userId)
            .single();
          
          updateData.remnant_count = (current?.remnant_count || 0) + remnant_count;
        } else if (operation === 'decrement') {
          // Get current remnant count and decrement
          const { data: current } = await supabase
            .from('profiles')
            .select('remnant_count')
            .eq('user_id', userId)
            .single();
          
          const newCount = (current?.remnant_count || 0) - remnant_count;
          if (newCount < 0) {
            return ApiResponse.error(res, 'Insufficient remnant count', 400);
          }
          updateData.remnant_count = newCount;
        } else {
          updateData.remnant_count = remnant_count;
        }
      }
      
      if (Object.keys(updateData).length === 0) {
        return ApiResponse.error(res, 'No game stats provided to update', 400);
      }
      
      const { error } = await supabase
        .from('profiles')
        .update(updateData)
        .eq('user_id', userId);
      
      if (error) {
        return ApiResponse.error(res, `Failed to update game stats: ${error.message}`, 400);
      }
      
      return ApiResponse.success(res, { success: true, ...updateData }, 'Game stats updated successfully');
      
    } catch (error) {
      console.error('Update game stats error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }
  
  // Get user's online status
  async getOnlineStatus(req, res) {
    try {
      const { userId } = req.params;
      
      if (!userId) {
        return ApiResponse.error(res, 'User ID is required', 400);
      }
      
      const { data: profile, error } = await supabase
        .from('profiles')
        .select('last_active, username, avatar_url')
        .eq('user_id', userId)
        .single();
      
      if (error || !profile) {
        return ApiResponse.notFound(res, 'User not found');
      }
      
      // Consider user online if they were active in the last 5 minutes
      const lastActive = new Date(profile.last_active);
      const now = new Date();
      const isOnline = (now - lastActive) < 5 * 60 * 1000;
      
      return ApiResponse.success(res, {
        userId,
        username: profile.username,
        avatar_url: profile.avatar_url,
        last_active: profile.last_active,
        is_online: isOnline
      });
      
    } catch (error) {
      console.error('Get online status error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }
  
  // Search users by username
  async searchUsers(req, res) {
    try {
      const { query, limit = 20, offset = 0 } = req.query;
      
      if (!query || query.length < 2) {
        return ApiResponse.error(res, 'Search query must be at least 2 characters', 400);
      }
      
      const { data: profiles, error } = await supabase
        .from('profiles')
        .select('user_id, username, avatar_url, level, bio')
        .ilike('username', `%${query}%`)
        .range(offset, offset + limit - 1)
        .order('level', { ascending: false });
      
      if (error) {
        return ApiResponse.error(res, `Search failed: ${error.message}`, 400);
      }
      
      return ApiResponse.success(res, {
        users: profiles || [],
        count: profiles?.length || 0
      }, 'Users retrieved successfully');
      
    } catch (error) {
      console.error('Search users error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }
}

module.exports = new ProfileController();