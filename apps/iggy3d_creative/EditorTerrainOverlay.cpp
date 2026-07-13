#include "EditorTerrain.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <vector>

#include "app/iggy3d/creative/document/Document.hpp"
#include "runtime/movement/MovementPolicy.hpp"

#include "EditorState.hpp"
#include "EditorTerrainInternal.hpp"
#include "EditorTerrainPaint.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

using terrain_detail::terrainGradePlan;
using terrain_detail::terrainPointerCoord;
using terrain_detail::terrainRodBounds;
using terrain_detail::terrainSeedPlan;

[[nodiscard]] cr::CreativeTerrainCoord2 aimedTerrainCoord(
    const CreativeEditorState& editor) noexcept {
  if (editor.terrain.selectionValid) {
    return editor.terrain.selectedCoord;
  }
  if (editor.terrain.hoverValid) {
    return editor.terrain.hoverCoord;
  }
  if (editor.interaction.target.terrainHit) {
    return editor.interaction.target.terrainCell;
  }
  const cr::CreativeGridCoord3 cell =
      editor.interaction.target.grid.targetCell;
  return {cell.x, cell.z};
}

void appendBounds(std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
                  cr::CreativeBounds bounds,
                  iggy3d::RenderLineColor color,
                  float thickness) {
  const cr::CreativeCoreVec3Conversion minimum =
      cr::creativeVec3ToCoreChecked(bounds.min);
  const cr::CreativeCoreVec3Conversion maximum =
      cr::creativeVec3ToCoreChecked(bounds.max);
  if (minimum.converted && maximum.converted) {
    appendStandaloneWireframeBoxEdges(lines, minimum.value, maximum.value,
                                      color, thickness);
  }
}

void appendTerrainFootprintOutline(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
    cr::CreativeGridSettings grid,
    cr::CreativeTerrainControlPoint control,
    iggy3d::RenderLineColor color,
    float thickness) {
  const std::int32_t radius = control.radiusCells;
  const auto insideDisk = [radius](std::int32_t dx, std::int32_t dz) {
    const std::int64_t x = dx;
    const std::int64_t z = dz;
    return x * x + z * z <=
           static_cast<std::int64_t>(radius) * radius;
  };
  const auto appendEdge = [&](cr::CreativeVec3 start, cr::CreativeVec3 end) {
    const cr::CreativeCoreVec3Conversion coreStart =
        cr::creativeVec3ToCoreChecked(start);
    const cr::CreativeCoreVec3Conversion coreEnd =
        cr::creativeVec3ToCoreChecked(end);
    if (!coreStart.converted || !coreEnd.converted) {
      return;
    }
    iggy3d::RenderCreativeWireframeDebugLine line;
    line.start = coreStart.value;
    line.end = coreEnd.value;
    line.color = color;
    line.thickness = thickness;
    lines.push_back(line);
  };

  const double y = grid.origin.y + grid.cellSizeMeters * 0.12;
  for (std::int32_t dz = -radius; dz <= radius; ++dz) {
    for (std::int32_t dx = -radius; dx <= radius; ++dx) {
      if (!insideDisk(dx, dz)) {
        continue;
      }
      const double minimumX =
          grid.origin.x + (control.coord.x + dx) * grid.cellSizeMeters;
      const double maximumX = minimumX + grid.cellSizeMeters;
      const double minimumZ =
          grid.origin.z + (control.coord.z + dz) * grid.cellSizeMeters;
      const double maximumZ = minimumZ + grid.cellSizeMeters;
      if (!insideDisk(dx - 1, dz)) {
        appendEdge({minimumX, y, minimumZ}, {minimumX, y, maximumZ});
      }
      if (!insideDisk(dx + 1, dz)) {
        appendEdge({maximumX, y, minimumZ}, {maximumX, y, maximumZ});
      }
      if (!insideDisk(dx, dz - 1)) {
        appendEdge({minimumX, y, minimumZ}, {maximumX, y, minimumZ});
      }
      if (!insideDisk(dx, dz + 1)) {
        appendEdge({minimumX, y, maximumZ}, {maximumX, y, maximumZ});
      }
    }
  }
}

