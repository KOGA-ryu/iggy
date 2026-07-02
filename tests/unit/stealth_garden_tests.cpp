// s6b — stealth garden: a headless, text-editable testbed that exercises the whole
// stealth loop (patrol + vision cone + LOS occlusion + graded alert) in one place.
//
// Geometry comes from fixtures/rooms/ascii/stealth_garden.iggyroom.txt (edit the grid to
// change walls). Spawns + the guard patrol beat + the exit objective come from
// fixtures/rooms/ascii/stealth_garden.scenario.iggy3d.toml (edit the [[ai_actors]] waypoint
// lines to change the beat). A scenario is a short in-code tape of player commands; adding a
// scenario is a new tape + assertion here — no engine or fixture change.
//
// This is an IN-LANE test harness (fixtures + content loader + runtime/ai), NOT the product
// tape/automation path: it builds the garden collision from the grid itself and drives the
// player via session.submitCommand, touching no src/app/**.

#include "content/FixtureScenarioLoader.hpp"
#include "content/assets/RoomAsset.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/ai/NpcAlertSystem.hpp"
#include "runtime/ai/NpcBehaviorDebugSnapshot.hpp"
#include "runtime/ai/NpcInvestigateSystem.hpp"
#include "runtime/ai/NpcBehaviorProfile.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/session/Session.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

constexpr const char* kGridPath = "fixtures/rooms/ascii/stealth_garden.iggyroom.txt";
constexpr const char* kScenarioPath =
    "fixtures/rooms/ascii/stealth_garden.scenario.iggy3d.toml";

// Cell (col,row) maps to world (col, 0, row); cell size 1 m (must match the scenario .toml).
iggy3d::Vec3 cellToWorld(int col, int row) {
  return iggy3d::Vec3{static_cast<float>(col), 0.0F, static_cast<float>(row)};
}

