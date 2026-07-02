# Deck-Driven Encounters with Board-State Tactical Reasoning — design seed v0.1

> **Lane:** Claude + the box fleet (AI behaviour / movement / later physics), per the user's
> 2026-07-02 lane assignment. (The seed text below says "hand to Codex" — that is residue from
> the chat it was drafted in; map GENERATION is Codex's lane, the AI that plays on those maps
> is this lane.) Sibling plans: `docs/stealth-ai-plan.md` (the suspicion substrate — largely
> BUILT), `docs/creative_mode/world_foundation_plan_v0_1.md` (the other lane's floor).
>
> **Planner ground-truth annotations (2026-07-02) — what of this already exists:**
> - §10–11 (suspicion layer) is substantially **SHIPPED** as stealth slices s5–s8: graded
>   alert FSM (`alertLevel` 0..1, Idle→Observant→Suspicious→Searching→Alert→combat, decay +
>   grace hysteresis), last-known-position investigate memory, patrol routes, the stealth-garden
>   headless testbed, and the s8 tuning readout (the knobs table §11 asks for). LOS occlusion +
>   vision cones shipped earlier. Sound events are NOT yet built (known backlog).
> - §12 (reasoning graph) is the biggest missing substrate: the physical map exists; the
>   compressed map-of-meaningful-positions does not.
> - §5–7 (deck layer: cards, requirements, budget, conflicts), §8 (chess scoring loop),
>   §9 (Go influence maps), §14–15 (personality/partner cards): none exist yet.
> - §16 (maps expose tactical affordances) is **the cross-lane contract**: Codex's map
>   generators AUTHOR the affordance metadata (ASCII glyphs / creative object kinds — note the
>   currently-inert markers T/R/? and the creative GameplayMarker descriptor family are exactly
>   this vocabulary waiting for readers); this lane's encounter system CONSUMES it. Design the
>   vocabulary ONCE. Deep symmetry worth exploiting: the reasoning-graph node vocabulary
>   (chokepoint / exit / hiding spot / high ground) is ALSO the recon-journal intel vocabulary
>   from the game vision — guard thoughts and thief notes describe the same map features.
> - §17–18 (battle reports, decision receipts) ride the repo's existing receipts+determinism
>   constitution directly.
> - §19's minimal prototype is a correct full-DNA minimum (source → generation → validation →
>   decision → report) and can run headless on the box, ctest-gated, like the stealth garden.

---

*The design seed below is the user's document, verbatim in substance.*

## 1. Purpose

This document describes a combat and encounter design system that combines three ideas:
card-game logic for enemy flavour, encounter variety, probability, and scenario generation;
chess-like logic for tactical move search, threat evaluation, and positioning; Go-like logic
for territory, influence, route control, containment, and pressure.

The goal is not to build expensive "perfect AI." The goal is to give enemies simple reasoning
tools that make them feel more alive, more human, and more situationally aware.

The system should allow one battlefield to produce many different combat scenarios. Instead of
hardcoding "goblin here, archer there, trap beside barrel," the battlefield exposes tactical
possibilities, and the encounter system deals enemies, hazards, personalities, objectives, and
tactics from structured decks.

```txt
Cards give enemies flavour.
Chess gives enemies tactics.
Go gives enemies space control.
Suspicion gives enemies memory.
```

## 2. Reference inspiration

Sierra's 1991 *Conquests of the Longbow: The Legend of Robin Hood*: story-driven, tracks player
performance across multiple systems (score, ransom money, surviving outlaws), resolves into
four endings, and embeds a Nine Men's Morris board-game sequence. Nine Men's Morris: 24 points,
nine pieces each, form "mills" (rows of three) to remove opponent pieces; phases for placing,
moving, and flying. The lesson is not to copy the game — it is that a small board-game
structure creates meaningful choices, tactical pressure, and branching outcomes with very
little machinery.

