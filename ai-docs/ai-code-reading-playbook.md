# AI Code Reading Playbook

This is a practical method for reading unfamiliar game AI code with grep. It is
about building a mental ownership map, not understanding every line.

## The Core Question

For any AI system, keep asking:

```text
Who owns the truth?
Who only calculates a temporary answer?
Who executes the result?
Who saves it?
Who rebuilds it?
```

If a file only computes candidates, scores options, or fills a command/action
record, it is usually not the owner of truth.

## The Five-Pass Reading Loop

### 1. Find Vocabulary

Start with enums, structs, and classes:

```bash
rg -n "enum .*State|enum .*Mode|enum .*Order|enum .*Objective|struct .*AI|class .*AI" /path/to/repo/src
```

Why: vocabulary shows how the game names its concepts. Before reading behavior,
learn the nouns.

Look for:

- states
- modes
- objectives
- orders
- actions
- jobs
- flags
- profile/type structs

### 2. Find Ownership Containers

Search likely owner nouns:

```bash
rg -n "struct .*Monster|class .*Monster|struct .*Ped|class .*Ped|struct .*Droid|class .*Droid|struct .*Creature|class .*Creature" /path/to/repo
```

Why: the owner container usually holds live actor truth. Header files reveal the
shape faster than implementation files.

Look for fields that smell authoritative:

- id
- position
- health
- faction/owner
- current state/mode
- current objective/order
- target id or target pointer
- inventory
- status flags
- save/load hooks

### 3. Find Update Flow

Search verbs:

```bash
rg -n "Process.*|Update.*|think\(|Tick|turn|move\(|Move.*|Do.*AI|Run.*AI" /path/to/repo/src
```

Why: update verbs show where state changes happen. This reveals the tick/turn
pipeline.

Look for:

- one central loop
- per-actor update
- action update
- movement update
- animation update
- save/load side effects

### 4. Find Map and Query Boundaries

Search environment words:

```bash
rg -n "Path|Line|LOS|visible|solid|block|collision|occup|tile|room|job|attractor|route|node" /path/to/repo/src
```

Why: AI behavior usually depends on world queries. These terms reveal whether
the actor owns knowledge or asks the map/session.

Look for:

- occupancy grids
- path graphs
- room/work-site stores
- visibility arrays
- collision flags
- candidate lists
- route nodes
- attractor queues

### 5. Find Persistence Boundaries

Search save/load terms:

```bash
rg -n "Save|Load|Restore|Serialize|Deserialize|save_.*|load_.*|chunk|snapshot" /path/to/repo/src
```

Why: save/load tells you what the game treats as real state. If something is not
saved, it is probably runtime scratch, a cache, or a rebuildable query product.

Look for:

- actor records
- inventory/items
- current objective/order
- mission/session flags
- mutable graph flags
- post-load repair
- pointer/id fixup
- rebuilt grids and caches

## How To Turn Hits Into Conclusions

Use this template for each finding:

```text
Path:
Found:
Looks authoritative because:
Looks derived because:
Owner:
Reads from:
Writes to:
Saved:
Rebuilt:
```

Example conclusion shape:

```text
DROID owns current order/action/move state because those fields live on the live
unit record and are updated in the per-unit tick. Map visibility and spatial
grids are not owned by DROID because they are rebuilt by runtime map/grid code.
```

## Noise Reduction Recipes

### If The Search Is Too Broad

Search headers first:

```bash
rg -n "struct Thing|struct CreatureControl" /path/to/repo/src/*.h
```

Or search only likely owner folders:

```bash
rg -n "struct Thing|struct CreatureControl" /path/to/repo/src/creature* /path/to/repo/src/thing*
```

### If A Common Word Matches Too Much

Add context words:

```bash
rg -n "Order|Action" /path/to/repo/src
rg -n "DroidOrder|DORDER_|DACTION_" /path/to/repo/src
```

The second search is better because it uses local naming conventions.

### If A Type Appears Everywhere

Find its definition first:

```bash
rg -n "struct monst\b|class CPed\b|struct DROID\b" /path/to/repo
```

Then inspect the header and only afterward search use sites.

### If You Need Function Definitions, Not Calls

Try adding return/type context:

```bash
rg -n "void .*Process|bool .*Move|int .*Save" /path/to/repo/src
```

Or search the exact function name and open the best owner file:

```bash
rg -n "ProcessObjective" /path/to/repo/src/peds
```

## Syntax Patterns Worth Memorizing

| Syntax | Meaning | Use |
|---|---|---|
| `rg -n` | search with line numbers | create source anchors |
| `"a|b|c"` | regex OR | search related clues together |
| `.*` | anything between words | find name families |
| `\(` | literal open parenthesis | find function calls/defs |
| `\b` | word boundary | avoid partial matches |
| `--files` | list files only | discover likely filenames |
| `cmd1 | cmd2` | shell pipe | filter one command's output with another |
| `\` at line end | continue shell command | readable long commands |
| `nl -ba` | numbered file output | stable line inspection |
| `sed -n 'x,yp'` | print line range | read around an anchor |

## Question-To-Command Recipes

### "Where Is Static Definition Data?"

```bash
rg -n "struct .*Data|struct .*Stats|class .*Stats|Profile|Template|Config|Definition" /path/to/repo
```

Static data usually appears in headers, tables, config loaders, or mod/data
folders.

### "Where Is Live Actor State?"

```bash
rg -n "struct .*Actor|class .*Actor|struct .*Monster|class .*Monster|struct .*Ped|class .*Ped|struct .*Unit|class .*Unit" /path/to/repo/src
```

Live state usually has id, position, owner/faction, status, current state, and
target fields.

### "Where Does AI Decide?"

```bash
rg -n "think\(|Process.*AI|Update.*AI|ProcessObjective|AiProc|actionUpdate|orderUpdate|dog_move|m_move\(" /path/to/repo/src
```

Decision code usually reads actor + map state and chooses action/order/mode.

### "Where Does Movement Legality Live?"

```bash
rg -n "Path|CanMove|Walk|solid|blocked|collision|occup|LineClear|LOS|mfndpos" /path/to/repo/src
```

Movement legality usually belongs to map/path/collision code, not the actor
definition.

### "What Is Saved Versus Rebuilt?"

```bash
rg -n "Save|Load|Restore|Fix|Rebuild|Init.*Level|PostLoad|chunk" /path/to/repo/src
```

Saved state is usually authoritative. Rebuilt state is usually cache, pointer
repair, occupancy, visibility, pathing, rendering, or resource binding.

## Red Flags While Reading

These are not automatically bad, but they mean "slow down and identify owner":

- one giant actor struct with many unrelated fields
- functions that both decide and execute
- save/load functions that serialize runtime scratch
- global arrays read by many AI functions
- behavior switches that mutate several systems at once
- pointers stored across save/load boundaries
- pathfinding functions that also choose behavior policy
- UI/debug panels that appear to own model truth

## Good Stopping Point

Stop reading once you can fill this:

```text
Static definition:
Live actor:
Objective/order:
Executor state:
Movement/path state:
Map/query owner:
Interaction/work-site owner:
Director/global AI:
Saved:
Rebuilt:
Main update function:
```

That is enough for an ownership map. Deeper implementation reading can wait
until a specific behavior needs to be understood.