std::string readFile(const char* path) {
  std::ifstream file(path);
  std::ostringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

struct Garden {
  bool ok = false;
  iggy3d::SpatialSurfaceSet surfaces;
  iggy3d::Vec3 guardSpawn;   // grid 'N'
  iggy3d::Vec3 playerSpawn;  // grid 'P'
  iggy3d::Vec3 exitCell;     // grid 'E'
};

// Read the ascii grid and bake it to collision: one Walkable floor plane over the interior
// plus one Box Blocker per '#' cell (y up to 3 m so walls occlude the 1 m eye ray as well as
// block movement). Also records the N/P/E glyph cells as world coords.
Garden loadGarden() {
  Garden garden;
  const std::string grid = readFile(kGridPath);
  if (grid.empty()) {
    return garden;
  }

  iggy3d::RoomAsset room;
  room.id = "stealth_garden";

  int maxCol = 0;
  int rows = 0;
  std::istringstream input(grid);
  std::string line;
  int row = 0;
  while (std::getline(input, line)) {
    if (line.empty()) {
      continue;
    }
    for (int col = 0; col < static_cast<int>(line.size()); ++col) {
      const char glyph = line[static_cast<std::size_t>(col)];
      maxCol = std::max(maxCol, col);
      if (glyph == '#') {
        const float x0 = static_cast<float>(col) - 0.5F;
        const float x1 = static_cast<float>(col) + 0.5F;
        const float z0 = static_cast<float>(row) - 0.5F;
        const float z1 = static_cast<float>(row) + 0.5F;
        iggy3d::RoomSpatialSurface wall;
        wall.id = "wall_" + std::to_string(col) + "_" + std::to_string(row);
        wall.sourceStaticMeshId = wall.id;
        wall.shape = iggy3d::RoomSpatialSurfaceShape::Box;
        wall.role = iggy3d::RoomSpatialSurfaceRole::Blocker;
        wall.pointsMeters = {{x0, 0.0F, z0}, {x1, 0.0F, z0}, {x1, 3.0F, z1}, {x0, 3.0F, z1}};
        wall.normal = {0.0F, 0.0F, 1.0F};
        wall.collisionMask = {"actor"};
        wall.blocksActor = true;
        room.spatialSurfaces.push_back(std::move(wall));
      } else if (glyph == 'N') {
        garden.guardSpawn = cellToWorld(col, row);
      } else if (glyph == 'P') {
        garden.playerSpawn = cellToWorld(col, row);
      } else if (glyph == 'E') {
        garden.exitCell = cellToWorld(col, row);
      }
    }
    ++row;
    rows = row;
  }

  iggy3d::RoomSpatialSurface floor;
  floor.id = "garden_floor";
  floor.sourceStaticMeshId = floor.id;
  floor.shape = iggy3d::RoomSpatialSurfaceShape::Plane;
  floor.role = iggy3d::RoomSpatialSurfaceRole::Walkable;
  const float fx = static_cast<float>(maxCol) + 0.5F;
  const float fz = static_cast<float>(rows) - 0.5F;
  floor.pointsMeters = {{-0.5F, 0.0F, -0.5F}, {fx, 0.0F, -0.5F}, {fx, 0.0F, fz}, {-0.5F, 0.0F, fz}};
  floor.normal = {0.0F, 1.0F, 0.0F};
  floor.collisionMask = {"actor"};
  room.spatialSurfaces.push_back(std::move(floor));

  garden.surfaces = iggy3d::buildSpatialSurfaceSet(room);
  garden.ok = true;
  return garden;
}

struct GardenSession {
  bool ok = false;
  std::optional<iggy3d::Session> session;
  iggy3d::SpatialSurfaceSet surfaces;
  iggy3d::EntityId player;
  iggy3d::EntityId guard;
  iggy3d::EntityId exit;
  iggy3d::Vec3 guardSpawn;
  iggy3d::Vec3 playerSpawn;
  iggy3d::Vec3 exitCell;
};

// Parse the scenario .toml (content lane) into a session, thread in the harness-baked garden
// collision, and assert the guard/player spawns sit on the grid N/P cells (drift guard).
GardenSession makeGardenSession() {
  GardenSession result;
  const Garden garden = loadGarden();
  if (!garden.ok) {
    expect(false, "garden grid loaded");
    return result;
  }

  const iggy3d::ScenarioLoadResult parsed = iggy3d::parseScenarioText(readFile(kScenarioPath));
  if (parsed.status != iggy3d::ScenarioLoadStatus::Ok) {
    expect(false, "garden scenario parsed");
    return result;
  }

  iggy3d::SessionCreateRequest request;
  request.packageId = "iggy3d.stealth_garden";
  request.config = parsed.seed.config;
  request.seed = parsed.seed;
  iggy3d::Result<iggy3d::Session> created = iggy3d::Session::create(request);
  if (created.status != iggy3d::ResultStatus::Ok) {
    expect(false, "garden session created");
    return result;
  }

  result.session.emplace(std::move(created.value));
  result.surfaces = garden.surfaces;

  const iggy3d::WorldState& world = result.session->state().world;
  const iggy3d::EntityState* player = world.findByStableName("player");
  const iggy3d::EntityState* guard = world.findByStableName("guard");
  const iggy3d::EntityState* exit = world.findByStableName("exit_key");
  if (player == nullptr || guard == nullptr || exit == nullptr) {
    expect(false, "garden entities resolved");
    return result;
  }
  result.player = player->id;
  result.guard = guard->id;
  result.exit = exit->id;
  result.guardSpawn = garden.guardSpawn;
  result.playerSpawn = garden.playerSpawn;
  result.exitCell = garden.exitCell;

  // Drift guard: spawns must land on the grid glyph cells.
  const bool spawnsMatch =
      iggy3d::nearlyEqual(guard->transform.position, garden.guardSpawn) &&
      iggy3d::nearlyEqual(player->transform.position, garden.playerSpawn) &&
      iggy3d::nearlyEqual(exit->transform.position, garden.exitCell);
  if (!expect(spawnsMatch, "spawns match grid glyph cells")) {
    return result;
  }

  result.ok = true;
  return result;
}

iggy3d::CommandRecord playerCommand(iggy3d::EntityId player, iggy3d::CommandKind kind) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = player;
  command.kind = kind;
  command.source = iggy3d::CommandSource::LocalPlayer;
  return command;
}

