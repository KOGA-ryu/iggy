#include "EditorTerrain.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <span>
#include <vector>

#include "app/iggy3d/creative/document/Document.hpp"
#include "runtime/movement/MovementPolicy.hpp"

#include "EditorState.hpp"
#include "EditorTerrainInternal.hpp"
#include "EditorTerrainPaint.hpp"
#include "EditorWorldLayout.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

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

[[nodiscard]] bool coordLess(cr::CreativeTerrainCoord2 lhs,
                             cr::CreativeTerrainCoord2 rhs) noexcept {
  return lhs.z < rhs.z || (lhs.z == rhs.z && lhs.x < rhs.x);
}

[[nodiscard]] bool containsCoord(
    std::span<const cr::CreativeTerrainCoord2> coords,
    cr::CreativeTerrainCoord2 coord) noexcept {
  return std::binary_search(coords.begin(), coords.end(), coord, coordLess);
}

[[nodiscard]] bool containsOffsetCoord(
    std::span<const cr::CreativeTerrainCoord2> coords,
    cr::CreativeTerrainCoord2 coord,
    std::int32_t dx,
    std::int32_t dz) noexcept {
  const std::int64_t x = static_cast<std::int64_t>(coord.x) + dx;
  const std::int64_t z = static_cast<std::int64_t>(coord.z) + dz;
  if (x < std::numeric_limits<std::int32_t>::min() ||
      x > std::numeric_limits<std::int32_t>::max() ||
      z < std::numeric_limits<std::int32_t>::min() ||
      z > std::numeric_limits<std::int32_t>::max()) {
    return false;
  }
  return containsCoord(coords, {static_cast<std::int32_t>(x),
                                static_cast<std::int32_t>(z)});
}

template <typename Height>
void appendTerrainCellUnionOutline(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
    cr::CreativeGridSettings grid,
    std::span<const cr::CreativeTerrainCoord2> coords,
    Height height,
    iggy3d::RenderLineColor color,
    float thickness) {
  for (const cr::CreativeTerrainCoord2 coord : coords) {
    const double minimumX =
        grid.origin.x + coord.x * grid.cellSizeMeters;
    const double maximumX = minimumX + grid.cellSizeMeters;
    const double minimumZ =
        grid.origin.z + coord.z * grid.cellSizeMeters;
    const double maximumZ = minimumZ + grid.cellSizeMeters;
    const double y = height(coord);
    if (!containsOffsetCoord(coords, coord, -1, 0)) {
      appendTerrainLine(lines, {minimumX, y, minimumZ},
                        {minimumX, y, maximumZ}, color, thickness);
    }
    if (!containsOffsetCoord(coords, coord, 1, 0)) {
      appendTerrainLine(lines, {maximumX, y, minimumZ},
                        {maximumX, y, maximumZ}, color, thickness);
    }
    if (!containsOffsetCoord(coords, coord, 0, -1)) {
      appendTerrainLine(lines, {minimumX, y, minimumZ},
                        {maximumX, y, minimumZ}, color, thickness);
    }
    if (!containsOffsetCoord(coords, coord, 0, 1)) {
      appendTerrainLine(lines, {minimumX, y, maximumZ},
                        {maximumX, y, maximumZ}, color, thickness);
    }
  }
}

[[nodiscard]] const cr::CreativeWorldLayoutTerrainSourceImpact*
selectedTerrainSourceImpact(const cr::CreativeDocument& document,
                            const CreativeEditorState& editor) noexcept {
  const CreativeEditorWorldLayoutState& layout = editor.worldLayout;
  const CreativeEditorWorldLayoutDiagnosticCache& cache =
      layout.diagnosticCache;
  if (!editor.desktopUi.showWorldLayout || !cache.valid ||
      cache.sourceEpoch != layout.sourceEpoch ||
      cache.layoutRevision != layout.revision ||
      cache.documentId != document.id() ||
      cache.documentRevision != document.revision() ||
      cache.terrainRevision != document.terrainField().revision() ||
      cache.materialRevision != document.terrainMaterialField().revision() ||
      !cache.report.terrainImpactPlan.accepted) {
    return nullptr;
  }
  const cr::CreativeWorldLayoutTable table =
      creativeEditorWorldLayoutSelectionTable(layout.selection.kind);
  if (table != cr::CreativeWorldLayoutTable::TerrainProfile &&
      table != cr::CreativeWorldLayoutTable::TerrainPath) {
    return nullptr;
  }
  return cr::findCreativeWorldLayoutTerrainSourceImpact(
      cache.report.terrainImpactPlan, table, layout.selection.index);
}

