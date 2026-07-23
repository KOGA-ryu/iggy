#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutHistory.hpp"

#include "app/iggy3d/creative/world/WorldLayoutOrthogonalRooms.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

app::CreativeEditorWorldLayoutState explicitGroundFloor() {
  app::CreativeEditorWorldLayoutState state;
  app::resetCreativeEditorWorldLayout(state, "level_operation_test");
  static_cast<void>(app::createCreativeEditorWorldLayoutBuildingShell(
      state, {{{0, 0}, {8, 4}}, 0.0, 3U, 0.25, 1U}));
  static_cast<void>(app::setCreativeEditorWorldLayoutTool(
      state, app::CreativeEditorWorldLayoutTool::Door));
  static_cast<void>(app::applyCreativeEditorWorldLayoutPoint(state,
                                                              {2.0, 0.1}));
  static_cast<void>(app::splitCreativeEditorWorldLayoutRoom(
      state, 0U, cr::CreativeWorldLayoutRoomSplitAxis::X, 4));
  return state;
}

bool stableKeysUnique(const cr::CreativeWorldLayout& layout) {
  std::unordered_set<std::string> keys;
  const auto add = [&](const std::string& key) {
    return !key.empty() && keys.insert(key).second;
  };
  if (!add(layout.stableKey)) {
    return false;
  }
  for (const auto& value : layout.buildings) {
    if (!add(value.stableKey)) return false;
  }
  for (const auto& value : layout.levels) {
    if (!add(value.stableKey)) return false;
  }
  for (const auto& value : layout.rooms) {
    if (!add(value.stableKey)) return false;
  }
  for (const auto& value : layout.topologyVertices) {
    if (!add(value.stableKey)) return false;
  }
  for (const auto& value : layout.topologyEdges) {
    if (!add(value.stableKey)) return false;
  }
  for (const auto& value : layout.openings) {
    if (!add(value.stableKey)) return false;
  }
  for (const auto& value : layout.roofApertures) {
    if (!add(value.stableKey)) return false;
  }
  for (const auto& value : layout.verticalConnectors) {
    if (!add(value.stableKey)) return false;
  }
  return true;
}

std::vector<std::string> vertexKeys(const cr::CreativeWorldLayout& layout) {
  std::vector<std::string> keys;
  keys.reserve(layout.topologyVertices.size());
  for (const auto& vertex : layout.topologyVertices) {
    keys.push_back(vertex.stableKey);
  }
  return keys;
}

std::vector<std::string> edgeKeys(const cr::CreativeWorldLayout& layout) {
  std::vector<std::string> keys;
  keys.reserve(layout.topologyEdges.size());
  for (const auto& edge : layout.topologyEdges) {
    keys.push_back(edge.stableKey);
  }
  return keys;
}

