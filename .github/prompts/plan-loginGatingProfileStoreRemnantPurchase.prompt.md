# Plan: Login Gating for Profile, Store & Remnant Purchase Screens

## TL;DR
Add a C++ `ApplyLoginGating(bool)` helper to each of the three widget classes. It toggles a `ContentPanel` container (Collapsed when not logged in) and a `LoginRequiredText` text widget (Collapsed when logged in). The Blueprint widget trees must be updated to wrap existing content in a container named `ContentPanel` and add a `TextBlock` named `LoginRequiredText` with the prompt message.

---

## Phase 1 — C++ changes (3 widgets)

### All three widgets follow the same pattern:

**Step 1: UserProfileWidget.h** — add under `protected:`:
- `UPROPERTY(meta = (BindWidgetOptional)) UWidget* ContentPanel;`
- `UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* LoginRequiredText;`
- Add `void ApplyLoginGating(bool bIsLoggedIn);` under `private:`

**Step 2: UserProfileWidget.cpp** — in `NativeConstruct`, after binding to GameInstance events, call `ApplyLoginGating(GameInstance->IsLoggedIn())`. In `HandleAuthStateChanged`, call `ApplyLoginGating(bIsLoggedIn)`.

**Step 3: StoreWidget.h** — same additions as Step 1.

**Step 4: StoreWidget.cpp**:
- `NativeConstruct`: guard `RefreshStore()` at end behind `IsLoggedIn()` check. Call `ApplyLoginGating` with initial state.
- `HandleAuthStateChanged`: logged-out branch — replace `SetNotification(TEXT("Please log in…"), true)` with just `ApplyLoginGating(false)`. Logged-in branch adds `ApplyLoginGating(true)`.
- Implement `ApplyLoginGating`: toggle `ContentPanel` and `LoginRequiredText` visibility.

**Step 5: RemnantPurchaseWidget.h** — same additions as Step 1.

**Step 6: RemnantPurchaseWidget.cpp**:
- `NativeConstruct`: guard `RefreshPackages()` at end behind `IsLoggedIn()`. Call `ApplyLoginGating` with initial state.
- `HandleAuthStateChanged`: same replacement as Step 4.
- Implement `ApplyLoginGating`.

Visibility rules in `ApplyLoginGating`:
- Logged in → `ContentPanel`: Visible; `LoginRequiredText`: Collapsed
- Not logged in → `ContentPanel`: Collapsed; `LoginRequiredText`: Visible
- Login message text: `"Please log in to access this feature."`

---

## Phase 2 — Blueprint widget tree changes (manual, in-editor)

For each of the 3 BP widgets (WB_UserProfile / WB_Store / WB_RemnantPurchase or equivalent):
1. Wrap all existing content inside a new container (`VerticalBox` or `Overlay`) named exactly `ContentPanel`
2. Add a `TextBlock` named exactly `LoginRequiredText` as a sibling
   - Default text: "Please log in to access this feature."
   - Default visibility: controlled by C++ at runtime; set to Collapsed in editor for non-logged-in state

---

## Relevant files
- `Source/Remnantborn/Remnantborn/Widgets/Auth/UserProfileWidget.h` — add `ContentPanel`, `LoginRequiredText`, `ApplyLoginGating`
- `Source/Remnantborn/Remnantborn/Widgets/Auth/UserProfileWidget.cpp` — call `ApplyLoginGating` in `NativeConstruct` + `HandleAuthStateChanged`
- `Source/Remnantborn/Remnantborn/Widgets/Store/StoreWidget.h` — add `ContentPanel`, `LoginRequiredText`, `ApplyLoginGating`
- `Source/Remnantborn/Remnantborn/Widgets/Store/StoreWidget.cpp` — guard `RefreshStore`, call `ApplyLoginGating`
- `Source/Remnantborn/Remnantborn/Widgets/Store/RemnantPurchaseWidget.h` — add `ContentPanel`, `LoginRequiredText`, `ApplyLoginGating`
- `Source/Remnantborn/Remnantborn/Widgets/Store/RemnantPurchaseWidget.cpp` — guard `RefreshPackages`, call `ApplyLoginGating`

---

## Verification
1. Compile C++ — no errors on all 3 widget pairs
2. Open each Blueprint widget in UMG editor; confirm `ContentPanel` and `LoginRequiredText` appear in the bind list
3. PIE test: launch game without logging in → navigate to Profile, Store, Remnant Purchase screens → only "Please log in to access this feature." should be visible
4. PIE test: log in → all three screens should show their normal content and the login message should disappear
5. PIE test: log out → screens revert to login-required message

---

## Decisions
- Login message: "Please log in to access this feature." (user-selected)
- `ContentPanel` collapses (not just hides) to avoid empty space while the login message is shown
- Store and RemnantPurchase: the `SetNotification("Please log in…")` calls are removed from logged-out branches since `LoginRequiredText` replaces them
- `RefreshStore` / `RefreshPackages` guarded in `NativeConstruct` — no network call if not logged in
