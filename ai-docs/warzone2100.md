# Warzone 2100 AI Study

Reference repo: `/Users/kogaryu/iggy/warzone2100-master`

Warzone2100 is useful because it separates a unit's high-level order from its
current action and from its movement/path state. That distinction is more
important than the specific RTS behaviors: "what the actor wants," "what it is
doing now," and "how it is physically getting there" are separate concepts.

## Core Ownership Map

### Static Definition Data

Static unit/build data is split between component stats and droid templates.

Important anchors:

- `/Users/kogaryu/iggy/warzone2100-master/src/statsdef.h:37`
- `/Users/kogaryu/iggy/warzone2100-master/src/statsdef.h:303`
- `/Users/kogaryu/iggy/warzone2100-master/src/statsdef.h:325`
- `/Users/kogaryu/iggy/warzone2100-master/src/statsdef.h:505`
- `/Users/kogaryu/iggy/warzone2100-master/src/statsdef.h:525`
- `/Users/kogaryu/iggy/warzone2100-master/src/droiddef.h:56`
- `/Users/kogaryu/iggy/warzone2100-master/src/template.cpp:44`

Static data owns:

- droid type: weapon, sensor, construct, command, repair, transporter, cyborg
- component stats: body, propulsion, sensor, repair, construct, weapons
- propulsion travel type and movement sounds
- body size, weapon slots, armor, resistance, body class
- droid template component ids and weapon ids
- per-player template lists

Learning point: unit identity is assembled from reusable components. The live
unit does not need to own all design data as unique logic.

### Live Actor State

The live unit is `DROID`.

Important anchors:

- `/Users/kogaryu/iggy/warzone2100-master/src/droiddef.h:101`
- `/Users/kogaryu/iggy/warzone2100-master/src/droiddef.h:139`
- `/Users/kogaryu/iggy/warzone2100-master/src/droiddef.h:147`
- `/Users/kogaryu/iggy/warzone2100-master/src/droiddef.h:151`
- `/Users/kogaryu/iggy/warzone2100-master/src/droiddef.h:161`
- `/Users/kogaryu/iggy/warzone2100-master/src/droiddef.h:164`
- `/Users/kogaryu/iggy/warzone2100-master/src/droiddef.h:181`
- `/Users/kogaryu/iggy/warzone2100-master/src/droiddef.h:186`
- `/Users/kogaryu/iggy/warzone2100-master/src/droiddef.h:197`

`DROID` owns:

- live name, type, copied component bits
- derived weight, base speed, original body
- experience, kills, shields, resistance
- group and base/rearm association
- queued order list
- current primary order
- secondary order flags
- current action state
- current movement control state

Learning point: Warzone stores a lot on the live actor, but the useful boundary
is clear: actor state owns current intent/action/movement, while definitions and
map services live elsewhere.

## The Three-Layer Unit Brain

Warzone's best lesson is the separation between orders, actions, and movement.

### Order: Durable Intent

Orders are high-level gameplay intent.

Important anchors:

- `/Users/kogaryu/iggy/warzone2100-master/src/orderdef.h:41`
- `/Users/kogaryu/iggy/warzone2100-master/src/orderdef.h:44`
- `/Users/kogaryu/iggy/warzone2100-master/src/orderdef.h:49`
- `/Users/kogaryu/iggy/warzone2100-master/src/orderdef.h:50`
- `/Users/kogaryu/iggy/warzone2100-master/src/orderdef.h:56`
- `/Users/kogaryu/iggy/warzone2100-master/src/orderdef.h:72`
- `/Users/kogaryu/iggy/warzone2100-master/src/orderdef.h:75`
- `/Users/kogaryu/iggy/warzone2100-master/src/orderdef.h:78`
- `/Users/kogaryu/iggy/warzone2100-master/src/orderdef.h:83`
- `/Users/kogaryu/iggy/warzone2100-master/src/orderdef.h:171`

Orders include:

- move
- attack
- build
- repair
- observe
- fire support
- return to base/repair
- guard
- scout
- patrol
- recover item/artifact
- circle
- hold

