# Widget Creation & Update Guide

This document provides step-by-step instructions and widget hierarchies for all new and updated widgets mentioned in `README_UNREAL_EDITOR_SETUP.md`. Follow these steps exactly inside Unreal Editor to ensure correct integration.

---

## 1. Store Character Card (New Widget)

**Blueprint Name:** `WBP_StoreCharacterCard`  
**Parent Class:** `StoreCharacterCardWidget`  
**Suggested Path:** `/Game/Remnantborn/Widgets/Store/`

### Designer Hierarchy
```
Border (root, padding=8, background=semi‑opaque dark)
└─ VerticalBox (spacing=4, align=center)
   ├─ Image: CharacterPortrait (optional, size 128x128)
   ├─ TextBlock: CharacterName (font=Bold, center)
   ├─ TextBlock: CharacterDescription (wrap, center)
   ├─ HorizontalBox (justify=space-between)
   │   ├─ TextBlock: PriceText (font=Bold)
   │   └─ Button: BuyButton
   │       └─ TextBlock: BuyButtonText
```

### Steps
1. Right-click in Content Browser ➜ User Interface ➜ Widget Blueprint.  
2. Name it `WBP_StoreCharacterCard` and set parent to `StoreCharacterCardWidget`.  
3. Open Blueprint, switch to Designer tab.  
4. Add widgets listed above with exact names.  
5. In Graph, update `InitializeCard` call:
   * call the new 2‑argument version and pass in `CharacterDataAsset` lookup from `CharacterId` (use `GetCharacterSelectionSubsystem` nodes).
   * set `CharacterPortrait` brush from the asset if non-null, otherwise fall back to loading `CharacterInfo.ImageUrl` via your existing image download logic.
6. Save and close.

---

## 2. Remnant Package Card (New Widget)

**Blueprint Name:** `WBP_RemnantPackageCard`  
**Parent Class:** `RemnantPackageCardWidget`  
**Suggested Path:** `/Game/Remnantborn/Widgets/Store/`

### Designer Hierarchy
```
Border (root, padding=6, border-image=nine-slice)
└─ VerticalBox (spacing=2)
   ├─ TextBlock: PackageNameText (font=Medium, center)
   ├─ TextBlock: AmountText (font=Large, center)
   ├─ TextBlock: PriceText (font=Bold, center)
   ├─ TextBlock: BestValueText (optional, color=gold)
   └─ Button: SelectButton (full-width)
```

### Steps
1. Create a Widget Blueprint with the specified name and parent.  
2. Add the widgets above, respecting exact naming.  
3. Save.

---

## 3. Store Page (New Widget)

**Blueprint Name:** `WBP_Store`  
**Parent Class:** `StoreWidget`  
**Path:** `/Game/Remnantborn/Widgets/Store/`

### Designer Hierarchy
```
Border (root, padding=10)
└─ VerticalBox (spacing=8)
   ├─ TextBlock: BalanceText (font=Large, align=center)
   ├─ WrapBox: CharacterGrid (uniform grid, wrap)
   ├─ TextBlock: ComingSoonText (hidden by default, center)
   ├─ CircularThrobber: LoadingSpinner (centered overlay)
   └─ TextBlock: NotificationText (color=warning)
```

### Steps
1. Create `WBP_Store` using the path and parent class.  
2. Add widgets and name them exactly.  
3. In Class Defaults, set `StoreCharacterCardClass` to `WBP_StoreCharacterCard`.  
4. Save blueprint.

---

## 4. Remnant Purchase Page (New Widget)

**Blueprint Name:** `WBP_RemnantPurchase`  
**Parent Class:** `RemnantPurchaseWidget`  
**Path:** `/Game/Remnantborn/Widgets/Store/`

