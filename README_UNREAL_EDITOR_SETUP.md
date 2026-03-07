# Remnantborn — Unreal Editor Integration Guide

This guide teaches exactly what to do **inside Unreal Editor** to connect the new Store, Remnant Purchase, Profile-owned characters, and Match Reward UI systems.

> Scope: editor wiring for the C++ systems already implemented in this repo.

---

## 1) Prerequisites before opening the editor

1. Build the project from Visual Studio (Development Editor / Win64).
2. Start backend API (`backend`) and ensure it is reachable at:
   - `http://localhost:3000/api`
3. Apply SQL migration:
   - `backend/migrations/001_store_and_remnants.sql`
4. Launch Unreal Editor and load the project.

---

## 2) Create/verify required Widget Blueprints

## 2.1 Store Character Card

Create Widget Blueprint:
- Name: `WBP_StoreCharacterCard`
- Parent Class: `StoreCharacterCardWidget`
- Suggested path: `/Game/Remnantborn/Widgets/Store/`

In Designer, add these named widgets (must match exactly):
- `CharacterPortrait` (Image, optional but recommended)
- `CharacterName` (TextBlock)
- `CharacterDescription` (TextBlock)
- `PriceText` (TextBlock)
- `BuyButton` (Button)
- `BuyButtonText` (TextBlock)

## 2.2 Remnant Package Card

Create Widget Blueprint:
- Name: `WBP_RemnantPackageCard`
- Parent Class: `RemnantPackageCardWidget`
- Path: `/Game/Remnantborn/Widgets/Store/`

Add named widgets:
- `AmountText` (TextBlock)
- `PriceText` (TextBlock)
- `PackageNameText` (TextBlock)
- `BestValueText` (TextBlock, optional)
- `SelectButton` (Button)

## 2.3 Store Page

Create Widget Blueprint:
- Name: `WBP_Store`
- Parent Class: `StoreWidget`
- Path: `/Game/Remnantborn/Widgets/Store/`

Add named widgets:
- `BalanceText` (TextBlock)
- `CharacterGrid` (WrapBox)
- `ComingSoonText` (TextBlock)
- `LoadingSpinner` (CircularThrobber)
- `NotificationText` (TextBlock)

In Class Defaults:
- Set `StoreCharacterCardClass` = `WBP_StoreCharacterCard`

## 2.4 Remnant Purchase Page

Create Widget Blueprint:
- Name: `WBP_RemnantPurchase`
- Parent Class: `RemnantPurchaseWidget`
- Path: `/Game/Remnantborn/Widgets/Store/`

Add named widgets:
- `BalanceText` (TextBlock)
- `PackageContainer` (HorizontalBox)
- `LoadingSpinner` (CircularThrobber)
- `SelectedPackageText` (TextBlock)
- `CardNumberInput` (EditableTextBox)
- `CardExpiryInput` (EditableTextBox)
- `CardCVVInput` (EditableTextBox)
- `PayNowButton` (Button)
- `NotificationText` (TextBlock)

In Class Defaults:
- Set `RemnantPackageCardClass` = `WBP_RemnantPackageCard`

## 2.5 Owned Character Card (Profile)

Create Widget Blueprint:
- Name: `WBP_OwnedCharacterCard`
- Parent Class: `OwnedCharacterCardWidget`
- Path: `/Game/Remnantborn/Widgets/Auth/`

Add named widgets:
- `CharacterPortrait` (Image, optional)
- `CharacterName` (TextBlock, optional)
- `StatusBadge` (TextBlock, optional)

## 2.6 Profile Widget updates

Open existing `WBP_UserProfile` (parent should be `UserProfileWidget`).

Required bound widgets:
- `UsernameText` (TextBlock)
- `LevelText` (TextBlock)
- `RemnantText` (TextBlock)
- `EmailText` (TextBlock)
- `AvatarImage` (Image)
- `ChangeAvatarButton` (Button) **(opens OS file dialog and uploads to Cloudinary)**
- `LogoutButton` (Button)
- `RefreshButton` (Button)

Optional but now supported (recommended):
- `BioText` (TextBlock)
- `MemberSinceText` (TextBlock)
- `CharacterScrollBox` (ScrollBox)

In Class Defaults:
- Set `OwnedCharacterCardClass` = `WBP_OwnedCharacterCard`

Additional notes:
- The profile edit screen now allows users to pick an image from disk; the blueprint function `PickImageFile` on the game instance invokes the native file picker. After choosing an image the widget automatically uploads the file via `UploadAvatar`.
- Make sure Cloudinary environment variables (`CLOUDINARY_CLOUD_NAME`, `CLOUDINARY_API_KEY`, `CLOUDINARY_API_SECRET`) are configured on the backend server.

## 2.7 Match Results Widget updates

Open existing `BP_MatchResultWidget` (parent should be `MatchResultsWidget`).

Required existing bound widgets:
- `TitleText`, `WinnerText`, `PersonalResultText`
- `VictoryOverlay`, `DefeatOverlay`
- `ResultsScrollBox`, `PlayerResultsBox`
- `ReturnToLobbyButton`, `PlayAgainButton`

