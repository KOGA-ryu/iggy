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
constexpr std::uint16_t kHouseWallHeightCells = 3U;
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
  CreativeWorldLayoutBuildingBlockoutRecipe recipe;
  recipe.request.footprint = {{0, 0}, {12, 12}};
  recipe.request.pattern =
      CreativeWorldLayoutBuildingBlockoutPattern::Grid2x2;
  recipe.request.wallThicknessCells = 0.25;
  recipe.request.wallHeightCells = kHouseWallHeightCells;
  recipe.request.storeys.count = 2U;
  recipe.request.storeys.connectStoreys = true;
  recipe.request.storeys.connectorKind =
      CreativeWorldLayoutVerticalConnectorKind::Stair;
  recipe.request.storeys.preferredDirection =
      CreativeWorldLayoutVerticalDirection::PositiveZ;
  recipe.floorTopLayer = kHouseFloorLayer;

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
                       CreativeTerrainRecipeKind kind,
                       std::initializer_list<CreativeTerrainPathPoint> points,
                       std::uint16_t halfWidthCells,
                       std::uint16_t baselineHeightCells) {
  CreativeWorldLayoutTerrainPath path;
  path.stableKey = std::move(stableKey);
  path.kind = kind;
  path.firstPointIndex = layout.terrainPathPoints.size();
  path.pointCount = points.size();
  path.elevation = CreativeTerrainPathElevation::Level;
  path.halfWidthCells = halfWidthCells;
  path.amplitudeCells = 1U;
  path.paintSurface = true;
  path.material = CreativeTerrainMaterial::Count;
  for (CreativeTerrainPathPoint point : points) {
    point.heightCells = baselineHeightCells;
    layout.terrainPathPoints.push_back(point);
  }
  layout.terrainPaths.push_back(std::move(path));
}

CreativeWorldLayoutBuildingEditResult builderEstateLayout(
    const CreativeWorldLayoutBuildingTemplate& houseTemplate) {
  CreativeWorldLayout layout;
  layout.stableKey = "builder_estate_layout";
  layout.terrainOwnership = CreativeWorldLayoutTerrainOwnership::ReplaceAll;
  appendPlateau(layout, "plateau.northwest", {20, 28});
  appendPlateau(layout, "plateau.north", {36, 28});
  appendPlateau(layout, "plateau.northeast", {52, 28});
  appendPlateau(layout, "plateau.southwest", {20, 40});
  appendPlateau(layout, "plateau.south", {36, 40});
  appendPlateau(layout, "plateau.southeast", {52, 40});
  appendTerrainPath(layout, "path.ditch", CreativeTerrainRecipeKind::Ditch,
                    {{{12, 20}, 4U}, {{64, 20}, 4U}}, 0U, 4U);
  appendTerrainPath(layout, "path.estate_road",
                    CreativeTerrainRecipeKind::Road,
                    {{{12, 44}, 3U}, {{64, 44}, 3U}}, 0U, 3U);

  CreativeWorldLayoutBuildingEditResult first =
      stampCreativeWorldLayoutBuildingTemplate(
          layout, houseTemplate, {{20, 28}, 1U, false, true});
  if (!first.accepted) {
    return first;
  }
  const CreativeWorldLayoutBuildingTemplateResult rotated =
      transformCreativeWorldLayoutBuildingTemplate(
          houseTemplate,
          CreativeWorldLayoutBuildingTransformOperation::RotateRight90);
  if (!rotated.accepted) {
    CreativeWorldLayoutBuildingEditResult failed;
    failed.requested = true;
    failed.reasonCode = rotated.reasonCode;
    return failed;
  }
  return stampCreativeWorldLayoutBuildingTemplate(
      first.edited, rotated.value,
      {{44, 28}, first.nextStableOrdinal, false, true});
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
  layout.objects = {
      boundedPlacement(CreativeObjectKind::Bridge, "bridge.ditch",
                       "Ditch Bridge",
                       {{34.0, 4.0, 17.0}, {38.0, 4.35, 23.0}}),
      boundedPlacement(CreativeObjectKind::Platform, "platform.main_approach",
                       "Main House Approach",
                       {{21.5, 4.0, 40.0}, {24.5, 4.2, 45.0}}),
      boundedPlacement(CreativeObjectKind::Furniture, "furniture.main_sofa",
                       "Main Great Room Sofa",
                       {{28.0, 4.5, 32.0}, {31.0, 5.5, 33.5}}),
      boundedPlacement(CreativeObjectKind::Furniture, "furniture.main_table",
                       "Main Great Room Table",
                       {{25.0, 4.5, 34.5}, {27.0, 5.2, 36.5}}),
      boundedPlacement(CreativeObjectKind::Furniture,
                       "furniture.main_counter", "Main Kitchen Counter",
                       {{27.0, 4.5, 28.5}, {31.5, 5.5, 30.0}}),
      boundedPlacement(CreativeObjectKind::Furniture, "furniture.guest_bed",
                       "Guest House Bed",
                       {{47.0, 4.5, 29.5}, {51.0, 5.2, 32.5}}),
      boundedPlacement(CreativeObjectKind::Crate, "prop.guest_crate",
                       "Guest House Crate",
                       {{53.0, 4.5, 36.0}, {54.25, 5.75, 37.25}}),
      boundedPlacement(CreativeObjectKind::Rock, "rock.ditch_west",
                       "Ditch West Boulder",
                       {{16.0, 3.0, 18.0}, {18.5, 5.0, 20.5}},
                       "boulder_01"),
      boundedPlacement(CreativeObjectKind::Rock, "rock.ditch_east",
                       "Ditch East Boulder",
                       {{59.0, 3.0, 18.5}, {61.0, 4.75, 20.5}},
                       "boulder_01"),
      pointPlacement(CreativeObjectKind::SpawnPoint, "anchor.player",
                     "Estate Arrival", {23.0, 4.5, 43.0}),
      pointPlacement(CreativeObjectKind::NpcSpawn, "anchor.caretaker",
                     "Estate Caretaker", {30.0, 4.5, 37.0}),
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
          {kEstateGridOrigin, 1.0, {80, 16, 80}}) ||
      !document.setWorldBounds(
          {kEstateGridOrigin, {40.0, 16.0, 40.0}}) ||
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
    setStatus(result, CreativeMapTemplateStatus::WorldLayoutFailed,
              compiled.receipt.reasonCode);
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
