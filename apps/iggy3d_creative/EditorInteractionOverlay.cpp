#include "EditorInteraction.hpp"

#include <algorithm>
#include <cstdio>
#include <string>

#include "EditorConnectedFill.hpp"
#include "EditorGroup.hpp"
#include "EditorPattern.hpp"
#include "EditorPreviewProxies.hpp"
#include "EditorState.hpp"
#include "EditorStructuralPlacement.hpp"
#include "EditorSurfaceExtrude.hpp"
#include "EditorTerrain.hpp"
#include "EditorTerrainPaint.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/input/HeldItemRegistry.hpp"
#include "render/debug/DebugHudText.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

[[nodiscard]] std::string shapeHotbarLabel(
    cr::CreativeHeldItemKind kind,
    const cr::CreativeToolSettings& settings) {
  std::string output =
      cr::creativeVolumeOperationForHeldItem(kind) ==
              cr::CreativeVolumeOperationKind::Fill
          ? "F"
          : "H";
  switch (settings.shapeBrushKind) {
    case cr::CreativeShapeBrushKind::Box: return output + "B";
    case cr::CreativeShapeBrushKind::Line: return output + "L";
    case cr::CreativeShapeBrushKind::Ellipsoid: return output + "E";
    case cr::CreativeShapeBrushKind::Cylinder:
      output.push_back('C');
      output.append(cr::toString(settings.shapeBrushAxis));
      return output;
    case cr::CreativeShapeBrushKind::Count: return output + "?";
  }
  return output + "?";
}

[[nodiscard]] std::string hotbarLabel(
    const cr::CreativeHotbarEntry& entry,
    const cr::CreativeToolSettings& settings,
    const CreativeMaterialBrushPresetBank& brushPresets,
    std::size_t slot) {
  switch (cr::describeCreativeHeldItem(entry.kind).hotbarLabelMode) {
    case cr::CreativeHeldItemHotbarLabelMode::DirectShape:
      return shapeHotbarLabel(entry.kind, settings);
    case cr::CreativeHeldItemHotbarLabelMode::MaterialBrush: {
      CreativeMaterialBrushGestureConfig config =
          creativeMaterialBrushGestureConfig(settings);
      static_cast<void>(
          creativeMaterialBrushPresetForSlot(brushPresets, slot, config));
      return creativeMaterialBrushPresetHotbarLabel(config);
    }
    case cr::CreativeHeldItemHotbarLabelMode::Material: {
      const std::string name(cr::toString(entry.objectKind));
      return name.substr(0, std::min<std::size_t>(4U, name.size()));
    }
    case cr::CreativeHeldItemHotbarLabelMode::KindPrefix:
    case cr::CreativeHeldItemHotbarLabelMode::Count:
      return std::string(cr::toString(entry.kind)).substr(0, 4);
  }
  return {};
}

void appendColoredText(std::vector<iggy3d::DebugHudGlyphQuad>& glyphs,
                       std::string_view text,
                       std::int32_t x,
                       std::int32_t y,
                       std::uint32_t width,
                       std::uint32_t height,
                       bool selected) {
  iggy3d::DebugHudLayoutResult layout =
      iggy3d::layoutDebugHudTextAt(text, x, y, width, height);
  for (iggy3d::DebugHudGlyphQuad& quad : layout.quads) {
    quad.r = selected ? 0.08F : 0.88F;
    quad.g = selected ? 0.08F : 0.90F;
    quad.b = selected ? 0.08F : 0.94F;
    quad.a = 1.0F;
  }
  glyphs.insert(glyphs.end(), layout.quads.begin(), layout.quads.end());
}

void appendHeldItemStatusText(
    std::vector<iggy3d::DebugHudGlyphQuad>& glyphs,
    std::string_view text,
    std::int32_t x,
    std::int32_t y,
    std::uint32_t width,
    std::uint32_t height) {
  iggy3d::DebugHudLayoutResult layout =
      iggy3d::layoutDebugHudTextAt(text, x, y, width, height);
  bool quickEditActive = false;
  for (iggy3d::DebugHudGlyphQuad& quad : layout.quads) {
    quickEditActive = quickEditActive || quad.source == '[';
    quad.r = quickEditActive ? 0.24F : 0.88F;
    quad.g = quickEditActive ? 1.0F : 0.90F;
    quad.b = quickEditActive ? 0.34F : 0.94F;
    quad.a = 1.0F;
  }
  glyphs.insert(glyphs.end(), layout.quads.begin(), layout.quads.end());
}

