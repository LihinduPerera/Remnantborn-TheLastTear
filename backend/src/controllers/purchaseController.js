const { supabase } = require('../config/database');
const ApiResponse = require('../utils/response');

class PurchaseController {
  // Get user's purchases
  async getPurchases(req, res) {
    try {
      const { userId } = req.params;
      
      if (!userId) {
        return ApiResponse.error(res, 'User ID is required', 400);
      }
      
      // Check if user is accessing their own purchases
      if (req.user.id !== userId && req.user.role !== 'admin') {
        return ApiResponse.forbidden(res, 'You can only view your own purchases');
      }
      
      const { data: purchases, error } = await supabase
        .from('purchases')
        .select('*')
        .eq('user_id', userId)
        .order('purchased_at', { ascending: false });
      
      if (error) {
        return ApiResponse.error(res, `Failed to fetch purchases: ${error.message}`, 400);
      }
      
      return ApiResponse.success(res, purchases || [], 'Purchases retrieved successfully');
      
    } catch (error) {
      console.error('Get purchases error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }
  
  // Create a new purchase
  async createPurchase(req, res) {
    try {
      const { userId, item_type, item_id, price } = req.body;
      
      if (!userId || !item_type || !item_id) {
        return ApiResponse.error(res, 'User ID, item type, and item ID are required', 400);
      }
      
      // Check if user is making purchase for themselves
      if (req.user.id !== userId) {
        return ApiResponse.forbidden(res, 'You can only make purchases for yourself');
      }
      
      // Check if user already owns this item
      const { data: existingPurchase } = await supabase
        .from('purchases')
        .select('id')
        .eq('user_id', userId)
        .eq('item_id', item_id)
        .single();
      
      if (existingPurchase) {
        return ApiResponse.conflict(res, 'Item already purchased');
      }
      
      // Get user's remnant count
      const { data: profile, error: profileError } = await supabase
        .from('profiles')
        .select('remnant_count')
        .eq('user_id', userId)
        .single();
      
      if (profileError || !profile) {
        return ApiResponse.notFound(res, 'User profile not found');
      }
      
      // If price is provided, check if user has enough currency
      if (price && price > 0) {
        if (profile.remnant_count < price) {
          return ApiResponse.error(res, 'Insufficient remnant count', 400);
        }
        
        // Deduct currency
        const { error: deductError } = await supabase
          .from('profiles')
          .update({ remnant_count: profile.remnant_count - price })
          .eq('user_id', userId);
        
        if (deductError) {
          return ApiResponse.error(res, 'Failed to deduct currency', 500);
        }
      }
      
      // Create purchase record
      const purchaseData = {
        user_id: userId,
        item_type,
        item_id,
        purchased_at: new Date().toISOString(),
        price: price || 0
      };
      
      const { data: purchase, error: purchaseError } = await supabase
        .from('purchases')
        .insert([purchaseData])
        .select()
        .single();
      
      if (purchaseError) {
        return ApiResponse.error(res, `Failed to create purchase: ${purchaseError.message}`, 400);
      }
      
      return ApiResponse.created(res, purchase, 'Purchase completed successfully');
      
    } catch (error) {
      console.error('Create purchase error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }
  
  // Check if user owns specific items
  async checkOwnership(req, res) {
    try {
      const { userId } = req.params;
      const { item_ids } = req.body;
      
      if (!userId || !item_ids || !Array.isArray(item_ids)) {
        return ApiResponse.error(res, 'User ID and item_ids array are required', 400);
      }
      
      const { data: purchases, error } = await supabase
        .from('purchases')
        .select('item_id')
        .eq('user_id', userId)
        .in('item_id', item_ids);
      
      if (error) {
        return ApiResponse.error(res, `Failed to check ownership: ${error.message}`, 400);
      }
      
      const ownedItems = purchases?.map(p => p.item_id) || [];
      const ownershipMap = {};
      
      item_ids.forEach(itemId => {
        ownershipMap[itemId] = ownedItems.includes(itemId);
      });
      
      return ApiResponse.success(res, {
        user_id: userId,
        ownership: ownershipMap,
        owned_items: ownedItems
      }, 'Ownership check completed');
      
    } catch (error) {
      console.error('Check ownership error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }
  
  // Get purchase history with pagination
  async getPurchaseHistory(req, res) {
    try {
      const { userId } = req.params;
      const { limit = 20, offset = 0, item_type } = req.query;
      
      if (!userId) {
        return ApiResponse.error(res, 'User ID is required', 400);
      }
      
      let query = supabase
        .from('purchases')
        .select('*', { count: 'exact' })
        .eq('user_id', userId)
        .order('purchased_at', { ascending: false })
        .range(offset, offset + limit - 1);
      
      if (item_type) {
        query = query.eq('item_type', item_type);
      }
      
      const { data: purchases, error, count } = await query;
      
      if (error) {
        return ApiResponse.error(res, `Failed to fetch purchase history: ${error.message}`, 400);
      }
      
      return ApiResponse.success(res, {
        purchases: purchases || [],
        pagination: {
          total: count || 0,
          limit: parseInt(limit),
          offset: parseInt(offset),
          has_more: (offset + purchases?.length) < (count || 0)
        }
      }, 'Purchase history retrieved successfully');
      
    } catch (error) {
      console.error('Get purchase history error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }
}

module.exports = new PurchaseController();