`DroidOrder` carries:

- order type
- primary position
- secondary position
- direction
- target object
- target structure stats
- repair/return data

Learning point: durable intent needs a typed payload. It should be saveable and
understandable without replaying the AI decision that created it.

### Secondary Orders: Standing Policy

Secondary orders are lightweight policy flags layered on top of the current
order.

Important anchors:

- `/Users/kogaryu/iggy/warzone2100-master/src/orderdef.h:88`
- `/Users/kogaryu/iggy/warzone2100-master/src/orderdef.h:91`
- `/Users/kogaryu/iggy/warzone2100-master/src/orderdef.h:92`
- `/Users/kogaryu/iggy/warzone2100-master/src/orderdef.h:93`
- `/Users/kogaryu/iggy/warzone2100-master/src/orderdef.h:94`
- `/Users/kogaryu/iggy/warzone2100-master/src/orderdef.h:100`
- `/Users/kogaryu/iggy/warzone2100-master/src/orderdef.h:108`

Secondary policies include:

- attack range
- repair fallback threshold
- attack level
- halt behavior: hold, guard, pursue
- patrol/circle toggles
- return-to-location flavor
- fire designator

Learning point: not every behavior switch should become a new order. Some are
standing policy that changes how orders are interpreted.

### Action: Current Executor State

Actions are what a droid is doing right now.

Important anchors:

- `/Users/kogaryu/iggy/warzone2100-master/src/actiondef.h:27`
- `/Users/kogaryu/iggy/warzone2100-master/src/actiondef.h:31`
- `/Users/kogaryu/iggy/warzone2100-master/src/actiondef.h:33`
- `/Users/kogaryu/iggy/warzone2100-master/src/actiondef.h:34`
- `/Users/kogaryu/iggy/warzone2100-master/src/actiondef.h:39`
- `/Users/kogaryu/iggy/warzone2100-master/src/actiondef.h:51`
- `/Users/kogaryu/iggy/warzone2100-master/src/actiondef.h:59`
- `/Users/kogaryu/iggy/warzone2100-master/src/actiondef.h:61`
- `/Users/kogaryu/iggy/warzone2100-master/src/actiondef.h:68`

Actions include:

- move
- attack
- observe
- build/repair/demolish
- move-to-attack
- rotate-to-attack
- move-to-observe
- move-to-repair
- wait-for-repair/rearm
- VTOL attack/rearm states
- return to position

Learning point: current action can differ from the durable order. A guard order
may temporarily become an attack action; a repair order may become a move-to-
repair action before the repair action starts.

### Movement: Path/Execution Cache

Movement is a lower-level control state, not the same as action or order.

Important anchors:

- `/Users/kogaryu/iggy/warzone2100-master/src/movedef.h:45`
- `/Users/kogaryu/iggy/warzone2100-master/src/movedef.h:47`
- `/Users/kogaryu/iggy/warzone2100-master/src/movedef.h:48`
- `/Users/kogaryu/iggy/warzone2100-master/src/movedef.h:49`
- `/Users/kogaryu/iggy/warzone2100-master/src/movedef.h:51`
- `/Users/kogaryu/iggy/warzone2100-master/src/movedef.h:56`
- `/Users/kogaryu/iggy/warzone2100-master/src/movedef.h:61`
- `/Users/kogaryu/iggy/warzone2100-master/src/movedef.h:65`

`MOVE_CONTROL` owns:

- movement status
- path index
- path nodes
- destination/source/target
- speed
- movement direction
- bump/stuck/shuffle data
- formation pointer
- VTOL vertical speed

Learning point: movement state is derived execution state. It should be close to
the runtime mover/path follower, not mixed into behavior definitions.

## Per-Unit Update Flow

Warzone updates each droid in a clear sequence.

Important anchors:

- `/Users/kogaryu/iggy/warzone2100-master/src/droid.cpp:935`
- `/Users/kogaryu/iggy/warzone2100-master/src/droid.cpp:939`
- `/Users/kogaryu/iggy/warzone2100-master/src/droid.cpp:945`
- `/Users/kogaryu/iggy/warzone2100-master/src/droid.cpp:950`
- `/Users/kogaryu/iggy/warzone2100-master/src/order.cpp:415`
- `/Users/kogaryu/iggy/warzone2100-master/src/action.cpp:680`
- `/Users/kogaryu/iggy/warzone2100-master/src/move.cpp:2165`

Flow:

1. AI update may choose opportunistic targets or adjust behavior.
2. Order update interprets durable order and queued orders.
3. Action update executes/advances current action.
4. Movement update advances physical/path state.

Learning point: this is a practical pipeline:

`AI decision -> objective/order update -> action proposal/update -> movement execution`

## Micro AI Targeting

Warzone's `ai.cpp` is mostly combat micro AI, not a complete strategic planner.

Important anchors:

- `/Users/kogaryu/iggy/warzone2100-master/src/ai.h:65`
- `/Users/kogaryu/iggy/warzone2100-master/src/ai.h:69`
- `/Users/kogaryu/iggy/warzone2100-master/src/ai.cpp:46`
- `/Users/kogaryu/iggy/warzone2100-master/src/ai.cpp:55`
- `/Users/kogaryu/iggy/warzone2100-master/src/ai.cpp:573`
- `/Users/kogaryu/iggy/warzone2100-master/src/ai.cpp:1135`
- `/Users/kogaryu/iggy/warzone2100-master/src/ai.cpp:1154`
- `/Users/kogaryu/iggy/warzone2100-master/src/ai.cpp:1194`
- `/Users/kogaryu/iggy/warzone2100-master/src/ai.cpp:1223`
- `/Users/kogaryu/iggy/warzone2100-master/src/ai.cpp:1241`

The micro AI:

- skips units that cannot attack or sense
- checks current order and action before choosing targets
- avoids overriding queued orders
- respects secondary attack policy
- chooses visible/sensor targets
- may set a new attack/observe action

Learning point: AI is allowed to opportunistically propose actions, but only
inside the boundaries set by current order and standing policy.

## Groups and Formations

Groups and formations are separate from individual unit orders.

Important anchors:

- `/Users/kogaryu/iggy/warzone2100-master/src/group.h:33`
- `/Users/kogaryu/iggy/warzone2100-master/src/group.h:40`
- `/Users/kogaryu/iggy/warzone2100-master/src/group.h:45`
- `/Users/kogaryu/iggy/warzone2100-master/src/group.h:49`
- `/Users/kogaryu/iggy/warzone2100-master/src/group.h:55`
- `/Users/kogaryu/iggy/warzone2100-master/src/group.h:57`
- `/Users/kogaryu/iggy/warzone2100-master/src/group.h:58`
- `/Users/kogaryu/iggy/warzone2100-master/src/formationdef.h:53`
- `/Users/kogaryu/iggy/warzone2100-master/src/formationdef.h:56`
- `/Users/kogaryu/iggy/warzone2100-master/src/formationdef.h:68`

Group owns:

- group type: normal, command, transporter
- member list
- commander pointer
- group id
- helpers to apply orders to all members

Formation owns:

- formation position and direction
- member slots
- line/rank offsets
- slowest-member speed
- player id

Learning point: group intent and formation layout are separate systems in this
reference. Squad behavior is not modeled only as fields on every individual
unit.

## Map Query Services

Warzone uses map-owned and runtime-owned query structures heavily.

Important anchors:

- `/Users/kogaryu/iggy/warzone2100-master/src/map.h:45`
- `/Users/kogaryu/iggy/warzone2100-master/src/map.h:69`
- `/Users/kogaryu/iggy/warzone2100-master/src/map.h:74`
- `/Users/kogaryu/iggy/warzone2100-master/src/map.h:89`
- `/Users/kogaryu/iggy/warzone2100-master/src/map.h:96`
- `/Users/kogaryu/iggy/warzone2100-master/src/gateway.h:40`
- `/Users/kogaryu/iggy/warzone2100-master/src/mapgrid.cpp:36`
- `/Users/kogaryu/iggy/warzone2100-master/src/mapgrid.cpp:53`
- `/Users/kogaryu/iggy/warzone2100-master/src/visibility.h:37`
- `/Users/kogaryu/iggy/warzone2100-master/src/visibility.h:62`