void submitMove(GardenSession& gs, iggy3d::Vec3 point) {
  iggy3d::CommandRecord command = playerCommand(gs.player, iggy3d::CommandKind::Move);
  command.payload.target.hasPoint = true;
  command.payload.target.point = point;
  (void)gs.session->submitCommand(command);
}

void submitWait(GardenSession& gs) {
  (void)gs.session->submitCommand(playerCommand(gs.player, iggy3d::CommandKind::Wait));
}

iggy3d::Vec3 playerPosition(const GardenSession& gs) {
  const iggy3d::EntityState* p = gs.session->state().world.findById(gs.player);
  return p == nullptr ? iggy3d::Vec3{} : p->transform.position;
}

// Step the player toward `target` in per-tick hops no larger than the config move distance
// (a single Move command that reaches beyond it is rejected). Returns false on a stuck tick.
bool walkPlayerTo(GardenSession& gs, iggy3d::Vec3 target, int maxTicks) {
  const float stepMeters = gs.session->state().config.movementDistanceMeters;
  for (int i = 0; i < maxTicks; ++i) {
    const iggy3d::Vec3 pos = playerPosition(gs);
    const float dx = target.x - pos.x;
    const float dz = target.z - pos.z;
    const float dist = std::sqrt(dx * dx + dz * dz);
    if (dist <= 0.2F) {
      return true;
    }
    const float step = std::min(stepMeters, dist);
    submitMove(gs, {pos.x + dx / dist * step, pos.y, pos.z + dz / dist * step});
    if (gs.session->tick(&gs.surfaces).status != iggy3d::ResultStatus::Ok) {
      return false;
    }
  }
  return iggy3d::nearlyEqual(playerPosition(gs), target, 0.3F);
}

void submitInteract(GardenSession& gs, iggy3d::EntityId target) {
  iggy3d::CommandRecord command = playerCommand(gs.player, iggy3d::CommandKind::Interact);
  command.payload.target.hasEntity = true;
  command.payload.target.entity = target;
  (void)gs.session->submitCommand(command);
}

const iggy3d::AiActorState* guardAi(const GardenSession& gs) {
  for (const iggy3d::AiActorState& actor : gs.session->state().ai.actors) {
    if (actor.actor == gs.guard) {
      return &actor;
    }
  }
  return nullptr;
}

iggy3d::AiActorState* mutableGuardAi(GardenSession& gs) {
  for (iggy3d::AiActorState& actor : gs.session->mutableStateForOwnedSystems().ai.actors) {
    if (actor.actor == gs.guard) {
      return &actor;
    }
  }
  return nullptr;
}

void teleportEntity(GardenSession& gs, iggy3d::EntityId id, iggy3d::Vec3 pos) {
  iggy3d::WorldState& world = gs.session->mutableStateForOwnedSystems().world;
  const iggy3d::EntityState* entity = world.findById(id);
  if (entity != nullptr) {
    iggy3d::EntityState copy = *entity;
    copy.transform.position = pos;
    (void)world.upsertEntity(copy);
  }
}

// Guard's alert rung read through the NPC behavior debug snapshot (the in-game read-out).
iggy3d::AiBehaviorKind guardBehaviorViaSnapshot(const GardenSession& gs) {
  const iggy3d::NpcBehaviorProfileCatalog catalog = iggy3d::makeBuiltInNpcBehaviorProfileCatalog();
  iggy3d::NpcBehaviorDebugSnapshotRequest request;
  request.enabled = true;
  request.world = &gs.session->state().world;
  request.ai = &gs.session->state().ai;
  request.combat = &gs.session->state().combat;
  request.profileCatalog = &catalog;
  request.sourceTick = gs.session->state().clock.tickIndex;
  const iggy3d::NpcBehaviorDebugSnapshot snapshot = iggy3d::buildNpcBehaviorDebugSnapshot(request);
  for (const iggy3d::NpcBehaviorDebugActorRow& row : snapshot.actors) {
    if (row.actor == gs.guard) {
      return row.behavior;
    }
  }
  return iggy3d::AiBehaviorKind::None;
}