void appendHeldQuickEditStatus(std::string& output,
                               const CreativeEditorState& editor) {
  const std::string quickEdit = creativeEditorQuickEditStatusLabel(editor);
  if (!quickEdit.empty()) {
    output.append(" | [");
    output.append(quickEdit);
    output.push_back(']');
  }
}

void appendDirectShapeStatus(std::string& output,
                             const CreativeEditorState& editor) {
  output.append(" | ");
  output.append(cr::toString(editor.toolSettings.shapeBrushKind));
  if (editor.toolSettings.shapeBrushKind ==
      cr::CreativeShapeBrushKind::Cylinder) {
    output.push_back(' ');
    output.append(cr::toString(editor.toolSettings.shapeBrushAxis));
  }
  output.append(" | ");
  output.append(cr::toString(editor.placeBrush));
  if (editor.volume.selection.phase ==
      cr::CreativeVolumeSelectionPhase::FirstCorner) {
    output.append(" | Corner 1");
  } else if (editor.volume.selection.phase ==
             cr::CreativeVolumeSelectionPhase::Complete) {
    output.append(" | Ready");
  }
  appendHeldQuickEditStatus(output, editor);
}

void appendMaterialBrushStatus(std::string& output,
                               const CreativeEditorState& editor,
                               const cr::CreativeHotbarEntry& held) {
  const CreativeMaterialBrushGestureConfig brushConfig =
      editor.interaction.materialStroke.hasBrushAnchor
          ? editor.interaction.materialStroke.brushConfig
          : creativeMaterialBrushGestureConfig(editor.toolSettings);
  output.append(" | ");
  output.append(cr::toString(held.objectKind));
  output.append(" | ");
  output.append(cr::toString(brushConfig.shape));
  if (brushConfig.shape == cr::CreativeMaterialBrushShape::Cylinder) {
    output.push_back(' ');
    output.append(cr::toString(brushConfig.axis));
  }
  output.append(" | ");
  output.append(cr::toString(brushConfig.size));
  output.append(" | ");
  output.append(cr::toString(brushConfig.fill));
  output.append(" | ");
  output.append(cr::toString(brushConfig.guide));
  if (brushConfig.symmetry != cr::CreativeMaterialBrushSymmetry::Off) {
    output.append(" | ");
    output.append(cr::toString(brushConfig.symmetry));
    const CreativeMaterialBrushPivotState& pivot =
        editor.interaction.materialBrushPivot;
    if (pivot.locked) {
      output.append(" | PIVOT LOCKED ");
      output.append(std::to_string(pivot.lockedCell.x));
      output.push_back(' ');
      output.append(std::to_string(pivot.lockedCell.y));
      output.push_back(' ');
      output.append(std::to_string(pivot.lockedCell.z));
    }
  }
  output.append(" | ");
  output.append(cr::toString(brushConfig.mask));
  if (brushConfig.mask == cr::CreativeMaterialBrushMask::Replace) {
    output.push_back(' ');
    output.append(
        brushConfig.replaceSourceKind == cr::CreativeObjectKind::Unknown
            ? std::string_view{"ANY"}
            : cr::toString(brushConfig.replaceSourceKind));
  }
  const cr::CreativeMaterialBrushStampPlan stamp =
      cr::planCreativeMaterialBrushStamp(
          creativeMaterialBrushStampRequest(brushConfig));
  if (stamp.accepted) {
    output.append(" | ");
    output.append(std::to_string(stamp.cellCount));
    output.append(" VOXELS");
  }
  appendHeldQuickEditStatus(output, editor);
}

void appendLinearArrayStatus(std::string& output,
                             const CreativeEditorState& editor) {
  output.append(" | ");
  output.append(cr::toString(editor.toolSettings.arrayMode));
  if (editor.toolSettings.arrayMode == cr::CreativeArrayMode::Radial) {
    output.append(" | ");
    output.append(cr::toString(editor.toolSettings.radialArrayAxis));
    output.append(" | ");
    output.append(cr::toString(editor.toolSettings.radialArrayInstanceCount));
    output.append(" | ");
    output.append(cr::toString(editor.toolSettings.radialArraySweep));
  }
  appendHeldQuickEditStatus(output, editor);
}

