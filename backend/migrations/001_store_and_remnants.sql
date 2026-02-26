CREATE TABLE IF NOT EXISTS store_items (
  id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
  item_id TEXT UNIQUE NOT NULL,
  item_type TEXT NOT NULL DEFAULT 'character',
  name TEXT NOT NULL,
  description TEXT,
  price INTEGER NOT NULL DEFAULT 0,
  image_url TEXT,
  is_active BOOLEAN NOT NULL DEFAULT true,
  sort_order INTEGER DEFAULT 0,
  created_at TIMESTAMPTZ DEFAULT now()
);

CREATE TABLE IF NOT EXISTS remnant_packages (
  id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
  name TEXT NOT NULL,
  remnant_amount INTEGER NOT NULL,
  display_price TEXT NOT NULL,
  price_cents INTEGER NOT NULL,
  is_active BOOLEAN NOT NULL DEFAULT true,
  sort_order INTEGER NOT NULL DEFAULT 0,
  created_at TIMESTAMPTZ DEFAULT now()
);

CREATE TABLE IF NOT EXISTS remnant_transactions (
  id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
  user_id UUID NOT NULL REFERENCES profiles(user_id),
  package_id UUID REFERENCES remnant_packages(id),
  amount INTEGER NOT NULL,
  transaction_type TEXT NOT NULL,
  reference_id TEXT,
  description TEXT,
  balance_after INTEGER,
  created_at TIMESTAMPTZ DEFAULT now()
);

CREATE INDEX IF NOT EXISTS idx_remnant_transactions_user ON remnant_transactions(user_id);
CREATE INDEX IF NOT EXISTS idx_remnant_transactions_type ON remnant_transactions(transaction_type);

INSERT INTO remnant_packages (name, remnant_amount, display_price, price_cents, is_active, sort_order)
SELECT 'Starter', 100, '$0.99', 99, true, 1
WHERE NOT EXISTS (SELECT 1 FROM remnant_packages WHERE name = 'Starter');

INSERT INTO remnant_packages (name, remnant_amount, display_price, price_cents, is_active, sort_order)
SELECT 'Explorer', 500, '$4.99', 499, true, 2
WHERE NOT EXISTS (SELECT 1 FROM remnant_packages WHERE name = 'Explorer');

INSERT INTO remnant_packages (name, remnant_amount, display_price, price_cents, is_active, sort_order)
SELECT 'Warrior', 1000, '$8.99', 899, true, 3
WHERE NOT EXISTS (SELECT 1 FROM remnant_packages WHERE name = 'Warrior');

INSERT INTO remnant_packages (name, remnant_amount, display_price, price_cents, is_active, sort_order)
SELECT 'Champion', 2500, '$19.99', 1999, true, 4
WHERE NOT EXISTS (SELECT 1 FROM remnant_packages WHERE name = 'Champion');
