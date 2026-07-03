// a6s1 influence_readout -- s8-style headless dashboard. Builds the garden graph + baked colliders
// + a small guard-sample set (the two patrolPosts, mimicking a6s2's placed-guards-at-waypoints),
// PRINTS the per-node channel table, and asserts board sanity. Retrieve via `ctest -V -R influence_readout`.

#include "runtime/ai/InfluenceMap.hpp"

#include "content/assets/RoomAsset.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/ai/ReasoningGraph.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp"

#include <algorithm>
#include <cmath>
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

}  // namespace

int main() {
  const iggy3d::RoomAsset room = buildGardenRoom();
  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);
  iggy3d::PhysicsSpatialSurfaceColliderBakeRequest request;
  request.surfaces = &surfaces;
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult bake =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces(request);
  const std::vector<iggy3d::Vec3> waypoints = {cellToWorld(1, 1), cellToWorld(1, 5)};
  const iggy3d::ReasoningGraph graph = iggy3d::buildReasoningGraph(room, waypoints);

  // Guard samples = the two patrolPosts (placed guards at waypoints, a6s2's shape).
  const std::vector<iggy3d::InfluenceGuardSample> guards = {{cellToWorld(1, 1)}, {cellToWorld(1, 5)}};
  const iggy3d::InfluenceMap map = iggy3d::buildInfluenceMap(graph, bake.colliders, guards);

  std::cout << "=== influence readout: stealth_garden (guards @ patrolPosts) ===\n";
  float maxEscape = -1.0F;
  std::size_t maxEscapeNode = graph.nodes.size();
  bool allFiniteNonNeg = true;
  bool visIsBinary = true;
  for (std::size_t i = 0; i < graph.nodes.size(); ++i) {
    const iggy3d::ReasoningNode& n = graph.nodes[i];
    const float gi = iggy3d::influenceValue(map, i, iggy3d::InfluenceChannel::guardInfluence);
    const float vis = iggy3d::influenceValue(map, i, iggy3d::InfluenceChannel::visibilityCoverage);
    const float esc = iggy3d::influenceValue(map, i, iggy3d::InfluenceChannel::escapeRoutePressure);
    const float obj = iggy3d::influenceValue(map, i, iggy3d::InfluenceChannel::objectiveControl);
    std::cout << "  node #" << n.id << " " << iggy3d::reasoningNodeKindName(n.kind) << " @("
              << n.positionMeters.x << "," << n.positionMeters.z << ") | guardInfluence=" << gi
              << " visibility=" << vis << " escape=" << esc << " objective=" << obj << "\n";
    for (const float v : {gi, vis, esc, obj}) {
      allFiniteNonNeg = allFiniteNonNeg && std::isfinite(v) && v >= 0.0F;
    }
    visIsBinary = visIsBinary && (vis == 0.0F || vis == 1.0F);
    if (esc > maxEscape) {
      maxEscape = esc;
      maxEscapeNode = i;
    }
  }

  bool ok = expect(!graph.nodes.empty(), "garden graph built");
  ok = ok && expect(allFiniteNonNeg, "every influence value is finite and >= 0");
  ok = ok && expect(visIsBinary, "visibilityCoverage is binary (0 or 1)");
  ok = ok && expect(maxEscapeNode < graph.nodes.size() &&
                        graph.nodes[maxEscapeNode].kind == iggy3d::ReasoningNodeKind::exit,
                    "the exit node holds the escapeRoutePressure column max");
  return ok ? 0 : 1;
}
