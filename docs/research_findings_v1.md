# Research Findings v1 - Guard Senses and Tactics

Local source snapshots inspected on 2026-07-09 under `/Users/kogaryu/game-references`.
These reference folders do not contain usable git metadata, so version notes come from
source files where available.

## Executive pressure tests

- TDM is the strongest source for P3-P6. Its AI vision is integrated over time,
  uses horizontal and vertical FOV for AI, gates on FOV before visibility, uses
  multi-point LOS for general actor visibility, and fails closed on missing sight
  prerequisites.
- TDM validates the P5 "sound cannot force combat by itself" ruling. Alert can
  ratchet from repeated accepted sound events, but combat is gated on having an
  enemy. Grace windows and noisemaker dedupe suppress some repeated audio events;
  there is no generic sustained-loop habituation model in the inspected code.
- TDM search has a fully fledged hiding-spot system, including darkness-based
  quality, but its current default search type uses generated radial search spots.
  The hiding-spot implementation is still the best shape for a P6d tactic.
- TDM sound propagation is portal/flood based with area and portal loss, not a
  single ray-blocked wall-loss check.
- OpenXCom is the best countermodel for P3d eye sockets: stance changes eye
  height, visibility is voxelized through height layers, and smoke accumulates
  along the ray instead of being binary blocked.
- Godot gives the P4 debug-lane shape: scene/sim objects publish server RIDs and
  resources; rendering-server debug instances are built and hidden independently.
- re3 supports a P5 cross-check that decay can pause under active pressure.
  DevilutionX supports simple home/pack leash predicates. Warzone 2100 supports a
  multi-guard shared-awareness socket. OpenTTD YAPF supports additive routing
  cost composition for chokepoint-style tactics.

## Brief 1 - The Dark Mod

Snapshot: `/Users/kogaryu/game-references/darkmod_src-trunk`, engine marker
`ENGINE_VERSION "TDM 2.15"` in `framework/Licensee.h:30`.

### Visual Detection

Primary file pointers:

- `/Users/kogaryu/game-references/darkmod_src-trunk/game/Actor.cpp:865`
  reads spawnargs `fov` default `150` and `fov_vert` default `-1`.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/Actor.cpp:1788`
  resolves eye height from `eye_height`, then eye/head joints, then bounds.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/Actor.cpp:2085`
  stores horizontal and vertical FOV cosine thresholds.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/ai/AI.cpp:12355`
  overrides FOV using the AI head joint. It projects target delta into the
  head-horizontal plane and returns true only when both horizontal and vertical
  dot products pass.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/Actor.cpp:2178`
  implements general `CanSee`: actor target eye, origin, shoulder 1, shoulder 2.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/ai/AI.cpp:10514`
  implements active player visual scan.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/ai/AI.cpp:10800`
  computes player visibility from lightgem, acuity, and distance falloff.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/gamesys/SysCvar.cpp:53`
  defines sight cvars: probability/magnitude legacy values plus max/min distance.

Shipped constants and defaults:

- AI default horizontal FOV: `150` degrees.
- AI default vertical FOV: `-1`, meaning reuse the horizontal threshold.
- `tdm_ai_sightmax = 40.0` meters.
- `tdm_ai_sightmin = 15.0` meters.
- `tdm_ai_sight_combat_cutoff = 20.0` meters.
- Vision menu multipliers in `SysCvar.cpp:543`: nearly blind `0.134`, forgiving
  `0.402`, challenging `0.804`, hardcore `1.005`.
- Visibility normalization time in `AI.cpp:83`: `s_VisNormtime = 0.2f`.
- Doom-to-meter factor in `AI.cpp:97`: `0.0254f`.

Detection pseudocode:

```cpp
// Active player scan, simplified from idAI::PerformVisualScan().
if (visualAcuity <= 0 || !inPlayerPVS(ai) || ignoreAlerts)
    return no_detect;
if (player.notarget || player.invisible || player.health <= 0)
    return no_detect;

if (!CheckFOV(player.eye) && !CheckFOV(player.origin))
    return no_detect;

// CanSeeExt(..., ignoreLighting=true, ignoreFOV=true) is an occlusion pass here.
if (!CanSeeExt(player, /*useLighting*/ false, /*useFOV*/ false))
    return no_detect;

visibility = GetVisibility(player); // lightgem * acuity, distance falloff.
if (visibility <= 0)
    return no_detect;

alertInc = visionDifficultyFactor * visibility;
newAlert = currentAlert + alertInc;

if (newAlert >= combatThreshold &&
    pathToPlayerReachable &&
    distanceToPlayer > tdm_ai_sight_combat_cutoff)
    newAlert = combatThreshold - 0.1f;

Event_AlertAI("vis", alertInc, player);
```

Multi-point LOS pseudocode:

```cpp
// Simplified from idActor::CanSee(actor).
points = {
    target.GetEyePosition(),
    target.GetPhysics()->GetOrigin(),
    target.origin + 0.7 * (eye - origin) + shoulderOffset,
    target.origin + 0.7 * (eye - origin) - shoulderOffset,
};

