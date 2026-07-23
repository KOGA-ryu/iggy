#include "app/iggy3d/creative/world/WorldLayout.hpp"
#include "app/iggy3d/creative/world/WorldLayoutCompileInternal.hpp"
#include "app/iggy3d/creative/world/WorldLayoutCodec.hpp"
#include "app/iggy3d/creative/world/WorldLayoutDimensions.hpp"
#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"
#include "app/iggy3d/creative/world/WorldLayoutOrthogonalRooms.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRoofs.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"
#include "app/iggy3d/creative/world/WorldLayoutVerticalConnectors.hpp"

#include "app/iggy3d/creative/recipes/StructuralSurfaceRecipe.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace iggy3d::creative {

using world_layout_compile::appendTagOnce;
using world_layout_compile::buildWorldLayoutObjectRecipes;
using world_layout_compile::childKey;
using world_layout_compile::finalizeWorldLayoutCompileResult;
using world_layout_compile::layoutBounds;
using world_layout_compile::layoutPoint;
using world_layout_compile::reconcileWorldLayoutRecipes;
using world_layout_compile::registerKey;
using world_layout_compile::setStatus;
using world_layout_compile::validDocument;
using world_layout_compile::validRect;
using world_layout_compile::validStableKey;
using world_layout_compile::validTerrainOwnership;
using world_layout_compile::worldCoordinate;

