# Spell Archetype Research

Updated: 2026-06-21

Exact purpose: collect open/reference spell and ability design patterns that can
inform bespoke `iggy3d` tactical real-time abilities without importing a copied
spell list or proprietary text.

This document is design research, not a file plan. It assumes the near-term
runtime surface is still builder-facing: deterministic commands, target/reach
queries, movement, combat events, status hooks, projection/debug output, and
renderer-visible projectile or area cues.

## Source Use

Use these references for patterns, not names or text.

- [D&D Beyond SRD v5.2.1](https://www.dndbeyond.com/srd) and the [SRD CC PDF](https://media.dndbeyond.com/compendium-images/srd/5.2/SRD_CC_v5.2.pdf): open class, spell stat block, range, duration, and spell-list structure.
- [5e SRD Spellcasting mirror](https://www.5esrd.com/spellcasting/): navigable reference for target path, area shapes, duration, and concentration-style constraints.
- [Pathfinder 2e Spells on Archives of Nethys](https://2e.aonprd.com/Spells.aspx), [Cast a Spell](https://2e.aonprd.com/Actions.aspx?ID=2734), [Conditions](https://2e.aonprd.com/Conditions.aspx), and [Action Economy / Ability Design](https://2e.aonprd.com/Rules.aspx?ID=2905): action-cost, focus/resource, status, and "small compelling set" patterns.
- [Open Legend core rules repository](https://github.com/openlegend/core-rules), especially [banes](https://raw.githubusercontent.com/openlegend/core-rules/master/banes/banes.yml) and [boons](https://raw.githubusercontent.com/openlegend/core-rules/master/boons/boons.yml): open status-effect vocabulary and genre-flexible effect separation.
- [Game Programming Patterns: Command](https://gameprogrammingpatterns.com/command.html): ability input should enter the runtime as explicit command data.
- [Unity Netcode client anticipation](https://docs.unity3d.com/Packages/com.unity.netcode.gameobjects@2.7/manual/advanced-topics/client-anticipation.html): separate visual anticipation from authoritative state.
- [Choosing the right network model for your multiplayer game](https://mas-bandwidth.com/choosing-the-right-network-model-for-your-multiplayer-game/): server-authoritative, deterministic, and snapshot tradeoffs.

## Design Translation Rules

- Do not ship SRD spell names or spell text as `iggy3d` abilities. Translate
  into combat verbs: heal, guard, reveal, mark, shove, blink, snare, obscure,
  pierce, cleanse, interrupt, and project.
- Every ability should resolve through an intent command, not through raw input
  or renderer state. Renderer-visible trails, decals, beams, and telegraphs are
  receipts of runtime decisions.
- Prefer prototypes that exercise existing or near-term runtime systems:
  stable target scans, reach checks, movement-to-point, projectile travel,
  collision queries, damage/status events, command log/replay, and projection.
- Treat "holy" and "arcane" as presentation and rule packages, not hardcoded
  engine categories. The runtime should care about tags such as `heal`,
  `shield`, `projectile`, `line_query`, `ground_area`, `teleport`, `stealth`,
  `mark`, `forced_move`, and `cleanse`.
- Keep early abilities deterministic. Avoid random spread, branching AI summons,
  procedural multi-stage illusions, or physics-chaos spells until replay and
  network receipt rules are proven.

## Shared Runtime Vocabulary

- Target models: self, single ally, single enemy, point, ground area, line,
  cone, capsule sweep, aura around caster, tether pair, projectile with owner,
  trap volume, and interactable object.
- Collision/query models: line of sight, ray cast, capsule cast, sphere overlap,
  cone overlap, nearest valid target, faction filter, reach gate, blocker query,
  ground snap, and projectile hit query.
- Status hooks: health delta, temp shield, guard bonus, courage/resist, marked,
  exposed, revealed, slowed, rooted, stunned, dazed, silenced, blinded/dazzled,
  hidden, displaced, phasing, burning/radiant tick, cleanse, interrupt, taunt,
  and cooldown modified.
- Movement hooks: impulse, forced step, dash, blink, pull, push, leash, root,
  slow field, speed boost, turn-rate modifier, collision bypass, and landing
  validation.
- Readability hooks: pre-cast tell, impact flash, persistent decal, target
  bracket, projectile trail, aura ring, line beam, vertical pillar, status icon,
  color family by damage/support type, and debug projection label.
- Network hooks: authoritative cast id, deterministic target set, predicted
  local VFX, correction-safe result receipt, projectile spawn tick, impact tick,
  status start/end tick, and no client-only gameplay mutation.

## Domain: Cleric

Cleric magic is best treated as stabilizing, revealing, cleansing, and shaping
safe territory. It should reward positioning without requiring twitch aiming.

### Archetype: Triage Pulse

- Gameplay purpose: emergency sustain that keeps a frontline actor alive and
  proves friendly targeting.
- Target model: self, single ally, or small ally cluster around a point.
- Runtime/physics needs: health delta, optional overheal cap, deterministic
  ally sort when multiple valid targets are in range.
- Collision/query needs: faction filter, range check, line-of-sight policy
  chosen per ability; early version can ignore blockers if it is a sacred pulse.
- Movement implications: encourages regrouping and escort movement; no forced
  movement in the first build.
- Camera/readability needs: vertical pillar or ring at healed actors, short
  duration so the effect is readable without becoming a screen wash.
- Status-effect hooks: heal, temp shield, remove one minor negative status.
- Cooldown/cost hooks: low to medium cooldown; cost can scale with number of
  allies affected.
- Multiplayer/networking concerns: client may anticipate the glow, but health
  and cleanse receipts must be authoritative and replayable.
- First-demo suitability: high if limited to self or one ally; medium if cluster
  targeting is needed.

### Archetype: Warding Seal

- Gameplay purpose: defensive ground control that makes a safe patch or route
  through a fight.
- Target model: ground point snapped to nav/physics surface; optional caster
  centered aura.
- Runtime/physics needs: persistent area lifetime, status application on enter,
  tick, and exit.
- Collision/query needs: sphere/cylinder overlap, ground snap, blocker policy
  for vertical differences.
- Movement implications: rewards standing ground; may slow enemies or reduce
  incoming damage while inside.
- Camera/readability needs: floor decal, soft vertical edge, clear ownership
  color, visible expiry pulse.
- Status-effect hooks: guard bonus, damage reduction, courage/resist,
  anti-fear, anti-stealth reveal for enemies.
- Cooldown/cost hooks: medium cooldown; cost should increase with radius and
  duration, not with per-tick target count.
- Multiplayer/networking concerns: area id plus start/end tick is cheaper and
  more deterministic than replicating every visual pulse.
- First-demo suitability: high for renderer/debug projection because the shape
  is simple and persistent.

### Archetype: Revealing Judgment

- Gameplay purpose: anti-rogue cleric tool that marks a target and makes it
  easier for allies to track or hit.
- Target model: single enemy, line-of-sight required.
- Runtime/physics needs: stable target selection, status duration, optional
  repeated reveal pulse.
- Collision/query needs: ray or visibility query; nearest-target fallback should
  be explicit if auto-targeting is allowed.
- Movement implications: target may be encouraged to break line of sight or
  leave the aura.
- Camera/readability needs: target bracket, overhead marker, thin beam on cast,
  and status icon in debug projection.
- Status-effect hooks: marked, revealed, exposed, anti-hidden.
- Cooldown/cost hooks: low damage or no damage; cost is mainly cooldown and
  opportunity cost.
- Multiplayer/networking concerns: hidden/revealed state must not leak to
  unauthorized clients before the authoritative reveal applies.
- First-demo suitability: high; proves target query, status event, and readable
  debug marker.

### Archetype: Cleansing Thread

- Gameplay purpose: counterplay to poison, burn, slow, silence, fear, and other
  negative states.
- Target model: self or single ally; later versions can chain to nearby allies.
- Runtime/physics needs: status registry, priority rules for which status is
  removed, and result reporting when nothing was cleansed.
- Collision/query needs: friendly target reach/range; no projectile collision
  needed unless presented as a tether.
- Movement implications: lets a slowed/rooted ally re-enter movement; should
  not teleport or reposition by itself.
- Camera/readability needs: tether from caster to target plus status icon clear.
- Status-effect hooks: cleanse one negative status, cleanse category, brief
  immunity window, or convert negative status into temp shield.
- Cooldown/cost hooks: medium cooldown; cost can depend on status severity.
- Multiplayer/networking concerns: cleanse ordering must be deterministic when
  multiple statuses expire on the same tick.
- First-demo suitability: medium until a useful negative status exists.

## Domain: Paladin

Paladin magic should sit at the contact point between weapon combat, protection,
and movement. It should make the frontline legible: who is protected, who is
challenged, and where the paladin is committing.

### Archetype: Radiant Strike

- Gameplay purpose: weapon-linked burst that turns a successful melee hit into
  a clear combat receipt.
- Target model: single enemy in melee reach; optional forward capsule for a
  cleaving variant.
- Runtime/physics needs: attack event hook, damage packet, optional bonus
  status if the target is already marked.
- Collision/query needs: reach query, melee capsule or target id validation.
- Movement implications: rewards closing distance; no auto-move in first build.
- Camera/readability needs: hit flash, short trail on weapon/projectile proxy,
  clear impact point.
- Status-effect hooks: radiant damage, exposed, interrupt, brief daze.
- Cooldown/cost hooks: charge spender or cooldown after confirmed hit; avoid
  consuming cost on rejected reach checks.
- Multiplayer/networking concerns: cast intent and hit confirmation must be
  separated so client prediction cannot invent hits.
- First-demo suitability: high because it extends the existing attack loop.

### Archetype: Guard Intercept

- Gameplay purpose: protect an ally by redirecting or reducing an incoming hit.
- Target model: ally within radius or tether; triggered by incoming damage.
- Runtime/physics needs: reaction/trigger window, damage modification, source
  and protected-target ids.
- Collision/query needs: ally range check, optional line between paladin and
  ally if physical interception is required.
- Movement implications: later variant can step the paladin toward the ally;
  first version should be pure damage reduction.
- Camera/readability needs: shield arc between attacker and protected ally,
  readable even in overhead camera.
- Status-effect hooks: guard, intercept, temp shield, taunt on attacker.
- Cooldown/cost hooks: per-trigger cooldown or shared guard resource; strict
  one-response policy if multiple paladins protect the same ally.
- Multiplayer/networking concerns: resolve in authoritative damage pipeline,
  then replicate a single intercept receipt.
- First-demo suitability: medium; excellent later, but needs incoming-damage
  trigger plumbing.

### Archetype: Challenge Mark

- Gameplay purpose: force a frontline relationship between paladin and enemy
  without hard mind control.
- Target model: single enemy in line of sight or melee reach.
- Runtime/physics needs: status duration and conditional penalty if target
  attacks anyone else.
- Collision/query needs: target scan and line-of-sight; optional periodic range
  check to maintain challenge.
- Movement implications: creates a leash-like tactical space; target can kite
  or break range.
- Camera/readability needs: two-point tether, target marker, pulse when the
  marked enemy violates the challenge.
- Status-effect hooks: marked, taunted/provoked, damage reduction for allies,
  retaliation trigger.
- Cooldown/cost hooks: low cost, medium cooldown; refresh should replace rather
  than stack.
- Multiplayer/networking concerns: conditional triggers must record source
  target and victim target to replay consistently.
- First-demo suitability: high; no complex physics and very visible.

### Archetype: Shield Charge

- Gameplay purpose: frontline engage tool that proves movement plus impact.
- Target model: enemy or ground point along a constrained line.
- Runtime/physics needs: dash movement, collision stop, impact event, optional
  knockback.
- Collision/query needs: capsule sweep, blocker hit, target hit, ground snap at
  destination.
- Movement implications: commits the paladin; must fail safely if destination
  is blocked.
- Camera/readability needs: windup, path telegraph, strong stop/impact frame.
- Status-effect hooks: knockback, daze, guard self on arrival, interrupt.
- Cooldown/cost hooks: high cooldown; cost may be refunded if no movement
  occurs due to admission failure.
- Multiplayer/networking concerns: authoritative path and impact tick are
  required; clients can show anticipation path only.
- First-demo suitability: medium-high once sweep movement exists.

## Domain: Mage

Mage magic is the broadest domain: projectile, area, force, blink, barrier,
counterplay, and terrain manipulation. For `iggy3d`, early mage prototypes
should stress projectile/render and shape-query proof without requiring complex
AI.

### Archetype: Arcane Bolt

- Gameplay purpose: baseline aimed projectile for combat, render trail, and hit
  receipt testing.
- Target model: single enemy target id, aim ray, or ground-directed projectile.
- Runtime/physics needs: projectile entity or transient projectile record,
  velocity, lifetime, owner, hit result, deterministic impact.
- Collision/query needs: ray or swept sphere/capsule; blocker and target hit
  filters.
- Movement implications: caster can fire while stationary first; later variants
  can allow cast while moving with aim penalty.
- Camera/readability needs: trail, muzzle flash, impact spark, debug path line.
- Status-effect hooks: damage, exposed on hit, small interrupt.
- Cooldown/cost hooks: low cooldown or basic attack; cost can be zero for first
  projectile proof.
- Multiplayer/networking concerns: spawn tick, initial transform, velocity, and
  authoritative hit tick must be logged.
- First-demo suitability: very high; this is the cleanest projectile/render
  prototype.

### Archetype: Force Push

- Gameplay purpose: spatial control that moves enemies or objects without
  needing high damage.
- Target model: cone, line, or single enemy.
- Runtime/physics needs: displacement solver, blocker-safe end position, stable
  ordering when multiple targets are moved.
- Collision/query needs: cone overlap or line sweep, per-target blocker checks,
  nav/ground validation.
- Movement implications: tests forced movement, ledge rules later, and tactical
  repositioning.
- Camera/readability needs: expanding wavefront, target motion arrows, impact
  dust or spark.
- Status-effect hooks: forced move, prone/knockdown later, interrupt.
- Cooldown/cost hooks: medium cooldown; cost scales with area and displacement.
- Multiplayer/networking concerns: replicate final movement result, not just
  client-calculated impulse.
- First-demo suitability: high if simplified to one target or short line.

### Archetype: Ground Burst

- Gameplay purpose: teach area targeting, delay tells, and multi-target damage.
- Target model: ground point with radius; optional delayed impact.
- Runtime/physics needs: area placement, cast delay, damage packet to valid
  occupants at impact tick.
- Collision/query needs: ground snap, radius overlap, line-of-sight or cover
  policy.
- Movement implications: creates dodge pressure; interacts well with slow-time
  tactical camera.
- Camera/readability needs: pre-impact circle, vertical column, strong impact
  ring, lingering scorch/decal.
- Status-effect hooks: damage, burning tick, slow field if persistent.
- Cooldown/cost hooks: medium/high cooldown; cost scales with radius, delay,
  damage, and persistence.
- Multiplayer/networking concerns: target point and impact tick are
  authoritative; clients can show tell immediately.
- First-demo suitability: high for renderer shape proof.

### Archetype: Blink Step

- Gameplay purpose: mage mobility and dodge repositioning.
- Target model: ground point within radius, optionally along aim direction.
- Runtime/physics needs: admission check, destination validation, optional path
  trace to prevent wall bypass.
- Collision/query needs: ground snap, occupancy test, line/arc blocker policy.
- Movement implications: bypasses normal movement cost; must be constrained by
  collision and encounter boundaries.
- Camera/readability needs: before/after silhouettes, arrival flash, camera
  smoothing so the jump is visible but not disorienting.
- Status-effect hooks: phasing during cast, brief invulnerability optional,
  displaced afterimage.
- Cooldown/cost hooks: medium/high cooldown; no damage in first version.
- Multiplayer/networking concerns: server authoritative destination; client can
  anticipate local visual but must accept correction.
- First-demo suitability: medium-high; strong movement playground fit.

### Archetype: Null Ward

- Gameplay purpose: mage defensive/counterplay layer for projectiles and
  status-heavy enemies.
- Target model: self shield, ally shield, or small barrier plane.
- Runtime/physics needs: shield value, absorb rules, projectile intercept rules
  if barrier form is used.
- Collision/query needs: incoming projectile intersection, damage pipeline hook,
  optional area overlap.
- Movement implications: self-shield allows repositioning; barrier form anchors
  space.
- Camera/readability needs: translucent shell or plane with visible hit ripples.
- Status-effect hooks: temp shield, cleanse, projectile block, status immunity.
- Cooldown/cost hooks: medium cooldown; shield budget should be finite.
- Multiplayer/networking concerns: interception must be authoritative to avoid
  clients disagreeing on projectile hits.
- First-demo suitability: medium; better after projectile basics are green.

## Domain: Rogue

Rogue abilities should be physical, deceptive, and timing-heavy. Magic-adjacent
rogue tools can exist, but their runtime identity is mobility, stealth,
precision, traps, and interrupt windows.

### Archetype: Evasive Dash

- Gameplay purpose: fast repositioning that proves non-teleport movement and
  avoidance windows.
- Target model: direction vector or ground point within short range.
- Runtime/physics needs: dash movement curve, collision stop, optional evasion
  status during travel.
- Collision/query needs: capsule sweep, ground snap, blocker result.
- Movement implications: core mobility tool; can cross small gaps later only if
  authored per level.
- Camera/readability needs: low trail, afterimage, path preview for tactical
  camera.
- Status-effect hooks: evading, displaced, speed boost, brief untargetable only
  if balanced.
- Cooldown/cost hooks: low/medium cooldown; maybe stamina-style charge count.
- Multiplayer/networking concerns: authoritative final position and collision
  result; predicted local dash should reconcile smoothly.
- First-demo suitability: high for movement playground.

### Archetype: Precision Mark

- Gameplay purpose: sets up a high-value target and makes rogue damage readable
  without hidden math.
- Target model: single enemy in sight, melee reach, or last-hit target.
- Runtime/physics needs: mark status, next-hit bonus, expiry, source ownership.
- Collision/query needs: target line-of-sight or reach depending on variant.
- Movement implications: encourages flank or backline pathing; no movement by
  itself.
- Camera/readability needs: subtle target bracket, critical-hit flash when
  consumed.
- Status-effect hooks: marked, exposed, armor break, bleed/persistent damage.
- Cooldown/cost hooks: low cooldown if consumed by next hit; cannot stack from
  multiple rogues unless the source id is retained.
- Multiplayer/networking concerns: mark consumption must resolve atomically with
  the damage event.
- First-demo suitability: high; pairs well with existing attack proof.

### Archetype: Trick Mine

- Gameplay purpose: player-authored hazard that tests deployables, trigger
  volumes, and delayed effects.
- Target model: ground point or object surface.
- Runtime/physics needs: deployable entity, owner, lifetime, trigger event,
  payload effect.
- Collision/query needs: ground snap, occupancy validation, sphere/capsule
  trigger overlap.
- Movement implications: controls lanes and punishes pursuit.
- Camera/readability needs: visible to owner, subtle but fair enemy tell, strong
  trigger effect.
- Status-effect hooks: slow, root, daze, bleed, reveal, or smoke.
- Cooldown/cost hooks: charge count plus max active mines; old mine can expire
  when cap is exceeded.
- Multiplayer/networking concerns: visibility rules and trigger authority must
  be explicit, especially for hidden traps.
- First-demo suitability: medium-high; useful after ground placement exists.

### Archetype: Smoke Veil

- Gameplay purpose: breaks targeting, creates a safe retreat, and tests visual
  obstruction rules.
- Target model: self-centered or ground area cloud.
- Runtime/physics needs: persistent volume, visibility modifier, target query
  override.
- Collision/query needs: area overlap and line-of-sight attenuation, not solid
  collision.
- Movement implications: encourages retreat, flank, or revive play; should not
  hard block movement.
- Camera/readability needs: transparent enough for player readability, clear
  edge, overhead decal.
- Status-effect hooks: hidden, concealed, revealed immunity break, accuracy
  penalty.
- Cooldown/cost hooks: medium cooldown; duration and radius are the main costs.
- Multiplayer/networking concerns: fog-of-war or stealth visibility must not be
  client-only. Authoritative targetability still wins.
- First-demo suitability: medium; visually useful, but target-obstruction rules
  need care.

### Archetype: Disrupting Cut

- Gameplay purpose: interrupt cast/channel/status sustain through a melee hit.
- Target model: melee enemy.
- Runtime/physics needs: attack hook, interruptible state flag, cast/channel
  event cancel.
- Collision/query needs: melee reach and target id validation.
- Movement implications: rewards closing on casters; no forced movement.
- Camera/readability needs: sharp hit cue and canceled-cast spark.
- Status-effect hooks: interrupt, silence, brief daze, cooldown tax.
- Cooldown/cost hooks: low damage but meaningful cooldown; do not let it cancel
  every major ability for free.
- Multiplayer/networking concerns: interrupt must resolve before the target's
  ability impact if both occur on the same tick; define priority.
- First-demo suitability: medium until cast/channel states exist.

## Domain: Cleric/Paladin Hybrid

The cleric/paladin intersection is sacred frontline support: protect a location,
bind threat to the defender, and turn rescue into visible commitment.

### Archetype: Consecrated Frontline

- Gameplay purpose: the party fights better behind or near the holy frontline.
- Target model: caster-centered aura or forward ground zone.
- Runtime/physics needs: persistent area, ally/enemy differentiated statuses,
  enter/exit events.
- Collision/query needs: radius overlap, faction filter, optional facing cone.
- Movement implications: encourages formation play and tactical camera
  positioning.
- Camera/readability needs: wide but low ring, edge tick, ally buff icons.
- Status-effect hooks: ally guard, enemy slow, enemy reveal, courage/resist.
- Cooldown/cost hooks: high cooldown or ultimate-like resource if it combines
  ally and enemy effects.
- Multiplayer/networking concerns: area membership should be recomputed
  deterministically by tick, not replicated as a client-maintained list.
- First-demo suitability: high if reduced to one ally buff plus enemy reveal.

### Archetype: Rescue Touch

- Gameplay purpose: paladin commits to an endangered ally and stabilizes them.
- Target model: single ally in reach; later short dash-to-ally variant.
- Runtime/physics needs: heal or shield, downed/staggered state hook later,
  optional movement admission if dash variant.
- Collision/query needs: ally reach, line-of-sight optional, dash path validation
  later.
- Movement implications: current version rewards positioning; dash version
  proves protected movement.
- Camera/readability needs: bright contact cue, shield wrap, clear ally target
  marker before cast.
- Status-effect hooks: heal, temp shield, cleanse, stabilize, courage.
- Cooldown/cost hooks: high cooldown if it prevents defeat; low effect if
  available often.
- Multiplayer/networking concerns: do not let client-side prediction revive or
  restore control before authoritative receipt.
- First-demo suitability: medium-high once ally/dummy damage states exist.

### Archetype: Vow Tether

- Gameplay purpose: bind paladin, ally, and enemy into a legible protection
  triangle.
- Target model: ally protected target plus enemy threat target, or paladin plus
  one enemy.
- Runtime/physics needs: pair status, conditional mitigation or retaliation.
- Collision/query needs: range checks between participants, optional
  line-of-sight maintenance.
- Movement implications: movement can stretch or break the tether; good for
  tactical spacing.
- Camera/readability needs: thin tether lines that do not clutter; pulse only on
  trigger.
- Status-effect hooks: guard, challenge, retaliation, exposed if vow broken.
- Cooldown/cost hooks: medium/high cooldown; one active vow per caster.
- Multiplayer/networking concerns: pair identity and break conditions must be
  stable in saves/replay.
- First-demo suitability: medium; good second wave after simple marks.

## Domain: Mage/Rogue Hybrid

The mage/rogue intersection is arcane misdirection: mobility, decoys, traps,
silence, and precision delivered through spatial tricks.

### Archetype: Blink Strike

- Gameplay purpose: combine repositioning with a precise hit, but keep both
  parts auditable.
- Target model: single enemy within blink range and valid destination near it.
- Runtime/physics needs: destination search, teleport, melee hit, status apply.
- Collision/query needs: line/blocker policy, landing occupancy, enemy reach at
  arrival.
- Movement implications: high commitment; can bypass normal approach if not
  tightly constrained.
- Camera/readability needs: departure flash, arrival slash, brief afterimage at
  source.
- Status-effect hooks: exposed, bleed, interrupt, displaced.
- Cooldown/cost hooks: high cooldown; split refund policy if teleport succeeds
  but attack fails should be avoided in first version.
- Multiplayer/networking concerns: one command should produce ordered receipts:
  admission, move, attack, status.
- First-demo suitability: medium-high after blink and melee hit are stable.

### Archetype: Decoy Double

- Gameplay purpose: create a readable false target that manipulates attention
  without full AI complexity.
- Target model: self or ground point near caster.
- Runtime/physics needs: temporary decoy entity, owner, lifetime, targetability
  rules, optional threat pulse.
- Collision/query needs: spawn occupancy validation; target queries must choose
  whether decoys count.
- Movement implications: caster can reposition while enemies waste targeting.
- Camera/readability needs: silhouette distinct from player but believable at a
  glance; collapse cue.
- Status-effect hooks: hidden on caster, taunt/provoke on decoy, reveal when
  hit.
- Cooldown/cost hooks: medium/high cooldown; one active decoy cap.
- Multiplayer/networking concerns: decoy authority and visibility must be
  explicit; never let client-only decoys affect target queries.
- First-demo suitability: medium; useful but requires targetability rules.

### Archetype: Arcane Snare

- Gameplay purpose: magic trap that roots or slows enemies at a chosen point.
- Target model: ground point, trap volume, or projectile-deployed trap.
- Runtime/physics needs: deployable, trigger volume, status duration, optional
  arming delay.
- Collision/query needs: ground snap, trigger overlap, blocker-safe placement.
- Movement implications: constrains enemy movement and creates combo setup for
  projectiles or ground burst.
- Camera/readability needs: visible rune ring after arming, strong trigger flash.
- Status-effect hooks: root, slow, revealed, exposed.
- Cooldown/cost hooks: charge count, max active snares, arming delay as balance.
- Multiplayer/networking concerns: hidden/visible trap rules must be
  deterministic by faction and observer.
- First-demo suitability: high if visible to all and simple root/slow.

### Archetype: Silence Step

- Gameplay purpose: rogue-mage counter to casters and support chains.
- Target model: short dash path or arrival point area.
- Runtime/physics needs: dash or blink movement, silence area or target status.
- Collision/query needs: movement sweep or destination validation, area overlap
  at arrival.
- Movement implications: lets a rogue enter danger to shut down a target.
- Camera/readability needs: muted ring at landing, clear debuff icon, no noisy
  VFX that contradicts "silence."
- Status-effect hooks: silence, interrupt, hidden break, exposed on caster.
- Cooldown/cost hooks: medium/high cooldown; shorter duration if attached to a
  mobility effect.
- Multiplayer/networking concerns: action denial must be authoritative and
  priority-ordered against same-tick cast completion.
- First-demo suitability: medium until cast/channel mechanics exist.

## Ranked First-Build Spell Prototypes

These are ranked for the current `iggy3d` movement playground and
projectile/render work, not for final game balance.

1. Arcane Bolt
   - Why first: proves projectile spawn, travel, hit query, impact receipt, and
     renderer trail with one enemy dummy.
   - Minimal build: straight projectile from caster to target/aim point, single
     damage event.
2. Radiant Strike
   - Why first: extends existing attack/combat proof with a visible magic hit.
   - Minimal build: melee reach gate, damage packet, impact VFX cue.
3. Revealing Judgment
   - Why first: target query plus status marker without movement complexity.
   - Minimal build: line-of-sight target id, `revealed` and `marked` status,
     debug projection label.
4. Warding Seal
   - Why first: persistent ground area and ally status proof.
   - Minimal build: ground point, radius overlap, temp guard while inside.
5. Force Push
   - Why first: tests deterministic forced movement and collision-safe end
     positions.
   - Minimal build: single target push along caster-to-target vector.
6. Evasive Dash
   - Why first: exercises movement playground with a player-authored short dash.
   - Minimal build: capsule sweep to destination, arrival receipt.
7. Ground Burst
   - Why first: excellent for telegraph readability and area query testing.
   - Minimal build: delayed radius impact at ground point.
8. Challenge Mark
   - Why first: durable frontline relationship, no projectile required.
   - Minimal build: status on enemy; extra receipt if it attacks non-paladin
     later.
9. Arcane Snare
   - Why first: deployable ground entity plus trigger overlap.
   - Minimal build: visible rune trap that applies slow/root on first enemy.
10. Triage Pulse
    - Why first: proves friendly targeting and health/status receipt.
    - Minimal build: self or one ally heal with capped amount.
11. Precision Mark
    - Why first: cheap rogue setup loop that pairs with Radiant Strike or basic
      attack.
    - Minimal build: next hit consumes mark and adds exposed/damage receipt.
12. Blink Step
    - Why first: strong movement fantasy and destination validation test.
    - Minimal build: ground-target blink with no damage and strict blocker rule.
13. Consecrated Frontline
    - Why first: combines aura readability with simple ally/enemy filters.
    - Minimal build: caster aura grants ally guard and reveals enemies.
14. Trick Mine
    - Why first: deployable/trap foundation for rogue and mage/rogue hybrids.
    - Minimal build: visible mine, trigger radius, daze or slow.
15. Null Ward
    - Why first: projectile counter once Arcane Bolt exists.
    - Minimal build: self shield absorbs one incoming projectile or fixed damage.

## Prototype Acceptance Questions

Ask these before handing any prototype to a builder:

- What is the exact target model: entity id, point, direction, volume, or
  trigger?
- Which query owns validity: admission, movement, combat, collision, or status?
- Does the ability mutate position, health, status, inventory/objectives, or
  only projection?
- What is logged in the command receipt: accepted, rejected, spawned, impacted,
  expired, status applied, status removed?
- Can replay compute the same target set and impact tick from the same command?
- What does the renderer need: projectile trail, decal, ring, beam, tether,
  icon, hit flash, or only debug text?
- What is the first failure mode: out of range, blocked line, invalid ground,
  target dead/inactive, cooldown unavailable, no resource, or status immune?

## Math/Data-Driven Later

Make these pure math or data-driven once the first authored prototypes are
green:

- Numeric tuning: cooldown, cost, range, radius, duration, speed, damage,
  healing, shield amount, displacement distance, tick interval.
- Target filters: faction, alive/active, targetable, interactable, hidden,
  airborne/grounded, marked, status immune.
- Shape queries: line, cone, sphere, cylinder, capsule, projected ground ring.
- Projectile travel: speed, radius, lifetime, gravity/no-gravity, pierce count,
  bounce count if ever added.
- Status lifecycle: apply tick, stack/refresh rule, expire tick, cleanse tags,
  immunity tags, deterministic priority.
- Friendly-fire and falloff policy.
- AI scoring against ability tags, for example:

```text
utility = target_value + status_value + positioning_value - risk - cooldown_pressure
```

- Network receipts: command id, cast id, projectile id, status id, start tick,
  impact tick, expire tick, source id, target ids.

Keep these formulas deterministic, finite-value guarded, and independent from
renderer frame rate.

## Authored/Bespoke Long Term

Keep these authored rather than fully generic:

- Ability identity, naming, icon silhouette, sound, animation timing, and color
  language.
- Telegraph timing and impact readability. These are game-feel decisions, not
  just radius values.
- Combo rules that define character identity: paladin challenge plus intercept,
  cleric seal plus cleanse, mage snare plus burst, rogue mark plus dash.
- Boss exceptions and encounter-specific interactions.
- Camera framing rules for major abilities, especially blink, charge, and large
  ground effects.
- AI personality: a paladin-like ally should protect differently from a cleric
  even if both can apply shields.
- The "one weird thing" per class domain. A generic tag library can support
  bespoke abilities, but it should not flatten them into interchangeable rows.

## Practical Builder Notes

- Start with five effect primitives: `Damage`, `Heal`, `ApplyStatus`,
  `MoveActor`, and `SpawnTimedArea`. Most first-build prototypes can be composed
  from these without a general spell engine.
- Add ability metadata only when two prototypes need it. Avoid building a full
  content scripting system before Arcane Bolt, Radiant Strike, Warding Seal, and
  Evasive Dash are proven.
- Projection should expose enough debug information for each prototype:
  ability id, source id, target ids, area center/radius, projectile path, status
  name, start tick, and expire tick.
- Renderer work should prioritize readable receipts over ornate effects: trail,
  beam, ring, marker, decal, and impact flash are enough for the first pass.
- Multiplayer-readiness means command/replay correctness first. Visual
  anticipation can be added later as a client-side layer if authoritative
  receipts stay stable.