for (point : points) {
    if (useFOV && !CheckFOV(point))
        continue;
    tr = TracePoint(selfEye, point, MASK_OPAQUE);
    if (tr.fraction >= 1.0f || traceHitIsTargetActor(tr, target))
        return true;
}
return false;
```

Failure policy:

- Fail closed for `notarget`, invisible, dead player, missing PVS, zero visual
  acuity, failed FOV, failed trace, or zero light/distance visibility.
- General actor `CanSee` accepts if any sampled point is visible; otherwise it
  fails closed.
- Darkness hiding has a localized fail-open policy: if the light-awareness system
  is unavailable, `IsEntityHiddenByDarkness` returns false, so darkness does not
  suppress sight.
- Combat escalation is capped below combat when sight exists but the AI has a
  reachable path and the player is farther than `tdm_ai_sight_combat_cutoff`.

Plan impact:

- P3d should not harden a single eye-to-eye ray as the final sight model. TDM's
  active scan uses eye/origin FOV gating and general visibility has multi-point
  actor samples.
- The R-ruling on verticality should be revised toward explicit vertical FOV for
  AI sight. TDM's base actor FOV is vertically loose, but AI sight overrides it
  with a vertical head-joint gate.
- Fail-closed is the right default for missing occlusion/sight data; the only
  fail-open exception found here is darkness-system absence.

### Alert Dynamics

Primary file pointers:

- `/Users/kogaryu/game-references/darkmod_src-trunk/game/ai/AI.cpp:1717`
  initializes alert thresholds.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/ai/AI.cpp:1726`
  initializes alert grace windows.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/ai/AI.cpp:1739`
  initializes decay times.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/ai/AI.cpp:1794`
  initializes alert-type weights.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/ai/AI.cpp:9746`
  suppresses weaker audio alerts in the same frame.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/ai/AI.cpp:9753`
  dedupes noisemakers already heard.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/ai/AI.cpp:9857`
  remembers noisemakers that caused searching or higher.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/ai/AI.cpp:9980`
  applies `Event_AlertAI`.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/ai/AI.cpp:10073`
  clamps and maps alert levels.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/ai/States/State.cpp:147`
  decays alert after a dead-time following the last rise.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/ai/States/ObservantState.cpp:102`
  computes Observant decay rate.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/ai/States/SuspiciousState.cpp:124`
  computes Suspicious decay rate.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/ai/States/SearchingState.cpp:124`
  computes Searching decay rate.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/ai/States/AgitatedSearchingState.cpp:105`
  computes AgitatedSearching decay rate.

Shipped constants and defaults:

- Alert thresholds: Relaxed below `1.5`, Observant `1.5`, Suspicious `6`,
  Searching `10`, AgitatedSearching `18`, Combat `23`.
- Grace times by band: `2`, `2`, `3`, `2` seconds.
- Grace fractions by band: `1.2`, `1.2`, `1`, `1.0`.
- Grace counts by band: `5`, `5`, `4`, `4`.
- Decay times by band: `5`, `8`, `25`, `65` seconds, with fuzz `1.5`, `2`,
  `8`, `20`.
- Alert decrease dead-time default in `State.cpp:126`: `300` ms.
- Audio threshold defaults to sound-propagation global default threshold
  `20.0`.
- Hearing menu multipliers in `SysCvar.cpp:550`: nearly deaf `0.2`, forgiving
  `0.6`, challenging `1.0`, hardcore `1.5`.
- High-value alert weights include enemy/projectile/dead/unconscious `40`,
  blinded `36`, rope `34`, suspicious item `33`, blood `30`, broken item `26`,
  missing item `25`, weapon `24`, open door `20`, light `10`, suspicious visual
  `8`, suspicious audio `5`.

Alert pseudocode:

```cpp
// Simplified from Event_AlertAI and SetAlertLevel.
if (ignoreAlerts || currentStateIgnoresAlertType(type))
    return;

if (graceActive && type != "vis") {
    graceThreshold = previousAlertBandBase * graceFraction;
    if (sameAlertActor && amount < graceThreshold && graceCountLeft > 0) {
        graceCountLeft--;
        return; // swallowed repeated small non-visual alert
    }
}

AI_AlertLevel += amount;
AI_AlertLevel = clamp(AI_AlertLevel, 0, combatThreshold * 2);

if (!canSearch && AI_AlertLevel >= searchingThreshold)
    AI_AlertLevel = searchingThreshold - 0.1f;

if (AI_AlertLevel >= combatThreshold) {
    if (enemy != null)
        state = Combat;
    else {
        state = AgitatedSearching;
        AI_AlertLevel = combatThreshold - 0.01f;
    }
}
```

Decay pseudocode:

```cpp
if (now >= memory.lastAlertRiseTime + memory.deadTimeAfterAlertRise &&
    AI_AlertLevel > 0) {
    AI_AlertLevel -= state.alertLevelDecreaseRate * secondsSinceLastThink;
}
```

Sustained-sound answer:

- TDM sound reaction is event-based. A propagated sound calls `HearSound`, which
  converts propagated volume over threshold into psychological loudness and then
  calls `PreAlertAI("aud", amount, origin)`.