void appendTerrainGradePreview(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    float thickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines) {
  constexpr iggy3d::RenderLineColor storedColor{0.18F, 0.58F, 0.68F, 0.75F};
  constexpr iggy3d::RenderLineColor anchorColor{0.18F, 0.82F, 0.92F, 1.0F};
  constexpr iggy3d::RenderLineColor gradeColor{0.30F, 1.0F, 0.38F, 1.0F};
  constexpr iggy3d::RenderLineColor falloffColor{0.90F, 0.74F, 0.18F, 0.8F};
  constexpr iggy3d::RenderLineColor invalidColor{1.0F, 0.20F, 0.16F, 1.0F};
  const cr::CreativeGridSettings grid = document.gridSettings();
  const CreativeTerrainGradeState& grade = editor.terrain.grade;
  const auto point = [&](cr::CreativeTerrainCoord2 coord,
                         std::uint16_t height) {
    return cr::CreativeVec3{
        grid.origin.x + (static_cast<double>(coord.x) + 0.5) *
                            grid.cellSizeMeters,
        grid.origin.y + height * grid.cellSizeMeters +
            grid.cellSizeMeters * 0.08,
        grid.origin.z + (static_cast<double>(coord.z) + 0.5) *
                            grid.cellSizeMeters};
  };
  const auto handleBounds = [&](cr::CreativeTerrainCoord2 coord,
                                std::uint16_t height) {
    const cr::CreativeVec3 center = point(coord, height);
    const double half = grid.cellSizeMeters * 0.18;
    return cr::CreativeBounds{{center.x - half, center.y - half,
                               center.z - half},
                              {center.x + half, center.y + half,
                               center.z + half}};
  };
  const auto appendBoundary = [&](const cr::CreativeTerrainGradeRecipe& recipe,
                                  double offsetCells,
                                  iggy3d::RenderLineColor color,
                                  float lineThickness) {
    const double dx = static_cast<double>(recipe.end.x) - recipe.start.x;
    const double dz = static_cast<double>(recipe.end.z) - recipe.start.z;
    const double length = std::hypot(dx, dz);
    if (!std::isfinite(length) || length <= 0.0) {
      return;
    }
    const double offset = (offsetCells + 0.5) * grid.cellSizeMeters;
    const double perpendicularX = -dz / length * offset;
    const double perpendicularZ = dx / length * offset;
    const cr::CreativeVec3 start = point(recipe.start,
                                         recipe.startHeightCells);
    const cr::CreativeVec3 end = point(recipe.end, recipe.endHeightCells);
    const cr::CreativeVec3 startLeft{start.x + perpendicularX, start.y,
                                     start.z + perpendicularZ};
    const cr::CreativeVec3 startRight{start.x - perpendicularX, start.y,
                                      start.z - perpendicularZ};
    const cr::CreativeVec3 endLeft{end.x + perpendicularX, end.y,
                                   end.z + perpendicularZ};
    const cr::CreativeVec3 endRight{end.x - perpendicularX, end.y,
                                    end.z - perpendicularZ};
    appendTerrainLine(lines, startLeft, endLeft, color, lineThickness);
    appendTerrainLine(lines, endLeft, endRight, color, lineThickness);
    appendTerrainLine(lines, endRight, startRight, color, lineThickness);
    appendTerrainLine(lines, startRight, startLeft, color, lineThickness);
  };

  for (const cr::CreativeTerrainOperation& operation :
       document.terrainOperationStack().operations) {
    if (operation.kind != cr::CreativeTerrainOperationKind::Grade ||
        operation.id == grade.editingOperationId) {
      continue;
    }
    appendTerrainLine(lines,
                      point(operation.grade.start,
                            operation.grade.startHeightCells),
                      point(operation.grade.end,
                            operation.grade.endHeightCells),
                      storedColor, thickness * 0.8F);
    appendBounds(lines,
                 handleBounds(operation.grade.start,
                              operation.grade.startHeightCells),
                 storedColor, thickness * 0.8F);
    appendBounds(lines,
                 handleBounds(operation.grade.end,
                              operation.grade.endHeightCells),
                 storedColor, thickness * 0.8F);
  }
  if (!grade.active) {
    return;
  }

  const bool valid = grade.operationPreview.receipt.accepted;
  const iggy3d::RenderLineColor activeColor = valid ? gradeColor : invalidColor;
  appendTerrainLine(lines, point(grade.recipe.start,
                                 grade.recipe.startHeightCells),
                    point(grade.recipe.end, grade.recipe.endHeightCells),
                    activeColor, thickness * 1.35F);
  appendBoundary(grade.recipe, grade.recipe.halfWidthCells, activeColor,
                 thickness);
  if (grade.recipe.falloffCells > 0U) {
    appendBoundary(grade.recipe,
                   grade.recipe.halfWidthCells + grade.recipe.falloffCells,
                   valid ? falloffColor : invalidColor, thickness * 0.75F);
  }
  appendBounds(lines,
               handleBounds(grade.recipe.start,
                            grade.recipe.startHeightCells),
               grade.selectedHandle == CreativeTerrainGradeHandle::Start
                   ? activeColor
                   : anchorColor,
               thickness * 1.5F);
  appendBounds(lines,
               handleBounds(grade.recipe.end, grade.recipe.endHeightCells),
               grade.selectedHandle == CreativeTerrainGradeHandle::End
                   ? activeColor
                   : anchorColor,
               thickness * 1.5F);
}

}  // namespace