[[nodiscard]] cr::CreativeVec3 terrainRodTopCenter(
    cr::CreativeGridSettings grid,
    cr::CreativeTerrainControlPoint control) noexcept {
  return {grid.origin.x +
              (static_cast<double>(control.coord.x) + 0.5) *
                  grid.cellSizeMeters,
          grid.origin.y + control.heightCells * grid.cellSizeMeters,
          grid.origin.z +
              (static_cast<double>(control.coord.z) + 0.5) *
                  grid.cellSizeMeters};
}

void appendTerrainLine(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
    cr::CreativeVec3 start,
    cr::CreativeVec3 end,
    iggy3d::RenderLineColor color,
    float thickness) {
  const cr::CreativeCoreVec3Conversion coreStart =
      cr::creativeVec3ToCoreChecked(start);
  const cr::CreativeCoreVec3Conversion coreEnd =
      cr::creativeVec3ToCoreChecked(end);
  if (!coreStart.converted || !coreEnd.converted) {
    return;
  }
  iggy3d::RenderCreativeWireframeDebugLine line;
  line.start = coreStart.value;
  line.end = coreEnd.value;
  line.color = color;
  line.thickness = thickness;
  lines.push_back(line);
}

void appendTerrainGradePreview(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    float thickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines) {
  constexpr iggy3d::RenderLineColor anchorColor{0.18F, 0.82F, 0.92F, 1.0F};
  constexpr iggy3d::RenderLineColor gradeColor{0.30F, 1.0F, 0.38F, 1.0F};
  constexpr iggy3d::RenderLineColor invalidColor{1.0F, 0.20F, 0.16F, 1.0F};
  const cr::CreativeGridSettings grid = document.gridSettings();
  cr::CreativeTerrainCoord2 target{};
  if (!editor.terrain.grade.anchorValid) {
    if (!terrainPointerCoord(editor, target)) {
      return;
    }
    const cr::CreativeTerrainControlPoint* control =
        document.terrainField().controlAt(target);
    if (control != nullptr) {
      appendBounds(lines, terrainRodBounds(grid, *control, 0.24), anchorColor,
                   thickness * 1.4F);
    }
    return;
  }

  const CreativeTerrainGradeState& grade = editor.terrain.grade;
  const cr::CreativeTerrainControlPoint anchor{
      grade.anchorCoord, grade.anchorHeightCells, grade.radiusCells};
  if (!terrainPointerCoord(editor, target)) {
    appendBounds(lines, terrainRodBounds(grid, anchor, 0.24), anchorColor,
                 thickness * 1.4F);
    return;
  }
  const cr::CreativeTerrainGradePlan plan = terrainGradePlan(editor, target);
  if (!plan.accepted) {
    const cr::CreativeTerrainControlPoint rejected{
        target, grade.targetHeightCells, grade.radiusCells};
    appendBounds(lines, terrainRodBounds(grid, anchor, 0.24), invalidColor,
                 thickness * 1.4F);
    appendBounds(lines, terrainRodBounds(grid, rejected, 0.24), invalidColor,
                 thickness * 1.4F);
    appendTerrainLine(lines, terrainRodTopCenter(grid, anchor),
                      terrainRodTopCenter(grid, rejected), invalidColor,
                      thickness * 1.2F);
    return;
  }

  cr::CreativeVec3 previousTop{};
  bool hasPrevious = false;
  for (const cr::CreativeTerrainControlEdit& edit : plan.items()) {
    appendBounds(lines, terrainRodBounds(grid, edit.control, 0.20), gradeColor,
                 thickness);
    const cr::CreativeVec3 top = terrainRodTopCenter(grid, edit.control);
    if (hasPrevious) {
      appendTerrainLine(lines, previousTop, top, gradeColor, thickness * 1.2F);
    }
    previousTop = top;
    hasPrevious = true;
  }
  if (!plan.items().empty()) {
    appendTerrainFootprintOutline(lines, grid, plan.items().front().control,
                                  anchorColor, thickness);
    if (plan.items().size() > 1U) {
      appendTerrainFootprintOutline(lines, grid, plan.items().back().control,
                                    gradeColor, thickness);
    }
  }
}

}  // namespace