- Repeated accepted sound events can ratchet alert.
- Very close repeated alerts can be swallowed by grace logic if they are
  non-visual, same actor, under the grace threshold, and under the count limit.
- Noisemakers have a stronger dedupe: once a noisemaker causes Searching or
  higher, it is remembered and later instances are ignored for its active
  duration, default `17` seconds.
- A sound-only alert without an enemy cannot enter Combat. `SetAlertLevel` clamps
  no-enemy combat to AgitatedSearching at `thresh_5 - 0.01`.
- I did not find a generic "continuous sound plateaus" or "habituates over time"
  rule for arbitrary looped sounds.

Failure policy:

- Visual alerts skip grace.
- Non-visual alerts can be swallowed by grace.
- Only the strongest audio alert in a frame is accepted.
- Audio alerts can be ignored by AI state, deafness/zero auditory acuity, or
  disabled audio-alert permissions.
- Combat requires an enemy object, not just a high scalar alert value.

Plan impact:

- P5 should fix sustained-noise bugs with an explicit policy. TDM does not give a
  generic habituation rule; it gives event ratchet plus grace/noisemaker dedupe.
- If we choose habituation, it should be an intentional design improvement, not
  treated as directly TDM-derived.
- The R7 cap is supported: non-enemy audio should top out below combat.

### Search

Primary file pointers:

- `/Users/kogaryu/game-references/darkmod_src-trunk/game/ai/game/SearchManager.h:57`
  stores search origin, limits, hiding spots, guard spots, and assignments.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/ai/game/SearchManager.h:98`
  defines search-role flags and masks.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/ai/game/SearchManager.cpp:26`
  defines search constants.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/ai/game/SearchManager.cpp:64`
  starts a search from `memory.alertPos`.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/ai/game/SearchManager.cpp:708`
  pulls hiding spots incrementally per frame.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/ai/game/SearchManager.cpp:1644`
  starts the hiding-spot finder.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/ai/game/SearchManager.cpp:2170`
  coordinates active searchers, guards, and observers.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/darkmodAAS/DarkmodAASHidingSpotFinder.cpp:26`
  defines hiding-spot constants.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/darkmodAAS/DarkmodAASHidingSpotFinder.cpp:645`
  scans AAS areas on a grid.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/darkmodAAS/DarkmodAASHidingSpotFinder.cpp:850`
  evaluates hiding spot quality using occlusion and darkness.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/ai/States/SearchingState.cpp:458`
  consumes hiding spots or generated spots by search type.

Shipped constants and defaults:

- `SEARCH_RADIUS_ONE_SEARCHER = 126.0`.
- `SEARCH_MAX_GUARD_SPOT_DISTANCE = 500.0`.
- `SEARCH_MIN_OBS_DISTANCE = 200.0`.
- `MIN_SEARCH_RADIUS = 100.0`.
- `MAX_SEARCH_TIME = 200.0`.
- `HIDE_GRID_SPACING = 40.0`.
- `OCCLUSION_HIDING_SPOT_QUALITY = 1.0`.
- `HIDING_SPOT_COMBINATION_DISTANCE = 100.0`.
- `MAX_AREAS_PER_PASS = 20`.
- `WALL_MARGIN_SIZE = 1.0`.
- `tdm_ai_hiding_spot_max_light_quotient = 2.0`.
- `tdm_ai_max_hiding_spot_tests_per_frame = 10`.
- `DELAY_RANDOM_SPOT_GEN = 3000` ms.
- Current search type cvar default inspected in `SysCvar.cpp:72` is
  `tdm_ai_search_type = 4`; in that mode, generated radial search spots are used
  rather than the precomputed hiding-spot list.

Hiding-spot pseudocode:

```cpp
search.origin = memory.alertPos;
search.limits = clampToSearchVolumeAndFloor(search.origin);

for (aasArea in searchBounds) {
    for (point in grid(aasArea.bounds, HIDE_GRID_SPACING)) {
        if (!insideSearchLimits(point))
            continue;

        quality = 0;
        if (occludedFromObserver(point, hidingHeight))
            quality = max(quality, OCCLUSION_HIDING_SPOT_QUALITY);

        lightQuotient = queryLightingAlongLine(point, point + up * hidingHeight);
        if (lightQuotient >= 0 &&
            lightQuotient < tdm_ai_hiding_spot_max_light_quotient) {
            darknessQuality =
                (maxLightQuotient - lightQuotient) / maxLightQuotient * 2;
            quality = max(quality, darknessQuality);
        }

        quality *= square((searchRadius - distance(point, origin)) / searchRadius);
        insertOrderedByQuality(point, clamp01(quality));
    }
}
```

Search assignment pseudocode:

```cpp
while (searchActive) {
    advanceHidingSpotFinder(maxTestsPerFrame);

    if (searchType < 3) {
        spot = nextReachableUnusedHidingSpotForSearcher();
    } else {
        spot = generatedRadialSpot(searchAgeOrAlertDrop);
    }

    if (spot.found)
        assignSearcherToSpot(spot);
    else if (randomFallbackReady)
        assignRandomNearbyInvestigation();
    else
        stopReactingOrLeaveSearchState();

    assignOrRecycleGuardsAndObservers();
}
```

