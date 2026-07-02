# Stealth garden — headless testbed (slice s6b)

A purpose-built, text-editable headless fixture that exercises the whole stealth loop in one
place: **patrol + vision cone + LOS occlusion + graded alert**. A guard patrols a walled
garden with a central wall island that breaks sight; scripted player tapes drive scenarios;
assertions read the guard's alert ladder from the NPC behavior debug snapshot.

Everything here is **data**. Adding a scenario or changing the guard's beat is a text edit —
no engine or harness code change.

## Files
- `stealth_garden.iggyroom.txt` — the ascii grid (geometry). `#` = wall, `.` = floor,
  `N` = guard spawn, `P` = player spawn, `E` = exit. The central 6×2 island sits between the
  top and bottom corridors so it **breaks line of sight** across the middle.
- `stealth_garden.scenario.iggy3d.toml` — spawns, the guard patrol beat, and the exit
  objective (content lane, parsed by `parseScenarioText`). Entity positions must match the
  grid glyph cells: cell `(col,row)` → world `(col, 0, row)`, cell size 1 m.
- `../../../tests/unit/stealth_garden_tests.cpp` — the headless harness + scenarios.

## How to change the geometry
Edit `stealth_garden.iggyroom.txt` and move the matching entity `position` in the `.toml`
(the harness asserts the guard/player/exit spawns still sit on the `N`/`P`/`E` cells).

## How to change the guard's beat
Edit the `waypoint = [x,y,z]` lines under `[[ai_actors]]` in the `.toml` (ordered route,
`patrol_mode = "loop"`). Facing follows travel automatically.

> Note: the shipped beat is a straight north-south leg on the far-west column (`x=1`), kept
> **axis-aligned on purpose**. A rectangular (cornered) loop currently trips a latent patrol
> bug where the guard stalls at a waypoint (the patrol arrival epsilon equals the point-move
> stop distance, so a diagonal approach parks the guard a floating-point hair outside the
> arrival ring and it never advances). Until that s6 patrol wiring is fixed, keep patrol legs
> axis-aligned. See the s6b slice REPORT for the flag + suggested fix.

## How to add a scenario
Add a function to `stealth_garden_tests.cpp` that:
1. `makeGardenSession()` (parses the scenario + bakes the grid collision, asserts spawns),
2. drives the player with `walkPlayerTo(...)` / `submitWait(...)` / `submitInteract(...)`,
3. samples the guard each tick with `guardBehaviorViaSnapshot(...)` / `guardAlertLevel(...)`,
4. asserts the guard's alert rung (and `SessionOutcome` where relevant),
5. register it in `main()`.

Player moves are clamped to the config move distance per tick — use `walkPlayerTo` (it steps
in ≤ move-distance hops) rather than a single far Move (which admission rejects).

## Starter scenarios
- **sneak-unseen** — the player slips up the far-east column to the exit while the guard
  patrols the far-west leg, staying outside the guard's 6 m perception radius the whole way;
  the guard never crosses the Suspicious rung and the run ends in Victory.
- **spotted** — the player lingers in front of the guard's patrol leg; the guard glimpses it
  on each sweep, alert accumulates (dead-time bridges the away sweeps), and once it crosses
  Suspicious the escalation FSM overrides patrol and the guard locks on and reaches Chasing.
- **island occlusion** — a focused check that with the guard and player on opposite sides of
  the island (same column, both in the cone and in radius) the guard's line of sight is
  **blocked**, so it does not perceive. This is the blind side the testbed is built around.