float guardAlertLevel(const GardenSession& gs) {
  const iggy3d::AiActorState* ai = guardAi(gs);
  return ai == nullptr ? -1.0F : ai->alertLevel;
}

std::uint8_t bandRank(iggy3d::AiBehaviorKind behavior) {
  // Alert ladder rung ordering (low -> high), independent of enum numeric value.
  switch (behavior) {
    case iggy3d::AiBehaviorKind::Observant: return 1;
    case iggy3d::AiBehaviorKind::Suspicious: return 2;
    case iggy3d::AiBehaviorKind::Searching: return 3;
    case iggy3d::AiBehaviorKind::Alert: return 4;
    case iggy3d::AiBehaviorKind::Chasing:
    case iggy3d::AiBehaviorKind::Attacking: return 5;
    default: return 0;  // None / Idle / Defeated / Returning
  }
}

bool tick(GardenSession& gs) {
  return gs.session->tick(&gs.surfaces).status == iggy3d::ResultStatus::Ok;
}

// --- Scenario 1: sneak-unseen ------------------------------------------------------------
// The player slips up the far (east) side to the exit while the guard patrols the west ring,
// staying outside the guard's 6 m perception radius the whole way. Assert the guard never
// climbs past Observant and the run ends in Victory.
bool sneakUnseenReachesExitWithoutAlarm() {
  GardenSession gs = makeGardenSession();
  if (!gs.ok) {
    return false;
  }

  const iggy3d::AlertProfile profile;  // matches melee_training's default alert tuning
  bool ok = true;
  bool everSuspicious = false;

  // Tape: hug the east column (x=12) up to the exit, then interact. Each move step is <= the
  // player's per-tick movement distance so one command == one clean step.
  const iggy3d::Vec3 path[] = {gs.playerSpawn, cellToWorld(12, 3), gs.exitCell, gs.exitCell};
  for (const iggy3d::Vec3& point : path) {
    submitMove(gs, point);
    ok = ok && expect(tick(gs), "sneak move tick ok");
    if (guardAlertLevel(gs) >= profile.suspiciousNorm ||
        bandRank(guardBehaviorViaSnapshot(gs)) >= 2) {
      everSuspicious = true;
    }
  }

  // Collect the exit item -> objective completes -> Victory.
  submitInteract(gs, gs.exit);
  ok = ok && expect(tick(gs), "sneak interact tick ok");
  for (int i = 0; i < 3; ++i) {
    submitWait(gs);
    ok = ok && expect(tick(gs), "sneak settle tick ok");
    if (guardAlertLevel(gs) >= profile.suspiciousNorm) {
      everSuspicious = true;
    }
  }

  ok = ok && expect(!everSuspicious, "guard never crosses the suspicious rung while sneaking") &&
       expect(gs.session->state().outcome == iggy3d::SessionOutcome::Victory,
              "sneak run ends in victory");
  return ok;
}

