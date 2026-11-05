# ProjectB — Design & Implementation Plan

> Living document. Updated as the project evolves.

## Current state (November 2026)

- **Engine:** Unreal Engine 5.8 source build, `E:\unrealengine58\UE_5.8`
- **C++ foundation:** Already implemented.
  - `AProjectBCharacter` extends `APBPlayerCharacter` (PBCharacterMovement).
  - RPG stats (Health, Stability), combat states, ability/inventory/targeting APIs.
  - Shooter weapon bridge retained for compatibility.
- **Existing content:**
  - `Content/Blueprints/Pulsar_BP.uasset` — LR2 pulsar actor already exists.
  - `Content/Characters/Mannequins/` — default mannequins.
  - `Content/FirstPerson/` — first-person template leftovers.
  - `Content/jRPGTemplate/` — large marketplace/template pack (blueprints, datatables, demo maps, widgets, enums, structs).
  - `Content/Variant_Horror/`, `Content/Variant_Shooter/`, `Content/Weapons/` — template/variant assets.
- **Missing:** White Tree environment import, main menu, clean UI framework, asset organization.

## Goal

A single, coherent UE5 project that:

1. Satisfies the university labs (LR2, LR3, LR6) by December 2026.
2. Becomes a playable action/exploration prototype by May 2027.
3. Runs acceptably on an RTX 3050 with limited disk space.

## Non-goals

- Do not ship a clone of *Expedition 33*.
- Do not keep every marketplace template asset permanently.
- Do not enable high-end rendering features just because they exist.

---

## Asset organization

### Rule: do not move existing assets with the file system

Moving `.uasset`/`.umap` files outside the Unreal Editor breaks hard references. The existing folders are left alone until they are migrated in-editor.

### New clean structure (for all new work)

```text
Content/
  ProjectB/
    Maps/
      LMB_MainMenu
      L_WhiteTree_Playground
      L_Lab02_Pulsar
      L_Lab03_Collision
      L_Lab06_Chests
      L_Dev_Mechanics
    UI/
      WBP_MainMenu
      WBP_HUD
      WBP_InteractionPrompt
      WBP_PauseMenu
      WBP_Options_Audio
      WBP_Options_Video
      WBP_Options_Controls
    Blueprints/
      Player/
        BP_ProjectBPlayerCharacter
        BP_ProjectBPlayerController
        BP_ProjectBGameMode
      Actors/
        BP_Interactable_Base
        BP_Pulsar
        BP_Chest
        BP_Door
        BP_Lever
      Interfaces/
        BPI_Interactable
        BPI_ActivateObject
      Abilities/
      Gameplay/
    Input/
      IMC_ProjectBDefault
      IA_Move
      IA_Look
      IA_Jump
      IA_Interact
      IA_Attack
      IA_Aim
      IA_Sprint
      IA_Crouch
      IA_Pause
    Environments/
      WhiteTree/
        (imported USD scene and its materials)
    Labs/
      Lab02/
      Lab03/
      Lab06/
```

### Migration plan for existing assets

| Source folder | Action | Destination |
|---------------|--------|-------------|
| `Content/Blueprints/Pulsar_BP.uasset` | Migrate in-editor | `Content/ProjectB/Blueprints/Actors/BP_Pulsar` |
| `Content/FirstPerson/` | Evaluate, then delete or migrate | `Content/ProjectB/Player/` if kept |
| `Content/jRPGTemplate/` | Keep but isolate; only reference systems we actually use | Leave in place; create wrappers in `Content/ProjectB/Blueprints/` |
| `Content/Variant_Horror/`, `Content/Variant_Shooter/`, `Content/Weapons/` | Audit before keeping | Delete or move to `_Deprecated/` |
| `Content/Characters/Mannequins/` | Keep until MetaHuman is ready | `Content/ProjectB/Characters/` later |

---

## Map plan

### LMB_MainMenu

- Empty level with a camera pointed at a simple backdrop.
- GameMode override: `BP_MenuGameMode` (no player character, UI-only).
- On begin play: create `WBP_MainMenu` and add to viewport.

### L_WhiteTree_Playground

- Import White Tree USD scene into `Content/ProjectB/Environments/WhiteTree/`.
- Add:
  - Player start.
  - Minimal lighting pass (use existing USD lighting if possible).
  - A few test interactables (chest, pulsar).
- Use this map to validate movement, camera, interaction, and combat feel.

### L_Lab02_Pulsar, L_Lab03_Collision, L_Lab06_Chests

- Dedicated small maps for each lab.
- Each map contains only what the lab requires, wired to the same `BP_ProjectBPlayerCharacter`.
- Labs are not separate projects — they are maps inside ProjectB.

### L_Dev_Mechanics

- Sandbox map for testing movement, combat, and abilities.
- Contains Karpov/test NPCs, dummy targets, and playground geometry.

---

## UI plan

### Widgets

