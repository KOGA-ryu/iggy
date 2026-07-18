#include "EditorWorldLayout.hpp"

#include "EditorWorldLayoutInternal.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <limits>
#include <numbers>
#include <string>
#include <string_view>
#include <utility>

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/input/Catalog.hpp"

namespace iggy3d_creative_app {
namespace {

struct AssetCategoryRule {
  std::string_view categoryId;
  CreativeEditorWorldLayoutAssetCategory category =
      CreativeEditorWorldLayoutAssetCategory::Props;
};

constexpr std::array kAssetCategoryRules{
    AssetCategoryRule{"bridge",
                      CreativeEditorWorldLayoutAssetCategory::Architecture},
    AssetCategoryRule{"ceiling",
                      CreativeEditorWorldLayoutAssetCategory::Architecture},
    AssetCategoryRule{"door",
                      CreativeEditorWorldLayoutAssetCategory::Architecture},
    AssetCategoryRule{"floor",
                      CreativeEditorWorldLayoutAssetCategory::Architecture},
    AssetCategoryRule{"foundation",
                      CreativeEditorWorldLayoutAssetCategory::Architecture},
    AssetCategoryRule{"roof",
                      CreativeEditorWorldLayoutAssetCategory::Architecture},
    AssetCategoryRule{"stairs",
                      CreativeEditorWorldLayoutAssetCategory::Architecture},
    AssetCategoryRule{"structure",
                      CreativeEditorWorldLayoutAssetCategory::Architecture},
    AssetCategoryRule{"walkway",
                      CreativeEditorWorldLayoutAssetCategory::Architecture},
    AssetCategoryRule{"wall",
                      CreativeEditorWorldLayoutAssetCategory::Architecture},
    AssetCategoryRule{"window",
                      CreativeEditorWorldLayoutAssetCategory::Architecture},
    AssetCategoryRule{"boulder",
                      CreativeEditorWorldLayoutAssetCategory::Nature},
    AssetCategoryRule{"foliage",
                      CreativeEditorWorldLayoutAssetCategory::Nature},
    AssetCategoryRule{"log", CreativeEditorWorldLayoutAssetCategory::Nature},
    AssetCategoryRule{"rock", CreativeEditorWorldLayoutAssetCategory::Nature},
    AssetCategoryRule{"shrub", CreativeEditorWorldLayoutAssetCategory::Nature},
    AssetCategoryRule{"terrain",
                      CreativeEditorWorldLayoutAssetCategory::Nature},
    AssetCategoryRule{"tree", CreativeEditorWorldLayoutAssetCategory::Nature},
    AssetCategoryRule{"stealth_blockout",
                      CreativeEditorWorldLayoutAssetCategory::Cover},
    AssetCategoryRule{"gameplay",
                      CreativeEditorWorldLayoutAssetCategory::Gameplay},
    AssetCategoryRule{"npc",
                      CreativeEditorWorldLayoutAssetCategory::Gameplay},
    AssetCategoryRule{"objective",
                      CreativeEditorWorldLayoutAssetCategory::Gameplay},
    AssetCategoryRule{"spawn",
                      CreativeEditorWorldLayoutAssetCategory::Gameplay},
    AssetCategoryRule{"trigger",
                      CreativeEditorWorldLayoutAssetCategory::Gameplay},
};

[[nodiscard]] std::string lowerAscii(std::string_view value) {
  std::string lowered(value);
  std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](char byte) {
    return static_cast<char>(
        std::tolower(static_cast<unsigned char>(byte)));
  });
  return lowered;
}

[[nodiscard]] bool finitePositiveScale(cr::CreativeVec3 scale) noexcept {
  return cr::isFiniteCreativeVec3(scale) && scale.x > 0.0 && scale.y > 0.0 &&
         scale.z > 0.0;
}

[[nodiscard]] bool samePlacementAsset(
    const CreativeEditorWorldLayoutCatalogPlacementState& placement,
    const cr::CreativeCatalogEntry& entry) noexcept {
  return placement.active && placement.kind == entry.hotbarEntry.objectKind &&
         placement.assetId == cr::creativeHotbarAssetId(entry.hotbarEntry) &&
         placement.sourceBoundsMeters.min.x ==
             entry.hotbarEntry.assetSourceBounds.min.x &&
         placement.sourceBoundsMeters.min.y ==
             entry.hotbarEntry.assetSourceBounds.min.y &&
         placement.sourceBoundsMeters.min.z ==
             entry.hotbarEntry.assetSourceBounds.min.z &&
         placement.sourceBoundsMeters.max.x ==
             entry.hotbarEntry.assetSourceBounds.max.x &&
         placement.sourceBoundsMeters.max.y ==
             entry.hotbarEntry.assetSourceBounds.max.y &&
         placement.sourceBoundsMeters.max.z ==
             entry.hotbarEntry.assetSourceBounds.max.z;
}

