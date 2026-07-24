#include "EditorWorldLayoutCanvasInternal.hpp"

#include "EditorWorldLayoutLifecycle.hpp"
#include "EditorWorldLayoutSources.hpp"
#include "EditorDraftingStyle.hpp"
#include "EditorPlayerSpawnPreview.hpp"
#include "EditorWorldLayoutTopography.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace iggy3d_creative_app {
namespace {

using CanvasTransform = CreativeEditorWorldLayoutCanvasTransform;

ImVec2 toScreen(const CanvasTransform& transform, double x, double z) {
  return creativeEditorWorldLayoutCanvasToScreen(transform, x, z);
}

CreativeEditorWorldLayoutPoint toWorld(const CanvasTransform& transform,
                                       ImVec2 screen) {
  return creativeEditorWorldLayoutCanvasToWorld(transform, screen);
}

ImU32 color(ImVec4 value) { return ImGui::ColorConvertFloat4ToU32(value); }

ImU32 draftingColor(CreativeEditorDraftingColor value,
                    float alphaMultiplier = 1.0F) {
  const std::uint8_t alpha = static_cast<std::uint8_t>(std::clamp(
      static_cast<float>(value.a) * alphaMultiplier, 0.0F, 255.0F));
  return IM_COL32(value.r, value.g, value.b, alpha);
}

bool selected(const CreativeEditorWorldLayoutState& state,
              CreativeEditorWorldLayoutSelectionKind kind, std::size_t index) {
  return state.selection.kind == kind && state.selection.index == index;
}

void drawGrid(ImDrawList& drawList, ImVec2 minimum, ImVec2 maximum,
              const CanvasTransform& transform) {
  const CreativeEditorWorldLayoutPoint worldMinimum =
      toWorld(transform, minimum);
  const CreativeEditorWorldLayoutPoint worldMaximum =
      toWorld(transform, maximum);
  const int firstX = static_cast<int>(std::floor(worldMinimum.x));
  const int lastX = static_cast<int>(std::ceil(worldMaximum.x));
  const int firstZ = static_cast<int>(std::floor(worldMinimum.z));
  const int lastZ = static_cast<int>(std::ceil(worldMaximum.z));
  for (int x = firstX; x <= lastX; ++x) {
    const float screenX = toScreen(transform, static_cast<double>(x), 0.0).x;
    const bool major = x % 5 == 0;
    drawList.AddLine({screenX, minimum.y}, {screenX, maximum.y},
                     major ? color({0.30F, 0.34F, 0.38F, 1.0F})
                           : color({0.20F, 0.23F, 0.26F, 1.0F}),
                     major ? 1.4F : 1.0F);
  }
  for (int z = firstZ; z <= lastZ; ++z) {
    const float screenZ = toScreen(transform, 0.0, static_cast<double>(z)).y;
    const bool major = z % 5 == 0;
    drawList.AddLine({minimum.x, screenZ}, {maximum.x, screenZ},
                     major ? color({0.30F, 0.34F, 0.38F, 1.0F})
                           : color({0.20F, 0.23F, 0.26F, 1.0F}),
                     major ? 1.4F : 1.0F);
  }
  const ImVec2 zero = toScreen(transform, 0.0, 0.0);
  drawList.AddLine({zero.x, minimum.y}, {zero.x, maximum.y},
                   color({0.40F, 0.63F, 0.86F, 1.0F}), 1.8F);
  drawList.AddLine({minimum.x, zero.y}, {maximum.x, zero.y},
                   color({0.86F, 0.42F, 0.36F, 1.0F}), 1.8F);
}

struct CanvasWorldBounds {
  double minimumX = 0.0;
  double maximumX = 0.0;
  double minimumZ = 0.0;
  double maximumZ = 0.0;
};

CanvasWorldBounds canvasWorldBounds(const CanvasTransform& transform,
                                    ImVec2 minimum,
                                    ImVec2 maximum) noexcept {
  const CreativeEditorWorldLayoutPoint first = toWorld(transform, minimum);
  const CreativeEditorWorldLayoutPoint second = toWorld(transform, maximum);
  return {std::min(first.x, second.x), std::max(first.x, second.x),
          std::min(first.z, second.z), std::max(first.z, second.z)};
}

bool overlapsCanvas(const CanvasWorldBounds& bounds,
                    double minimumX,
                    double maximumX,
                    double minimumZ,
                    double maximumZ) noexcept {
  return maximumX >= bounds.minimumX && minimumX <= bounds.maximumX &&
         maximumZ >= bounds.minimumZ && minimumZ <= bounds.maximumZ;
}

ImVec4 mixColor(ImVec4 first, ImVec4 second, float amount) noexcept {
  const float t = std::clamp(amount, 0.0F, 1.0F);
  return {first.x + (second.x - first.x) * t,
          first.y + (second.y - first.y) * t,
          first.z + (second.z - first.z) * t,
          first.w + (second.w - first.w) * t};
}

ImVec4 topographyBandColor(
    const CreativeEditorWorldLayoutTopographyPlan& plan,
    std::uint16_t heightCells) noexcept {
  constexpr ImVec4 low{0.12F, 0.31F, 0.22F, 0.82F};
  constexpr ImVec4 middle{0.34F, 0.36F, 0.22F, 0.84F};
  constexpr ImVec4 high{0.48F, 0.43F, 0.40F, 0.88F};
  const std::uint16_t range =
      plan.maximumHeightCells - plan.minimumHeightCells;
  if (range == 0U) {
    return middle;
  }
  constexpr float bandCount = 10.0F;
  const float normalized =
      static_cast<float>(heightCells - plan.minimumHeightCells) /
      static_cast<float>(range);
  const float banded = std::floor(normalized * bandCount) / bandCount;
  return banded <= 0.5F
             ? mixColor(low, middle, banded * 2.0F)
             : mixColor(middle, high, (banded - 0.5F) * 2.0F);
}

void drawTopographyBands(
    ImDrawList& drawList,
    const CanvasTransform& transform,
    const CanvasWorldBounds& visibleBounds,
    const CreativeEditorWorldLayoutTopographyState& topography) {
  const CreativeEditorWorldLayoutTopographyPlan& plan = topography.plan;
  if (!topography.visible || !topography.cacheValid || !plan.accepted ||
      plan.status != CreativeEditorWorldLayoutTopographyStatus::Ready) {
    return;
  }
  for (const cr::CreativeTerrainAnalysisCell& cell : plan.analysis.cells) {
    const double minimumX = static_cast<double>(cell.coord.x);
    const double minimumZ = static_cast<double>(cell.coord.z);
    if (!overlapsCanvas(visibleBounds, minimumX, minimumX + 1.0, minimumZ,
                        minimumZ + 1.0)) {
      continue;
    }
    const ImVec2 minimum = toScreen(transform, minimumX, minimumZ);
    const ImVec2 maximum =
        toScreen(transform, minimumX + 1.0, minimumZ + 1.0);
    if (cell.terrainPresent && topography.elevationBandsVisible) {
      drawList.AddRectFilled(
          minimum, maximum,
          color(topographyBandColor(plan, cell.heightCells)));
    }
    if (cell.terrainPresent && topography.slopeBandsVisible) {
      CreativeEditorDraftingRole role = CreativeEditorDraftingRole::SlopeFlat;
      switch (cell.slopeBand) {
        case cr::CreativeTerrainSlopeBand::Flat:
          role = CreativeEditorDraftingRole::SlopeFlat;
          break;
        case cr::CreativeTerrainSlopeBand::Gentle:
          role = CreativeEditorDraftingRole::SlopeGentle;
          break;
        case cr::CreativeTerrainSlopeBand::Steep:
          role = CreativeEditorDraftingRole::SlopeSteep;
          break;
        case cr::CreativeTerrainSlopeBand::Extreme:
          role = CreativeEditorDraftingRole::SlopeExtreme;
          break;
        case cr::CreativeTerrainSlopeBand::Unavailable:
        case cr::CreativeTerrainSlopeBand::Count:
          continue;
      }
      const CreativeEditorDraftingStyle& style =
          creativeEditorDraftingStyle(role);
      drawList.AddRectFilled(minimum, maximum,
                             draftingColor(style.tint, style.fillAlpha));
    }
    if (topography.cutFillVisible && plan.analysis.hasReference &&
        cell.cutFill != cr::CreativeTerrainCutFillKind::Unchanged) {
      const CreativeEditorDraftingRole role =
          cell.cutFill == cr::CreativeTerrainCutFillKind::Cut
              ? CreativeEditorDraftingRole::CutArea
              : CreativeEditorDraftingRole::FillArea;
      const CreativeEditorDraftingStyle& style =
          creativeEditorDraftingStyle(role);
      const float magnitude = std::clamp(
          static_cast<float>(std::abs(cell.deltaCells)) /
              static_cast<float>(cr::kCreativeTerrainMaximumHeightCells),
          0.0F, 1.0F);
      const float alpha = std::clamp(style.fillAlpha + magnitude * 0.28F,
                                     style.fillAlpha, 0.72F);
      drawList.AddRectFilled(minimum, maximum,
                             draftingColor(style.tint, alpha));
      if (style.fillPattern == CreativeEditorDraftingFillPattern::Hatched) {
        drawList.AddLine(minimum, maximum, draftingColor(style.tint), 1.0F);
        drawList.AddLine({minimum.x, maximum.y}, {maximum.x, minimum.y},
                         draftingColor(style.tint), 1.0F);
      }
    }
  }
}

void drawTopographyLabels(
    ImDrawList& drawList,
    const CanvasTransform& transform,
    const CreativeEditorWorldLayoutTopographyState& topography,
    cr::CreativeGridSettings grid) {
  const CreativeEditorWorldLayoutTopographyPlan& plan = topography.plan;
  if (!topography.visible || !topography.contourLabelsVisible ||
      !topography.cacheValid || !plan.accepted ||
      plan.status != CreativeEditorWorldLayoutTopographyStatus::Ready ||
      !plan.analysis.contours.accepted) {
    return;
  }
  const CreativeEditorDraftingStyle& style = creativeEditorDraftingStyle(
      CreativeEditorDraftingRole::ContourMajor);
  for (const cr::CreativeTerrainContourLabel& label : plan.analysis.labels) {
    char text[48];
    const double meters =
        static_cast<double>(label.levelCells) * grid.cellSizeMeters;
    const int length = std::snprintf(
        text, sizeof(text), "%u / %.2f m",
        static_cast<unsigned int>(label.levelCells), meters);
    if (length <= 0) {
      continue;
    }
    const ImVec2 anchor =
        toScreen(transform, label.point.x, label.point.z);
    const ImVec2 size = ImGui::CalcTextSize(text);
    const ImVec2 minimum{anchor.x - size.x * 0.5F - 3.0F,
                         anchor.y - size.y * 0.5F - 2.0F};
    const ImVec2 maximum{anchor.x + size.x * 0.5F + 3.0F,
                         anchor.y + size.y * 0.5F + 2.0F};
    drawList.AddRectFilled(minimum, maximum, IM_COL32(25, 29, 32, 220), 2.0F);
    drawList.AddText({anchor.x - size.x * 0.5F,
                      anchor.y - size.y * 0.5F},
                     draftingColor(style.tint), text);
  }
}

void drawTerrainRegionSelection(
    ImDrawList& drawList,
    const CanvasTransform& transform,
    const CreativeEditorWorldLayoutTerrainRegionState& region) {
  if (!region.editingEnabled || !region.regionValid) {
    return;
  }
  const cr::CreativeTerrainRegionRecipe& recipe = region.recipe;
  const double maximumX =
      static_cast<double>(recipe.bounds.minimum.x) +
      recipe.bounds.widthCells;
  const double maximumZ =
      static_cast<double>(recipe.bounds.minimum.z) +
      recipe.bounds.depthCells;
  const CreativeEditorWorldLayoutTerrainRegionPhase phase =
      classifyCreativeEditorWorldLayoutTerrainRegionPhase(region);
  ImVec4 tint{0.30F, 0.72F, 1.0F, 1.0F};  // AwaitingPreview: blue draft.
  switch (phase) {
    case CreativeEditorWorldLayoutTerrainRegionPhase::Selecting:
      tint = ImVec4{0.98F, 0.85F, 0.20F, 1.0F};
      break;
    case CreativeEditorWorldLayoutTerrainRegionPhase::Ready:
      tint = ImVec4{0.25F, 0.95F, 0.48F, 1.0F};
      break;
    case CreativeEditorWorldLayoutTerrainRegionPhase::Rejected:
      tint = ImVec4{0.95F, 0.30F, 0.28F, 1.0F};
      break;
    default:
      break;
  }
  const ImVec2 minimum =
      toScreen(transform, recipe.bounds.minimum.x, recipe.bounds.minimum.z);
  const ImVec2 maximum = toScreen(transform, maximumX, maximumZ);
  ImVec4 fill = tint;
  fill.w = 0.14F;
  if (recipe.mask == cr::CreativeTerrainCompositionMask::Ellipse) {
    const ImVec2 center{(minimum.x + maximum.x) * 0.5F,
                        (minimum.y + maximum.y) * 0.5F};
    const ImVec2 radius{(maximum.x - minimum.x) * 0.5F,
                        (maximum.y - minimum.y) * 0.5F};
    drawList.AddEllipseFilled(center, radius, color(fill));
    drawList.AddEllipse(center, radius, color(tint), 0.0F, 0, 2.5F);
    if (region.selecting) {
      // Thin bounds guide so the drag rectangle stays aimable while the
      // composition mask is elliptical.
      drawList.AddRect(minimum, maximum, color(fill), 0.0F, 0, 1.0F);
    }
  } else {
    drawList.AddRectFilled(minimum, maximum, color(fill));
    drawList.AddRect(minimum, maximum, color(tint), 0.0F, 0, 2.5F);
  }
  if (region.selecting) {
    return;
  }

  const ImVec2 center{(minimum.x + maximum.x) * 0.5F,
                      (minimum.y + maximum.y) * 0.5F};
  constexpr float handleHalfSize = 4.0F;
  const std::array handles{
      minimum,
      ImVec2{center.x, minimum.y},
      ImVec2{maximum.x, minimum.y},
      ImVec2{minimum.x, center.y},
      ImVec2{maximum.x, center.y},
      ImVec2{minimum.x, maximum.y},
      ImVec2{center.x, maximum.y},
      maximum,
  };
  for (const ImVec2 handle : handles) {
    drawList.AddRectFilled(
        {handle.x - handleHalfSize, handle.y - handleHalfSize},
        {handle.x + handleHalfSize, handle.y + handleHalfSize}, color(tint));
    drawList.AddRect(
        {handle.x - handleHalfSize, handle.y - handleHalfSize},
        {handle.x + handleHalfSize, handle.y + handleHalfSize},
        IM_COL32(20, 24, 28, 255));
  }
  drawList.AddCircleFilled(center, handleHalfSize, color(tint));
  drawList.AddCircle(center, handleHalfSize, IM_COL32(20, 24, 28, 255));
}

void drawTopographyHoverFacts(
    const CreativeEditorWorldLayoutTopographyState& topography,
    CreativeEditorWorldLayoutPoint point,
    cr::CreativeGridSettings grid) {
  if (!topography.visible || !topography.cacheValid ||
      !topography.plan.accepted) {
    return;
  }
  const CreativeEditorWorldLayoutTopographySample sample =
      sampleCreativeEditorWorldLayoutTopography(topography.plan, point.x,
                                                point.z);
  if (!sample.present) {
    return;
  }
  const double heightMeters =
      static_cast<double>(sample.heightCells) * grid.cellSizeMeters;
  const std::string_view slopeBand = cr::toString(sample.slopeBand);
  if (sample.cutFill != cr::CreativeTerrainCutFillKind::Unchanged) {
    const std::string_view cutFill = cr::toString(sample.cutFill);
    ImGui::SetTooltip(
        "Cell X %d  Z %d\nHeight %u cells  %.2f m\nSlope %.1f deg  %.*s\n%.*s %d cells",
        sample.coord.x, sample.coord.z,
        static_cast<unsigned int>(sample.heightCells), heightMeters,
        sample.slopeDegrees, static_cast<int>(slopeBand.size()),
        slopeBand.data(), static_cast<int>(cutFill.size()), cutFill.data(),
        std::abs(static_cast<int>(sample.deltaCells)));
    return;
  }
  ImGui::SetTooltip(
      "Cell X %d  Z %d\nHeight %u cells  %.2f m\nSlope %.1f deg  %.*s",
      sample.coord.x, sample.coord.z,
      static_cast<unsigned int>(sample.heightCells), heightMeters,
      sample.slopeDegrees,
      static_cast<int>(slopeBand.size()), slopeBand.data());
}

void drawObjectSymbols(ImDrawList& drawList,
                       const CanvasTransform& transform,
                       const CreativeEditorWorldLayoutState& state,
                       cr::CreativeGridSettings grid) {
  const cr::CreativeWorldLayout& source =
      creativeEditorWorldLayoutDisplaySource(state);
  for (std::size_t index = 0U; index < source.objects.size(); ++index) {
    const cr::CreativeWorldLayoutObject& object = source.objects[index];
    const bool isSelected = selected(
        state, CreativeEditorWorldLayoutSelectionKind::Object, index);
    const bool previewing = state.objectManipulation.active &&
                            state.objectManipulation.objectIndex == index &&
                            state.objectManipulation.sourceRevision ==
                                state.revision;
    if (!isSelected && !previewing) {
      continue;
    }
    const CreativeEditorWorldLayoutObjectSettings* preview =
        previewing ? &state.objectManipulation.previewSettings : nullptr;
    const ImU32 outline =
        previewing ? (state.objectManipulation.previewValid
                          ? color({0.20F, 0.78F, 0.38F, 1.0F})
                          : color({0.92F, 0.29F, 0.24F, 1.0F}))
        : isSelected ? color({0.96F, 0.82F, 0.22F, 1.0F})
        : object.kind == cr::CreativeObjectKind::Rock
            ? color({0.66F, 0.69F, 0.72F, 1.0F})
        : object.kind == cr::CreativeObjectKind::SpawnPoint
            ? color({0.24F, 0.86F, 0.43F, 1.0F})
        : object.kind == cr::CreativeObjectKind::NpcSpawn
            ? color({0.72F, 0.42F, 0.88F, 1.0F})
            : color({0.90F, 0.58F, 0.25F, 1.0F});
    cr::CreativeWorldLayoutObject displayed = object;
    if (preview != nullptr) {
      displayed.mode = preview->mode;
      displayed.boundsCells = preview->boundsCells;
      displayed.pointCells = preview->pointCells;
      displayed.assetSourceBoundsMeters = preview->assetSourceBoundsMeters;
      displayed.hasAssetSourceBounds = preview->hasAssetSourceBounds;
      displayed.yawRadians = preview->yawRadians;
      displayed.scale = preview->scale;
      displayed.playerSpawn = preview->playerSpawn;
    }
    ImVec2 labelAnchor;
    const CreativeEditorWorldLayoutObjectFootprint footprint =
        planCreativeEditorWorldLayoutObjectFootprint(displayed, grid);
    if (footprint.valid) {
      std::array<ImVec2, 4U> points;
      for (std::size_t pointIndex = 0U; pointIndex < points.size(); ++pointIndex) {
        points[pointIndex] = toScreen(transform, footprint.corners[pointIndex].x,
                                     footprint.corners[pointIndex].z);
      }
      drawList.AddConvexPolyFilled(
          points.data(), static_cast<int>(points.size()),
          previewing
              ? (state.objectManipulation.previewValid
                     ? color({0.20F, 0.78F, 0.38F, 0.22F})
                     : color({0.92F, 0.29F, 0.24F, 0.22F}))
              : color({0.54F, 0.48F, 0.40F, 0.28F}));
      drawList.AddPolyline(points.data(), static_cast<int>(points.size()),
                           outline, ImDrawFlags_Closed,
                           previewing || isSelected ? 3.0F : 1.8F);
      labelAnchor = points.front();
    } else {
      const ImVec2 center =
          toScreen(transform, displayed.pointCells.x, displayed.pointCells.z);
      drawList.AddCircleFilled(center, previewing || isSelected ? 7.0F : 5.0F,
                               outline);
      drawList.AddCircle(center, previewing || isSelected ? 11.0F : 8.0F,
                         outline, 16,
                         previewing || isSelected ? 3.0F : 1.5F);
      if (displayed.kind == cr::CreativeObjectKind::SpawnPoint) {
        const CreativePlayerSpawnPlanSymbol spawnSymbol =
            planCreativePlayerSpawnPlanSymbol(
                displayed.pointCells, displayed.yawRadians,
                displayed.playerSpawn, grid.cellSizeMeters);
        if (spawnSymbol.drawable) {
          const ImU32 spawnTint =
              spawnSymbol.runtimeReady ? color({0.24F, 0.86F, 0.43F, 1.0F})
                                       : color({0.92F, 0.29F, 0.24F, 1.0F});
          drawList.AddCircle(
              center,
              static_cast<float>(spawnSymbol.bodyRadiusCells) *
                  transform.pixelsPerCell,
              spawnTint, 32, 2.5F);
          drawList.AddCircle(
              center,
              static_cast<float>(spawnSymbol.clearanceRadiusCells) *
                  transform.pixelsPerCell,
              spawnTint, 48, 1.25F);
          drawList.AddLine(
              center,
              toScreen(transform, spawnSymbol.facingEndCells.x,
                       spawnSymbol.facingEndCells.z),
              spawnTint, 3.0F);
          drawList.AddLine({center.x - 4.0F, center.y},
                           {center.x + 4.0F, center.y}, spawnTint, 2.0F);
          drawList.AddLine({center.x, center.y - 4.0F},
                           {center.x, center.y + 4.0F}, spawnTint, 2.0F);
        }
      }
      labelAnchor = center;
    }
    if (previewing || isSelected) {
      drawList.AddText({labelAnchor.x + 7.0F, labelAnchor.y + 7.0F}, outline,
                       object.name.c_str());
    }
  }
}


}  // namespace

