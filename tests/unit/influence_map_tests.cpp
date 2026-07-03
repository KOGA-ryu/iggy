// a6s1 — L7 influence-map kernel: pure, headless, STRUCTURAL tests over hand-built graphs (+ the
// real garden geometry for the LOS-occlusion visibility case). No real-config goldens.

#include "runtime/ai/InfluenceMap.hpp"

#include "content/assets/RoomAsset.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/ai/ReasoningGraph.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
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

iggy3d::ReasoningNode node(std::uint32_t id, iggy3d::ReasoningNodeKind kind, iggy3d::Vec3 pos) {
  iggy3d::ReasoningNode n;
  n.id = id;
  n.kind = kind;
  n.positionMeters = pos;
  n.sourceLabel = "test";
  return n;
}

iggy3d::ReasoningEdge edge(std::uint32_t from, std::uint32_t to, float lengthMeters) {
  iggy3d::ReasoningEdge e;
  e.from = from;
  e.to = to;
  e.kind = iggy3d::ReasoningEdgeKind::walkable;
  e.lengthMeters = lengthMeters;
  return e;
}

float channel(const iggy3d::InfluenceMap& map, std::size_t i, iggy3d::InfluenceChannel c) {
  return iggy3d::influenceValue(map, i, c);
}

const std::vector<iggy3d::PhysicsAabbCollider> kNoColliders;
const std::vector<iggy3d::InfluenceGuardSample> kNoGuards;
const iggy3d::InfluenceMapConfig kConfig;

bool determinism() {
  iggy3d::ReasoningGraph g;
  g.nodes = {node(0, iggy3d::ReasoningNodeKind::exit, {0, 0, 0}),
             node(1, iggy3d::ReasoningNodeKind::reference, {3, 0, 0}),
             node(2, iggy3d::ReasoningNodeKind::objective, {6, 0, 0})};
  g.edges = {edge(0, 1, 3), edge(1, 2, 3)};
  const std::vector<iggy3d::InfluenceGuardSample> guards = {{{1, 0, 0}}, {{5, 0, 0}}};
  const iggy3d::InfluenceMap a = iggy3d::buildInfluenceMap(g, kNoColliders, guards, kConfig);
  const iggy3d::InfluenceMap b = iggy3d::buildInfluenceMap(g, kNoColliders, guards, kConfig);
  return expect(a.nodeCount == b.nodeCount && a.values == b.values,
                "identical inputs -> bitwise-identical influence map") &&
         expect(a.nodeCount == 3U && a.values.size() == 3U, "row i corresponds to nodes[i]");
}

bool escapePeaksAtExit() {
  // exit - a - b - c chain, no occluders.
  iggy3d::ReasoningGraph g;
  g.nodes = {node(0, iggy3d::ReasoningNodeKind::exit, {0, 0, 0}),
             node(1, iggy3d::ReasoningNodeKind::reference, {3, 0, 0}),
             node(2, iggy3d::ReasoningNodeKind::reference, {6, 0, 0}),
             node(3, iggy3d::ReasoningNodeKind::reference, {9, 0, 0})};
  g.edges = {edge(0, 1, 3), edge(1, 2, 3), edge(2, 3, 3)};
  const iggy3d::InfluenceMap map = iggy3d::buildInfluenceMap(g, kNoColliders, kNoGuards, kConfig);
  const auto esc = [&map](std::size_t i) {
    return channel(map, i, iggy3d::InfluenceChannel::escapeRoutePressure);
  };
  return expect(esc(0) == 1.0F, "escapeRoutePressure peaks (1.0) at the exit node") &&
         expect(esc(0) > esc(1) && esc(1) > esc(2) && esc(2) > esc(3),
                "escapeRoutePressure strictly decays with graph distance from the exit");
}

bool objectivePeaksAtObjective() {
  iggy3d::ReasoningGraph g;
  g.nodes = {node(0, iggy3d::ReasoningNodeKind::objective, {0, 0, 0}),
             node(1, iggy3d::ReasoningNodeKind::reference, {3, 0, 0}),
             node(2, iggy3d::ReasoningNodeKind::reference, {6, 0, 0})};
  g.edges = {edge(0, 1, 3), edge(1, 2, 3)};
  const iggy3d::InfluenceMap map = iggy3d::buildInfluenceMap(g, kNoColliders, kNoGuards, kConfig);
  const auto obj = [&map](std::size_t i) {
    return channel(map, i, iggy3d::InfluenceChannel::objectiveControl);
  };
  return expect(obj(0) == 1.0F && obj(0) > obj(1) && obj(1) > obj(2),
                "objectiveControl peaks at the objective and decays with graph distance");
}

