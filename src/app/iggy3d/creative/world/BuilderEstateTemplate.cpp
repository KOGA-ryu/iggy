#include "app/iggy3d/creative/world/MapTemplate.hpp"

#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/recipes/ObjectLibraryRecipe.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBlockoutMaterialization.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>

namespace iggy3d::creative {
namespace {

constexpr CreativeVec3 kEstateGridOrigin{-40.0, 0.0, -40.0};
constexpr std::int32_t kHouseFloorLayer = 4;
constexpr double kEstateGridCellSizeMeters = 1.0;
constexpr CreativeWorldLayoutArchitecturalProfileKind
    kHouseArchitecturalProfileKind =
        CreativeWorldLayoutArchitecturalProfileKind::Grand;
constexpr std::array<std::string_view, 1U> kBuiltInBuildingTemplateIds{
    kBuilderEstateHouseTemplateId};
constexpr std::array<std::string_view, 2U> kEstateHouseTags{
    "map_template:builder_estate", "building_role:estate_house"};

void setStatus(CreativeMapTemplateResult& result,
               CreativeMapTemplateStatus status,
               std::string_view reasonCode,
               bool accepted = false) {
  result.status = status;
  result.reasonCode = reasonCode;
  result.accepted = accepted;
}

CreativeWorldLayoutBuildingTemplateResult builderEstateHouseTemplate() {
  CreativeWorldLayout source;
  source.stableKey = "builder_estate_house_source";
  const CreativeWorldLayoutArchitecturalProfile profile =
      defaultCreativeWorldLayoutArchitecturalProfile(
          kHouseArchitecturalProfileKind);
  CreativeGridSettings grid;
  grid.cellSizeMeters = kEstateGridCellSizeMeters;
  std::uint16_t floorToFloorCells = 0U;
  if (!resolveCreativeWorldLayoutArchitecturalProfileFloorToFloorCells(
          grid, profile, floorToFloorCells)) {
    CreativeWorldLayoutBuildingTemplateResult failed;
    failed.requested = true;
    failed.status = CreativeWorldLayoutBuildingTemplateStatus::InvalidTemplate;
    failed.reasonCode =
        "creative_builder_estate_architectural_profile_invalid";
    return failed;
  }
  CreativeWorldLayoutBuildingBlockoutRecipe recipe;
  recipe.request.footprint = {{0, 0}, {48, 48}};
  recipe.request.pattern =
      CreativeWorldLayoutBuildingBlockoutPattern::Grid2x2;
  recipe.request.wallThicknessCells = 0.25;
  recipe.request.floorToFloorCells = floorToFloorCells;
  recipe.request.storeys.count = 2U;
  recipe.request.storeys.connectStoreys = true;
  recipe.request.storeys.connectorKind =
      CreativeWorldLayoutVerticalConnectorKind::Stair;
  recipe.request.storeys.preferredDirection =
      CreativeWorldLayoutVerticalDirection::PositiveZ;
  recipe.floorTopLayer = kHouseFloorLayer;
  recipe.floorThicknessLayers = profile.floorThicknessLayers;
  recipe.ceilingThicknessLayers = profile.ceilingThicknessLayers;
  recipe.roofThicknessLayers = profile.roofThicknessLayers;
  recipe.architecturalProfileKind = profile.kind;

  const CreativeWorldLayoutBuildingEditResult materialized =
      materializeCreativeWorldLayoutBuildingBlockout(
          source, recipe, 1U, {"Estate House", kEstateHouseTags});
  if (!materialized.accepted) {
    CreativeWorldLayoutBuildingTemplateResult failed;
    failed.requested = true;
    failed.status = CreativeWorldLayoutBuildingTemplateStatus::InvalidTemplate;
    failed.reasonCode = materialized.reasonCode;
    return failed;
  }
  return captureCreativeWorldLayoutBuildingTemplate(
      materialized.edited,
      {0U, std::string{kBuilderEstateHouseTemplateId}, "Estate House"});
}

void appendPlateau(CreativeWorldLayout& layout,
                   std::string stableKey,
                   CreativeTerrainCoord2 center) {
  CreativeWorldLayoutTerrainProfile plateau;
  plateau.stableKey = std::move(stableKey);
  plateau.kind = CreativeTerrainRecipeKind::Plateau;
  plateau.center = center;
  plateau.baseHeightCells = 4U;
  plateau.radiusCells = 8U;
  plateau.amplitudeCells = 1U;
  plateau.spacingCells = 4U;
  plateau.blend = CreativeTerrainProfileBlend::Set;
  plateau.rodPolicy = CreativeTerrainProfileRodPolicy::Fill;
  layout.terrainProfiles.push_back(std::move(plateau));
}

void appendTerrainPath(CreativeWorldLayout& layout,
                       std::string stableKey,
                       CreativeTerrainPathKind kind,
                       std::initializer_list<CreativeTerrainPathPoint> points,
                       std::uint16_t halfWidthCells,
                       std::uint16_t baselineHeightCells) {
  CreativeWorldLayoutTerrainPath path;
  path.stableKey = std::move(stableKey);
  path.recipe.kind = kind;
  path.recipe.elevation = CreativeTerrainPathElevation::Level;
  path.recipe.crossSection = kind == CreativeTerrainPathKind::Trench
                                 ? CreativeTerrainPathCrossSection::Cut
                                 : CreativeTerrainPathCrossSection::Flat;
  path.recipe.paintSurface = true;
  switch (kind) {
    case CreativeTerrainPathKind::Road:
      path.recipe.material = CreativeTerrainMaterial::Dirt;
      break;
    case CreativeTerrainPathKind::River:
    case CreativeTerrainPathKind::Trench:
      path.recipe.material = CreativeTerrainMaterial::Sand;
      break;
    case CreativeTerrainPathKind::Ridge:
      path.recipe.material = CreativeTerrainMaterial::Stone;
      break;
    case CreativeTerrainPathKind::Count:
      path.recipe.material = CreativeTerrainMaterial::Count;
      break;
  }
  CreativeTerrainPathSourcePointId nextPointId = 1U;
  for (CreativeTerrainPathPoint point : points) {
    point.heightCells = baselineHeightCells;
    path.recipe.points.push_back({nextPointId++, point.coord,
                                  point.heightCells, halfWidthCells, 1U, 0});
  }
  path.recipe.nextPointId = nextPointId;
  layout.terrainPaths.push_back(std::move(path));
}

CreativeWorldLayoutBuildingEditResult builderEstateLayout(
    const CreativeWorldLayoutBuildingTemplate& houseTemplate) {
  CreativeWorldLayout layout;
  layout.stableKey = "builder_estate_layout";
  layout.terrainOwnership =
      CreativeWorldLayoutTerrainOwnership::PreserveExisting;
  appendPlateau(layout, "plateau.row_1.col_1", {20, 36});
  appendPlateau(layout, "plateau.row_1.col_2", {40, 36});
  appendPlateau(layout, "plateau.row_1.col_3", {60, 36});
  appendPlateau(layout, "plateau.row_2.col_1", {20, 52});
  appendPlateau(layout, "plateau.row_2.col_2", {40, 52});
  appendPlateau(layout, "plateau.row_2.col_3", {60, 52});
  appendPlateau(layout, "plateau.row_3.col_1", {20, 68});
  appendPlateau(layout, "plateau.row_3.col_2", {40, 68});
  appendPlateau(layout, "plateau.row_3.col_3", {60, 68});
  appendTerrainPath(layout, "path.ditch", CreativeTerrainPathKind::Trench,
                    {{{12, 20}, 4U}, {{36, 20}, 4U}, {{64, 20}, 4U}},
                    1U, 4U);
  CreativeTerrainPathSourceRecipe& ditch =
      layout.terrainPaths.back().recipe;
  ditch.watercourse.bankSlopeCells = 1U;
  ditch.watercourse.nextCrossingId = 2U;
  ditch.watercourse.crossings = {{1U, 2U, 1U, 2U, 12U}};
  appendTerrainPath(layout, "path.estate_road",
                    CreativeTerrainPathKind::Road,
                    {{{12, 78}, 3U}, {{64, 78}, 3U}}, 0U, 3U);

  return stampCreativeWorldLayoutBuildingTemplate(
      layout, houseTemplate, {{16, 28}, 1U, false, true});
}

CreativeWorldLayoutObject boundedPlacement(
    CreativeObjectKind kind,
    std::string stableKey,
    std::string name,
    CreativeBounds boundsCells,
    std::string assetId = {}) {
  CreativeWorldLayoutObject placement;
  placement.kind = kind;
  placement.mode = CreativeObjectLibraryPlacementMode::Bounds;
  placement.stableKey = std::move(stableKey);
  placement.name = std::move(name);
  placement.assetId = std::move(assetId);
  placement.boundsCells = boundsCells;
  placement.tags = {"map_template:builder_estate"};
  return placement;
}

CreativeWorldLayoutObject pointPlacement(
    CreativeObjectKind kind,
    std::string stableKey,
    std::string name,
    CreativeVec3 pointCells) {
  CreativeWorldLayoutObject placement;
  placement.kind = kind;
  placement.mode = CreativeObjectLibraryPlacementMode::Point;
  placement.stableKey = std::move(stableKey);
  placement.name = std::move(name);
  placement.pointCells = pointCells;
  placement.tags = {"map_template:builder_estate"};
  return placement;
}

void appendBuilderEstateObjects(CreativeWorldLayout& layout) {
  // These values are grid-cell coordinates. Builder Estate's grid origin is
  // {-40, 0, -40}, so the compiler reconstructs the original world-space
  // placement without embedding document-specific coordinates in the recipe.
  CreativeWorldLayoutObject ditchBridge =
      boundedPlacement(CreativeObjectKind::Bridge, "bridge.ditch",
                       "Ditch Bridge",
                       {{34.0, 4.0, 17.0}, {38.0, 5.0, 23.0}});
  ditchBridge.usesBridgeRecipe = true;
  ditchBridge.bridge.watercoursePathKey = "path.ditch";
  ditchBridge.bridge.crossingId = 1U;
  ditchBridge.bridge.settings.deckWidthMeters = 4.0;
  layout.objects = {
      std::move(ditchBridge),
      boundedPlacement(CreativeObjectKind::Platform, "platform.main_approach",
                       "Main House Approach",
                       {{38.5, 4.0, 76.0}, {41.5, 4.2, 79.0}}),
      boundedPlacement(CreativeObjectKind::Furniture, "furniture.main_sofa",
                       "Main Great Room Sofa",
                       {{36.0, 4.5, 32.0}, {39.0, 5.5, 33.5}}),
      boundedPlacement(CreativeObjectKind::Furniture, "furniture.main_table",
                       "Main Great Room Table",
                       {{33.0, 4.5, 34.5}, {35.0, 5.2, 36.5}}),
      boundedPlacement(CreativeObjectKind::Furniture,
                       "furniture.main_counter", "Main Kitchen Counter",
                       {{35.0, 4.5, 28.5}, {39.5, 5.5, 30.0}}),
      boundedPlacement(CreativeObjectKind::Rock, "rock.ditch_west",
                       "Ditch West Boulder",
                       {{16.0, 3.0, 18.0}, {18.5, 5.0, 20.5}},
                       "boulder_01"),
      boundedPlacement(CreativeObjectKind::Rock, "rock.ditch_east",
                       "Ditch East Boulder",
                       {{59.0, 3.0, 18.5}, {61.0, 4.75, 20.5}},
                       "boulder_01"),
      pointPlacement(CreativeObjectKind::SpawnPoint, "anchor.player",
                     "Estate Arrival", {40.0, 4.5, 79.0}),
      pointPlacement(CreativeObjectKind::NpcSpawn, "anchor.caretaker",
                     "Estate Caretaker", {38.0, 4.5, 37.0}),
  };
}

std::uint64_t linkedBuildingCount(const CreativeWorldLayout& layout) {
  std::uint64_t count = 0U;
  for (std::size_t index = 0U; index < layout.buildings.size(); ++index) {
    if (creativeWorldLayoutBuildingTemplateInstanceProvenance(layout, index)
            .valid) {
      ++count;
    }
  }
  return count;
}

}  // namespace

std::span<const std::string_view> creativeBuiltInBuildingTemplateIds()
    noexcept {
  return kBuiltInBuildingTemplateIds;
}

CreativeWorldLayoutBuildingTemplateResult
buildCreativeBuiltInBuildingTemplate(std::string_view templateId) {
  if (templateId == kBuilderEstateHouseTemplateId) {
    return builderEstateHouseTemplate();
  }
  CreativeWorldLayoutBuildingTemplateResult result;
  result.requested = true;
  result.status = CreativeWorldLayoutBuildingTemplateStatus::InvalidRequest;
  result.reasonCode =
      "creative_builtin_building_template_unknown";
  return result;
}

CreativeMapTemplateResult buildBuilderEstateMapTemplate(
    CreativeDocumentId documentId) {
  CreativeMapTemplateResult result;
  result.requested = true;
  result.templateId = kBuilderEstateMapTemplateId;
  if (documentId == kInvalidDocumentId) {
    setStatus(result, CreativeMapTemplateStatus::InvalidDocumentId,
              "creative_map_template_document_id_invalid");
    return result;
  }

  Facade facade;
  CreativeDocument document = CreativeDocument::create("Builder Estate");
  if (!document.assignId(documentId) ||
      !document.setGridSettings(
          {kEstateGridOrigin, kEstateGridCellSizeMeters, {80, 64, 80}}) ||
      !document.setWorldBounds(
          {kEstateGridOrigin, {40.0, 64.0, 40.0}}) ||
      !facade.installDocument(std::move(document)).accepted) {
    setStatus(result, CreativeMapTemplateStatus::DocumentSetupFailed,
              "creative_builder_estate_document_setup_failed");
    return result;
  }

  CreativeWorldLayoutBuildingTemplateResult house =
      builderEstateHouseTemplate();
  if (!house.accepted) {
    setStatus(result, CreativeMapTemplateStatus::BuildingTemplateFailed,
              house.reasonCode);
    return result;
  }
  CreativeWorldLayoutBuildingEditResult authored =
      builderEstateLayout(house.value);
  if (!authored.accepted) {
    setStatus(result, CreativeMapTemplateStatus::WorldLayoutFailed,
              authored.reasonCode);
    return result;
  }
  appendBuilderEstateObjects(authored.edited);
  const CreativeWorldLayoutCompileResult compiled =
      buildCreativeWorldLayoutPlan(facade.document(), authored.edited);
  if (!compiled.receipt.accepted) {
    const std::string_view reasonCode =
        compiled.receipt.kernelReasonCode !=
                "creative_world_layout_kernel_not_requested"
            ? std::string_view{compiled.receipt.kernelReasonCode}
            : std::string_view{compiled.receipt.reasonCode};
    setStatus(result, CreativeMapTemplateStatus::WorldLayoutFailed,
              reasonCode);
    return result;
  }
  const CreativeWorldLayoutApplyReceipt applied =
      applyCreativeWorldLayoutPlan(facade, compiled.plan);
  if (!applied.accepted || !applied.changed) {
    setStatus(result, CreativeMapTemplateStatus::WorldLayoutFailed,
              applied.reasonCode);
    return result;
  }

  result.document = facade.document();
  result.objectCount = result.document.objectCount();
  result.terrainControlCount = result.document.terrainField().controlCount();
  result.terrainMaterialOverrideCount =
      result.document.terrainMaterialField().overrideCount();
  result.worldLayout = std::move(authored.edited);
  result.worldLayoutPresent = true;
  result.linkedBuildingInstanceCount = linkedBuildingCount(result.worldLayout);
  result.roomSymbolCount = result.worldLayout.rooms.size();
  result.openingSymbolCount = result.worldLayout.openings.size();
  result.supplementalRecipeCount = result.worldLayout.objects.empty() ? 0U : 1U;
  result.buildingTemplates.push_back(std::move(house.value));
  for (const CreativeObject& object : result.document.objects()) {
    if (object.kind == CreativeObjectKind::Floor) {
      result.primaryFloorObjectId = object.id;
      break;
    }
  }
  if (result.primaryFloorObjectId == kInvalidObjectId) {
    setStatus(result, CreativeMapTemplateStatus::WorldLayoutFailed,
              "creative_builder_estate_primary_floor_missing");
    return result;
  }
  setStatus(result, CreativeMapTemplateStatus::Ready,
            "creative_builder_estate_ready", true);
  return result;
}

}  // namespace iggy3d::creative
