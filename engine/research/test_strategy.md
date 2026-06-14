# Test Strategy

Purpose: describe what each test layer should prove and when a slice has enough coverage to move up the ownership stack.

## General Rules

- Test pure transforms before integration steps.
- Test source-of-truth delegation instead of duplicating behavior in a higher layer.
- Every state packet or cache update should have an input-not-mutated test.
- Boundary decisions should be encoded as behavior tests, not comments only.
- Add shared test fixtures only when they remove mechanical repetition without hiding scenario intent.

## Layer Expectations

### Core/math

Proves:
- raw value semantics
- edge behavior such as inclusive AABB checks
- no hidden normalization unless explicitly named

Avoid:
- importing scene/runtime policy into math tests

### Resource/catalog

Proves:
- deterministic validation
- duplicate/empty id policy
- order preservation
- exact string/id preservation

Avoid:
- file IO
- backend handle assumptions

### Server/query

Proves:
- world/order scanning behavior
- defensive invalid-input behavior
- no mutation of query inputs
- edge rules such as touching/overlap

Avoid:
- scene/player/runtime ownership
- gameplay filtering

### Scene transforms

Proves:
- authoritative state interpretation
- derived cache construction/update from source state
- changed-item reporting
- exact delegation to server primitives
- interaction target/effect catalogs validate deterministically
- interaction query/reach/plan/effect-plan helpers remain pure and report diagnostics
- interaction effect appliers return updated registries, record events, and count applied/deferred/failed effects without mutating inputs
- inventory stacks and item-drop registries validate deterministically
- item definition catalogs validate deterministic id/name/stack-limit policy
- inventory stack policy proves catalog lookup, stack caps, duplicate ids by catalog policy, and no mutation
- pickup plans preserve requested ids/positions, enforce enabled/range policy, and do not mutate inventories or drops
- pickup transfers add inventory stacks and consume drops through source-of-truth helpers without mutating inputs
- policy pickup transfers preserve stack-policy diagnostics, add/drop diagnostics, and inventory event ordering
- NPC AI tactical maps, behavior pools, trait sets, trait pools/draws, hand/read/map-read/map-play/tell/fold reports, Play-control proposal/apply/report boundaries, actor registries, control registries, decision reports, route/path reports, movement proposals, and command-frame mappers validate inputs, preserve order, and avoid mutation
- UI models project runtime reports/settings/tool state into read-only model/action data

Avoid:
- runtime tick order
- backend or save semantics
- applying interaction effects outside interaction target registry state without an explicit ownership slice
- placing inventory/drop state in `RuntimeSessionState` or save snapshots without an explicit ownership slice
- UI tests that require backend rendering, platform windows, or device input

### Player scene behavior

Proves:
- command interpretation against `PlayerAgentState`
- input intent validation, gating, and command-frame mapping
- movement intent/application rules
- physics delegation through `CharacterMove2D`
- no mutation of inputs

Avoid:
- raw input binding
- command queues
- runtime session orchestration

### Runtime orchestration

Proves:
- call order between existing lower-level steps
- state carry-forward
- diagnostics preservation
- queue push/drain and input-gate diagnostics are preserved without reinterpreting them
- interaction and effect-plan diagnostics are preserved without applying effects
- interaction effect-apply orchestration delegates to scene/interaction appliers and does not persist target registries implicitly
- frame-level effect application applies interactions sequentially and keeps `RuntimeInteractionState` explicit
- pickup orchestration preserves explicit `RuntimeInventoryState` and delegates transfer semantics to scene/inventory
- policy pickup orchestration preserves item-definition/stack-policy diagnostics and inventory events without reimplementing scene/inventory rules
- gameplay frame orchestration carries session, command queue, interaction state, inventory state, reports, and inventory events forward without changing lower-level semantics
- policy gameplay frame orchestration proves item-definition catalog input, policy pickup diagnostics, report counts, runner carry-forward, and event accumulation without changing the simple gameplay frame lane
- NPC AI queue orchestration preserves scene/ai decision diagnostics, queue order, and command-runner behavior without reimplementing tactical scoring or path/movement proposal logic
- tick index changes only where the tick step owns them
- derived caches copied/preserved, not rebuilt implicitly

Avoid:
- duplicating scene/server logic
- duplicating player input gate or mapper logic
- duplicating scene/interaction target, reach, or effect-plan rules
- duplicating scene/ai trait draw, hand/read/map-read/map-play/tell/fold, Play-control proposal/apply/report, scoring, routing, path-report, or movement-proposal rules
- hidden cache rebuilds
- raw input mapping

## Promotion Rule

Do not add runtime integration until lower layers have tests:

```text
server primitive tests
  -> scene transform tests
  -> runtime orchestration tests
```

Example:

```text
CollisionWorld2D / CharacterMove2D tests
  -> PlayerMovementExecutor2D tests
  -> RuntimePlayerCommandExecutionStep tests
```

## Required Test Patterns

Use these when relevant:

- empty input succeeds or fails according to explicit policy
- one-item behavior
- multiple-item order preservation
- duplicate handling
- invalid input diagnostics
- copy/no mutation of input structs
- source-compatible default behavior
- manual composition across boundaries

## Current Shared Test Support

- `engine/tests/support/GeometryAssertions.hpp`
- `engine/tests/support/PlayerFixtures.hpp`
- `engine/tests/support/CommandFrameFixtures.hpp`

Add new support only for repeated mechanics. Keep domain-specific expected values visible in tests.