CreativeEditorTerrainSourceImpactOverlayFacts
appendCreativeEditorWorldLayoutTerrainImpactOverlay(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines,
    bool captureMode) {
  CreativeEditorTerrainSourceImpactOverlayFacts facts;
  if (captureMode) {
    return facts;
  }
  const cr::CreativeWorldLayoutTerrainSourceImpact* impact =
      selectedTerrainSourceImpact(document, editor);
  if (impact == nullptr) {
    return facts;
  }

  constexpr iggy3d::RenderLineColor currentColor{0.98F, 0.88F, 0.16F, 1.0F};
  constexpr iggy3d::RenderLineColor driftedColor{1.0F, 0.20F, 0.18F, 1.0F};
  const bool pending =
      editor.worldLayout.generatedRevision != editor.worldLayout.revision;
  const iggy3d::RenderLineColor color =
      !pending && impact->status ==
                      cr::CreativeWorldLayoutTerrainImpactStatus::Current
          ? currentColor
          : driftedColor;
  const float thickness = std::max(0.02F, wireThickness * 0.85F);
  const cr::CreativeGridSettings grid = document.gridSettings();
  const std::size_t before = wireLines.size();

  cr::CreativeBounds aggregate{};
  if (cr::creativeWorldLayoutTerrainImpactWorldBounds(*impact, grid,
                                                       aggregate)) {
    appendBounds(wireLines, aggregate, color, thickness * 1.4F);
  }
  for (const cr::CreativeTerrainControlPoint& control : impact->controls) {
    appendBounds(wireLines, terrainRodBounds(grid, control, 0.20), color,
                 thickness);
  }
  if (!impact->influenceCells.empty() &&
      !impact->influenceCellsClipped) {
    appendTerrainCellUnionOutline(
        wireLines, grid, impact->influenceCells,
        [grid](cr::CreativeTerrainCoord2) {
          return grid.origin.y + grid.cellSizeMeters * 0.12;
        },
        color, thickness * 0.8F);
  }
  if (!impact->materials.empty()) {
    std::vector<cr::CreativeTerrainCoord2> materialCoords;
    materialCoords.reserve(impact->materials.size());
    for (const cr::CreativeWorldLayoutTerrainMaterialImpact& material :
         impact->materials) {
      materialCoords.push_back(material.coord);
    }
    appendTerrainCellUnionOutline(
        wireLines, grid, materialCoords,
        [&document, grid](cr::CreativeTerrainCoord2 coord) {
          const cr::CreativeTerrainHeightSample sample =
              cr::sampleCreativeTerrainHeight(document.terrainField(), coord);
          return grid.origin.y +
                 (sample.present ? sample.heightCells + 0.04 : 0.12) *
                     grid.cellSizeMeters;
        },
        color, thickness * 1.1F);
  }

  facts.active = true;
  facts.status = pending
                     ? cr::CreativeWorldLayoutTerrainImpactStatus::Drifted
                     : impact->status;
  facts.controlCount = impact->controls.size();
  facts.materialCellCount = impact->materials.size();
  facts.edgeCount = wireLines.size() - before;
  facts.influenceCellsClipped = impact->influenceCellsClipped;
  return facts;
}

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