Failure policy:

- Hiding-spot search is incremental; callers wait at least one frame after start.
- Hiding spots are skipped if already used, outside assignment limits, not in AAS,
  or unreachable by path.
- If hiding spots run out, TDM can generate random/radial fallback spots.
- Search state exits when alert decays below the state's threshold.

Plan impact:

- P6d tactic #1 should use the hiding-spot shape, but the plan should note that
  current TDM defaults rely on generated radial search. Hiding-spot search is
  proven tech, not necessarily the current default behavior.
- Search coordination should keep separate roles for searcher, guard, and
  observer; TDM has a manager-level assignment object rather than every guard
  independently choosing the next point.

### Sound

Primary file pointers:

- `/Users/kogaryu/game-references/darkmod_src-trunk/game/SndProp.cpp:430`
  starts sound propagation from a sound shader and soundprop def.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/SndPropLoader.cpp:214`
  reads soundprop globals and falls back to defaults if missing.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/SndPropLoader.cpp:246`
  defines default soundprop constants.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/SndProp.cpp:900`
  computes initial portal expansion losses.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/SndProp.cpp:969`
  runs the portal flood/wavefront.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/SndProp.cpp:1132`
  processes same-area AI.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/SndProp.cpp:1204`
  selects the apparent origin/path for AI hearing.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/ai/AI.cpp:9678`
  maps SPL to loudness.
- `/Users/kogaryu/game-references/darkmod_src-trunk/game/ai/AI.cpp:9697`
  applies hearing and alert conversion.
- `/Users/kogaryu/game-references/darkmod_src-trunk/sound/snd_world.cpp:823`
  contains lower-level portal-occlusion sound-origin resolution.

Shipped constants and defaults:

- `MaxPaths = 3`.
- `DoorExpand = 1.0`.
- `Falloff_Outd = 10.0`.
- `Falloff_Ind = 9.0`.
- `kappa0 = 0.015`.
- `DefaultDoorLoss = 20.0`.
- `MaxRange = 2.2`.
- `MaxRangeCalVol = 30`.
- `MaxEnvRange = 50`.
- `DefaultThreshold = 20.0`.
- `s_MAX_FLOODNODES = 200`.
- Soundprop `alert_factor` default `1`.
- Soundprop `alert_max` default `30`.
- Audio-engine portal trace depth in `snd_world.cpp`: `MAX_PORTAL_TRACE_DEPTH = 10`.

Sound propagation pseudocode:

```cpp
def = lookup("sprGS_" + soundName);
if (!def)
    return no_sound_alert;

vol0 = soundVolume + volMod + areaVolumeOffset + cv_ai_sndvol;
range = pow2((vol0 - MaxRangeCalVol) / 7) * MaxRange;

queue = portalsReachableFromSoundOrigin(vol0, minAIThreshold);
while (!queue.empty() && floodNodes < s_MAX_FLOODNODES) {
    node = queue.popLeastLoss();
    if (node.volumeBelowMinimum)
        continue;
    if (visitedWithBetterLoss(node.area))
        continue;

    processAIInArea(node.area, node.propagatedVolume, apparentOriginViaPortalPath);
    enqueueNeighborPortalsWithAreaLossAndPortalAILoss(node);
}
```

Hearing pseudocode:

```cpp
if (ignoreAlerts || auditoryAcuity <= 0)
    return;

if (propagatedVolume <= auditoryThreshold)
    return;

psychLoud = 1 + (propagatedVolume - auditoryThreshold) * alertFactor;
psychLoud = min(psychLoud, alertMax);
psychLoud *= auditoryAcuity;

if (pathToRealSoundOriginReachable)
    alertPos = realSoundOrigin;
else
    alertPos = apparentPortalOrigin;

