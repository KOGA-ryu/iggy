# ASCII Source-Plan TOML Fixtures

These files are dev/test fixtures for the ASCII source-plan authoring lane. They are examples that feed the TOML reader, authoring adapter, profile scenario runner, and final debug-row projection. They are not runtime truth, a save format, or an Edi/UI content format.

Run a self-contained fixture from a configured build with:

```sh
engine/build/iggy_scenario_toml_runner engine/tests/fixtures/runtime/ascii_source_plan/self_contained_guard_room.toml
```

Add `--trace` before the path to include per-frame rows and counts.

Canonical success fixtures are stable examples and are covered by CLI golden rows:

- `moving_guard_room.toml`: one guard actor moves from an authored `[[frame_controls]]` entry.
- `multi_frame_guard_room.toml`: one guard actor moves across multiple authored frame ids.
- `player_and_guard_room.toml`: player `move_to_tile` and NPC `frame_controls` share a frame id.
- `player_interacts_guard_room.toml`: player `interact` toggles an authored interaction target.
- `player_picks_up_item_room.toml`: player moves near an authored item drop and picks it up.
- `mixed_mini_scenario.toml`: small combined scenario with NPC movement, player movement, pickup, and interaction.

Regression-only fixtures are kept for lower-level parser, adapter, and diagnostic tests:

- `valid_guard_room.toml`: parse-valid but intentionally not self-contained; conversion should report a missing profile trait unless C++ config supplies it.
- `self_contained_guard_room.toml`: focused empty-config adapter acceptance fixture; the canonical `player_and_guard_room.toml` covers the same visible final behavior.
- `semantic_invalid_guard_room.toml`: source-plan validation failure fixture.
- `corrupt_guard_room.toml`: TOML syntax failure fixture.

Canonical fixture contract:

- Canonical success fixtures must be self-contained: every actor `profile_id` must have a matching `[[profiles]]` entry in the same TOML file.
- Canonical success fixtures must run through `iggy_scenario_toml_runner <path>` with no C++ default frame, profile catalog, terrain policy, or other hidden converter config.
- Canonical success fixtures must have stable CLI expectations in `iggy_scenario_toml_runner_tests.cpp`, including summary counts and final ASCII rows.
- Regression-only fixtures may omit facts or contain invalid TOML/source-plan data when the omission or failure is the behavior under test.

Supported tables by example:

```toml
format_id = "iggy:ascii-source-plan"
version = 1
source_id = "scenario:example"

[grid]
width = 7
height = 4
background = "."
rows = [
  "#######",
  "#A@.k.#",
  "#.....#",
  "#######",
]

[[legend]]
glyph = "A"
kind = "actor"
role_id = "role:npc"
maps_to_scenario_marker = true
scenario_marker_kind = "actor"

[[legend]]
glyph = "@"
kind = "player_start"
role_id = "role:player-start"
maps_to_scenario_marker = true
scenario_marker_kind = "player_start"

[[profiles]]
id = "profile:guard"
strength = 10
dexterity = 10
constitution = 10
intelligence = 10
wisdom = 10
charisma = 10

[[cells]]
id = "cell:guard"
row = 1
column = 1
glyph = "A"
local_tile = { x = 1, y = 1 }
local_position = { x = 1.5, y = 1.5 }
cell_bounds = { min_x = 1.0, min_y = 1.0, max_x = 2.0, max_y = 2.0 }
marker_id = "npc:guard"
profile_id = "profile:guard"

[[frame_controls]]
frame_id = "frame:shared"
npc = "npc:guard"
behavior = "seeking"
move_mode = "walk"
target = { x = 2.5, y = 1.5 }

[[frame_player_commands]]
frame_id = "frame:shared"
command = "move_to_tile"
x = 3
y = 1

[[interaction_targets]]
target_id = "target:lever"
kind = "usable"
tile = { x = 3, y = 1 }
position = { x = 3.5, y = 1.5 }
radius = 1.0
enabled = true
effect = "toggle_target"
effect_target_id = "target:lever"
enabled_value = false

[[item_drops]]
drop_id = "drop:key"
item_id = "item:key"
count = 1
tile = { x = 4, y = 1 }
position = { x = 4.5, y = 1.5 }
pickup_radius = 1.0
enabled = true
glyph = "k"

[expect]
final_rows = [
  "#######",
  "#.A@..#",
  "#.....#",
  "#######",
]
frame_count = 1
accepted_command_count = 1
picked_up_count = 0
interaction_changed = false
npc_moved_count = 1
```

Non-goals for these fixtures: scripting, UI/Edi behavior, save/load behavior, directory scanning, and a full TOML implementation.