namespace {

[[nodiscard]] constexpr std::string_view roofPartSuffix(
    CreativeStructuralRoofPartKind kind) noexcept {
  switch (kind) {
    case CreativeStructuralRoofPartKind::FlatPanel:
      return ".roof.flat";
    case CreativeStructuralRoofPartKind::ShedPanel:
      return ".roof.shed";
    case CreativeStructuralRoofPartKind::GableFirst:
      return ".roof.gable.first";
    case CreativeStructuralRoofPartKind::GableSecond:
      return ".roof.gable.second";
    case CreativeStructuralRoofPartKind::HipNorth:
      return ".roof.hip.north";
    case CreativeStructuralRoofPartKind::HipEast:
      return ".roof.hip.east";
    case CreativeStructuralRoofPartKind::HipSouth:
      return ".roof.hip.south";
    case CreativeStructuralRoofPartKind::HipWest:
      return ".roof.hip.west";
    case CreativeStructuralRoofPartKind::Count:
      break;
  }
  return {};
}

[[nodiscard]] constexpr std::string_view roofPartLabel(
    CreativeStructuralRoofPartKind kind) noexcept {
  switch (kind) {
    case CreativeStructuralRoofPartKind::FlatPanel:
      return " Roof Flat Panel";
    case CreativeStructuralRoofPartKind::ShedPanel:
      return " Roof Shed Panel";
    case CreativeStructuralRoofPartKind::GableFirst:
      return " Roof Gable Panel 1";
    case CreativeStructuralRoofPartKind::GableSecond:
      return " Roof Gable Panel 2";
    case CreativeStructuralRoofPartKind::HipNorth:
      return " Roof Hip North";
    case CreativeStructuralRoofPartKind::HipEast:
      return " Roof Hip East";
    case CreativeStructuralRoofPartKind::HipSouth:
      return " Roof Hip South";
    case CreativeStructuralRoofPartKind::HipWest:
      return " Roof Hip West";
    case CreativeStructuralRoofPartKind::Count:
      break;
  }
  return {};
}

}  // namespace

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
  const CreativeWorldLayoutRoomGraph roomGraph =
      layout.rooms.empty() ? CreativeWorldLayoutRoomGraph{}
                           : buildCreativeWorldLayoutRoomGraph(layout);
  if (!layout.rooms.empty() && !roomGraph.accepted) {
    result.receipt.failedTable = CreativeWorldLayoutTable::Room;
    result.receipt.failedIndex = roomGraph.failedRoomIndex;
    setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
              roomGraph.reasonCode);
    return result;
  }

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

  std::vector<std::size_t> roofApertureCounts(layout.levels.size(), 0U);
  for (std::size_t index = 0U; index < layout.roofApertures.size(); ++index) {
    const CreativeWorldLayoutRoofAperture& symbol =
        layout.roofApertures[index];
    const bool finite = std::isfinite(symbol.minimumXCells) &&
                        std::isfinite(symbol.maximumXCells) &&
                        std::isfinite(symbol.minimumZCells) &&
                        std::isfinite(symbol.maximumZCells);
    if (symbol.levelIndex >= layout.levels.size() || symbol.name.empty() ||
        symbol.kind >= CreativeStructuralRoofApertureKind::Count || !finite ||
        symbol.minimumXCells >= symbol.maximumXCells ||
        symbol.minimumZCells >= symbol.maximumZCells ||
        !creativeWorldLayoutLevelIsTopmostOccupied(layout,
                                                    symbol.levelIndex) ||
        ++roofApertureCounts[symbol.levelIndex] >
            kCreativeStructuralRoofApertureCapacity) {
      result.receipt.failedTable = CreativeWorldLayoutTable::RoofAperture;
      result.receipt.failedIndex = index;
      setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                "creative_world_layout_roof_aperture_invalid");
      return result;
    }
    const CreativeWorldLayoutLevel& level = layout.levels[symbol.levelIndex];
    const std::string key = childKey(
        layout.buildings[level.buildingIndex].stableKey, symbol.stableKey);
    if (!registerKey(stableKeys, key,
                     CreativeWorldLayoutTable::RoofAperture, index,
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
    const CreativeWorldLayoutLevelDimensions dimensions =
        measureCreativeWorldLayoutLevelDimensions(grid, layout,
                                                  symbol.levelIndex);
    if (!dimensions.accepted ||
        dimensions.buildingIndex != symbol.buildingIndex) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Room;
      result.receipt.failedIndex = index;
      result.receipt.kernelReasonCode = dimensions.reasonCode;
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
        [&](CreativeObjectKind kind, double anchorPlaneMeters,
            std::uint16_t layerCount, std::string_view suffix,
            std::string_view label,
            const CreativeWorldLayoutVerticalConnectorPlan* cutout) {
          std::vector<CreativeStructuralSurfaceRecipeResult> pieces;
          const std::span<const CreativeWorldLayoutRect> sourceRects =
              creativeWorldLayoutRoomSurfaceRects(roomGraph, index);
          for (CreativeWorldLayoutRect sourceRect : sourceRects) {
            CreativeStructuralSurfaceRecipeRequest surface;
            surface.kind = kind;
            surface.layerCount = layerCount;
            if (!worldCoordinate(grid.origin.x, grid.cellSizeMeters,
                                 sourceRect.minimum.x, surface.minimumX) ||
                !worldCoordinate(grid.origin.x, grid.cellSizeMeters,
                                 sourceRect.maximum.x, surface.maximumX) ||
                !worldCoordinate(grid.origin.z, grid.cellSizeMeters,
                                 sourceRect.minimum.z, surface.minimumZ) ||
                !worldCoordinate(grid.origin.z, grid.cellSizeMeters,
                                 sourceRect.maximum.z, surface.maximumZ) ||
                !std::isfinite(anchorPlaneMeters)) {
              result.receipt.failedTable = CreativeWorldLayoutTable::Room;
              result.receipt.failedIndex = index;
              setStatus(result.receipt,
                        CreativeWorldLayoutStatus::InvalidSymbol,
                        "creative_world_layout_room_surface_invalid");
              return false;
            }
            surface.anchorPlaneMeters = anchorPlaneMeters;

            const CreativeWorldLayoutRect intersection =
                cutout == nullptr
                    ? CreativeWorldLayoutRect{}
                    : CreativeWorldLayoutRect{
                          {std::max(sourceRect.minimum.x,
                                    cutout->openingFootprint.minimum.x),
                           std::max(sourceRect.minimum.z,
                                    cutout->openingFootprint.minimum.z)},
                          {std::min(sourceRect.maximum.x,
                                    cutout->openingFootprint.maximum.x),
                           std::min(sourceRect.maximum.z,
                                    cutout->openingFootprint.maximum.z)}};
            const bool intersects =
                cutout != nullptr && validRect(intersection);
            if (!intersects) {
              const CreativeStructuralSurfaceRecipeResult planned =
                  planCreativeStructuralSurface(surface);
              if (!planned.accepted) {
                result.receipt.kernelReasonCode = planned.reasonCode;
                result.receipt.failedTable = CreativeWorldLayoutTable::Room;
                result.receipt.failedIndex = index;
                setStatus(result.receipt,
                          CreativeWorldLayoutStatus::KernelRejected,
                          "creative_world_layout_room_surface_rejected");
                return false;
              }
              pieces.push_back(planned);
              continue;
            }
            if (intersection.minimum == sourceRect.minimum &&
                intersection.maximum == sourceRect.maximum) {
              continue;
            }
            CreativeStructuralSurfaceCutoutRequest cutoutRequest;
            cutoutRequest.surface = surface;
            if (!worldCoordinate(grid.origin.x, grid.cellSizeMeters,
                                 intersection.minimum.x,
                                 cutoutRequest.cutoutMinimumX) ||
                !worldCoordinate(grid.origin.x, grid.cellSizeMeters,
                                 intersection.maximum.x,
                                 cutoutRequest.cutoutMaximumX) ||
                !worldCoordinate(grid.origin.z, grid.cellSizeMeters,
                                 intersection.minimum.z,
                                 cutoutRequest.cutoutMinimumZ) ||
                !worldCoordinate(grid.origin.z, grid.cellSizeMeters,
                                 intersection.maximum.z,
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
            pieces.insert(pieces.end(), cut.pieces.begin(),
                          cut.pieces.begin() +
                              static_cast<std::ptrdiff_t>(cut.pieceCount));
          }
          if (pieces.empty()) {
            result.receipt.failedTable = CreativeWorldLayoutTable::Room;
            result.receipt.failedIndex = index;
            setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                      "creative_world_layout_room_surface_consumed");
            return false;
          }

          const bool partitioned = pieces.size() > 1U || cutout != nullptr;
          for (std::size_t pieceIndex = 0U; pieceIndex < pieces.size();
               ++pieceIndex) {
            const std::string localKey =
                symbol.stableKey + std::string(suffix) +
                (!partitioned
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
                    (!partitioned
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

    if (!appendSurface(CreativeObjectKind::Floor, dimensions.floorTopMeters,
                       dimensions.floorThicknessLayers, ".floor", " Floor",
                       floorCutout)) {
      return result;
    }
    const bool roof =
        dimensions.upperSurfaceKind == CreativeObjectKind::Roof;
    const CreativeWorldLayoutLevel& level =
        layout.levels[dimensions.levelIndex];
    const bool usesAuthoredLevelRoof =
        level.roofStyle != CreativeStructuralRoofStyle::Flat ||
        level.roofOverhangCells > 0.0 ||
        roofApertureCounts[dimensions.levelIndex] > 0U;
    if (roof && usesAuthoredLevelRoof) {
      const bool firstRoomForLevel = std::none_of(
          layout.rooms.begin(), layout.rooms.begin() +
                                    static_cast<std::ptrdiff_t>(index),
          [&](const CreativeWorldLayoutRoom& room) {
            return room.levelIndex == dimensions.levelIndex;
          });
      if (!firstRoomForLevel) {
        continue;
      }
      const CreativeWorldLayoutRoofPlan roofPlan =
          planCreativeWorldLayoutRoof(grid, layout, dimensions.levelIndex);
      if (!roofPlan.accepted) {
        if (roofPlan.closure.failedApertureIndex <
            roofPlan.sourceApertureCount) {
          result.receipt.failedTable =
              CreativeWorldLayoutTable::RoofAperture;
          result.receipt.failedIndex =
              roofPlan.sourceApertureIndices[
                  roofPlan.closure.failedApertureIndex];
        } else {
          result.receipt.failedTable = CreativeWorldLayoutTable::Level;
          result.receipt.failedIndex = dimensions.levelIndex;
        }
        result.receipt.kernelReasonCode = roofPlan.reasonCode;
        setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                  "creative_world_layout_roof_rejected");
        return result;
      }
      std::array<std::size_t, kCreativeStructuralRoofPartCapacity>
          sourcePieceCounts{};
      std::array<std::size_t, kCreativeStructuralRoofPartCapacity>
          sourcePieceOrdinals{};
      for (std::size_t pieceIndex = 0U;
           pieceIndex < roofPlan.closure.pieceCount; ++pieceIndex) {
        const std::size_t sourcePartIndex =
            roofPlan.closure.pieces[pieceIndex].sourcePartIndex;
        if (sourcePartIndex >= sourcePieceCounts.size()) {
          result.receipt.failedTable = CreativeWorldLayoutTable::Level;
          result.receipt.failedIndex = dimensions.levelIndex;
          setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                    "creative_world_layout_roof_piece_owner_invalid");
          return result;
        }
        ++sourcePieceCounts[sourcePartIndex];
      }
      for (std::size_t pieceIndex = 0U;
           pieceIndex < roofPlan.closure.pieceCount; ++pieceIndex) {
        const CreativeStructuralRoofAperturePiece& piece =
            roofPlan.closure.pieces[pieceIndex];
        const CreativeStructuralRoofPart& part = piece.part;
        const std::string_view suffix = roofPartSuffix(part.partKind);
        const std::string_view label = roofPartLabel(part.partKind);
        if (suffix.empty() || label.empty()) {
          result.receipt.failedTable = CreativeWorldLayoutTable::Level;
          result.receipt.failedIndex = dimensions.levelIndex;
          setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                    "creative_world_layout_roof_part_kind_invalid");
          return result;
        }
        std::string localKey = level.stableKey + std::string(suffix);
        std::string localName = level.name + std::string(label);
        if (sourcePieceCounts[piece.sourcePartIndex] > 1U) {
          const std::size_t ordinal =
              ++sourcePieceOrdinals[piece.sourcePartIndex];
          localKey += ".part." + std::to_string(ordinal);
          localName += " Part " + std::to_string(ordinal);
        }
        const std::string key = childKey(
            layout.buildings[level.buildingIndex].stableKey,
            localKey);
        if (!registerKey(stableKeys, key, CreativeWorldLayoutTable::Level,
                         dimensions.levelIndex, result.receipt)) {
          return result;
        }
        CreativeBuildingBoxSpec box{
            part.kind, key, std::move(localName),
            part.bounds, {1.0, 1.0, 1.0}, {},
            part.rotationEulerRadians};
        appendTagOnce(
            box.tags,
            creativeWorldLayoutProvenanceTag(
                layout, CreativeWorldLayoutTable::Level,
                dimensions.levelIndex));
        appendTagOnce(box.tags,
                      creativeStructuralMaterialTag(roofPlan.geometry.material));
        buildings[level.buildingIndex].boxes.push_back(std::move(box));
      }
      for (std::size_t insertIndex = 0U;
           insertIndex < roofPlan.closure.insertCount; ++insertIndex) {
        const CreativeStructuralRoofApertureInsertPlan& insert =
            roofPlan.closure.inserts[insertIndex];
        if (insert.apertureIndex >= roofPlan.sourceApertureCount) {
          result.receipt.failedTable = CreativeWorldLayoutTable::RoofAperture;
          result.receipt.failedIndex = insert.apertureIndex;
          setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                    "creative_world_layout_roof_aperture_owner_invalid");
          return result;
        }
        const std::size_t sourceIndex =
            roofPlan.sourceApertureIndices[insert.apertureIndex];
        const CreativeWorldLayoutRoofAperture& aperture =
            layout.roofApertures[sourceIndex];
        const std::string key = childKey(
            layout.buildings[level.buildingIndex].stableKey,
            aperture.stableKey + ".insert");
        if (!registerKey(stableKeys, key,
                         CreativeWorldLayoutTable::RoofAperture, sourceIndex,
                         result.receipt)) {
          return result;
        }
        CreativeBuildingBoxSpec box{
            insert.kind, key, aperture.name + " Insert", insert.bounds,
            {1.0, 1.0, 1.0}, {}, insert.rotationEulerRadians};
        appendTagOnce(
            box.tags,
            creativeWorldLayoutProvenanceTag(
                layout, CreativeWorldLayoutTable::RoofAperture,
                sourceIndex));
        buildings[level.buildingIndex].boxes.push_back(std::move(box));
      }
      continue;
    }
    if (!appendSurface(dimensions.upperSurfaceKind,
                       dimensions.upperSurfaceSupportMeters,
                       dimensions.upperSurfaceThicknessLayers,
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
    appendTagOnce(box.tags, creativeStructuralMaterialTag(symbol.material));
    buildings[symbol.buildingIndex].boxes.push_back(std::move(box));
  }

  std::vector<std::size_t> localWallIndices(
      expanded.walls.size(), kInvalidCreativeWorldLayoutIndex);
  for (std::size_t index = 0U; index < expanded.walls.size(); ++index) {
    const CreativeWorldLayoutWall& symbol = expanded.walls[index];
    if (symbol.buildingIndex >= buildings.size() || symbol.name.empty() ||
        !std::isfinite(symbol.baseLayer) ||
        symbol.heightCells == 0U || !std::isfinite(symbol.thicknessCells) ||
        symbol.thicknessCells <= 0.0 ||
        symbol.profile >= CreativeWorldLayoutWallProfile::Count ||
        symbol.material >= CreativeStructuralMaterial::Count ||
        symbol.joinStyle >= CreativeWorldLayoutWallJoinStyle::Count) {
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
    appendTagOnce(wall.tags, creativeStructuralMaterialTag(symbol.material));
    if (index < layout.walls.size()) {
      appendTagOnce(wall.tags, creativeWorldLayoutProvenanceTag(
                                   layout, CreativeWorldLayoutTable::Wall,
                                   index));
    } else if (index < roomExpansion.wallProvenance.size()) {
      for (const auto& contributor :
           roomExpansion.wallProvenance[index].contributors) {
        appendTagOnce(wall.tags, creativeWorldLayoutProvenanceTag(
                                     layout,
                                     CreativeWorldLayoutTable::TopologyEdge,
                                     contributor.topologyEdgeIndex));
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
    const CreativeWorldLayoutOpeningDimensions dimensions =
        measureCreativeWorldLayoutOpeningDimensions(grid, expanded, index);
    if (!dimensions.accepted || dimensions.buildingIndex != buildingIndex) {
      result.receipt.failedTable = CreativeWorldLayoutTable::Opening;
      result.receipt.failedIndex = index;
      result.receipt.kernelReasonCode = dimensions.reasonCode;
      setStatus(result.receipt, CreativeWorldLayoutStatus::KernelRejected,
                "creative_world_layout_opening_dimensions_rejected");
      return result;
    }
    const std::string key = childKey(
        layout.buildings[buildingIndex].stableKey, symbol.stableKey);
    if (!registerKey(stableKeys, key, CreativeWorldLayoutTable::Opening, index,
                     result.receipt)) {
      return result;
    }
    CreativeBuildingOpeningSpec opening;
    opening.kind = symbol.kind;
    opening.door = symbol.door;
    opening.window = symbol.window;
    opening.facing = symbol.facing;
    opening.stableKey = key;
    opening.name = symbol.name;
    opening.centerOffsetMeters = dimensions.centerOffsetMeters;
    opening.widthMeters = dimensions.widthMeters;
    opening.cutoutBottomMeters = dimensions.cutoutBottomOffsetMeters;
    opening.cutoutHeightMeters = dimensions.cutoutHeightMeters;
    opening.includeInsert = symbol.includeInsert;
    opening.insertBottomMeters = dimensions.insertBottomOffsetMeters;
    opening.insertHeightMeters = dimensions.insertHeightMeters;
    opening.insertWidthMeters = dimensions.insertWidthMeters;
    opening.insertThicknessMeters = dimensions.insertThicknessMeters;
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

  std::unordered_set<std::string> bridgeAttachments;
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
    if (symbol.usesBridgeRecipe) {
      CreativeBounds bridgeBounds;
      const std::string attachmentKey =
          symbol.bridge.watercoursePathKey + "\x1f" +
          std::to_string(symbol.bridge.crossingId);
      if (symbol.kind != CreativeObjectKind::Bridge ||
          symbol.mode != CreativeObjectLibraryPlacementMode::Bounds ||
          !symbol.assetId.empty() || symbol.hasAssetSourceBounds ||
          !isValidCreativeBridgeSourceRecipe(symbol.bridge) ||
          !layoutBounds(grid, symbol.boundsCells, bridgeBounds)) {
        result.receipt.failedTable = CreativeWorldLayoutTable::Object;
        result.receipt.failedIndex = index;
        setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                  "creative_world_layout_bridge_source_invalid");
        return result;
      }
      if (!bridgeAttachments.insert(attachmentKey).second) {
        result.receipt.failedTable = CreativeWorldLayoutTable::Object;
        result.receipt.failedIndex = index;
        setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSymbol,
                  "creative_world_layout_bridge_attachment_duplicate");
        return result;
      }
      continue;
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
    placement.playerSpawn = symbol.playerSpawn;
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

  if (!buildWorldLayoutObjectRecipes(
          document, layout, grid, stableKeys, buildings,
          desiredObjectRecipes, desiredLibraryRecipes, result)) {
    return result;
  }

  if (!reconcileWorldLayoutRecipes(document, layoutTag, options,
                                   desiredObjectRecipes, result)) {
    return result;
  }

  result.plan.sourceLayoutFingerprint = fingerprintCreativeWorldLayout(layout);
  if (result.plan.sourceLayoutFingerprint == 0U) {
    setStatus(result.receipt, CreativeWorldLayoutStatus::InvalidSchema,
              "creative_world_layout_source_fingerprint_invalid");
    return result;
  }
  finalizeWorldLayoutCompileResult(document, layout, result);
  return result;
}

}  // namespace iggy3d::creative
