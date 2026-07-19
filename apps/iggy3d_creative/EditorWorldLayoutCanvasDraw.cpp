#include "EditorWorldLayoutCanvasInternal.hpp"

#include "EditorDesktopModel.hpp"
#include "EditorWorldLayoutTopography.hpp"

#include "app/iggy3d/creative/world/WorldLayoutLevels.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRoofs.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <utility>

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

ImVec4 generatedScopeTint(cr::CreativeWorldLayoutTable table) {
  const CreativeDesktopGeneratedSourceScopeTint tint =
      creativeDesktopGeneratedSourceScopeTint(table);
  return {tint.r, tint.g, tint.b, tint.a};
}

bool selected(const CreativeEditorWorldLayoutState& state,
              CreativeEditorWorldLayoutSelectionKind kind, std::size_t index) {
  return state.selection.kind == kind && state.selection.index == index;
}

bool roomOnActiveLevel(const CreativeEditorWorldLayoutState& state,
                       const cr::CreativeWorldLayout& source,
                       std::size_t roomIndex) noexcept {
  return roomIndex < source.rooms.size() &&
         (state.activeLevelIndex >= source.levels.size() ||
          source.rooms[roomIndex].levelIndex == state.activeLevelIndex);
}

bool openingOnActiveLevel(const CreativeEditorWorldLayoutState& state,
                          const cr::CreativeWorldLayout& source,
                          std::size_t openingIndex) noexcept {
  if (openingIndex >= source.openings.size()) {
    return false;
  }
  const cr::CreativeWorldLayoutOpening& opening = source.openings[openingIndex];
  return opening.hostKind !=
             cr::CreativeWorldLayoutOpeningHostKind::RoomEdge ||
         roomOnActiveLevel(state, source, opening.roomIndex);
}