void appendTerrainControlStatus(std::string& output,
                                const CreativeEditorState& editor) {
  const bool seedMode = editor.toolSettings.terrainRodStampMode ==
                        cr::CreativeTerrainRodStampMode::Seed;
  if (!seedMode && editor.terrain.selectionValid) {
    output.append(" | EDIT ");
    output.append(std::to_string(editor.terrain.selectedCoord.x));
    output.push_back(' ');
    output.append(std::to_string(editor.terrain.selectedCoord.z));
  } else if (!seedMode && editor.terrain.hoverValid) {
    output.append(" | ROD ");
    output.append(std::to_string(editor.terrain.hoverCoord.x));
    output.push_back(' ');
    output.append(std::to_string(editor.terrain.hoverCoord.z));
  }
  if (seedMode) {
    output.append(" | SEED RADIUS ");
    output.append(std::to_string(cr::creativeTerrainSeedRadiusCells(
        editor.toolSettings.terrainSeedRadius)));
    output.append(" | SPACING ");
    output.append(std::to_string(cr::creativeTerrainSeedSpacingCells(
        editor.toolSettings.terrainSeedSpacing)));
  }
  appendHeldQuickEditStatus(output, editor);
}

void appendTerrainGradeStatus(std::string& output,
                              const CreativeEditorState& editor) {
  if (editor.terrain.grade.anchorValid) {
    output.append(" | START ");
    output.append(std::to_string(editor.terrain.grade.anchorCoord.x));
    output.push_back(' ');
    output.append(std::to_string(editor.terrain.grade.anchorCoord.z));
  } else {
    output.append(" | SET START ROD");
  }
  appendHeldQuickEditStatus(output, editor);
}

void appendTerrainSculptStatus(std::string& output,
                               const CreativeEditorState& editor) {
  output.append(" | ");
  output.append(creativeEditorTerrainSculptQuickEditLabel(editor));
  if (editor.terrain.sculpt.preview.valid &&
      !editor.terrain.sculpt.preview.plan.accepted) {
    output.append(" | NO RODS");
  }
}

void appendTerrainProfileStatus(std::string& output,
                                const CreativeEditorState& editor) {
  output.append(" | ");
  output.append(cr::toString(editor.toolSettings.terrainProfileKind));
  output.append(" | ");
  output.append(cr::toString(editor.toolSettings.terrainProfileBlend));
  output.append(" | ");
  output.append(cr::toString(editor.toolSettings.terrainProfileRodPolicy));
  output.append(" | ");
  output.append(creativeEditorTerrainProfileQuickEditLabel(editor));
  if (editor.terrain.profile.preview.valid &&
      !editor.terrain.profile.preview.plan.accepted) {
    output.append(" | ");
    output.append(cr::toString(editor.terrain.profile.preview.plan.status));
  }
}

void appendTerrainPathStatus(std::string& output,
                             const CreativeEditorState& editor) {
  output.append(" | ");
  output.append(cr::toString(editor.toolSettings.terrainPathKind));
  output.append(" | ");
  output.append(cr::toString(editor.toolSettings.terrainPathElevation));
  output.append(" | ");
  output.append(creativeEditorTerrainPathQuickEditLabel(editor));
  output.append(" | POINTS ");
  output.append(std::to_string(editor.terrain.path.pointCount));
  if (editor.terrain.path.preview.valid &&
      editor.terrain.path.preview.pointCount >= 2U &&
      !editor.terrain.path.preview.plan.accepted) {
    output.append(" | ");
    output.append(cr::toString(editor.terrain.path.preview.plan.status));
  }
}

void appendTerrainRegionStatus(std::string& output,
                               const CreativeEditorState& editor) {
  output.append(" | ");
  output.append(creativeEditorTerrainRegionQuickEditLabel(editor));
  if (editor.terrain.region.stamp.active) {
    const CreativeTerrainStampPreviewCache& preview =
        editor.terrain.region.stamp.preview;
    if (preview.valid) {
      output.append(" | ");
      output.append(std::to_string(preview.plan.finalControlCount));
      output.append(" RODS");
      if (!preview.plan.accepted) {
        output.append(" | ");
        output.append(cr::toString(preview.plan.status));
      }
    }
    return;
  }
  const CreativeTerrainRegionPreviewCache& preview =
      editor.terrain.region.preview;
  if (preview.valid) {
    output.append(" | ");
    output.append(std::to_string(preview.plan.affectedControlCount));
    output.append(" RODS");
    if (!preview.plan.accepted) {
      output.append(" | ");
      output.append(cr::toString(preview.plan.status));
    }
  }
}