bool duplicateReorderAndDeletePreserveExplicitTopology() {
  app::CreativeEditorWorldLayoutState state = explicitGroundFloor();
  state.source.roofApertures.push_back(
      {0U,
       cr::CreativeStructuralRoofApertureKind::Skylight,
       "level_operation_skylight",
       "Level operation skylight",
       1.25,
       2.75,
       1.0,
       2.0});
  const cr::CreativeWorldLayoutRoomGraph baselineGraph =
      cr::buildCreativeWorldLayoutRoomGraph(state.source);
  const std::size_t roomCount = state.source.rooms.size();
  const std::size_t vertexCount = state.source.topologyVertices.size();
  const std::size_t edgeCount = state.source.topologyEdges.size();
  const std::size_t boundaryCount = state.source.roomBoundaries.size();
  const std::size_t openingCount = state.source.openings.size();
  const std::vector<std::string> originalVertexKeys = vertexKeys(state.source);
  const std::vector<std::string> originalEdgeKeys = edgeKeys(state.source);
  const std::string originalLevelKey = state.source.levels[0].stableKey;
  const std::uint64_t revisionBefore = state.revision;
  const std::uint64_t historyBefore =
      app::creativeEditorWorldLayoutSourceUndoDepth(state);

  const app::CreativeEditorWorldLayoutEditReceipt duplicated =
      app::applyCreativeEditorWorldLayoutLevelOperation(
          state, app::CreativeEditorWorldLayoutLevelOperation::Duplicate, 0U,
          0U);
  const cr::CreativeWorldLayoutRoomGraph duplicatedGraph =
      cr::buildCreativeWorldLayoutRoomGraph(state.source);
  const std::string duplicateLevelKey = state.source.levels[1].stableKey;
  const bool duplicateOwnsCompleteGraph =
      duplicated.accepted && duplicated.changed && duplicatedGraph.accepted &&
      duplicatedGraph.sourceWasExplicit && state.source.levels.size() == 2U &&
      state.source.rooms.size() == roomCount * 2U &&
      state.source.topologyVertices.size() == vertexCount * 2U &&
      state.source.topologyEdges.size() == edgeCount * 2U &&
      state.source.roomBoundaries.size() == boundaryCount * 2U &&
      state.source.openings.size() == openingCount * 2U &&
      state.source.roofApertures.size() == 1U &&
      state.source.roofApertures[0].levelIndex == 1U &&
      state.source.roofApertures[0].stableKey ==
          "level_operation_skylight" &&
      state.source.openings.back().roomIndex >= roomCount &&
      state.source.openings.back().roomTopologyEdgeIndex >= edgeCount &&
      state.source.rooms[roomCount].levelIndex == 1U &&
      state.source.topologyVertices[vertexCount].levelIndex == 1U &&
      state.source.topologyEdges[edgeCount].levelIndex == 1U &&
      originalLevelKey != duplicateLevelKey && stableKeysUnique(state.source);

  cr::CreativeWorldLayoutVerticalConnector connector;
  connector.buildingIndex = 0U;
  connector.lowerRoomIndex = 0U;
  connector.upperRoomIndex = roomCount;
  connector.stableKey = "level_operation_connector";
  connector.name = "Level operation connector";
  connector.footprint = {{1, 1}, {3, 3}};
  state.source.verticalConnectors.push_back(connector);

  const app::CreativeEditorWorldLayoutEditReceipt reordered =
      app::applyCreativeEditorWorldLayoutLevelOperation(
          state, app::CreativeEditorWorldLayoutLevelOperation::MoveEarlier, 0U,
          1U);
  const cr::CreativeWorldLayoutRoomGraph reorderedGraph =
      cr::buildCreativeWorldLayoutRoomGraph(state.source);
  const bool reorderRemapsEveryLevelOwner =
      reordered.accepted && reordered.changed && reorderedGraph.accepted &&
      state.source.levels[0].stableKey == duplicateLevelKey &&
      state.source.levels[1].stableKey == originalLevelKey &&
      state.source.rooms[0].levelIndex == 1U &&
      state.source.rooms[roomCount].levelIndex == 0U &&
      state.source.topologyVertices[0].levelIndex == 1U &&
      state.source.topologyVertices[vertexCount].levelIndex == 0U &&
      state.source.topologyEdges[0].levelIndex == 1U &&
      state.source.topologyEdges[edgeCount].levelIndex == 0U &&
      state.source.roofApertures.size() == 1U &&
      state.source.roofApertures[0].levelIndex == 0U;

  const app::CreativeEditorWorldLayoutEditReceipt deleted =
      app::applyCreativeEditorWorldLayoutLevelOperation(
          state, app::CreativeEditorWorldLayoutLevelOperation::Delete, 0U, 0U);
  const cr::CreativeWorldLayoutRoomGraph finalGraph =
      cr::buildCreativeWorldLayoutRoomGraph(state.source);
  const bool deleteRestoresOriginalGraph =
      deleted.accepted && deleted.changed && finalGraph.accepted &&
      finalGraph.sourceWasExplicit && state.source.levels.size() == 1U &&
      state.source.levels[0].stableKey == originalLevelKey &&
      state.source.rooms.size() == roomCount &&
      state.source.topologyVertices.size() == vertexCount &&
      state.source.topologyEdges.size() == edgeCount &&
      state.source.roomBoundaries.size() == boundaryCount &&
      state.source.openings.size() == openingCount &&
      state.source.roofApertures.empty() &&
      state.source.verticalConnectors.empty() &&
      vertexKeys(state.source) == originalVertexKeys &&
      edgeKeys(state.source) == originalEdgeKeys &&
      state.source.rooms[0].levelIndex == 0U &&
      state.source.topologyVertices[0].levelIndex == 0U &&
      state.source.topologyEdges[0].levelIndex == 0U &&
      state.activeLevelIndex == 0U;

  return expect(baselineGraph.accepted && baselineGraph.sourceWasExplicit,
                "fixture starts with canonical explicit room topology") &&
         expect(duplicateOwnsCompleteGraph,
                "level duplicate copies the complete canonical floor plan") &&
         expect(reorderRemapsEveryLevelOwner,
                "level reorder remaps rooms vertices and edges together") &&
         expect(deleteRestoresOriginalGraph,
                "level delete compacts topology openings and connectors") &&
         expect(state.revision == revisionBefore + 3U &&
                    app::creativeEditorWorldLayoutSourceUndoDepth(state) ==
                        historyBefore + 3U,
                "each level mutation owns exactly one source history entry");
}

