#include "app/iggy3d/creative/world/WorldLayout.hpp"
#include "app/iggy3d/creative/world/WorldLayoutCompileInternal.hpp"
#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRoofs.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"
#include "app/iggy3d/creative/world/WorldLayoutVerticalConnectors.hpp"

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/recipes/StructuralSurfaceRecipe.hpp"
#include "app/iggy3d/creative/recipes/TerrainGrounding.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace iggy3d::creative {

using world_layout_compile::appendTagOnce;
using world_layout_compile::childKey;
using world_layout_compile::collectObjectRemovalOrder;
using world_layout_compile::layoutBounds;
using world_layout_compile::layoutPoint;
using world_layout_compile::registerKey;
using world_layout_compile::setStatus;
using world_layout_compile::shiftBuildingVertically;
using world_layout_compile::stageWorldLayoutTerrain;
using world_layout_compile::validDocument;
using world_layout_compile::validRect;
using world_layout_compile::validStableKey;
using world_layout_compile::validTerrainOwnership;
using world_layout_compile::worldCoordinate;

CreativeWorldLayoutCompileResult buildCreativeWorldLayoutPlan(
    const CreativeDocument& document,
    const CreativeWorldLayout& layout,
    CreativeWorldLayoutCompileOptions options) {
  CreativeWorldLayoutCompileResult result;
  result.receipt.requested = true;
  if (!validDocument(document)) {
    setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidDocument,
              "creative_world_layout_document_invalid");
    return result;
  }
  if (layout.schemaVersion != kCreativeWorldLayoutSchemaVersion ||
      !validStableKey(layout.stableKey) ||
      !validTerrainOwnership(layout.terrainOwnership)) {
    setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSchema,
              "creative_world_layout_schema_invalid");
    return result;
  }

  const CreativeWorldLayoutRoomCompileResult roomExpansion =
      expandCreativeWorldLayoutRooms(layout);
  if (!roomExpansion.accepted) {
    result.receipt.failedTable = CreativeWorldLayoutTable::Room;
    if (roomExpansion.status ==
        CreativeWorldLayoutRoomCompileStatus::InvalidOpeningHost) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Opening;
    } else if (roomExpansion.status ==
               CreativeWorldLayoutRoomCompileStatus::InvalidLevel) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Level;
    }
    result.receipt.failedIndex = roomExpansion.failedIndex;
    setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
              roomExpansion.reasonCode);
    return result;
  }
  const CreativeWorldLayout& expanded = roomExpansion.expanded;

  result.plan.layoutKey = layout.stableKey;
  result.plan.sourceDocumentId = document.id();
  result.plan.sourceDocumentRevision = document.revision();
  result.plan.sourceTerrainRevision = document.terrainField().revision();
  result.plan.sourceTerrainHeightRevision =
      document.terrainHeightField().revision();
  result.plan.sourceMaterialRevision =
      document.terrainMaterialField().revision();

  const std::string layoutTag = creativeWorldLayoutTag(layout.stableKey);
  std::vector<CreativeRecipePlan> desiredObjectRecipes;
  std::vector<CreativeRecipePlan> desiredLibraryRecipes;

  std::unordered_set<std::string> stableKeys;
  std::vector<CreativeBuildingRecipeRequest> buildings(layout.buildings.size());
  const CreativeGridSettings grid = document.gridSettings();
  for (std::size_t index = 0U; index < layout.buildings.size(); ++index) {
    const CreativeWorldLayoutBuilding& symbol = layout.buildings[index];
    if (!registerKey(stableKeys, symbol.stableKey,
                     CreativeWorldLayoutTable::Building, index,
                     result.receipt)) {
      return result;
    }
    if (symbol.name.empty() ||
        (symbol.rootMode != CreativeBuildingRootMode::None &&
         symbol.rootMode != CreativeBuildingRootMode::CreateRoom) ||
        symbol.groundingMode >= CreativeWorldLayoutGroundingMode::Count) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Building;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                "creative_world_layout_building_invalid");
      return result;
    }
    CreativeBuildingRecipeRequest& building = buildings[index];
    building.stableKey = childKey(layout.stableKey, symbol.stableKey);
    building.name = symbol.name;
    building.rootMode = symbol.rootMode;
    building.visible = symbol.visible;
    building.tags = symbol.tags;
    appendTagOnce(building.tags, layoutTag);
    appendTagOnce(building.tags, creativeWorldLayoutProvenanceTag(
                                     layout,
                                     CreativeWorldLayoutTable::Building,
                                     index));
    if (symbol.rootMode == CreativeBuildingRootMode::CreateRoom &&
        !layoutBounds(grid, symbol.rootFootprint, symbol.rootBaseLayer,
                      symbol.rootHeightCells, building.rootBounds)) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Building;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                "creative_world_layout_building_bounds_invalid");
      return result;
    }
  }

  for (std::size_t index = 0U; index < layout.levels.size(); ++index) {
    const CreativeWorldLayoutLevel& symbol = layout.levels[index];
    if (symbol.buildingIndex >= buildings.size() || symbol.name.empty() ||
        !std::isfinite(symbol.floorTopLayer) ||
        symbol.wallHeightCells == 0U || symbol.floorThicknessLayers == 0U ||
        symbol.ceilingThicknessLayers == 0U ||
        symbol.roofThicknessLayers == 0U) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Level;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                "creative_world_layout_level_invalid");
      return result;
    }
    const std::string key = childKey(
        layout.buildings[symbol.buildingIndex].stableKey, symbol.stableKey);
    if (!registerKey(stableKeys, key, CreativeWorldLayoutTable::Level, index,
                     result.receipt)) {
      return result;
    }
  }

  for (std::size_t index = 0U; index < layout.rooms.size(); ++index) {
    const CreativeWorldLayoutRoom& symbol = layout.rooms[index];
    if (symbol.buildingIndex >= buildings.size() ||
        symbol.levelIndex >= layout.levels.size() ||
        layout.levels[symbol.levelIndex].buildingIndex !=
            symbol.buildingIndex ||
        symbol.name.empty()) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Room;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                "creative_world_layout_room_invalid");
      return result;
    }
    const std::string key = childKey(
        layout.buildings[symbol.buildingIndex].stableKey, symbol.stableKey);
    if (!registerKey(stableKeys, key, CreativeWorldLayoutTable::Room, index,
                     result.receipt)) {
      return result;
    }
  }

  std::vector<CreativeWorldLayoutVerticalConnectorPlan> connectorPlans;
  connectorPlans.reserve(layout.verticalConnectors.size());
  for (std::size_t index = 0U; index < layout.verticalConnectors.size();
       ++index) {
    const CreativeWorldLayoutVerticalConnector& symbol =
        layout.verticalConnectors[index];
    const CreativeWorldLayoutVerticalConnectorPlan connector =
        planCreativeWorldLayoutVerticalConnector(grid, layout, index);
    if (!connector.accepted) {
      result.receipt.failedTable = CreativeWorldLayoutTable::VerticalConnector;
      result.receipt.failedIndex = index;
      result.receipt.kernelReasonCode = connector.reasonCode;
      setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                "creative_world_layout_vertical_connector_rejected");
      return result;
    }
    const std::string key = childKey(
        layout.buildings[symbol.buildingIndex].stableKey, symbol.stableKey);
    if (!registerKey(stableKeys, key,
                     CreativeWorldLayoutTable::VerticalConnector, index,
                     result.receipt)) {
      return result;
    }
    connectorPlans.push_back(connector);
  }

  for (std::size_t index = 0U; index < expanded.boxes.size(); ++index) {
    const CreativeWorldLayoutBox& symbol = expanded.boxes[index];
    if (symbol.buildingIndex >= buildings.size() || symbol.name.empty()) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Box;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                "creative_world_layout_box_invalid");
      return result;
    }
    const std::string key = childKey(
        layout.buildings[symbol.buildingIndex].stableKey, symbol.stableKey);
    if (!registerKey(stableKeys, key, CreativeWorldLayoutTable::Box, index,
                     result.receipt)) {
      return result;
    }
    CreativeBounds bounds;
    const CreativeStructuralSurfaceAnchor surfaceAnchor =
        creativeStructuralSurfaceAnchor(symbol.kind);
    if (surfaceAnchor == CreativeStructuralSurfaceAnchor::None) {
      if (!layoutBounds(grid, symbol.footprint, symbol.anchorLayer,
                        symbol.layerCount, bounds)) {
        result.receipt.failedTable = CreativeWorldLayoutTable::Box;
        result.receipt.failedIndex = index;
        setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                  "creative_world_layout_box_bounds_invalid");
        return result;
      }
    } else {
      CreativeStructuralSurfaceRecipeRequest surface;
      surface.kind = symbol.kind;
      surface.layerCount = symbol.layerCount;
      if (!validRect(symbol.footprint) ||
          !std::isfinite(grid.cellSizeMeters) || grid.cellSizeMeters <= 0.0 ||
          !worldCoordinate(grid.origin.x, grid.cellSizeMeters,
                           symbol.footprint.minimum.x, surface.minimumX) ||
          !worldCoordinate(grid.origin.x, grid.cellSizeMeters,
                           symbol.footprint.maximum.x, surface.maximumX) ||
          !worldCoordinate(grid.origin.z, grid.cellSizeMeters,
                           symbol.footprint.minimum.z, surface.minimumZ) ||
          !worldCoordinate(grid.origin.z, grid.cellSizeMeters,
                           symbol.footprint.maximum.z, surface.maximumZ) ||
          !worldCoordinate(grid.origin.y, grid.cellSizeMeters,
                           symbol.anchorLayer, surface.anchorPlaneMeters)) {
        result.receipt.failedTable = CreativeWorldLayoutTable::Box;
        result.receipt.failedIndex = index;
        setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                  "creative_world_layout_box_bounds_invalid");
        return result;
      }
      const CreativeStructuralSurfaceRecipeResult planned =
          planCreativeStructuralSurface(surface);
      if (!planned.accepted) {
        result.receipt.failedTable = CreativeWorldLayoutTable::Box;
        result.receipt.failedIndex = index;
        result.receipt.kernelReasonCode = planned.reasonCode;
        setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                  "creative_world_layout_box_geometry_rejected");
        return result;
      }
      bounds = planned.bounds;
    }
    CreativeBuildingBoxSpec box{symbol.kind, key, symbol.name, bounds};
    appendTagOnce(box.tags, creativeWorldLayoutProvenanceTag(
                                layout, CreativeWorldLayoutTable::Box,
                                index));
    buildings[symbol.buildingIndex].boxes.push_back(std::move(box));
  }

  for (std::size_t index = 0U; index < layout.rooms.size(); ++index) {
    const CreativeWorldLayoutRoom& symbol = layout.rooms[index];
    const CreativeRectangularRoomGeometryPlan geometry =
        planCreativeWorldLayoutRoomGeometry(grid, layout, index);
    const CreativeWorldLayoutResolvedRoomGeometry resolved =
        resolveCreativeWorldLayoutRoomGeometry(layout, index);
    if (!geometry.accepted || !resolved.valid) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Room;
      result.receipt.failedIndex = index;
      result.receipt.kernelReasonCode = geometry.reasonCode;
      setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                "creative_world_layout_room_geometry_rejected");
      return result;
    }
    const CreativeWorldLayoutVerticalConnectorPlan* floorCutout = nullptr;
    const CreativeWorldLayoutVerticalConnectorPlan* upperCutout = nullptr;
    for (const CreativeWorldLayoutVerticalConnectorPlan& connector :
         connectorPlans) {
      if (connector.upperRoomIndex == index) {
        floorCutout = &connector;
      }
      if (connector.lowerRoomIndex == index) {
        upperCutout = &connector;
      }
    }

    const auto appendSurface =
        [&](CreativeObjectKind kind, double anchorLayer,
            std::uint16_t layerCount, std::string_view suffix,
            std::string_view label,
            const CreativeWorldLayoutVerticalConnectorPlan* cutout) {
          CreativeStructuralSurfaceRecipeRequest surface;
          surface.kind = kind;
          surface.layerCount = layerCount;
          if (!worldCoordinate(grid.origin.x, grid.cellSizeMeters,
                               symbol.footprint.minimum.x, surface.minimumX) ||
              !worldCoordinate(grid.origin.x, grid.cellSizeMeters,
                               symbol.footprint.maximum.x, surface.maximumX) ||
              !worldCoordinate(grid.origin.z, grid.cellSizeMeters,
                               symbol.footprint.minimum.z, surface.minimumZ) ||
              !worldCoordinate(grid.origin.z, grid.cellSizeMeters,
                               symbol.footprint.maximum.z, surface.maximumZ) ||
              !worldCoordinate(grid.origin.y, grid.cellSizeMeters, anchorLayer,
                               surface.anchorPlaneMeters)) {
            result.receipt.failedTable = CreativeWorldLayoutTable::Room;
            result.receipt.failedIndex = index;
            setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                      "creative_world_layout_room_surface_invalid");
            return false;
          }

          std::array<CreativeStructuralSurfaceRecipeResult,
                     kCreativeStructuralSurfaceCutoutPieceCapacity>
              pieces{};
          std::size_t pieceCount = 1U;
          if (cutout == nullptr) {
            pieces[0] = planCreativeStructuralSurface(surface);
            if (!pieces[0].accepted) {
              result.receipt.kernelReasonCode = pieces[0].reasonCode;
              result.receipt.failedTable = CreativeWorldLayoutTable::Room;
              result.receipt.failedIndex = index;
              setStatus(result.receipt,
                        CreativeWorldLayoutStatus::KernelRejected,
                        "creative_world_layout_room_surface_rejected");
              return false;
            }
          } else {
            CreativeStructuralSurfaceCutoutRequest cutoutRequest;
            cutoutRequest.surface = surface;
            if (!worldCoordinate(grid.origin.x, grid.cellSizeMeters,
                                 cutout->openingFootprint.minimum.x,
                                 cutoutRequest.cutoutMinimumX) ||
                !worldCoordinate(grid.origin.x, grid.cellSizeMeters,
                                 cutout->openingFootprint.maximum.x,
                                 cutoutRequest.cutoutMaximumX) ||
                !worldCoordinate(grid.origin.z, grid.cellSizeMeters,
                                 cutout->openingFootprint.minimum.z,
                                 cutoutRequest.cutoutMinimumZ) ||
                !worldCoordinate(grid.origin.z, grid.cellSizeMeters,
                                 cutout->openingFootprint.maximum.z,
                                 cutoutRequest.cutoutMaximumZ)) {
              result.receipt.failedTable =
                  CreativeWorldLayoutTable::VerticalConnector;
              result.receipt.failedIndex =
                  static_cast<std::size_t>(cutout - connectorPlans.data());
              setStatus(
                  result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                  "creative_world_layout_vertical_connector_cutout_invalid");
              return false;
            }
            const CreativeStructuralSurfaceCutoutResult cut =
                planCreativeStructuralSurfaceCutout(cutoutRequest);
            if (!cut.accepted || cut.pieceCount == 0U) {
              result.receipt.failedTable =
                  CreativeWorldLayoutTable::VerticalConnector;
              result.receipt.failedIndex =
                  static_cast<std::size_t>(cutout - connectorPlans.data());
              result.receipt.kernelReasonCode = cut.reasonCode;
              setStatus(
                  result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                  "creative_world_layout_vertical_connector_cutout_rejected");
              return false;
            }
            pieces = cut.pieces;
            pieceCount = cut.pieceCount;
          }

          for (std::size_t pieceIndex = 0U; pieceIndex < pieceCount;
               ++pieceIndex) {
            const std::string localKey =
                symbol.stableKey + std::string(suffix) +
                (cutout == nullptr
                     ? std::string{}
                     : ".part." + std::to_string(pieceIndex + 1U));
            const std::string key = childKey(
                layout.buildings[symbol.buildingIndex].stableKey, localKey);
            if (!registerKey(stableKeys, key, CreativeWorldLayoutTable::Room,
                             index, result.receipt)) {
              return false;
            }
            CreativeBuildingBoxSpec box{
                kind, key,
                symbol.name + std::string(label) +
                    (cutout == nullptr
                         ? std::string{}
                         : " Part " + std::to_string(pieceIndex + 1U)),
                pieces[pieceIndex].bounds};
            appendTagOnce(box.tags,
                          creativeWorldLayoutProvenanceTag(
                              layout, CreativeWorldLayoutTable::Room, index));
            buildings[symbol.buildingIndex].boxes.push_back(std::move(box));
          }
          return true;
        };

    if (!appendSurface(CreativeObjectKind::Floor, resolved.floorTopLayer,
                       resolved.floorThicknessLayers, ".floor", " Floor",
                       floorCutout)) {
      return result;
    }
    const bool roof = resolved.upperSurfaceKind == CreativeObjectKind::Roof;
    const CreativeWorldLayoutLevel& level =
        layout.levels[resolved.levelIndex];
    const bool usesAuthoredLevelRoof =
        level.roofStyle == CreativeStructuralRoofStyle::Gable ||
        level.roofOverhangCells > 0.0;
    if (roof && usesAuthoredLevelRoof) {
      const bool firstRoomForLevel = std::none_of(
          layout.rooms.begin(), layout.rooms.begin() +
                                    static_cast<std::ptrdiff_t>(index),
          [&](const CreativeWorldLayoutRoom& room) {
            return room.levelIndex == resolved.levelIndex;
          });
      if (!firstRoomForLevel) {
        continue;
      }
      const CreativeWorldLayoutRoofPlan roofPlan =
          planCreativeWorldLayoutRoof(grid, layout, resolved.levelIndex);
      if (!roofPlan.accepted) {
        result.receipt.failedTable = CreativeWorldLayoutTable::Level;
        result.receipt.failedIndex = resolved.levelIndex;
        result.receipt.kernelReasonCode = roofPlan.reasonCode;
        setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                  "creative_world_layout_roof_rejected");
        return result;
      }
      constexpr std::array<std::string_view,
                           kCreativeStructuralRoofPartCapacity>
          kPartSuffixes{".roof.base", ".roof.slope.first",
                        ".roof.slope.second"};
      constexpr std::array<std::string_view,
                           kCreativeStructuralRoofPartCapacity>
          kPartLabels{" Roof Base", " Roof Slope 1", " Roof Slope 2"};
      for (std::size_t partIndex = 0U;
           partIndex < roofPlan.geometry.partCount; ++partIndex) {
        const CreativeStructuralRoofPart& part =
            roofPlan.geometry.parts[partIndex];
        const std::string key = childKey(
            layout.buildings[level.buildingIndex].stableKey,
            level.stableKey + std::string(kPartSuffixes[partIndex]));
        if (!registerKey(stableKeys, key, CreativeWorldLayoutTable::Level,
                         resolved.levelIndex, result.receipt)) {
          return result;
        }
        CreativeBuildingBoxSpec box{
            part.kind, key, level.name + std::string(kPartLabels[partIndex]),
            part.bounds, {1.0, 1.0, 1.0}, {},
            part.rotationEulerRadians};
        appendTagOnce(
            box.tags,
            creativeWorldLayoutProvenanceTag(
                layout, CreativeWorldLayoutTable::Level,
                resolved.levelIndex));
        buildings[level.buildingIndex].boxes.push_back(std::move(box));
      }
      continue;
    }
    if (!appendSurface(resolved.upperSurfaceKind,
                       resolved.floorTopLayer + resolved.wallHeightCells,
                       resolved.upperSurfaceThicknessLayers,
                       roof ? ".roof" : ".ceiling", roof ? " Roof" : " Ceiling",
                       upperCutout)) {
      return result;
    }
  }

  for (std::size_t index = 0U; index < connectorPlans.size(); ++index) {
    const CreativeWorldLayoutVerticalConnector& symbol =
        layout.verticalConnectors[index];
    const CreativeWorldLayoutVerticalConnectorPlan& connector =
        connectorPlans[index];
    const std::string key = childKey(
        layout.buildings[symbol.buildingIndex].stableKey, symbol.stableKey);
    CreativeBuildingBoxSpec box{connector.objectKind,
                                key,
                                symbol.name,
                                connector.authoredBounds,
                                {1.0, 1.0, 1.0},
                                {},
                                connector.rotationEulerRadians};
    appendTagOnce(
        box.tags,
        creativeWorldLayoutProvenanceTag(
            layout, CreativeWorldLayoutTable::VerticalConnector, index));
    buildings[symbol.buildingIndex].boxes.push_back(std::move(box));
  }

  std::vector<std::size_t> localWallIndices(
      expanded.walls.size(), kInvalidCreativeWorldLayoutIndex);
  for (std::size_t index = 0U; index < expanded.walls.size(); ++index) {
    const CreativeWorldLayoutWall& symbol = expanded.walls[index];
    if (symbol.buildingIndex >= buildings.size() || symbol.name.empty() ||
        !std::isfinite(symbol.baseLayer) ||
        symbol.heightCells == 0U || !std::isfinite(symbol.thicknessCells) ||
        symbol.thicknessCells <= 0.0) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Wall;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                "creative_world_layout_wall_invalid");
      return result;
    }
    const std::string key = childKey(
        layout.buildings[symbol.buildingIndex].stableKey, symbol.stableKey);
    if (!registerKey(stableKeys, key, CreativeWorldLayoutTable::Wall, index,
                     result.receipt)) {
      return result;
    }
    CreativeBuildingWallSpec wall;
    wall.stableKey = key;
    wall.name = symbol.name;
    wall.heightMeters = symbol.heightCells * grid.cellSizeMeters;
    wall.thicknessMeters = symbol.thicknessCells * grid.cellSizeMeters;
    if (index < layout.walls.size()) {
      appendTagOnce(wall.tags, creativeWorldLayoutProvenanceTag(
                                   layout, CreativeWorldLayoutTable::Wall,
                                   index));
    } else if (index < roomExpansion.wallProvenance.size()) {
      for (const auto& contributor :
           roomExpansion.wallProvenance[index].contributors) {
        appendTagOnce(wall.tags, creativeWorldLayoutRoomEdgeProvenanceTag(
                                     layout, contributor.roomIndex,
                                     contributor.roomEdge));
      }
    }
    if (!layoutPoint(grid, symbol.start, symbol.baseLayer, wall.start) ||
        !layoutPoint(grid, symbol.end, symbol.baseLayer, wall.end)) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Wall;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                "creative_world_layout_wall_coordinate_invalid");
      return result;
    }
    localWallIndices[index] = buildings[symbol.buildingIndex].walls.size();
    buildings[symbol.buildingIndex].walls.push_back(std::move(wall));
  }

  for (std::size_t index = 0U; index < expanded.openings.size(); ++index) {
    const CreativeWorldLayoutOpening& symbol = expanded.openings[index];
    if (symbol.hostKind != CreativeWorldLayoutOpeningHostKind::Wall ||
        symbol.wallIndex >= expanded.walls.size() || symbol.name.empty()) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Opening;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                "creative_world_layout_opening_invalid");
      return result;
    }
    const CreativeWorldLayoutWall& wallSymbol = expanded.walls[symbol.wallIndex];
    const std::size_t buildingIndex = wallSymbol.buildingIndex;
    const std::string key = childKey(
        layout.buildings[buildingIndex].stableKey, symbol.stableKey);
    if (!registerKey(stableKeys, key, CreativeWorldLayoutTable::Opening, index,
                     result.receipt)) {
      return result;
    }
    CreativeBuildingOpeningSpec opening;
    opening.kind = symbol.kind;
    opening.pose = symbol.pose;
    opening.stableKey = key;
    opening.name = symbol.name;
    opening.centerOffsetMeters =
        symbol.centerOffsetCells * grid.cellSizeMeters;
    opening.widthMeters = symbol.widthCells * grid.cellSizeMeters;
    opening.cutoutBottomMeters =
        symbol.cutoutBottomCells * grid.cellSizeMeters;
    opening.cutoutHeightMeters =
        symbol.cutoutHeightCells * grid.cellSizeMeters;
    opening.includeInsert = symbol.includeInsert;
    opening.insertBottomMeters =
        (symbol.kind == CreativeBuildingOpeningKind::Window &&
         symbol.insertBottomCells == 0.0)
            ? opening.cutoutBottomMeters
            : symbol.insertBottomCells * grid.cellSizeMeters;
    opening.insertHeightMeters =
        symbol.insertHeightCells * grid.cellSizeMeters;
    opening.insertWidthMeters = symbol.insertWidthCells * grid.cellSizeMeters;
    opening.insertThicknessMeters =
        symbol.insertThicknessCells * grid.cellSizeMeters;
    opening.insertAssetId = symbol.insertAssetId;
    opening.insertAssetSourceBoundsMeters =
        symbol.insertAssetSourceBoundsMeters;
    opening.hasInsertAssetSourceBounds =
        symbol.hasInsertAssetSourceBounds;
    appendTagOnce(opening.tags, creativeWorldLayoutProvenanceTag(
                                    layout,
                                    CreativeWorldLayoutTable::Opening,
                                    index));
    const CreativeWorldLayoutOpening& sourceOpening = layout.openings[index];
    if (sourceOpening.hostKind ==
            CreativeWorldLayoutOpeningHostKind::RoomEdge &&
        sourceOpening.roomIndex < layout.rooms.size()) {
      appendTagOnce(opening.tags,
                    creativeWorldLayoutRoomEdgeProvenanceTag(
                        layout, sourceOpening.roomIndex,
                        sourceOpening.roomEdge));
    } else if (sourceOpening.hostKind ==
                   CreativeWorldLayoutOpeningHostKind::Wall &&
               sourceOpening.wallIndex < layout.walls.size()) {
      appendTagOnce(opening.tags, creativeWorldLayoutProvenanceTag(
                                      layout,
                                      CreativeWorldLayoutTable::Wall,
                                      sourceOpening.wallIndex));
    }
    buildings[buildingIndex]
        .walls[localWallIndices[symbol.wallIndex]]
        .openings.push_back(std::move(opening));
  }

  for (std::size_t index = 0U; index < layout.objects.size(); ++index) {
    const CreativeWorldLayoutObject& symbol = layout.objects[index];
    if (!registerKey(stableKeys, symbol.stableKey,
                     CreativeWorldLayoutTable::Object, index,
                     result.receipt)) {
      return result;
    }
    if (symbol.name.empty() ||
        symbol.mode >= CreativeObjectLibraryPlacementMode::Count) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Object;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                "creative_world_layout_object_invalid");
      return result;
    }

    CreativeObjectLibraryRecipeRequest objectRequest;
    objectRequest.stableKey =
        layout.stableKey + ".objects." + symbol.stableKey;
    objectRequest.name = symbol.name;
    CreativeObjectLibraryPlacementSpec placement;
    placement.kind = symbol.kind;
    placement.mode = symbol.mode;
    placement.stableKey = symbol.stableKey;
    placement.name = symbol.name;
    placement.assetId = symbol.assetId;
    placement.assetSourceBounds = symbol.assetSourceBoundsMeters;
    placement.hasAssetSourceBounds = symbol.hasAssetSourceBounds;
    placement.yawRadians = symbol.yawRadians;
    placement.scale = symbol.scale;
    placement.visible = symbol.visible;
    placement.tags = symbol.tags;
    appendTagOnce(placement.tags, layoutTag);
    appendTagOnce(placement.tags, creativeWorldLayoutProvenanceTag(
                                      layout,
                                      CreativeWorldLayoutTable::Object,
                                      index));
    const bool positionReady =
        symbol.mode == CreativeObjectLibraryPlacementMode::Bounds
            ? layoutBounds(grid, symbol.boundsCells, placement.bounds)
            : layoutPoint(grid, symbol.pointCells, placement.point);
    if (!positionReady) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Object;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                "creative_world_layout_object_coordinate_invalid");
      return result;
    }
    objectRequest.placements.push_back(std::move(placement));

    CreativeObjectLibraryRecipeResult objects =
        buildCreativeObjectLibraryRecipe(objectRequest);
    if (!objects.receipt.accepted) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Object;
      result.receipt.failedIndex = objects.receipt.failedPlacementIndex;
      result.receipt.kernelReasonCode = objects.receipt.reasonCode;
      setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                "creative_world_layout_object_recipe_rejected");
      return result;
    }
    objects.plan.definitionFingerprint =
        fingerprintCreativeRecipePlan(objects.plan);
    if (objects.plan.definitionFingerprint == 0U) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Object;
      result.receipt.failedIndex = index;
      result.receipt.kernelReasonCode =
          "creative_recipe_definition_fingerprint_invalid";
      setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                "creative_world_layout_object_fingerprint_rejected");
      return result;
    }
    desiredLibraryRecipes.push_back(std::move(objects.plan));
  }

  CreativeDocument terrainStaged = document;
  if (!stageWorldLayoutTerrain(document, layout, stableKeys, result,
                               terrainStaged)) {
    return result;
  }
  const CreativeTerrainSurfacePlan terrainSurface =
      buildCreativeComposedTerrainSurfacePlan(
          terrainStaged.terrainField(), document.terrainHeightField());
  if (!terrainSurface.accepted) {
    result.receipt.kernelReasonCode = terrainSurface.reasonCode;
    setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
              "creative_world_layout_terrain_surface_rejected");
    return result;
  }

  const double floorLayerThickness =
      defaultCreativeStructuralLayerThicknessMeters(CreativeObjectKind::Floor);
  for (std::size_t index = 0U; index < buildings.size(); ++index) {
    const CreativeWorldLayoutBuilding& symbol = layout.buildings[index];
    if (symbol.groundingMode ==
        CreativeWorldLayoutGroundingMode::Foundation) {
      double authoredGroundLayer =
          std::numeric_limits<double>::infinity();
      for (const CreativeWorldLayoutLevel& level : layout.levels) {
        if (level.buildingIndex == index) {
          const double floorThicknessLayers =
              static_cast<double>(level.floorThicknessLayers) *
              floorLayerThickness / grid.cellSizeMeters;
          authoredGroundLayer =
              std::min(authoredGroundLayer,
                       level.floorTopLayer - floorThicknessLayers);
        }
      }
      if (!std::isfinite(authoredGroundLayer)) {
        authoredGroundLayer = static_cast<double>(symbol.rootBaseLayer);
      }
      const CreativeTerrainGroundingPlan grounding =
          planCreativeTerrainGrounding(
              {&terrainSurface, symbol.rootFootprint.minimum,
               symbol.rootFootprint.maximum, authoredGroundLayer,
               symbol.maximumGroundReliefCells});
      if (!grounding.accepted) {
        result.receipt.failedTable = CreativeWorldLayoutTable::Building;
        result.receipt.failedIndex = index;
        result.receipt.kernelReasonCode = grounding.reasonCode;
        setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                  "creative_world_layout_building_grounding_rejected");
        return result;
      }

      const double offsetMeters =
          grounding.verticalOffsetLayers * grid.cellSizeMeters;
      if (!shiftBuildingVertically(buildings[index], offsetMeters)) {
        result.receipt.failedTable = CreativeWorldLayoutTable::Building;
        result.receipt.failedIndex = index;
        setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                  "creative_world_layout_building_grounding_offset_invalid");
        return result;
      }
      ++result.receipt.groundedBuildingCount;

      if (grounding.reliefCells > 0U) {
        const std::string foundationKey =
            childKey(symbol.stableKey, "foundation");
        if (!registerKey(stableKeys, foundationKey,
                         CreativeWorldLayoutTable::Building, index,
                         result.receipt)) {
          return result;
        }
        CreativeBounds foundationBounds;
        if (!worldCoordinate(grid.origin.x, grid.cellSizeMeters,
                             symbol.rootFootprint.minimum.x,
                             foundationBounds.min.x) ||
            !worldCoordinate(grid.origin.x, grid.cellSizeMeters,
                             symbol.rootFootprint.maximum.x,
                             foundationBounds.max.x) ||
            !worldCoordinate(grid.origin.z, grid.cellSizeMeters,
                             symbol.rootFootprint.minimum.z,
                             foundationBounds.min.z) ||
            !worldCoordinate(grid.origin.z, grid.cellSizeMeters,
                             symbol.rootFootprint.maximum.z,
                             foundationBounds.max.z) ||
            !worldCoordinate(grid.origin.y, grid.cellSizeMeters,
                             grounding.minimumHeightCells,
                             foundationBounds.min.y) ||
            !worldCoordinate(grid.origin.y, grid.cellSizeMeters,
                             grounding.maximumHeightCells,
                             foundationBounds.max.y)) {
          result.receipt.failedTable = CreativeWorldLayoutTable::Building;
          result.receipt.failedIndex = index;
          setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                    "creative_world_layout_foundation_bounds_invalid");
          return result;
        }
        CreativeBuildingBoxSpec foundation{
            CreativeObjectKind::Floor, foundationKey,
            symbol.name + " Foundation", foundationBounds};
        appendTagOnce(foundation.tags,
                      creativeWorldLayoutProvenanceTag(
                          layout, CreativeWorldLayoutTable::Building, index));
        appendTagOnce(foundation.tags, "creative_world_layout:foundation");
        buildings[index].boxes.insert(buildings[index].boxes.begin(),
                                      std::move(foundation));
        ++result.receipt.foundationObjectCount;
      }
    }

    CreativeBuildingRecipeResult built =
        buildCreativeBuildingRecipe(buildings[index]);
    if (!built.receipt.accepted) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Building;
      result.receipt.failedIndex = index;
      result.receipt.kernelReasonCode = built.receipt.reasonCode;
      setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                "creative_world_layout_building_rejected");
      return result;
    }
    built.plan.definitionFingerprint =
        fingerprintCreativeRecipePlan(built.plan);
    if (built.plan.definitionFingerprint == 0U) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Building;
      result.receipt.failedIndex = index;
      result.receipt.kernelReasonCode =
          "creative_recipe_definition_fingerprint_invalid";
      setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                "creative_world_layout_building_fingerprint_rejected");
      return result;
    }
    desiredObjectRecipes.push_back(std::move(built.plan));
  }
  for (CreativeRecipePlan& recipe : desiredLibraryRecipes) {
    desiredObjectRecipes.push_back(std::move(recipe));
  }

  CreativeWorldLayoutReconciliationResult reconciliation =
      reconcileCreativeWorldLayoutRecipes(
          {&document, layoutTag, desiredObjectRecipes,
           options.conflictDecisions});
  result.recipeChanges = std::move(reconciliation.changes);
  result.receipt.objectRecipeCreateCount =
      reconciliation.createRecipeCount;
  result.receipt.objectRecipeKeepCount = reconciliation.keepRecipeCount;
  result.receipt.objectRecipeRefinedCount =
      reconciliation.refinedRecipeCount;
  result.receipt.objectRecipePatchCount =
      reconciliation.patchRecipeCount;
  result.receipt.objectRecipeReplaceCount =
      reconciliation.replaceRecipeCount;
  result.receipt.objectRecipeConflictCount =
      reconciliation.conflictRecipeCount;
  result.receipt.objectRecipeDetachCount =
      reconciliation.detachRecipeCount;
  if (!reconciliation.accepted) {
    setStatus(result.receipt,
              reconciliation.blocked
                  ? CreativeWorldLayoutStatus::RefinementConflict
                  : CreativeWorldLayoutStatus::InvalidDocument,
              reconciliation.reasonCode);
    return result;
  }

  result.plan.objectDetachIds =
      std::move(reconciliation.detachObjectIds);
  if (!collectObjectRemovalOrder(document,
                                 reconciliation.removeObjectIds,
                                 result.plan.objectRemoveIds)) {
    setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidDocument,
              "creative_world_layout_owned_object_graph_invalid");
    return result;
  }
  std::vector<bool> claimedRecipes(desiredObjectRecipes.size(), false);
  result.plan.objectRecipePatches.reserve(
      reconciliation.patchDecisions.size());
  for (CreativeWorldLayoutRecipePatchDecision& decision :
       reconciliation.patchDecisions) {
    if (decision.desiredRecipeIndex >= desiredObjectRecipes.size() ||
        decision.existingObjectIds.size() !=
            desiredObjectRecipes[decision.desiredRecipeIndex].objects.size() ||
        decision.memberActions.size() !=
            desiredObjectRecipes[decision.desiredRecipeIndex].objects.size() ||
        claimedRecipes[decision.desiredRecipeIndex]) {
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidDocument,
                "creative_world_layout_reconciliation_patch_invalid");
      return result;
    }
    claimedRecipes[decision.desiredRecipeIndex] = true;
    CreativeWorldLayoutRecipePatch patch;
    patch.recipe =
        std::move(desiredObjectRecipes[decision.desiredRecipeIndex]);
    patch.objectIds = std::move(decision.existingObjectIds);
    patch.memberActions = std::move(decision.memberActions);
    result.plan.objectRecipePatches.push_back(std::move(patch));
  }
  result.plan.objectRecipes.reserve(
      reconciliation.applyRecipeIndices.size());
  for (const std::size_t recipeIndex : reconciliation.applyRecipeIndices) {
    if (recipeIndex >= desiredObjectRecipes.size() ||
        claimedRecipes[recipeIndex]) {
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidDocument,
                "creative_world_layout_reconciliation_index_invalid");
      return result;
    }
    claimedRecipes[recipeIndex] = true;
    result.plan.objectRecipes.push_back(
        std::move(desiredObjectRecipes[recipeIndex]));
  }

  CreativeObjectId nextObjectId = document.nextObjectId();
  for (std::size_t patchIndex = 0U;
       patchIndex < result.plan.objectRecipePatches.size(); ++patchIndex) {
    CreativeWorldLayoutRecipePatch& patch =
        result.plan.objectRecipePatches[patchIndex];
    const std::size_t createCount = static_cast<std::size_t>(std::count(
        patch.memberActions.begin(), patch.memberActions.end(),
        CreativeWorldLayoutRecipeMemberAction::Create));
    if (nextObjectId == kInvalidObjectId ||
        createCount > std::numeric_limits<CreativeObjectId>::max() -
                          nextObjectId) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Object;
      result.receipt.failedIndex = patchIndex;
      result.receipt.kernelReasonCode =
          "creative_recipe_object_id_overflow";
      setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                "creative_world_layout_object_materialize_rejected");
      return result;
    }
    for (std::size_t objectIndex = 0U;
         objectIndex < patch.objectIds.size(); ++objectIndex) {
      const CreativeWorldLayoutRecipeMemberAction action =
          patch.memberActions[objectIndex];
      if (action == CreativeWorldLayoutRecipeMemberAction::Create) {
        if (patch.objectIds[objectIndex] != kInvalidObjectId) {
          result.receipt.failedTable = CreativeWorldLayoutTable::Object;
          result.receipt.failedIndex = patchIndex;
          result.receipt.kernelReasonCode =
              "creative_world_layout_patch_create_id_present";
          setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                    "creative_world_layout_object_materialize_rejected");
          return result;
        }
        patch.objectIds[objectIndex] = nextObjectId++;
      } else if ((action !=
                      CreativeWorldLayoutRecipeMemberAction::Preserve &&
                  action != CreativeWorldLayoutRecipeMemberAction::Update) ||
                 patch.objectIds[objectIndex] == kInvalidObjectId) {
        result.receipt.failedTable = CreativeWorldLayoutTable::Object;
        result.receipt.failedIndex = patchIndex;
        result.receipt.kernelReasonCode =
            "creative_world_layout_patch_member_invalid";
        setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                  "creative_world_layout_object_materialize_rejected");
        return result;
      }
    }
    const CreativeRecipeMaterializeResult validated =
        materializeCreativeRecipe(patch.recipe, patch.objectIds);
    if (!validated.receipt.accepted) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Object;
      result.receipt.failedIndex = patchIndex;
      result.receipt.kernelReasonCode = validated.receipt.reasonCode;
      setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                "creative_world_layout_object_materialize_rejected");
      return result;
    }
    result.receipt.objectCount += validated.createRequests.size();
  }
  for (std::size_t index = 0U; index < result.plan.objectRecipes.size();
       ++index) {
    const CreativeRecipeMaterializeResult validated =
        materializeCreativeRecipe(result.plan.objectRecipes[index],
                                  nextObjectId);
    if (!validated.receipt.accepted) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Object;
      result.receipt.failedIndex = index;
      result.receipt.kernelReasonCode = validated.receipt.reasonCode;
      setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                "creative_world_layout_object_materialize_rejected");
      return result;
    }
    nextObjectId +=
        static_cast<CreativeObjectId>(validated.createRequests.size());
    result.receipt.objectCount += validated.createRequests.size();
  }

  const bool hasSourceSymbols =
      !layout.buildings.empty() || !layout.levels.empty() ||
      !layout.rooms.empty() || !layout.boxes.empty() ||
      !layout.walls.empty() || !layout.openings.empty() ||
      !layout.objects.empty() ||
      !layout.terrainProfiles.empty() || !layout.terrainPaths.empty();
  const bool hasOperations = !result.plan.objectDetachIds.empty() ||
                             !result.plan.objectRemoveIds.empty() ||
                             !result.plan.objectRecipePatches.empty() ||
                             !result.plan.objectRecipes.empty() ||
                             !result.plan.terrainEdits.empty() ||
                             !result.plan.materialEdits.empty();
  if (!hasSourceSymbols && !hasOperations) {
    setStatus(result.receipt, CreativeWorldLayoutStatus::Empty,
              "creative_world_layout_empty");
    result.plan = {};
    return result;
  }

  const CreativeWorldLayoutPreviewResult preview =
      previewCreativeWorldLayoutPlan(document, result.plan);
  if (!preview.accepted) {
    setStatus(result.receipt, preview.status, preview.reasonCode);
    result.plan = {};
    return result;
  }

  result.receipt.buildingCount = layout.buildings.size();
  result.receipt.objectRecipeCount =
      result.plan.objectRecipePatches.size() +
      result.plan.objectRecipes.size();
  result.receipt.objectDetachCount = result.plan.objectDetachIds.size();
  result.receipt.objectRemoveCount = result.plan.objectRemoveIds.size();
  result.receipt.terrainControlEditCount = result.plan.terrainEdits.size();
  result.receipt.terrainMaterialEditCount = result.plan.materialEdits.size();
  const bool changed = preview.status == CreativeWorldLayoutStatus::Ready;
  setStatus(result.receipt, preview.status,
            changed ? "creative_world_layout_ready"
                    : "creative_world_layout_no_change",
            true);
  return result;
}

}  // namespace iggy3d::creative
