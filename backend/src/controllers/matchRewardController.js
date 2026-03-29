const { supabase } = require('../config/database');
const ApiResponse = require('../utils/response');

const WINNER_REWARD = 15;
const PARTICIPANT_REWARD = 5;

const NOT_FOUND_CODE = 'PGRST116';
const UUID_REGEX = /^[0-9a-f]{8}-[0-9a-f]{4}-[1-5][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/i;

function toFiniteNumber(value, fallback = 0) {
  const parsed = Number(value);
  return Number.isFinite(parsed) ? parsed : fallback;
}

function toNonNegativeInt(value, fallback = 0) {
  const parsed = Math.floor(toFiniteNumber(value, fallback));
  return parsed >= 0 ? parsed : fallback;
}

function cleanString(value, maxLength = 128) {
  if (typeof value !== 'string') {
    return '';
  }

  return value.trim().slice(0, maxLength);
}

function isUuid(value) {
  return UUID_REGEX.test(String(value || '').trim());
}

function sanitizeMetadata(metadata) {
  if (!metadata || typeof metadata !== 'object' || Array.isArray(metadata)) {
    return {};
  }

  return metadata;
}

function isDuplicateKeyError(error) {
  return error && (error.code === '23505' || String(error.message || '').toLowerCase().includes('duplicate key'));
}

function buildParticipantKey(participant, index) {
  const userId = cleanString(participant.userId || participant.user_id, 64).toLowerCase();
  if (userId) {
    return `user:${userId}`;
  }

  const playerId = participant.playerId ?? participant.player_id;
  if (Number.isFinite(Number(playerId))) {
    return `pid:${Math.floor(Number(playerId))}`;
  }

  const playerName = cleanString(participant.playerName || participant.player_name, 96).toLowerCase();
  if (playerName) {
    return `name:${playerName}`;
  }

  return `slot:${index}`;
}

function normalizeParticipant(rawParticipant, index) {
  const participant = rawParticipant || {};
  const isWinner = Boolean(participant.isWinner ?? participant.is_winner);
  const eliminationOrder = toNonNegativeInt(participant.eliminationOrder ?? participant.elimination_order, isWinner ? 0 : 1);
  let placement = toNonNegativeInt(participant.placement, 0);

  if (placement <= 0) {
    if (isWinner) {
      placement = 1;
    } else if (eliminationOrder > 0) {
      placement = eliminationOrder + 1;
    }
  }

  return {
    participantKey: buildParticipantKey(participant, index),
    userId: isUuid(participant.userId || participant.user_id) ? cleanString(participant.userId || participant.user_id, 64) : null,
    playerName: cleanString(participant.playerName || participant.player_name, 96) || `Player-${index + 1}`,
    playerId: Number.isFinite(Number(participant.playerId ?? participant.player_id)) ? Math.floor(Number(participant.playerId ?? participant.player_id)) : null,
    characterId: cleanString(participant.characterId || participant.character_id, 96) || null,
    placement,
    eliminationOrder,
    survivalTimeSeconds: Math.max(0, toFiniteNumber(participant.survivalTimeSeconds ?? participant.survival_time_seconds, 0)),
    isWinner,
    isAliveAtEnd: Boolean(participant.isAliveAtEnd ?? participant.is_alive_at_end),
    disconnected: Boolean(participant.disconnected),
    disconnectReason: cleanString(participant.disconnectReason || participant.disconnect_reason, 160) || null,
    killCount: toNonNegativeInt(participant.killCount ?? participant.kill_count, 0),
    deathCount: toNonNegativeInt(participant.deathCount ?? participant.death_count, 0),
    damageDealt: Math.max(0, toFiniteNumber(participant.damageDealt ?? participant.damage_dealt, 0)),
    damageTaken: Math.max(0, toFiniteNumber(participant.damageTaken ?? participant.damage_taken, 0)),
  };
}

async function finalizeRewardTransaction(transactionId, userId, rewardAmount) {
  const { data: profile, error: profileError } = await supabase
    .from('profiles')
    .select('remnant_count')
    .eq('user_id', userId)
    .single();

  if (profileError || !profile) {
    throw new Error('User profile not found');
  }

  const currentRemnants = Number(profile.remnant_count || 0);
  const newBalance = currentRemnants + rewardAmount;

  const { error: updateError } = await supabase
    .from('profiles')
    .update({ remnant_count: newBalance })
    .eq('user_id', userId);

  if (updateError) {
    throw new Error(`Failed to update balance: ${updateError.message}`);
  }

  const { error: txUpdateError } = await supabase
    .from('remnant_transactions')
    .update({ balance_after: newBalance })
    .eq('id', transactionId);

  if (txUpdateError) {
    throw new Error(`Failed to finalize transaction balance: ${txUpdateError.message}`);
  }

  return newBalance;
}

async function grantMatchRewardForUser({ userId, isWinner, matchDuration, placement, matchId }) {
  const rewardAmount = isWinner ? WINNER_REWARD : PARTICIPANT_REWARD;
  const referenceId = matchId ? String(matchId) : null;

  let existingTx = null;
  if (referenceId) {
    const { data, error } = await supabase
      .from('remnant_transactions')
      .select('id, amount, balance_after')
      .eq('user_id', userId)
      .eq('transaction_type', 'match_reward')
      .eq('reference_id', referenceId)
      .single();

    if (error && error.code !== NOT_FOUND_CODE) {
      throw new Error(`Failed to lookup existing reward: ${error.message}`);
    }

    existingTx = data || null;
  }

  if (existingTx) {
    if (existingTx.balance_after == null) {
      const newBalance = await finalizeRewardTransaction(existingTx.id, userId, rewardAmount);
      return {
        rewardAmount,
        newRemnantCount: newBalance,
        alreadyGranted: false,
      };
    }

    return {
      rewardAmount: Number(existingTx.amount || rewardAmount),
      newRemnantCount: Number(existingTx.balance_after || 0),
      alreadyGranted: true,
    };
  }

  const description = `Match reward - ${isWinner ? 'Winner' : 'Participant'} (duration: ${Math.floor(matchDuration)}s, placement: ${placement})`;

  const { data: insertedTx, error: insertError } = await supabase
    .from('remnant_transactions')
    .insert([{
      user_id: userId,
      amount: rewardAmount,
      transaction_type: 'match_reward',
      reference_id: referenceId,
      description,
      balance_after: null,
    }])
    .select('id')
    .single();

  if (insertError) {
    if (isDuplicateKeyError(insertError)) {
      return grantMatchRewardForUser({ userId, isWinner, matchDuration, placement, matchId });
    }

    throw new Error(`Failed to create reward transaction: ${insertError.message}`);
  }

  const newBalance = await finalizeRewardTransaction(insertedTx.id, userId, rewardAmount);

  return {
    rewardAmount,
    newRemnantCount: newBalance,
    alreadyGranted: false,
  };
}

class MatchRewardController {
  async submitReward(req, res) {
    try {
      const userId = req.user?.id;
      const { isWinner, matchDuration = 0, eliminationOrder = 0, matchId = null } = req.body;

      if (!userId) {
        return ApiResponse.unauthorized(res, 'Unauthorized');
      }

      const rewardResult = await grantMatchRewardForUser({
        userId,
        isWinner: Boolean(isWinner),
        matchDuration: Math.max(0, toFiniteNumber(matchDuration, 0)),
        placement: toNonNegativeInt(eliminationOrder, 0),
        matchId,
      });

      if (matchId && rewardResult.alreadyGranted) {
        return ApiResponse.conflict(res, 'Reward already submitted for this match');
      }

      return ApiResponse.success(res, {
        reward_amount: rewardResult.rewardAmount,
        new_remnant_count: rewardResult.newRemnantCount,
        is_winner: !!isWinner,
      }, 'Match reward granted successfully');
    } catch (error) {
      console.error('MatchReward submitReward error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }

  async submitMatchComplete(req, res) {
    try {
      const hostUserId = req.user?.id;
      if (!hostUserId) {
        return ApiResponse.unauthorized(res, 'Unauthorized');
      }

      const matchId = cleanString(req.body?.matchId, 128);
      const participants = Array.isArray(req.body?.participants) ? req.body.participants : [];

      if (!matchId) {
        return ApiResponse.error(res, 'matchId is required', 400);
      }

      if (participants.length === 0) {
        return ApiResponse.error(res, 'participants array is required and cannot be empty', 400);
      }

      const normalizedByKey = new Map();
      participants.forEach((participant, index) => {
        const normalized = normalizeParticipant(participant, index);
        normalizedByKey.set(normalized.participantKey, normalized);
      });

      const normalizedParticipants = Array.from(normalizedByKey.values());
      const winner = normalizedParticipants.find((participant) => participant.isWinner);
      const isDraw = !winner;
      const endedAt = cleanString(req.body?.endedAt, 40) || new Date().toISOString();
      const durationSeconds = toNonNegativeInt(req.body?.durationSeconds, 0);
      const startedAt = cleanString(req.body?.startedAt, 40)
        || (durationSeconds > 0
          ? new Date(new Date(endedAt).getTime() - (durationSeconds * 1000)).toISOString()
          : null);

      const expectedPlayerCount = toNonNegativeInt(req.body?.expectedPlayerCount, normalizedParticipants.length);

      const matchPayload = {
        match_id: matchId,
        map_name: cleanString(req.body?.mapName, 128) || 'UnknownMap',
        game_mode: cleanString(req.body?.gameMode, 64) || 'unknown',
        host_user_id: hostUserId,
        winner_user_id: winner?.userId || null,
        is_draw: isDraw,
        started_at: startedAt,
        ended_at: endedAt,
        duration_seconds: durationSeconds,
        expected_player_count: expectedPlayerCount,
        metadata: sanitizeMetadata(req.body?.metadata),
        updated_at: new Date().toISOString(),
      };

      const { data: matchRow, error: matchError } = await supabase
        .from('match_results')
        .upsert([matchPayload], { onConflict: 'match_id' })
        .select('id, match_id')
        .single();

      if (matchError || !matchRow) {
        return ApiResponse.error(res, `Failed to store match result: ${matchError?.message || 'Unknown error'}`, 400);
      }

      const participantRows = normalizedParticipants.map((participant) => ({
        match_result_id: matchRow.id,
        participant_key: participant.participantKey,
        user_id: participant.userId,
        player_name: participant.playerName,
        player_id: participant.playerId,
        character_id: participant.characterId,
        placement: participant.placement,
        elimination_order: participant.eliminationOrder,
        survival_time_seconds: participant.survivalTimeSeconds,
        is_winner: participant.isWinner,
        is_alive_at_end: participant.isAliveAtEnd,
        disconnected: participant.disconnected,
        disconnect_reason: participant.disconnectReason,
        kill_count: participant.killCount,
        death_count: participant.deathCount,
        damage_dealt: participant.damageDealt,
        damage_taken: participant.damageTaken,
      }));

      const { error: participantError } = await supabase
        .from('match_participants')
        .upsert(participantRows, { onConflict: 'match_result_id,participant_key' });

      if (participantError) {
        return ApiResponse.error(res, `Failed to store participants: ${participantError.message}`, 400);
      }

      const rewardResults = [];
      for (const participant of normalizedParticipants) {
        if (!participant.userId) {
          continue;
        }

        const rewardResult = await grantMatchRewardForUser({
          userId: participant.userId,
          isWinner: participant.isWinner,
          matchDuration: durationSeconds,
          placement: participant.placement,
          matchId,
        });

        rewardResults.push({
          userId: participant.userId,
          isWinner: participant.isWinner,
          placement: participant.placement,
          rewardAmount: rewardResult.rewardAmount,
          newRemnantCount: rewardResult.newRemnantCount,
          alreadyGranted: rewardResult.alreadyGranted,
        });
      }

      const myReward = rewardResults.find((reward) => reward.userId === hostUserId) || null;
      const idempotentReplay = rewardResults.length > 0 && rewardResults.every((reward) => reward.alreadyGranted);

      return ApiResponse.success(res, {
        match_id: matchId,
        participants_saved: participantRows.length,
        rewards_processed: rewardResults.length,
        idempotent_replay: idempotentReplay,
        my_result: myReward ? {
          reward_amount: myReward.rewardAmount,
          new_remnant_count: myReward.newRemnantCount,
          is_winner: myReward.isWinner,
          placement: myReward.placement,
        } : null,
      }, 'Match results saved successfully');
    } catch (error) {
      console.error('MatchReward submitMatchComplete error:', error);
      return ApiResponse.serverError(res, 'Internal server error');
    }
  }
}

module.exports = new MatchRewardController();