PreAlertAI("aud", psychLoud, alertPos);
```

Failure policy:

- Missing soundprop def means no propagation.
- Missing global soundprop def logs a warning and uses defaults.
- Invalid portals or over-budget flood nodes stop expansion rather than inventing
  a direct ray result.
- If the real sound origin is unreachable, AI still reacts to the apparent portal
  origin when the propagated sound is audible.

Plan impact:

- P5/P6 sound should not model walls as a one-ray binary obstruction. TDM's
  useful primitive is a portal/area graph with accumulated area and portal loss.
- Apparent-origin fallback is important: unreachable true origin should become a
  search target near the last portal/path, not a silent failure.

## Brief 2 - OpenXCom

Snapshot: `/Users/kogaryu/game-references/OpenXcom-master`, version marker
`OPENXCOM_VERSION_SHORT "1.0"` and `OPENXCOM_VERSION_LONG "1.0.0.0"` in
`src/version.h:21`.

Primary file pointers:

- `/Users/kogaryu/game-references/OpenXcom-master/src/Battlescape/TileEngine.h:41`
  defines sight constants.
- `/Users/kogaryu/game-references/OpenXcom-master/src/Battlescape/TileEngine.cpp:49`
  defines sampled height offsets.
- `/Users/kogaryu/game-references/OpenXcom-master/src/Battlescape/TileEngine.cpp:228`
  computes FOV and visible tiles/units.
- `/Users/kogaryu/game-references/OpenXcom-master/src/Battlescape/TileEngine.cpp:362`
  computes the unit sight origin voxel.
- `/Users/kogaryu/game-references/OpenXcom-master/src/Battlescape/TileEngine.cpp:393`
  checks unit visibility and smoke accumulation.
- `/Users/kogaryu/game-references/OpenXcom-master/src/Battlescape/TileEngine.cpp:460`
  samples voxel exposure.
- `/Users/kogaryu/game-references/OpenXcom-master/src/Battlescape/TileEngine.cpp:539`
  tests multiple target slices/heights in `canTargetUnit`.
- `/Users/kogaryu/game-references/OpenXcom-master/src/Battlescape/TileEngine.cpp:2225`
  traces voxel lines.
- `/Users/kogaryu/game-references/OpenXcom-master/src/Battlescape/TileEngine.cpp:2525`
  checks terrain/unit voxels.
- `/Users/kogaryu/game-references/OpenXcom-master/src/Savegame/BattleUnit.cpp:2125`
  returns kneel or stand height.
- `/Users/kogaryu/game-references/OpenXcom-master/bin/standard/xcom1/soldiers.rul:42`
  gives soldier `standHeight: 22`, `kneelHeight: 14`.

Shipped constants and defaults:

- `MAX_VIEW_DISTANCE = 20` tiles.
- `MAX_VOXEL_VIEW_DISTANCE = 20 * 16 = 320` voxels.
- `MAX_DARKNESS_TO_SEE_UNITS = 9`.
- Height offsets from center: `0, -2, +2, -4, +4, -6, +6, -8, +8, -12, +12`.
- X-COM soldier standing height: `22`.
- X-COM soldier kneeling height: `14`.
- Smoke tile value clamps in `Tile.cpp:713` to `1..15`.

Sight-origin pseudocode:

```cpp
origin = tileOrigin(unit.position);
origin.x += 8;
origin.y += 8;
origin.z += unit.getHeight() + unit.getFloatHeight() - terrainLevel - 1;

if (unit.isLarge()) {
    origin.x += 8;
    origin.y += 8;
    origin.z += 1;
}

if (floorAboveExists && origin.z would clip)
    origin.z = belowFloorAbove;
```

Visibility pseudocode:

```cpp
if (!tile || !tile.unit)
    return false;

if (viewer.isPlayer()) {
    if (distance > MAX_DARKNESS_TO_SEE_UNITS && tile.shade > MAX_DARKNESS_TO_SEE_UNITS)
        return false;
    if (distance > MAX_VIEW_DISTANCE)
        return false;
}

if (sameFaction)
    return true;

origin = getSightOriginVoxel(viewer);
if (!canTargetUnit(origin, target, scanVoxel))
    return false;

trajectory = calculateLine(origin, scanVoxel, doVoxelCheck=true);
visibleDistance = trajectory.size();
for (voxel in trajectory) {
    tile = voxel.tile;
    if (!tile.onFire())
        visibleDistance += tile.smoke / 3;
    if (visibleDistance > MAX_VOXEL_VIEW_DISTANCE)
        return false;
}
return true;
```

Failure policy:

- Missing tiles/units, excessive darkness, excessive range, and blocking voxels
  fail closed.
- Friendly units are always visible.
- Smoke is graduated occlusion: it consumes remaining visibility budget along the
  path rather than making a tile simply blocked.
- Target visibility uses multiple slices and height offsets, so one blocked
  center ray is not the whole answer.

Plan impact:

- P3d should introduce a stance-aware eye socket. OpenXCom's `getHeight()` is a
  direct precedent for `standingEyeHeight` versus `sneakEyeHeight`.
- A future occlusion table should have room for graduated material or atmosphere
  costs, not only `Blocked`.
- Voxel or layered height sampling is useful evidence against flattening guard
  sight to a single 2D sector.

## Brief 3 - Godot

Snapshot: `/Users/kogaryu/game-references/godot-master`, version marker in
`version.py:1`: Godot Engine `4.7.0 rc`.

### Debug Geometry Lane

Primary file pointers:

- `/Users/kogaryu/game-references/godot-master/scene/3d/navigation/navigation_region_3d.cpp:31`
  includes `NavigationServer3D` and `RenderingServer`.
- `/Users/kogaryu/game-references/godot-master/scene/3d/navigation/navigation_region_3d.cpp:430`
  creates the navigation region RID and hooks debug signals.
- `/Users/kogaryu/game-references/godot-master/scene/3d/navigation/navigation_region_3d.cpp:378`
  attaches the region to the world's navigation map on tree entry.
- `/Users/kogaryu/game-references/godot-master/scene/3d/navigation/navigation_region_3d.cpp:413`
  pushes transform changes to NavigationServer and RenderingServer debug
  instances.
- `/Users/kogaryu/game-references/godot-master/scene/3d/navigation/navigation_region_3d.cpp:482`
  builds/hides debug mesh instances.
- `/Users/kogaryu/game-references/godot-master/scene/3d/physics/collision_object_3d.cpp:352`
  gates collision debug visibility on scene-tree debug settings.
- `/Users/kogaryu/game-references/godot-master/scene/3d/physics/collision_object_3d.cpp:379`
  creates rendering-server debug instances for shapes.
- `/Users/kogaryu/game-references/godot-master/servers/navigation_3d/navigation_server_3d.cpp:311`
  defines nav debug colors/toggles.

Debug-lane pseudocode:

```cpp
onEnterTree:
    regionRID = NavigationServer3D::region_create();
    NavigationServer3D::region_set_map(regionRID, world.navigationMap);
    NavigationServer3D::region_set_transform(regionRID, globalTransform);

