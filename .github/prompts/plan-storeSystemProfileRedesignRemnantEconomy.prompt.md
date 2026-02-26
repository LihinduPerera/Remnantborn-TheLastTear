# Remnantborn — Store System, Profile Redesign & Remnant Economy

## Master Implementation Plan (0% → 100%)

**Scope**: Transform the main menu into a tabbed navigation layout (Play / Profile / Store / Remnant Purchase), implement a full character store backed by a catalog database, add a fake-payment Remnant purchase flow, redesign the Profile page with horizontal premium character display, and wire post-match Remnant rewards (15 winner / 5 participant). Both existing characters (Kade, Lira) remain free; the system is fully extensible for future premium characters. Work spans 4 layers: Supabase schema, Node.js backend, C++ engine code, and UMG widgets.

---

## Current State Summary

| Layer | What Exists | What's Missing |
|---|---|---|
| **Database** | `profiles.remnant_count` (default 100), `purchases` table (user_id, item_type, item_id, price, purchased_at) | No `store_items` catalog, no `remnant_packages`, no `remnant_transactions` audit trail |
| **Backend API** | `purchaseController` (create purchase, check ownership, history), `profileController` (CRUD, game stats with increment/decrement/set) | No store browsing endpoint, no catalog, no Remnant purchase endpoint, no match reward endpoint |
| **C++ Data** | `UCharacterDataAsset` (has `bUnlockedByDefault`), `UCharacterSelectionSubsystem` (unlock TMap, `UnlockCharacter()`), `FUserProfile.PurchasedItems`, `FUserProfile.RemnantCount` | No `RemnantPrice` on DataAsset, no `StoreSubsystem`, no HTTP methods for store/Remnants, unlock system not connected to backend purchases |
| **Widgets** | `UMainMenuWidget` (host/find/join/auth panels), `UUserProfileWidget` (basic: username/level/remnants/email/avatar/logout/refresh), `UCharacterSelectionWidget` (vertical list, only unlocked), `UMatchResultsWidget` (winner/results/ranked list) | No tab navigation, no store UI, no Remnant purchase UI, profile is basic, no post-match reward display |
| **Match System** | `AMultiplayerMatchGameState` tracks `FPlayerMatchResult` (alive, winner, survival time, elimination order), last-man-standing win condition | No post-match Remnant rewards, no reward submission to backend |

---

## Phase 1 — Database Schema (Supabase)

### 1.1 Create `store_items` Table

```sql
CREATE TABLE store_items (
  id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
  item_id TEXT UNIQUE NOT NULL,           -- matches CharacterDataAsset.CharacterID (e.g., "Kade", "Lira", "Echo")
  item_type TEXT NOT NULL DEFAULT 'character', -- 'character', 'cosmetic', etc.
  name TEXT NOT NULL,
  description TEXT,
  price INTEGER NOT NULL DEFAULT 0,       -- cost in Remnants (0 = free)
  image_url TEXT,                          -- optional CDN image for store display
  is_active BOOLEAN NOT NULL DEFAULT true, -- whether visible in store
  sort_order INTEGER DEFAULT 0,
  created_at TIMESTAMPTZ DEFAULT now()
);

-- Seed existing free characters (not shown in store since price=0, but registered for completeness)
INSERT INTO store_items (item_id, item_type, name, description, price, is_active, sort_order) VALUES
  ('Kade', 'character', 'Kade', 'A battle-hardened warrior with unmatched sword skills.', 0, false, 1),
  ('Lira', 'character', 'Lira', 'A swift ranger who commands the wind and strikes from afar.', 0, false, 2);

-- Example future premium characters (uncomment when DataAssets are created)
-- INSERT INTO store_items (item_id, item_type, name, description, price, is_active, sort_order) VALUES
--   ('Echo', 'character', 'Echo', 'A mysterious shadow assassin who phases between dimensions.', 500, true, 3),
--   ('Vex', 'character', 'Vex', 'An ancient mage wielding devastating elemental forces.', 800, true, 4),
--   ('Nyx', 'character', 'Nyx', 'A void walker who manipulates gravity and space itself.', 1200, true, 5),
--   ('Thane', 'character', 'Thane', 'An armored titan with earth-shattering defensive abilities.', 1500, true, 6);
```

### 1.2 Create `remnant_packages` Table

```sql
CREATE TABLE remnant_packages (
  id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
  name TEXT NOT NULL,
  remnant_amount INTEGER NOT NULL,
  display_price TEXT NOT NULL,            -- "$4.99" (for UI display)
  price_cents INTEGER NOT NULL,           -- 499 (for fake processing logic)
  is_active BOOLEAN NOT NULL DEFAULT true,
  sort_order INTEGER NOT NULL DEFAULT 0,
  created_at TIMESTAMPTZ DEFAULT now()
);

INSERT INTO remnant_packages (name, remnant_amount, display_price, price_cents, is_active, sort_order) VALUES
  ('Starter',   100,  '$0.99',   99,  true, 1),
  ('Explorer',  500,  '$4.99',  499,  true, 2),
  ('Warrior',  1000,  '$8.99',  899,  true, 3),
  ('Champion', 2500, '$19.99', 1999,  true, 4);
```

### 1.3 Create `remnant_transactions` Table (Audit Trail)

```sql
CREATE TABLE remnant_transactions (
  id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
  user_id UUID NOT NULL REFERENCES profiles(user_id),
  package_id UUID REFERENCES remnant_packages(id),  -- null for match rewards
  amount INTEGER NOT NULL,                            -- positive = credit, negative = debit
  transaction_type TEXT NOT NULL,                      -- 'purchase', 'match_reward', 'character_purchase', 'initial_grant'
  reference_id TEXT,                                   -- fake payment ID, match ID, character item_id, etc.
  description TEXT,                                    -- human-readable note
  balance_after INTEGER,                               -- snapshot of remnant_count after this transaction
  created_at TIMESTAMPTZ DEFAULT now()
);

CREATE INDEX idx_remnant_transactions_user ON remnant_transactions(user_id);
CREATE INDEX idx_remnant_transactions_type ON remnant_transactions(transaction_type);
```

### 1.4 Supabase RLS Policies

```sql
-- store_items: public read, no client write
ALTER TABLE store_items ENABLE ROW LEVEL SECURITY;
CREATE POLICY "Anyone can read active store items" ON store_items FOR SELECT USING (is_active = true);

-- remnant_packages: public read, no client write
ALTER TABLE remnant_packages ENABLE ROW LEVEL SECURITY;
CREATE POLICY "Anyone can read active packages" ON remnant_packages FOR SELECT USING (is_active = true);

-- remnant_transactions: users read own, service role writes
ALTER TABLE remnant_transactions ENABLE ROW LEVEL SECURITY;
CREATE POLICY "Users read own transactions" ON remnant_transactions FOR SELECT USING (auth.uid() = user_id);
```

---

## Phase 2 — Backend API (Node.js + Express)

### 2.1 New File: `backend/src/controllers/storeController.js`

**Endpoints:**

| Method | Path | Auth | Description |
|---|---|---|---|
| GET | `/api/store/characters` | Yes | Returns all active store characters with ownership status for requesting user |
| GET | `/api/store/packages` | No | Returns all active Remnant packages sorted by sort_order |
| POST | `/api/store/buy-character` | Yes | Purchase a premium character with Remnants |
| POST | `/api/store/buy-remnants` | Yes | Purchase Remnants with fake payment |

**`GET /api/store/characters` logic:**
1. Query `store_items` where `item_type = 'character'` AND `is_active = true`, ordered by `sort_order`
2. Query `purchases` for `req.user.id` where `item_type = 'character'` to get owned item_ids
3. Query `profiles` for `req.user.id` to get current `remnant_count`
4. Merge: each character gets `owned: boolean`, `can_afford: boolean`
5. Return `{ characters: [...], remnant_count: number }`

**`POST /api/store/buy-character` logic:**
```
Body: { characterId: string }

1. Validate characterId exists in store_items, is active, item_type = 'character'
2. Get character price from store_items
3. Check user doesn't already own it (query purchases)
4. Get user's remnant_count from profiles
5. Verify remnant_count >= price
6. BEGIN TRANSACTION:
   a. Deduct: UPDATE profiles SET remnant_count = remnant_count - price WHERE user_id = userId
   b. Record purchase: INSERT INTO purchases (user_id, item_type, item_id, price)
   c. Record transaction: INSERT INTO remnant_transactions (user_id, amount: -price, transaction_type: 'character_purchase', reference_id: characterId, balance_after: new_balance)
7. Return { success: true, character: {...}, new_remnant_count: number }
```