void appendConnectedFillStatus(std::string& output,
                               const CreativeEditorState& editor,
                               const cr::CreativeHotbarEntry& held) {
  output.append(" | ");
  output.append(cr::toString(held.objectKind));
  output.append(" | ");
  output.append(cr::toString(editor.toolSettings.connectedFillLimit));
  const CreativeEditorConnectedFillCache& cache =
      editor.interaction.connectedFill;
  const CreativeEditorWorldTarget& target = editor.interaction.target;
  const bool cacheMatchesTarget =
      cache.valid && target.voxelHit && cache.seedCell == target.voxelCell &&
      cache.limit == editor.toolSettings.connectedFillLimit;
  if (cacheMatchesTarget) {
    output.append(" | ");
    if (cache.plan.accepted) {
      output.append(std::to_string(cache.plan.cellCount));
      output.append(" CELLS");
    } else if (cache.plan.status ==
               cr::CreativeConnectedFillStatus::CapacityExceeded) {
      output.append("TOO LARGE");
    } else {
      output.append(cr::toString(cache.plan.status));
    }
  }
  appendHeldQuickEditStatus(output, editor);
}

void appendSurfaceExtrudeStatus(std::string& output,
                                const CreativeEditorState& editor,
                                const cr::CreativeHotbarEntry& held) {
  output.append(" | ");
  output.append(cr::toString(held.objectKind));
  output.append(" | ");
  output.append(cr::toString(editor.toolSettings.surfaceExtrudeDepth));
  output.append(" | ");
  output.append(cr::toString(editor.toolSettings.surfaceExtrudeLimit));
  const CreativeEditorSurfaceExtrudeCache& cache =
      editor.interaction.surfaceExtrude;
  const CreativeEditorWorldTarget& target = editor.interaction.target;
  cr::CreativeGridCoord3 outward{};
  const bool faceValid =
      creativeSurfaceFaceOffset(target.grid.faceNormal, outward);
  const bool cacheMatchesTarget =
      cache.valid && target.voxelHit && faceValid &&
      cache.seedCell == target.voxelCell && cache.outward == outward &&
      cache.kind == cr::CreativeSurfaceExtrudeKind::Extrude &&
      cache.depth == editor.toolSettings.surfaceExtrudeDepth &&
      cache.affectedCellLimit == editor.toolSettings.surfaceExtrudeLimit;
  if (cacheMatchesTarget) {
    output.append(" | ");
    if (cache.plan.accepted) {
      output.append(std::to_string(cache.plan.surfaceCellCount));
      output.append(" FACE / ");
      output.append(std::to_string(cache.plan.mutationCellCount));
      output.append(" CELLS");
    } else if (cache.plan.status ==
               cr::CreativeSurfaceExtrudeStatus::CapacityExceeded) {
      output.append("TOO LARGE");
    } else {
      output.append(cr::toString(cache.plan.status));
    }
  }
  appendHeldQuickEditStatus(output, editor);
}

}  // namespace

