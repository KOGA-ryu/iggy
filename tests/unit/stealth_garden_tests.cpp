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
#include "runtime/ai/NpcSoundPerception.hpp"
#include "runtime/ai/ReasoningGraph.hpp"
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
#include <utility>
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

float planarDistance(iggy3d::Vec3 a, iggy3d::Vec3 b) {
  const float dx = b.x - a.x;
  const float dz = b.z - a.z;
  return std::sqrt(dx * dx + dz * dz);
}

// Bitwise graph equality (a3s2 post-load pin): ids/kinds/positions/labels + the full edge list.
bool graphsEqual(const iggy3d::ReasoningGraph& a, const iggy3d::ReasoningGraph& b) {
  if (a.nodes.size() != b.nodes.size() || a.edges.size() != b.edges.size()) {
    return false;
  }
  for (std::size_t i = 0; i < a.nodes.size(); ++i) {
    if (a.nodes[i].id != b.nodes[i].id || a.nodes[i].kind != b.nodes[i].kind ||
        a.nodes[i].positionMeters.x != b.nodes[i].positionMeters.x ||
        a.nodes[i].positionMeters.y != b.nodes[i].positionMeters.y ||
        a.nodes[i].positionMeters.z != b.nodes[i].positionMeters.z ||
        a.nodes[i].sourceLabel != b.nodes[i].sourceLabel) {
      return false;
    }
  }
  for (std::size_t i = 0; i < a.edges.size(); ++i) {
    if (a.edges[i].from != b.edges[i].from || a.edges[i].to != b.edges[i].to ||
        a.edges[i].kind != b.edges[i].kind || a.edges[i].lengthMeters != b.edges[i].lengthMeters) {
      return false;
    }
  }
  return true;
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
  iggy3d::RoomAsset room;    // a3s1: the authored room (walls/floor + N/P/E anchors)
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

  // a3s1 fixture growth: author the meaningful positions as room anchors so the L4 reasoning
  // graph has content to derive. The grid tracked these glyph cells above; expose them as the
  // exit + the two entity references (guard 'N', player 'P').
  const auto pushAnchor = [&room](const char* kind, iggy3d::Vec3 pos) {
    iggy3d::RoomAnchorAsset a;
    a.id = std::string("anchor_") + kind;
    a.kind = kind;
    a.positionMeters = pos;
    room.anchors.push_back(std::move(a));
  };
  pushAnchor("exit", garden.exitCell);
  pushAnchor("npc", garden.guardSpawn);
  pushAnchor("spawn", garden.playerSpawn);

  garden.surfaces = iggy3d::buildSpatialSurfaceSet(room);
  garden.room = room;
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

// --- a1s2: sound emission → hearing → investigation ----------------------------------------
// SNEAK INVARIANT (mechanical margin). The sneak tape (sneakUnseenReachesExitWithoutAlarm)
// stays green; here we assert WHY: across every emitting sneak tick the loudest footstep's
// audibility at the (west-patrolling) guard -- computed by the pure kernel with NO wall (the
// conservative upper bound; the island usually occludes it anyway) -- stays a stated margin
// below the guard's hearing threshold, so the guard provably never hears the east-column sneak.
bool sneakFootstepsStayBelowHearingMargin() {
  GardenSession gs = makeGardenSession();
  if (!gs.ok) {
    return false;
  }
  const iggy3d::RuntimeConfig cfg = gs.session->state().config;
  const iggy3d::SoundPerceptionConfig snd;  // reference hearing tuning (builtin profile default)
  constexpr float kMarginDb = 3.0F;         // STATED mechanical margin

  const auto guardPos = [&]() {
    const iggy3d::EntityState* e = gs.session->state().world.findById(gs.guard);
    return e == nullptr ? iggy3d::Vec3{} : e->transform.position;
  };
  const auto audibilityAt = [&](iggy3d::Vec3 origin, float d, iggy3d::Vec3 listener) {
    iggy3d::SoundEvent step;
    step.originMeters = origin;
    step.loudnessDb = cfg.footstepBaseLoudnessDb + cfg.footstepLoudnessPerMeterDb * d;
    step.alertFactor = cfg.footstepAlertFactor;
    step.alertMax = cfg.footstepAlertMaxUnits;
    return iggy3d::soundAudibilityDb(step, listener, snd, /*blockerBetween=*/false);
  };

  bool ok = true;
  bool anyEmitted = false;
  float maxAudibility = -1000.0F;
  const iggy3d::Vec3 path[] = {gs.playerSpawn, cellToWorld(12, 3), gs.exitCell, gs.exitCell};
  for (const iggy3d::Vec3& point : path) {
    const iggy3d::Vec3 prev = playerPosition(gs);
    const iggy3d::Vec3 guardBefore = guardPos();  // guard hears from ~its pre-move position
    submitMove(gs, point);
    ok = ok && expect(tick(gs), "sneak-margin move tick ok");
    const iggy3d::Vec3 origin = playerPosition(gs);
    const float d = planarDistance(prev, origin);
    if (d <= 0.0F) {
      continue;  // a zero-displacement move emits no footstep
    }
    anyEmitted = true;
    // Upper-bound over both the guard's pre- and post-move positions (whichever is louder).
    maxAudibility = std::max(maxAudibility, audibilityAt(origin, d, guardBefore));
    maxAudibility = std::max(maxAudibility, audibilityAt(origin, d, guardPos()));
  }
  ok = ok && expect(anyEmitted, "the sneak run actually emits footsteps");
  ok = ok && expect(maxAudibility <= snd.hearingThresholdDb - kMarginDb,
                    "loudest sneak footstep stays >= 3 dB below the guard's hearing threshold");
  return ok;
}

// CONVERSE (white-box, tuning-coupled -- the ONE quarantine test for a1s2). Freeze the guard at
// a fixed cell facing AWAY (east) so it can never SEE the noise-maker; pace the player right
// behind it (west) emitting footsteps until sustained hearing climbs it into Searching and
// plants the noise origin in last-known memory. Then the player flees far and goes silent, and
// the guard investigates the STALE origin -- moving toward it WITHOUT ever gaining LOS and NEVER
// reaching Chasing (the hasValidTarget=false cap on noise-only alert).
bool heardNoiseSearchesAndInvestigatesButNeverChases() {
  GardenSession gs = makeGardenSession();
  if (!gs.ok) {
    return false;
  }
  const iggy3d::Vec3 guardCell = cellToWorld(3, 3);
  const iggy3d::Vec3 pacerA = cellToWorld(2, 3);      // 1.0 m west of the guard
  const iggy3d::Vec3 pacerB{2.4F, 0.0F, 3.0F};        // 0.6 m west of the guard
  const iggy3d::Vec3 eastFacing{1.0F, 0.0F, 0.0F};    // guard looks AWAY from the noise

  if (iggy3d::AiActorState* g0 = mutableGuardAi(gs)) {
    g0->patrolWaypoints = {guardCell};  // degenerate beat: never patrols into LOS
    g0->patrolTargetIndex = 0;
  }
  teleportEntity(gs, gs.guard, guardCell);
  teleportEntity(gs, gs.player, pacerA);

  const auto guardPos = [&]() {
    const iggy3d::EntityState* e = gs.session->state().world.findById(gs.guard);
    return e == nullptr ? iggy3d::Vec3{} : e->transform.position;
  };
  const auto sawPlayer = [&]() {
    const iggy3d::AiActorState* g = guardAi(gs);
    return g != nullptr && g->lastTargetInVisionCone && g->lastTargetHasLineOfSight;
  };

  bool ok = true;
  bool reachedSearching = false;
  bool everSawPlayer = false;
  std::uint8_t maxBand = 0U;
  // Phase 1: loiter behind the frozen guard until noise alone escalates it into Searching.
  for (int i = 0; i < 400 && !reachedSearching; ++i) {
    if (iggy3d::AiActorState* g = mutableGuardAi(gs)) {
      g->facingDirection = eastFacing;  // this tick's perception uses this facing
    }
    teleportEntity(gs, gs.guard, guardCell);  // freeze position: never wander into LOS
    submitMove(gs, (i % 2 == 0) ? pacerB : pacerA);
    ok = ok && expect(tick(gs), "converse loiter tick ok");
    everSawPlayer = everSawPlayer || sawPlayer();
    const std::uint8_t band = bandRank(guardBehaviorViaSnapshot(gs));
    maxBand = std::max(maxBand, band);
    reachedSearching = band >= 3U;
  }
  ok = ok && expect(reachedSearching, "sustained noise escalates the guard into Searching");
  ok = ok && expect(!everSawPlayer, "guard HEARS but never gains LOS to the noise-maker");

  const iggy3d::AiActorState* gMid = guardAi(gs);
  if (gMid == nullptr) {
    return false;
  }
  ok = ok && expect(gMid->hasLastKnownTarget, "heard-and-unseen plants a last-known origin");
  const iggy3d::Vec3 origin = gMid->lastKnownTargetPosition;
  ok = ok && expect(origin.x < guardCell.x - 0.25F,
                    "recorded origin is the noise to the guard's west, not the guard's own cell");

  // Phase 2: player flees far and goes silent; the guard investigates the stale origin. Measure
  // progress from the FROZEN cell (the guard's Phase-1 anchor); the investigate move enqueued on
  // the last loiter tick executes here and walks it toward the noise.
  teleportEntity(gs, gs.player, cellToWorld(12, 6));
  const float startDist = planarDistance(guardCell, origin);
  float minDist = planarDistance(guardPos(), origin);
  for (int i = 0; i < 40; ++i) {
    submitWait(gs);  // clock stays alive; no player movement -> no new noise
    ok = ok && expect(tick(gs), "converse investigate tick ok");
    minDist = std::min(minDist, planarDistance(guardPos(), origin));
    maxBand = std::max(maxBand, bandRank(guardBehaviorViaSnapshot(gs)));
    everSawPlayer = everSawPlayer || sawPlayer();
  }
  ok = ok && expect(minDist < startDist - 0.1F,
                    "guard investigates -- walks toward the heard origin");
  ok = ok && expect(!everSawPlayer, "guard never gains LOS across the whole converse");
  ok = ok && expect(maxBand < 5U, "noise-only guard never reaches Chasing (band 5)");
  return ok;
}

// --- a3s1: L4 reasoning graph shape pinned on the grown garden fixture -----------------------
// loadGarden now authors exit/npc/spawn anchors; feed the room + the scenario's patrol ring into
// buildReasoningGraph and pin the SHAPE (counts + kinds + key positions + island occlusion), NOT
// float noise. This is the greenfield graph exercised end to end on real garden geometry.
bool reasoningGraphShapeMatchesGarden() {
  const Garden garden = loadGarden();
  if (!expect(garden.ok, "garden loaded for reasoning graph")) {
    return false;
  }
  // The scenario's guard beat (fixtures/rooms/ascii/stealth_garden.scenario.iggy3d.toml waypoint
  // lines): the west leg (1,1) <-> (1,5). Passed as the builder's caller-supplied waypoint input.
  const std::vector<iggy3d::Vec3> waypoints = {cellToWorld(1, 1), cellToWorld(1, 5)};
  const iggy3d::ReasoningGraph g = iggy3d::buildReasoningGraph(garden.room, waypoints);

  const auto find = [&g](iggy3d::ReasoningNodeKind kind, iggy3d::Vec3 pos) -> const iggy3d::ReasoningNode* {
    for (const iggy3d::ReasoningNode& n : g.nodes) {
      if (n.kind == kind && iggy3d::nearlyEqual(n.positionMeters, pos, 0.01F)) {
        return &n;
      }
    }
    return nullptr;
  };
  const auto hasEdge = [&g](const iggy3d::ReasoningNode* a, const iggy3d::ReasoningNode* b) {
    if (a == nullptr || b == nullptr) {
      return false;
    }
    for (const iggy3d::ReasoningEdge& e : g.edges) {
      if ((e.from == a->id && e.to == b->id) || (e.from == b->id && e.to == a->id)) {
        return true;
      }
    }
    return false;
  };

  // Shape: 1 exit + 2 references (N,P) + 2 patrolPosts = 5 nodes.
  int exits = 0;
  int references = 0;
  int posts = 0;
  for (const iggy3d::ReasoningNode& n : g.nodes) {
    if (n.kind == iggy3d::ReasoningNodeKind::exit) ++exits;
    if (n.kind == iggy3d::ReasoningNodeKind::reference) ++references;
    if (n.kind == iggy3d::ReasoningNodeKind::patrolPost) ++posts;
  }
  bool ok = expect(g.nodes.size() == 5U, "garden graph has 5 nodes") &&
            expect(exits == 1 && references == 2 && posts == 2,
                   "1 exit + 2 references (N,P) + 2 patrolPosts");

  const iggy3d::ReasoningNode* exitNode = find(iggy3d::ReasoningNodeKind::exit, cellToWorld(12, 1));
  const iggy3d::ReasoningNode* npcRef = find(iggy3d::ReasoningNodeKind::reference, cellToWorld(1, 1));
  const iggy3d::ReasoningNode* spawnRef = find(iggy3d::ReasoningNodeKind::reference, cellToWorld(12, 6));
  const iggy3d::ReasoningNode* postBottom = find(iggy3d::ReasoningNodeKind::patrolPost, cellToWorld(1, 5));
  ok = ok && expect(exitNode != nullptr, "exit node sits at the E cell (12,1)") &&
       expect(npcRef != nullptr, "reference node at the N cell (1,1)") &&
       expect(spawnRef != nullptr, "reference node at the P cell (12,6)") &&
       expect(postBottom != nullptr, "patrolPost at the bottom waypoint (1,5)");

  // Island occlusion: a top-row node (exit @12,1) to a bottom node (patrolPost @1,5) crosses the
  // island (cols 4-9, rows 3-4) -> NO direct walkable edge. An open top-row pair IS linked.
  ok = ok && expect(!hasEdge(exitNode, postBottom),
                    "no direct edge through the island (exit -> bottom patrolPost)") &&
       expect(hasEdge(exitNode, npcRef),
              "open top-row pair is linked (exit -> N reference across the clear top row)");
  return ok;
}

// a3s2: the session CARRIES the graph (survives ticks) and after a load the slot is a valid EMPTY
// value (transient-in-persistence), re-derivable to the pre-save graph from the same room.
bool reasoningGraphCarriesAcrossTicksAndClearsOnLoad() {
  GardenSession gs = makeGardenSession();
  if (!gs.ok) {
    return false;
  }
  const Garden garden = loadGarden();
  const std::vector<iggy3d::Vec3> waypoints = {cellToWorld(1, 1), cellToWorld(1, 5)};
  const iggy3d::ReasoningGraph built = iggy3d::buildReasoningGraph(garden.room, waypoints);
  gs.session->setReasoningGraph(built);

  bool ok = expect(!built.nodes.empty(), "garden graph is non-empty") &&
            expect(graphsEqual(gs.session->state().reasoningGraph, built),
                   "session carries the graph after setReasoningGraph");

  // The slot lives OUTSIDE transient, so ticking (which clears transient) never touches it.
  for (int i = 0; i < 3; ++i) {
    submitWait(gs);
    ok = ok && expect(tick(gs), "carry tick ok");
  }
  ok = ok && expect(graphsEqual(gs.session->state().reasoningGraph, built),
                    "graph survives ticks unchanged");

  // Post-load: replaceStateFromLoad installs a loaded state whose reasoningGraph is DEFAULT-EMPTY
  // -- the save envelope never carried it (off SaveCodec/SaveEnvelope), so a decoded state has no
  // graph. Model that loaded state and assert the slot becomes valid-empty, not stale/dangling.
  iggy3d::SessionState loaded = gs.session->state();
  loaded.reasoningGraph = {};
  const iggy3d::SessionLoadResult load = gs.session->replaceStateFromLoad(std::move(loaded));
  ok = ok && expect(load.status == iggy3d::SessionLoadStatus::Ok, "replaceStateFromLoad ok") &&
       expect(gs.session->state().reasoningGraph.nodes.empty() &&
                  gs.session->state().reasoningGraph.edges.empty(),
              "post-load the graph slot is a valid EMPTY value");

  // Re-supply the SAME room's freshly-built graph: buildReasoningGraph is deterministic, so it
  // reproduces the pre-save graph bitwise (the honest in-lane round-trip pin).
  const iggy3d::ReasoningGraph rebuilt = iggy3d::buildReasoningGraph(garden.room, waypoints);
  gs.session->setReasoningGraph(rebuilt);
  ok = ok && expect(graphsEqual(gs.session->state().reasoningGraph, built),
                    "re-supplying the same room reproduces the pre-save graph");
  return ok;
}

}  // namespace

int main() {
  const bool ok = islandBreaksLineOfSight() && sneakUnseenReachesExitWithoutAlarm() &&
                  spottedGuardEscalatesToChasing() && gardenGuardLapsIslandRing() &&
                  breakContactInvestigatesThenGivesUp() &&
                  sneakFootstepsStayBelowHearingMargin() &&
                  heardNoiseSearchesAndInvestigatesButNeverChases() &&
                  reasoningGraphShapeMatchesGarden() &&
                  reasoningGraphCarriesAcrossTicksAndClearsOnLoad();
  return ok ? 0 : 1;
}
