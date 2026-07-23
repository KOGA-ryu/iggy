#include "EditorWorldLayoutHierarchy.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace iggy3d_creative_app {
namespace {

namespace cr = iggy3d::creative;

[[nodiscard]] char asciiLower(char value) noexcept {
  return value >= 'A' && value <= 'Z'
             ? static_cast<char>(value - 'A' + 'a')
             : value;
}

[[nodiscard]] bool asciiContains(std::string_view value,
                                 std::string_view term) noexcept {
  if (term.empty()) {
    return true;
  }
  if (term.size() > value.size()) {
    return false;
  }
  for (std::size_t start = 0U; start <= value.size() - term.size(); ++start) {
    std::size_t offset = 0U;
    while (offset < term.size() &&
           asciiLower(value[start + offset]) == asciiLower(term[offset])) {
      ++offset;
    }
    if (offset == term.size()) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] std::vector<std::string_view> queryTerms(
    std::string_view query) {
  std::vector<std::string_view> terms;
  std::size_t cursor = 0U;
  while (cursor < query.size()) {
    while (cursor < query.size() && query[cursor] == ' ') {
      ++cursor;
    }
    const std::size_t begin = cursor;
    while (cursor < query.size() && query[cursor] != ' ') {
      ++cursor;
    }
    if (begin < cursor) {
      terms.push_back(query.substr(begin, cursor - begin));
    }
  }
  return terms;
}

[[nodiscard]] bool rowMatches(
    const CreativeEditorWorldLayoutHierarchyRow& row,
    const std::vector<std::string_view>& terms) noexcept {
  for (const std::string_view term : terms) {
    if (!asciiContains(row.label, term) &&
        !asciiContains(row.typeLabel, term) &&
        !asciiContains(row.stableKey, term)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] std::string labelOrFallback(std::string_view name,
                                          std::string_view fallback,
                                          std::size_t index) {
  return name.empty() ? std::string(fallback) + " " +
                            std::to_string(index + 1U)
                      : std::string(name);
}

[[nodiscard]] std::string verticalConnectorType(
    cr::CreativeWorldLayoutVerticalConnectorKind kind) {
  switch (kind) {
    case cr::CreativeWorldLayoutVerticalConnectorKind::Stair:
      return "Stair";
    case cr::CreativeWorldLayoutVerticalConnectorKind::Ramp:
      return "Ramp";
    case cr::CreativeWorldLayoutVerticalConnectorKind::Count:
      break;
  }
  return "Connector";
}

[[nodiscard]] std::string roofApertureType(
    cr::CreativeStructuralRoofApertureKind kind) {
  switch (kind) {
    case cr::CreativeStructuralRoofApertureKind::Skylight:
      return "Skylight";
    case cr::CreativeStructuralRoofApertureKind::ChimneyClearance:
      return "Chimney clearance";
    case cr::CreativeStructuralRoofApertureKind::Count:
      break;
  }
  return "Roof aperture";
}

[[nodiscard]] std::string topologyEdgeLabel(
    const cr::CreativeWorldLayout& layout, std::size_t edgeIndex) {
  if (edgeIndex >= layout.topologyEdges.size()) {
    return "Wall";
  }
  const cr::CreativeWorldLayoutTopologyEdge& edge =
      layout.topologyEdges[edgeIndex];
  if (edge.startVertexIndex >= layout.topologyVertices.size() ||
      edge.endVertexIndex >= layout.topologyVertices.size()) {
    return "Wall " + std::to_string(edgeIndex + 1U);
  }
  const cr::CreativeTerrainCoord2 start =
      layout.topologyVertices[edge.startVertexIndex].position;
  const cr::CreativeTerrainCoord2 end =
      layout.topologyVertices[edge.endVertexIndex].position;
  return "Wall (" + std::to_string(start.x) + ", " +
         std::to_string(start.z) + ") to (" + std::to_string(end.x) +
         ", " + std::to_string(end.z) + ")";
}

class HierarchyBuilder {
 public:
  HierarchyBuilder(std::uint64_t sourceEpoch, std::uint64_t revision) {
    model_.sourceEpoch = sourceEpoch;
    model_.layoutRevision = revision;
  }

  [[nodiscard]] std::size_t addGroup(std::string label,
                                     std::string uiKey,
                                     std::size_t parentRow) {
    CreativeEditorWorldLayoutHierarchyRow row;
    row.kind = CreativeEditorWorldLayoutHierarchyRowKind::Group;
    row.parentRow = parentRow;
    row.depth = depthFor(parentRow);
    row.label = std::move(label);
    row.typeLabel = "Group";
    row.uiKey = std::move(uiKey);
    return append(std::move(row));
  }

  std::size_t addSymbol(
      cr::CreativeWorldLayoutTable table, std::size_t sourceIndex,
      std::string label, std::string typeLabel, std::string stableKey,
      std::size_t parentRow,
      CreativeEditorWorldLayoutHierarchyRecovery recovery =
          CreativeEditorWorldLayoutHierarchyRecovery::None) {
    CreativeEditorWorldLayoutHierarchyRow row;
    row.kind = CreativeEditorWorldLayoutHierarchyRowKind::Symbol;
    row.table = table;
    row.sourceIndex = sourceIndex;
    row.parentRow = parentRow;
    row.depth = depthFor(parentRow);
    row.label = std::move(label);
    row.typeLabel = std::move(typeLabel);
    row.stableKey = std::move(stableKey);
    row.uiKey = "symbol:" + std::to_string(static_cast<unsigned>(table)) +
                ":" + row.stableKey + ":" +
                std::to_string(sourceIndex);
    row.recovery = recovery;
    if (recovery != CreativeEditorWorldLayoutHierarchyRecovery::None) {
      ++model_.recoveredSymbolCount;
    }
    ++model_.sourceSymbolCount;
    return append(std::move(row));
  }

  [[nodiscard]] CreativeEditorWorldLayoutHierarchyModel finish() {
    std::vector<std::size_t> openRows;
    for (std::size_t index = 0U; index < model_.rows.size(); ++index) {
      while (!openRows.empty() &&
             model_.rows[openRows.back()].depth >= model_.rows[index].depth) {
        model_.rows[openRows.back()].subtreeEnd = index;
        openRows.pop_back();
      }
      openRows.push_back(index);
    }
    while (!openRows.empty()) {
      model_.rows[openRows.back()].subtreeEnd = model_.rows.size();
      openRows.pop_back();
    }
    return std::move(model_);
  }

 private:
  [[nodiscard]] std::uint32_t depthFor(std::size_t parentRow) const {
    return parentRow == kInvalidCreativeEditorWorldLayoutHierarchyRow
               ? 0U
               : model_.rows[parentRow].depth + 1U;
  }

  [[nodiscard]] std::size_t append(
      CreativeEditorWorldLayoutHierarchyRow row) {
    const std::size_t index = model_.rows.size();
    if (row.parentRow != kInvalidCreativeEditorWorldLayoutHierarchyRow) {
      model_.rows[row.parentRow].hasChildren = true;
    }
    model_.rows.push_back(std::move(row));
    return index;
  }

  CreativeEditorWorldLayoutHierarchyModel model_;
};

}  // namespace

CreativeEditorWorldLayoutHierarchyModel
buildCreativeEditorWorldLayoutHierarchy(
    std::uint64_t sourceEpoch, std::uint64_t layoutRevision,
    const cr::CreativeWorldLayout& layout) {
  HierarchyBuilder builder(sourceEpoch, layoutRevision);
  std::vector<bool> levelEmitted(layout.levels.size(), false);
  std::vector<bool> roomEmitted(layout.rooms.size(), false);
  std::vector<bool> topologyEdgeEmitted(layout.topologyEdges.size(), false);
  std::vector<bool> connectorEmitted(layout.verticalConnectors.size(), false);
  std::vector<bool> boxEmitted(layout.boxes.size(), false);
  std::vector<bool> wallEmitted(layout.walls.size(), false);
  std::vector<bool> openingEmitted(layout.openings.size(), false);
  std::vector<bool> roofApertureEmitted(layout.roofApertures.size(), false);

  for (std::size_t buildingIndex = 0U;
       buildingIndex < layout.buildings.size(); ++buildingIndex) {
    const cr::CreativeWorldLayoutBuilding& building =
        layout.buildings[buildingIndex];
    const std::size_t buildingRow = builder.addSymbol(
        cr::CreativeWorldLayoutTable::Building, buildingIndex,
        labelOrFallback(building.name, "Building", buildingIndex), "Building",
        building.stableKey, kInvalidCreativeEditorWorldLayoutHierarchyRow);

    std::vector<std::size_t> levels;
    for (std::size_t index = 0U; index < layout.levels.size(); ++index) {
      if (layout.levels[index].buildingIndex == buildingIndex) {
        levels.push_back(index);
      }
    }
    if (!levels.empty()) {
      const std::size_t group = builder.addGroup(
          "Levels", "group:" + building.stableKey + ":levels", buildingRow);
      for (const std::size_t levelIndex : levels) {
        const cr::CreativeWorldLayoutLevel& level = layout.levels[levelIndex];
        const std::size_t levelRow = builder.addSymbol(
            cr::CreativeWorldLayoutTable::Level, levelIndex,
            labelOrFallback(level.name, "Level", levelIndex), "Level",
            level.stableKey, group);
        levelEmitted[levelIndex] = true;
        std::vector<std::size_t> roofApertures;
        for (std::size_t apertureIndex = 0U;
             apertureIndex < layout.roofApertures.size(); ++apertureIndex) {
          if (layout.roofApertures[apertureIndex].levelIndex == levelIndex) {
            roofApertures.push_back(apertureIndex);
          }
        }
        if (!roofApertures.empty()) {
          const std::size_t aperturesGroup = builder.addGroup(
              "Roof apertures",
              "group:" + level.stableKey + ":roof-apertures", levelRow);
          for (const std::size_t apertureIndex : roofApertures) {
            const cr::CreativeWorldLayoutRoofAperture& aperture =
                layout.roofApertures[apertureIndex];
            const std::string type = roofApertureType(aperture.kind);
            builder.addSymbol(
                cr::CreativeWorldLayoutTable::RoofAperture, apertureIndex,
                labelOrFallback(aperture.name, type, apertureIndex), type,
                aperture.stableKey, aperturesGroup);
            roofApertureEmitted[apertureIndex] = true;
          }
        }
        std::vector<std::size_t> topologyEdges;
        for (std::size_t edgeIndex = 0U;
             edgeIndex < layout.topologyEdges.size(); ++edgeIndex) {
          if (layout.topologyEdges[edgeIndex].levelIndex == levelIndex) {
            topologyEdges.push_back(edgeIndex);
          }
        }
        if (!topologyEdges.empty()) {
          const std::size_t wallsGroup = builder.addGroup(
              "Walls", "group:" + level.stableKey + ":walls", levelRow);
          for (const std::size_t edgeIndex : topologyEdges) {
            const cr::CreativeWorldLayoutTopologyEdge& edge =
                layout.topologyEdges[edgeIndex];
            builder.addSymbol(cr::CreativeWorldLayoutTable::TopologyEdge,
                              edgeIndex,
                              topologyEdgeLabel(layout, edgeIndex), "Wall",
                              edge.stableKey, wallsGroup);
            topologyEdgeEmitted[edgeIndex] = true;
          }
        }
        for (std::size_t roomIndex = 0U; roomIndex < layout.rooms.size();
             ++roomIndex) {
          const cr::CreativeWorldLayoutRoom& room = layout.rooms[roomIndex];
          if (room.buildingIndex != buildingIndex ||
              room.levelIndex != levelIndex) {
            continue;
          }
          const std::size_t roomRow = builder.addSymbol(
              cr::CreativeWorldLayoutTable::Room, roomIndex,
              labelOrFallback(room.name, "Room", roomIndex), "Room",
              room.stableKey, levelRow);
          roomEmitted[roomIndex] = true;
          for (std::size_t openingIndex = 0U;
               openingIndex < layout.openings.size(); ++openingIndex) {
            const cr::CreativeWorldLayoutOpening& opening =
                layout.openings[openingIndex];
            if (opening.hostKind !=
                    cr::CreativeWorldLayoutOpeningHostKind::RoomEdge ||
                opening.roomIndex != roomIndex) {
              continue;
            }
            builder.addSymbol(
                cr::CreativeWorldLayoutTable::Opening, openingIndex,
                labelOrFallback(opening.name, "Opening", openingIndex),
                std::string(cr::toString(opening.kind)), opening.stableKey,
                roomRow);
            openingEmitted[openingIndex] = true;
          }
        }
      }
    }

    std::vector<std::size_t> boxes;
    for (std::size_t index = 0U; index < layout.boxes.size(); ++index) {
      if (layout.boxes[index].buildingIndex == buildingIndex) {
        boxes.push_back(index);
      }
    }
    if (!boxes.empty()) {
      const std::size_t group = builder.addGroup(
          "Structure", "group:" + building.stableKey + ":structure",
          buildingRow);
      for (const std::size_t index : boxes) {
        const cr::CreativeWorldLayoutBox& box = layout.boxes[index];
        builder.addSymbol(
            cr::CreativeWorldLayoutTable::Box, index,
            labelOrFallback(box.name, "Structure", index),
            std::string(cr::toString(box.kind)), box.stableKey, group);
        boxEmitted[index] = true;
      }
    }

    std::vector<std::size_t> walls;
    for (std::size_t index = 0U; index < layout.walls.size(); ++index) {
      if (layout.walls[index].buildingIndex == buildingIndex) {
        walls.push_back(index);
      }
    }
    if (!walls.empty()) {
      const std::size_t group = builder.addGroup(
          "Partitions", "group:" + building.stableKey + ":partitions",
          buildingRow);
      for (const std::size_t wallIndex : walls) {
        const cr::CreativeWorldLayoutWall& wall = layout.walls[wallIndex];
        const std::size_t wallRow = builder.addSymbol(
            cr::CreativeWorldLayoutTable::Wall, wallIndex,
            labelOrFallback(wall.name, "Wall", wallIndex), "Wall",
            wall.stableKey, group);
        wallEmitted[wallIndex] = true;
        for (std::size_t openingIndex = 0U;
             openingIndex < layout.openings.size(); ++openingIndex) {
          const cr::CreativeWorldLayoutOpening& opening =
              layout.openings[openingIndex];
          if (opening.hostKind !=
                  cr::CreativeWorldLayoutOpeningHostKind::Wall ||
              opening.wallIndex != wallIndex) {
            continue;
          }
          builder.addSymbol(
              cr::CreativeWorldLayoutTable::Opening, openingIndex,
              labelOrFallback(opening.name, "Opening", openingIndex),
              std::string(cr::toString(opening.kind)), opening.stableKey,
              wallRow);
          openingEmitted[openingIndex] = true;
        }
      }
    }

    std::vector<std::size_t> connectors;
    for (std::size_t index = 0U; index < layout.verticalConnectors.size();
         ++index) {
      if (layout.verticalConnectors[index].buildingIndex == buildingIndex) {
        connectors.push_back(index);
      }
    }
    if (!connectors.empty()) {
      const std::size_t group = builder.addGroup(
          "Vertical", "group:" + building.stableKey + ":vertical",
          buildingRow);
      for (const std::size_t index : connectors) {
        const cr::CreativeWorldLayoutVerticalConnector& connector =
            layout.verticalConnectors[index];
        const std::string type = verticalConnectorType(connector.kind);
        builder.addSymbol(
            cr::CreativeWorldLayoutTable::VerticalConnector, index,
            labelOrFallback(connector.name, type, index), type,
            connector.stableKey, group);
        connectorEmitted[index] = true;
      }
    }
  }

  if (!layout.terrainProfiles.empty() || !layout.terrainPaths.empty()) {
    const std::size_t terrain = builder.addGroup(
        "Terrain", "group:terrain",
        kInvalidCreativeEditorWorldLayoutHierarchyRow);
    for (std::size_t index = 0U; index < layout.terrainProfiles.size();
         ++index) {
      const cr::CreativeWorldLayoutTerrainProfile& profile =
          layout.terrainProfiles[index];
      const std::string type = std::string(cr::toString(profile.kind));
      builder.addSymbol(cr::CreativeWorldLayoutTable::TerrainProfile, index,
                        type + " " + std::to_string(index + 1U), type,
                        profile.stableKey, terrain);
    }
    for (std::size_t index = 0U; index < layout.terrainPaths.size(); ++index) {
      const cr::CreativeWorldLayoutTerrainPath& path =
          layout.terrainPaths[index];
      const std::string type = std::string(cr::toString(path.recipe.kind));
      builder.addSymbol(cr::CreativeWorldLayoutTable::TerrainPath, index,
                        type + " " + std::to_string(index + 1U), type,
                        path.stableKey, terrain);
    }
  }

  if (!layout.objects.empty()) {
    const std::size_t objects = builder.addGroup(
        "Objects", "group:objects",
        kInvalidCreativeEditorWorldLayoutHierarchyRow);
    for (std::size_t index = 0U; index < layout.objects.size(); ++index) {
      const cr::CreativeWorldLayoutObject& object = layout.objects[index];
      builder.addSymbol(
          cr::CreativeWorldLayoutTable::Object, index,
          labelOrFallback(object.name, "Object", index),
          std::string(cr::toString(object.kind)), object.stableKey, objects);
    }
  }

  const auto anyMissing = [](const std::vector<bool>& values) {
    return std::find(values.begin(), values.end(), false) != values.end();
  };
  if (anyMissing(levelEmitted) || anyMissing(roomEmitted) ||
      anyMissing(topologyEdgeEmitted) ||
      anyMissing(connectorEmitted) || anyMissing(boxEmitted) ||
      anyMissing(wallEmitted) || anyMissing(openingEmitted) ||
      anyMissing(roofApertureEmitted)) {
    const std::size_t unassigned = builder.addGroup(
        "Unassigned", "group:unassigned",
        kInvalidCreativeEditorWorldLayoutHierarchyRow);
    const auto recovery =
        CreativeEditorWorldLayoutHierarchyRecovery::Unassigned;
    for (std::size_t index = 0U; index < layout.levels.size(); ++index) {
      if (!levelEmitted[index]) {
        const auto& value = layout.levels[index];
        builder.addSymbol(cr::CreativeWorldLayoutTable::Level, index,
                          labelOrFallback(value.name, "Level", index),
                          "Level", value.stableKey, unassigned, recovery);
      }
    }
    for (std::size_t index = 0U; index < layout.rooms.size(); ++index) {
      if (!roomEmitted[index]) {
        const auto& value = layout.rooms[index];
        builder.addSymbol(cr::CreativeWorldLayoutTable::Room, index,
                          labelOrFallback(value.name, "Room", index), "Room",
                          value.stableKey, unassigned, recovery);
      }
    }
    for (std::size_t index = 0U; index < layout.topologyEdges.size();
         ++index) {
      if (!topologyEdgeEmitted[index]) {
        const auto& value = layout.topologyEdges[index];
        builder.addSymbol(cr::CreativeWorldLayoutTable::TopologyEdge, index,
                          topologyEdgeLabel(layout, index), "Wall",
                          value.stableKey, unassigned, recovery);
      }
    }
    for (std::size_t index = 0U; index < layout.verticalConnectors.size();
         ++index) {
      if (!connectorEmitted[index]) {
        const auto& value = layout.verticalConnectors[index];
        const std::string type = verticalConnectorType(value.kind);
        builder.addSymbol(cr::CreativeWorldLayoutTable::VerticalConnector,
                          index, labelOrFallback(value.name, type, index), type,
                          value.stableKey, unassigned, recovery);
      }
    }
    for (std::size_t index = 0U; index < layout.boxes.size(); ++index) {
      if (!boxEmitted[index]) {
        const auto& value = layout.boxes[index];
        builder.addSymbol(cr::CreativeWorldLayoutTable::Box, index,
                          labelOrFallback(value.name, "Structure", index),
                          std::string(cr::toString(value.kind)), value.stableKey,
                          unassigned, recovery);
      }
    }
    for (std::size_t index = 0U; index < layout.walls.size(); ++index) {
      if (!wallEmitted[index]) {
        const auto& value = layout.walls[index];
        builder.addSymbol(cr::CreativeWorldLayoutTable::Wall, index,
                          labelOrFallback(value.name, "Wall", index), "Wall",
                          value.stableKey, unassigned, recovery);
      }
    }
    for (std::size_t index = 0U; index < layout.openings.size(); ++index) {
      if (!openingEmitted[index]) {
        const auto& value = layout.openings[index];
        builder.addSymbol(cr::CreativeWorldLayoutTable::Opening, index,
                          labelOrFallback(value.name, "Opening", index),
                          std::string(cr::toString(value.kind)), value.stableKey,
                          unassigned, recovery);
      }
    }
    for (std::size_t index = 0U; index < layout.roofApertures.size();
         ++index) {
      if (!roofApertureEmitted[index]) {
        const auto& value = layout.roofApertures[index];
        const std::string type = roofApertureType(value.kind);
        builder.addSymbol(cr::CreativeWorldLayoutTable::RoofAperture, index,
                          labelOrFallback(value.name, type, index), type,
                          value.stableKey, unassigned, recovery);
      }
    }
  }

  return builder.finish();
}

const CreativeEditorWorldLayoutHierarchyModel&
refreshCreativeEditorWorldLayoutHierarchy(
    CreativeEditorWorldLayoutHierarchyCache& cache,
    std::uint64_t sourceEpoch, std::uint64_t layoutRevision,
    const cr::CreativeWorldLayout& layout) {
  if (cache.valid && cache.model.sourceEpoch == sourceEpoch &&
      cache.model.layoutRevision == layoutRevision) {
    return cache.model;
  }
  cache.model =
      buildCreativeEditorWorldLayoutHierarchy(sourceEpoch, layoutRevision,
                                               layout);
  cache.valid = true;
  ++cache.buildCount;
  return cache.model;
}

std::vector<std::size_t> filterCreativeEditorWorldLayoutHierarchy(
    const CreativeEditorWorldLayoutHierarchyModel& model,
    std::string_view query) {
  std::vector<std::size_t> result;
  const std::vector<std::string_view> terms = queryTerms(query);
  if (terms.empty()) {
    result.resize(model.rows.size());
    for (std::size_t index = 0U; index < result.size(); ++index) {
      result[index] = index;
    }
    return result;
  }

  std::vector<bool> keep(model.rows.size(), false);
  for (std::size_t index = 0U; index < model.rows.size(); ++index) {
    if (!rowMatches(model.rows[index], terms)) {
      continue;
    }
    std::size_t cursor = index;
    while (cursor != kInvalidCreativeEditorWorldLayoutHierarchyRow &&
           !keep[cursor]) {
      keep[cursor] = true;
      cursor = model.rows[cursor].parentRow;
    }
  }
  result.reserve(model.rows.size());
  for (std::size_t index = 0U; index < keep.size(); ++index) {
    if (keep[index]) {
      result.push_back(index);
    }
  }
  return result;
}

}  // namespace iggy3d_creative_app