std::string creativeMaterialBrushPresetHotbarLabel(
    const CreativeMaterialBrushGestureConfig& preset) {
  std::string output;
  switch (preset.shape) {
    case cr::CreativeMaterialBrushShape::Cube:
      output = "C";
      break;
    case cr::CreativeMaterialBrushShape::Sphere:
      output = "S";
      break;
    case cr::CreativeMaterialBrushShape::Cylinder:
      output = "C";
      switch (preset.axis) {
        case cr::CreativeAxis3::X:
          output.append("X");
          break;
        case cr::CreativeAxis3::Y:
          output.append("Y");
          break;
        case cr::CreativeAxis3::Z:
          output.append("Z");
          break;
        case cr::CreativeAxis3::Count:
          return "B?";
      }
      break;
    case cr::CreativeMaterialBrushShape::Count:
      return "B?";
  }

  switch (preset.size) {
    case cr::CreativeMaterialBrushSize::OneCell:
      output.append("1");
      break;
    case cr::CreativeMaterialBrushSize::ThreeCells:
      output.append("3");
      break;
    case cr::CreativeMaterialBrushSize::FiveCells:
      output.append("5");
      break;
    case cr::CreativeMaterialBrushSize::Count:
      return "B?";
  }

  switch (preset.fill) {
    case cr::CreativeMaterialBrushFill::Solid:
      break;
    case cr::CreativeMaterialBrushFill::Shell:
      output.append("H");
      break;
    case cr::CreativeMaterialBrushFill::Count:
      return "B?";
  }

  switch (preset.symmetry) {
    case cr::CreativeMaterialBrushSymmetry::Off:
      break;
    case cr::CreativeMaterialBrushSymmetry::MirrorX:
    case cr::CreativeMaterialBrushSymmetry::MirrorY:
    case cr::CreativeMaterialBrushSymmetry::MirrorZ:
    case cr::CreativeMaterialBrushSymmetry::MirrorXZ:
      output.append("M");
      break;
    case cr::CreativeMaterialBrushSymmetry::Count:
      return "B?";
  }
  return output;
}

std::string creativeEditorHeldItemStatusLabel(
    const CreativeEditorState& editor) {
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  std::string output(cr::toString(held.kind));
  if (held.kind == cr::CreativeHeldItemKind::ObjectMove &&
      editor.interaction.movingPlatformPathEdit.available) {
    output.append(" | ROUTE ");
    output.append(std::to_string(
        editor.interaction.movingPlatformPathEdit.pointCount));
    output.append(" POINTS");
    if (editor.interaction.movingPlatformPathEdit.pointSelected) {
      output.append(" | EDIT ");
      output.append(std::to_string(
          static_cast<unsigned int>(
              editor.interaction.movingPlatformPathEdit.selectedPointIndex) +
          1U));
      output.push_back('/');
      output.append(std::to_string(
          editor.interaction.movingPlatformPathEdit.pointCount));
    }
  }
  switch (cr::describeCreativeHeldItem(held.kind).statusMode) {
    case cr::CreativeHeldItemStatusMode::MaterialBrush:
      appendMaterialBrushStatus(output, editor, held);
      break;
    case cr::CreativeHeldItemStatusMode::DirectShape:
      appendDirectShapeStatus(output, editor);
      break;
    case cr::CreativeHeldItemStatusMode::LinearArray:
      appendLinearArrayStatus(output, editor);
      break;
    case cr::CreativeHeldItemStatusMode::TerrainControl:
      appendTerrainControlStatus(output, editor);
      break;
    case cr::CreativeHeldItemStatusMode::TerrainPaint:
      output.append(" | ");
      output.append(cr::toString(editor.toolSettings.terrainPaintMode));
      output.append(" | ");
      output.append(cr::toString(editor.toolSettings.terrainPaintMaterial));
      switch (editor.toolSettings.terrainPaintMode) {
        case cr::CreativeTerrainPaintMode::Brush:
          output.append(" | RADIUS ");
          output.append(cr::toString(editor.toolSettings.terrainPaintRadius));
          break;
        case cr::CreativeTerrainPaintMode::Connected:
          break;
        case cr::CreativeTerrainPaintMode::Region:
          output.append(" | FROM ");
          output.append(cr::toString(editor.toolSettings.terrainPaintSource));
          switch (editor.terrainPaint.regionPhase) {
            case CreativeEditorTerrainPaintRegionPhase::Empty:
              output.append(" | CORNER 1");
              break;
            case CreativeEditorTerrainPaintRegionPhase::FirstCorner:
              output.append(" | CORNER 2");
              break;
            case CreativeEditorTerrainPaintRegionPhase::Complete:
              output.append(" | READY");
              break;
          }
          break;
        case cr::CreativeTerrainPaintMode::Count:
          break;
      }
      appendHeldQuickEditStatus(output, editor);
      break;
    case cr::CreativeHeldItemStatusMode::TerrainGrade:
      appendTerrainGradeStatus(output, editor);
      break;
    case cr::CreativeHeldItemStatusMode::TerrainSculpt:
      appendTerrainSculptStatus(output, editor);
      break;
    case cr::CreativeHeldItemStatusMode::TerrainProfile:
      appendTerrainProfileStatus(output, editor);
      break;
    case cr::CreativeHeldItemStatusMode::TerrainPath:
      appendTerrainPathStatus(output, editor);
      break;
    case cr::CreativeHeldItemStatusMode::TerrainRegion:
      appendTerrainRegionStatus(output, editor);
      break;
    case cr::CreativeHeldItemStatusMode::ConnectedFill:
      appendConnectedFillStatus(output, editor, held);
      break;
    case cr::CreativeHeldItemStatusMode::SurfaceExtrude:
      appendSurfaceExtrudeStatus(output, editor, held);
      break;
    case cr::CreativeHeldItemStatusMode::LogicLink: {
      output.append(" | ");
      output.append(cr::toString(editor.logicLinks.action));
      if (editor.logicLinks.sourceObjectId == cr::kInvalidObjectId) {
        output.append(" | SELECT CONTROL");
      } else {
        output.append(" | SOURCE ");
        output.append(std::to_string(editor.logicLinks.sourceObjectId));
      }
      break;
    }
    case cr::CreativeHeldItemStatusMode::Material:
      output.append(" | ");
      output.append(cr::toString(held.objectKind));
      if (creativeEditorUsesStructuralSpan(held)) {
        output.append(editor.interaction.structuralSpan.active
                          ? " | SET END"
                          : " | SET START");
      } else {
        appendHeldQuickEditStatus(output, editor);
      }
      break;
    case cr::CreativeHeldItemStatusMode::QuickEdit:
    case cr::CreativeHeldItemStatusMode::Count:
      appendHeldQuickEditStatus(output, editor);
      break;
  }
  if (creativeEditorGroupFocusActive(editor.groupFocus)) {
    output.append(" | EDIT GROUP ");
    output.append(std::to_string(editor.groupFocus.depth));
    output.append(" | CIRCLE EXIT");
  }
  return output;
}

