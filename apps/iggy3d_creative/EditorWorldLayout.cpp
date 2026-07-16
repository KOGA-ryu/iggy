#include "EditorWorldLayout.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string_view>
#include <utility>

namespace iggy3d_creative_app {
namespace {

constexpr double kOpeningHitToleranceCells = 0.75;
constexpr double kSelectionHitToleranceCells = 0.35;

bool finitePoint(CreativeEditorWorldLayoutPoint point) noexcept {
  return std::isfinite(point.x) && std::isfinite(point.z);
}

bool toGridCoord(CreativeEditorWorldLayoutPoint point,
                 cr::CreativeTerrainCoord2& output) noexcept {
  if (!finitePoint(point)) {
    return false;
  }
  const double roundedX = std::round(point.x);
  const double roundedZ = std::round(point.z);
  if (roundedX < std::numeric_limits<std::int32_t>::min() ||
      roundedX > std::numeric_limits<std::int32_t>::max() ||
      roundedZ < std::numeric_limits<std::int32_t>::min() ||
      roundedZ > std::numeric_limits<std::int32_t>::max()) {
    return false;
  }
  output.x = static_cast<std::int32_t>(roundedX);
  output.z = static_cast<std::int32_t>(roundedZ);
  return true;
}

void invalidatePreview(CreativeEditorWorldLayoutState& state) {
  state.previewVisible = false;
  state.previewLayoutRevision = 0U;
  state.preview = {};
}

void noteSourceChange(CreativeEditorWorldLayoutState& state,
                      std::string reason) {
  if (state.revision != std::numeric_limits<std::uint64_t>::max()) {
    ++state.revision;
  }
  invalidatePreview(state);
  state.statusMessage = std::move(reason);
}

bool keyExists(const cr::CreativeWorldLayout& layout, std::string_view key) {
  const auto matches = [&](const auto& value) {
    return value.stableKey == key;
  };
  return std::any_of(layout.buildings.begin(), layout.buildings.end(),
                     matches) ||
         std::any_of(layout.boxes.begin(), layout.boxes.end(), matches) ||
         std::any_of(layout.walls.begin(), layout.walls.end(), matches) ||
         std::any_of(layout.openings.begin(), layout.openings.end(), matches) ||
         std::any_of(layout.terrainProfiles.begin(),
                     layout.terrainProfiles.end(), matches) ||
         std::any_of(layout.terrainPaths.begin(), layout.terrainPaths.end(),
                     matches);
}

std::string mintKey(CreativeEditorWorldLayoutState& state,
                    std::string_view prefix) {
  for (;;) {
    const std::string candidate =
        std::string(prefix) + "_" + std::to_string(state.nextStableOrdinal++);
    if (!keyExists(state.source, candidate)) {
      return candidate;
    }
  }
}

std::size_t ensurePrimaryBuilding(CreativeEditorWorldLayoutState& state) {
  if (!state.source.buildings.empty()) {
    return 0U;
  }
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = mintKey(state, "building");
  building.name = "Building 1";
  building.rootMode = cr::CreativeBuildingRootMode::None;
  state.source.buildings.push_back(std::move(building));
  return 0U;
}

cr::CreativeWorldLayoutRect normalizedRect(cr::CreativeTerrainCoord2 first,
                                           cr::CreativeTerrainCoord2 second) {
  return {{std::min(first.x, second.x), std::min(first.z, second.z)},
          {std::max(first.x, second.x), std::max(first.z, second.z)}};
}

cr::CreativeTerrainCoord2 cardinalEnd(cr::CreativeTerrainCoord2 start,
                                      cr::CreativeTerrainCoord2 requested) {
  const std::int64_t deltaX = static_cast<std::int64_t>(requested.x) - start.x;
  const std::int64_t deltaZ = static_cast<std::int64_t>(requested.z) - start.z;
  if (std::llabs(deltaX) >= std::llabs(deltaZ)) {
    requested.z = start.z;
  } else {
    requested.x = start.x;
  }
  return requested;
}

struct WallProjection {
  bool hit = false;
  std::size_t wallIndex = cr::kInvalidCreativeWorldLayoutIndex;
  double centerOffsetCells = 0.0;
  double distanceCells = std::numeric_limits<double>::infinity();
  CreativeEditorWorldLayoutPoint projected{};
};

WallProjection nearestWall(const cr::CreativeWorldLayout& layout,
                           CreativeEditorWorldLayoutPoint point,
                           double tolerance) {
  WallProjection best;
  for (std::size_t index = 0U; index < layout.walls.size(); ++index) {
    const cr::CreativeWorldLayoutWall& wall = layout.walls[index];
    const double dx = static_cast<double>(wall.end.x) - wall.start.x;
    const double dz = static_cast<double>(wall.end.z) - wall.start.z;
    const double lengthSquared = dx * dx + dz * dz;
    if (lengthSquared <= 0.0) {
      continue;
    }
    const double relativeX = point.x - wall.start.x;
    const double relativeZ = point.z - wall.start.z;
    const double t =
        std::clamp((relativeX * dx + relativeZ * dz) / lengthSquared, 0.0, 1.0);
    const double projectedX = wall.start.x + t * dx;
    const double projectedZ = wall.start.z + t * dz;
    const double distance =
        std::hypot(point.x - projectedX, point.z - projectedZ);
    if (distance < best.distanceCells) {
      best.hit = distance <= tolerance;
      best.wallIndex = index;
      best.centerOffsetCells = t * std::sqrt(lengthSquared);
      best.distanceCells = distance;
      best.projected = {projectedX, projectedZ};
    }
  }
  return best;
}

CreativeEditorWorldLayoutSelection hitTest(
    const cr::CreativeWorldLayout& layout,
    CreativeEditorWorldLayoutPoint point) {
  const WallProjection openingWall =
      nearestWall(layout, point, kSelectionHitToleranceCells);
  if (openingWall.hit &&
      openingWall.wallIndex != cr::kInvalidCreativeWorldLayoutIndex) {
    for (std::size_t index = layout.openings.size(); index > 0U; --index) {
      const cr::CreativeWorldLayoutOpening& opening =
          layout.openings[index - 1U];
      if (opening.wallIndex != openingWall.wallIndex) {
        continue;
      }
      const double halfWidth = std::max(0.25, opening.widthCells * 0.5);
      if (std::fabs(opening.centerOffsetCells -
                    openingWall.centerOffsetCells) <= halfWidth) {
        return {CreativeEditorWorldLayoutSelectionKind::Opening, index - 1U};
      }
    }
    if (openingWall.hit) {
      return {CreativeEditorWorldLayoutSelectionKind::Wall,
              openingWall.wallIndex};
    }
  }
  for (std::size_t index = layout.boxes.size(); index > 0U; --index) {
    const cr::CreativeWorldLayoutRect& rect =
        layout.boxes[index - 1U].footprint;
    if (point.x >= rect.minimum.x && point.x <= rect.maximum.x &&
        point.z >= rect.minimum.z && point.z <= rect.maximum.z) {
      return {CreativeEditorWorldLayoutSelectionKind::Box, index - 1U};
    }
  }
  return {};
}

CreativeEditorWorldLayoutEditReceipt selectAt(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point) {
  const CreativeEditorWorldLayoutSelection selected =
      hitTest(state.source, point);
  const bool changed = selected.kind != state.selection.kind ||
                       selected.index != state.selection.index;
  state.selection = selected;
  state.anchorActive = false;
  state.statusMessage =
      selected.kind == CreativeEditorWorldLayoutSelectionKind::None
          ? "selection cleared"
          : "layout symbol selected";
  return {true, changed, "creative_editor_world_layout_selected"};
}

CreativeEditorWorldLayoutEditReceipt addFloorPoint(
    CreativeEditorWorldLayoutState& state, cr::CreativeTerrainCoord2 point) {
  if (!state.anchorActive) {
    state.anchorActive = true;
    state.anchor = point;
    state.statusMessage = "floor start set; choose opposite corner";
    return {true, false, "creative_editor_world_layout_anchor_set"};
  }
  const cr::CreativeWorldLayoutRect rect = normalizedRect(state.anchor, point);
  state.anchorActive = false;
  if (rect.minimum.x == rect.maximum.x || rect.minimum.z == rect.maximum.z) {
    state.statusMessage = "floor needs width and depth";
    return {false, false, "creative_editor_world_layout_floor_degenerate"};
  }
  cr::CreativeWorldLayoutBox box;
  box.buildingIndex = ensurePrimaryBuilding(state);
  box.kind = cr::CreativeObjectKind::Floor;
  box.stableKey = mintKey(state, "floor");
  box.name = "Floor " + std::to_string(state.source.boxes.size() + 1U);
  box.footprint = rect;
  box.baseLayer = 0;
  box.heightCells = 1U;
  state.source.boxes.push_back(std::move(box));
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Box,
                     state.source.boxes.size() - 1U};
  noteSourceChange(state, "floor added");
  return {true, true, "creative_editor_world_layout_floor_added"};
}

CreativeEditorWorldLayoutEditReceipt addWallPoint(
    CreativeEditorWorldLayoutState& state, cr::CreativeTerrainCoord2 point) {
  if (!state.anchorActive) {
    state.anchorActive = true;
    state.anchor = point;
    state.statusMessage = "wall start set; choose end";
    return {true, false, "creative_editor_world_layout_anchor_set"};
  }
  point = cardinalEnd(state.anchor, point);
  state.anchorActive = false;
  if (point == state.anchor) {
    state.statusMessage = "wall needs length";
    return {false, false, "creative_editor_world_layout_wall_degenerate"};
  }
  cr::CreativeWorldLayoutWall wall;
  wall.buildingIndex = ensurePrimaryBuilding(state);
  wall.stableKey = mintKey(state, "wall");
  wall.name = "Wall " + std::to_string(state.source.walls.size() + 1U);
  wall.start = state.anchor;
  wall.end = point;
  wall.baseLayer = 0;
  wall.heightCells = 3U;
  wall.thicknessCells = 0.25;
  state.source.walls.push_back(std::move(wall));
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Wall,
                     state.source.walls.size() - 1U};
  noteSourceChange(state, "wall added");
  return {true, true, "creative_editor_world_layout_wall_added"};
}