**`POST /api/store/buy-remnants` logic:**
```
Body: { packageId: string, cardNumber: string, cardExpiry: string, cardCVV: string }

1. Validate packageId exists in remnant_packages, is active
2. Validate card format (simple regex — 16 digits, MM/YY, 3 digits) — always approve
3. Simulate 1.5s processing delay (setTimeout)
4. Get package remnant_amount
5. UPDATE profiles SET remnant_count = remnant_count + remnant_amount WHERE user_id = userId
6. Generate fake receipt ID: "RMN-" + timestamp + random 4 chars
7. INSERT INTO remnant_transactions (user_id, package_id, amount: +remnant_amount, transaction_type: 'purchase', reference_id: fakeReceiptId, balance_after: new_balance)
8. Return { success: true, remnants_added: number, new_remnant_count: number, receipt_id: string }
```

### 2.2 New File: `backend/src/controllers/matchRewardController.js`

| Method | Path | Auth | Description |
|---|---|---|---|
| POST | `/api/match/reward` | Yes | Submit post-match Remnant reward |

**`POST /api/match/reward` logic:**
```
Body: { userId: string, isWinner: boolean, matchDuration: number, eliminationOrder: number }

1. Validate userId matches req.user.id (server-authoritative: the hosting player submits for themselves)
2. Calculate reward:
   - Winner: 15 Remnants
   - Participant (non-winner): 5 Remnants
3. UPDATE profiles SET remnant_count = remnant_count + reward WHERE user_id = userId
4. Get new balance
5. INSERT INTO remnant_transactions (user_id, amount: +reward, transaction_type: 'match_reward', reference_id: null, description: 'Match reward - Winner/Participant', balance_after: new_balance)
6. Return { success: true, reward_amount: number, new_remnant_count: number, is_winner: boolean }
```

### 2.3 New File: `backend/src/routes/storeRoutes.js`

```javascript
const express = require('express');
const router = express.Router();
const storeController = require('../controllers/storeController');
const { authenticate } = require('../middleware/auth');

router.get('/characters', authenticate, storeController.getCharacters);
router.get('/packages', storeController.getPackages);
router.post('/buy-character', authenticate, storeController.buyCharacter);
router.post('/buy-remnants', authenticate, storeController.buyRemnants);

module.exports = router;
```

### 2.4 New File: `backend/src/routes/matchRewardRoutes.js`

```javascript
const express = require('express');
const router = express.Router();
const matchRewardController = require('../controllers/matchRewardController');
const { authenticate } = require('../middleware/auth');

router.post('/reward', authenticate, matchRewardController.submitReward);

module.exports = router;
```

### 2.5 Modify: `backend/src/index.js`

Add route registrations:
```javascript
const storeRoutes = require('./routes/storeRoutes');
const matchRewardRoutes = require('./routes/matchRewardRoutes');

app.use('/api/store', storeRoutes);
app.use('/api/match', matchRewardRoutes);
```

---

## Phase 3 — C++ Data Layer

### 3.1 Extend `UCharacterDataAsset`

**File**: `Source/Remnantborn/Remnantborn/CharacterSelection/CharacterDataAsset.h`

Add two new properties:

```cpp
// Price in Remnants (0 = free character)
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Store")
int32 RemnantPrice = 0;

// Extended description for store page display
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Store")
FText StoreDescription;
```

**DataAsset updates in Unreal Editor:**
- Kade: `RemnantPrice = 0`, `bUnlockedByDefault = true`
- Lira: `RemnantPrice = 0`, `bUnlockedByDefault = true`
- Future premium characters: set `RemnantPrice` to 300–1500, `bUnlockedByDefault = false`

### 3.2 New Structs in `UEdsHttpService.h`

```cpp
// Store character info from backend
USTRUCT(BlueprintType)
struct FStoreCharacterInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite) FString ItemId;
    UPROPERTY(BlueprintReadWrite) FString Name;
    UPROPERTY(BlueprintReadWrite) FString Description;
    UPROPERTY(BlueprintReadWrite) int32 Price = 0;
    UPROPERTY(BlueprintReadWrite) FString ImageUrl;
    UPROPERTY(BlueprintReadWrite) bool bOwned = false;
    UPROPERTY(BlueprintReadWrite) bool bCanAfford = false;
};

// Remnant package from backend
USTRUCT(BlueprintType)
struct FRemnantPackage
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite) FString PackageId;
    UPROPERTY(BlueprintReadWrite) FString Name;
    UPROPERTY(BlueprintReadWrite) int32 RemnantAmount = 0;
    UPROPERTY(BlueprintReadWrite) FString DisplayPrice;
    UPROPERTY(BlueprintReadWrite) int32 SortOrder = 0;
};

// Character purchase response
USTRUCT(BlueprintType)
struct FCharacterPurchaseResponse
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite) bool bSuccess = false;
    UPROPERTY(BlueprintReadWrite) FString CharacterId;
    UPROPERTY(BlueprintReadWrite) int32 NewRemnantCount = 0;
    UPROPERTY(BlueprintReadWrite) FString ErrorMessage;
};

// Remnant purchase response
USTRUCT(BlueprintType)
struct FRemnantPurchaseResponse
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite) bool bSuccess = false;
    UPROPERTY(BlueprintReadWrite) int32 RemnantsAdded = 0;
    UPROPERTY(BlueprintReadWrite) int32 NewRemnantCount = 0;
    UPROPERTY(BlueprintReadWrite) FString ReceiptId;
    UPROPERTY(BlueprintReadWrite) FString ErrorMessage;
};

// Match reward response
USTRUCT(BlueprintType)
struct FMatchRewardResponse
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite) bool bSuccess = false;
    UPROPERTY(BlueprintReadWrite) int32 RewardAmount = 0;
    UPROPERTY(BlueprintReadWrite) int32 NewRemnantCount = 0;
    UPROPERTY(BlueprintReadWrite) bool bIsWinner = false;
    UPROPERTY(BlueprintReadWrite) FString ErrorMessage;
};
```

### 3.3 New HTTP Methods in `UEdsHttpService`

**File**: `Source/Remnantborn/Remnantborn/OnlineService/UEdsHttpService.h` — add declarations:

```cpp
// === Store Methods ===
DECLARE_DELEGATE_OneParam(FOnStoreCharactersResponse, const TArray<FStoreCharacterInfo>&);
DECLARE_DELEGATE_OneParam(FOnRemnantPackagesResponse, const TArray<FRemnantPackage>&);
DECLARE_DELEGATE_OneParam(FOnCharacterPurchaseResponse, const FCharacterPurchaseResponse&);
DECLARE_DELEGATE_OneParam(FOnRemnantPurchaseResponse, const FRemnantPurchaseResponse&);
DECLARE_DELEGATE_OneParam(FOnMatchRewardResponse, const FMatchRewardResponse&);

void GetStoreCharacters(const FString& AuthToken, FOnStoreCharactersResponse Callback);
void GetRemnantPackages(const FString& AuthToken, FOnRemnantPackagesResponse Callback);
void BuyCharacter(const FString& AuthToken, const FString& CharacterId, FOnCharacterPurchaseResponse Callback);
void BuyRemnants(const FString& AuthToken, const FString& PackageId, const FString& CardNumber, const FString& CardExpiry, const FString& CardCVV, FOnRemnantPurchaseResponse Callback);
void SubmitMatchReward(const FString& AuthToken, const FString& UserId, bool bIsWinner, float MatchDuration, int32 EliminationOrder, FOnMatchRewardResponse Callback);
```

**File**: `Source/Remnantborn/Remnantborn/OnlineService/UEdsHttpService.cpp` — implement each method using `SendRequestWithAuth()`, parsing JSON responses into the corresponding structs.

### 3.4 New `UStoreSubsystem` (GameInstance Subsystem)

**Files**: `Source/Remnantborn/Remnantborn/Store/StoreSubsystem.h` and `.cpp`