bool invalidTopologyRejectsWithoutConsumingIdentity() {
  app::CreativeEditorWorldLayoutState state = explicitGroundFloor();
  state.source.topologyEdges[0].startVertexIndex =
      cr::kInvalidCreativeWorldLayoutIndex;
  const cr::CreativeWorldLayout sourceBefore = state.source;
  const std::uint64_t revisionBefore = state.revision;
  const std::uint64_t ordinalBefore = state.nextStableOrdinal;
  const std::uint64_t historyBefore =
      app::creativeEditorWorldLayoutSourceUndoDepth(state);

  const app::CreativeEditorWorldLayoutEditReceipt rejected =
      app::applyCreativeEditorWorldLayoutLevelOperation(
          state, app::CreativeEditorWorldLayoutLevelOperation::Duplicate, 0U,
          0U);

  return expect(!rejected.accepted && !rejected.changed,
                "invalid source topology rejects level duplication") &&
         expect(state.revision == revisionBefore &&
                    state.nextStableOrdinal == ordinalBefore &&
                    app::creativeEditorWorldLayoutSourceUndoDepth(state) ==
                        historyBefore &&
                    state.source.levels.size() == sourceBefore.levels.size() &&
                    state.source.rooms.size() == sourceBefore.rooms.size() &&
                    state.source.topologyVertices.size() ==
                        sourceBefore.topologyVertices.size() &&
                    state.source.topologyEdges[0].startVertexIndex ==
                        cr::kInvalidCreativeWorldLayoutIndex,
                "rejected level mutation changes no source identity or history");
}

bool newLevelsPreserveFloorDatumSpacing() {
  app::CreativeEditorWorldLayoutState state = explicitGroundFloor();
  state.source.levels[0].wallHeightCells = 9U;
  const app::CreativeEditorWorldLayoutEditReceipt added =
      app::applyCreativeEditorWorldLayoutLevelOperation(
          state, app::CreativeEditorWorldLayoutLevelOperation::Add, 0U);
  if (!added.accepted || state.source.levels.size() != 2U) {
    return expect(false, "first added level uses the building spacing");
  }

  state.source.levels[1].wallHeightCells = 8U;
  const app::CreativeEditorWorldLayoutEditReceipt duplicated =
      app::applyCreativeEditorWorldLayoutLevelOperation(
          state, app::CreativeEditorWorldLayoutLevelOperation::Duplicate, 0U,
          0U);
  return expect(state.source.levels[1].floorTopLayer == 3.0,
                "single-level fallback uses the building floor spacing") &&
         expect(duplicated.accepted && duplicated.changed &&
                    state.source.levels.size() == 3U &&
                    state.source.levels[2].floorTopLayer == 6.0,
                "later levels continue the floor-datum interval instead of wall height");
}

}  // namespace

int main() {
  const bool ok = duplicateReorderAndDeletePreserveExplicitTopology() &&
                  invalidTopologyRejectsWithoutConsumingIdentity() &&
                  newLevelsPreserveFloorDatumSpacing();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