CreativeEditorWorldLayoutEditReceipt addOpening(
    CreativeEditorWorldLayoutState& state, CreativeEditorWorldLayoutPoint point,
    cr::CreativeBuildingOpeningKind kind) {
  const WallProjection projection =
      nearestWall(state.source, point, kOpeningHitToleranceCells);
  if (!projection.hit) {
    state.statusMessage = "place the opening on an existing wall";
    return {false, false, "creative_editor_world_layout_wall_not_found"};
  }
  cr::CreativeWorldLayoutOpening opening;
  opening.wallIndex = projection.wallIndex;
  opening.kind = kind;
  opening.pose = cr::CreativeBuildingOpeningPose::Closed;
  opening.includeInsert = true;
  if (kind == cr::CreativeBuildingOpeningKind::Door) {
    opening.widthCells = 1.0;
    opening.cutoutBottomCells = 0.0;
    opening.cutoutHeightCells = 2.1;
    opening.insertBottomCells = 0.0;
    opening.insertHeightCells = 2.1;
    opening.insertWidthCells = 1.0;
    opening.insertThicknessCells = 0.15;
  } else {
    opening.widthCells = 1.5;
    opening.cutoutBottomCells = 1.0;
    opening.cutoutHeightCells = 1.2;
    opening.insertBottomCells = 1.0;
    opening.insertHeightCells = 1.2;
    opening.insertWidthCells = 1.5;
    opening.insertThicknessCells = 0.10;
  }
  const cr::CreativeWorldLayoutWall& wall =
      state.source.walls[projection.wallIndex];
  const double wallLength =
      std::hypot(static_cast<double>(wall.end.x) - wall.start.x,
                 static_cast<double>(wall.end.z) - wall.start.z);
  if (wallLength < opening.widthCells) {
    state.statusMessage = "wall is too short for this opening";
    return {false, false, "creative_editor_world_layout_wall_too_short"};
  }
  const double halfWidth = opening.widthCells * 0.5;
  opening.centerOffsetCells =
      std::clamp(std::round(projection.centerOffsetCells * 4.0) / 4.0,
                 halfWidth, wallLength - halfWidth);
  for (const cr::CreativeWorldLayoutOpening& existing : state.source.openings) {
    if (existing.wallIndex != opening.wallIndex) {
      continue;
    }
    const double minimumSeparation =
        (existing.widthCells + opening.widthCells) * 0.5;
    if (std::fabs(existing.centerOffsetCells - opening.centerOffsetCells) <
        minimumSeparation) {
      state.statusMessage = "opening overlaps an existing opening";
      return {false, false, "creative_editor_world_layout_opening_overlap"};
    }
  }
  opening.stableKey = mintKey(
      state, kind == cr::CreativeBuildingOpeningKind::Door ? "door" : "window");
  opening.name =
      (kind == cr::CreativeBuildingOpeningKind::Door ? "Door " : "Window ") +
      std::to_string(state.source.openings.size() + 1U);
  state.source.openings.push_back(std::move(opening));
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Opening,
                     state.source.openings.size() - 1U};
  noteSourceChange(state, kind == cr::CreativeBuildingOpeningKind::Door
                              ? "door added"
                              : "window added");
  return {true, true, "creative_editor_world_layout_opening_added"};
}

}  // namespace

