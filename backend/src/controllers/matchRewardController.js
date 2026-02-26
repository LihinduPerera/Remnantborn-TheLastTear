const { supabase } = require('../config/database');
const ApiResponse = require('../utils/response');

class MatchRewardController {
  async submitReward(req, res) {
    try {
      const userId = req.user?.id;
      const { isWinner, matchDuration = 0, eliminationOrder = 0, matchId = null } = req.body;

      if (!userId) {
        return ApiResponse.unauthorized(res, 'Unauthorized');
      }

      const winnerReward = 15;
      const participantReward = 5;
      const rewardAmount = isWinner ? winnerReward : participantReward;

      if (matchId) {
        const { data: existing } = await supabase
          .from('remnant_transactions')
          .select('id')
          .eq('user_id', userId)
          .eq('transaction_type', 'match_reward')
          .eq('reference_id', String(matchId))
          .single();

        if (existing) {
          return ApiResponse.conflict(res, 'Reward already submitted for this match');
        }
      }

      const { data: profile, error: profileError } = await supabase
        .from('profiles')
        .select('remnant_count')
        .eq('user_id', userId)
        .single();

      if (profileError || !profile) {
        return ApiResponse.notFound(res, 'User profile not found');
      }

      const currentRemnants = Number(profile.remnant_count || 0);
      const newBalance = currentRemnants + rewardAmount;

      const { error: updateError } = await supabase
        .from('profiles')
        .update({ remnant_count: newBalance })
        .eq('user_id', userId);

      if (updateError) {
        return ApiResponse.error(res, `Failed to update balance: ${updateError.message}`, 400);
      }

      await supabase.from('remnant_transactions').insert([{
        user_id: userId,
        amount: rewardAmount,
        transaction_type: 'match_reward',
        reference_id: matchId ? String(matchId) : null,
        description: `Match reward - ${isWinner ? 'Winner' : 'Participant'} (duration: ${matchDuration}s, placement: ${eliminationOrder})`,
        balance_after: newBalance,
      }]);

      return ApiResponse.success(res, {
        reward_amount: rewardAmount,
        new_remnant_count: newBalance,
        is_winner: !!isWinner,
      }, 'Match reward granted successfully');
    } catch (error) {
      console.error('MatchReward submitReward error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }
}

module.exports = new MatchRewardController();
