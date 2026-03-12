# Map Selection System — Editor Setup Steps

This document describes exactly what you need to do inside the Unreal Editor to finish the scalable map selection system. Follow each numbered step in order.

## 1. Create Data Asset Folder
1. Open the Content Browser.
2. Navigate to `/Game/Remnantborn/Blueprints` (create the path if it doesn't exist).
3. Right-click and choose **New Folder**. Name it `DataStructures`.
4. Inside `DataStructures`, create another folder named `MapDataAsset`.

## 2. Create a Map Data Asset
1. In the `MapDataAsset` folder, right-click and choose **Miscellaneous > Data Asset**.
2. When prompted, select the class `MapDataAsset` (you may need to search for it).
3. Name the new asset `DA_TestGround`.
4. Open `DA_TestGround` and fill in the fields:
   - **MapID**: `TestGround` (FName)
   - **MapName**: `Test Ground` (FText)
   - **MapDescription**: optional descriptive text.
   - **MapThumbnail**: assign a texture if desired.
   - **MapLevel**: click the dropdown and choose `/Game/Remnantborn/Levels/TestGround` (or any existing level to test).
5. Save the asset.

> **Tip:** New maps are added by duplicating `DA_TestGround` and changing `MapID`/fields.

## 3. Create Map Selector Entry Widget Blueprint
1. In the Content Browser, create a new folder `Widgets/Lobby` if it doesn't exist.
2. Right-click in that folder and choose **User Interface > Widget Blueprint**.
3. Name the blueprint `WBP_MapSelectorEntry`.
4. Open `WBP_MapSelectorEntry` in the UMG editor.
5. Add the following widgets (example layout):
   - `Border` or `HorizontalBox` as root.
   - Inside root, add an `Image` for the thumbnail; set its `Variable` name to `MapThumbnailImage`.
   - Add a `TextBlock` for the name; mark it `Variable` -> `MapNameText`.
   - Add a `Button` (variable `SelectButton`) which will be clicked to select the map.
6. Bind the `OnClicked` event of `SelectButton` to call `OnMapSelected` delegate or to a custom event that forwards `MapID`.
   (Alternatively, leave unbound—Blueprint wiring below will set it up dynamically.)
7. Compile and save the widget.

## 4. Update Lobby Widget Blueprint
1. Open `WBP_LobbyWidget` (the main lobby UI blueprint).
2. Add the following UI elements:
   - `HorizontalBox` container named `MapSelectorContainer`. Place it where host controls reside (e.g. near max-players buttons). Set Visibility to `Collapsed` by default.
   - Inside `MapSelectorContainer`, add a `VerticalBox` named `MapListContainer` (will hold entries).
   - Also add a `TextBlock` named `SelectedMapNameText` (outside/beside list, visible to all players).
3. In the **Details** panel of `WBP_LobbyWidget`, under Variables, add a new variable `MapEntryWidgetClass` of type `MapSelectorEntryWidget` (Object Reference, subclass). Set `Editable` and `Expose on Spawn` if needed.
4. Assign the default value of `MapEntryWidgetClass` to the `WBP_MapSelectorEntry` blueprint created earlier.
5. Compile and save.

## 5. Rebind Existing Lobby Widget Logic (if necessary)
1. Ensure `HostControlsContainer` is still present; `MapSelectorContainer` should be a peer of it or inside it based on desired layout.
2. Compile to check for any broken bindings (should match names exactly). Fix any missing references by reconnecting the widget variables.

## 6. Test in Editor
1. Launch a play-in-editor (PIE) multiplayer session with 2 clients.
2. Host should now see the map selector panel appear (since the widget class is set and map subsystem loads one entry).
3. Verify the `SelectedMapNameText` shows "Test Ground" by default.
4. Click the entry as host; ensure the name text updates and the selection is broadcast to the other client (they should see the name change too).
5. Start the match; confirm travel occurs to the test map.
6. Stop PIE.

## 7. Add Additional Maps (Verification)
1. Duplicate `DA_TestGround` and rename it to `DA_AnotherMap`.
2. Change fields appropriately (e.g., `MapID=AnotherMap`, `MapName="Another Map"`, `MapLevel` to a second level or same one for test).
3. Run PIE again; host list should now contain two entries automatically.
4. Selecting either entry should update the display and travel to corresponding map.

## Notes
- No C++ recompilation is required when adding or modifying map data assets.
- Map thumbnail and description are optional but improve the UI appearance.
- Keep `MapID` unique for each asset; the subsystem looks up by that value.

---
By following these steps in the editor, your map selection system will be fully operational and ready for further design iteration.