const char* creativeEditorWorldLayoutToolLabel(
    CreativeEditorWorldLayoutTool tool) noexcept {
  switch (tool) {
    case CreativeEditorWorldLayoutTool::Select:
      return "Select";
    case CreativeEditorWorldLayoutTool::Floor:
      return "Floor";
    case CreativeEditorWorldLayoutTool::Wall:
      return "Wall";
    case CreativeEditorWorldLayoutTool::Door:
      return "Door";
    case CreativeEditorWorldLayoutTool::Window:
      return "Window";
    case CreativeEditorWorldLayoutTool::Count:
      break;
  }
  return "Unknown";
}

void resetCreativeEditorWorldLayout(CreativeEditorWorldLayoutState& state,
                                    std::string layoutKey) {
  state = {};
  state.source.stableKey =
      layoutKey.empty() ? "world_layout" : std::move(layoutKey);
  state.statusMessage = "blank layout";
}

void installCreativeEditorWorldLayout(CreativeEditorWorldLayoutState& state,
                                      cr::CreativeWorldLayout layout) {
  state = {};
  state.source = std::move(layout);
  state.nextStableOrdinal =
      1U + state.source.buildings.size() + state.source.boxes.size() +
      state.source.walls.size() + state.source.openings.size() +
      state.source.terrainProfiles.size() + state.source.terrainPaths.size();
  state.statusMessage = "layout loaded";
}