std::pair<double, double> buildingPreviewOffset(
    const CreativeEditorWorldLayoutState& state,
    std::size_t buildingIndex) noexcept {
  if (!state.buildingManipulation.active ||
      state.buildingManipulation.buildingIndex != buildingIndex) {
    return {0.0, 0.0};
  }
  return {
      static_cast<double>(state.buildingManipulation.previewDeltaXCells),
      static_cast<double>(state.buildingManipulation.previewDeltaZCells),
  };
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

void drawFloor(ImDrawList& drawList, const CanvasTransform& transform,
               const CreativeEditorWorldLayoutState& state,
               std::size_t boxIndex) {
  const cr::CreativeWorldLayoutBox& box =
      creativeEditorWorldLayoutDisplaySource(state).boxes[boxIndex];
  const auto [deltaX, deltaZ] =
      buildingPreviewOffset(state, box.buildingIndex);
  const ImVec2 minimum =
      toScreen(transform, box.footprint.minimum.x + deltaX,
               box.footprint.minimum.z + deltaZ);
  const ImVec2 maximum =
      toScreen(transform, box.footprint.maximum.x + deltaX,
               box.footprint.maximum.z + deltaZ);
  const bool isSelected = selected(
      state, CreativeEditorWorldLayoutSelectionKind::Box, boxIndex);
  drawList.AddRectFilled(minimum, maximum, color({0.32F, 0.42F, 0.37F, 0.72F}));
  drawList.AddRect(minimum, maximum,
                   isSelected ? color({0.96F, 0.82F, 0.22F, 1.0F})
                              : color({0.53F, 0.70F, 0.59F, 1.0F}),
                   0.0F, 0, isSelected ? 3.0F : 1.5F);
}

void drawRoom(ImDrawList& drawList, const CanvasTransform& transform,
              const CreativeEditorWorldLayoutState& state,
              std::size_t roomIndex) {
  const cr::CreativeWorldLayoutRoom& room =
      creativeEditorWorldLayoutDisplaySource(state).rooms[roomIndex];
  const auto [deltaX, deltaZ] =
      buildingPreviewOffset(state, room.buildingIndex);
  const ImVec2 minimum =
      toScreen(transform, room.footprint.minimum.x + deltaX,
               room.footprint.minimum.z + deltaZ);
  const ImVec2 maximum =
      toScreen(transform, room.footprint.maximum.x + deltaX,
               room.footprint.maximum.z + deltaZ);
  const bool isSelected = selected(
      state, CreativeEditorWorldLayoutSelectionKind::Room, roomIndex);
  drawList.AddRectFilled(minimum, maximum,
                         color({0.22F, 0.34F, 0.42F, 0.38F}));
  drawList.AddRect(minimum, maximum,
                   isSelected
                       ? color(generatedScopeTint(
                             cr::CreativeWorldLayoutTable::Room))
                              : color({0.70F, 0.78F, 0.86F, 1.0F}),
                   0.0F, 0, isSelected ? 4.0F : 3.0F);
}

void drawLevelSelection(ImDrawList& drawList,
                        const CanvasTransform& transform,
                        const CreativeEditorWorldLayoutState& state) {
  const cr::CreativeWorldLayout& source =
      creativeEditorWorldLayoutDisplaySource(state);
  if (state.buildingTemplatePlacement.active ||
      state.selection.kind !=
          CreativeEditorWorldLayoutSelectionKind::Level ||
      state.selection.index >= source.levels.size()) {
    return;
  }

  bool haveBounds = false;
  double minimumX = 0.0;
  double minimumZ = 0.0;
  double maximumX = 0.0;
  double maximumZ = 0.0;
  const cr::CreativeWorldLayoutLevel& level =
      source.levels[state.selection.index];
  for (const cr::CreativeWorldLayoutRoom& room : source.rooms) {
    if (room.levelIndex != state.selection.index ||
        room.buildingIndex != level.buildingIndex) {
      continue;
    }
    const auto [deltaX, deltaZ] =
        buildingPreviewOffset(state, room.buildingIndex);
    const double roomMinimumX = room.footprint.minimum.x + deltaX;
    const double roomMinimumZ = room.footprint.minimum.z + deltaZ;
    const double roomMaximumX = room.footprint.maximum.x + deltaX;
    const double roomMaximumZ = room.footprint.maximum.z + deltaZ;
    if (!haveBounds) {
      minimumX = roomMinimumX;
      minimumZ = roomMinimumZ;
      maximumX = roomMaximumX;
      maximumZ = roomMaximumZ;
      haveBounds = true;
      continue;
    }
    minimumX = std::min(minimumX, roomMinimumX);
    minimumZ = std::min(minimumZ, roomMinimumZ);
    maximumX = std::max(maximumX, roomMaximumX);
    maximumZ = std::max(maximumZ, roomMaximumZ);
  }
  if (!haveBounds) {
    return;
  }

  const ImVec2 minimum = toScreen(transform, minimumX, minimumZ);
  const ImVec2 maximum = toScreen(transform, maximumX, maximumZ);
  const ImVec4 tint =
      generatedScopeTint(cr::CreativeWorldLayoutTable::Level);
  ImVec4 fill = tint;
  fill.w = 0.05F;
  drawList.AddRectFilled(minimum, maximum, color(fill));
  drawList.AddRect(minimum, maximum, color(tint), 0.0F, 0, 3.0F);
  drawList.AddText({minimum.x + 8.0F, minimum.y + 7.0F}, color(tint),
                   level.name.c_str());
}

void drawActiveLevelRoof(ImDrawList& drawList,
                         const CanvasTransform& transform,
                         const CreativeEditorWorldLayoutState& state) {
  const cr::CreativeWorldLayout& source =
      creativeEditorWorldLayoutDisplaySource(state);
  if (state.activeLevelIndex >= source.levels.size()) {
    return;
  }
  const cr::CreativeWorldLayoutLevel& level =
      source.levels[state.activeLevelIndex];
  const bool drawsAuthoredFootprint =
      level.roofStyle == cr::CreativeStructuralRoofStyle::Gable ||
      level.roofOverhangCells > 0.0;
  if (!drawsAuthoredFootprint ||
      !cr::creativeWorldLayoutLevelIsTopmostOccupied(
          source, state.activeLevelIndex)) {
    return;
  }
  cr::CreativeWorldLayoutRect footprint;
  if (!cr::creativeWorldLayoutLevelRoofFootprint(
          source, state.activeLevelIndex, footprint)) {
    return;
  }
  const auto [deltaX, deltaZ] =
      buildingPreviewOffset(state, level.buildingIndex);
  const double minimumX = footprint.minimum.x - level.roofOverhangCells +
                          deltaX;
  const double maximumX = footprint.maximum.x + level.roofOverhangCells +
                          deltaX;
  const double minimumZ = footprint.minimum.z - level.roofOverhangCells +
                          deltaZ;
  const double maximumZ = footprint.maximum.z + level.roofOverhangCells +
                          deltaZ;
  const ImVec2 first = toScreen(transform, minimumX, minimumZ);
  const ImVec2 second = toScreen(transform, maximumX, maximumZ);
  const ImVec2 minimum{std::min(first.x, second.x),
                       std::min(first.y, second.y)};
  const ImVec2 maximum{std::max(first.x, second.x),
                       std::max(first.y, second.y)};
  const ImU32 outline = color({0.34F, 0.86F, 0.56F, 0.92F});
  drawList.AddRectFilled(minimum, maximum,
                         color({0.20F, 0.54F, 0.34F, 0.10F}));
  drawList.AddRect(minimum, maximum, outline, 0.0F, 0, 2.0F);
  if (level.roofStyle != cr::CreativeStructuralRoofStyle::Gable) {
    return;
  }
  if (level.roofRidgeAxis == cr::CreativeStructuralRoofRidgeAxis::X) {
    const double centerZ = (minimumZ + maximumZ) * 0.5;
    drawList.AddLine(toScreen(transform, minimumX, centerZ),
                     toScreen(transform, maximumX, centerZ), outline, 3.0F);
  } else {
    const double centerX = (minimumX + maximumX) * 0.5;
    drawList.AddLine(toScreen(transform, centerX, minimumZ),
                     toScreen(transform, centerX, maximumZ), outline, 3.0F);
  }
}

void drawSharedRoomEdges(ImDrawList& drawList,
                         const CanvasTransform& transform,
                         const CreativeEditorWorldLayoutState& state) {
  const cr::CreativeWorldLayout& source =
      creativeEditorWorldLayoutDisplaySource(state);
  const auto spans = cr::inspectCreativeWorldLayoutSharedRoomEdges(source);
  const ImU32 sharedColor = color({0.26F, 0.84F, 0.58F, 1.0F});
  for (const cr::CreativeWorldLayoutSharedRoomEdgeSpan& span : spans) {
    if (!roomOnActiveLevel(state, source, span.firstRoomIndex)) {
      continue;
    }
    const auto [deltaX, deltaZ] =
        buildingPreviewOffset(state,
                              source.rooms[span.firstRoomIndex].buildingIndex);
    drawList.AddLine(
        toScreen(transform, span.start.x + deltaX, span.start.z + deltaZ),
        toScreen(transform, span.end.x + deltaX, span.end.z + deltaZ),
        sharedColor, 4.0F);
  }
}

std::pair<ImVec2, ImVec2> screenRect(
    const CanvasTransform& transform, cr::CreativeWorldLayoutRect rect) {
  const ImVec2 first =
      toScreen(transform, rect.minimum.x, rect.minimum.z);
  const ImVec2 second =
      toScreen(transform, rect.maximum.x, rect.maximum.z);
  return {{std::min(first.x, second.x), std::min(first.y, second.y)},
          {std::max(first.x, second.x), std::max(first.y, second.y)}};
}

void drawRectManipulation(ImDrawList& drawList,
                          const CanvasTransform& transform,
                          cr::CreativeWorldLayoutRect footprint, bool active,
                          bool previewValid);

void drawVerticalConnector(ImDrawList& drawList,
                           const CanvasTransform& transform,
                           const CreativeEditorWorldLayoutState& state,
                           std::size_t connectorIndex) {
  const cr::CreativeWorldLayout& source =
      creativeEditorWorldLayoutDisplaySource(state);
  if (!creativeEditorWorldLayoutVerticalConnectorOnActiveLevel(
          state, source, connectorIndex)) {
    return;
  }
  const cr::CreativeWorldLayoutVerticalConnector& connector =
      source.verticalConnectors[connectorIndex];
  const bool active = state.verticalConnectorManipulation.active &&
                      state.verticalConnectorManipulation.target
                              .connectorIndex == connectorIndex;
  const auto [deltaX, deltaZ] =
      buildingPreviewOffset(state, connector.buildingIndex);
  const cr::CreativeWorldLayoutRect footprint =
      active ? state.verticalConnectorManipulation.previewFootprint
             : connector.footprint;
  const cr::CreativeWorldLayoutVerticalDirection direction =
      active ? state.verticalConnectorManipulation.previewDirection
             : connector.direction;
  const ImVec2 first = toScreen(transform, footprint.minimum.x + deltaX,
                                footprint.minimum.z + deltaZ);
  const ImVec2 second = toScreen(transform, footprint.maximum.x + deltaX,
                                 footprint.maximum.z + deltaZ);
  const ImVec2 minimum{std::min(first.x, second.x),
                       std::min(first.y, second.y)};
  const ImVec2 maximum{std::max(first.x, second.x),
                       std::max(first.y, second.y)};
  const bool isSelected =
      selected(state, CreativeEditorWorldLayoutSelectionKind::VerticalConnector,
               connectorIndex);
  const cr::CreativeWorldLayoutVerticalConnectorKind kind =
      active ? state.verticalConnectorManipulation.previewKind
             : connector.kind;
  const bool isRamp =
      kind == cr::CreativeWorldLayoutVerticalConnectorKind::Ramp;
  const ImU32 outline = active && !state.verticalConnectorManipulation.previewValid
                            ? color({0.92F, 0.29F, 0.24F, 1.0F})
                        : isSelected ? color({0.96F, 0.82F, 0.22F, 1.0F})
                        : isRamp   ? color({0.92F, 0.58F, 0.20F, 1.0F})
                                   : color({0.24F, 0.72F, 0.88F, 1.0F});
  drawList.AddRectFilled(minimum, maximum,
                         isRamp ? color({0.72F, 0.38F, 0.12F, 0.24F})
                                : color({0.18F, 0.55F, 0.72F, 0.24F}));
  drawList.AddRect(minimum, maximum, outline, 0.0F, 0,
                   isSelected ? 3.0F : 2.0F);

  CreativeEditorWorldLayoutPoint low;
  CreativeEditorWorldLayoutPoint high;
  if (!resolveCreativeEditorWorldLayoutVerticalConnectorAxis(
          footprint, direction, low, high)) {
    return;
  }
  low.x += deltaX;
  low.z += deltaZ;
  high.x += deltaX;
  high.z += deltaZ;
  const ImVec2 lowScreen = toScreen(transform, low.x, low.z);
  const ImVec2 highScreen = toScreen(transform, high.x, high.z);
  drawList.AddLine(lowScreen, highScreen, outline, 2.5F);
  const float dx = highScreen.x - lowScreen.x;
  const float dy = highScreen.y - lowScreen.y;
  const float length = std::hypot(dx, dy);
  if (length > 0.0F) {
    const float ux = dx / length;
    const float uy = dy / length;
    const ImVec2 base{highScreen.x - ux * 11.0F, highScreen.y - uy * 11.0F};
    drawList.AddTriangleFilled(
        highScreen, {base.x - uy * 5.5F, base.y + ux * 5.5F},
        {base.x + uy * 5.5F, base.y - ux * 5.5F}, outline);
  }
  if (!isRamp) {
    for (int tread = 1; tread < 6; ++tread) {
      const float t = static_cast<float>(tread) / 6.0F;
      if (std::fabs(dx) >= std::fabs(dy)) {
        const float x = lowScreen.x + dx * t;
        drawList.AddLine({x, minimum.y + 3.0F}, {x, maximum.y - 3.0F}, outline,
                         1.0F);
      } else {
        const float y = lowScreen.y + dy * t;
        drawList.AddLine({minimum.x + 3.0F, y}, {maximum.x - 3.0F, y}, outline,
                         1.0F);
      }
    }
  }
  if (isSelected) {
    drawRectManipulation(drawList, transform, footprint, active,
                         !active ||
                             state.verticalConnectorManipulation.previewValid);
    CreativeEditorWorldLayoutPoint directionHandle;
    if (resolveCreativeEditorWorldLayoutVerticalConnectorDirectionHandle(
            footprint, direction, directionHandle)) {
      directionHandle.x += deltaX;
      directionHandle.z += deltaZ;
      const ImVec2 handleScreen =
          toScreen(transform, directionHandle.x, directionHandle.z);
      drawList.AddLine(highScreen, handleScreen, outline, 2.0F);
      drawList.AddCircleFilled(handleScreen, 5.0F, outline);
      drawList.AddCircle(handleScreen, 8.0F, outline, 0, 1.5F);
    }
  }
}

void drawRectManipulation(ImDrawList& drawList,
                          const CanvasTransform& transform,
                          cr::CreativeWorldLayoutRect footprint, bool active,
                          bool previewValid) {
  const auto [minimum, maximum] = screenRect(transform, footprint);
  if (active) {
    const ImVec4 tint = previewValid
                            ? ImVec4{0.20F, 0.78F, 0.38F, 1.0F}
                            : ImVec4{0.92F, 0.29F, 0.24F, 1.0F};
    ImVec4 fill = tint;
    fill.w = 0.24F;
    drawList.AddRectFilled(minimum, maximum, color(fill));
    drawList.AddRect(minimum, maximum, color(tint), 0.0F, 0, 3.0F);
    const std::int64_t width =
        static_cast<std::int64_t>(footprint.maximum.x) - footprint.minimum.x;
    const std::int64_t depth =
        static_cast<std::int64_t>(footprint.maximum.z) - footprint.minimum.z;
    const std::string dimensions =
        std::to_string(width) + " x " + std::to_string(depth);
    drawList.AddText({minimum.x + 7.0F, minimum.y + 7.0F}, color(tint),
                     dimensions.c_str());
  }

  const float centerX = (minimum.x + maximum.x) * 0.5F;
  const float centerY = (minimum.y + maximum.y) * 0.5F;
  const std::array<ImVec2, 8U> handles = {
      ImVec2{minimum.x, minimum.y}, ImVec2{centerX, minimum.y},
      ImVec2{maximum.x, minimum.y}, ImVec2{maximum.x, centerY},
      ImVec2{maximum.x, maximum.y}, ImVec2{centerX, maximum.y},
      ImVec2{minimum.x, maximum.y}, ImVec2{minimum.x, centerY},
  };
  const ImU32 handleColor =
      active ? (previewValid
                    ? color({0.20F, 0.78F, 0.38F, 1.0F})
                    : color({0.92F, 0.29F, 0.24F, 1.0F}))
             : color({0.96F, 0.82F, 0.22F, 1.0F});
  for (const ImVec2 handle : handles) {
    drawList.AddRectFilled({handle.x - 4.0F, handle.y - 4.0F},
                           {handle.x + 4.0F, handle.y + 4.0F}, handleColor);
  }
}

void drawRoomManipulation(ImDrawList& drawList,
                          const CanvasTransform& transform,
                          const CreativeEditorWorldLayoutState& state) {
  if (state.selection.kind != CreativeEditorWorldLayoutSelectionKind::Room ||
      !roomOnActiveLevel(state, state.source, state.selection.index)) {
    return;
  }
  const bool active = state.roomManipulation.active &&
                      state.roomManipulation.target.roomIndex ==
                          state.selection.index;
  drawRectManipulation(
      drawList, transform,
      active ? state.roomManipulation.previewFootprint
             : state.source.rooms[state.selection.index].footprint,
      active, state.roomManipulation.previewValid);
}

void drawBoxManipulation(ImDrawList& drawList,
                         const CanvasTransform& transform,
                         const CreativeEditorWorldLayoutState& state) {
  if (state.selection.kind != CreativeEditorWorldLayoutSelectionKind::Box ||
      state.selection.index >= state.source.boxes.size()) {
    return;
  }
  const bool active = state.boxManipulation.active &&
                      state.boxManipulation.target.boxIndex ==
                          state.selection.index;
  drawRectManipulation(
      drawList, transform,
      active ? state.boxManipulation.previewFootprint
             : state.source.boxes[state.selection.index].footprint,
      active, state.boxManipulation.previewValid);
}

void drawWall(ImDrawList& drawList, const CanvasTransform& transform,
              const CreativeEditorWorldLayoutState& state,
              std::size_t wallIndex) {
  const cr::CreativeWorldLayoutWall& wall =
      creativeEditorWorldLayoutDisplaySource(state).walls[wallIndex];
  const bool isSelected =
      selected(state, CreativeEditorWorldLayoutSelectionKind::Wall, wallIndex);
  const bool active = state.wallManipulation.active &&
                      state.wallManipulation.target.wallIndex == wallIndex;
  const auto [deltaX, deltaZ] =
      buildingPreviewOffset(state, wall.buildingIndex);
  const double startX =
      (active ? state.wallManipulation.previewStart.x : wall.start.x) + deltaX;
  const double startZ =
      (active ? state.wallManipulation.previewStart.z : wall.start.z) + deltaZ;
  const double endX =
      (active ? state.wallManipulation.previewEnd.x : wall.end.x) + deltaX;
  const double endZ =
      (active ? state.wallManipulation.previewEnd.z : wall.end.z) + deltaZ;
  const ImVec2 start = toScreen(transform, startX, startZ);
  const ImVec2 end = toScreen(transform, endX, endZ);
  const ImU32 wallColor =
      active ? (state.wallManipulation.previewValid
                    ? color({0.20F, 0.78F, 0.38F, 1.0F})
                    : color({0.92F, 0.29F, 0.24F, 1.0F}))
      : isSelected ? color({0.96F, 0.82F, 0.22F, 1.0F})
                   : color({0.72F, 0.76F, 0.81F, 1.0F});
  drawList.AddLine(start, end, wallColor,
                   active || isSelected ? 7.0F : 5.0F);
  if (active || isSelected) {
    drawList.AddRectFilled({start.x - 4.0F, start.y - 4.0F},
                           {start.x + 4.0F, start.y + 4.0F}, wallColor);
    drawList.AddRectFilled({end.x - 4.0F, end.y - 4.0F},
                           {end.x + 4.0F, end.y + 4.0F}, wallColor);
  }
  if (active) {
    char dimensions[48]{};
    const double length = std::hypot(endX - startX, endZ - startZ);
    std::snprintf(dimensions, sizeof(dimensions), "%.0f long", length);
    drawList.AddText({(start.x + end.x) * 0.5F + 7.0F,
                      (start.y + end.y) * 0.5F + 7.0F},
                     wallColor, dimensions);
  }
}

void drawBuildingSelection(ImDrawList& drawList,
                           const CanvasTransform& transform,
                           const CreativeEditorWorldLayoutState& state) {
  if (state.buildingTemplatePlacement.active ||
      state.selection.kind !=
          CreativeEditorWorldLayoutSelectionKind::Building ||
      state.selection.index >= state.source.buildings.size()) {
    return;
  }
  CreativeEditorWorldLayoutBuildingBounds bounds;
  if (!readCreativeEditorWorldLayoutBuildingBounds(
          state, state.selection.index, bounds)) {
    return;
  }
  const bool moveActive = state.buildingManipulation.active &&
                          state.buildingManipulation.buildingIndex ==
                              state.selection.index;
  const bool transformActive =
      state.buildingTransform.active &&
      state.buildingTransform.buildingIndex == state.selection.index;
  const bool active = moveActive || transformActive;
  const double deltaX = moveActive ? static_cast<double>(
                                         state.buildingManipulation
                                             .previewDeltaXCells)
                                   : 0.0;
  const double deltaZ = moveActive ? static_cast<double>(
                                         state.buildingManipulation
                                             .previewDeltaZCells)
                                   : 0.0;
  const ImVec2 minimum =
      toScreen(transform, bounds.minimum.x + deltaX,
               bounds.minimum.z + deltaZ);
  const ImVec2 maximum =
      toScreen(transform, bounds.maximum.x + deltaX,
               bounds.maximum.z + deltaZ);
  const ImVec4 tint =
      active ? ((transformActive || state.buildingManipulation.previewValid)
                    ? ImVec4{0.20F, 0.78F, 0.38F, 1.0F}
                    : ImVec4{0.92F, 0.29F, 0.24F, 1.0F})
             : generatedScopeTint(cr::CreativeWorldLayoutTable::Building);
  ImVec4 fill = tint;
  fill.w = active ? 0.12F : 0.05F;
  drawList.AddRectFilled(minimum, maximum, color(fill));
  drawList.AddRect(minimum, maximum, color(tint), 0.0F, 0,
                   active ? 4.0F : 3.0F);
  drawList.AddRectFilled({minimum.x - 5.0F, minimum.y - 5.0F},
                         {minimum.x + 5.0F, minimum.y + 5.0F}, color(tint));
  const std::string& name =
      state.source.buildings[state.selection.index].name;
  drawList.AddText({minimum.x + 8.0F, minimum.y + 7.0F}, color(tint),
                   name.c_str());
}

void drawBuildingTemplatePlacement(
    ImDrawList& drawList, const CanvasTransform& transform,
    const CreativeEditorWorldLayoutState& state) {
  const auto& placement = state.buildingTemplatePlacement;
  if (!placement.active || !placement.previewBounds.valid) {
    return;
  }
  const ImVec2 minimum =
      toScreen(transform, placement.previewBounds.minimum.x,
               placement.previewBounds.minimum.z);
  const ImVec2 maximum =
      toScreen(transform, placement.previewBounds.maximum.x,
               placement.previewBounds.maximum.z);
  const ImVec4 tint = placement.previewValid
                          ? ImVec4{0.20F, 0.78F, 0.38F, 1.0F}
                          : ImVec4{0.92F, 0.29F, 0.24F, 1.0F};
  ImVec4 fill = tint;
  fill.w = 0.10F;
  drawList.AddRectFilled(minimum, maximum, color(fill));
  drawList.AddRect(minimum, maximum, color(tint), 0.0F, 0, 4.0F);
  drawList.AddRectFilled({minimum.x - 5.0F, minimum.y - 5.0F},
                         {minimum.x + 5.0F, minimum.y + 5.0F}, color(tint));
  const std::string& label = placement.orientedTemplate.label;
  drawList.AddText({minimum.x + 8.0F, minimum.y + 7.0F}, color(tint),
                   label.c_str());
}

CreativeEditorWorldLayoutPoint openingPoint(
    CreativeEditorWorldLayoutOpeningHost host, double offsetCells) {
  const double t = offsetCells / host.lengthCells;
  return {host.start.x + (host.end.x - host.start.x) * t,
          host.start.z + (host.end.z - host.start.z) * t};
}

void drawOpenings(ImDrawList& drawList, const CanvasTransform& transform,
                  const CreativeEditorWorldLayoutState& state) {
  const cr::CreativeWorldLayout& source =
      creativeEditorWorldLayoutDisplaySource(state);
  for (std::size_t index = 0U; index < source.openings.size(); ++index) {
    if (!openingOnActiveLevel(state, source, index)) {
      continue;
    }
    const cr::CreativeWorldLayoutOpening& opening =
        source.openings[index];
    const CreativeEditorWorldLayoutOpeningHost host =
        resolveCreativeEditorWorldLayoutOpeningHost(state, index);
    if (!host.valid) {
      continue;
    }
    const bool isDoor = opening.kind == cr::CreativeBuildingOpeningKind::Door;
    const bool isSelected =
        selected(state, CreativeEditorWorldLayoutSelectionKind::Opening, index);
    const bool active = state.openingManipulation.active &&
                        state.openingManipulation.target.openingIndex == index;
    const bool hostWallActive =
        state.wallManipulation.active &&
        opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall &&
        opening.wallIndex == state.wallManipulation.target.wallIndex;
    const double centerOffset = active
                                    ? state.openingManipulation
                                          .previewCenterOffsetCells
                                    : hostWallActive
                                          ? opening.centerOffsetCells +
                                                state.wallManipulation
                                                    .previewOpeningOffsetDeltaCells
                                          : opening.centerOffsetCells;
    const double width = active ? state.openingManipulation.previewWidthCells
                                : opening.widthCells;
    const CreativeEditorWorldLayoutPoint centerPoint =
        openingPoint(host, centerOffset);
    const CreativeEditorWorldLayoutPoint startPoint =
        openingPoint(host, centerOffset - width * 0.5);
    const CreativeEditorWorldLayoutPoint endPoint =
        openingPoint(host, centerOffset + width * 0.5);
    const ImVec2 center =
        toScreen(transform, centerPoint.x, centerPoint.z);
    const ImVec2 start = toScreen(transform, startPoint.x, startPoint.z);
    const ImVec2 end = toScreen(transform, endPoint.x, endPoint.z);
    const bool previewing = active || hostWallActive;
    const bool previewValid =
        active ? state.openingManipulation.previewValid
               : state.wallManipulation.previewValid;
    const ImU32 markerColor =
        previewing ? (previewValid
                          ? color({0.20F, 0.78F, 0.38F, 1.0F})
                          : color({0.92F, 0.29F, 0.24F, 1.0F}))
        : isSelected  ? color({0.96F, 0.82F, 0.22F, 1.0F})
        : isDoor      ? color({0.31F, 0.82F, 0.43F, 1.0F})
                      : color({0.27F, 0.72F, 0.91F, 1.0F});
    drawList.AddLine(start, end, markerColor,
                     previewing || isSelected ? 7.0F : 5.0F);
    if (isDoor) {
      drawList.AddCircleFilled(center, previewing || isSelected ? 7.0F : 5.0F,
                               markerColor);
    } else {
      const float half = previewing || isSelected ? 7.0F : 5.0F;
      drawList.AddRectFilled({center.x - half, center.y - half},
                             {center.x + half, center.y + half}, markerColor);
    }
    if (active || isSelected) {
      drawList.AddRectFilled({start.x - 4.0F, start.y - 4.0F},
                             {start.x + 4.0F, start.y + 4.0F}, markerColor);
      drawList.AddRectFilled({end.x - 4.0F, end.y - 4.0F},
                             {end.x + 4.0F, end.y + 4.0F}, markerColor);
    }
    if (active) {
      char dimensions[48]{};
      std::snprintf(dimensions, sizeof(dimensions), "%.2f wide", width);
      drawList.AddText({center.x + 8.0F, center.y + 8.0F}, markerColor,
                       dimensions);
    }
  }
}

void drawOpeningPlacementPlan(
    ImDrawList& drawList, const CanvasTransform& transform,
    CreativeEditorWorldLayoutPoint hovered,
    const CreativeEditorWorldLayoutOpeningPlacementPlan& plan,
    cr::CreativeBuildingOpeningKind kind, std::string_view label) {
  const ImU32 previewColor =
      !plan.accepted
          ? color({0.92F, 0.29F, 0.24F, 1.0F})
          : kind == cr::CreativeBuildingOpeningKind::Door
                ? color({0.20F, 0.82F, 0.38F, 1.0F})
                : color({0.27F, 0.72F, 0.91F, 1.0F});
  if (!plan.accepted) {
    const ImVec2 center = toScreen(transform, hovered.x, hovered.z);
    drawList.AddLine({center.x - 7.0F, center.y - 7.0F},
                     {center.x + 7.0F, center.y + 7.0F}, previewColor, 3.0F);
    drawList.AddLine({center.x - 7.0F, center.y + 7.0F},
                     {center.x + 7.0F, center.y - 7.0F}, previewColor, 3.0F);
    drawList.AddText({center.x + 11.0F, center.y + 9.0F}, previewColor,
                     plan.message.data());
    return;
  }

  const ImVec2 start =
      toScreen(transform, plan.startPoint.x, plan.startPoint.z);
  const ImVec2 center =
      toScreen(transform, plan.centerPoint.x, plan.centerPoint.z);
  const ImVec2 end = toScreen(transform, plan.endPoint.x, plan.endPoint.z);
  if (plan.pointerDistanceCells > 0.01) {
    const ImVec2 pointer = toScreen(transform, hovered.x, hovered.z);
    drawList.AddLine(pointer, center, previewColor, 1.0F);
  }
  drawList.AddLine(start, end, previewColor, 8.0F);
  drawList.AddRectFilled({start.x - 3.5F, start.y - 3.5F},
                         {start.x + 3.5F, start.y + 3.5F}, previewColor);
  drawList.AddRectFilled({end.x - 3.5F, end.y - 3.5F},
                         {end.x + 3.5F, end.y + 3.5F}, previewColor);
  if (kind == cr::CreativeBuildingOpeningKind::Door) {
    drawList.AddCircleFilled(center, 6.0F, previewColor);
  } else {
    drawList.AddRectFilled({center.x - 6.0F, center.y - 6.0F},
                           {center.x + 6.0F, center.y + 6.0F}, previewColor);
  }
  char placementLabel[96]{};
  const std::size_t visibleLabelSize =
      std::min(label.size(), std::size_t{48U});
  std::snprintf(placementLabel, sizeof(placementLabel),
                "%.*s | %.2f wide x %.2f high",
                static_cast<int>(visibleLabelSize), label.data(),
                plan.opening.widthCells, plan.opening.cutoutHeightCells);
  drawList.AddText({center.x + 9.0F, center.y + 9.0F}, previewColor,
                   placementLabel);
}

void drawOpeningPlacementPreview(
    ImDrawList& drawList, const CanvasTransform& transform,
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint hovered) {
  cr::CreativeBuildingOpeningKind kind;
  if (state.tool == CreativeEditorWorldLayoutTool::Door) {
    kind = cr::CreativeBuildingOpeningKind::Door;
  } else if (state.tool == CreativeEditorWorldLayoutTool::Window) {
    kind = cr::CreativeBuildingOpeningKind::Window;
  } else {
    return;
  }
  const CreativeEditorWorldLayoutOpeningPlacementPlan plan =
      planCreativeEditorWorldLayoutOpeningPlacement(state, hovered, kind);
  drawOpeningPlacementPlan(
      drawList, transform, hovered, plan, kind,
      kind == cr::CreativeBuildingOpeningKind::Door ? "Door" : "Window");
}

void drawAnchorPreview(ImDrawList& drawList, const CanvasTransform& transform,
                       const CreativeEditorWorldLayoutState& state,
                       CreativeEditorWorldLayoutPoint hovered) {
  if (!state.anchorActive) {
    return;
  }
  const double snappedX = std::round(hovered.x);
  const double snappedZ = std::round(hovered.z);
  const ImU32 previewColor = color({0.96F, 0.82F, 0.22F, 0.95F});
  const ImVec2 start = toScreen(transform, state.anchor.x, state.anchor.z);
  if (state.tool == CreativeEditorWorldLayoutTool::BuildingShell ||
      state.tool == CreativeEditorWorldLayoutTool::Room ||
      state.tool == CreativeEditorWorldLayoutTool::Floor ||
      creativeEditorWorldLayoutToolIsVerticalConnector(state.tool) ||
      state.tool == CreativeEditorWorldLayoutTool::Bridge) {
    const ImVec2 end = toScreen(transform, snappedX, snappedZ);
    if (state.tool == CreativeEditorWorldLayoutTool::BuildingShell ||
        state.tool == CreativeEditorWorldLayoutTool::Room ||
        creativeEditorWorldLayoutToolIsVerticalConnector(state.tool) ||
        state.tool == CreativeEditorWorldLayoutTool::Bridge) {
      drawList.AddRectFilled(
          {std::min(start.x, end.x), std::min(start.y, end.y)},
          {std::max(start.x, end.x), std::max(start.y, end.y)},
          state.tool == CreativeEditorWorldLayoutTool::Ramp
              ? color({0.72F, 0.38F, 0.12F, 0.28F})
              : creativeEditorWorldLayoutToolIsVerticalConnector(state.tool)
                    ? color({0.18F, 0.55F, 0.72F, 0.28F})
                    : color({0.22F, 0.58F, 0.38F, 0.22F}));
    }
    drawList.AddRect({std::min(start.x, end.x), std::min(start.y, end.y)},
                     {std::max(start.x, end.x), std::max(start.y, end.y)},
                     previewColor, 0.0F, 0, 2.0F);
    const int width = static_cast<int>(std::fabs(snappedX - state.anchor.x));
    const int depth = static_cast<int>(std::fabs(snappedZ - state.anchor.z));
    const std::string dimensions =
        std::to_string(width) + " x " + std::to_string(depth);
    drawList.AddText({std::min(start.x, end.x) + 6.0F,
                      std::min(start.y, end.y) + 6.0F},
                     previewColor, dimensions.c_str());
  } else if (state.tool == CreativeEditorWorldLayoutTool::Wall) {
    const double deltaX = std::fabs(snappedX - state.anchor.x);
    const double deltaZ = std::fabs(snappedZ - state.anchor.z);
    const ImVec2 end = deltaX >= deltaZ
                           ? toScreen(transform, snappedX, state.anchor.z)
                           : toScreen(transform, state.anchor.x, snappedZ);
    drawList.AddLine(start, end, previewColor, 4.0F);
  } else if (state.tool == CreativeEditorWorldLayoutTool::Road ||
             state.tool == CreativeEditorWorldLayoutTool::Ditch) {
    const ImVec2 end = toScreen(transform, snappedX, snappedZ);
    const float width = state.tool == CreativeEditorWorldLayoutTool::Ditch
                            ? transform.pixelsPerCell * 3.0F
                            : transform.pixelsPerCell * 3.0F;
    drawList.AddLine(start, end,
                     state.tool == CreativeEditorWorldLayoutTool::Ditch
                         ? color({0.25F, 0.47F, 0.68F, 0.32F})
                         : color({0.72F, 0.56F, 0.28F, 0.32F}),
                     width);
    drawList.AddLine(start, end, previewColor, 2.0F);
  }
}

void drawCatalogPlacementPreview(
    ImDrawList& drawList, const CanvasTransform& transform,
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint hovered,
    cr::CreativeGridSettings grid) {
  if (state.tool != CreativeEditorWorldLayoutTool::CatalogAsset ||
      !state.catalogPlacement.active) {
    return;
  }
  const CreativeEditorWorldLayoutCatalogPlacementPlan plan =
      planCreativeEditorWorldLayoutCatalogPlacement(state, hovered, grid);
  if (plan.hostedOpening) {
    drawOpeningPlacementPlan(
        drawList, transform, hovered, plan.openingPlacement,
        state.catalogPlacement.categoryId == "window"
            ? cr::CreativeBuildingOpeningKind::Window
            : cr::CreativeBuildingOpeningKind::Door,
        state.catalogPlacement.label);
    return;
  }
  const ImU32 outline = plan.accepted
                            ? color({0.20F, 0.82F, 0.38F, 1.0F})
                            : color({0.92F, 0.29F, 0.24F, 1.0F});
  if (!plan.accepted || !plan.footprint.valid) {
    const ImVec2 center = toScreen(transform, hovered.x, hovered.z);
    drawList.AddLine({center.x - 8.0F, center.y - 8.0F},
                     {center.x + 8.0F, center.y + 8.0F}, outline, 3.0F);
    drawList.AddLine({center.x - 8.0F, center.y + 8.0F},
                     {center.x + 8.0F, center.y - 8.0F}, outline, 3.0F);
    drawList.AddText({center.x + 12.0F, center.y + 10.0F}, outline,
                     plan.message.c_str());
    return;
  }
  std::array<ImVec2, 4U> points;
  for (std::size_t index = 0U; index < points.size(); ++index) {
    points[index] = toScreen(transform, plan.footprint.corners[index].x,
                             plan.footprint.corners[index].z);
  }
  drawList.AddConvexPolyFilled(points.data(), static_cast<int>(points.size()),
                               color({0.20F, 0.82F, 0.38F, 0.22F}));
  drawList.AddPolyline(points.data(), static_cast<int>(points.size()), outline,
                       ImDrawFlags_Closed, 2.5F);
  const ImVec2 pivot = toScreen(transform, plan.object.pointCells.x,
                                plan.object.pointCells.z);
  if (plan.snapMode == CreativeEditorWorldLayoutCatalogSnapMode::Wall) {
    const CreativeEditorWorldLayoutPoint centerline{
        plan.snapSurfacePoint.x -
            plan.snapNormal.x * plan.snapWallThicknessCells * 0.5,
        plan.snapSurfacePoint.z -
            plan.snapNormal.z * plan.snapWallThicknessCells * 0.5};
    const CreativeEditorWorldLayoutPoint tangent{-plan.snapNormal.z,
                                                  plan.snapNormal.x};
    const CreativeEditorWorldLayoutPoint faceStart{
        plan.snapSurfacePoint.x - tangent.x * 0.45,
        plan.snapSurfacePoint.z - tangent.z * 0.45};
    const CreativeEditorWorldLayoutPoint faceEnd{
        plan.snapSurfacePoint.x + tangent.x * 0.45,
        plan.snapSurfacePoint.z + tangent.z * 0.45};
    const CreativeEditorWorldLayoutPoint normalTip{
        plan.snapSurfacePoint.x + plan.snapNormal.x * 0.65,
        plan.snapSurfacePoint.z + plan.snapNormal.z * 0.65};
    const ImVec2 centerlineScreen =
        toScreen(transform, centerline.x, centerline.z);
    const ImVec2 surfaceScreen = toScreen(
        transform, plan.snapSurfacePoint.x, plan.snapSurfacePoint.z);
    const ImVec2 normalTipScreen =
        toScreen(transform, normalTip.x, normalTip.z);
    const ImU32 faceColor = color({0.98F, 0.78F, 0.20F, 1.0F});
    if (plan.snapDistanceCells > 0.01) {
      const ImVec2 pointer = toScreen(transform, hovered.x, hovered.z);
      drawList.AddLine(pointer, centerlineScreen, outline, 1.0F);
    }
    drawList.AddLine(centerlineScreen, surfaceScreen, faceColor, 2.0F);
    drawList.AddLine(toScreen(transform, faceStart.x, faceStart.z),
                     toScreen(transform, faceEnd.x, faceEnd.z), faceColor,
                     3.0F);
    drawList.AddCircleFilled(surfaceScreen, 3.5F, faceColor);
    drawList.AddLine(surfaceScreen, normalTipScreen, outline, 2.0F);
    const float arrowDx = normalTipScreen.x - surfaceScreen.x;
    const float arrowDy = normalTipScreen.y - surfaceScreen.y;
    const float arrowLength = std::hypot(arrowDx, arrowDy);
    if (arrowLength > 0.0F) {
      const float unitX = arrowDx / arrowLength;
      const float unitY = arrowDy / arrowLength;
      const ImVec2 arrowBase{normalTipScreen.x - unitX * 7.0F,
                             normalTipScreen.y - unitY * 7.0F};
      const ImVec2 arrowSide{-unitY * 3.5F, unitX * 3.5F};
      drawList.AddTriangleFilled(
          normalTipScreen,
          {arrowBase.x + arrowSide.x, arrowBase.y + arrowSide.y},
          {arrowBase.x - arrowSide.x, arrowBase.y - arrowSide.y}, outline);
    }
  }
  drawList.AddCircleFilled(pivot, 3.5F, outline);
  const std::string previewLabel =
      state.catalogPlacement.label + " | " + plan.message;
  drawList.AddText({points.front().x + 6.0F, points.front().y + 6.0F}, outline,
                   previewLabel.c_str());
}


}  // namespace

