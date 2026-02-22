const { supabase } = require('../config/database');
const ApiResponse = require('../utils/response');

class ChatController {
  // Send a message
  async sendMessage(req, res) {
    try {
      const { receiver_id, message, channel = 'private' } = req.body;
      const sender_id = req.user.id;
      
      if (!receiver_id && channel === 'private') {
        return ApiResponse.error(res, 'Receiver ID is required for private messages', 400);
      }
      
      if (!message || message.trim().length === 0) {
        return ApiResponse.error(res, 'Message cannot be empty', 400);
      }
      
      if (message.length > 1000) {
        return ApiResponse.error(res, 'Message too long (max 1000 characters)', 400);
      }
      
      // For private messages, check if receiver exists
      if (receiver_id && channel === 'private') {
        const { data: receiver } = await supabase
          .from('profiles')
          .select('user_id')
          .eq('user_id', receiver_id)
          .single();
        
        if (!receiver) {
          return ApiResponse.notFound(res, 'Receiver not found');
        }
      }
      
      const chatData = {
        sender_id,
        receiver_id: channel === 'private' ? receiver_id : null,
        message: message.trim(),
        channel,
        created_at: new Date().toISOString()
      };
      
      const { data: chat, error } = await supabase
        .from('chats')
        .insert([chatData])
        .select()
        .single();
      
      if (error) {
        return ApiResponse.error(res, `Failed to send message: ${error.message}`, 400);
      }
      
      // Update sender's last activity
      await supabase
        .from('profiles')
        .update({ last_active: new Date().toISOString() })
        .eq('user_id', sender_id);
      
      return ApiResponse.created(res, {
        chat_id: chat.id,
        ...chatData
      }, 'Message sent successfully');
      
    } catch (error) {
      console.error('Send message error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }
  
  // Get messages
  async getMessages(req, res) {
    try {
      const { withUserId, channel, limit = 50, offset = 0, startDate, endDate } = req.query;
      const userId = req.user.id;
      
      let query = supabase
        .from('chats')
        .select(`
          *,
          sender:profiles!chats_sender_id_fkey(username, avatar_url),
          receiver:profiles!chats_receiver_id_fkey(username, avatar_url)
        `, { count: 'exact' })
        .or(`sender_id.eq.${userId},receiver_id.eq.${userId}`)
        .order('created_at', { ascending: false })
        .range(offset, offset + limit - 1);
      
      // Filter by channel
      if (channel) {
        query = query.eq('channel', channel);
      }
      
      // Filter by private conversation
      if (withUserId) {
        query = query.or(`and(sender_id.eq.${userId},receiver_id.eq.${withUserId}),and(sender_id.eq.${withUserId},receiver_id.eq.${userId})`);
      }
      
      // Filter by date range
      if (startDate) {
        query = query.gte('created_at', startDate);
      }
      
      if (endDate) {
        query = query.lte('created_at', endDate);
      }
      
      const { data: messages, error, count } = await query;
      
      if (error) {
        return ApiResponse.error(res, `Failed to fetch messages: ${error.message}`, 400);
      }
      
      // Format response
      const formattedMessages = messages?.map(msg => ({
        id: msg.id,
        sender_id: msg.sender_id,
        sender_username: msg.sender?.username,
        sender_avatar: msg.sender?.avatar_url,
        receiver_id: msg.receiver_id,
        receiver_username: msg.receiver?.username,
        receiver_avatar: msg.receiver?.avatar_url,
        message: msg.message,
        channel: msg.channel,
        created_at: msg.created_at
      })).reverse() || [];
      
      return ApiResponse.success(res, {
        messages: formattedMessages,
        pagination: {
          total: count || 0,
          limit: parseInt(limit),
          offset: parseInt(offset),
          has_more: (offset + messages?.length) < (count || 0)
        }
      }, 'Messages retrieved successfully');
      
    } catch (error) {
      console.error('Get messages error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }
  
  // Get conversation list
  async getConversations(req, res) {
    try {
      const userId = req.user.id;
      
      // Get distinct conversations
      const { data: conversations, error } = await supabase
        .from('chats')
        .select(`
          sender_id,
          receiver_id,
          created_at,
          sender:profiles!chats_sender_id_fkey(username, avatar_url),
          receiver:profiles!chats_receiver_id_fkey(username, avatar_url)
        `)
        .or(`sender_id.eq.${userId},receiver_id.eq.${userId}`)
        .order('created_at', { ascending: false });
      
      if (error) {
        return ApiResponse.error(res, `Failed to fetch conversations: ${error.message}`, 400);
      }
      
      // Group by conversation partner
      const conversationMap = new Map();
      
      conversations?.forEach(msg => {
        const partnerId = msg.sender_id === userId ? msg.receiver_id : msg.sender_id;
        
        if (!partnerId) return; // Skip channel messages
        
        if (!conversationMap.has(partnerId)) {
          const partnerProfile = msg.sender_id === userId ? msg.receiver : msg.sender;
          
          conversationMap.set(partnerId, {
            partner_id: partnerId,
            partner_username: partnerProfile?.username,
            partner_avatar: partnerProfile?.avatar_url,
            last_message: msg.message,
            last_message_at: msg.created_at,
            unread_count: 0 // You'd need to track read status
          });
        }
      });
      
      const conversationList = Array.from(conversationMap.values())
        .sort((a, b) => new Date(b.last_message_at) - new Date(a.last_message_at));
      
      return ApiResponse.success(res, conversationList, 'Conversations retrieved successfully');
      
    } catch (error) {
      console.error('Get conversations error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }
  
  // Get channel messages
  async getChannelMessages(req, res) {
    try {
      const { channel } = req.params;
      const { limit = 100, offset = 0 } = req.query;
      
      if (!channel) {
        return ApiResponse.error(res, 'Channel name is required', 400);
      }
      
      const { data: messages, error } = await supabase
        .from('chats')
        .select(`
          *,
          sender:profiles!chats_sender_id_fkey(username, avatar_url)
        `)
        .eq('channel', channel)
        .is('receiver_id', null)
        .order('created_at', { ascending: false })
        .range(offset, offset + limit - 1);
      
      if (error) {
        return ApiResponse.error(res, `Failed to fetch channel messages: ${error.message}`, 400);
      }
      
      const formattedMessages = messages?.map(msg => ({
        id: msg.id,
        sender_id: msg.sender_id,
        sender_username: msg.sender?.username,
        sender_avatar: msg.sender?.avatar_url,
        message: msg.message,
        channel: msg.channel,
        created_at: msg.created_at
      })).reverse() || [];
      
      return ApiResponse.success(res, formattedMessages, 'Channel messages retrieved successfully');
      
    } catch (error) {
      console.error('Get channel messages error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }
}

module.exports = new ChatController();