# re3 Miami AI Study

Reference repo: `/Users/kogaryu/iggy/re3-miami`

re3 Miami is useful because it shows open-world NPC behavior split across
pedestrian objectives, executable ped states, movement intensity, threat/type
data, attractors, vehicle autopilot, and path graphs. The useful lesson is the
layering, not the giant mutable `CPed` object.

## Core Ownership Map

### Static Ped Type and Stats Data

Useful files:

- `/Users/kogaryu/iggy/re3-miami/src/peds/PedType.h:4`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedType.h:33`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedType.h:63`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedType.h:96`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedType.h:155`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedType.cpp:43`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedType.cpp:82`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedType.cpp:186`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedType.cpp:244`

Ped type data owns social categories and threat/avoid masks: player, civilian,
cop, gangs, emergency, criminal, special, gun, cop car, fast car, explosion,
dead peds, and related flags.

Ped stats own personality/tuning values: flee distance, heading turn rate, fear,
temper, lawfulness, attack strength, defense weakness, and behavior flags.

Learning point: faction/social threat profile data is separate from live actor
state. Runtime threat response combines static profile data with observed
threats.

### Live Ped State

Useful files:

- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:190`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:280`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:353`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:504`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:528`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:536`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:553`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:574`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:581`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:639`

`CPed` owns many layers in one object, but the fields reveal useful layers:

- `m_objective` and `m_prevObjective`: durable intention.
- `m_nPedState` and `m_nLastPedState`: current executable/animation-like state.
- `m_nMoveState`: movement intensity: still, walk, jog, run, sprint, thrown.
- path arrays and route fields: current route/path scratch.
- `m_threatEntity`, `m_fearFlags`, `m_eventOrThreat`, `m_threatFlags`: live
  perception/threat memory.
- `m_attractor` and `m_positionInQueue`: link into a map-owned interaction.
- `m_pedStats` and `m_nPedType`: links back to static profile/type data.

Learning point: re3 stores objective, state, move mode, perception memory,
attractor assignment, and path-follow state together on the ped, even though
they are conceptually different layers.

## Objective, State, and Move Mode

Useful files:

- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAI.cpp:46`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAI.cpp:55`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAI.cpp:90`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAI.cpp:122`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAI.cpp:130`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAI.cpp:776`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAI.cpp:812`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAI.cpp:1122`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:717`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:928`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:978`

The cleanest conceptual split:

- objective: what the ped wants to accomplish, such as flee, guard, kill, goto,
  follow, enter vehicle, wait at shelter, or use an attractor.
- ped state: what the ped is currently doing, such as idle, seek, flee, attack,
  chat, follow path, enter car, drive, die.
- move state: how hard the ped is moving: still, walk, jog, run, sprint.

`SetObjective` stores/restores previous objectives and treats vehicle entry,
leaving, and leader changes as temporary objectives. `ProcessObjective` maps the
durable objective into seek/flee/attack/enter-car state changes and move modes.

Learning point: objective, executable state, and movement intensity are distinct
layers. "Flee target running" is objective + move mode + target, not one field.

## Threat and Reaction Flow

Useful files:

- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:397`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:513`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:639`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:843`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:8490`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:8517`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:8538`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:8542`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:8654`

Threat response is profile-shaped. A ped has fear/temper stats and type-level
threat masks. Runtime scanning fills a threat entity/flag. Reaction code chooses
whether to flee, fight, cower, duck, or resume idle behavior.

Learning point: threat reaction combines profile data, observed threat, and live
actor state. It is not only pathfinding or movement.

## Pathing and Wandering

Useful files:

- `/Users/kogaryu/iggy/re3-miami/src/control/PathFind.h:17`
- `/Users/kogaryu/iggy/re3-miami/src/control/PathFind.h:59`
- `/Users/kogaryu/iggy/re3-miami/src/control/PathFind.h:203`
- `/Users/kogaryu/iggy/re3-miami/src/control/PathFind.h:256`
- `/Users/kogaryu/iggy/re3-miami/src/control/PathFind.h:263`
- `/Users/kogaryu/iggy/re3-miami/src/control/PathFind.h:264`
- `/Users/kogaryu/iggy/re3-miami/src/control/PathFind.h:265`
- `/Users/kogaryu/iggy/re3-miami/src/control/PathFind.h:266`
- `/Users/kogaryu/iggy/re3-miami/src/control/PathFind.cpp:1527`
- `/Users/kogaryu/iggy/re3-miami/src/control/PathFind.cpp:1604`
- `/Users/kogaryu/iggy/re3-miami/src/control/PathFind.cpp:1670`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:6208`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:6260`

`CPathFind` owns car and pedestrian path nodes, connections, disabled flags,
between-level flags, spawn rates, search scratch, and node queries. Pedestrians
ask the path system for path nodes, then store a short path-follow buffer in the
ped.

`SetFollowPath` distinguishes direct path-clear movement from path-node
following. It stores the destination, abort radius, target entity, movement
mode, and current node list on the ped.

Learning point: the path graph is owned by `CPathFind`. Peds hold short
path-follow buffers and destinations, not the whole graph or spawn rules.

## Attractors and Interaction Points

Useful files:

- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAttractor.h:28`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAttractor.h:39`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAttractor.h:46`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAttractor.h:78`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAttractor.h:81`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAttractor.h:82`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAttractor.h:84`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAttractor.cpp:644`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAttractor.cpp:665`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAttractor.cpp:678`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAttractor.cpp:721`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedAttractor.cpp:780`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:9355`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.cpp:9361`

