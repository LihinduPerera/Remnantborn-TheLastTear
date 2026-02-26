const { supabase } = require('../config/database');
const ApiResponse = require('../utils/response');

const delay = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

class StoreController {
  async getCharacters(req, res) {
    try {
      const userId = req.user?.id;
      if (!userId) {
        return ApiResponse.unauthorized(res, 'Unauthorized');
      }

      const { data: items, error: itemsError } = await supabase
        .from('store_items')
        .select('item_id, item_type, name, description, price, image_url, is_active, sort_order')
        .eq('item_type', 'character')
        .eq('is_active', true)
        .order('sort_order', { ascending: true });

      if (itemsError) {
        return ApiResponse.error(res, `Failed to fetch store items: ${itemsError.message}`, 400);
      }

      const { data: purchases, error: purchaseError } = await supabase
        .from('purchases')
        .select('item_id')
        .eq('user_id', userId)
        .eq('item_type', 'character');

      if (purchaseError) {
        return ApiResponse.error(res, `Failed to fetch ownership: ${purchaseError.message}`, 400);
      }

      const { data: profile, error: profileError } = await supabase
        .from('profiles')
        .select('remnant_count')
        .eq('user_id', userId)
        .single();

      if (profileError || !profile) {
        return ApiResponse.notFound(res, 'User profile not found');
      }

      const ownedSet = new Set((purchases || []).map((p) => p.item_id));

      const characters = (items || []).map((item) => {
        const owned = ownedSet.has(item.item_id);
        return {
          item_id: item.item_id,
          item_type: item.item_type,
          name: item.name,
          description: item.description,
          price: item.price,
          image_url: item.image_url,
          owned,
          can_afford: profile.remnant_count >= (item.price || 0),
        };
      });

      return ApiResponse.success(res, {
        remnant_count: profile.remnant_count,
        characters,
      }, 'Store characters retrieved successfully');
    } catch (error) {
      console.error('Store getCharacters error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }

  async getPackages(req, res) {
    try {
      const { data: packages, error } = await supabase
        .from('remnant_packages')
        .select('id, name, remnant_amount, display_price, price_cents, sort_order')
        .eq('is_active', true)
        .order('sort_order', { ascending: true });

      if (error) {
        return ApiResponse.error(res, `Failed to fetch remnant packages: ${error.message}`, 400);
      }

      return ApiResponse.success(res, packages || [], 'Remnant packages retrieved successfully');
    } catch (error) {
      console.error('Store getPackages error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }

  async buyCharacter(req, res) {
    try {
      const userId = req.user?.id;
      const { characterId } = req.body;

      if (!userId) {
        return ApiResponse.unauthorized(res, 'Unauthorized');
      }

      if (!characterId) {
        return ApiResponse.error(res, 'characterId is required', 400);
      }

      const { data: item, error: itemError } = await supabase
        .from('store_items')
        .select('item_id, item_type, name, price, is_active')
        .eq('item_id', characterId)
        .eq('item_type', 'character')
        .single();

      if (itemError || !item || !item.is_active) {
        return ApiResponse.notFound(res, 'Character not found in store');
      }

      const { data: existingPurchase } = await supabase
        .from('purchases')
        .select('id')
        .eq('user_id', userId)
        .eq('item_type', 'character')
        .eq('item_id', characterId)
        .single();

      if (existingPurchase) {
        return ApiResponse.conflict(res, 'Character already owned');
      }

      const { data: profile, error: profileError } = await supabase
        .from('profiles')
        .select('remnant_count')
        .eq('user_id', userId)
        .single();

      if (profileError || !profile) {
        return ApiResponse.notFound(res, 'User profile not found');
      }

      const price = Number(item.price || 0);
      const currentRemnants = Number(profile.remnant_count || 0);

      if (currentRemnants < price) {
        return ApiResponse.error(res, 'Insufficient remnant count', 400);
      }

      const newBalance = currentRemnants - price;

      const { error: balanceError } = await supabase
        .from('profiles')
        .update({ remnant_count: newBalance })
        .eq('user_id', userId);

      if (balanceError) {
        return ApiResponse.error(res, `Failed to update balance: ${balanceError.message}`, 400);
      }

      const { data: purchase, error: purchaseError } = await supabase
        .from('purchases')
        .insert([{
          user_id: userId,
          item_type: 'character',
          item_id: characterId,
          price,
          purchased_at: new Date().toISOString(),
        }])
        .select()
        .single();

      if (purchaseError) {
        await supabase
          .from('profiles')
          .update({ remnant_count: currentRemnants })
          .eq('user_id', userId);

        if (purchaseError.code === '23505') {
          return ApiResponse.conflict(res, 'Character already owned');
        }
        return ApiResponse.error(res, `Failed to create purchase: ${purchaseError.message}`, 400);
      }

      const transactionPayload = {
        user_id: userId,
        amount: -price,
        transaction_type: 'character_purchase',
        reference_id: characterId,
        description: `Character purchase: ${item.name || characterId}`,
        balance_after: newBalance,
      };

      const { data: pkgRows } = await supabase
        .from('remnant_packages')
        .select('id')
        .limit(1);

      if (pkgRows && pkgRows.length > 0) {
        transactionPayload.package_id = null;
      }

      await supabase.from('remnant_transactions').insert([transactionPayload]);

      return ApiResponse.created(res, {
        purchase,
        character_id: characterId,
        new_remnant_count: newBalance,
      }, 'Character purchased successfully');
    } catch (error) {
      console.error('Store buyCharacter error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }

  async buyRemnants(req, res) {
    try {
      const userId = req.user?.id;
      const { packageId, cardNumber, cardExpiry, cardCVV } = req.body;

      if (!userId) {
        return ApiResponse.unauthorized(res, 'Unauthorized');
      }

      if (!packageId || !cardNumber || !cardExpiry || !cardCVV) {
        return ApiResponse.error(res, 'packageId, cardNumber, cardExpiry and cardCVV are required', 400);
      }

      const sanitizedCard = String(cardNumber).replace(/\s|-/g, '');
      if (!/^\d{16}$/.test(sanitizedCard)) {
        return ApiResponse.error(res, 'Invalid card number format', 400);
      }

      if (!/^(0[1-9]|1[0-2])\/[0-9]{2}$/.test(String(cardExpiry))) {
        return ApiResponse.error(res, 'Invalid expiry format. Use MM/YY', 400);
      }

      if (!/^\d{3,4}$/.test(String(cardCVV))) {
        return ApiResponse.error(res, 'Invalid CVV format', 400);
      }

      const { data: packageData, error: packageError } = await supabase
        .from('remnant_packages')
        .select('id, name, remnant_amount, display_price, is_active')
        .eq('id', packageId)
        .single();

      if (packageError || !packageData || !packageData.is_active) {
        return ApiResponse.notFound(res, 'Remnant package not found');
      }

      await delay(1500);

      const { data: profile, error: profileError } = await supabase
        .from('profiles')
        .select('remnant_count')
        .eq('user_id', userId)
        .single();

      if (profileError || !profile) {
        return ApiResponse.notFound(res, 'User profile not found');
      }

      const currentRemnants = Number(profile.remnant_count || 0);
      const remnantAmount = Number(packageData.remnant_amount || 0);
      const newBalance = currentRemnants + remnantAmount;

      const { error: updateError } = await supabase
        .from('profiles')
        .update({ remnant_count: newBalance })
        .eq('user_id', userId);

      if (updateError) {
        return ApiResponse.error(res, `Failed to update balance: ${updateError.message}`, 400);
      }

      const receiptId = `RMN-${Date.now()}-${Math.random().toString(36).substring(2, 6).toUpperCase()}`;

      await supabase.from('remnant_transactions').insert([{
        user_id: userId,
        package_id: packageData.id,
        amount: remnantAmount,
        transaction_type: 'purchase',
        reference_id: receiptId,
        description: `Remnant package purchase: ${packageData.name}`,
        balance_after: newBalance,
      }]);

      return ApiResponse.success(res, {
        package_id: packageData.id,
        package_name: packageData.name,
        remnants_added: remnantAmount,
        new_remnant_count: newBalance,
        receipt_id: receiptId,
      }, 'Remnant purchase successful');
    } catch (error) {
      console.error('Store buyRemnants error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }
}

module.exports = new StoreController();