void markCreativeEditorWorldLayoutSaved(
    CreativeEditorWorldLayoutState& state) noexcept {
  state.savedRevision = state.revision;
}

bool creativeEditorWorldLayoutDirty(
    const CreativeEditorWorldLayoutState& state) noexcept {
  return state.revision != state.savedRevision;
}

bool creativeEditorWorldLayoutPreviewActive(
    const CreativeEditorWorldLayoutState& state) noexcept {
  return state.previewVisible && state.preview.accepted &&
         state.preview.document.isValid() &&
         state.previewLayoutRevision == state.revision;
}

const cr::CreativeDocument& creativeEditorWorldLayoutRenderDocument(
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& liveDocument) noexcept {
  return creativeEditorWorldLayoutPreviewActive(state) ? state.preview.document
                                                       : liveDocument;
}

CreativeEditorWorldLayoutEditReceipt setCreativeEditorWorldLayoutTool(
    CreativeEditorWorldLayoutState& state, CreativeEditorWorldLayoutTool tool) {
  if (tool >= CreativeEditorWorldLayoutTool::Count) {
    return {false, false, "creative_editor_world_layout_tool_invalid"};
  }
  const bool changed = state.tool != tool || state.anchorActive;
  state.tool = tool;
  state.anchorActive = false;
  state.statusMessage =
      std::string(creativeEditorWorldLayoutToolLabel(tool)) + " tool";
  return {true, changed, "creative_editor_world_layout_tool_set"};
}

CreativeEditorWorldLayoutEditReceipt applyCreativeEditorWorldLayoutPoint(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point) {
  if (!finitePoint(point)) {
    return {false, false, "creative_editor_world_layout_point_non_finite"};
  }
  if (state.tool == CreativeEditorWorldLayoutTool::Select) {
    return selectAt(state, point);
  }
  if (state.tool == CreativeEditorWorldLayoutTool::Door) {
    return addOpening(state, point, cr::CreativeBuildingOpeningKind::Door);
  }
  if (state.tool == CreativeEditorWorldLayoutTool::Window) {
    return addOpening(state, point, cr::CreativeBuildingOpeningKind::Window);
  }
  cr::CreativeTerrainCoord2 gridPoint;
  if (!toGridCoord(point, gridPoint)) {
    return {false, false, "creative_editor_world_layout_point_out_of_range"};
  }
  if (state.tool == CreativeEditorWorldLayoutTool::Floor) {
    return addFloorPoint(state, gridPoint);
  }
  if (state.tool == CreativeEditorWorldLayoutTool::Wall) {
    return addWallPoint(state, gridPoint);
  }
  return {false, false, "creative_editor_world_layout_tool_invalid"};
}