void appendCreativeEditorTerrainFootprintOutline(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
    cr::CreativeGridSettings grid,
    cr::CreativeTerrainControlPoint control,
    iggy3d::RenderLineColor color,
    float thickness) {
  appendTerrainFootprintOutline(lines, grid, control, color, thickness);
}

void appendCreativeEditorTerrainControlGuide(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
    cr::CreativeGridSettings grid,
    cr::CreativeTerrainControlPoint control,
    iggy3d::RenderLineColor color,
    float thickness) {
  appendBounds(lines, terrainRodBounds(grid, control, 0.18), color, thickness);
}

void appendCreativeEditorTerrainPatchSlopeTriangles(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
    const cr::CreativeTerrainSurfacePatch& patch,
    float thickness) {
  constexpr iggy3d::RenderLineColor walkable{0.20F, 1.0F, 0.35F, 1.0F};
  constexpr iggy3d::RenderLineColor careful{1.0F, 0.82F, 0.16F, 1.0F};
  constexpr iggy3d::RenderLineColor blocked{1.0F, 0.20F, 0.18F, 1.0F};
  const cr::CreativeCoreVec3Conversion center =
      cr::creativeVec3ToCoreChecked(patch.center);
  if (!center.converted) {
    return;
  }
  for (std::size_t index = 0U; index < patch.corners.size(); ++index) {
    const cr::CreativeCoreVec3Conversion first =
        cr::creativeVec3ToCoreChecked(patch.corners[index]);
    const cr::CreativeCoreVec3Conversion second = cr::creativeVec3ToCoreChecked(
        patch.corners[(index + 1U) % patch.corners.size()]);
    if (!first.converted || !second.converted) {
      continue;
    }
    iggy3d::RenderLineColor color = blocked;
    iggy3d::Vec3 normal;
    if (iggy3d::tryNormalize(
            iggy3d::cross(first.value - center.value,
                          second.value - center.value),
            normal)) {
      if (normal.y < 0.0F) {
        normal = normal * -1.0F;
      }
      const iggy3d::SlopeSample slope = iggy3d::sampleSlope(normal);
      if (slope.valid && slope.walkable) {
        color = slope.carefulFooting ? careful : walkable;
      }
    }
    const auto append = [&lines, color, thickness](iggy3d::Vec3 start,
                                                    iggy3d::Vec3 end) {
      iggy3d::RenderCreativeWireframeDebugLine line;
      line.start = start;
      line.end = end;
      line.color = color;
      line.thickness = thickness;
      lines.push_back(line);
    };
    append(center.value, first.value);
    append(first.value, second.value);
    append(second.value, center.value);
  }
}

