# Plan: Scalable Map Selection System

## Overview
Create a Data Asset-driven map selection system for the lobby, mirroring the existing CharacterDataAsset/CharacterSelectionSubsystem pattern. Host selects a map from the lobby UI → replicated to all clients → used at ServerTravel.

## Phase 1 — UMapDataAsset (new C++ class)
- Create `Source/Remnantborn/Remnantborn/MapSelection/MapDataAsset.h/.cpp`
- Inherits `UDataAsset`, `BlueprintType`
- Properties: `MapID` (FName), `MapName` (FText), `MapDescription` (FText), `MapThumbnail` (UTexture2D*), `MapLevel` (TSoftObjectPtr<UWorld>)
- Helper: `FString GetMapPath() const` → `MapLevel.ToSoftObjectPath().GetLongPackageName()` → package path suitable for ServerTravel

## Phase 2 — UMapSelectionSubsystem (new C++ class)
- Create `Source/Remnantborn/Remnantborn/MapSelection/MapSelectionSubsystem.h/.cpp`
- Inherits `UGameInstanceSubsystem`
- `Initialize()` calls `LoadAvailableMaps()`
- `LoadAvailableMaps()`: AssetRegistry scan of `/Game/Remnantborn/Blueprints/DataStructures/MapDataAsset/` for `UMapDataAsset` assets (mirrors `CharacterSelectionSubsystem::LoadCharactersFromDirectory`)
- Public functions: `GetAvailableMaps()` → `TArray<UMapDataAsset*>`, `GetMapByID(FName)` → `UMapDataAsset*`, `GetDefaultMap()` → first in array

## Phase 3 — LobbyGameState (modify)
- Add `UPROPERTY(ReplicatedUsing=OnRep_SelectedMapID) FName SelectedMapID`
- `OnRep_SelectedMapID()` → `OnLobbyStateChanged.Broadcast()` (free UI refresh on all clients)
- Add `void SetSelectedMap(FName NewMapID)` (server-side setter for GameMode to call)
- `GetLifetimeReplicatedProps`: add `DOREPLIFETIME(ALobbyGameState, SelectedMapID)`

## Phase 4 — LobbyGameMode (modify)
- Remove hardcoded `GameMapPath` UPROPERTY
- Add `UFUNCTION(BlueprintCallable) void SetSelectedMap(FName MapID)` — sets `LobbyGameState::SelectedMapID` via `SetSelectedMap()`
- `BeginPlay()`: after loading characters, query `UMapSelectionSubsystem`→`GetDefaultMap()` and call `SetSelectedMap(DefaultMap->MapID)` to seed initial state
- `ExecuteMatchTravel()`: resolve path via `UMapSelectionSubsystem::GetMapByID(LobbyGS->SelectedMapID)->GetMapPath()`, fallback to `/Game/Remnantborn/Levels/TestGround` if lookup fails

## Phase 5 — LobbyPlayerController (modify)
- Add `UFUNCTION(BlueprintCallable) void SetSelectedMap(FName MapID)` — guards with `if (!IsHost()) return`
- Add private `UFUNCTION(Server, Reliable, WithValidation) void Server_SetSelectedMap(FName MapID)`
- Validate: `return !MapID.IsNone()`
- Implementation: `Cast<ALobbyGameMode>(GetWorld()->GetAuthGameMode())->SetSelectedMap(MapID)`

## Phase 6 — UMapSelectorEntryWidget (new C++ class)
- Create `Source/Remnantborn/Remnantborn/Widgets/Lobby/MapSelectorEntryWidget.h/.cpp`
- Mirrors `UPlayerListEntryWidget` pattern
- `void SetMapInfo(UMapDataAsset* MapData, bool bIsSelected)`
- `DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMapEntrySelected, FName, MapID) OnMapSelected`
- BindWidget properties: `UTextBlock* MapNameText`, `UImage* MapThumbnailImage`, `UButton* SelectButton`
- `SelectButton` OnClicked → broadcast `OnMapSelected`