[[nodiscard]] bool pointInsideFootprint(
    const CreativeEditorWorldLayoutObjectFootprint& footprint,
    CreativeEditorWorldLayoutPoint point) noexcept {
  bool positive = false;
  bool negative = false;
  for (std::size_t index = 0U; index < footprint.corners.size(); ++index) {
    const CreativeEditorWorldLayoutPoint start = footprint.corners[index];
    const CreativeEditorWorldLayoutPoint end =
        footprint.corners[(index + 1U) % footprint.corners.size()];
    const double cross = (end.x - start.x) * (point.z - start.z) -
                         (end.z - start.z) * (point.x - start.x);
    positive = positive || cross > 1.0e-9;
    negative = negative || cross < -1.0e-9;
    if (positive && negative) {
      return false;
    }
  }
  return true;
}

}  // namespace

const char* creativeEditorWorldLayoutAssetCategoryLabel(
    CreativeEditorWorldLayoutAssetCategory category) noexcept {
  switch (category) {
    case CreativeEditorWorldLayoutAssetCategory::Architecture:
      return "Architecture";
    case CreativeEditorWorldLayoutAssetCategory::Nature:
      return "Nature";
    case CreativeEditorWorldLayoutAssetCategory::Cover:
      return "Cover";
    case CreativeEditorWorldLayoutAssetCategory::Props:
      return "Props";
    case CreativeEditorWorldLayoutAssetCategory::Gameplay:
      return "Gameplay";
    case CreativeEditorWorldLayoutAssetCategory::Count:
      break;
  }
  return "Unknown";
}

CreativeEditorWorldLayoutAssetCategory
classifyCreativeEditorWorldLayoutAsset(
    const cr::CreativeCatalogEntry& entry) noexcept {
  const std::string_view categoryId = entry.assetAuthoringMetadata.categoryId;
  const auto found = std::find_if(
      kAssetCategoryRules.begin(), kAssetCategoryRules.end(),
      [categoryId](const AssetCategoryRule& rule) {
        return rule.categoryId == categoryId;
      });
  return found == kAssetCategoryRules.end()
             ? CreativeEditorWorldLayoutAssetCategory::Props
             : found->category;
}

bool creativeEditorWorldLayoutAssetMatchesQuery(
    const cr::CreativeCatalogEntry& entry, std::string_view query) {
  if (query.empty()) {
    return true;
  }
  const std::string lowered = lowerAscii(query);
  return entry.searchText.find(lowered) != std::string::npos;
}

CreativeEditorWorldLayoutEditReceipt
selectCreativeEditorWorldLayoutCatalogAsset(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeCatalogEntry& entry) {
  const std::string_view assetId = cr::creativeHotbarAssetId(entry.hotbarEntry);
  const cr::CreativeBoundsMetrics bounds =
      cr::measureCreativeBounds(entry.hotbarEntry.assetSourceBounds);
  if (entry.category != cr::CreativeCatalogEntryCategory::Asset ||
      entry.hotbarEntry.objectKind <= cr::CreativeObjectKind::Unknown ||
      entry.hotbarEntry.objectKind >= cr::CreativeObjectKind::Count ||
      assetId.empty() || !entry.hotbarEntry.hasAssetBounds || !bounds.valid ||
      !cr::isPositiveCreativeVec3(bounds.size)) {
    return {false, false,
            "creative_editor_world_layout_catalog_asset_invalid"};
  }

  const bool changed = state.tool != CreativeEditorWorldLayoutTool::CatalogAsset ||
                       !samePlacementAsset(state.catalogPlacement, entry);
  const double elevation = state.catalogPlacement.elevationCells;
  const double yaw = state.catalogPlacement.yawDegrees;
  const cr::CreativeVec3 scale = state.catalogPlacement.scale;
  static_cast<void>(setCreativeEditorWorldLayoutTool(
      state, CreativeEditorWorldLayoutTool::CatalogAsset));
  state.catalogPlacement = {};
  state.catalogPlacement.active = true;
  state.catalogPlacement.kind = entry.hotbarEntry.objectKind;
  state.catalogPlacement.assetId = std::string(assetId);
  state.catalogPlacement.label =
      entry.label.empty() ? std::string(assetId) : entry.label;
  state.catalogPlacement.categoryId =
      entry.assetAuthoringMetadata.categoryId;
  state.catalogPlacement.sourceBoundsMeters =
      entry.hotbarEntry.assetSourceBounds;
  state.catalogPlacement.elevationCells = elevation;
  state.catalogPlacement.yawDegrees = yaw;
  state.catalogPlacement.scale = scale;
  state.assetCategory = classifyCreativeEditorWorldLayoutAsset(entry);
  state.statusMessage = state.catalogPlacement.label + " ready to place";
  return {true, changed,
          "creative_editor_world_layout_catalog_asset_selected"};
}