CreativeEditorWorldLayoutCanvasPointerGeometry
drawCreativeEditorWorldLayoutCanvasScene(
    ImDrawList& drawList, ImVec2 minimum, ImVec2 maximum,
    const CreativeEditorWorldLayoutCanvasTransform& transform,
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutTopographyState& topography,
    const cr::CreativeGridSettings& grid, ImVec2 pointerPosition,
    bool hovered) {
  drawList.PushClipRect(minimum, maximum, true);
  drawList.AddRectFilled(minimum, maximum,
                         color({0.105F, 0.12F, 0.135F, 1.0F}));
  const CanvasWorldBounds visibleBounds =
      canvasWorldBounds(transform, minimum, maximum);
  drawTopographyBands(drawList, transform, visibleBounds, topography);
  drawGrid(drawList, minimum, maximum, transform);
  drawTopographyContours(drawList, transform, visibleBounds, topography);
  drawTerrainRegionSelection(drawList, transform, topography.region);
  const cr::CreativeWorldLayout& displaySource =
      creativeEditorWorldLayoutDisplaySource(state);
  drawTerrainSymbols(drawList, transform, state);
  for (std::size_t index = 0U; index < displaySource.rooms.size(); ++index) {
    if (roomOnActiveLevel(state, displaySource, index)) {
      drawRoom(drawList, transform, state, index);
    }
  }
  drawActiveLevelRoof(drawList, transform, state);
  for (std::size_t index = 0U; index < displaySource.boxes.size(); ++index) {
    drawFloor(drawList, transform, state, index);
  }
  for (std::size_t index = 0U; index < displaySource.verticalConnectors.size();
       ++index) {
    drawVerticalConnector(drawList, transform, state, index);
  }
  for (std::size_t index = 0U; index < displaySource.walls.size(); ++index) {
    drawWall(drawList, transform, state, index);
  }
  drawSharedRoomEdges(drawList, transform, state);
  drawOpenings(drawList, transform, state);
  drawObjectSymbols(drawList, transform, state, grid);
  drawLevelSelection(drawList, transform, state);
  drawBuildingSelection(drawList, transform, state);
  drawBuildingTemplatePlacement(drawList, transform, state);
  drawRoomManipulation(drawList, transform, state);
  drawBoxManipulation(drawList, transform, state);

  CreativeEditorWorldLayoutCanvasPointerGeometry geometry;
  geometry.pointerPoint = toWorld(transform, pointerPosition);
  const ImVec2 boundedPointer{
      std::clamp(pointerPosition.x, minimum.x, maximum.x),
      std::clamp(pointerPosition.y, minimum.y, maximum.y)};
  geometry.hoveredPoint = toWorld(transform, boundedPointer);
  geometry.handleToleranceCells = std::clamp(
      8.0 / static_cast<double>(transform.pixelsPerCell), 0.10, 0.45);
  if (hovered && !state.buildingTemplatePlacement.active) {
    drawOpeningPlacementPreview(drawList, transform, state,
                                geometry.hoveredPoint);
    drawCatalogPlacementPreview(drawList, transform, state,
                                geometry.hoveredPoint, grid);
    drawAnchorPreview(drawList, transform, state, geometry.hoveredPoint);
  }
  drawList.PopClipRect();

  if (hovered) {
    drawTopographyHoverFacts(topography, geometry.pointerPoint, grid);
  }
  return geometry;
}

}  // namespace iggy3d_creative_app