void appendTerrainGenerationFootprint(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    float thickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
    bool captureMode) {
  const CreativeEditorTerrainGenerationState& state =
      editor.terrainGeneration;
  if (captureMode || editor.catalog.model.open ||
      editor.catalog.toolWheel.open || editor.toolOptions.open ||
      editor.controls.open || editor.transform.active ||
      editor.transform.controlsOpen) {
    return;
  }

  const cr::CreativeTerrainGeneratorRecipe* recipe = nullptr;
  cr::CreativeTerrainCompositionMask mask = state.compositionRecipe.mask;
  if (state.previewActive) {
    recipe = &state.recipe;
  } else {
    const cr::CreativeTerrainOperation* operation =
        cr::findCreativeTerrainOperation(document.terrainOperationStack(),
                                         state.editingOperationId);
    if (operation != nullptr) {
      recipe = &operation->generation;
      mask = operation->composition.mask;
    }
  }
  if (recipe == nullptr || recipe->bounds.widthCells == 0U ||
      recipe->bounds.depthCells == 0U) {
    return;
  }

  constexpr iggy3d::RenderLineColor selectedColor{0.18F, 0.82F, 0.92F, 1.0F};
  constexpr iggy3d::RenderLineColor previewColor{0.20F, 1.0F, 0.35F, 1.0F};
  constexpr iggy3d::RenderLineColor rejectedColor{1.0F, 0.18F, 0.14F, 1.0F};
  const iggy3d::RenderLineColor color =
      !state.previewActive
          ? selectedColor
          : state.operationPreview.receipt.accepted ? previewColor
                                                    : rejectedColor;
  const cr::CreativeGridSettings grid = document.gridSettings();
  const double minimumX =
      grid.origin.x + recipe->bounds.minimum.x * grid.cellSizeMeters;
  const double minimumZ =
      grid.origin.z + recipe->bounds.minimum.z * grid.cellSizeMeters;
  const double maximumX =
      minimumX + recipe->bounds.widthCells * grid.cellSizeMeters;
  const double maximumZ =
      minimumZ + recipe->bounds.depthCells * grid.cellSizeMeters;
  const double y =
      grid.origin.y +
      (static_cast<double>(recipe->baseHeightCells) +
       static_cast<double>(recipe->reliefCells) + 0.15) *
          grid.cellSizeMeters;
  if (mask == cr::CreativeTerrainCompositionMask::Rectangle) {
    appendTerrainLine(lines, {minimumX, y, minimumZ},
                      {maximumX, y, minimumZ}, color, thickness);
    appendTerrainLine(lines, {maximumX, y, minimumZ},
                      {maximumX, y, maximumZ}, color, thickness);
    appendTerrainLine(lines, {maximumX, y, maximumZ},
                      {minimumX, y, maximumZ}, color, thickness);
    appendTerrainLine(lines, {minimumX, y, maximumZ},
                      {minimumX, y, minimumZ}, color, thickness);
    return;
  }

  constexpr std::size_t kEllipseSegmentCount = 32U;
  constexpr double kTau = 6.28318530717958647692;
  const double centerX = (minimumX + maximumX) * 0.5;
  const double centerZ = (minimumZ + maximumZ) * 0.5;
  const double radiusX = (maximumX - minimumX) * 0.5;
  const double radiusZ = (maximumZ - minimumZ) * 0.5;
  for (std::size_t index = 0U; index < kEllipseSegmentCount; ++index) {
    const double firstAngle =
        kTau * static_cast<double>(index) / kEllipseSegmentCount;
    const double secondAngle =
        kTau * static_cast<double>(index + 1U) / kEllipseSegmentCount;
    appendTerrainLine(
        lines,
        {centerX + std::cos(firstAngle) * radiusX, y,
         centerZ + std::sin(firstAngle) * radiusZ},
        {centerX + std::cos(secondAngle) * radiusX, y,
         centerZ + std::sin(secondAngle) * radiusZ},
        color, thickness);
  }
}

void appendCreativeEditorTerrainOverlay(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines,
    bool captureMode) {
  appendTerrainGenerationFootprint(document, editor,
                                   std::max(0.02F, wireThickness * 0.75F),
                                   wireLines, captureMode);
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