Attractors are map/object-owned interaction points: ATM, seat, stop, pizza,
shelter, and ice cream. The manager owns vectors of active attractors and
registers/deregisters peds. Each attractor owns approaching and waiting queues,
max capacity, timing, heading/position constraints, effect position, queue
direction, and use direction.

`SetNewAttraction` maps an accepted attractor to an objective and stores queue
position on the ped. The ped does not own the attractor truth.

Learning point: attractors are world-owned interaction points with capacity,
queue slots, approach/use positions, and accepted actor ids. Peds hold their
assignment and queue position.

## Vehicle AI and Autopilot

Useful files:

- `/Users/kogaryu/iggy/re3-miami/src/control/AutoPilot.h:7`
- `/Users/kogaryu/iggy/re3-miami/src/control/AutoPilot.h:38`
- `/Users/kogaryu/iggy/re3-miami/src/control/AutoPilot.h:53`
- `/Users/kogaryu/iggy/re3-miami/src/control/AutoPilot.h:62`
- `/Users/kogaryu/iggy/re3-miami/src/control/AutoPilot.h:79`
- `/Users/kogaryu/iggy/re3-miami/src/control/AutoPilot.h:93`
- `/Users/kogaryu/iggy/re3-miami/src/control/AutoPilot.h:94`
- `/Users/kogaryu/iggy/re3-miami/src/control/CarAI.h:7`
- `/Users/kogaryu/iggy/re3-miami/src/vehicles/Vehicle.h:179`
- `/Users/kogaryu/iggy/re3-miami/src/vehicles/Vehicle.h:185`
- `/Users/kogaryu/iggy/re3-miami/src/control/AutoPilot.cpp:50`

Vehicles own an `AutoPilot` object. Autopilot splits:

- mission: cruise, goto coords, ram/block player/car, attack, stop, delete.
- temporary action: wait, reverse, handbrake turn, turn, forward, swerve.
- driving style: stop, slow down, avoid, plough through, ignore lights.
- route/path node state, cruise speed, destination, target car.

Learning point: vehicle AI uses the same broad split: mission, temporary action,
movement style, and route scratch.

## Routes and Scripted Movement

Useful files:

- `/Users/kogaryu/iggy/re3-miami/src/peds/PedRoutes.h:3`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedRoutes.h:9`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedRoutes.h:12`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedRoutes.cpp:9`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedRoutes.cpp:25`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedRoutes.cpp:41`
- `/Users/kogaryu/iggy/re3-miami/src/peds/PedRoutes.cpp:57`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:553`
- `/Users/kogaryu/iggy/re3-miami/src/peds/Ped.h:959`

Ped routes are global route points with route ids and positions. Peds hold route
progress fields and can be assigned a follow-route objective.

Learning point: scripted route data is global route/map data. Ped state holds
current route id and progress.

## Save/Load Ownership

Useful files:

- `/Users/kogaryu/iggy/re3-miami/src/save/MemoryCard.cpp:292`
- `/Users/kogaryu/iggy/re3-miami/src/save/MemoryCard.cpp:320`
- `/Users/kogaryu/iggy/re3-miami/src/save/MemoryCard.cpp:335`
- `/Users/kogaryu/iggy/re3-miami/src/save/MemoryCard.cpp:437`
- `/Users/kogaryu/iggy/re3-miami/src/save/MemoryCard.cpp:440`
- `/Users/kogaryu/iggy/re3-miami/src/save/MemoryCard.cpp:446`
- `/Users/kogaryu/iggy/re3-miami/src/save/MemoryCard.cpp:457`
- `/Users/kogaryu/iggy/re3-miami/src/save/MemoryCard.cpp:480`
- `/Users/kogaryu/iggy/re3-miami/src/save/MemoryCard.cpp:501`
- `/Users/kogaryu/iggy/re3-miami/src/control/PathFind.cpp:1784`
- `/Users/kogaryu/iggy/re3-miami/src/control/PathFind.cpp:1805`

Save/load is block-based. It restores scripts, ped pool, garages, vehicles,
objects, path dynamic flags, pickups, phones, restart points, radar, zones, gang
data, car generators, particles, audio script objects, player info, stats,
streaming state, and ped type data.

Path save does not serialize the whole graph. It stores mutable path flags such
as disabled and between-level bits; the base graph is loaded/generated
elsewhere. Load also performs collision and streaming repair before full restore.

Learning point: save/load persists actor/session truth and small mutable graph
flags. Base navigation, collision, render, and presentation caches are rebuilt
from authoritative data.

## Reference Patterns

- Objective/state/move-mode split.
- Previous-objective restore for temporary actions.
- Threat masks separate from current observed threat.
- Movement intensity as data, not hidden in behavior code.
- Level-owned path graph with actor-owned short path-follow scratch.
- Attractors as interaction points with capacity and queues.
- Save only dynamic path flags, not the whole static graph.

## Legacy Costs

- Giant `CPed` object combining AI, animation, weapons, vehicle, chat, path,
  threat, interaction, and save fields.
- Huge objective switch as the long-term architecture.
- Raw pointer ownership/reference cleanup patterns.
- Hardcoded city-specific objectives like taxis, pizza, and ice cream.
- Global singleton-heavy control flow.

## Minimal Reference Lesson

re3's clean open-world ped skeleton is:

- objective: what the actor is trying to do
- state: what it is executing now
- move mode: how it moves while executing
- threat memory: what it currently reacts to
- interaction assignment: which level-owned point/queue accepted it

The strength is the layering; the cost is that many of those layers still live
inside one very large `CPed` object.