Add these optional reward widgets:
- `RewardAmountText` (TextBlock)
- `NewBalanceText` (TextBlock)
- `RewardRemnantIcon` (Image)

Place them in your results layout where reward should appear.

---

## 3) Main Menu wiring (open Store + Remnant Purchase)

`UMainMenuWidget` already supports Login/Profile popups. Store and Remnant pages can be opened directly in Blueprint.

Open `WBP_MainMenu` (parent `MainMenuWidget`):

1. Add two Buttons in Designer:
   - `OpenStoreButton`
   - `OpenRemnantPurchaseButton`

2. In Graph:
   - On `OpenStoreButton.OnClicked`:
     - `Create Widget` (`WBP_Store`) → `Add to Viewport` (Z=100)
   - On `OpenRemnantPurchaseButton.OnClicked`:
     - `Create Widget` (`WBP_RemnantPurchase`) → `Add to Viewport` (Z=100)

3. Also ensure existing class defaults are set on `WBP_MainMenu`:
   - `LoginWidgetClass` = your login widget BP
   - `ProfileWidgetClass` = `WBP_UserProfile`

### Optional: Tabbed layout in Main Menu

If you want one-screen tabs (Play/Profile/Store/Remnants):
- Use a `WidgetSwitcher` in `WBP_MainMenu`
- Create 4 tab buttons
- On each button click, call `SetActiveWidgetIndex` (0..3)
- Put existing multiplayer controls in Play tab
- Embed `WBP_UserProfile`, `WBP_Store`, `WBP_RemnantPurchase` in other tabs

(Your C++ doesn’t require tabs specifically; this is purely presentation.)

---

## 4) GameMode and PlayerController hookups

## 4.1 Main menu map

`MainMenuGameMode` already tries loading:
- `/Game/Remnantborn/Widgets/OnlineService/WBP_MainMenu`

So verify:
- Asset exists at that exact path/name, **or** update that C++ path if renamed.

In World Settings for your menu map:
- `GameMode Override` = your main menu game mode class (or BP child of it)

## 4.2 Match map

Create or open your BP child of `AMultiplayerPlayerController`.

In Class Defaults set:
- `MatchResultsWidgetClass` = `BP_MatchResultWidget`

Then in your multiplayer game mode (or BP child), ensure:
- `PlayerControllerClass` = that BP_MultiplayerPlayerController

This is required for:
- `Client_ShowMatchResults`
- `Client_SubmitMatchReward`
- reward display update via `OnMatchRewardReceived`

---

## 5) Character data + unlock setup

For each `CharacterDataAsset`:
- Free characters (Kade/Lira):
  - `bUnlockedByDefault = true`
  - `RemnantPrice = 0`
- Premium characters:
  - `bUnlockedByDefault = false`
  - `RemnantPrice > 0`
  - `StoreDescription` set

Important:
- Backend store catalog (`store_items`) is authoritative for purchase availability/price.
- Local `RemnantPrice` is mainly for local display consistency.

---

## 6) Backend-data assumptions that affect editor behavior

For store widgets to show items, backend must return active rows:
- `store_items`: active character rows
- `remnant_packages`: active package rows

If no active premium characters exist, `WBP_Store` should show `ComingSoonText`.

---

## 7) End-to-end test flow in editor

1. Play menu map.
2. Login.
3. Open Profile:
   - check username/level/remnants
   - check owned character cards populate in `CharacterScrollBox`
4. Open Store:
   - catalog loads
   - buy premium character
   - balance updates
5. Open Remnant Purchase:
   - packages load
   - select package + fake card fields
   - buy and confirm new balance
6. Start multiplayer match.
7. Finish match:
   - results widget opens
   - reward text shows (`+15` winner / `+5` participant)
8. Return to menu/profile:
   - remnant balance persists
   - purchased character is unlocked in character selection.

---

## 8) Quick troubleshooting

- Store page empty:
  - check backend running, API URL reachable
  - verify `store_items.is_active = true` rows exist
- Package cards not appearing:
  - in `WBP_RemnantPurchase`, set `RemnantPackageCardClass`
- Character cards not appearing in store:
  - in `WBP_Store`, set `StoreCharacterCardClass`
- Owned cards not appearing in profile:
  - in `WBP_UserProfile`, set `OwnedCharacterCardClass`
  - ensure `CharacterScrollBox` exists and named exactly
- Match reward UI not updating:
  - ensure `MatchResultsWidgetClass` is set on multiplayer player controller BP
  - ensure reward widgets exist (`RewardAmountText`, `NewBalanceText`)
- Main menu not spawning:
  - verify `WBP_MainMenu` path matches `/Game/Remnantborn/Widgets/OnlineService/WBP_MainMenu`

---

## 9) Recommended asset naming (for consistency)

- `WBP_MainMenu`
- `WBP_UserProfile`
- `WBP_Store`
- `WBP_StoreCharacterCard`
- `WBP_RemnantPurchase`
- `WBP_RemnantPackageCard`
- `WBP_OwnedCharacterCard`
- `BP_MatchResultWidget`
- `BP_MultiplayerPlayerController`

This naming keeps class-default assignments fast and reduces binding mistakes.