bool noExitYieldsZeroEscape() {
  iggy3d::ReasoningGraph g;
  g.nodes = {node(0, iggy3d::ReasoningNodeKind::reference, {0, 0, 0}),
             node(1, iggy3d::ReasoningNodeKind::reference, {3, 0, 0})};
  g.edges = {edge(0, 1, 3)};
  const iggy3d::InfluenceMap map = iggy3d::buildInfluenceMap(g, kNoColliders, kNoGuards, kConfig);
  return expect(channel(map, 0, iggy3d::InfluenceChannel::escapeRoutePressure) == 0.0F &&
                    channel(map, 1, iggy3d::InfluenceChannel::escapeRoutePressure) == 0.0F,
                "no exit node -> escapeRoutePressure 0 everywhere (honest)");
}

bool guardInfluenceFallsOffWithRadiusCutoff() {
  iggy3d::ReasoningGraph g;
  g.nodes = {node(0, iggy3d::ReasoningNodeKind::reference, {2, 0, 0}),
             node(1, iggy3d::ReasoningNodeKind::reference, {7, 0, 0}),
             node(2, iggy3d::ReasoningNodeKind::reference, {8, 0, 0}),
             node(3, iggy3d::ReasoningNodeKind::reference, {10, 0, 0})};
  const std::vector<iggy3d::InfluenceGuardSample> guards = {{{0, 0, 0}}};  // radius default 8
  const iggy3d::InfluenceMap map = iggy3d::buildInfluenceMap(g, kNoColliders, guards, kConfig);
  const auto gi = [&map](std::size_t i) {
    return channel(map, i, iggy3d::InfluenceChannel::guardInfluence);
  };
  return expect(gi(0) > gi(1) && gi(1) > 0.0F, "nearer node has higher guard influence") &&
         expect(gi(2) == 0.0F && gi(3) == 0.0F,
                "a node at/beyond the guard radius has exactly 0 influence (hard cutoff)");
}

// --- garden geometry for the LOS-occlusion visibility case -----------------------------------
iggy3d::Vec3 cellToWorld(int col, int row) {
  return iggy3d::Vec3{static_cast<float>(col), 0.0F, static_cast<float>(row)};
}