// --- Scenario 2: spotted -----------------------------------------------------------------
// The player steps into the guard's patrol lane on the west side and lingers in its cone.
// Assert the alert ladder climbs through the rungs and reaches Chasing within a bounded cap.
bool spottedGuardEscalatesToChasing() {
  GardenSession gs = makeGardenSession();
  if (!gs.ok) {
    return false;
  }

  // Walk to a cell in front of the guard's north-south patrol leg (x=1), so the guard sees
  // the player on each sweep toward it (dead-time bridges the away sweep).
  bool ok = expect(walkPlayerTo(gs, cellToWorld(1, 6), 20), "spotted reached the watch post");

  // Linger in view: the patrolling guard glimpses the player on each sweep toward it, alert
  // accumulates (dead-time bridges the away sweeps), and once it crosses Suspicious the s5
  // escalation overrides patrol and the guard locks on and climbs to Chasing.
  bool sawObservant = false;
  bool sawSuspicious = false;
  bool sawSearching = false;
  bool sawAlert = false;
  bool reachedChasing = false;
  bool alertNeverReset = true;  // once climbing, never falls all the way back to calm
  constexpr int kCap = 300;
  for (int i = 0; i < kCap && !reachedChasing; ++i) {
    submitWait(gs);  // linger; player Wait also keeps the clock advancing
    ok = ok && expect(tick(gs), "spotted linger tick ok");
    switch (bandRank(guardBehaviorViaSnapshot(gs))) {
      case 1: sawObservant = true; break;
      case 2: sawSuspicious = true; break;
      case 3: sawSearching = true; break;
      case 4: sawAlert = true; break;
      case 5: reachedChasing = true; break;
      default: break;  // rank 0 (Idle between glimpses) is expected before lock-on
    }
    if (sawSuspicious && guardAlertLevel(gs) <= 0.0F) {
      alertNeverReset = false;
    }
  }

  ok = ok && expect(sawObservant, "spotted passed through observant") &&
       expect(sawSuspicious, "spotted passed through suspicious") &&
       expect(sawSearching, "spotted passed through searching") &&
       expect(sawAlert, "spotted passed through alert") &&
       expect(reachedChasing, "spotted reached chasing within cap") &&
       expect(alertNeverReset, "alert did not reset to calm mid-climb");
  return ok;
}

// --- Guard laps the island ring (s6c) ----------------------------------------------------
// A rectangular patrol loop in the walkable ring AROUND the island (diagonal corners). Before
// the s6c fix the guard stalled at a corner; now it must visit all four corners in order and
// complete a full lap. The player is parked far outside so the guard stays low-alert and
// patrols the whole time. This is why s6b shipped a straight beat — s6c makes rings reliable.
bool gardenGuardLapsIslandRing() {
  GardenSession gs = makeGardenSession();
  if (!gs.ok) {
    return false;
  }

  // Rectangular ring between the border and the island (rows 2 & 5, cols 2 & 11 are all floor;
  // the island occupies cols 4-9, rows 3-4, i.e. INSIDE this loop).
  const iggy3d::Vec3 ring[] = {cellToWorld(2, 2), cellToWorld(11, 2), cellToWorld(11, 5),
                               cellToWorld(2, 5)};
  for (iggy3d::AiActorState& ai : gs.session->mutableStateForOwnedSystems().ai.actors) {
    if (ai.actor == gs.guard) {
      ai.patrolWaypoints = {ring[0], ring[1], ring[2], ring[3]};
      ai.patrolTargetIndex = 0;
    }
  }
  // Park the player far outside the garden so it is never perceived (guard stays patrolling).
  iggy3d::WorldState& world = gs.session->mutableStateForOwnedSystems().world;
  const iggy3d::EntityState* p = world.findById(gs.player);
  if (p != nullptr) {
    iggy3d::EntityState copy = *p;
    copy.transform.position = {50.0F, 0.0F, 50.0F};
    (void)world.upsertEntity(copy);
  }

  bool ok = true;
  bool visited[4] = {false, false, false, false};
  int lastIndex = 0;
  bool inOrder = true;
  bool completedLap = false;
  // Perimeter ~24 m at ~1 m/tick + spawn approach; a lap is ~26 ticks. Give ~2 laps of slack.
  for (int i = 0; i < 70 && !completedLap; ++i) {
    ok = ok && expect(tick(gs), "ring lap tick ok");
    const iggy3d::AiActorState* ai = guardAi(gs);
    if (ai == nullptr) {
      return expect(false, "ring lap guard present");
    }
    ok = ok && expect(ai->lastIntent == iggy3d::AiIntentKind::Patrol, "ring lap stays patrolling");
    const int idx = static_cast<int>(ai->patrolTargetIndex);
    if (idx != lastIndex) {
      if (idx != (lastIndex + 1) % 4) {
        inOrder = false;
      }
      if (lastIndex == 3 && idx == 0 && visited[0] && visited[1] && visited[2]) {
        completedLap = true;
      }
      lastIndex = idx;
    }
    visited[idx] = true;
  }

  ok = ok && expect(visited[0] && visited[1] && visited[2] && visited[3],
                    "ring lap visits all four corners") &&
       expect(inOrder, "ring lap advances corners in order (no stall)") &&
       expect(completedLap, "ring lap completes a full lap around the island");
  return ok;
}

