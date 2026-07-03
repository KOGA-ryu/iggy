// a5s2 guard_decision_readout -- s8-style headless dashboard. Reads the garden grid fixture, builds
// the L6 reasoning graph + colliders, runs the a5s1 kernel for the search scenario (hot guard at
// patrolPost(1,5) with a cold trail south of the island), and PRINTS the reasoning receipt -- the
// chosen node + signed factors. Retrieve via `ctest -V -R guard_decision_readout`.

#include "runtime/ai/GuardDecision.hpp"

#include "content/assets/RoomAsset.hpp"
#include "core/ids/EntityId.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/ai/NpcPersonalityWeights.hpp"
#include "runtime/ai/ReasoningGraph.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp"

#include <algorithm>
#include <cstddef>
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

constexpr const char* kGridPath = "fixtures/rooms/ascii/stealth_garden.iggyroom.txt";

iggy3d::Vec3 cellToWorld(int col, int row) {
  return iggy3d::Vec3{static_cast<float>(col), 0.0F, static_cast<float>(row)};
}

// Mirror the garden loader: '#' cells + a floor into spatialSurfaces, N/P/E into exit/npc/spawn.
iggy3d::RoomAsset buildGardenRoom() {
  iggy3d::RoomAsset room;
  room.id = "stealth_garden";
  std::ifstream file(kGridPath);
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
  const std::vector<iggy3d::Vec3> waypoints = {cellToWorld(1, 1), cellToWorld(1, 5)};
  const iggy3d::ReasoningGraph graph = iggy3d::buildReasoningGraph(room, waypoints);
  iggy3d::PhysicsSpatialSurfaceColliderBakeRequest req;
  const iggy3d::SpatialSurfaceSet surfaces = iggy3d::buildSpatialSurfaceSet(room);
  req.surfaces = &surfaces;
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult bake =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces(req);

  // Search scenario: hot guard at patrolPost(1,5), a COLD trail south of the island.
  const iggy3d::GuardDecisionConfig config;
  iggy3d::GuardMemorySample memory;
  memory.hasMemorySample = true;
  memory.lastKnownPosition = cellToWorld(6, 6);
  memory.lastKnownTick = 1;
  const std::uint64_t tick = 20;
  const iggy3d::GuardDecision decision =
      iggy3d::chooseSearchNode(graph, bake.colliders, cellToWorld(1, 5), memory, tick,
                               iggy3d::EntityId{}, iggy3d::NpcPersonalityWeights{}, config);

  std::cout << "=== guard decision readout: stealth_garden search ===\n";
  std::cout << "guard @ patrolPost(1,5), cold trail @ (6,6) recorded tick 1, decided tick " << tick
            << "\n";
  std::cout << "hasChoice=" << decision.receipt.hasChoice
            << " chosenNodeId=" << decision.receipt.chosenNodeId << " total="
            << decision.receipt.totalScore << "\n";
  if (decision.receipt.hasChoice) {
    const iggy3d::ReasoningNode& n = graph.nodes[decision.receipt.chosenNodeId];
    std::cout << "  chosen kind=" << iggy3d::reasoningNodeKindName(n.kind) << " @("
              << n.positionMeters.x << ", " << n.positionMeters.z << ")\n";
  }
  for (const iggy3d::GuardDecisionFactor& f : decision.receipt.factors) {
    std::cout << "  factor " << f.name << " = " << f.value << "\n";
  }

  return expect(decision.nodeId.has_value() && decision.receipt.hasChoice,
                "the search scenario yields a scored choice with a receipt")
             ? 0
             : 1;
}
