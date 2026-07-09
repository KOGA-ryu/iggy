# External Research Requests v1 — battle-tested shapes for the guard senses & tactics arc

For the research team. Each brief: **why** (which phase of `docs/guard_senses_and_tactics_plan_v1.md` it
feeds), **the questions**, and the **deliverable format**. We want the *shape* of proven implementations —
algorithm structure, the actual tuned constants, and their failure-policy decisions — with **file/function
pointers** so we can read the real code afterward. Pseudocode over prose; numbers over adjectives.

**Priority order: Brief 1 is worth more than all others combined. If only one gets done, do that one.**

---

## Brief 1 — The Dark Mod: guard perception, alert dynamics, and search (feeds P3, P5, P6d)

TDM is the reference stealth-AI codebase, and our alert-band thresholds are already TDM-derived — we should
know how deep the rest of their model goes before we finish ours.

**1a. Visual detection function** *(feeds P3, and pressure-tests our rulings R1–R3)*
- The full input set and combination shape: distance, angle-off-axis, light level on target, target motion,
  target size/stance, fog — how are they combined (multiplicative? thresholded? accumulated over time)?
- Is detection **instant or integrated** (a "visibility score accumulates until threshold" model)? What are
  the time constants?
- **Vertical FOV**: do they gate vision vertically at all? How do they handle a thief on a ledge/rooftop
  above a guard (Thief verticality is the genre's escape verb — what does their cone actually do)?
- **Eye origin**: per-stance eye heights? Where do they cast from, and do they sample MULTIPLE target points
  (head/chest/feet) instead of one eye-to-eye ray? If multi-point: how many, and how do partial results
  combine?
- **Occlusion trace policy**: what happens on a failed/degenerate trace — fail open (seen) or closed
  (unseen)? What blocks vision (materials, water, fog volumes)? Any margin/epsilon handling at surface
  contact? What is their equivalent of our `startInside` case?

**1b. Alert state machine** *(feeds P5 — validates or refutes our fix spec before we build it)*
- The alert levels + exact numeric thresholds, rise increments per stimulus type (visual/audio/tactile/
  evidence), and the **decay** model: curves, per-level delays, floors ("never fully calms down"?).
- **The sustained-stimulus question (our P5 bug)**: what does a *continuous* sound (machine, looping noise)
  do to their alert level — ratchet, plateau, or habituate? Do repeated identical stimuli get diminishing
  returns? Is there stimulus *novelty* tracking?
- Grace/latency windows: is there an equivalent of our grace-window swallow of small rises?
- What can and cannot reach combat alert (our R7: sound alone is capped below Combat — do they do the same)?

**1c. Search & investigation** *(feeds P6d tactic #1 — they have literally solved "check hiding spots")*
- Their hiding-spot search system: how are candidate search points generated (darkness-based grid?), scored,
  and ordered? Radius around last-known? How many points before give-up?
- Give-up semantics: timers, what resets them, how the guard de-escalates back to patrol.
- Do searchers communicate (multi-guard search coordination), and how minimal is v1 of that?

**1d. Sound propagation** *(feeds P3's sound mapping + a future socket)*
- Distance model + how walls/doors attenuate (portal-based propagation vs ray-blocked wall-loss like ours?).
- What a heard sound *grants*: alert rise only, or an investigate target at the origin? Positional fuzz on
  the heard location?

**Deliverable:** per sub-brief — the algorithm in pseudocode, the constants table (their shipped tuning
values), the failure-policy notes, and file/class/function pointers into the TDM source.

---

## Brief 2 — OpenXCom: LOS, height layers, and stance (feeds P3 pins + the stance socket)

- Their tile/voxel LOS algorithm: how a sight line is traced through a 3D tile world (voxel bresenham?),
  and how **height layers** interact with vision (seeing up/down between floors).
- **Eye height per stance**: standing vs kneeling — where is it defined, how does it change LOS results, and
  how does stance interact with cover height (our sneak-eye socket needs exactly this shape).
- Graduated occlusion: smoke/partial cover *accumulating* along a ray vs binary blocked — how is it summed,
  and is accumulation worth the complexity (we are binary today)?
- Spotting/reaction system: is perception event-driven (unit enters view triggers) or per-tick polled?
- **Deliverable:** trace-algorithm pseudocode, stance/eye constants, accumulation model, file pointers.

## Brief 3 — Godot: debug-draw architecture + navigation activation (feeds P4, P6c)

- How debug geometry flows from simulation to renderer **without coupling the sim to the render lane**:
  their debug-draw modes, who owns the geometry, immediate vs retained, how toggles are taxonomized
  (per-system debug channels?). We have a runtime→app→render lane law; we want their version of it.
- NavigationServer: how nav regions/graphs are **built and activated at world/scene load** (their shape for
  our P6c "reasoning graph at world launch" hook), and how nav debug rendering is wired.
- **Deliverable:** the data-flow diagram (sim → server → renderer), toggle taxonomy, activation lifecycle,
  file pointers.

## Brief 4 (opportunistic bundle — smaller sweeps, one pass each)

- **re3-miami — wanted-level dynamics** *(P5 cross-check)*: escalation/decay rules, what pauses decay
  (line-of-sight to cops?), evidence radius, the timer constants. It's an alert FSM at city scale — we want
  its decay-pause policy compared against ours.
- **DevilutionX — monster aggro/leash** *(sanity-check)*: aggro acquisition, leash-to-home semantics, and
  de-aggro — compared against our home-leash predicates (`horizontalDistanceMeters` vs home).
- **Warzone 2100 — sensor model** *(future socket)*: sensor ranges/types, ECM/counter-sensor, shared
  team-visibility — the shape of "perception radius as a first-class tunable object" and multi-agent shared
  awareness (our multi-guard socket).
- **OpenTTD — YAPF pathfinder** *(P6d chokepoint-hold cross-check)*: cost-model structure for route choice —
  how penalties/costs compose, for when our route machinery needs "prefer the chokepoint between X and Y"
  style costs.
- **Deliverable:** one page each — model shape, constants, one insight we should steal, file pointers.

---

## Standing format notes for all briefs

- **Constants matter as much as algorithms** — shipped tuning values are decades of playtesting; we will
  seed ours from theirs (as the alert bands already did).
- **Failure policies matter most** — every "what happens when the trace fails / the input is degenerate /
  the state is stale" decision, explicitly. That's where our bugs were.
- Cite everything (repo path + function). We build from the shape, then read the source at the cited lines.
- Version/commit of the repo consulted, so the pointers stay stable.