### Designer Hierarchy
```
Border (root, padding=10)
└─ VerticalBox (spacing=6)
   ├─ TextBlock: BalanceText (font=Large, align=center)
   ├─ WrapBox: PackageContainer (item spacing=4)
   ├─ TextBlock: SelectedPackageText (bold)
   ├─ HorizontalBox (spacing=4)
   │   ├─ VerticalBox
   │   │   ├─ TextBlock: CardNumberLabel (small)
   │   │   └─ EditableTextBox: CardNumberInput
   │   ├─ VerticalBox
   │   │   ├─ TextBlock: CardExpiryLabel (small)
   │   │   └─ EditableTextBox: CardExpiryInput
   │   └─ VerticalBox
   │       ├─ TextBlock: CardCVVLabel (small)
   │       └─ EditableTextBox: CardCVVInput
   ├─ Button: PayNowButton (full-width, style=primary)
   │   └─ TextBlock: PayNowButtonText
   ├─ TextBlock: NotificationText (color=warning)
   └─ CircularThrobber: LoadingSpinner (centered overlay)
```

### Steps
1. Create the widget blueprint.  
2. Add all listed widgets with proper names.  
3. In Class Defaults, assign `RemnantPackageCardClass` to `WBP_RemnantPackageCard`.  
4. Save.

---

## 5. Owned Character Card (New Widget)

**Blueprint Name:** `WBP_OwnedCharacterCard`  
**Parent Class:** `OwnedCharacterCardWidget`  
**Path:** `/Game/Remnantborn/Widgets/Auth/`

### Designer Hierarchy
```
Border (root, padding=5, background=semi‑transparent)
└─ HorizontalBox (spacing=4, align=center)
   ├─ Image: CharacterPortrait (optional, size=64x64)
   └─ VerticalBox
       ├─ TextBlock: CharacterName (optional, font=Medium)
       └─ TextBlock: StatusBadge (optional, color=accent)
```

### Steps
1. Create new widget blueprint with given name and parent.  
2. Add optional widgets, naming them exactly if used.  
3. Save.

---

## 6. User Profile Widget (Update Existing)

**Blueprint Name:** `WBP_UserProfile`  
**Parent Class:** `UserProfileWidget`

### Required Bindings
- TextBlock: UsernameText
- TextBlock: LevelText
- TextBlock: RemnantText
- TextBlock: EmailText
- Image: AvatarImage
- Button: ChangeAvatarButton (opens OS file dialog)
- Button: LogoutButton
- Button: RefreshButton

### Optional but Recommended
- TextBlock: BioText
- TextBlock: MemberSinceText
- ScrollBox: CharacterScrollBox

### Class Defaults
- Set `OwnedCharacterCardClass` to `WBP_OwnedCharacterCard`

### Steps
1. Open existing `WBP_UserProfile`.  
2. Ensure all required widgets are present and bound.  
3. Add optional widgets if not already.  
4. Update Class Defaults.  
5. Save.

---

## 7. Match Results Widget (Update Existing)

**Blueprint Name:** `BP_MatchResultWidget`  
**Parent Class:** `MatchResultsWidget`

### Existing Bound Widgets (Verify)
- TitleText, WinnerText, PersonalResultText
- VictoryOverlay, DefeatOverlay
- ResultsScrollBox, PlayerResultsBox
- ReturnToLobbyButton, PlayAgainButton

### New Reward Widgets (Add)
- TextBlock: RewardAmountText
- TextBlock: NewBalanceText
- Image: RewardRemnantIcon

### Steps
1. Open `BP_MatchResultWidget`.  
2. Confirm existing bindings.  
3. Add the reward widgets and position them.  
4. Save.

---

## 8. MainMenu Widget Updates (Existing)

**Blueprint Name:** `WBP_MainMenu`  
**Parent Class:** `MainMenuWidget`

### Add Buttons in Designer
- Button: OpenStoreButton
- Button: OpenRemnantPurchaseButton

*The existing Profile button has been replaced by an avatar image. In the designer remove the old ProfileButton and add the following:*
### Graph Logic
- On `OpenStoreButton.OnClicked`:
  - Create Widget `WBP_Store`, Add to Viewport (Z=100)