```txt
Map as board.
Enemies as pieces.
Cards as encounter flavour.
Board reasoning as tactical AI.
Suspicion graph as enemy memory.
```

## 3. Core design principle

A battlefield should not be authored as one fixed fight. It should be authored as a **tactical
possibility space**.

Not this:

```txt
Map: Old Bridge
Enemy: goblin at x=12,y=8
Enemy: archer at x=4,y=3
Hazard: fire barrel enabled
Weather: clear
```

This:

```txt
Map: Old Bridge

Tactical slots:
  roof archer slot
  bridge chokepoint
  flank spawn left
  flank spawn right
  trap slot near narrow path
  objective slot near cart
  mud-sensitive riverbank
  fire hazard socket

Encounter system:
  deals enemy roles
  deals faction
  deals weather
  deals tactic
  deals objective
  validates placement
  emits battle report
```

One map then produces many encounters (goblin ambush in rain / bandit patrol with fire barrels /
undead search party in fog — same bridge, different activated affordances). Replayability
without random nonsense: the map stays readable; the encounter changes because different cards
activate different affordances.

## 4. System overview

```txt
Battlefield Layer
  terrain, cover, elevation, chokepoints, spawn regions, hazard sockets

Deck Layer
  enemy roles, faction, weather, hazards, objectives, tactics, personalities

Tactical Reasoning Layer
  move search, threat scoring, territory control, route blocking, ally support

Suspicion / Memory Layer
  last-known player position, likely routes, searched nodes, clue trails
```

Data flow:

```txt
Battlefield source
  -> encounter cards are dealt
  -> enemies/hazards/objectives are assembled
  -> placement is validated
  -> AI receives role/personality/tactic weights
  -> battle plays out
  -> playtest telemetry records outcome
  -> report explains generated scenario and AI choices
```

## 5. Card layer: enemy flavour and encounter variation

The card layer decides what kind of encounter appears: who showed up, what faction, what roles,
what weapons/abilities, what hazards, what tactical posture, what objective, what personality
traits.

Card categories:

```txt
Faction:      goblins, bandits, guards, undead, wolves, cultists
Enemy role:   brute, archer, trapper, healer, commander, skirmisher, scout
Weapon:       spear, bow, shield, dagger, torch, net, crossbow
Ability:      dash, rally, poison, trap placement, shield wall, howl, smoke bomb
Personality:  coward, hunter, veteran, zealot, brute, commander, novice
Hazard:       rain, fog, fire barrels, mud, falling rocks, weak bridge, darkness
Tactic:       patrol, ambush, pincer, defend objective, bait, retreat, surround
Objective:    protect cart, guard prisoner, delay player, hold bridge, escape with loot
```

Example generated hand: Old Mill Bridge + Redcap Goblins + {Trapper, Archer, Skirmisher×2} +
Rain + Split Ambush + Protect Stolen Cart → archer to roof slot, trapper to bridge chokepoint,
skirmishers to flank spawns, rain reduces visibility and raises mud penalties, cart becomes the
tactical center.

## 6. Card requirements and validation

Cards are not dumb random modifiers. Every card declares what it **requires**, **provides**,
**modifies**, and **costs**.

```txt
Card: Goblin Trapper        | Card: Heavy Rain             | Card: Pincer Ambush
Type: EnemyRole             | Type: Weather                | Type: TeamTactic
Cost: 2                     | Cost: 1                      | Cost: 3
Requires:                   | Requires:                    | Requires:
  trap slot or narrow path  |   outdoor map or broken roof |   two flank spawn regions
Provides:                   | Effects:                     |   two mobile enemies
  light enemy body          |   reduces vision range       | Effects:
  trap placement action     |   dampens fire spread        |   pressure groups
  route denial behavior     |   mud penalty near dirt      |   exit-block value up
AI weights:                 |                              |   delayed reveal on trigger
  prefers chokepoints,      |                              |
  avoids melee, guards traps|                              |
```

