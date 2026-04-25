# GAS Damage Burst Gender Grunt Setup

This document explains how to wire the new C++ implementation so `GameplayCue.Damage.Burst` plays a gender-specific grunt with per-character override support.

## What was added (C++)

- `ERemnantbornCharacterGender` enum on `ARemnantbornCharacterBase`:
  - `Male`
  - `Female`
- Per-character Blueprint-editable fields on `ARemnantbornCharacterBase`:
  - `CharacterGender`
  - `DamageBurstGruntOverrideSounds`
- New cue notify class:
  - `UGameplayCueNotify_DamageBurstGrunt`
  - Selects sound with priority:
    1. Character override sounds (if any)
    2. Cue-level fallback arrays by gender (`MaleDamageBurstGruntSounds` / `FemaleDamageBurstGruntSounds`)
  - Plays sound as 3D positional audio at damaged target location
  - Skips execution on dedicated server

## Editor wiring steps

1. Open `GC_DamageBurst` in the Gameplay Cues folder.
2. Reparent it to `GameplayCueNotify_DamageBurstGrunt`.
3. Keep existing VFX logic in the cue graph as-is.
4. If your cue Blueprint overrides `OnExecute`, make sure it calls parent so the C++ grunt logic runs.
5. In class defaults for the cue, assign:
   - `MaleDamageBurstGruntSounds`
   - `FemaleDamageBurstGruntSounds`
6. Ensure the cue tag is `GameplayCue.Damage.Burst`.
7. If you use `GC_DamageBurst_WithNiagra` in gameplay, repeat the same reparent/setup there.

Alternative: keep your current cue parent and call `SelectDamageBurstGruntSound` on the target `RemnantbornCharacterBase` from the cue graph, then use `Play Sound at Location` with the selected result.

## Per-character setup

For each character Blueprint derived from `BP_RemnantbornCharacterBase`:

1. Open the character Blueprint (example: Lira/Zoory).
2. In Class Defaults, set `CharacterGender` to `Male` or `Female`.
3. Optionally assign `DamageBurstGruntOverrideSounds`.
   - If this list is non-empty, it overrides gender fallbacks for that character.

## Multiplayer behavior

- The sound is played as world-space audio, so nearby players hear it.
- Selection is deterministic by target character config (gender + optional overrides).
- No additional replication fields are required for this feature.

## Quick test

1. Run 2-player PIE.
2. Apply damage to male and female characters.
3. Verify:
   - Male target -> male grunt fallback (or male character override)
   - Female target -> female grunt fallback (or female character override)
4. Clear one character override list and confirm fallback still works.
