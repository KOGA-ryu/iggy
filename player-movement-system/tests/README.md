# Movement Tests

Good first tests:

- click on walkable tile creates `WalkTo`
- click while inventory owns focus creates no command
- click while text entry is active creates no command
- `WalkTo` on blocked tile is rejected
- path is consumed one step at a time
- `Stop` clears the current path
- stun state rejects movement
- animation lock rejects movement
- animation cancel window allows movement again
- stand-ground rejects `WalkTo` but keeps player facing/intent available
- diagonal movement through a blocked corner is rejected
- diagonal path cost discourages unnecessary zig-zagging
- `future` updates when a step is committed
- empty tile target creates `WalkTo`
- enemy target creates `MoveThenAct(Attack)`
- item target creates `MoveThenAct(Pickup)`
- NPC target creates `MoveThenAct(Talk)`
- object target creates `MoveThenAct(Interact)`
- stand-ground plus attack target creates `StandAndAct(Attack)`
- destination action survives until path is consumed
- action executor rejects invalid targets
- action executor waits when target is out of range
- action executor applies attack animation commitment
- action executor clears destination action after execution
- MoveThenAct emits CommandAccepted -> PathStarted -> StepCommitted -> DestinationActionReady -> AnimationLocked -> ActionExecuted
- CommandLog replay produces the same movement/action event sequence
- MovementCodec round-trips MoveThenAct command packets
- enemy pursuit obeys maxStepsPerTick
- enemy in range enters attack windup
- enemy attack windup transitions into recovery
- CombatResolver applies deterministic damage and defeat
- ActionExecutor attack can damage registered combat target
- CombatSystem emits Hit and Defeated events with damage and remaining HP
- Enemy attack windup resolves combat against player and emits CombatEvent
- SimulationTick drains queued commands, updates movement, and resolves combat

Run them with:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