CreativeEditorWorldLayoutObjectFootprint
planCreativeEditorWorldLayoutObjectFootprint(
    const cr::CreativeWorldLayoutObject& object,
    cr::CreativeGridSettings grid) noexcept {
  CreativeEditorWorldLayoutObjectFootprint result;
  if (object.mode == cr::CreativeObjectLibraryPlacementMode::Bounds) {
    const cr::CreativeBoundsMetrics measured =
        cr::measureCreativeBounds(object.boundsCells);
    if (!measured.valid || !cr::isPositiveCreativeVec3(measured.size)) {
      return result;
    }
    result.axisAlignedBoundsCells = object.boundsCells;
    result.corners = {
        CreativeEditorWorldLayoutPoint{object.boundsCells.min.x,
                                       object.boundsCells.min.z},
        CreativeEditorWorldLayoutPoint{object.boundsCells.max.x,
                                       object.boundsCells.min.z},
        CreativeEditorWorldLayoutPoint{object.boundsCells.max.x,
                                       object.boundsCells.max.z},
        CreativeEditorWorldLayoutPoint{object.boundsCells.min.x,
                                       object.boundsCells.max.z},
    };
    result.valid = true;
    return result;
  }
  if (object.mode != cr::CreativeObjectLibraryPlacementMode::Point ||
      !object.hasAssetSourceBounds ||
      !std::isfinite(grid.cellSizeMeters) || grid.cellSizeMeters <= 0.0 ||
      !cr::isFiniteCreativeVec3(object.pointCells) ||
      !std::isfinite(object.yawRadians) || !finitePositiveScale(object.scale)) {
    return result;
  }
  const cr::CreativeBoundsMetrics source =
      cr::measureCreativeBounds(object.assetSourceBoundsMeters);
  if (!source.valid || !cr::isPositiveCreativeVec3(source.size)) {
    return result;
  }

  const double inverseCell = 1.0 / grid.cellSizeMeters;
  const cr::CreativeBounds authoredCells{
      {object.pointCells.x + object.assetSourceBoundsMeters.min.x * inverseCell,
       object.pointCells.y + object.assetSourceBoundsMeters.min.y * inverseCell,
       object.pointCells.z + object.assetSourceBoundsMeters.min.z * inverseCell},
      {object.pointCells.x + object.assetSourceBoundsMeters.max.x * inverseCell,
       object.pointCells.y + object.assetSourceBoundsMeters.max.y * inverseCell,
       object.pointCells.z + object.assetSourceBoundsMeters.max.z * inverseCell},
  };
  cr::CreativeTransform transform;
  transform.position = object.pointCells;
  transform.rotationEulerRadians.y = object.yawRadians;
  transform.scale = object.scale;
  const cr::CreativeTransformedBounds transformed =
      cr::resolveCreativeTransformedBounds(authoredCells, transform);
  if (!transformed.valid) {
    return result;
  }
  constexpr std::array<std::size_t, 4U> kPlanCornerIndices{0U, 1U, 5U, 4U};
  for (std::size_t index = 0U; index < result.corners.size(); ++index) {
    const cr::CreativeVec3 corner = transformed.corners[kPlanCornerIndices[index]];
    result.corners[index] = {corner.x, corner.z};
  }
  result.axisAlignedBoundsCells = transformed.worldBounds;
  result.valid = true;
  return result;
}