```cpp
UCLASS()
class REMNANTBORN_API UStoreSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    // Fetch data from backend
    UFUNCTION(BlueprintCallable, Category = "Store")
    void FetchStoreCatalog();

    UFUNCTION(BlueprintCallable, Category = "Store")
    void FetchRemnantPackages();

    // Purchase actions
    UFUNCTION(BlueprintCallable, Category = "Store")
    void PurchaseCharacter(const FString& CharacterId);

    UFUNCTION(BlueprintCallable, Category = "Store")
    void PurchaseRemnants(const FString& PackageId, const FString& CardNumber, const FString& CardExpiry, const FString& CardCVV);

    // Data accessors
    UFUNCTION(BlueprintPure, Category = "Store")
    TArray<FStoreCharacterInfo> GetCachedCharacters() const { return CachedCharacters; }

    UFUNCTION(BlueprintPure, Category = "Store")
    TArray<FRemnantPackage> GetCachedPackages() const { return CachedPackages; }

    UFUNCTION(BlueprintPure, Category = "Store")
    bool IsCatalogLoaded() const { return bCatalogLoaded; }

    // Events
    UPROPERTY(BlueprintAssignable) FOnStoreCatalogLoaded OnStoreCatalogLoaded;       // fires after FetchStoreCatalog completes
    UPROPERTY(BlueprintAssignable) FOnRemnantPackagesLoaded OnPackagesLoaded;        // fires after FetchRemnantPackages completes
    UPROPERTY(BlueprintAssignable) FOnCharacterPurchaseComplete OnCharacterPurchased; // fires after PurchaseCharacter completes
    UPROPERTY(BlueprintAssignable) FOnRemnantPurchaseComplete OnRemnantsPurchased;    // fires after PurchaseRemnants completes
    UPROPERTY(BlueprintAssignable) FOnStoreError OnStoreError;                        // fires on any error

private:
    TArray<FStoreCharacterInfo> CachedCharacters;
    TArray<FRemnantPackage> CachedPackages;
    bool bCatalogLoaded = false;
    bool bPackagesLoaded = false;

    // Internal callbacks
    void OnCatalogReceived(const TArray<FStoreCharacterInfo>& Characters);
    void OnPackagesReceived(const TArray<FRemnantPackage>& Packages);
    void OnCharacterPurchaseResult(const FCharacterPurchaseResponse& Response);
    void OnRemnantPurchaseResult(const FRemnantPurchaseResponse& Response);

    // Helper to get HttpService and auth token from GameInstance
    UEdsHttpService* GetHttpService() const;
    FString GetAuthToken() const;
};
```

**Implementation flow for `PurchaseCharacter`:**
1. Call `HttpService->BuyCharacter(AuthToken, CharacterId, Callback)`
2. On success callback:
   - Call `CharacterSelectionSubsystem->UnlockCharacter(FName(*CharacterId))`
   - Update `GameInstance->CurrentUserProfile.RemnantCount = Response.NewRemnantCount`
   - Mark the character as owned in `CachedCharacters`
   - Broadcast `OnCharacterPurchased`
   - Broadcast `GameInstance->OnProfileUpdated` (so UI refreshes everywhere)
3. On failure: Broadcast `OnStoreError` with error message

### 3.5 Modify `UCharacterSelectionSubsystem`

**File**: `Source/Remnantborn/Remnantborn/CharacterSelection/CharacterSelectionSubsystem.h` — add:

```cpp
// Sync unlock status from backend purchased items list
UFUNCTION(BlueprintCallable, Category = "Character Selection")
void SyncUnlocksFromBackend(const TArray<FString>& PurchasedItems);

// Get ALL characters including locked ones (for Store display)
UFUNCTION(BlueprintCallable, Category = "Character Selection")
TArray<UCharacterDataAsset*> GetAllCharactersIncludingLocked() const;
```

**File**: `Source/Remnantborn/Remnantborn/CharacterSelection/CharacterSelectionSubsystem.cpp` — implement:

```cpp
void UCharacterSelectionSubsystem::SyncUnlocksFromBackend(const TArray<FString>& PurchasedItems)
{
    // First, reset all non-default unlocks
    for (UCharacterDataAsset* Character : AvailableCharacters)
    {
        if (Character)
        {
            // Keep bUnlockedByDefault characters unlocked, reset others
            CharacterUnlockStatus.Add(Character->CharacterID, Character->bUnlockedByDefault);
        }
    }

    // Then unlock characters that appear in PurchasedItems
    for (const FString& ItemId : PurchasedItems)
    {
        FName CharID(*ItemId);
        if (CharacterUnlockStatus.Contains(CharID))
        {
            CharacterUnlockStatus[CharID] = true;
            OnCharacterUnlocked.Broadcast(CharID, true);
        }
    }
}

TArray<UCharacterDataAsset*> UCharacterSelectionSubsystem::GetAllCharactersIncludingLocked() const
{
    return AvailableCharacters; // Return the full list, not filtered by unlock status
}
```

### 3.6 Post-Match Reward Submission

**File**: `Source/Remnantborn/Remnantborn/GameModes/MultiplayerMatchGameState.cpp` — modify `CheckForMatchEnd()`:

After `SetMatchState(EMatchState::Finished)`, add reward submission logic. Each player's controller submits their own reward:

**File**: `Source/Remnantborn/Remnantborn/OnlineService/MultiplayerPlayerController/MultiplayerPlayerController.h` — add:

```cpp
// Called when match ends — submits reward to backend
UFUNCTION(Client, Reliable)
void ClientReceiveMatchReward(int32 RewardAmount, bool bIsWinner);

UFUNCTION(BlueprintCallable, Category = "Match")
void SubmitMatchReward(bool bIsWinner, float MatchDuration, int32 EliminationOrder);

// Delegate for UI to bind to
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMatchRewardReceived, int32, RewardAmount, int32, NewBalance);

UPROPERTY(BlueprintAssignable)
FOnMatchRewardReceived OnMatchRewardReceived;
```

**Implementation flow:**
1. `AMultiplayerGameMode` detects match end → iterates `PlayerResults`
2. For each player, calls the owning `AMultiplayerPlayerController->SubmitMatchReward(bIsWinner, duration, eliminationOrder)` (server-side call)
3. `SubmitMatchReward` calls `HttpService->SubmitMatchReward(...)` with the authenticated player's token
4. On response, calls `ClientReceiveMatchReward(RewardAmount, bIsWinner)` (client RPC)
5. Client-side RPC broadcasts `OnMatchRewardReceived` → `UMatchResultsWidget` displays "+15 Remnants" or "+5 Remnants"

### 3.7 Login → Unlock Sync Integration

**File**: `Source/Remnantborn/Remnantborn/OnlineService/MyOnlineGameInstance.cpp`

In the auth response handler (after successful login, dev-login, or verify-token), add:

```cpp
// After SetAuthState(...) and profile is populated:
UCharacterSelectionSubsystem* CharSub = GetSubsystem<UCharacterSelectionSubsystem>();
if (CharSub)
{
    CharSub->SyncUnlocksFromBackend(CurrentUserProfile.PurchasedItems);
}
```

Similarly in the `GetUserProfile` / `GetMyProfile` response handler — re-sync unlocks whenever profile is refreshed.

---

## Phase 4 — Widget Architecture (C++ Backing Classes)

### 4.1 Tab Navigation System (Reusable)

#### `UTabButtonWidget`

**Files**: `Source/Remnantborn/Remnantborn/Widgets/Navigation/TabButtonWidget.h/.cpp`

```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTabClicked, int32, TabIndex);

UCLASS()
class REMNANTBORN_API UTabButtonWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    void InitializeTab(int32 InTabIndex, const FText& InLabel, UTexture2D* InIcon);

    UFUNCTION(BlueprintCallable)
    void SetActive(bool bActive);

    UPROPERTY(BlueprintAssignable)
    FOnTabClicked OnTabClicked;

protected:
    UPROPERTY(meta = (BindWidget)) UButton* TabButton;
    UPROPERTY(meta = (BindWidget)) UImage* TabIcon;
    UPROPERTY(meta = (BindWidget)) UTextBlock* TabLabel;
    UPROPERTY(meta = (BindWidget)) UImage* ActiveIndicator;  // accent bar shown when active

private:
    int32 TabIndex = 0;

    UFUNCTION() void OnButtonClicked();
};
```

