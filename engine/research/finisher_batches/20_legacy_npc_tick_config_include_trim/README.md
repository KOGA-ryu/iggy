# 20 Legacy NPC Tick Config Include Trim

Status: complete.

Goal: reduce accidental coupling to `NpcAgentController.hpp` for headers that
only store or pass `npc_ai::NpcAgentTickConfig`.

Scope:
- Move only `npc_ai::NpcAgentTickConfig` into a narrow
  `NpcAgentTickConfig.hpp` header.
- Keep `NpcAgentTickResult` and `NpcAgentController` in
  `NpcAgentController.hpp`.
- Replace controller includes in runtime and scene config pass-through headers
  with the narrow tick config header.
- Add direct full controller includes only where implementation code constructs
  or consumes controller/result types.

Implementation:
- Added `modules/npc_ai/NpcAgentTickConfig.hpp`.
- Updated `NpcAgentController.hpp` to include the new config header.
- Narrowed config-only runtime, scene, and NPC batch updater headers.
- Kept actual controller/result users on `NpcAgentController.hpp`.

Verification:
- Focused build and CTest for controller, batch updater, and runtime tick
  surfaces.
- Remaining `NpcAgentController.hpp` include grep reviewed.
- Full CTest because exported runtime/module headers changed.
- `git diff --check`
