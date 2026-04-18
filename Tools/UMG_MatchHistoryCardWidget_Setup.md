# WBP_MatchHistoryCardWidget Setup

Create a new Widget Blueprint at:
- /Game/Widgets/Auth/WBP_MatchHistoryCardWidget

Parent class:
- MatchHistoryCardWidget (C++ class)

Bind these optional widget names exactly:
- ResultText (TextBlock)
- MapText (TextBlock)
- PlacementText (TextBlock)
- DurationText (TextBlock)
- MetaText (TextBlock)

Recommended layout:
- Root: Border
- Child: VerticalBox
- Row 1: HorizontalBox with ResultText (left) and MapText (right)
- Row 2: HorizontalBox with PlacementText (left) and DurationText (right)
- Row 3: MetaText (wrap enabled)

Suggested styling:
- Border background color: #0F1724 with 85% opacity
- Border padding: 12
- ResultText: bold, slightly larger (14-16)
- Map/Placement/Duration: regular size 12-14
- MetaText: muted color and wrap enabled

Behavior notes:
- ResultText color is set by C++ each update:
  - WIN: green
  - LOSS: red
  - DRAW: gold
- If this Blueprint exists at the path above, UserProfileWidget auto-loads it.
- If missing, system falls back to C++ MatchHistoryCardWidget dynamic layout.