onNavigationMeshChanged:
    NavigationServer3D::region_set_navigation_mesh(regionRID, mesh);
    if (debugEnabled)
        rebuildRenderingServerDebugMeshFromNavigationMesh();
    else
        hideDebugInstance();

onTransformChanged:
    NavigationServer3D::region_set_transform(regionRID, globalTransform);
    RenderingServer::instance_set_transform(debugInstanceRID, globalTransform);
```

Failure policy:

- Debug off, no tree, no mesh, or no debug instance means hide/free debug
  rendering resources; simulation RIDs continue independently.
- Debug rendering is rebuilt from server/resource snapshots and does not become a
  gameplay dependency.

Plan impact:

- P4 should keep runtime sensing data, app/debug toggles, and renderer instances
  on separate lanes. The runtime should publish immutable or frame-local debug
  facts; the app/render layer owns meshes, colors, and visibility toggles.

### Navigation Build Lifecycle

Primary file pointers:

- `/Users/kogaryu/game-references/godot-master/scene/3d/navigation/navigation_region_3d.cpp:227`
  kicks off a navigation mesh bake.
- `/Users/kogaryu/game-references/godot-master/modules/navigation_3d/3d/godot_navigation_server_3d.cpp:1192`
  validates parse/bake requests before delegating to the generator.
- `/Users/kogaryu/game-references/godot-master/modules/navigation_3d/3d/nav_mesh_generator_3d.cpp:147`
  validates parse source geometry on the main thread.
- `/Users/kogaryu/game-references/godot-master/modules/navigation_3d/3d/nav_mesh_generator_3d.cpp:161`
  performs synchronous bake and handles empty source data.
- `/Users/kogaryu/game-references/godot-master/modules/navigation_3d/3d/nav_mesh_generator_3d.cpp:199`
  performs async bake or falls back when threading is disabled.
- `/Users/kogaryu/game-references/godot-master/modules/navigation_3d/3d/nav_mesh_generator_3d.cpp:281`
  parses root node children or group nodes.
- `/Users/kogaryu/game-references/godot-master/scene/resources/3d/navigation_mesh_source_geometry_data_3d.cpp:99`
  adds mesh triangle data to source geometry.
- `/Users/kogaryu/game-references/godot-master/scene/3d/mesh_instance_3d.cpp:876`
  registers mesh-instance parsing.
- `/Users/kogaryu/game-references/godot-master/scene/3d/physics/static_body_3d.cpp:104`
  registers static-body collision parsing.

Lifecycle pseudocode:

```cpp
NavigationRegion3D::bake_navigation_mesh(onThread):
    require main thread;
    require navigationMesh != null;
    source = NavigationMeshSourceGeometryData3D();
    NavigationServer3D::parse_source_geometry_data(navigationMesh, source, this);
    if (onThread)
        NavigationServer3D::bake_from_source_geometry_data_async(navigationMesh, source, callback);
    else
        NavigationServer3D::bake_from_source_geometry_data(navigationMesh, source, callback);

NavMeshGenerator::parse:
    require main thread, valid root, root inside SceneTree, valid sourceData;
    clear sourceData;
    collect mesh instances and/or static collision shapes by parser callbacks;

NavMeshGenerator::bake:
    if sourceData empty:
        clear mesh;
        emit changed;
        callback();
        return;
    reject duplicate active bake for same mesh;
    run Recast bake;
    emit changed;