// --- Break contact: investigate last-known, then give up (s7) -----------------------------
// The guard sees the player just north of the island, then the player ducks below the island so
// LOS breaks. The guard must (a) have recorded the last-known spot, (b) enter Searching and MOVE
// toward it (not reset), (c) dwell there, (d) give up and return to patrol once alert decays.
bool breakContactInvestigatesThenGivesUp() {
  GardenSession gs = makeGardenSession();
  if (!gs.ok) {
    return false;
  }

  // Set the guard just above the island facing south, alert already in the Searching band; put
  // the player one cell south (north of the island, clear LOS) so the guard sees it.
  iggy3d::AiActorState* g = mutableGuardAi(gs);
  if (g == nullptr) {
    return expect(false, "break-contact guard present");
  }
  g->alertLevel = 0.55F;  // Searching band
  g->lastRiseTick = 0;
  const iggy3d::Vec3 seenAt = cellToWorld(6, 2);
  teleportEntity(gs, gs.guard, cellToWorld(6, 1));
  teleportEntity(gs, gs.player, seenAt);

  // (a) See the player -> record last-known.
  submitWait(gs);
  bool ok = expect(tick(gs), "break-contact see tick ok");
  const iggy3d::AiActorState* ai = guardAi(gs);
  ok = ok && expect(ai != nullptr && ai->hasLastKnownTarget, "records last-known while seen") &&
       expect(ai != nullptr && iggy3d::nearlyEqual(ai->lastKnownTargetPosition, seenAt, 0.05F),
              "last-known is where the player was seen");

  // Break contact: the player ducks to the bottom corridor, behind the island.
  teleportEntity(gs, gs.player, cellToWorld(6, 6));

  // (b) Unseen but still Searching -> investigate: LOS is now blocked and the guard MOVES toward
  // the last-known spot (does not reset to patrol).
  bool sawInvestigate = false;
  bool losBrokenWhileInRadius = false;
  for (int i = 0; i < 8; ++i) {
    submitWait(gs);
    ok = ok && expect(tick(gs), "break-contact investigate tick ok");
    ai = guardAi(gs);
    if (ai != nullptr && ai->lastIntent == iggy3d::AiIntentKind::Investigate) {
      sawInvestigate = true;
    }
    if (ai != nullptr && ai->lastTargetInRadius && !ai->lastTargetHasLineOfSight) {
      losBrokenWhileInRadius = true;  // the island is doing the occluding
    }
  }
  const iggy3d::EntityState* guardEntity = gs.session->state().world.findById(gs.guard);
  ok = ok && expect(sawInvestigate, "guard investigates toward last-known after losing sight") &&
       expect(losBrokenWhileInRadius, "island breaks LOS during the search") &&
       expect(guardEntity != nullptr && guardEntity->transform.position.z > 1.5F,
              "guard advanced toward the last-known spot");

  // (c) Dwell/look around at the spot.
  bool sawDwell = false;
  for (int i = 0; i < 20 && !sawDwell; ++i) {
    submitWait(gs);
    ok = ok && expect(tick(gs), "break-contact dwell tick ok");
    ai = guardAi(gs);
    if (ai != nullptr && ai->hasLastKnownTarget && ai->investigateDwellTicks > 0U &&
        ai->lastIntent == iggy3d::AiIntentKind::Wait) {
      sawDwell = true;
    }
  }
  ok = ok && expect(sawDwell, "guard dwells (looks around) at last-known");

  // (d) Give up after the dwell, then return to patrol once alert decays (simulate the decay).
  bool gaveUp = false;
  for (int i = 0; i < static_cast<int>(iggy3d::kInvestigateDwellTicks) + 10 && !gaveUp; ++i) {
    submitWait(gs);
    ok = ok && expect(tick(gs), "break-contact giveup tick ok");
    ai = guardAi(gs);
    if (ai != nullptr && !ai->hasLastKnownTarget) {
      gaveUp = true;
    }
  }
  ok = ok && expect(gaveUp, "guard gives up after the dwell");

  // Alert fully decays with no re-acquire -> patrol resumes.
  g = mutableGuardAi(gs);
  g->alertLevel = 0.0F;
  submitWait(gs);
  ok = ok && expect(tick(gs), "break-contact patrol-resume tick ok");
  ai = guardAi(gs);
  ok = ok && expect(ai != nullptr && ai->lastIntent == iggy3d::AiIntentKind::Patrol,
                    "guard returns to patrol after giving up");
  return ok;
}