Map/query data includes:

- tile blocking by movement/travel type
- temporary pathfinding block flags
- gateway routing data
- danger/threat aux maps
- spatial grid for nearby object queries
- visibility updates

Learning point: map intelligence belongs beside the map/runtime session. Units
query it rather than owning those shared query structures.

## Main Loop Placement

Global update order rebuilds query caches before visibility/map updates.

Important anchors:

- `/Users/kogaryu/iggy/warzone2100-master/src/loop.cpp:531`
- `/Users/kogaryu/iggy/warzone2100-master/src/loop.cpp:545`
- `/Users/kogaryu/iggy/warzone2100-master/src/loop.cpp:548`
- `/Users/kogaryu/iggy/warzone2100-master/src/loop.cpp:551`
- `/Users/kogaryu/iggy/warzone2100-master/src/loop.cpp:554`

Flow:

1. Send pending droid orders.
2. Update visibility level state.
3. Rebuild spatial grid from live objects.
4. Process object visibility.
5. Update map state.

Learning point: shared query caches are rebuilt at known runtime points rather
than being silently owned by arbitrary unit behavior.

## Save/Load Shape

Warzone saves a large amount of runtime detail, and the classification is
valuable for understanding what the game treats as durable state.

Important anchors:

- `/Users/kogaryu/iggy/warzone2100-master/src/game.cpp:5988`
- `/Users/kogaryu/iggy/warzone2100-master/src/game.cpp:6038`
- `/Users/kogaryu/iggy/warzone2100-master/src/game.cpp:6048`
- `/Users/kogaryu/iggy/warzone2100-master/src/game.cpp:6049`
- `/Users/kogaryu/iggy/warzone2100-master/src/game.cpp:6075`
- `/Users/kogaryu/iggy/warzone2100-master/src/game.cpp:6077`
- `/Users/kogaryu/iggy/warzone2100-master/src/game.cpp:6090`
- `/Users/kogaryu/iggy/warzone2100-master/src/game.cpp:6114`
- `/Users/kogaryu/iggy/warzone2100-master/src/game.cpp:5869`
- `/Users/kogaryu/iggy/warzone2100-master/src/game.cpp:5945`

Saved data includes:

- droid base object state
- current order
- order queue
- secondary order flags
- current action and action progress
- droid type and component ids
- experience/kills/shields/resistance
- movement state and path nodes
- formation info

Recomputed or reattached after load:

- pathfinding jobs can be recreated
- formations are found or created from saved formation info
- live object lists are rebuilt
- runtime grids/visibility are rebuilt elsewhere in the loop

Learning point: this reference persists actor state, durable orders, and some
execution state, then rebuilds or reattaches path jobs, formations, object lists,
grids, and visibility.

## Legacy Costs

The reference also shows costs:

- one huge mutable actor struct
- raw pointer-heavy ownership
- mixing group behavior into individual actor state
- many RTS-specific order/action kinds
- opportunistic AI must be gated by explicit queued commands and policy

## Minimal Reference Lesson

The smallest Warzone-style AI loop is:

1. Unit has a durable order.
2. Unit has secondary policy flags.
3. AI may propose opportunistic action only if objective/policy allows it.
4. Objective runner converts objective into current action.
5. Action runner converts current action into movement/combat/interaction work.
6. Movement executor owns path-follow state.
7. Map/runtime owns shared query caches.
8. Save actor truth, orders, and selected execution state; rebuild shared query
   structures as runtime state.

Warzone's main lesson is not RTS unit behavior. The lesson is the clean mental
model: order is intent, action is current execution, movement is the low-level
path/control cache, and map intelligence belongs to the map/runtime layer.
