# Iggy Departments

This directory is the durable map for Iggy's local department workflow. The
hub owns the total roadmap; departments own slices of that roadmap in isolated
worktrees.

For the operating model, see
`engine/research/local_mac_department_orchestration.md`.

## Departments

| Department | Owns | First questions before work |
| --- | --- | --- |
| Runtime | runtime frames, sessions, save/load, reports, command flow | What is the source of truth? What is compatibility-bearing? |
| AI/NPC | AI maps, profiles, actor/NPC movement, navigation, legacy NPC migration | What owns decisions? What is derived? What is compute-sensitive? |
| Authoring | TOML/package/preview/facade/content fixture lane | What is declarative content vs runtime behavior? |
| UI/Product | shell, preview/editor surfaces, product workflow | How does a user see, inspect, and act on the capability? |
| Platform/Integration | tmux control room, scripts, CMake, CI lanes, merge gates | How do we make departments faster and merges safer? |

Each department has these roles:

- **planner**: owns the department bucket, refines slices, dispatches work;
- **builder**: implements scoped feature or fixture work;
- **researcher**: scouts semantics, prior art, repo surfaces, compute costs;
- **reviewer**: adversarial gate for ownership, risk, and fit;
- **finisher**: cleanup, docs, test-support extraction, merge polish;
- **apprentice/Spark**: short-lived scouts or tiny bounded implementation tasks.

## Hub Rules

- The head planner assigns department objectives, not tiny implementation
  details.
- Department planners use their researcher and reviewer before dispatch when
  ownership, semantics, or cost is unclear.
- Builders and finishers commit on their department branches.
- Integration happens only through the hub after verification and review.
- UI/Product receives early surface-design work from every department before
  code needs UI exposure.

## Current Buckets

- Runtime: `runtime/README.md`
- AI/NPC: `ai_npc/README.md`
- Authoring: `authoring/README.md`
- UI/Product: `ui_product/README.md`
- Platform/Integration: `platform_integration/README.md`

Use these as department charters and bucket heads. Add packet subdirectories
only when a department needs a multi-slice queue.