```

Failure policy:

- Invalid mesh/root/tree/source data produces errors and no bake.
- Empty source data clears the navigation mesh and still emits change/callback.
- Duplicate active bakes are rejected.
- Runtime parsing of rendering meshes warns because GPU readback blocks; Godot
  recommends collision shapes or procedural data for runtime rebake.

Plan impact:

- P6c world-launch graph hook should treat navigation building as a lifecycle
  stage with explicit source collection, validation, bake, callback, and change
  notification. Empty nav input should be a visible empty graph, not undefined
  behavior.

## Brief 4 - Opportunistic Bundle

### re3-miami - Wanted Decay Policy

Primary file pointers:

- `/Users/kogaryu/game-references/re3-miami/src/core/Wanted.cpp:15`
  defines maximum wanted constants.
- `/Users/kogaryu/game-references/re3-miami/src/core/Wanted.cpp:89`
  sets wanted level chaos thresholds.
- `/Users/kogaryu/game-references/re3-miami/src/core/Wanted.cpp:228`
  maps crimes to chaos values.
- `/Users/kogaryu/game-references/re3-miami/src/core/Wanted.cpp:310`
  maps chaos into wanted bands.
- `/Users/kogaryu/game-references/re3-miami/src/core/Wanted.cpp:399`
  applies decay/pause behavior.
- `/Users/kogaryu/game-references/re3-miami/src/core/Wanted.cpp:484`
  ages and reports queued crimes.
- `/Users/kogaryu/game-references/re3-miami/src/core/Wanted.cpp:501`
  suspends wanted state.

Shipped constants and defaults:

- Maximum wanted level: `6`.
- Maximum wanted chaos: `9600`.
- Band thresholds: L1 `50`, L2 `180`, L3 `550`, L4 `1200`, L5 `2400`,
  L6 `4800`.
- Decay tick gate: every `1000` ms.
- Crime queue report delay: `500` ms.
- Crime queue expiry: `10000` ms.
- Suspension minimum state expires after `20000` ms.
- Decay requires no police within radius `18` and wanted level at most `1`; above
  level `1`, the timer advances but chaos does not decay.

Pseudocode:

```cpp
if (now - lastWantedUpdate > 1000) {
    if (wantedLevel > 1) {
        lastWantedUpdate = now; // paused decay at high heat
    } else if (policeWithinRadius(player, 18) == 0) {
        chaos = max(0, chaos - 1);
        lastWantedUpdate = now;
    }
}
```

Plan impact:

- P5 can legitimately pause or slow decay while a guard/pursuer has active
  pressure. The pause condition should be explicit and visible in debug output.

### DevilutionX - Pack Leash Semantics

Primary file pointers:

- `/Users/kogaryu/game-references/DevilutionX-master/Source/monster.h:143`
  defines `LeaderRelation::None`, `Leashed`, and `Separated`.
- `/Users/kogaryu/game-references/DevilutionX-master/Source/monster.h:287`
  stores `leader`, `leaderRelation`, and `packSize`.
- `/Users/kogaryu/game-references/DevilutionX-master/Source/monster.cpp:309`
  places a group around a leader, optionally leashed.
- `/Users/kogaryu/game-references/DevilutionX-master/Source/monster.cpp:1482`
  releases leashed minions.
- `/Users/kogaryu/game-references/DevilutionX-master/Source/monster.cpp:1677`
  synchronizes leashed minion activity with leader activity.
- `/Users/kogaryu/game-references/DevilutionX-master/Source/monster.cpp:1697`
  separates or reunites leashed minions based on line solidity and distance.
- `/Users/kogaryu/game-references/DevilutionX-master/Source/monster.cpp:4278`
  keeps visible monsters active and decays activity when not visible.
- `/Users/kogaryu/game-references/DevilutionX-master/Source/monster.cpp:4886`
  sets leader relation.

Shipped constants and defaults:

- Group placement retries: outer `10`, inner `100`.
- Leashed placement requires minion within `< 4` tiles of leader origin in both
  axes.
- Reunite distance: walking distance `< 4`.
- Visible active duration is saturated to `UINT8_MAX`; non-visible non-Diablo
  active ticks decrement by one per update.

Pseudocode:

```cpp
if (relation == Leashed && lineToLeaderIsSolid()) {
    leader.packSize--;
    relation = Separated;
}

if (relation == Separated &&
    lineToLeaderIsClear() &&
    walkingDistanceToLeader < 4) {
    leader.packSize++;
    relation = Leashed;
}

if (relation == Leashed) {
    leader.activeForTicks = max(leader.activeForTicks, minion.activeForTicks - 1);
    minion.activeForTicks = max(minion.activeForTicks, leader.activeForTicks - 1);
}
```

Plan impact:

- Home-leash predicates should be stateful and cheap: clear/solid line plus a
  small reunion radius is enough for the first implementation.
- Pack membership and activity propagation should be separate from pathfinding.

### Warzone 2100 - Shared Awareness and Sensors

Primary file pointers:

- `/Users/kogaryu/game-references/warzone2100-master/src/visibility.h:41`
  documents `visibleObject` as requiring a viewer with a sensor.
- `/Users/kogaryu/game-references/warzone2100-master/src/visibility.h:72`
  provides a fast squared range check.
- `/Users/kogaryu/game-references/warzone2100-master/src/visibility.h:82`
  resolves sensor range, with ECM range overriding sensor range.
- `/Users/kogaryu/game-references/warzone2100-master/src/visibility.cpp:56`
  defines visibility fade rates.
- `/Users/kogaryu/game-references/warzone2100-master/src/visibility.cpp:121`
  updates tile visibility from watcher and sensor counts.
- `/Users/kogaryu/game-references/warzone2100-master/src/visibility.cpp:431`
  updates tiles for an object and excludes unbuilt structures/walls.
- `/Users/kogaryu/game-references/warzone2100-master/src/visibility.cpp:485`
  checks whether one object can see another.
- `/Users/kogaryu/game-references/warzone2100-master/src/visibility.cpp:658`
  forwards seen state to allies with shared vision.
- `/Users/kogaryu/game-references/warzone2100-master/src/visibility.cpp:737`
  iterates unseen grid candidates in sensor range.
- `/Users/kogaryu/game-references/warzone2100-master/src/visibility.cpp:757`
  fades visibility levels up and down.
- `/Users/kogaryu/game-references/warzone2100-master/src/visibility.cpp:865`
  gives radar detectors partial visibility on active radar targets.
- `/Users/kogaryu/game-references/warzone2100-master/src/combat.cpp:387`
  scans all sensors for counter-battery support.

Shipped constants and defaults:

- Visibility increase base: `255 * 2`.
- Visibility decrease base: `50`.
- Full visibility: `UBYTE_MAX`.
- Radar-blip/partial visibility: `UBYTE_MAX / 2`.
- Active radar detector range: `objSensorRange(viewer) * 10`.
- Tile is visible if direct watchers exist, or if sensor watchers exist and
  enemy jamming does not suppress them.

Pseudocode:

```cpp
tileVisible[player] =
    tile.watchers[player] > 0 ||
    (tile.sensors[player] > 0 && !jammedByNonAlly(tile, player));

