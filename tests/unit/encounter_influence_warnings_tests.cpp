// a6s2 — the encounter battle report's warnings derived from the L7 influence map over the PLACED
// encounter. STRUCTURAL rule-isolation + union/dedup + determinism (hand-built placements; no deal).

#include "runtime/encounter/EncounterPlacement.hpp"

#include "content/assets/RoomAsset.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/ai/ReasoningGraph.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp"

#include <algorithm>
#include <iostream>
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

iggy3d::RoomAnchorAsset anchor(const std::string& kind, iggy3d::Vec3 pos) {
  iggy3d::RoomAnchorAsset a;
  a.id = "anchor_" + kind;
  a.kind = kind;
  a.positionMeters = pos;
  return a;
}

iggy3d::ReasoningGraph graphOf(const iggy3d::RoomAsset& room) {
  const std::vector<iggy3d::Vec3> noWaypoints;
  return iggy3d::buildReasoningGraph(room, noWaypoints);
}

std::vector<iggy3d::PhysicsAabbCollider> bake(const iggy3d::RoomAsset& room) {
  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);
  iggy3d::PhysicsSpatialSurfaceColliderBakeRequest request;
  request.surfaces = &surfaces;
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult result =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces(request);
  return result.ok ? result.colliders : std::vector<iggy3d::PhysicsAabbCollider>{};
}

struct EnemySpec {
  std::string name;
  iggy3d::Vec3 position;
  std::vector<iggy3d::Vec3> waypoints;
};

iggy3d::EncounterPlacementResult makePlacement(const std::vector<EnemySpec>& enemies) {
  iggy3d::EncounterPlacementResult placement;
  for (const EnemySpec& spec : enemies) {
    iggy3d::ScenarioEntitySeed entity;
    entity.stableName = spec.name;
    entity.kind = iggy3d::EntityKind::Npc;
    entity.transform.position = spec.position;
    placement.enemyEntities.push_back(std::move(entity));
    iggy3d::ScenarioAiActorSeed aiActor;
    aiActor.actorStableName = spec.name;
    aiActor.patrolWaypoints = spec.waypoints;
    placement.enemyAiActors.push_back(std::move(aiActor));
  }
  return placement;
}

bool hasWarning(const std::vector<std::string>& warnings, std::string_view code) {
  return std::find(warnings.begin(), warnings.end(), std::string(code)) != warnings.end();
}

const iggy3d::InfluenceWarningConfig kWarnConfig;

bool objectiveLowCoverageFiresAndSilences() {
  iggy3d::RoomAsset room;
  room.id = "objective_room";
  room.anchors = {anchor("spawn", {1, 0, 1}), anchor("exit", {12, 0, 1}), anchor("treasure", {6, 0, 6})};
  const iggy3d::ReasoningGraph graph = graphOf(room);
  const std::vector<iggy3d::PhysicsAabbCollider> colliders = bake(room);

  // FIRES: the only guard is far (> guard radius) from the objective => guardInfluence 0 there.
  const iggy3d::EncounterPlacementResult far = makePlacement({{"g0", {0, 0, 0}, {}}});
  const std::vector<std::string> farWarnings =
      iggy3d::deriveEncounterInfluenceWarnings(graph, colliders, far, kWarnConfig);
  bool ok = expect(hasWarning(farWarnings, "objective_low_coverage"),
                   "a guard far from the objective fires objective_low_coverage");

  // SILENT: a guard ON the objective => guardInfluence 1.0 (>= threshold) => no warning.
  const iggy3d::EncounterPlacementResult onObjective = makePlacement({{"g0", {6, 0, 6}, {}}});
  const std::vector<std::string> onWarnings =
      iggy3d::deriveEncounterInfluenceWarnings(graph, colliders, onObjective, kWarnConfig);
  ok = ok && expect(!hasWarning(onWarnings, "objective_low_coverage"),
                    "a guard on the objective silences objective_low_coverage");
  return ok;
}

