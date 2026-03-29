CREATE TABLE IF NOT EXISTS match_results (
  id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
  match_id TEXT UNIQUE NOT NULL,
  map_name TEXT NOT NULL,
  game_mode TEXT NOT NULL DEFAULT 'unknown',
  host_user_id UUID REFERENCES profiles(user_id),
  winner_user_id UUID REFERENCES profiles(user_id),
  is_draw BOOLEAN NOT NULL DEFAULT false,
  started_at TIMESTAMPTZ,
  ended_at TIMESTAMPTZ NOT NULL DEFAULT now(),
  duration_seconds INTEGER NOT NULL DEFAULT 0,
  expected_player_count INTEGER,
  metadata JSONB NOT NULL DEFAULT '{}'::jsonb,
  rewards_granted BOOLEAN NOT NULL DEFAULT false,
  rewards_granted_at TIMESTAMPTZ,
  created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
  updated_at TIMESTAMPTZ NOT NULL DEFAULT now(),
  CONSTRAINT match_results_duration_non_negative CHECK (duration_seconds >= 0),
  CONSTRAINT match_results_expected_players_non_negative CHECK (expected_player_count IS NULL OR expected_player_count >= 0)
);

CREATE TABLE IF NOT EXISTS match_participants (
  id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
  match_result_id UUID NOT NULL REFERENCES match_results(id) ON DELETE CASCADE,
  participant_key TEXT NOT NULL,
  user_id UUID REFERENCES profiles(user_id),
  player_name TEXT NOT NULL,
  player_id INTEGER,
  character_id TEXT,
  placement INTEGER NOT NULL DEFAULT 0,
  elimination_order INTEGER NOT NULL DEFAULT 0,
  survival_time_seconds DOUBLE PRECISION NOT NULL DEFAULT 0,
  is_winner BOOLEAN NOT NULL DEFAULT false,
  is_alive_at_end BOOLEAN NOT NULL DEFAULT false,
  disconnected BOOLEAN NOT NULL DEFAULT false,
  disconnect_reason TEXT,
  kill_count INTEGER NOT NULL DEFAULT 0,
  death_count INTEGER NOT NULL DEFAULT 0,
  damage_dealt DOUBLE PRECISION NOT NULL DEFAULT 0,
  damage_taken DOUBLE PRECISION NOT NULL DEFAULT 0,
  created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
  CONSTRAINT match_participants_unique_participant UNIQUE (match_result_id, participant_key),
  CONSTRAINT match_participants_placement_non_negative CHECK (placement >= 0),
  CONSTRAINT match_participants_elim_non_negative CHECK (elimination_order >= 0),
  CONSTRAINT match_participants_survival_non_negative CHECK (survival_time_seconds >= 0),
  CONSTRAINT match_participants_kills_non_negative CHECK (kill_count >= 0),
  CONSTRAINT match_participants_deaths_non_negative CHECK (death_count >= 0),
  CONSTRAINT match_participants_damage_dealt_non_negative CHECK (damage_dealt >= 0),
  CONSTRAINT match_participants_damage_taken_non_negative CHECK (damage_taken >= 0)
);

CREATE INDEX IF NOT EXISTS idx_match_results_host ON match_results(host_user_id);
CREATE INDEX IF NOT EXISTS idx_match_results_winner ON match_results(winner_user_id);
CREATE INDEX IF NOT EXISTS idx_match_results_ended_at ON match_results(ended_at DESC);
CREATE INDEX IF NOT EXISTS idx_match_participants_match_result_id ON match_participants(match_result_id);
CREATE INDEX IF NOT EXISTS idx_match_participants_user_created ON match_participants(user_id, created_at DESC);

CREATE OR REPLACE FUNCTION cleanup_old_match_results(retention_days INTEGER DEFAULT 90)
RETURNS INTEGER
LANGUAGE plpgsql
AS $$
DECLARE
  deleted_count INTEGER;
BEGIN
  DELETE FROM match_results
  WHERE ended_at < (now() - make_interval(days => retention_days));

  GET DIAGNOSTICS deleted_count = ROW_COUNT;
  RETURN deleted_count;
END;
$$;