for (candidate in unseenGridAround(viewer, objSensorRange(viewer))) {
    val = visibleObject(viewer, candidate, wallsBlock=false);
    if (val > 0) {
        for (ally in players) {
            if (hasSharedVision(viewer.player, ally))
                candidate.seenThisTick[ally] = max(candidate.seenThisTick[ally], val);
        }
        triggerSeenEvent(viewer, candidate);
    }
}

for (player in players) {
    if (seenThisTick[player] > visible[player])
        visible[player] += visLevelInc;
    else
        visible[player] -= visLevelDec;
}
```

Failure policy:

- Invalid viewer/target, off-map positions, unbuilt structures, walls, and gates
  return not visible.
- Jammed sensor-only visibility becomes partial radar blip if the tile is sensor
  watched but not directly watched.
- Visibility sharing uses max intensity across allies rather than duplicating
  each sensor's local state machine.

Plan impact:

- Multi-guard shared awareness should be an explicit socket: one guard's sensor
  result can raise shared seen state for allies, with intensity and decay handled
  centrally.

### OpenTTD - YAPF Cost Composition

Primary file pointers:

- `/Users/kogaryu/game-references/OpenTTD-master/src/pathfinder/pathfinder_type.h:15`
  defines shared pathfinder constants.
- `/Users/kogaryu/game-references/OpenTTD-master/src/pathfinder/yapf/yapf_road.cpp:37`
  adds slope costs.
- `/Users/kogaryu/game-references/OpenTTD-master/src/pathfinder/yapf/yapf_road.cpp:62`
  composes one-tile costs.
- `/Users/kogaryu/game-references/OpenTTD-master/src/pathfinder/yapf/yapf_road.cpp:113`
  calculates segment cost until a branch/destination/depot/limit.
- `/Users/kogaryu/game-references/OpenTTD-master/src/pathfinder/yapf/yapf_base.hpp:252`
  inserts nodes and replaces open duplicates only when cheaper.

Shipped constants and defaults:

- `YAPF_TILE_LENGTH = 100`.
- `YAPF_TILE_CORNER_LENGTH = 71`.
- `YAPF_INFINITE_PENALTY = 1000 * YAPF_TILE_LENGTH`.
- Path cache segments: `8`.
- Destination limit: `8`.

Pseudocode:

```cpp
cost = baseTileCost;
if (turning)
    cost += curvePenalty;
if (slopeUp)
    cost += slopePenalty;
if (levelCrossing)
    cost += crossingPenalty;
if (roadStopOccupied)
    cost += stopOccupiedPenalty;
if (segmentCost > maxCost)
    rejectPath;

if (openDuplicate && newEstimate < oldEstimate)
    replaceOpenNode;
else if (closedDuplicateWithLowerEstimate)
    assertEstimatorInvariant();
```

Failure policy:

- Impossible paths are rejected; undesirable paths receive additive penalties.
- Closed-node replacement is treated as an invariant violation because the
  estimator should be consistent and edge costs should not be negative.

Plan impact:

- Chokepoint-hold routing should start as additive cost composition: danger,
  crowding, cover, and holding-position value should be penalties/bonuses in a
  bounded table. Use hard blocks only for impossible traversal.

## Direct Plan Fold-Ins

- P3d: add stance-aware sight origin and prepare for multi-point target sampling.
  Source pressure: TDM actor samples plus OpenXCom kneel/stand eye heights.
- P3/R verticality: replace any "horizontal-only is enough" note with "AI sight
  uses explicit vertical FOV; debug should expose vertical misses separately."
- P4: publish sensing/search debug facts from runtime, but build draw resources in
  app/render. Source pressure: Godot RenderingServer debug instances.
- P5: model sound as event ratchet with explicit suppression policy. Recommended
  first cut: strongest event per frame, same-source grace window, optional
  noisemaker-style dedupe, and no combat without target/enemy evidence.
- P5 debug: expose decay pause reason. Source pressure: TDM decay dead-time and
  re3 high-heat/police-nearby pause.
- P6c: world launch graph should validate source data, build or clear nav graph,
  and publish completion. Empty graph is a result, not a crash path.
- P6d: hide-spot tactic should rank candidates by darkness/occlusion/reachability
  and assign through a manager. Searcher/guard/observer roles should not each
  roll their own independent next-point logic.