Validation checks: does the map support the tactic; are required slots available; is the player
start safe; are objectives reachable; are enemies outside initial player vision; does total
difficulty fit the budget; do any cards conflict. Conflicts get RULES, not crashes (fire barrel
+ heavy rain → allow with dampened spread, or reject if fire is meant to be central).

## 7. Difficulty budget

```txt
Easy 5 / Medium 10 / Hard 16 points.
Skirmisher 1, Archer 2, Trapper 2, Commander 4, Rain 1, Fog 2,
Ambush 2, Pincer 3, Reinforcements-after-5-turns 2.
```

Draw until the budget fills or no legal cards remain. Report the drawn hand, the validation
results, and the total score.

## 8. Chess layer: tactical move search

Answers: where should this enemy move, what can it attack, what threats exist, what move
improves position / protects an ally / blocks the player / creates a fork.

```c
Action best_action = none;
int best_score = -999999;
for each legal_action:
    GameState test_state = simulate_action(current_state, legal_action);
    int score = evaluate_state(test_state, enemy);
    if (score > best_score) { best_score = score; best_action = legal_action; }
apply_action(current_state, best_action);
```

One- or two-step lookahead is enough at first. Scoring factors: + reaches cover, + high ground,
+ line of sight, + blocks escape route, + protects objective, + supports ally, + threatens
player, + controls chokepoint; − isolated, − exposed flank, − blocks ally, − objective
undefended, − danger zone, − wasted action.

Dumb AI: player seen → run at player. Tactical AI: player seen near west hallway → best move is
NOT the direct chase → move to south doorway to cut escape, second guard takes the west route.
Chess-like because the enemy evaluates board position, not just distance.

## 9. Go layer: influence and space control

Answers: what space do we control, what routes are weak, which escape paths are open, which
group is isolated, which positions influence the largest area, where does standing create
pressure. Useful maps: danger, visibility, sound, escape-route, objective-control,
guard-influence, player-suspicion.

A brute chases directly; a veteran holds the chokepoint; a hunter cuts the escape route; a
commander spreads units to control exits. Enemies feel like they understand space.

## 10. Suspicion graph AI

The key rule:

```txt
Do not make AI know where the player is.
Make AI know where the player probably is.
```

Improved loop: sees player → stores last-known position + direction + speed → marks nearby
routes suspicious → searches likely paths → checks hiding spots → blocks exits → alert decays
gradually. Requires TWO maps:

```txt
Physical map:   walls, floors, collision, doors, light, sound, vision cones
Reasoning map:  exits, chokepoints, hiding spots, patrol nodes, doors, stairs, objectives
```

The physical map moves bodies. The reasoning map moves thoughts.

## 11. Suspicion events

Increase: saw player, heard footstep, heard object break, found open door, found body, found
missing item, saw extinguished torch, saw blood trail, heard ally call, lost line of sight.
Decrease/redirect: searched-and-found-nothing, ally already watching, path blocked, door
locked, time passed, decoy found.

Alert states 0 calm / 25 suspicious / 50 searching / 75 alert / 100 confirmed contact.
Example deltas: saw player +60, footstep +20, found body +80, searched empty room −10, gradual
time decay. Never full-alert-to-asleep in a few frames.

## 12. Reasoning graph

A compressed version of the map: meaningful positions only, not every tile.
Node types: doorway, stair, chokepoint, hiding spot, cover cluster, objective, window, ladder,
exit, patrol post, high ground, sound source, last-known position.
Edge types: walkable, hidden, climb, locked, noisy, dangerous, guarded.

When the player disappears, score likely nodes: last-seen high; direction-of-travel high;
nearest exit high; dark hiding spot medium; watched hallway low; locked room impossible unless
key/lockpick known.

## 13. Guard decision loop