// --- Island occlusion (the blind side the testbed is built around) -----------------------
// Place the guard on the top row and the player on the bottom row on the SAME column, with
// the central island between them. Assert the guard has the player in its cone and radius but
// LOS is BLOCKED (occluded) -> not perceived. Contrast a clear column (no island) where LOS
// is open. This is the s6b HARD-STOP check: if the island does not occlude, stop and flag.
bool islandBreaksLineOfSight() {
  GardenSession gs = makeGardenSession();
  if (!gs.ok) {
    return false;
  }

  // White-box place the guard atop the island column (x=6, z=1) facing +z (south, across the
  // island) and the player directly south (x=6, z=6). Freeze the guard's patrol for the probe.
  for (iggy3d::AiActorState& ai : gs.session->mutableStateForOwnedSystems().ai.actors) {
    if (ai.actor == gs.guard) {
      ai.patrolWaypoints.clear();  // no patrol so facing stays as set for this probe
      ai.facingDirection = {0.0F, 0.0F, 1.0F};
    }
  }
  iggy3d::WorldState& world = gs.session->mutableStateForOwnedSystems().world;
  const auto moveEntity = [&world](iggy3d::EntityId id, iggy3d::Vec3 pos) {
    const iggy3d::EntityState* entity = world.findById(id);
    if (entity != nullptr) {
      iggy3d::EntityState copy = *entity;
      copy.transform.position = pos;
      (void)world.upsertEntity(copy);
    }
  };
  moveEntity(gs.guard, cellToWorld(6, 1));
  moveEntity(gs.player, cellToWorld(6, 6));

  submitWait(gs);
  bool ok = expect(tick(gs), "occlusion tick ok");
  const iggy3d::AiActorState* ai = guardAi(gs);
  ok = ok && expect(ai != nullptr, "occlusion guard present");
  if (ai == nullptr) {
    return false;
  }
  // The player is inside the radius and dead-ahead in the cone, but the island wall blocks the
  // eye ray, so the guard cannot see it and does not perceive.
  ok = ok && expect(ai->lastTargetInRadius, "player within guard radius across island") &&
       expect(ai->lastTargetInVisionCone, "player within guard cone across island") &&
       expect(!ai->lastTargetHasLineOfSight, "island occludes guard line of sight") &&
       expect(guardAlertLevel(gs) <= 0.0F, "occluded guard does not raise alert");
  return ok;
}

}  // namespace

int main() {
  const bool ok = islandBreaksLineOfSight() && sneakUnseenReachesExitWithoutAlarm() &&
                  spottedGuardEscalatesToChasing() && gardenGuardLapsIslandRing() &&
                  breakContactInvestigatesThenGivesUp();
  return ok ? 0 : 1;
}