CreativeEditorWorldLayoutCatalogPlacementPlan
planCreativeEditorWorldLayoutCatalogPlacement(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point,
    cr::CreativeGridSettings grid) {
  CreativeEditorWorldLayoutCatalogPlacementPlan result;
  const CreativeEditorWorldLayoutCatalogPlacementState& selected =
      state.catalogPlacement;
  if (state.tool != CreativeEditorWorldLayoutTool::CatalogAsset ||
      !selected.active || !detail::finiteWorldLayoutPoint(point) ||
      selected.kind <= cr::CreativeObjectKind::Unknown ||
      selected.kind >= cr::CreativeObjectKind::Count ||
      selected.assetId.empty() || selected.label.empty() ||
      !std::isfinite(selected.elevationCells) ||
      !std::isfinite(selected.yawDegrees) ||
      !finitePositiveScale(selected.scale) ||
      !std::isfinite(grid.cellSizeMeters) || grid.cellSizeMeters <= 0.0) {
    result.reasonCode =
        "creative_editor_world_layout_catalog_placement_invalid";
    return result;
  }
  const double snappedX = std::round(point.x);
  const double snappedZ = std::round(point.z);
  if (snappedX < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
      snappedX > static_cast<double>(std::numeric_limits<std::int32_t>::max()) ||
      snappedZ < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
      snappedZ > static_cast<double>(std::numeric_limits<std::int32_t>::max())) {
    result.reasonCode =
        "creative_editor_world_layout_catalog_placement_out_of_range";
    return result;
  }

  result.object.kind = selected.kind;
  result.object.mode = cr::CreativeObjectLibraryPlacementMode::Point;
  result.object.name = selected.label;
  result.object.assetId = selected.assetId;
  result.object.pointCells = {snappedX, selected.elevationCells, snappedZ};
  result.object.assetSourceBoundsMeters = selected.sourceBoundsMeters;
  result.object.hasAssetSourceBounds = true;
  result.object.yawRadians =
      std::remainder(selected.yawDegrees, 360.0) * std::numbers::pi / 180.0;
  result.object.scale = selected.scale;
  result.object.tags = {"world_layout:object", "world_layout:catalog_asset"};
  result.footprint =
      planCreativeEditorWorldLayoutObjectFootprint(result.object, grid);
  if (!result.footprint.valid) {
    result.object = {};
    result.reasonCode =
        "creative_editor_world_layout_catalog_footprint_invalid";
    return result;
  }
  result.accepted = true;
  result.reasonCode = "creative_editor_world_layout_catalog_placement_ready";
  return result;
}

std::size_t findCreativeEditorWorldLayoutObjectAt(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point,
    cr::CreativeGridSettings grid) noexcept {
  if (!detail::finiteWorldLayoutPoint(point)) {
    return cr::kInvalidCreativeWorldLayoutIndex;
  }
  for (std::size_t index = state.source.objects.size(); index > 0U; --index) {
    const cr::CreativeWorldLayoutObject& object =
        state.source.objects[index - 1U];
    const CreativeEditorWorldLayoutObjectFootprint footprint =
        planCreativeEditorWorldLayoutObjectFootprint(object, grid);
    const bool hit =
        footprint.valid
            ? pointInsideFootprint(footprint, point)
            : object.mode == cr::CreativeObjectLibraryPlacementMode::Point &&
                  std::hypot(point.x - object.pointCells.x,
                             point.z - object.pointCells.z) <= 0.6;
    if (hit) {
      return index - 1U;
    }
  }
  return cr::kInvalidCreativeWorldLayoutIndex;
}

CreativeEditorWorldLayoutEditReceipt applyCreativeEditorWorldLayoutPoint(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point,
    cr::CreativeGridSettings grid) {
  if (state.tool != CreativeEditorWorldLayoutTool::CatalogAsset) {
    return applyCreativeEditorWorldLayoutPoint(state, point);
  }
  CreativeEditorWorldLayoutCatalogPlacementPlan plan =
      planCreativeEditorWorldLayoutCatalogPlacement(state, point, grid);
  if (!plan.accepted) {
    state.statusMessage = plan.reasonCode;
    return {false, false, std::move(plan.reasonCode)};
  }
  plan.object.stableKey = detail::mintWorldLayoutStableKey(state, "asset");
  state.source.objects.push_back(std::move(plan.object));
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Object,
                     state.source.objects.size() - 1U};
  detail::noteWorldLayoutSourceChange(state, "catalog asset added");
  return {true, true,
          "creative_editor_world_layout_catalog_asset_added"};
}

}  // namespace iggy3d_creative_app