## Phase 7 — LobbyWidget (modify)
- `.h`: Add `UPROPERTY(meta=(BindWidget)) UHorizontalBox* MapSelectorContainer` (host-only, like `HostControlsContainer`)
- `.h`: Add `UPROPERTY(meta=(BindWidget)) UTextBlock* SelectedMapNameText` (visible to all players)
- `.h`: Add `UPROPERTY(meta=(BindWidget)) UVerticalBox* MapListContainer` (inside MapSelectorContainer)
- `.h`: Add `UPROPERTY(EditAnywhere, Category="Widgets") TSubclassOf<UMapSelectorEntryWidget> MapEntryWidgetClass`
- `.cpp`: `UpdateUI()` calls new `UpdateMapSelection()`
- `UpdateMapSelection()`: query `UMapSelectionSubsystem`→`GetAvailableMaps()`, rebuild map entry list in `MapListContainer`; update `SelectedMapNameText` from `LobbyGS->SelectedMapID`; control `MapSelectorContainer` visibility (host only)
- Each entry: `OnMapSelected` delegates bound to lambda calling `LobbyPC->SetSelectedMap(MapID)`
- `RebuildMapList()` helper: removes old entries, creates new `UMapSelectorEntryWidget` per map, marks selected one

## Phase 8 — Content & Blueprint Setup (editor work)
1. Create folder `/Game/Remnantborn/Blueprints/DataStructures/MapDataAsset/`
2. Create `DA_TestGround`: MapID=`TestGround`, MapName="Test Ground", MapLevel→`/Game/Remnantborn/Levels/TestGround`
3. Create `WBP_MapSelectorEntry` Blueprint (parent: `UMapSelectorEntryWidget`): thumbnail image + name text + select button with proper UMG layout
4. Update `WBP_LobbyWidget`: add `MapSelectorContainer` (HBox, host-only), `SelectedMapNameText` (visible to all), `MapListContainer` (VBox inside MapSelectorContainer). Assign `MapEntryWidgetClass = WBP_MapSelectorEntry`
5. No C++ changes needed when adding new maps — just create a new `UMapDataAsset` in the folder

## Key Files
- `Source/Remnantborn/Remnantborn/MapSelection/MapDataAsset.h/.cpp` — new
- `Source/Remnantborn/Remnantborn/MapSelection/MapSelectionSubsystem.h/.cpp` — new
- `Source/Remnantborn/Remnantborn/Widgets/Lobby/MapSelectorEntryWidget.h/.cpp` — new
- `Source/Remnantborn/Remnantborn/GameModes/LobbyGameState.h/.cpp` — add SelectedMapID replicated
- `Source/Remnantborn/Remnantborn/GameModes/LobbyGameMode.h/.cpp` — remove hardcoded path, add SetSelectedMap, update ExecuteMatchTravel
- `Source/Remnantborn/Remnantborn/OnlineService/LobbyPlayerController/LobbyPlayerController.h/.cpp` — add Server_SetSelectedMap RPC
- `Source/Remnantborn/Remnantborn/Widgets/Lobby/LobbyWidget.h/.cpp` — map UI widgets + UpdateMapSelection()

## Verification
1. Compile successfully with no errors
2. In editor: create DA_TestGround asset, create WBP_MapSelectorEntry, update WBP_LobbyWidget bindings
3. Host lobby: map selector panel visible only to host, shows available maps
4. Host selects a map → all clients see SelectedMapNameText update (via OnLobbyStateChanged)
5. Match starts → server travels to the selected map
6. Add a second DA for a new dummy map → it auto-appears in the host's list with zero code changes

## Decisions
- `TSoftObjectPtr<UWorld>` for map path (editor-validated, type-safe) over raw FString
- AssetRegistry scan (no AssetManager) mirrors existing CharacterSelectionSubsystem exactly
- SelectedMapID lives on LobbyGameState (replicated) — leverages existing OnLobbyStateChanged broadcast chain
- Map selection is host-only (no voting); clients see the current selection as read-only text
- Default map set at LobbyGameMode::BeginPlay() to first discovered asset
- All maps are freely available (no unlock/store system for maps in this plan)