```c
Node best_node = none;
int best_score = -999999;
for each candidate_node:
    int score = suspicion(candidate_node)
              + strategic_value(candidate_node)
              - travel_cost(guard, candidate_node)
              - ally_coverage(candidate_node)
              + personality_bonus(guard, candidate_node);
    if (score > best_score) { best_score = score; best_node = candidate_node; }
assign_guard_to_search(best_node);
```

Actions: search hiding spot, block exit, guard objective, call ally, watch chokepoint, sweep
room, return cautiously to patrol, raise alarm.

## 14. Personality and role differences

Same reasoning system, different weights: Coward (avoids isolated nodes, calls allies early),
Veteran (blocks exits, holds formation, values chokepoints, slower alert decay), Hunter
(predicts escape routes, follows clue trails longer), Brute (rushes last-known, kicks doors),
Novice (overcommits, gives up sooner), Commander (assigns allies to nodes, controls the search
net). Defined by cards:

```txt
Personality Card: Veteran
  +20 chokepoint score, +15 ally support, -15 isolated chase, slower alert decay
```

## 15. Partner sync cards

Shield+Archer (hold lane / fire from cover), Trapper+Skirmisher (block route / pressure
escape), Healer+Brute (brute overextends because recovery exists), Commander+Cowards (cowards
hold while commander lives), Wolf+Hunter (scent follow / route cutoff). Encounters generate
TEAM identity, not just individuals.

## 16. Map requirements

Maps expose tactical metadata: spawn regions (incl. enemy-safe), player start region, objective
sockets, hazard sockets, cover clusters, high/low ground, trap slots, narrow routes, flank
routes, chokepoints, escape routes, visibility zones, weather-sensitive zones.

The map author does not place every enemy. **The map author creates affordances. The encounter
system decides which affordances are used.**

## 17. Battle report

Every generated encounter produces a report: battlefield, budget, cards drawn, placement (slot
assignments by name), validation results (reachability, spawn-vision, conflict resolutions),
estimated difficulty, warnings ("right flank route may be too strong; consider cover near
player entry"). The report is for debugging, tuning, playtesting, and AI handoff.

## 18. Debugging AI decisions

Every AI decision produces a small reasoning receipt:

```txt
Guard Decision Report
  Guard: Veteran Archer
  Chosen action: MoveToHighGround
  Score: 82
  Reasons: +30 line of sight, +20 protects objective, +15 near ally,
           +10 controls bridge route, -5 exposed to ranged
```

The developer can always inspect why an enemy moved, searched, attacked, retreated, or blocked.
No black boxes.

## 19. Minimal prototype

```txt
One ASCII battlefield.
Three enemy role cards (Brute, Archer, Trapper).
Three hazard cards (Rain, Fog, Fire Barrel).
Three tactic cards (Patrol, Ambush, Defend Objective).
One encounter budget.
One validator.
One simple enemy placement pass.
One AI scoring loop.
One battle report.
```

Minimum map metadata: player_start, roof_archer_slot, bridge_chokepoint, flank_spawn_left,
flank_spawn_right, objective_slot, trap_slot.
Minimum validation: player start reachable, objective reachable, spawn slots valid, no spawn
inside starting vision, card requirements satisfied, budget respected.
Minimum AI: generate legal moves → simulate → score → choose best → print decision report.

## 20. Naming

Formal: **Deck-Driven Encounters with Board-State Tactical Reasoning.**
Short: **Deck-Driven Tactical AI.** Stealth subsystem: **Suspicion Graph AI.**

## 21. Final design summary

Combat encounters as generated tactical hands. Cards create flavour, variety, and identity;
chess gives move search; Go gives space control; suspicion gives memory and deduction. The
battlefield is a structured possibility space — one map, many battles. Enemies do not need
expensive intelligence; they need useful tools: cards, tactical slots, scoring functions,
suspicion maps, and decision reports. The goal is not perfect AI. The goal is **inspired AI**:
readable, imperfect, purposeful decisions.