void appendCreativeEditorTerrainOverlay(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines,
    bool captureMode) {
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const bool terrainControl =
      held.kind == cr::CreativeHeldItemKind::TerrainControl;
  const bool terrainPaint = held.kind == cr::CreativeHeldItemKind::TerrainPaint;
  const bool terrainGrade = held.kind == cr::CreativeHeldItemKind::TerrainGrade;
  const bool terrainSculpt =
      held.kind == cr::CreativeHeldItemKind::TerrainSculpt;
  const bool terrainProfile =
      held.kind == cr::CreativeHeldItemKind::TerrainProfile;
  const bool terrainPath = held.kind == cr::CreativeHeldItemKind::TerrainPath;
  const bool terrainRegion =
      held.kind == cr::CreativeHeldItemKind::TerrainRegion;
  if (captureMode ||
      (!terrainControl && !terrainPaint && !terrainGrade && !terrainSculpt &&
       !terrainProfile && !terrainPath && !terrainRegion) ||
      editor.catalog.model.open || editor.catalog.toolWheel.open ||
      editor.toolOptions.open || editor.controls.open || editor.transform.active ||
      editor.transform.controlsOpen) {
    return;
  }

  const cr::CreativeGridSettings grid = document.gridSettings();
  const float thickness = std::max(0.02F, wireThickness * 0.75F);
  constexpr iggy3d::RenderLineColor existingColor{0.18F, 0.82F, 0.92F, 1.0F};
  constexpr iggy3d::RenderLineColor influenceColor{0.16F, 0.45F, 0.52F, 0.9F};
  constexpr iggy3d::RenderLineColor hoverColor{0.30F, 1.0F, 0.38F, 1.0F};
  constexpr iggy3d::RenderLineColor selectedColor{1.0F, 0.42F, 0.82F, 1.0F};
  constexpr iggy3d::RenderLineColor previewColor{0.98F, 0.88F, 0.16F, 1.0F};
  if (terrainPaint) {
    appendCreativeEditorTerrainPaintOverlay(document, editor, thickness,
                                            wireLines);
    return;
  }
  for (const cr::CreativeTerrainControlPoint& control :
       document.terrainField().controls()) {
    const bool selected = editor.terrain.selectionValid &&
                          editor.terrain.selectedCoord == control.coord;
    const bool hovered = editor.terrain.hoverValid &&
                         editor.terrain.hoverCoord == control.coord;
    appendBounds(wireLines, terrainRodBounds(grid, control, 0.18),
                 selected ? selectedColor : hovered ? hoverColor : existingColor,
                 (selected || hovered) ? thickness * 1.5F : thickness);
    cr::CreativeBounds influence = terrainRodBounds(
        grid, control, static_cast<double>(control.radiusCells * 2U + 1U));
    influence.max.y = influence.min.y + grid.cellSizeMeters * 0.08;
    appendBounds(wireLines, influence, influenceColor, thickness * 0.65F);
  }

  if (terrainSculpt) {
    appendCreativeEditorTerrainSculptOverlay(document, editor, thickness,
                                             wireLines);
    return;
  }

  if (terrainProfile) {
    appendCreativeEditorTerrainProfileOverlay(document, editor, thickness,
                                              wireLines);
    return;
  }

  if (terrainPath) {
    appendCreativeEditorTerrainPathOverlay(document, editor, thickness,
                                           wireLines);
    return;
  }

  if (terrainRegion) {
    if (editor.terrain.region.stamp.active) {
      appendCreativeEditorTerrainStampOverlay(document, editor, thickness,
                                              wireLines);
    } else {
      appendCreativeEditorTerrainRegionOverlay(document, editor, thickness,
                                               wireLines);
    }
    return;
  }

  if (terrainGrade) {
    appendTerrainGradePreview(document, editor, thickness, wireLines);
    return;
  }

  if (terrainControl &&
      editor.toolSettings.terrainRodStampMode ==
          cr::CreativeTerrainRodStampMode::Seed) {
    cr::CreativeTerrainCoord2 center{};
    if (!terrainPointerCoord(editor, center)) {
      return;
    }
    const cr::CreativeMaterialStrokeKind kind =
        editor.terrain.stroke.repeat.active
            ? editor.terrain.stroke.repeat.kind
            : cr::CreativeMaterialStrokeKind::Place;
    const cr::CreativeTerrainSeedPlan plan =
        terrainSeedPlan(document, editor, center, kind);
    constexpr iggy3d::RenderLineColor seedColor{0.30F, 1.0F, 0.38F, 1.0F};
    constexpr iggy3d::RenderLineColor clearColor{1.0F, 0.20F, 0.18F, 1.0F};
    const iggy3d::RenderLineColor color =
        !plan.accepted || kind == cr::CreativeMaterialStrokeKind::Remove
            ? clearColor
            : seedColor;
    appendTerrainFootprintOutline(
        wireLines, grid,
        {center, editor.terrain.heightCells,
         cr::creativeTerrainSeedRadiusCells(
             editor.toolSettings.terrainSeedRadius)},
        color, thickness * 1.2F);
    for (const cr::CreativeTerrainControlEdit& edit : plan.items()) {
      appendBounds(wireLines, terrainRodBounds(grid, edit.control, 0.22), color,
                   thickness * 1.15F);
    }
    return;
  }

  if (editor.terrain.selectionValid || editor.terrain.hoverValid ||
      editor.interaction.target.grid.valid) {
    const cr::CreativeTerrainControlPoint preview{
        aimedTerrainCoord(editor), editor.terrain.heightCells,
        editor.terrain.radiusCells};
    appendBounds(wireLines, terrainRodBounds(grid, preview, 0.24), previewColor,
                 thickness * 1.2F);
    appendTerrainFootprintOutline(wireLines, grid, preview, previewColor,
                                  thickness);
  }
}

}  // namespace iggy3d_creative_app
