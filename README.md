# ProjectB

> **WORK IN PROGRESS — This README will be updated as the project evolves.**

## Project Information

ProjectB is a **Unreal Engine 5.8 C++ project** built against a source-built engine at `E:\unrealengine58\UE_5.8`. It serves two parallel goals:

1. **Academic track** — complete university labs for *“Programming Graphics Applications”* (Программирование графических приложений) by December 2026.
2. **Personal prototype track** — grow the same project into an original gameplay prototype / coursework by May 2027.

The project deliberately uses **UE5 instead of the method-book’s UE4**, adapting the lab concepts to the modern editor rather than forcing an outdated engine. It is also **not** a clone of *Expedition 33* or any other game; the White Tree environment is only a temporary test sandbox.

## Plans & Roadmap

### Immediate (now — December 2026)

- Stabilize the build environment (resolve disk-space / `C:\` saturation causing NuGet/UBA/UBT failures).
- Ensure `PBCharacterMovement` compiles cleanly for UE 5.8.
- Replace deprecated `UCharacterMovementComponent::GetMovementBase()` usage.
- Set up a sane `Content/` and `Source/` folder structure.
- Integrate required labs into the main project as subsystems/maps:
  - **LR2:** `BP_Pulsar` — `BeginPlay`, `Print String`, `Point Light`, `Timeline`, intensity/color changes.
  - **LR3:** extend `BP_Pulsar` with custom events/functions, collision sphere, `OnComponentBeginOverlap`, `Cast To`, random color.
  - **LR6:** `BP_Chest`, Blueprint Interface (`BPI_ActivateObject`), lid rotation, inheritance, Event Dispatcher.
- Import the already-exported White Tree USD scene and make it playable.
- Evaluate MetaHuman for a non-standard but still human body silhouette before considering Blender/rigging workarounds.

### Mid-term (December 2026 — May 2027)

- Build a solid **movement + exploration** core using PBCharacterMovement.
- Implement a generic **interaction system** (`BPI_Interactable`).
- Start **unarmed/melee combat** (fists first) and design a reusable combat loop.
- Add an **abilities/progression** layer (health/stamina, ability slots, basic HUD).
- Package everything into a coherent coursework prototype that goes beyond the lab checklist.

### Long-term (post-May 2027)

- Expand the prototype into a full original game concept.
- Replace White Tree with original levels once core systems are proven.
- Consider multiplayer or advanced systems only after the single-player loop is solid.

## Details

### Engine & environment

- **Engine:** Unreal Engine 5.8 (source build)
- **Engine path:** `E:\unrealengine58\UE_5.8`
- **Project path:** `E:\ue5project\ProjectB`
- **Visual Studio toolchain:** 14.50 / Windows 10.0.22621.0 SDK
- **Key plugin:** `PBCharacterMovement`

### Known issues

- The C++ code itself compiled successfully in the last build attempt, but the overall build failed because `C:\` ran out of space during NuGet restore and UBA cache writes.
- One code warning exists: `GetMovementBase` is deprecated; use `GetMovementBaseObject` cast instead.

### Folder conventions

```text
Content/
  Maps/
    Lab02_Map
    Lab03_Map
    Lab06_Map
    WhiteTree_Playground
  Blueprints/
    BP_Pulsar
    BP_Chest
    BP_PlayerCharacter
  Interfaces/
    BPI_ActivateObject
  Materials/
  MetaHumans/
```

### Deliberately out of scope for now

- Lore-finalized RPG concept
- Chess Arena as a required feature
- Firearms or sword combat before unarmed melee works
- Nanite/Lumen-heavy rendering on limited VRAM

---

**This README is intentionally live and will be refined as project information, plans, and technical details settle.**