bool noObjectiveNodeNeverFires() {
  iggy3d::RoomAsset room;  // no treasure/key/pickup => no objective node
  room.id = "no_objective";
  room.anchors = {anchor("spawn", {1, 0, 1}), anchor("exit", {12, 0, 1})};
  const iggy3d::EncounterPlacementResult placement = makePlacement({{"g0", {0, 0, 0}, {}}});
  const std::vector<std::string> warnings = iggy3d::deriveEncounterInfluenceWarnings(
      graphOf(room), bake(room), placement, kWarnConfig);
  return expect(!hasWarning(warnings, "objective_low_coverage"),
                "no objective node -> objective_low_coverage never fires");
}

bool unwatchedEscapeRouteFiresAndSilences() {
  iggy3d::RoomAsset room;  // exit far so a distant guard leaves it unwatched (no objective node)
  room.id = "escape_room";
  room.anchors = {anchor("spawn", {1, 0, 1}), anchor("exit", {30, 0, 1})};
  const iggy3d::ReasoningGraph graph = graphOf(room);
  const std::vector<iggy3d::PhysicsAabbCollider> colliders = bake(room);

  // SILENT: a guard near the exit with clear LOS covers it.
  const iggy3d::EncounterPlacementResult watching = makePlacement({{"g0", {28, 0, 1}, {}}});
  const std::vector<std::string> watchingWarnings =
      iggy3d::deriveEncounterInfluenceWarnings(graph, colliders, watching, kWarnConfig);
  bool ok = expect(!hasWarning(watchingWarnings, "unwatched_escape_route"),
                   "a guard watching the exit silences unwatched_escape_route");

  // FIRES: the only guard is > visibility radius from the exit.
  const iggy3d::EncounterPlacementResult away = makePlacement({{"g0", {1, 0, 1}, {}}});
  const std::vector<std::string> awayWarnings =
      iggy3d::deriveEncounterInfluenceWarnings(graph, colliders, away, kWarnConfig);
  ok = ok && expect(hasWarning(awayWarnings, "unwatched_escape_route"),
                    "a guard far from the exit fires unwatched_escape_route");
  return ok;
}

bool guardSamplesUnionAndDedup() {
  // Two guards sharing the same patrol_post waypoints; guard0's entity sits on a shared waypoint.
  const iggy3d::EncounterPlacementResult placement =
      makePlacement({{"g0", {2, 0, 5}, {{2, 0, 5}, {11, 0, 5}}},
                     {"g1", {10, 0, 1}, {{2, 0, 5}, {11, 0, 5}}}});
  const std::vector<iggy3d::InfluenceGuardSample> samples =
      iggy3d::encounterGuardSamples(placement);
  // Unique positions: (2,5), (11,5), (10,1) = 3.
  return expect(samples.size() == 3U,
                "encounterGuardSamples unions entity + waypoints and dedups shared positions");
}

bool derivationIsDeterministic() {
  iggy3d::RoomAsset room;
  room.id = "determinism_room";
  room.anchors = {anchor("spawn", {1, 0, 1}), anchor("exit", {30, 0, 1}), anchor("treasure", {6, 0, 6})};
  const iggy3d::ReasoningGraph graph = graphOf(room);
  const std::vector<iggy3d::PhysicsAabbCollider> colliders = bake(room);
  const iggy3d::EncounterPlacementResult placement = makePlacement({{"g0", {0, 0, 0}, {}}});
  const std::vector<std::string> a =
      iggy3d::deriveEncounterInfluenceWarnings(graph, colliders, placement, kWarnConfig);
  const std::vector<std::string> b =
      iggy3d::deriveEncounterInfluenceWarnings(graph, colliders, placement, kWarnConfig);
  return expect(a == b && a.size() == 2U,
                "warnings are deterministic (call-twice identical; both rules fire here)");
}

}  // namespace

int main() {
  const bool ok = objectiveLowCoverageFiresAndSilences() && noObjectiveNodeNeverFires() &&
                  unwatchedEscapeRouteFiresAndSilences() && guardSamplesUnionAndDedup() &&
                  derivationIsDeterministic();
  return ok ? 0 : 1;
}