#### `UTabNavigationWidget`

**Files**: `Source/Remnantborn/Remnantborn/Widgets/Navigation/TabNavigationWidget.h/.cpp`

```cpp
UCLASS()
class REMNANTBORN_API UTabNavigationWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintCallable)
    void AddTab(const FText& Label, UTexture2D* Icon, UUserWidget* ContentWidget);

    UFUNCTION(BlueprintCallable)
    void SetActiveTab(int32 TabIndex);

    UFUNCTION(BlueprintPure)
    int32 GetActiveTabIndex() const { return ActiveTabIndex; }

protected:
    // Left sidebar containing tab buttons
    UPROPERTY(meta = (BindWidget)) UVerticalBox* TabButtonContainer;

    // Right content area
    UPROPERTY(meta = (BindWidget)) UWidgetSwitcher* ContentSwitcher;

    // Optional: auth panel at bottom of sidebar
    UPROPERTY(meta = (BindWidget), meta = (BindWidgetOptional)) UNamedSlot* AuthPanelSlot;

    // Tab button widget class (set in Blueprint)
    UPROPERTY(EditAnywhere, Category = "Navigation")
    TSubclassOf<UTabButtonWidget> TabButtonClass;

private:
    TArray<UTabButtonWidget*> TabButtons;
    int32 ActiveTabIndex = 0;

    UFUNCTION() void OnTabSelected(int32 TabIndex);
};
```

### 4.2 Redesigned Main Menu Widget

**File**: `Source/Remnantborn/Remnantborn/Widgets/MainMenuWidget.h` — restructure:

The existing `UMainMenuWidget` will be refactored. Its root becomes `UTabNavigationWidget`. The existing multiplayer controls (host, find, join, session list, quit, lobby settings) move into a **Play** content panel. The auth panel (login button and logged-in user display) stays persistent in the sidebar.

```
┌─────────────────────────────────────────────────────┐
│ [Avatar] Username                                    │  ← persistent auth panel (top of sidebar)
│ Lvl 5 | ◆ 350 Remnants                              │
├───────────┬─────────────────────────────────────────┤
│  ▶ Play   │                                         │
│  👤 Prof  │    [Active Tab Content Area]             │
│  🏪 Store │                                         │
│  💎 Remn  │    - Play: host/find/join/sessions/quit │
│           │    - Profile: redesigned profile page    │
│           │    - Store: character store              │
│           │    - Remnants: package purchase          │
│           │                                         │
│  [Quit]   │                                         │
└───────────┴─────────────────────────────────────────┘
```

**Key changes to `UMainMenuWidget`**:
- Remove direct references to `HostButton`, `FindButton`, etc. from the top level — they move into a child `UPlayTabWidget`
- Add `UTabNavigationWidget* TabNavigation` as the root bind
- On construct: create 4 tab content widgets, call `TabNavigation->AddTab(...)` for each
- Auth panel binding remains but is placed in `TabNavigation->AuthPanelSlot`

#### New `UPlayTabWidget`

**Files**: `Source/Remnantborn/Remnantborn/Widgets/MainMenu/PlayTabWidget.h/.cpp`

Contains all existing multiplayer controls extracted from `UMainMenuWidget`:
- Host Game, Find Sessions, Join by IP, Direct Join buttons
- Session list (ListView)
- Lobby settings (max players combo, use lobby checkbox)
- Quit button

All existing event handling logic moves here from `UMainMenuWidget`.

### 4.3 Redesigned Profile Widget

**File**: `Source/Remnantborn/Remnantborn/Widgets/Auth/UserProfileWidget.h` — major overhaul:

```cpp
UCLASS()
class REMNANTBORN_API UUserProfileWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintCallable)
    void UpdateProfile(const FString& Username, int32 Level, int32 RemnantCount, const FString& AvatarUrl, const FString& Bio, const FString& CreatedAt);

    UFUNCTION(BlueprintCallable)
    void PopulateOwnedCharacters();

protected:
    // === Header Section ===
    UPROPERTY(meta = (BindWidget)) UImage* AvatarImage;           // large rounded avatar
    UPROPERTY(meta = (BindWidget)) UTextBlock* UsernameText;      // styled heading
    UPROPERTY(meta = (BindWidget)) UTextBlock* LevelBadge;        // "Level 5" badge
    UPROPERTY(meta = (BindWidget)) UImage* RemnantIcon;           // currency icon
    UPROPERTY(meta = (BindWidget)) UTextBlock* RemnantText;       // "350"
    UPROPERTY(meta = (BindWidget)) UTextBlock* BioText;           // user bio
    UPROPERTY(meta = (BindWidget)) UTextBlock* MemberSinceText;   // "Member since Jan 2026"
    UPROPERTY(meta = (BindWidget)) UButton* EditProfileButton;    // opens ProfileEditWidget

    // === Action Buttons ===
    UPROPERTY(meta = (BindWidget)) UButton* LogoutButton;
    UPROPERTY(meta = (BindWidget)) UButton* RefreshButton;

    // === My Characters Section ===
    UPROPERTY(meta = (BindWidget)) UTextBlock* MyCharactersTitle;  // "My Characters"
    UPROPERTY(meta = (BindWidget)) UScrollBox* CharacterScrollBox; // horizontal scroll

    // Character card widget class
    UPROPERTY(EditAnywhere, Category = "Widgets")
    TSubclassOf<UOwnedCharacterCardWidget> OwnedCharacterCardClass;

    // === Hidden (moved to widgets or removed) ===
    // EmailText removed from main profile view (moved to edit profile)

private:
    TArray<UOwnedCharacterCardWidget*> CharacterCards;

    UFUNCTION() void OnLogoutClicked();
    UFUNCTION() void OnRefreshClicked();
    UFUNCTION() void OnEditProfileClicked();
};
```

Layout:
```
┌──────────────────────────────────────────────────┐
│  ┌──────┐                                        │
│  │Avatar│  Username                    [Edit] [↻] │
│  │(100px│  Level 5  |  ◆ 350 Remnants            │
│  └──────┘  "Bio text here..."                    │
│            Member since January 2026             │
├──────────────────────────────────────────────────┤
│  My Characters                                   │
│  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐        │
│  │ Kade │  │ Lira │  │ Echo │  │ More →│ scroll │
│  │[port]│  │[port]│  │[port]│  │      │        │
│  │ Free │  │ Free │  │ Purch│  │      │        │
│  └──────┘  └──────┘  └──────┘  └──────┘        │
│  ← horizontal scroll →                          │
├──────────────────────────────────────────────────┤
│                               [Logout]           │
└──────────────────────────────────────────────────┘
```

#### New `UOwnedCharacterCardWidget`

**Files**: `Source/Remnantborn/Remnantborn/Widgets/Profile/OwnedCharacterCardWidget.h/.cpp`

```cpp
UCLASS()
class REMNANTBORN_API UOwnedCharacterCardWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    void InitializeCard(UCharacterDataAsset* CharacterData, bool bIsPurchased);

protected:
    UPROPERTY(meta = (BindWidget)) UImage* CharacterPortrait;    // 160×220 portrait
    UPROPERTY(meta = (BindWidget)) UTextBlock* CharacterName;     // name below portrait
    UPROPERTY(meta = (BindWidget)) UTextBlock* StatusBadge;       // "Free" or "Purchased"

private:
    UPROPERTY() UCharacterDataAsset* CachedCharacterData;
};
```

### 4.4 Store Widget

**Files**: `Source/Remnantborn/Remnantborn/Widgets/Store/StoreWidget.h/.cpp`

