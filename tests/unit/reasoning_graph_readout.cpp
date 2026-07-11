// a3s2 reasoning_graph_readout — s8-style headless dashboard. Reads the garden grid fixture, builds
// the L4 reasoning graph (a3s1) + summary (a3s2), PRINTS nodes (kind + position + provenance) and
// edges to stdout, and asserts the shape matches a3s1's garden pin (5 nodes; no direct edge through
// the island) — the cross-slice consistency check. Retrieve via `ctest -V -R reasoning_graph_readout`.

#include "runtime/ai/ReasoningGraph.hpp"

#include "content/assets/RoomAsset.hpp"
#include "core/math/Vec3.hpp"

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

// Mirror stealth_garden_tests::loadGarden: bake '#' cells + a floor into spatialSurfaces, and the
// N/P/E glyphs into exit/npc/spawn anchors -- the same authored room the garden pin builds from.
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
  const iggy3d::ReasoningGraphSummary summary = iggy3d::summarizeReasoningGraph(graph);

  std::cout << "=== reasoning graph readout: stealth_garden ===\n";
  std::cout << "nodes=" << summary.nodeCount << " edges=" << summary.edgeCount << "\n";
  for (const iggy3d::ReasoningNode& n : graph.nodes) {
    std::cout << "  node #" << n.id << "  " << iggy3d::reasoningNodeKindName(n.kind) << "  @("
              << n.positionMeters.x << ", " << n.positionMeters.y << ", " << n.positionMeters.z
              << ")  <- " << n.sourceLabel << "\n";
  }
  for (const iggy3d::ReasoningEdge& e : graph.edges) {
    std::cout << "  edge " << e.from << " -> " << e.to << "  "
              << iggy3d::reasoningEdgeKindName(e.kind) << "  len=" << e.lengthMeters << "\n";
  }

  // Cross-slice consistency with a3s1's garden pin: 5 nodes = 1 exit + 2 reference + 2 patrolPost.
  const auto kindCount = [&summary](iggy3d::ReasoningNodeKind k) {
    return summary.perKindCounts[static_cast<std::size_t>(k)];
  };
  // Fail closed on a missing grid file: without it the anchors land at the
  // origin and the count pins pass against a degenerate room.
  bool ok = expect(std::ifstream(kGridPath).good(),
                   "garden grid fixture file present") &&
            expect(!room.anchors.empty(), "garden grid fixture loaded") &&
            expect(summary.nodeCount == 5U, "garden graph has 5 nodes (matches a3s1)") &&
            expect(kindCount(iggy3d::ReasoningNodeKind::exit) == 1U, "1 exit node") &&
            expect(kindCount(iggy3d::ReasoningNodeKind::reference) == 2U, "2 reference nodes") &&
            expect(kindCount(iggy3d::ReasoningNodeKind::patrolPost) == 2U, "2 patrolPost nodes");

  // Island occlusion consistency: no direct edge exit(12,1) -> bottom patrolPost(1,5).
  const auto find = [&graph](iggy3d::ReasoningNodeKind kind,
                             iggy3d::Vec3 pos) -> const iggy3d::ReasoningNode* {
    for (const iggy3d::ReasoningNode& n : graph.nodes) {
      if (n.kind == kind && iggy3d::nearlyEqual(n.positionMeters, pos, 0.01F)) {
        return &n;
      }
    }
    return nullptr;
  };
  const iggy3d::ReasoningNode* exitNode = find(iggy3d::ReasoningNodeKind::exit, cellToWorld(12, 1));
  const iggy3d::ReasoningNode* postBottom =
      find(iggy3d::ReasoningNodeKind::patrolPost, cellToWorld(1, 5));
  bool throughIsland = false;
  if (exitNode != nullptr && postBottom != nullptr) {
    for (const iggy3d::ReasoningEdge& e : graph.edges) {
      if ((e.from == exitNode->id && e.to == postBottom->id) ||
          (e.from == postBottom->id && e.to == exitNode->id)) {
        throughIsland = true;
      }
    }
  }
  ok = ok && expect(exitNode != nullptr && postBottom != nullptr,
                    "exit + bottom patrolPost nodes present") &&
       expect(!throughIsland, "no direct edge through the island (matches a3s1 garden pin)");

  // a7s1: a HAND-BUILT room authoring the affordance-vocabulary v0.1 kinds -> the graph maps each to
  // its §12 node. Prints the authored nodes so a re-authored map's shape is visible over SSH.
  iggy3d::RoomAsset affordances;
  affordances.id = "authored_affordances";
  const auto authoredAnchor = [&affordances](const char* kind, iggy3d::Vec3 pos) {
    iggy3d::RoomAnchorAsset a;
    a.id = std::string("anchor_") + kind;
    a.kind = kind;
    a.positionMeters = pos;
    affordances.anchors.push_back(std::move(a));
  };
  authoredAnchor("chokepoint", cellToWorld(1, 1));
  authoredAnchor("high_ground", cellToWorld(2, 2));
  authoredAnchor("hiding_spot", cellToWorld(3, 3));
  authoredAnchor("cover", cellToWorld(4, 4));
  authoredAnchor("patrol_post", cellToWorld(5, 5));
  authoredAnchor("monster", cellToWorld(6, 6));
  authoredAnchor("trap", cellToWorld(7, 7));  // deck metadata -> no node
  const std::vector<iggy3d::Vec3> noWaypoints;
  const iggy3d::ReasoningGraph affGraph = iggy3d::buildReasoningGraph(affordances, noWaypoints);

  std::cout << "\n=== reasoning graph readout: authored affordances (hand-built) ===\n";
  std::cout << "nodes=" << affGraph.nodes.size() << " (trap/unknown produce none)\n";
  for (const iggy3d::ReasoningNode& n : affGraph.nodes) {
    std::cout << "  node #" << n.id << "  " << iggy3d::reasoningNodeKindName(n.kind) << "  <- "
              << n.sourceLabel << "\n";
  }
  ok = ok && expect(affGraph.nodes.size() == 6U,
                    "authored affordance kinds map to 6 nodes (trap excluded)");

  return ok ? 0 : 1;
}
