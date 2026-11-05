# ProjectB — UI Plan

## Widget inventory

All UI widgets live under `Content/ProjectB/UI/`.

### WBP_MainMenu

**Purpose:** First screen after boot.

**Buttons:**
- New Game → opens `L_WhiteTree_Playground`
- Continue → opens last played map (placeholder)
- Options → opens `WBP_Options_Video` or a tabbed options panel
- Exit → `QuitGame`

**Visual notes:**
- Full-screen dark backdrop.
- Centered vertical button list.
- Large project title text (placeholder: "ProjectB").

### WBP_HUD

**Purpose:** Always-visible in-game HUD.

**Elements:**
- Health bar (top-left)
- Stability bar (below health)
- Interaction prompt (center-bottom)
- Crosshair/reticle (center)
- Optional: minimap, objective text (later)

**Binding:**
- Bind to `AProjectBCharacter::OnRPGHealthChanged` and `OnRPGStabilityChanged`.
- Use `GetPlayerCharacter` cast to `AProjectBCharacter`.

### WBP_InteractionPrompt

**Purpose:** Show context-sensitive interact text.

**Elements:**
- Single text block: `"[E] Open Chest"`, `"[E] Activate"`, etc.
- Hidden by default.

**Usage:**
- Interactable actor calls `ShowPrompt` on overlap.
- Calls `HidePrompt` on end overlap.

### WBP_PauseMenu

**Purpose:** Pause overlay.

**Buttons:**
- Resume
- Options → `WBP_Options_*`
- Quit to Main Menu → `OpenLevel LMB_MainMenu`
- Quit to Desktop → `QuitGame`

**Behavior:**
- On open: set input mode UI-only, show cursor, pause game.
- On close: set input mode game-only, hide cursor, unpause.

### WBP_Options_Video

**Controls:**
- Resolution dropdown
- Window mode (Fullscreen/Windowed/Borderless)
- VSync toggle
- Frame rate limit slider
- Quality preset dropdown (Low/Medium/High/Epic)
- Shadow quality
- Effects quality
- View distance
- Anti-aliasing

### WBP_Options_Audio

**Controls:**
- Master volume
- Music volume
- SFX volume
- Voice volume

### WBP_Options_Controls

**Controls:**
- Mouse sensitivity slider
- Invert Y toggle
- Show key rebind list (later)

---

## UI manager pattern

Use a single `BP_ProjectBGameInstance` or `BP_ProjectBPlayerController` to:

1. Create and store references to persistent widgets.
2. Handle level transitions without losing UI state.
3. Apply video/audio options.

For now, keep it simple:
- `BP_ProjectBPlayerController` creates `WBP_HUD` in `BeginPlay`.
- `BP_ProjectBPlayerController` handles pause menu on `IA_Pause`.
- Main menu uses its own GameMode/PlayerController.

---

## Naming conventions

| Prefix | Meaning |
|--------|---------|
| `WBP_` | Widget Blueprint |
| `L_` | Game level map |
| `LMB_` | Menu level map |
| `LDev_` | Development/test map |
| `BP_` | Actor Blueprint |
| `BPI_` | Blueprint Interface |
| `IMC_` | Input Mapping Context |
| `IA_` | Input Action |

---

## Implementation order

1. `WBP_MainMenu` + `LMB_MainMenu` + menu GameMode.
2. `WBP_HUD` + bind health/stability.
3. `WBP_InteractionPrompt` + interactable actors.
4. `WBP_PauseMenu`.
5. Options sub-menus.