```cpp
UCLASS()
class REMNANTBORN_API UStoreWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UFUNCTION(BlueprintCallable)
    void RefreshStore();

protected:
    // Header
    UPROPERTY(meta = (BindWidget)) UTextBlock* StoreTitleText;        // "Character Store"
    UPROPERTY(meta = (BindWidget)) UImage* RemnantIcon;
    UPROPERTY(meta = (BindWidget)) UTextBlock* BalanceText;           // "◆ 350"

    // Character grid
    UPROPERTY(meta = (BindWidget)) UWrapBox* CharacterGrid;           // wrapping grid of cards
    UPROPERTY(meta = (BindWidget)) UTextBlock* ComingSoonText;        // shown when no premium characters exist
    UPROPERTY(meta = (BindWidget)) UCircularThrobber* LoadingSpinner; // shown while fetching

    // Character card class
    UPROPERTY(EditAnywhere, Category = "Widgets")
    TSubclassOf<UStoreCharacterCardWidget> StoreCharacterCardClass;

    // Notification
    UPROPERTY(meta = (BindWidget)) UTextBlock* NotificationText;      // success/error toast

private:
    TArray<UStoreCharacterCardWidget*> CharacterCards;

    void OnCatalogLoaded();
    void OnCharacterPurchased(const FString& CharacterId, int32 NewBalance);
    void OnStoreError(const FString& ErrorMessage);
    void ShowNotification(const FText& Message, bool bIsError = false);
};
```

#### `UStoreCharacterCardWidget`

**Files**: `Source/Remnantborn/Remnantborn/Widgets/Store/StoreCharacterCardWidget.h/.cpp`

```cpp
UCLASS()
class REMNANTBORN_API UStoreCharacterCardWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    void InitializeCard(const FStoreCharacterInfo& CharacterInfo, UCharacterDataAsset* LocalData);

    UFUNCTION(BlueprintCallable)
    void SetOwned(bool bOwned);

protected:
    UPROPERTY(meta = (BindWidget)) UImage* CharacterPortrait;     // large portrait
    UPROPERTY(meta = (BindWidget)) UTextBlock* CharacterName;
    UPROPERTY(meta = (BindWidget)) UTextBlock* CharacterDescription; // truncated
    UPROPERTY(meta = (BindWidget)) UImage* RemnantIcon;
    UPROPERTY(meta = (BindWidget)) UTextBlock* PriceText;         // "500"
    UPROPERTY(meta = (BindWidget)) UButton* BuyButton;
    UPROPERTY(meta = (BindWidget)) UTextBlock* BuyButtonText;     // "Purchase" or "Owned"
    UPROPERTY(meta = (BindWidget)) UImage* LockOverlay;           // semi-transparent lock icon
    UPROPERTY(meta = (BindWidget)) UImage* OwnedOverlay;          // checkmark overlay

private:
    FStoreCharacterInfo CachedInfo;

    UFUNCTION() void OnBuyClicked();
};
```

Card layout:
```
┌──────────────────┐
│    [Portrait]    │
│   (200×280 px)   │
│                  │
│  ┌─ Lock Icon ─┐ │  ← shown if not owned
│  └─────────────┘ │
├──────────────────┤
│  Character Name  │
│  "Short desc..." │
├──────────────────┤
│  ◆ 500 Remnants  │
│  [  Purchase  ]  │  ← disabled if owned or can't afford
└──────────────────┘

--- After purchase: ---
┌──────────────────┐
│    [Portrait]    │
│   ✓ OWNED        │  ← green checkmark overlay
├──────────────────┤
│  Character Name  │
│  ◆ 500           │
│  [   Owned   ]   │  ← greyed out
└──────────────────┘
```

### 4.5 Remnant Purchase Widget

**Files**: `Source/Remnantborn/Remnantborn/Widgets/Store/RemnantPurchaseWidget.h/.cpp`

```cpp
UCLASS()
class REMNANTBORN_API URemnantPurchaseWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintCallable)
    void RefreshPackages();

protected:
    // Header
    UPROPERTY(meta = (BindWidget)) UTextBlock* PageTitleText;      // "Purchase Remnants"
    UPROPERTY(meta = (BindWidget)) UImage* RemnantIcon;
    UPROPERTY(meta = (BindWidget)) UTextBlock* BalanceText;        // current balance

    // Package grid
    UPROPERTY(meta = (BindWidget)) UHorizontalBox* PackageContainer; // 4 packages in a row
    UPROPERTY(meta = (BindWidget)) UCircularThrobber* LoadingSpinner;

    // Payment form (hidden until package selected)
    UPROPERTY(meta = (BindWidget)) UCanvasPanel* PaymentFormPanel;
    UPROPERTY(meta = (BindWidget)) UTextBlock* SelectedPackageText; // "Explorer - 500 Remnants - $4.99"
    UPROPERTY(meta = (BindWidget)) UEditableTextBox* CardNumberInput;
    UPROPERTY(meta = (BindWidget)) UEditableTextBox* CardExpiryInput;
    UPROPERTY(meta = (BindWidget)) UEditableTextBox* CardCVVInput;
    UPROPERTY(meta = (BindWidget)) UButton* PayNowButton;
    UPROPERTY(meta = (BindWidget)) UButton* CancelPaymentButton;

    // Processing overlay
    UPROPERTY(meta = (BindWidget)) UCanvasPanel* ProcessingOverlay;
    UPROPERTY(meta = (BindWidget)) UCircularThrobber* ProcessingSpinner;
    UPROPERTY(meta = (BindWidget)) UTextBlock* ProcessingText;     // "Processing payment..."

    // Success confirmation
    UPROPERTY(meta = (BindWidget)) UCanvasPanel* SuccessPanel;
    UPROPERTY(meta = (BindWidget)) UTextBlock* SuccessText;        // "500 Remnants added!"
    UPROPERTY(meta = (BindWidget)) UTextBlock* ReceiptText;        // "Receipt: RMN-..."
    UPROPERTY(meta = (BindWidget)) UTextBlock* NewBalanceText;     // "New Balance: ◆ 850"
    UPROPERTY(meta = (BindWidget)) UButton* DismissSuccessButton;

    // Package card class
    UPROPERTY(EditAnywhere, Category = "Widgets")
    TSubclassOf<URemnantPackageCardWidget> PackageCardClass;

private:
    FString SelectedPackageId;
    TArray<URemnantPackageCardWidget*> PackageCards;

    UFUNCTION() void OnPayNowClicked();
    UFUNCTION() void OnCancelPaymentClicked();
    UFUNCTION() void OnDismissSuccessClicked();
    void OnPackageSelected(const FString& PackageId);
    void OnPurchaseComplete(const FRemnantPurchaseResponse& Response);
    void ShowPaymentForm(const FRemnantPackage& Package);
    void ShowProcessing();
    void ShowSuccess(int32 RemnantsAdded, int32 NewBalance, const FString& ReceiptId);
    void ResetToPackageSelection();
    bool ValidateCardInput() const;
};
```

UI flow:
```
Step 1: Package Selection
┌────────────────────────────────────────────────────┐
│  Purchase Remnants                    ◆ 350        │
├────────────────────────────────────────────────────┤
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ │
│  │  ◆ 100   │ │  ◆ 500   │ │ ◆ 1000   │ │ ◆ 2500   │ │
│  │  $0.99   │ │  $4.99   │ │  $8.99   │ │ $19.99   │ │
│  │ Starter  │ │ Explorer │ │ Warrior  │ │ Champion │ │
│  │ [Select] │ │ [Select] │ │ [Select] │ │ [Select] │ │
│  └──────────┘ └──────────┘ └──────────┘ │BEST VALUE│ │
│                                          └──────────┘ │
└────────────────────────────────────────────────────┘

Step 2: Payment Form (after selecting "Explorer")
┌────────────────────────────────────────────────────┐
│  Explorer - 500 Remnants - $4.99                   │
├────────────────────────────────────────────────────┤
│  Card Number:  [4242 4242 4242 4242]               │
│  Expiry:       [12/27]     CVV: [123]              │
│                                                    │
│        [ Pay $4.99 ]     [ Cancel ]                │
└────────────────────────────────────────────────────┘

Step 3: Processing
┌────────────────────────────────────────────────────┐
│              ⟳ Processing payment...               │
│              Please wait...                        │
└────────────────────────────────────────────────────┘

Step 4: Success
┌────────────────────────────────────────────────────┐
│              ✓ Payment Successful!                 │
│              500 Remnants added to your account    │
│              Receipt: RMN-20260226-A3F2            │
│              New Balance: ◆ 850                    │
│                                                    │
│                    [ Done ]                        │
└────────────────────────────────────────────────────┘
```

#### `URemnantPackageCardWidget`

**Files**: `Source/Remnantborn/Remnantborn/Widgets/Store/RemnantPackageCardWidget.h/.cpp`