void appendCreativeEditorCrosshairOverlay(
    const CreativeEditorState& editor,
    const iggy3d::RenderContentViewport& region,
    std::vector<iggy3d::RenderUiRect>& uiRects) {
  if (region.width == 0U || region.height == 0U) {
    return;
  }
  const bool inventoryModalOpen = editor.catalog.model.open ||
                                  editor.catalog.toolWheel.open ||
                                  editor.toolOptions.open ||
                                  editor.transform.controlsOpen;
  if (inventoryModalOpen) {
    return;
  }
  const std::int32_t centerX =
      region.x + static_cast<std::int32_t>(region.width / 2U);
  const std::int32_t centerY =
      region.y + static_cast<std::int32_t>(region.height / 2U);
  const CreativeEditorPlacementFeedback& feedback =
      editor.interaction.placementFeedback;
  const bool feedbackVisible =
      creativeEditorPlacementFeedbackVisible(feedback, editor.frameIndex);
  const bool placed =
      feedbackVisible &&
      feedback.status == CreativeEditorPlacementFeedbackStatus::Placed;
  const bool rejected =
      feedbackVisible &&
      feedback.status == CreativeEditorPlacementFeedbackStatus::Rejected;
  const float crosshairR = rejected ? 1.0F : placed ? 0.25F : 0.95F;
  const float crosshairG = rejected ? 0.18F : placed ? 1.0F : 0.95F;
  const float crosshairB = rejected ? 0.14F : placed ? 0.35F : 0.95F;
  uiRects.push_back({centerX - 8, centerY - 1, 17, 3, crosshairR, crosshairG,
                     crosshairB, 0.92F});
  uiRects.push_back({centerX - 1, centerY - 8, 3, 17, crosshairR, crosshairG,
                     crosshairB, 0.92F});
  if (feedbackVisible) {
    uiRects.push_back({centerX - 13, centerY - 13, 11, 3, crosshairR,
                       crosshairG, crosshairB, 0.94F});
    uiRects.push_back({centerX + 3, centerY - 13, 11, 3, crosshairR, crosshairG,
                       crosshairB, 0.94F});
    uiRects.push_back({centerX - 13, centerY + 11, 11, 3, crosshairR,
                       crosshairG, crosshairB, 0.94F});
    uiRects.push_back({centerX + 3, centerY + 11, 11, 3, crosshairR, crosshairG,
                       crosshairB, 0.94F});
  }
}