iggy3d::RoomAsset buildGardenRoom() {
  iggy3d::RoomAsset room;
  room.id = "stealth_garden";
  std::ifstream file("fixtures/rooms/ascii/stealth_garden.iggyroom.txt");
  std::ostringstream buffer;
  buffer << file.rdbuf();
  const std::string grid = buffer.str();

  int maxCol = 0;
  int rows = 0;
  int row = 0;
  iggy3d::Vec3 guardSpawn;
  iggy3d::Vec3 playerSpawn;
  iggy3d::Vec3 exitCell;
  std::istringstream input(grid);
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty()) {
      continue;
    }
    for (int col = 0; col < static_cast<int>(line.size()); ++col) {
      const char glyph = line[static_cast<std::size_t>(col)];
      maxCol = std::max(maxCol, col);
      if (glyph == '#') {
        iggy3d::RoomSpatialSurface wall;
        wall.id = "wall_" + std::to_string(col) + "_" + std::to_string(row);
        wall.sourceStaticMeshId = wall.id;
        wall.shape = iggy3d::RoomSpatialSurfaceShape::Box;
        wall.role = iggy3d::RoomSpatialSurfaceRole::Blocker;
        const float x0 = static_cast<float>(col) - 0.5F;
        const float x1 = static_cast<float>(col) + 0.5F;
        const float z0 = static_cast<float>(row) - 0.5F;
        const float z1 = static_cast<float>(row) + 0.5F;
        wall.pointsMeters = {{x0, 0.0F, z0}, {x1, 0.0F, z0}, {x1, 3.0F, z1}, {x0, 3.0F, z1}};
        wall.normal = {0.0F, 0.0F, 1.0F};
        wall.collisionMask = {"actor"};
        wall.blocksActor = true;
        room.spatialSurfaces.push_back(std::move(wall));
      } else if (glyph == 'N') {
        guardSpawn = cellToWorld(col, row);
      } else if (glyph == 'P') {
        playerSpawn = cellToWorld(col, row);
      } else if (glyph == 'E') {
        exitCell = cellToWorld(col, row);
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
  const auto pushAnchor = [&room](const char* kind, iggy3d::Vec3 pos) {
    iggy3d::RoomAnchorAsset a;
    a.id = std::string("anchor_") + kind;
    a.kind = kind;
    a.positionMeters = pos;
    room.anchors.push_back(std::move(a));
  };
  pushAnchor("exit", exitCell);
  pushAnchor("npc", guardSpawn);
  pushAnchor("spawn", playerSpawn);
  return room;
}

bool visibilityRespectsIslandOcclusion() {
  const iggy3d::RoomAsset room = buildGardenRoom();
  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);
  iggy3d::PhysicsSpatialSurfaceColliderBakeRequest request;
  request.surfaces = &surfaces;
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult bake =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces(request);
  const std::vector<iggy3d::Vec3> waypoints = {cellToWorld(1, 1), cellToWorld(1, 5)};
  const iggy3d::ReasoningGraph graph = iggy3d::buildReasoningGraph(room, waypoints);

  // A guard at the exit corner (12,1): clear top-row LOS to patrolPost(1,1), island-occluded to
  // patrolPost(1,5).
  const std::vector<iggy3d::InfluenceGuardSample> guards = {{cellToWorld(12, 1)}};
  const iggy3d::InfluenceMap map = iggy3d::buildInfluenceMap(graph, bake.colliders, guards, kConfig);

  const auto indexOf = [&graph](iggy3d::ReasoningNodeKind kind, iggy3d::Vec3 pos) -> std::size_t {
    for (std::size_t i = 0; i < graph.nodes.size(); ++i) {
      if (graph.nodes[i].kind == kind && iggy3d::nearlyEqual(graph.nodes[i].positionMeters, pos, 0.01F)) {
        return i;
      }
    }
    return graph.nodes.size();
  };
  const std::size_t openNode = indexOf(iggy3d::ReasoningNodeKind::patrolPost, cellToWorld(1, 1));
  const std::size_t occluded = indexOf(iggy3d::ReasoningNodeKind::patrolPost, cellToWorld(1, 5));
  bool ok = expect(openNode < graph.nodes.size() && occluded < graph.nodes.size(),
                   "garden patrolPost nodes found");
  ok = ok && expect(channel(map, openNode, iggy3d::InfluenceChannel::visibilityCoverage) == 1.0F,
                    "open top-row node is covered (radius + clear LOS)");
  ok = ok && expect(channel(map, occluded, iggy3d::InfluenceChannel::visibilityCoverage) == 0.0F,
                    "island-occluded node is uncovered (potential visibility respects LOS)");
  return ok;
}

bool reservedSlotsAreZero() {
  iggy3d::ReasoningGraph g;
  g.nodes = {node(0, iggy3d::ReasoningNodeKind::exit, {0, 0, 0}),
             node(1, iggy3d::ReasoningNodeKind::objective, {3, 0, 0})};
  g.edges = {edge(0, 1, 3)};
  const std::vector<iggy3d::InfluenceGuardSample> guards = {{{0, 0, 0}}};
  const iggy3d::InfluenceMap map = iggy3d::buildInfluenceMap(g, kNoColliders, guards, kConfig);
  bool ok = true;
  for (std::size_t i = 0; i < map.nodeCount; ++i) {
    ok = ok && expect(channel(map, i, iggy3d::InfluenceChannel::danger) == 0.0F &&
                          channel(map, i, iggy3d::InfluenceChannel::soundPressure) == 0.0F,
                      "reserved channels danger + soundPressure are 0 (not computed v1)");
  }
  const iggy3d::ReasoningGraph empty;
  const iggy3d::InfluenceMap emptyMap = iggy3d::buildInfluenceMap(empty, kNoColliders, guards, kConfig);
  return ok && expect(emptyMap.nodeCount == 0U && emptyMap.values.empty(),
                      "empty graph -> empty map (no crash)");
}

}  // namespace

int main() {
  const bool ok = determinism() && escapePeaksAtExit() && objectivePeaksAtObjective() &&
                  noExitYieldsZeroEscape() && guardInfluenceFallsOffWithRadiusCutoff() &&
                  visibilityRespectsIslandOcclusion() && reservedSlotsAreZero();
  return ok ? 0 : 1;
}