```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPackageSelected, const FString&, PackageId);

UCLASS()
class REMNANTBORN_API URemnantPackageCardWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    void InitializeCard(const FRemnantPackage& Package, bool bIsBestValue = false);

    UPROPERTY(BlueprintAssignable)
    FOnPackageSelected OnPackageSelected;

protected:
    UPROPERTY(meta = (BindWidget)) UImage* RemnantIcon;
    UPROPERTY(meta = (BindWidget)) UTextBlock* AmountText;        // "500"
    UPROPERTY(meta = (BindWidget)) UTextBlock* PriceText;         // "$4.99"
    UPROPERTY(meta = (BindWidget)) UTextBlock* PackageName;       // "Explorer"
    UPROPERTY(meta = (BindWidget)) UButton* SelectButton;
    UPROPERTY(meta = (BindWidget, BindWidgetOptional)) UTextBlock* BestValueBadge;  // "BEST VALUE" (hidden unless flagged)

private:
    FString CachedPackageId;
    UFUNCTION() void OnSelectClicked();
};
```

### 4.6 Match Results Widget Update

**File**: `Source/Remnantborn/Remnantborn/Widgets/MatchResults/MatchResultsWidget.h` — add:

```cpp
// Reward display
UPROPERTY(meta = (BindWidget)) UTextBlock* RewardAmountText;   // "+15 Remnants" or "+5 Remnants"
UPROPERTY(meta = (BindWidget)) UImage* RewardRemnantIcon;
UPROPERTY(meta = (BindWidget)) UTextBlock* NewBalanceText;     // "Balance: ◆ 365"

UFUNCTION(BlueprintCallable)
void DisplayMatchReward(int32 RewardAmount, int32 NewBalance, bool bIsWinner);
```

**Implementation**: Fade-in animation, count-up effect from 0 to RewardAmount over 1 second. Winner gets gold text, participant gets silver text.

---

## Phase 5 — UMG Blueprint Creation

### 5.1 Widget Blueprints to Create

All in `/Content/Remnantborn/Widgets/`:

| Blueprint | Parent C++ Class | Location |
|---|---|---|
| `WBP_TabNavigation` | `UTabNavigationWidget` | `Navigation/` |
| `WBP_TabButton` | `UTabButtonWidget` | `Navigation/` |
| `WBP_PlayTab` | `UPlayTabWidget` | `MainMenu/` |
| `WBP_Store` | `UStoreWidget` | `Store/` |
| `WBP_StoreCharacterCard` | `UStoreCharacterCardWidget` | `Store/` |
| `WBP_RemnantPurchase` | `URemnantPurchaseWidget` | `Store/` |
| `WBP_RemnantPackageCard` | `URemnantPackageCardWidget` | `Store/` |
| `WBP_OwnedCharacterCard` | `UOwnedCharacterCardWidget` | `Profile/` |

### 5.2 Existing Blueprints to Redesign

| Blueprint | Changes |
|---|---|
| `WBP_MainMenu` | Root layout becomes tabbed navigation; existing controls re-parented into PlayTab content |
| `WBP_UserProfile` | Complete visual redesign — header with large avatar, horizontal character scroll, modern card-based layout |
| `BP_MatchResultWidget` | Add reward display section at bottom (amount, icon, new balance) |

### 5.3 New Art Assets Needed

| Asset | Description | Location |
|---|---|---|
| `T_RemnantCurrency` | Remnant currency icon (crystal/gem, ~64×64) | `Content/Remnantborn/Widgets/Images/Icons/` |
| `T_LockIcon` | Lock overlay for locked characters (~48×48) | `Content/Remnantborn/Widgets/Images/Icons/` |
| `T_CheckmarkIcon` | Owned checkmark overlay (~48×48) | `Content/Remnantborn/Widgets/Images/Icons/` |
| `T_TabPlay` | Play tab icon (~32×32) | `Content/Remnantborn/Widgets/Images/Icons/` |
| `T_TabProfile` | Profile tab icon (~32×32) | `Content/Remnantborn/Widgets/Images/Icons/` |
| `T_TabStore` | Store tab icon (~32×32) | `Content/Remnantborn/Widgets/Images/Icons/` |
| `T_TabRemnants` | Remnants tab icon (~32×32) | `Content/Remnantborn/Widgets/Images/Icons/` |
| `T_BestValueBadge` | "Best Value" ribbon/badge for Champion package | `Content/Remnantborn/Widgets/Images/Icons/` |

### 5.4 Visual Style Guide