void appendCreativeEditorInteractionOverlay(
    const CreativeEditorState& editor,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    float wireThickness,
    std::vector<iggy3d::RenderUiRect>& uiRects,
    std::vector<iggy3d::DebugHudGlyphQuad>& glyphs,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines) {
  if (drawableWidth == 0U || drawableHeight == 0U) {
    return;
  }

  // The crosshair is emitted separately (appendCreativeEditorCrosshairOverlay)
  // so it survives when the legacy HUD is suppressed on keyboard/mouse (DD-15).
  const bool inventoryModalOpen = editor.catalog.model.open ||
                                  editor.catalog.toolWheel.open ||
                                  editor.toolOptions.open ||
                                  editor.transform.controlsOpen;

  constexpr std::int32_t slotSize = 44;
  constexpr std::int32_t gap = 4;
  constexpr std::int32_t totalWidth =
      static_cast<std::int32_t>(cr::kCreativeHotbarSlotCount) * slotSize +
      static_cast<std::int32_t>(cr::kCreativeHotbarSlotCount - 1U) * gap;
  const std::int32_t hotbarX =
      std::max(4, (static_cast<std::int32_t>(drawableWidth) - totalWidth) / 2);
  const std::int32_t hotbarY =
      std::max(4, static_cast<std::int32_t>(drawableHeight) - slotSize - 12);
  for (std::size_t slot = 0; slot < cr::kCreativeHotbarSlotCount; ++slot) {
    const bool selected = editor.interaction.hotbar.selectedSlot == slot;
    const std::int32_t x =
        hotbarX + static_cast<std::int32_t>(slot) * (slotSize + gap);
    uiRects.push_back({x, hotbarY,
                       static_cast<std::uint32_t>(slotSize),
                       static_cast<std::uint32_t>(slotSize),
                       selected ? 0.92F : 0.08F,
                       selected ? 0.92F : 0.09F,
                       selected ? 0.88F : 0.11F,
                       selected ? 0.96F : 0.86F});
    char slotLabel[4];
    std::snprintf(slotLabel, sizeof(slotLabel), "%zu", slot + 1U);
    appendColoredText(glyphs, slotLabel, x + 4, hotbarY + 3,
                      drawableWidth, drawableHeight, selected);
    appendColoredText(glyphs,
                      hotbarLabel(editor.interaction.hotbar.entries[slot],
                                  editor.toolSettings,
                                  editor.interaction.materialBrushPresets,
                                  slot),
                      x + 4, hotbarY + 22, drawableWidth, drawableHeight,
                      selected);
  }

  if (!inventoryModalOpen && !editor.transform.active) {
    const std::string heldLabel = creativeEditorHeldItemStatusLabel(editor);
    const std::int32_t heldLabelX = std::max(
        4, static_cast<std::int32_t>(drawableWidth / 2U) -
               static_cast<std::int32_t>(heldLabel.size() * 6U));
    appendHeldItemStatusText(glyphs, heldLabel, heldLabelX, hotbarY - 22,
                             drawableWidth, drawableHeight);
  }

  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const cr::CreativeHeldItemFrameMode heldFrameMode =
      cr::describeCreativeHeldItem(held.kind).frameMode;
  if (!inventoryModalOpen && !editor.transform.active &&
      editor.interaction.target.grid.valid &&
      heldFrameMode != cr::CreativeHeldItemFrameMode::MaterialStroke) {
    const cr::CreativeBounds& bounds =
        editor.interaction.target.grid.targetCellBounds;
    const cr::CreativeCoreVec3Conversion boxMin =
        cr::creativeVec3ToCoreChecked(bounds.min);
    const cr::CreativeCoreVec3Conversion boxMax =
        cr::creativeVec3ToCoreChecked(bounds.max);
    if (boxMin.converted && boxMax.converted) {
      appendStandaloneWireframeBoxEdges(
          wireLines, boxMin.value, boxMax.value,
          iggy3d::RenderLineColor{0.96F, 0.96F, 0.96F, 1.0F},
          std::max(0.025F, wireThickness * 0.7F));
    }
  }
}

}  // namespace iggy3d_creative_app