- On `OpenRemnantPurchaseButton.OnClicked`:
  - Create Widget `WBP_RemnantPurchase`, Add to Viewport (Z=100)

- **Avatar click (optional)**: if you wrapped the image in `ProfileAvatarButton`, wire its `OnClicked` event to call the `OnAvatarClicked` function exposed by `MainMenuWidget`.

### Class Defaults (verify)
- `LoginWidgetClass` = your login widget BP
- `ProfileWidgetClass` = `WBP_UserProfile`

- (NEW) `ProfileAvatarImage` must be bound to the avatar image widget in the designer. If you use a button wrapper, also bind `ProfileAvatarButton`.

> **Optional:** Implement tabbed layout with `WidgetSwitcher` and 4 tab buttons for Play/Profile/Store/Remnants.

---

### 9. General Notes
- All widget names must match exactly; case-sensitive.
- Blueprint classes referenced in defaults must exist in the Content Browser at the paths used.
- After creating/updating widgets, compile and save each blueprint.

Follow this guide sequentially during setup to avoid missing dependencies.

---

## 10. Styling & Attractiveness Tips 🎨
To make the new and updated widgets visually appealing, apply the following best practices while authoring them in Unreal Editor:

### Layout & Composition
- **Consistent spacing:** Use `SizeBox`, `Padding`, and `ScaleBox` widgets to maintain uniform margins between elements.
- **Alignment helpers:** Leverage `HorizontalBox`, `VerticalBox`, `GridPanel`, and `WrapBox` for responsive layouts.
- **Hierarchy clarity:** Group related elements inside `Border` or `CanvasPanel` containers so designers can see the structure at a glance.

### Colors & Typography
- **Game palette:** Stick to your project’s color palette for text, backgrounds, and borders. Use the `Color` picker and save swatches as variables for reuse.
- **Readable fonts:** Choose a clear font (e.g. Roboto, Montserrat) and set contrasting colors for text vs.
 background.
- **Dynamic text styles:** Use `SlateFontInfo` and the `SetFont` node in the graph to adjust size/style based on device resolution.

### Imagery & Icons
- **High‑quality assets:** Import 2x or 4x-resolution PNGs for character portraits, icons, and buttons; configure `Texture` settings with `UI` compression.
- **Nine-slice borders:** Use `Border` widgets with nine-slice images for scalable panels that keep corners sharp.
- **Animated images:** For animations, consider `Flipbook` or `Widget Animation` timelines to add subtle motion (e.g., pulsing buy button).

### Buttons & Interactions
- **Hover/pressed states:** Define `Style` brush colors or images for normal, hovered, and pressed states of `Button` widgets.
- **Audio feedback:** Add `OnHovered`/`OnClicked` events to play UI sounds or trigger light animations.
- **Visual affordance:** Use drop shadows or glow effects (via `ShadowOffset`/`ShadowColor`) to make interactive elements stand out.

### Animations & Transitions
- Create `Widget Animations` for entrances/exits (fade-ins, slide-ups) and loop small attention-getting effects (e.g. blinking "Best Value" label).
- Use `Play Animation` nodes in the graph to trigger animations when the widget is constructed or when data changes.

### Responsiveness & Scaling
- Set up `Scale Boxes` around content so the layouts adapt to different resolutions and aspect ratios.
- Test widgets in `Standalone Game` at various resolutions using the `Aspect Ratio Preview` panel in Designer.

### Accessibility
- Provide tooltip text using `SetToolTip` nodes for icons or unclear controls.
- Ensure color choices meet contrast ratios for readability.

> 💡 **Tip:** Create a `UI_Style` data asset or a shared `Widget Blueprint` containing common styled elements (buttons, borders, fonts) so the entire UI stays consistent and easier to tweak globally.

Applying these recommendations will turn functional widgets into engaging UI elements that feel polished and integrate nicely with your game’s visual identity.