void drawCreativeEditorWorldLayoutTerrainBackground(
    ImDrawList& drawList, ImVec2 minimum, ImVec2 maximum,
    const CreativeEditorWorldLayoutCanvasTransform& transform,
    const CreativeEditorWorldLayoutTopographyState& topography) {
  const CanvasWorldBounds visibleBounds =
      canvasWorldBounds(transform, minimum, maximum);
  drawTopographyBands(drawList, transform, visibleBounds, topography);
  drawGrid(drawList, minimum, maximum, transform);
  drawTerrainRegionSelection(drawList, transform, topography.region);
}

void drawCreativeEditorWorldLayoutTerrainAnnotations(
    ImDrawList& drawList,
    const CreativeEditorWorldLayoutCanvasTransform& transform,
    const CreativeEditorWorldLayoutTopographyState& topography,
    const cr::CreativeGridSettings& grid) {
  drawTopographyLabels(drawList, transform, topography, grid);
}

void drawCreativeEditorWorldLayoutObjectSymbols(
    ImDrawList& drawList,
    const CreativeEditorWorldLayoutCanvasTransform& transform,
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeGridSettings& grid) {
  drawObjectSymbols(drawList, transform, state, grid);
}

void drawCreativeEditorWorldLayoutTopographyHoverFacts(
    const CreativeEditorWorldLayoutTopographyState& topography,
    CreativeEditorWorldLayoutPoint point, cr::CreativeGridSettings grid) {
  drawTopographyHoverFacts(topography, point, grid);
}

}  // namespace iggy3d_creative_app
