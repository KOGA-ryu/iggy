#include "EditorWorldLayoutCanvasInternal.hpp"

#include "EditorWorldLayoutTopography.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

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
  if (!topography.visible || !topography.elevationBandsVisible ||
      !topography.cacheValid || !plan.accepted ||
      plan.status != CreativeEditorWorldLayoutTopographyStatus::Ready) {
    return;
  }
  for (const cr::CreativeTerrainColumn& column : plan.columns) {
    const double minimumX = static_cast<double>(column.coord.x);
    const double minimumZ = static_cast<double>(column.coord.z);
    if (!overlapsCanvas(visibleBounds, minimumX, minimumX + 1.0, minimumZ,
                        minimumZ + 1.0)) {
      continue;
    }
    drawList.AddRectFilled(
        toScreen(transform, minimumX, minimumZ),
        toScreen(transform, minimumX + 1.0, minimumZ + 1.0),
        color(topographyBandColor(plan, column.heightCells)));
  }
}

void drawTopographyContours(
    ImDrawList& drawList,
    const CanvasTransform& transform,
    const CanvasWorldBounds& visibleBounds,
    const CreativeEditorWorldLayoutTopographyState& topography) {
  const CreativeEditorWorldLayoutTopographyPlan& plan = topography.plan;
  if (!topography.visible || !topography.cacheValid || !plan.accepted ||
      !plan.contours.accepted ||
      plan.contours.status != cr::CreativeTerrainContourPlanStatus::Ready) {
    return;
  }
  const ImU32 minor = color({0.35F, 0.79F, 0.76F, 0.90F});
  const ImU32 major = color({0.98F, 0.80F, 0.22F, 1.0F});
  for (const cr::CreativeTerrainContourSegment& segment :
       plan.contours.segments) {
    const double minimumX = std::min(segment.start.x, segment.end.x);
    const double maximumX = std::max(segment.start.x, segment.end.x);
    const double minimumZ = std::min(segment.start.z, segment.end.z);
    const double maximumZ = std::max(segment.start.z, segment.end.z);
    if (!overlapsCanvas(visibleBounds, minimumX, maximumX, minimumZ,
                        maximumZ)) {
      continue;
    }
    drawList.AddLine(toScreen(transform, segment.start.x, segment.start.z),
                     toScreen(transform, segment.end.x, segment.end.z),
                     segment.major ? major : minor,
                     segment.major ? 2.25F : 1.25F);
  }
}

void drawTerrainRegionSelection(
    ImDrawList& drawList,
    const CanvasTransform& transform,
    const CreativeEditorWorldLayoutTerrainRegionState& region) {
  if (!region.editingEnabled || !region.regionValid) {
    return;
  }
  const double maximumX =
      static_cast<double>(region.bounds.minimum.x) +
      region.bounds.widthCells;
  const double maximumZ =
      static_cast<double>(region.bounds.minimum.z) +
      region.bounds.depthCells;
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
      toScreen(transform, region.bounds.minimum.x, region.bounds.minimum.z);
  const ImVec2 maximum = toScreen(transform, maximumX, maximumZ);
  ImVec4 fill = tint;
  fill.w = 0.14F;
  if (region.mask == cr::CreativeTerrainCompositionMask::Ellipse) {
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
  ImGui::SetTooltip(
      "Cell X %d  Z %d\nHeight %u cells  %.2f m\nSlope %.1f deg",
      sample.coord.x, sample.coord.z,
      static_cast<unsigned int>(sample.heightCells), heightMeters,
      sample.slopeDegrees);
}

void drawTerrainSymbols(ImDrawList& drawList,
                        const CanvasTransform& transform,
                        const CreativeEditorWorldLayoutState& state) {
  const cr::CreativeWorldLayout& source =
      creativeEditorWorldLayoutDisplaySource(state);
  for (std::size_t index = 0U; index < source.terrainProfiles.size(); ++index) {
    const cr::CreativeWorldLayoutTerrainProfile& profile =
        source.terrainProfiles[index];
    const ImVec2 center =
        toScreen(transform, profile.center.x, profile.center.z);
    const float radius = std::max(
        5.0F, static_cast<float>(profile.radiusCells) * transform.pixelsPerCell);
    const bool isSelected = selected(
        state, CreativeEditorWorldLayoutSelectionKind::TerrainProfile, index);
    drawList.AddCircleFilled(center, radius,
                             color({0.25F, 0.46F, 0.28F, 0.12F}), 48);
    drawList.AddCircle(center, radius,
                       isSelected ? color({0.96F, 0.82F, 0.22F, 1.0F})
                                  : color({0.38F, 0.68F, 0.42F, 0.78F}),
                       48, isSelected ? 3.0F : 1.5F);
    drawList.AddCircleFilled(center, isSelected ? 6.0F : 4.0F,
                             isSelected
                                 ? color({0.96F, 0.82F, 0.22F, 1.0F})
                                 : color({0.38F, 0.68F, 0.42F, 1.0F}));
  }
  for (std::size_t index = 0U; index < source.terrainPaths.size(); ++index) {
    const cr::CreativeWorldLayoutTerrainPath& path = source.terrainPaths[index];
    if (path.pointCount < 2U ||
        path.firstPointIndex > source.terrainPathPoints.size() ||
        path.pointCount >
            source.terrainPathPoints.size() - path.firstPointIndex) {
      continue;
    }
    const bool isSelected = selected(
        state, CreativeEditorWorldLayoutSelectionKind::TerrainPath, index);
    const ImVec4 tint = path.kind == cr::CreativeTerrainRecipeKind::Ditch
                            ? ImVec4{0.25F, 0.47F, 0.68F, 1.0F}
                            : ImVec4{0.72F, 0.56F, 0.28F, 1.0F};
    ImVec4 fill = tint;
    fill.w = 0.24F;
    const float width = std::max(
        3.0F, static_cast<float>(path.halfWidthCells * 2U + 1U) *
                  transform.pixelsPerCell);
    for (std::size_t pointIndex = path.firstPointIndex + 1U;
         pointIndex < path.firstPointIndex + path.pointCount; ++pointIndex) {
      const cr::CreativeTerrainCoord2 startCoord =
          source.terrainPathPoints[pointIndex - 1U].coord;
      const cr::CreativeTerrainCoord2 endCoord =
          source.terrainPathPoints[pointIndex].coord;
      const ImVec2 start = toScreen(transform, startCoord.x, startCoord.z);
      const ImVec2 end = toScreen(transform, endCoord.x, endCoord.z);
      drawList.AddLine(start, end, color(fill), width);
      drawList.AddLine(start, end,
                       isSelected ? color({0.96F, 0.82F, 0.22F, 1.0F})
                                  : color(tint),
                       isSelected ? 4.0F : 2.0F);
    }
  }
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
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutTopographyState& topography) {
  const CanvasWorldBounds visibleBounds =
      canvasWorldBounds(transform, minimum, maximum);
  drawTopographyBands(drawList, transform, visibleBounds, topography);
  drawGrid(drawList, minimum, maximum, transform);
  drawTopographyContours(drawList, transform, visibleBounds, topography);
  drawTerrainRegionSelection(drawList, transform, topography.region);
  drawTerrainSymbols(drawList, transform, state);
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