- **Color Palette**: Dark backgrounds (#1A1A2E, #16213E), accent gold (#E8B04B), accent blue (#0F3460), success green (#2ECC71), error red (#E74C3C), text white (#FFFFFF), text muted (#B0B0B0)
- **Font Sizes**: Page titles: 28pt bold, Section headers: 20pt semi-bold, Body: 14pt regular, Badges: 10pt bold uppercase
- **Cards**: Rounded corners (8px), subtle border (#333), dark fill (#222), 2px accent border on hover
- **Buttons**: Rounded (6px), primary: gold fill with dark text, secondary: dark fill with white border, disabled: 40% opacity
- **Transitions**: 0.2s ease for tab switches, 0.3s fade for notifications, 1s count-up for reward display

---

## Phase 6 — Integration & Data Flow

### 6.1 Complete Data Flow Diagrams

#### Login → Profile → Unlocks
```
User opens game
  → MainMenuGameMode loads WBP_MainMenu
  → GameInstance.LoadSavedAuth()
    → HttpService.VerifyToken()
      ✓ → SetAuthState(true, token, userId, profile)
        → CharacterSelectionSubsystem.SyncUnlocksFromBackend(profile.PurchasedItems)
        → OnAuthStateChanged.Broadcast(true)
        → OnProfileUpdated.Broadcast(profile)
          → UI updates: sidebar shows username/avatar/remnants
      ✗ → ClearAuth(), show login button
```

#### Store Purchase Flow
```
User clicks Store tab
  → StoreWidget.NativeConstruct()
  → StoreSubsystem.FetchStoreCatalog()
    → HttpService.GetStoreCharacters(token)
      → Backend: SELECT store_items + JOIN purchases → returns [{itemId, name, price, owned}]
    → OnCatalogLoaded: populate CharacterGrid with StoreCharacterCardWidgets
  → User clicks "Purchase" on a character card
    → StoreSubsystem.PurchaseCharacter(characterId)
      → HttpService.BuyCharacter(token, characterId)
        → Backend: validate → deduct remnants → insert purchase → insert transaction → return
      ✓ → CharacterSelectionSubsystem.UnlockCharacter(characterId)
        → Update CachedCharacters (mark owned)
        → Update GameInstance.CurrentUserProfile.RemnantCount
        → Broadcast OnCharacterPurchased
          → StoreWidget: update card to "Owned", update balance display
          → ProfileWidget: refresh character list (if visible)
      ✗ → Broadcast OnStoreError → show error notification
```

#### Remnant Purchase Flow
```
User clicks Remnant Purchase tab
  → RemnantPurchaseWidget.NativeConstruct()
  → StoreSubsystem.FetchRemnantPackages()
    → HttpService.GetRemnantPackages(token)
    → OnPackagesLoaded: display 4 package cards
  → User selects package → payment form appears
  → User enters fake card details → clicks "Pay Now"
    → ValidateCardInput() — basic format check
    → ShowProcessing()
    → StoreSubsystem.PurchaseRemnants(packageId, card, expiry, cvv)
      → HttpService.BuyRemnants(...)
        → Backend: validate → 1.5s delay → credit remnants → insert transaction → return receipt
      ✓ → Update GameInstance.CurrentUserProfile.RemnantCount
        → ShowSuccess(remnantsAdded, newBalance, receiptId)
        → Broadcast OnRemnantsPurchased → all balance displays update
      ✗ → ShowError()
```

#### Post-Match Reward Flow
```
Match ends (last man standing)
  → AMultiplayerMatchGameState.CheckForMatchEnd()
    → SetMatchState(Finished), SetWinner(name)
  → AMultiplayerGameMode detects Finished state
    → For each PlayerController:
      → Determine bIsWinner from PlayerResults
      → Call PC->SubmitMatchReward(bIsWinner, duration, eliminationOrder)
      → HttpService.SubmitMatchReward(token, userId, bIsWinner, ...)
        → Backend: calculate reward (15 or 5) → increment remnant_count → insert transaction
      ✓ → PC->ClientReceiveMatchReward(rewardAmount, newBalance) [Client RPC]
        → OnMatchRewardReceived.Broadcast(amount, balance)
          → MatchResultsWidget.DisplayMatchReward(amount, balance, bIsWinner)
            → Animated "+15 Remnants" display with count-up effect
```

### 6.2 Module Dependencies Update

**File**: `Source/Remnantborn/Remnantborn.Build.cs`

No new module dependencies needed — HTTP, Json, JsonUtilities, UMG, Slate, SlateCore are already included.

---

## Phase 7 — Testing & Verification

### 7.1 Backend API Tests

| Test | Endpoint | Expected Result |
|---|---|---|
| Fetch empty store | `GET /api/store/characters` | `{ characters: [], remnant_count: 100 }` (no active premium chars) |
| Fetch packages | `GET /api/store/packages` | 4 packages sorted by sort_order |
| Buy character (success) | `POST /api/store/buy-character` | Purchase recorded, remnants deducted, new balance returned |
| Buy character (duplicate) | `POST /api/store/buy-character` | 409 Conflict — already owned |
| Buy character (insufficient) | `POST /api/store/buy-character` | 400 — insufficient remnants |
| Buy remnants | `POST /api/store/buy-remnants` | Remnants credited, receipt generated, ~1.5s delay |
| Buy remnants (invalid card) | `POST /api/store/buy-remnants` | 400 — invalid card format |
| Match reward (winner) | `POST /api/match/reward` | +15 Remnants, new balance |
| Match reward (participant) | `POST /api/match/reward` | +5 Remnants, new balance |

### 7.2 Economy Balance Test

```
1. Create new account → verify 100 Remnants
2. Win 10 matches → balance should be 100 + (10 × 15) = 250
3. Lose 10 matches → balance should be 250 + (10 × 5) = 300
4. Try to buy 500-Remnant character → FAIL (insufficient)
5. Buy "Explorer" Remnant package (500) → balance = 800
6. Buy 500-Remnant character → SUCCESS, balance = 300
7. Verify character appears as unlocked in lobby selection
8. Verify character appears in Profile's "My Characters" list
9. Try to buy same character again → FAIL (already owned)
```

### 7.3 UI/UX Verification

| Check | Detail |
|---|---|
| Tab navigation | All 4 tabs switch correctly, active indicator moves, content swaps smoothly |
| Auth state | Sidebar shows login button when logged out, user info when logged in |
| Profile layout | Avatar, username, level, remnants, bio, member-since all display correctly |
| Character cards | Horizontal scroll works, "Free"/"Purchased" badges show correctly |
| Store loading | Spinner shows while fetching, "Coming Soon" shows when no premium chars exist |
| Store cards | Lock overlay on locked, checkmark on owned, price displayed, buy button states |
| Remnant purchase | Package selection → payment form → processing → success → balance update |
| Match rewards | Reward text appears post-match, animated, correct amount |
| Cross-widget sync | Buying in Store updates Profile character list AND balance across all visible widgets |
| Widget memory | No leaks — all widgets clean up properly on NativeDestruct |

### 7.4 Multiplayer Test

```
1. Player A creates lobby (host)
2. Player B joins lobby
3. Both select characters (only unlocked ones visible)
4. Match starts → Player A wins
5. Both see match results with reward amounts (+15 / +5)
6. Both return to lobby → balances updated in sidebar
7. Player B navigates to Store, buys a premium character with accumulated Remnants
8. Player B returns to lobby, newly unlocked character appears in selection
```

---

## Implementation Order (Recommended Sequence)

| Step | Task | Dependencies | Est. Effort |
|---|---|---|---|
| 1 | Create Supabase tables (store_items, remnant_packages, remnant_transactions) | None | 30 min |
| 2 | Implement storeController.js + storeRoutes.js | Tables exist | 2 hours |
| 3 | Implement matchRewardController.js + matchRewardRoutes.js | Tables exist | 1 hour |
| 4 | Register new routes in index.js | Controllers exist | 10 min |
| 5 | Test all new endpoints with Postman/curl | Backend complete | 30 min |
| 6 | Extend UCharacterDataAsset (RemnantPrice, StoreDescription) | None | 15 min |
| 7 | Add new structs to UEdsHttpService.h | None | 30 min |
| 8 | Add new HTTP methods to UEdsHttpService.cpp | Structs defined, backend running | 2 hours |
| 9 | Create UStoreSubsystem (h + cpp) | HttpService methods exist | 2 hours |
| 10 | Modify UCharacterSelectionSubsystem (SyncUnlocksFromBackend, GetAllCharactersIncludingLocked) | None | 30 min |
| 11 | Wire login → unlock sync in MyOnlineGameInstance | CharacterSelectionSubsystem updated | 30 min |
| 12 | Create UTabButtonWidget (h + cpp) | None | 45 min |
| 13 | Create UTabNavigationWidget (h + cpp) | TabButtonWidget exists | 1 hour |
| 14 | Create UPlayTabWidget — extract from MainMenuWidget | TabNavigation exists | 1.5 hours |
| 15 | Refactor UMainMenuWidget to use TabNavigation | PlayTabWidget + TabNavigation exist | 2 hours |
| 16 | Create UOwnedCharacterCardWidget (h + cpp) | None | 30 min |
| 17 | Redesign UUserProfileWidget (h + cpp) | OwnedCharacterCardWidget exists | 1.5 hours |
| 18 | Create UStoreCharacterCardWidget (h + cpp) | StoreSubsystem exists | 1 hour |
| 19 | Create UStoreWidget (h + cpp) | StoreCharacterCardWidget + StoreSubsystem | 1.5 hours |
| 20 | Create URemnantPackageCardWidget (h + cpp) | None | 30 min |
| 21 | Create URemnantPurchaseWidget (h + cpp) | PackageCardWidget + StoreSubsystem | 1.5 hours |
| 22 | Update UMatchResultsWidget for rewards | None | 45 min |
| 23 | Add post-match reward logic (GameMode + PlayerController) | MatchReward HTTP + MatchResultsWidget | 2 hours |
| 24 | Compile C++ — resolve all build errors | All C++ complete | 1 hour |
| 25 | Create art assets (icons, badges) | None (parallel) | 1 hour |
| 26 | Create WBP_TabButton (UMG Designer) | C++ TabButtonWidget | 30 min |
| 27 | Create WBP_TabNavigation (UMG Designer) | C++ TabNavigationWidget + WBP_TabButton | 45 min |
| 28 | Create WBP_PlayTab (UMG Designer) | C++ PlayTabWidget | 1 hour |
| 29 | Redesign WBP_MainMenu (UMG Designer) | WBP_TabNavigation + WBP_PlayTab | 1.5 hours |
| 30 | Create WBP_OwnedCharacterCard (UMG Designer) | C++ OwnedCharacterCardWidget | 20 min |
| 31 | Redesign WBP_UserProfile (UMG Designer) | WBP_OwnedCharacterCard | 1 hour |
| 32 | Create WBP_StoreCharacterCard (UMG Designer) | C++ StoreCharacterCardWidget | 30 min |
| 33 | Create WBP_Store (UMG Designer) | WBP_StoreCharacterCard | 45 min |
| 34 | Create WBP_RemnantPackageCard (UMG Designer) | C++ RemnantPackageCardWidget | 20 min |
| 35 | Create WBP_RemnantPurchase (UMG Designer) | WBP_RemnantPackageCard | 45 min |
| 36 | Update BP_MatchResultWidget (UMG Designer) | C++ MatchResultsWidget updated | 20 min |
| 37 | Update Kade + Lira DataAssets (set RemnantPrice, StoreDescription) | CharacterDataAsset extended | 10 min |
| 38 | Full integration test (login → store → buy → lobby → match → reward) | Everything | 2 hours |
| 39 | Polish — animations, hover effects, transitions, error handling edge cases | Everything | 3 hours |
| 40 | Final multiplayer playtest | Everything | 1 hour |

**Total estimated effort: ~35 hours**

---

## File Manifest (All Files to Create or Modify)

### New Files

| File | Type |
|---|---|
| `backend/src/controllers/storeController.js` | Backend |
| `backend/src/controllers/matchRewardController.js` | Backend |
| `backend/src/routes/storeRoutes.js` | Backend |
| `backend/src/routes/matchRewardRoutes.js` | Backend |
| `Source/Remnantborn/Remnantborn/Store/StoreSubsystem.h` | C++ |
| `Source/Remnantborn/Remnantborn/Store/StoreSubsystem.cpp` | C++ |
| `Source/Remnantborn/Remnantborn/Widgets/Navigation/TabButtonWidget.h` | C++ |
| `Source/Remnantborn/Remnantborn/Widgets/Navigation/TabButtonWidget.cpp` | C++ |
| `Source/Remnantborn/Remnantborn/Widgets/Navigation/TabNavigationWidget.h` | C++ |
| `Source/Remnantborn/Remnantborn/Widgets/Navigation/TabNavigationWidget.cpp` | C++ |
| `Source/Remnantborn/Remnantborn/Widgets/MainMenu/PlayTabWidget.h` | C++ |
| `Source/Remnantborn/Remnantborn/Widgets/MainMenu/PlayTabWidget.cpp` | C++ |
| `Source/Remnantborn/Remnantborn/Widgets/Profile/OwnedCharacterCardWidget.h` | C++ |
| `Source/Remnantborn/Remnantborn/Widgets/Profile/OwnedCharacterCardWidget.cpp` | C++ |
| `Source/Remnantborn/Remnantborn/Widgets/Store/StoreWidget.h` | C++ |
| `Source/Remnantborn/Remnantborn/Widgets/Store/StoreWidget.cpp` | C++ |
| `Source/Remnantborn/Remnantborn/Widgets/Store/StoreCharacterCardWidget.h` | C++ |
| `Source/Remnantborn/Remnantborn/Widgets/Store/StoreCharacterCardWidget.cpp` | C++ |
| `Source/Remnantborn/Remnantborn/Widgets/Store/RemnantPurchaseWidget.h` | C++ |
| `Source/Remnantborn/Remnantborn/Widgets/Store/RemnantPurchaseWidget.cpp` | C++ |
| `Source/Remnantborn/Remnantborn/Widgets/Store/RemnantPackageCardWidget.h` | C++ |
| `Source/Remnantborn/Remnantborn/Widgets/Store/RemnantPackageCardWidget.cpp` | C++ |

### Modified Files

| File | Changes |
|---|---|
| `backend/src/index.js` | Register `/api/store` and `/api/match` routes |
| `Source/Remnantborn/Remnantborn/CharacterSelection/CharacterDataAsset.h` | Add `RemnantPrice`, `StoreDescription` |
| `Source/Remnantborn/Remnantborn/CharacterSelection/CharacterSelectionSubsystem.h` | Add `SyncUnlocksFromBackend()`, `GetAllCharactersIncludingLocked()` |
| `Source/Remnantborn/Remnantborn/CharacterSelection/CharacterSelectionSubsystem.cpp` | Implement new methods |
| `Source/Remnantborn/Remnantborn/OnlineService/UEdsHttpService.h` | Add store/Remnant/reward structs + method declarations |
| `Source/Remnantborn/Remnantborn/OnlineService/UEdsHttpService.cpp` | Implement new HTTP methods |
| `Source/Remnantborn/Remnantborn/OnlineService/MyOnlineGameInstance.h` | Add store-related methods/delegates |
| `Source/Remnantborn/Remnantborn/OnlineService/MyOnlineGameInstance.cpp` | Wire login→unlock sync, store convenience methods |
| `Source/Remnantborn/Remnantborn/OnlineService/MultiplayerPlayerController/MultiplayerPlayerController.h` | Add reward RPC + delegate |
| `Source/Remnantborn/Remnantborn/OnlineService/MultiplayerPlayerController/MultiplayerPlayerController.cpp` | Implement reward submission |
| `Source/Remnantborn/Remnantborn/GameModes/MultiplayerGameMode.cpp` | Trigger reward submission on match end |
| `Source/Remnantborn/Remnantborn/Widgets/MainMenuWidget.h` | Refactor to tabbed layout |
| `Source/Remnantborn/Remnantborn/Widgets/MainMenuWidget.cpp` | Move controls to PlayTab, add tab setup |
| `Source/Remnantborn/Remnantborn/Widgets/Auth/UserProfileWidget.h` | Complete redesign |
| `Source/Remnantborn/Remnantborn/Widgets/Auth/UserProfileWidget.cpp` | Implement new profile layout + character cards |
| `Source/Remnantborn/Remnantborn/Widgets/MatchResults/MatchResultsWidget.h` | Add reward display fields |
| `Source/Remnantborn/Remnantborn/Widgets/MatchResults/MatchResultsWidget.cpp` | Implement reward display + animation |

### UMG Blueprints to Create (in Unreal Editor)

| Blueprint | Content Path |
|---|---|
| `WBP_TabButton` | `/Content/Remnantborn/Widgets/Navigation/` |
| `WBP_TabNavigation` | `/Content/Remnantborn/Widgets/Navigation/` |
| `WBP_PlayTab` | `/Content/Remnantborn/Widgets/MainMenu/` |
| `WBP_OwnedCharacterCard` | `/Content/Remnantborn/Widgets/Profile/` |
| `WBP_Store` | `/Content/Remnantborn/Widgets/Store/` |
| `WBP_StoreCharacterCard` | `/Content/Remnantborn/Widgets/Store/` |
| `WBP_RemnantPurchase` | `/Content/Remnantborn/Widgets/Store/` |
| `WBP_RemnantPackageCard` | `/Content/Remnantborn/Widgets/Store/` |

### UMG Blueprints to Redesign (in Unreal Editor)

| Blueprint | Content Path |
|---|---|
| `WBP_MainMenu` | `/Content/Remnantborn/Widgets/OnlineService/` |
| `WBP_UserProfile` | `/Content/Remnantborn/Widgets/OnlineService/` |
| `BP_MatchResultWidget` | `/Content/Remnantborn/Widgets/MatchResult/` |

### DataAssets to Update (in Unreal Editor)

| Asset | Changes |
|---|---|
| `Kade.uasset` | Set `RemnantPrice = 0`, `StoreDescription = "..."` |
| `Lira.uasset` | Set `RemnantPrice = 0`, `StoreDescription = "..."` |

---

## Key Design Decisions

| Decision | Rationale |
|---|---|
| Both Kade and Lira are free (`bUnlockedByDefault = true`, `RemnantPrice = 0`) | User's choice — Store displays "Coming Soon" until premium characters are added |
| Main menu replaced entirely with tabbed layout | User's choice — Play tab houses existing multiplayer controls so nothing is lost |
| Match rewards are server-authoritative only | The hosting player's server submits rewards. Prevents client-side cheating. Each player's auth token is used for their own reward submission |
| Tiered packages (4 fixed prices) with fake payment | Simulates realistic payment flow without real gateway. Card validation is cosmetic only — always approves |
| `remnant_transactions` audit trail | Full history of all Remnant flows enables debugging, analytics, and potential refund logic |
| Store catalog lives in database, not DataAssets | Backend is the source of truth for prices and availability. DataAssets hold `RemnantPrice` as a local fallback/display hint, but the backend price is authoritative for purchases |
| `UStoreSubsystem` as GameInstance subsystem | Persists across level travel, accessible from any widget, single source of truth for cached store data |
| Character unlock sync on login | `SyncUnlocksFromBackend()` ensures C++ unlock TMap always matches backend state, even if purchases were made from another device |

---

## Economy Balance Summary

| Parameter | Value |
|---|---|
| Starting Remnants | 100 |
| Match Win Reward | 15 Remnants |
| Match Participation Reward | 5 Remnants |
| Cheapest Premium Character (planned) | 300 Remnants |
| Most Expensive Character (planned) | 1500 Remnants |
| Matches to afford 300-Remnant character (wins only, from 0) | 20 matches |
| Matches to afford 300-Remnant character (from 100 starting, all wins) | 14 matches |
| Matches to afford 1500-Remnant character (from 100, all wins) | 94 matches |
| Remnant Package — Starter | 100 for $0.99 |
| Remnant Package — Explorer | 500 for $4.99 |
| Remnant Package — Warrior | 1000 for $8.99 |
| Remnant Package — Champion | 2500 for $19.99 |

This economy intentionally makes pure-grind progression slow, incentivizing Remnant purchases for faster access to premium content.