| Widget | Purpose |
|--------|---------|
| `WBP_MainMenu` | New Game, Continue, Options, Exit |
| `WBP_HUD` | Health bar, stability bar, interaction prompt, crosshair/reticle |
| `WBP_InteractionPrompt` | Shows "[E] Interact" style prompts |
| `WBP_PauseMenu` | Resume, Options, Quit to Main Menu, Quit to Desktop |
| `WBP_Options_*` | Sub-menus for video/audio/controls |

### Menu flow

```text
Boot
  -> LMB_MainMenu
       -> New Game -> L_WhiteTree_Playground
       -> Continue -> last played map (save system later)
       -> Options  -> WBP_Options_Video/Audio/Controls
       -> Exit     -> QuitGame

In-game
  -> WBP_HUD always visible
  -> Interact -> WBP_InteractionPrompt
  -> Pause (Esc) -> WBP_PauseMenu
```

### Input conventions

| Action | Keyboard/Mouse | Gamepad |
|--------|---------------|---------|
| Move | WASD | Left stick |
| Look | Mouse | Right stick |
| Jump | Space | A/Cross |
| Interact | E | X/Square |
| Attack | LMB | RT/R2 |
| Aim/Block | RMB | LT/L2 |
| Sprint | Shift | L3 |
| Crouch | Ctrl | B/Circle |
| Pause/Menu | Esc | Start |

---

## Gameplay loop (initial)

1. Spawn in White Tree playground.
2. Move and explore using PBCharacterMovement.
3. Approach interactable actors (chests, doors, pulsars).
4. Press Interact to activate.
5. Unarmed melee attack for combat tests.
6. Health/stability tracked on HUD.

Later additions:
- Enemies with basic AI (Behaviour Tree/Blackboard).
- Weapon pickups.
- Simple quest/objective marker.

---

## Rendering & performance

Current `DefaultEngine.ini` enables Lumen, Virtual Shadow Maps, Ray Tracing, and Substrate. For an RTX 3050 this is heavy.

Recommended config for development:

```ini
[/Script/Engine.RendererSettings]
r.ReflectionMethod=1                ; Lumen reflections (keep if performant, else 0)
r.DynamicGlobalIlluminationMethod=1 ; Lumen GI
r.AllowStaticLighting=False
r.Shadow.Virtual.Enable=1
r.RayTracing=False                  ; disable RTX for dev on RTX 3050
r.Substrate=False                   ; disable Substrate unless required
r.DefaultFeature.AutoExposure.ExtendDefaultLuminanceRange=True
r.DefaultFeature.LocalExposure.HighlightContrastScale=0.8
r.DefaultFeature.LocalExposure.ShadowContrastScale=0.8
```

If framerate is still poor, drop to:

```ini
r.ReflectionMethod=0
r.DynamicGlobalIlluminationMethod=0
r.Shadow.Virtual.Enable=0
```

Only re-enable high-end features after the gameplay loop is solid.

---

## Disk-space mitigation

Because `C:\` keeps running out of space:

1. Keep the project on `E:\`.
2. Move NuGet caches to `E:\`:
   - `NUGET_PACKAGES=E:\NuGet\packages`
   - `NUGET_HTTP_CACHE_PATH=E:\NuGet\v3-cache`
3. Move Unreal Build Accelerator cache to `E:\` via `BuildConfiguration.xml` in `Saved\UnrealBuildTool\`.
4. Periodically delete `Saved\Logs`, `DerivedDataCache`, and `Intermediate` after backups.
5. Do not commit `Saved/` or `DerivedDataCache/` to Git (already in `.gitignore`).

---

## Lab integration checklist

### LR2 — Pulsar

- [x] `BP_Pulsar` exists.
- [ ] Migrate to `Content/ProjectB/Blueprints/Actors/BP_Pulsar`.
- [ ] Create `L_Lab02_Pulsar` map.
- [ ] Verify BeginPlay -> Timeline -> Point Light intensity/color.

### LR3 — Collision & random color

- [ ] Extend `BP_Pulsar` with sphere collision.
- [ ] On overlap with player, call custom event/function to set random color.
- [ ] Create `L_Lab03_Collision` map.

### LR6 — Chest & interface

- [ ] Create `BPI_ActivateObject` interface.
- [ ] Create `BP_Chest` implementing the interface.
- [ ] Timeline: open lid on activate.
- [ ] Create inherited chest variant.
- [ ] Create `BP_Dispatcher_Test` to demonstrate Event Dispatcher.
- [ ] Create `L_Lab06_Chests` map.

---

## Next immediate actions

1. Create the main menu map and widget in-editor.
2. Import White Tree USD into `Content/ProjectB/Environments/WhiteTree/`.
3. Adjust rendering config for RTX 3050.
4. Migrate `Pulsar_BP` to the new `ProjectB` folder structure.
5. Wire `DefaultEngine.ini` to use `LMB_MainMenu` as `GameDefaultMap`.
