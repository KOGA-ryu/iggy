// THE MAP DEMO proof (Leg C). Drives the REAL chain end-to-end, headless:
// template -> document -> prepareCreativePlay (validation + bake + catalog)
// -> activateCreativeRuntimeSandbox (reasoning graph + explicit patrol plans +
// session) -> 2000-tick patrol simulation (twice, position-hashed) ->
// grid flood-fill reachability -> clamber-only bypass geometry pins.
// Also keeps the committed fixture save in sync with the generator.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <cstring>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/play/PlayPreparation.hpp"
#include "app/iggy3d/creative/play/RuntimeSandbox.hpp"
#include "app/iggy3d/creative/world/MapTemplate.hpp"
#include "app/iggy3d/creative/world/WorldService.hpp"
#include "content/assets/StaticMeshAsset.hpp"
#include "core/grid/Reachability.hpp"
#include "runtime/movement/MovementParams.hpp"
#include "runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp"

namespace {

namespace cr = iggy3d::creative;

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

constexpr double kWorldMin = -40.0;
constexpr std::int32_t kGridCells = 80;
constexpr std::uint32_t kSimTicks = 2000U;

// ---- shared chain steps --------------------------------------------------

cr::CreativeMapTemplateResult buildMap() {
  return cr::buildCreativeMapTemplate(cr::kMapDemoTemplateId, 1U);
}

struct ActivatedDemo {
  bool ok = false;
  cr::CreativePlayPreparationResult preparation;
  cr::CreativeRuntimeSandboxActivationReceipt receipt;
  std::optional<cr::CreativeRuntimeSandbox> sandbox;
};

bool samePath(std::span<const cr::CreativePathPoint> left,
              std::span<const cr::CreativePathPoint> right) {
  if (left.size() != right.size()) {
    return false;
  }
  for (std::size_t index = 0U; index < left.size(); ++index) {
    if (left[index].position.x != right[index].position.x ||
        left[index].position.y != right[index].position.y ||
        left[index].position.z != right[index].position.z ||
        left[index].dwellSeconds != right[index].dwellSeconds ||
        left[index].outgoingSpeedMultiplier !=
            right[index].outgoingSpeedMultiplier) {
      return false;
    }
  }
  return true;
}

ActivatedDemo activateDemo(const cr::CreativeDocument& document,
                           const iggy3d::StaticMeshAssetCatalog& catalog) {
  ActivatedDemo result;
  cr::CreativePlayPreparationRequest preparationRequest;
  preparationRequest.document = &document;
  preparationRequest.staticMeshAssetCatalog = &catalog;
  result.preparation = cr::prepareCreativePlay(preparationRequest);
  if (!result.preparation.accepted || !result.preparation.payload) {
    return result;
  }
  cr::CreativeRuntimeSandboxActivationRequest activationRequest;
  activationRequest.payload = *result.preparation.payload;
  activationRequest.sourceDocument = &document;
  cr::CreativeRuntimeSandboxActivationResult activation =
      cr::activateCreativeRuntimeSandbox(std::move(activationRequest));
  result.receipt = activation.receipt;
  result.sandbox = std::move(activation.sandbox);
  result.ok = result.sandbox.has_value();
  return result;
}

// ---- 1. generator determinism + fixture sync ----------------------------

bool generatorIsDeterministic() {
  const cr::CreativeMapTemplateResult first = buildMap();
  const cr::CreativeMapTemplateResult second = buildMap();
  if (!expect(first.accepted && second.accepted, "map_demo template builds")) {
    return false;
  }
  bool identical = first.document.objectCount() == second.document.objectCount();
  const auto& firstObjects = first.document.objects();
  const auto& secondObjects = second.document.objects();
  for (std::size_t i = 0; identical && i < firstObjects.size(); ++i) {
    identical = firstObjects[i].name == secondObjects[i].name &&
                firstObjects[i].kind == secondObjects[i].kind &&
                firstObjects[i].parentId == secondObjects[i].parentId &&
                samePath(firstObjects[i].pathPoints,
                         secondObjects[i].pathPoints) &&
                firstObjects[i].transform.position.x ==
                    secondObjects[i].transform.position.x &&
                firstObjects[i].transform.position.z ==
                    secondObjects[i].transform.position.z;
  }
  return expect(identical, "two template builds are object-identical") &&
         expect(first.document.objectCount() >= 90U,
                "map has expected object volume");
}

bool generatorUsesExplicitPatrolTopology() {
  const cr::CreativeMapTemplateResult map = buildMap();
  if (!expect(map.accepted, "explicit patrol map builds")) {
    return false;
  }
  std::size_t routeCount = 0U;
  std::size_t waypointCount = 0U;
  std::size_t parentedActorCount = 0U;
  std::size_t stationaryActorCount = 0U;
  std::size_t legacyPatrolNodeCount = 0U;
  bool ownersValid = true;
  for (const cr::CreativeObject& object : map.document.objects()) {
    if (object.kind == cr::CreativeObjectKind::PatrolRoute) {
      ++routeCount;
      waypointCount += object.pathPoints.size();
    } else if (object.kind == cr::CreativeObjectKind::PatrolNode) {
      ++legacyPatrolNodeCount;
    } else if (object.kind == cr::CreativeObjectKind::NpcSpawn) {
      if (!object.parentId.has_value()) {
        ++stationaryActorCount;
        continue;
      }
      ++parentedActorCount;
      const cr::CreativeObject* owner =
          map.document.findObject(*object.parentId);
      ownersValid = ownersValid && owner != nullptr &&
                    owner->kind == cr::CreativeObjectKind::PatrolRoute;
    }
  }
  return expect(routeCount == 6U && waypointCount == 18U,
                "six explicit routes own eighteen waypoints") &&
         expect(parentedActorCount == 6U && stationaryActorCount == 2U &&
                    ownersValid,
                "six moving guards reference routes; two watches are stationary") &&
         expect(legacyPatrolNodeCount == 0U,
                "map no longer encodes patrol meaning as loose markers");
}

bool fixtureMatchesGenerator() {
  const iggy3d::CreativeWorldOpenResult opened =
      iggy3d::openCreativeWorld({"fixtures/worlds", "map_demo"});
  if (!expect(opened.accepted, "committed fixture loads via the real codec")) {
    return false;
  }
  const cr::CreativeMapTemplateResult built = buildMap();
  bool topologyMatches =
      opened.document.objectCount() == built.document.objectCount();
  const auto& openedObjects = opened.document.objects();
  const auto& builtObjects = built.document.objects();
  for (std::size_t index = 0U;
       topologyMatches && index < builtObjects.size(); ++index) {
    topologyMatches =
        openedObjects[index].id == builtObjects[index].id &&
        openedObjects[index].kind == builtObjects[index].kind &&
        openedObjects[index].name == builtObjects[index].name &&
        openedObjects[index].parentId == builtObjects[index].parentId &&
        samePath(openedObjects[index].pathPoints,
                 builtObjects[index].pathPoints);
  }
  return expect(topologyMatches,
                "fixture preserves generated ids, parents, and route paths") &&
         expect(opened.objectCount == built.objectCount,
                "fixture receipt count matches");
}

// ---- 2. preparation + activation (Leg A pin lives here) ------------------

bool activationSeedsGraphAndPatrols() {
  const cr::CreativeMapTemplateResult map = buildMap();
  const iggy3d::StaticMeshAssetCatalog catalog =
      iggy3d::discoverStaticMeshAssetCatalog("assets/creative");
  if (!expect(!catalog.entries.empty() && catalog.failures.empty(),
              "asset catalog discovers cleanly")) {
    return false;
  }
  ActivatedDemo demo = activateDemo(map.document, catalog);
  if (!expect(demo.preparation.accepted, "play preparation accepts the map") ||
      !expect(demo.ok, "sandbox activation succeeds")) {
    std::cerr << "  preparation status: "
              << toString(demo.preparation.status)
              << " activation: " << toString(demo.receipt.status) << '\n';
    for (const cr::CreativeMapDiagnostic& diagnostic :
         demo.preparation.validation.diagnostics) {
      std::cerr << "  diagnostic: " << toString(diagnostic.code)
                << " object=" << diagnostic.objectId
                << " subject='" << diagnostic.subject
                << "' detail='" << diagnostic.detail << "'\n";
    }
    return false;
  }
  // Leg A pin: play activation produced a non-empty reasoning graph whose
  // patrol posts came from explicit PatrolRoute plans.
  const iggy3d::ReasoningGraphSummary& graph = demo.receipt.reasoningGraph;
  const std::size_t patrolPostNodes = graph.perKindCounts[static_cast<std::size_t>(
      iggy3d::ReasoningNodeKind::patrolPost)];
  bool ok =
      expect(graph.nodeCount > 0U, "session reasoning graph is non-empty") &&
      expect(graph.edgeCount > 0U, "reasoning graph has walkable edges") &&
      expect(patrolPostNodes >= 18U,
             "reasoning graph carries every authored patrol post") &&
      expect(demo.receipt.scenario.npcEntityCount == 8U,
             "eight guards seeded") &&
      expect(demo.receipt.scenario.patrolRouteCount == 6U,
             "six patrol routes wired") &&
      expect(demo.receipt.scenario.patrolWaypointCount == 18U,
             "eighteen patrol waypoints adopted");
  // A guard's route crosses the graph: every patrol waypoint coincides with
  // a patrolPost graph node by construction (same anchors) -- assert the
  // counts already did; here assert the session actually carries routes.
  std::size_t patrollingGuards = 0U;
  for (const iggy3d::AiActorState& actor :
       demo.sandbox->session.state().ai.actors) {
    if (!actor.patrolWaypoints.empty()) {
      ++patrollingGuards;
      ok = ok && expect(actor.patrolWaypoints.size() >= 2U,
                        "route has at least two waypoints");
    }
  }
  ok = ok && expect(patrollingGuards == 6U,
                    "six session guards carry patrol routes");
  return ok;
}

// ---- 3. the 2000-tick patrol simulation ----------------------------------

std::uint64_t hashPositions(std::uint64_t hash, const iggy3d::SessionState& state) {
  const auto mix = [&hash](float value) {
    std::uint32_t bits;
    static_assert(sizeof(bits) == sizeof(value));
    std::memcpy(&bits, &value, sizeof(bits));
    hash ^= bits;
    hash *= 1099511628211ULL;
  };
  for (const iggy3d::EntityState& entity : state.world.entities()) {
    mix(entity.transform.position.x);
    mix(entity.transform.position.y);
    mix(entity.transform.position.z);
  }
  return hash;
}

struct SimRun {
  bool ok = false;
  std::uint64_t positionHash = 1469598103934665603ULL;
  // per patrol guard: min/max over the run + waypoint visit flags
  std::map<std::string, double> travelExtent;
  std::map<std::string, std::size_t> waypointsVisited;
};

SimRun simulate(const cr::CreativeDocument& document,
                const iggy3d::StaticMeshAssetCatalog& catalog) {
  SimRun run;
  ActivatedDemo demo = activateDemo(document, catalog);
  if (!demo.ok) {
    return run;
  }
  iggy3d::Session& session = demo.sandbox->session;
  struct GuardTrack {
    iggy3d::EntityId id;
    std::string name;
    std::vector<iggy3d::Vec3> waypoints;
    std::vector<bool> visited;
    iggy3d::Vec3 minSeen;
    iggy3d::Vec3 maxSeen;
  };
  std::vector<GuardTrack> guards;
  for (const iggy3d::AiActorState& actor : session.state().ai.actors) {
    if (actor.patrolWaypoints.empty()) {
      continue;
    }
    const iggy3d::EntityState* entity =
        session.state().world.findById(actor.actor);
    if (entity == nullptr) {
      return run;
    }
    GuardTrack track;
    track.id = entity->id;
    track.name = entity->stableName;
    track.waypoints = actor.patrolWaypoints;
    track.visited.assign(actor.patrolWaypoints.size(), false);
    track.minSeen = track.maxSeen = entity->transform.position;
    guards.push_back(std::move(track));
  }
  if (guards.size() != 6U) {
    return run;
  }

  for (std::uint32_t tick = 0; tick < kSimTicks; ++tick) {
    const iggy3d::StatusResult status = session.tickWithOptions(
        {&demo.sandbox->collisionSurfaces, true});
    if (status.status != iggy3d::ResultStatus::Ok) {
      std::cerr << "  tick " << tick << " failed: " << status.error.code
                << '\n';
      return run;
    }
    for (GuardTrack& guard : guards) {
      const iggy3d::EntityState* entity =
          session.state().world.findById(guard.id);
      if (entity == nullptr) {
        return run;
      }
      const iggy3d::Vec3 position = entity->transform.position;
      guard.minSeen.x = std::min(guard.minSeen.x, position.x);
      guard.minSeen.z = std::min(guard.minSeen.z, position.z);
      guard.maxSeen.x = std::max(guard.maxSeen.x, position.x);
      guard.maxSeen.z = std::max(guard.maxSeen.z, position.z);
      for (std::size_t i = 0; i < guard.waypoints.size(); ++i) {
        const double dx = position.x - guard.waypoints[i].x;
        const double dz = position.z - guard.waypoints[i].z;
        if (dx * dx + dz * dz < 1.5 * 1.5) {
          guard.visited[i] = true;
        }
      }
    }
    if (tick % 50U == 0U) {
      run.positionHash = hashPositions(run.positionHash, session.state());
    }
  }
  run.positionHash = hashPositions(run.positionHash, session.state());
  for (const GuardTrack& guard : guards) {
    run.travelExtent[guard.name] =
        std::max(guard.maxSeen.x - guard.minSeen.x,
                 guard.maxSeen.z - guard.minSeen.z);
    run.waypointsVisited[guard.name] = static_cast<std::size_t>(
        std::count(guard.visited.begin(), guard.visited.end(), true));
  }
  run.ok = true;
  return run;
}

bool patrolSimulationProgressesAndIsDeterministic() {
  const cr::CreativeMapTemplateResult map = buildMap();
  const iggy3d::StaticMeshAssetCatalog catalog =
      iggy3d::discoverStaticMeshAssetCatalog("assets/creative");
  const SimRun first = simulate(map.document, catalog);
  const SimRun second = simulate(map.document, catalog);
  bool ok = expect(first.ok && second.ok, "both 2000-tick sims complete");
  if (!ok) {
    return false;
  }
  for (const auto& [name, extent] : first.travelExtent) {
    ok = ok && expect(extent > 8.0,
                      ("guard progresses along its route: " + name).c_str());
  }
  for (const auto& [name, visited] : first.waypointsVisited) {
    ok = ok && expect(visited >= 2U,
                      ("guard visits multiple waypoints: " + name).c_str());
  }
  ok = ok && expect(first.positionHash == second.positionHash,
                    "two sims produce identical position hashes");
  return ok;
}

// ---- 4. reachability + clamber-only bypasses -----------------------------

struct BakedWorld {
  bool ok = false;
  std::vector<iggy3d::PhysicsAabbCollider> colliders;
  cr::CreativeDocument document;
};

BakedWorld bakeWorld() {
  BakedWorld world;
  cr::CreativeMapTemplateResult map = buildMap();
  const iggy3d::StaticMeshAssetCatalog catalog =
      iggy3d::discoverStaticMeshAssetCatalog("assets/creative");
  ActivatedDemo demo = activateDemo(map.document, catalog);
  if (!demo.ok) {
    return world;
  }
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult bake =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces(
          {&demo.sandbox->collisionSurfaces, {}});
  if (!bake.ok) {
    return world;
  }
  world.colliders = bake.colliders;
  world.document = std::move(map.document);
  world.ok = true;
  return world;
}

iggy3d::ReachabilityCoord toCell(double x, double z) {
  return {static_cast<std::int32_t>(std::floor(x - kWorldMin)),
          static_cast<std::int32_t>(std::floor(z - kWorldMin))};
}

// A ground cell is walkable when no blocking collider occupies the player
// column above it (radius-inflated point test at the cell center).
iggy3d::ReachabilityGrid buildWalkGrid(
    const std::vector<iggy3d::PhysicsAabbCollider>& colliders) {
  const iggy3d::MovementParams params;
  iggy3d::ReachabilityGrid grid;
  grid.width = kGridCells;
  grid.depth = kGridCells;
  grid.walkable.assign(
      static_cast<std::size_t>(kGridCells) * kGridCells, 1U);
  for (std::int32_t cz = 0; cz < kGridCells; ++cz) {
    for (std::int32_t cx = 0; cx < kGridCells; ++cx) {
      const double x = kWorldMin + cx + 0.5;
      const double z = kWorldMin + cz + 0.5;
      for (const iggy3d::PhysicsAabbCollider& collider : colliders) {
        if (collider.sensor) {
          continue;
        }
        const auto& b = collider.bounds;
        if (b.max.y <= 0.05F || b.min.y >= params.heightMeters) {
          continue;  // outside the standing column
        }
        if (b.max.y - 0.0F <= params.stepHeightMeters && b.min.y <= 0.05F) {
          continue;  // steppable clutter does not block a cell
        }
        if (x + params.radiusMeters > b.min.x &&
            x - params.radiusMeters < b.max.x &&
            z + params.radiusMeters > b.min.z &&
            z - params.radiusMeters < b.max.z) {
          grid.walkable[static_cast<std::size_t>(cz) *
                            static_cast<std::size_t>(kGridCells) +
                        static_cast<std::size_t>(cx)] = 0U;
          break;
        }
      }
    }
  }
  return grid;
}

const cr::CreativeObject* findObject(const cr::CreativeDocument& document,
                                     std::string_view name) {
  for (const cr::CreativeObject& object : document.objects()) {
    if (object.name == name) {
      return &object;
    }
  }
  return nullptr;
}

bool reachabilityProvesSneakPathAndPatrols() {
  BakedWorld world = bakeWorld();
  if (!expect(world.ok, "world bakes for reachability")) {
    return false;
  }
  const iggy3d::ReachabilityGrid grid = buildWalkGrid(world.colliders);
  const cr::CreativeObject* spawn = findObject(world.document, "Player Spawn");
  const cr::CreativeObject* objective =
      findObject(world.document, "Objective Cache");
  if (!expect(spawn != nullptr && objective != nullptr,
              "spawn and objective authored")) {
    return false;
  }
  const std::array<iggy3d::ReachabilityCoord, 1> seeds{
      toCell(spawn->transform.position.x, spawn->transform.position.z)};
  const iggy3d::ReachabilityReceipt flood = iggy3d::floodFillReachability(
      grid, seeds, iggy3d::ReachabilityConnectivity::FourWay);
  if (!expect(flood.ok && flood.reachedCellCount > 1000U,
              "flood fill reaches a large connected field")) {
    return false;
  }
  const auto reached = [&](double x, double z) {
    const iggy3d::ReachabilityCoord cell = toCell(x, z);
    return flood.reached[static_cast<std::size_t>(cell.z) *
                             static_cast<std::size_t>(kGridCells) +
                         static_cast<std::size_t>(cell.x)] != 0U;
  };
  bool ok = expect(reached(objective->transform.position.x,
                           objective->transform.position.z),
                   "objective is walk-reachable from spawn (sneak path)");
  std::size_t waypointCount = 0U;
  for (const cr::CreativeObject& object : world.document.objects()) {
    if (object.kind == cr::CreativeObjectKind::PatrolRoute) {
      for (std::size_t index = 0U; index < object.pathPoints.size(); ++index) {
        const cr::CreativeVec3 position = object.pathPoints[index].position;
        ++waypointCount;
        ok = ok && expect(
                       reached(position.x, position.z),
                       ("patrol waypoint walkable: " + object.name + " " +
                        std::to_string(index + 1U))
                           .c_str());
      }
    } else if (object.kind == cr::CreativeObjectKind::NpcSpawn) {
      ok = ok && expect(reached(object.transform.position.x,
                                object.transform.position.z),
                        ("guard spawn walkable: " + object.name).c_str());
    }
  }
  return ok && expect(waypointCount == 18U, "eighteen authored waypoints");
}

bool clamberBypassesAreClamberOnly() {
  BakedWorld world = bakeWorld();
  if (!expect(world.ok, "world bakes for bypass check")) {
    return false;
  }
  const iggy3d::MovementParams params;
  bool ok = true;
  // Both walkway families: top must sit in the clamber band above the
  // surrounding ground (walk/autoStep cannot ascend; clamber can), and no
  // foreign steppable collider may form a staircase into them.
  const struct {
    const char* prefix;
    double top;
  } kBypasses[] = {{"Bypass1 Walkway", 1.5}, {"Bypass2 Walkway", 3.0}};
  for (const auto& bypass : kBypasses) {
    std::size_t segments = 0U;
    for (const cr::CreativeObject& object : world.document.objects()) {
      if (object.name.rfind(bypass.prefix, 0) != 0) {
        continue;
      }
      ++segments;
      const double top = object.bounds.max.y;
      ok = ok && expect(std::fabs(top - bypass.top) < 0.01,
                        "walkway top at designed height");
      if (bypass.top <= params.clamberBandTopMeters) {
        ok = ok && expect(top - 0.0 > params.stepHeightMeters &&
                              top - 0.0 <= params.clamberBandTopMeters,
                          "walkway rise is clamber-only from ground");
      }
      // No steppable ladder of foreign colliders into the walkway top: any
      // collider whose top lands within autoStep below the walkway top and
      // overlaps its 1m neighborhood would break the clamber-only claim.
      for (const iggy3d::PhysicsAabbCollider& collider : world.colliders) {
        const double colliderTop = collider.bounds.max.y;
        if (colliderTop <= bypass.top - params.stepHeightMeters ||
            colliderTop >= bypass.top + 0.01) {
          continue;
        }
        if (std::fabs(colliderTop - bypass.top) < 0.01) {
          continue;  // sibling walkway segments share the top plane
        }
        const bool nearby =
            collider.bounds.max.x > object.bounds.min.x - 1.0 &&
            collider.bounds.min.x < object.bounds.max.x + 1.0 &&
            collider.bounds.max.z > object.bounds.min.z - 1.0 &&
            collider.bounds.min.z < object.bounds.max.z + 1.0;
        ok = ok && expect(!nearby,
                          "no steppable ramp into the clamber-only walkway");
      }
    }
    ok = ok && expect(segments >= 3U, "bypass has a full walkway run");
  }
  // Giant stair risers are 0.5m: above autoStep, inside the band -- every
  // step of bypass #2's entry is itself a clamber.
  ok = ok && expect(0.5 > params.stepHeightMeters &&
                        0.5 <= params.clamberBandTopMeters,
                    "giant stair risers are clamber-band steps");
  // The kit fail block is in-map and above the band (must refuse).
  const cr::CreativeObject* fail = findObject(world.document, "Ladder Fail 2p0");
  ok = ok && expect(fail != nullptr &&
                        fail->bounds.max.y > params.clamberBandTopMeters,
                    "the 2.0 fail block stands above the clamber band");
  return ok;
}

}  // namespace

int main() {
  const bool ok = generatorIsDeterministic() &&
                  generatorUsesExplicitPatrolTopology() &&
                  fixtureMatchesGenerator() &&
                  activationSeedsGraphAndPatrols() &&
                  patrolSimulationProgressesAndIsDeterministic() &&
                  reachabilityProvesSneakPathAndPatrols() &&
                  clamberBypassesAreClamberOnly();
  if (ok) {
    std::cout << "map_demo_tests passed\n";
  }
  return ok ? 0 : 1;
}