CreativeEditorWorldLayoutEditReceipt deleteCreativeEditorWorldLayoutSelection(
    CreativeEditorWorldLayoutState& state) {
  const CreativeEditorWorldLayoutSelection selected = state.selection;
  if (selected.kind == CreativeEditorWorldLayoutSelectionKind::Box &&
      selected.index < state.source.boxes.size()) {
    state.source.boxes.erase(state.source.boxes.begin() +
                             static_cast<std::ptrdiff_t>(selected.index));
  } else if (selected.kind == CreativeEditorWorldLayoutSelectionKind::Opening &&
             selected.index < state.source.openings.size()) {
    state.source.openings.erase(state.source.openings.begin() +
                                static_cast<std::ptrdiff_t>(selected.index));
  } else if (selected.kind == CreativeEditorWorldLayoutSelectionKind::Wall &&
             selected.index < state.source.walls.size()) {
    const std::size_t removedWall = selected.index;
    state.source.walls.erase(state.source.walls.begin() +
                             static_cast<std::ptrdiff_t>(removedWall));
    std::erase_if(state.source.openings,
                  [&](cr::CreativeWorldLayoutOpening& opening) {
                    if (opening.wallIndex == removedWall) {
                      return true;
                    }
                    if (opening.wallIndex > removedWall) {
                      --opening.wallIndex;
                    }
                    return false;
                  });
  } else {
    return {false, false, "creative_editor_world_layout_selection_missing"};
  }
  state.selection = {};
  noteSourceChange(state, "layout symbol deleted");
  return {true, true, "creative_editor_world_layout_selection_deleted"};
}

CreativeEditorWorldLayoutEditReceipt cancelCreativeEditorWorldLayoutPreview(
    CreativeEditorWorldLayoutState& state) noexcept {
  const bool changed = state.previewVisible;
  invalidatePreview(state);
  state.statusMessage = "3D preview closed";
  return {true, changed, "creative_editor_world_layout_preview_cancelled"};
}

CreativeEditorWorldLayoutPreviewReceipt previewCreativeEditorWorldLayout(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document) {
  CreativeEditorWorldLayoutPreviewReceipt receipt;
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, state.source);
  receipt.status = compiled.receipt.status;
  if (!compiled.receipt.accepted) {
    state.statusMessage = compiled.receipt.reasonCode;
    receipt.reasonCode = compiled.receipt.reasonCode;
    invalidatePreview(state);
    return receipt;
  }
  cr::CreativeWorldLayoutPreviewResult preview =
      cr::previewCreativeWorldLayoutPlan(document, compiled.plan);
  receipt.status = preview.status;
  receipt.accepted = preview.accepted;
  receipt.changed = preview.accepted;
  receipt.reasonCode = preview.reasonCode;
  if (!preview.accepted) {
    state.statusMessage = preview.reasonCode;
    invalidatePreview(state);
    return receipt;
  }
  state.preview = std::move(preview);
  state.previewVisible = true;
  state.previewLayoutRevision = state.revision;
  state.statusMessage = "exact 3D preview ready";
  return receipt;
}

CreativeEditorWorldLayoutApplyReceipt confirmCreativeEditorWorldLayout(
    CreativeEditorWorldLayoutState& state, cr::CreativeAppState& appState) {
  CreativeEditorWorldLayoutApplyReceipt result;
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(),
                                       state.source);
  if (!compiled.receipt.accepted) {
    result.reasonCode = compiled.receipt.reasonCode;
    state.statusMessage = result.reasonCode;
    return result;
  }
  result.apply = cr::applyCreativeWorldLayoutPlanWithHistory(
      appState, compiled.plan, "desktop_world_layout_confirm");
  result.accepted = result.apply.accepted;
  result.changed = result.apply.changed;
  result.reasonCode = result.apply.reasonCode;
  if (result.accepted) {
    state.generatedRevision = state.revision;
    invalidatePreview(state);
    state.statusMessage =
        result.changed ? "layout generated in 3D" : "3D output already current";
  } else {
    state.statusMessage = result.reasonCode;
  }
  return result;
}

}  // namespace iggy3d_creative_app
