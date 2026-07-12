#include "EditorTerrain.hpp"

#include "EditorEdits.hpp"
#include "EditorInteraction.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/document/Document.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

void invalidateStampPreview(
    CreativeTerrainStampPlacementState& stamp) noexcept {
  stamp.preview.valid = false;
  stamp.preview.renderAccepted = false;
  stamp.preview.patches.clear();
}

void setStampFeedback(CreativeEditorState& editor, bool accepted) noexcept {
  setCreativeEditorPlacementFeedback(
      editor.interaction,
      accepted ? CreativeEditorPlacementFeedbackStatus::Placed
               : CreativeEditorPlacementFeedbackStatus::Rejected,
      editor.frameIndex);
}

[[nodiscard]] bool selectedRegionBounds(
    const CreativeEditorState& editor,
    cr::CreativeTerrainCoord2& minimumCoord,
    cr::CreativeTerrainCoord2& maximumCoord) noexcept {
  const cr::CreativeVolumeSelection& selection = editor.volume.selection;
  if (!cr::creativeVolumeSelectionValid(selection)) {
    return false;
  }
  minimumCoord = {
      std::min(selection.firstCell.x, selection.secondCell.x),
      std::min(selection.firstCell.z, selection.secondCell.z)};
  maximumCoord = {
      std::max(selection.firstCell.x, selection.secondCell.x),
      std::max(selection.firstCell.z, selection.secondCell.z)};
  return true;
}

[[nodiscard]] cr::CreativeTerrainStampRequest stampRequest(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    const cr::CreativeTerrainStamp& stamp,
    cr::CreativeTerrainCoord2 targetMinimum) noexcept {
  cr::CreativeTerrainStampRequest request;
  request.stamp = &stamp;
  request.destinationControls = document.terrainField().controls();
  request.targetMinimum = targetMinimum;
  request.quarterTurns = editor.terrain.region.stamp.quarterTurns;
  request.mirrorX = editor.terrain.region.stamp.mirrorX;
  request.mirrorZ = editor.terrain.region.stamp.mirrorZ;
  request.mode = editor.toolSettings.terrainStampMode;
  request.elevationMode = editor.toolSettings.terrainStampElevationMode;
  const cr::CreativeTerrainHeightSample targetSurface =
      cr::sampleCreativeTerrainHeight(document.terrainField(), targetMinimum);
  request.targetSurfacePresent = targetSurface.present;
  request.targetSurfaceHeightCells =
      targetSurface.present ? targetSurface.heightCells : 0U;
  request.manualHeightOffsetCells =
      editor.terrain.region.stamp.heightOffsetCells;
  return request;
}

[[nodiscard]] bool previewKeyMatches(
    const CreativeTerrainStampPreviewCache& cache,
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    const cr::CreativeTerrainStamp& stamp,
    const cr::CreativeTerrainStampRequest& request) noexcept {
  const CreativeTerrainStampPlacementState& state = editor.terrain.region.stamp;
  return cache.valid && cache.documentId == document.id() &&
         cache.terrainRevision == document.terrainField().revision() &&
         cache.stampSignature == stamp.contentSignature &&
         cache.targetMinimum == request.targetMinimum &&
         cache.quarterTurns == state.quarterTurns &&
         cache.mirrorX == state.mirrorX && cache.mirrorZ == state.mirrorZ &&
         cache.mode == request.mode &&
         cache.elevationMode == request.elevationMode &&
         cache.targetSurfacePresent == request.targetSurfacePresent &&
         cache.targetSurfaceHeightCells == request.targetSurfaceHeightCells &&
         cache.manualHeightOffsetCells == request.manualHeightOffsetCells;
}

[[nodiscard]] std::string_view transformControlLabel(
    CreativeTerrainStampTransformControl control) noexcept {
  switch (control) {
    case CreativeTerrainStampTransformControl::Rotation: return "ROTATION";
    case CreativeTerrainStampTransformControl::MirrorX: return "MIRROR X";
    case CreativeTerrainStampTransformControl::MirrorZ: return "MIRROR Z";
    case CreativeTerrainStampTransformControl::HeightOffset: return "HEIGHT";
    case CreativeTerrainStampTransformControl::Count: break;
  }
  return "INVALID";
}

[[nodiscard]] bool cycleTransformControl(
    CreativeTerrainStampTransformControl& control,
    int direction) noexcept {
  const int count =
      static_cast<int>(CreativeTerrainStampTransformControl::Count);
  const int before = static_cast<int>(control);
  const int after = (before + (direction > 0 ? 1 : count - 1)) % count;
  control = static_cast<CreativeTerrainStampTransformControl>(after);
  return after != before;
}

}  // namespace

CreativeEditorTerrainStampReceipt copyCreativeEditorTerrainRegionToStamp(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor) noexcept {
  CreativeEditorTerrainStampReceipt receipt;
  receipt.requested = true;
  receipt.action = CreativeEditorTerrainStampAction::Copy;
  cr::CreativeTerrainCoord2 minimumCoord{};
  cr::CreativeTerrainCoord2 maximumCoord{};
  if (!selectedRegionBounds(editor, minimumCoord, maximumCoord)) {
    receipt.reasonCode = "creative_editor_terrain_stamp_selection_invalid";
    setStampFeedback(editor, false);
    return receipt;
  }
  const cr::CreativeDocument& document = appState.facade.document();
  receipt.copy = cr::copyCreativeTerrainRegionToStamp(
      document.id(), document.revision(), document.terrainField().controls(),
      minimumCoord, maximumCoord, appState.terrainStamp);
  receipt.accepted = receipt.copy.accepted;
  receipt.changed = receipt.copy.accepted;
  receipt.reasonCode = receipt.copy.reasonCode;
  editor.terrain.region.stamp.lastCopy = receipt.copy;
  setStampFeedback(editor, receipt.accepted);
  return receipt;
}

CreativeEditorTerrainStampReceipt beginCreativeEditorTerrainStampPreview(
    const cr::CreativeAppState& appState,
    CreativeEditorState& editor) noexcept {
  CreativeEditorTerrainStampReceipt receipt;
  receipt.requested = true;
  receipt.action = CreativeEditorTerrainStampAction::BeginPreview;
  if (!cr::isValidCreativeTerrainStamp(appState.terrainStamp)) {
    receipt.reasonCode = "creative_editor_terrain_stamp_clipboard_invalid";
    setStampFeedback(editor, false);
    return receipt;
  }
  CreativeTerrainStampPlacementState& state = editor.terrain.region.stamp;
  const cr::CreativeTerrainStampCopyReceipt lastCopy = state.lastCopy;
  state = {};
  state.active = true;
  state.lastCopy = lastCopy;
  clearCreativeEditorPlacementFeedback(editor.interaction);
  receipt.accepted = true;
  receipt.changed = true;
  receipt.reasonCode = "creative_editor_terrain_stamp_preview_began";
  return receipt;
}

CreativeEditorTerrainStampReceipt applyCreativeEditorTerrainStampWithHistory(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source) {
  CreativeEditorTerrainStampReceipt receipt;
  receipt.requested = true;
  receipt.action = CreativeEditorTerrainStampAction::Apply;
  CreativeTerrainStampPlacementState& state = editor.terrain.region.stamp;
  if (!state.active ||
      !cr::isValidCreativeTerrainStamp(appState.terrainStamp)) {
    receipt.reasonCode = "creative_editor_terrain_stamp_preview_inactive";
    setStampFeedback(editor, false);
    return receipt;
  }
  static_cast<void>(refreshCreativeEditorTerrainStampPreview(
      editor.terrain, appState.facade.document(), editor,
      appState.terrainStamp));
  receipt.plan = state.preview.plan;
  receipt.accepted = state.preview.valid && receipt.plan.accepted;
  receipt.reasonCode = state.preview.valid
                           ? receipt.plan.reasonCode
                           : "creative_editor_terrain_stamp_target_invalid";
  if (!receipt.accepted || receipt.plan.items().empty()) {
    setStampFeedback(editor, receipt.accepted);
    return receipt;
  }

  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  receipt.mutation = appState.facade.applyTerrainControlEdits(
      receipt.plan.items());
  state.lastMutation = receipt.mutation;
  editor.terrain.lastMutation = receipt.mutation;
  receipt.accepted = receipt.mutation.accepted;
  receipt.changed = receipt.mutation.changed;
  receipt.reasonCode = receipt.mutation.reasonCode;
  static_cast<void>(completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      receipt.mutation.accepted && receipt.mutation.changed,
      receipt.mutation.reasonCode));
  if (receipt.changed) {
    invalidateStampPreview(state);
  }
  setStampFeedback(editor, receipt.accepted);
  return receipt;
}

CreativeEditorTerrainStampReceipt cancelCreativeEditorTerrainStamp(
    CreativeEditorState& editor) noexcept {
  CreativeEditorTerrainStampReceipt receipt;
  receipt.requested = true;
  receipt.action = CreativeEditorTerrainStampAction::Cancel;
  CreativeTerrainStampPlacementState& state = editor.terrain.region.stamp;
  receipt.changed = state.active;
  receipt.accepted = true;
  state.active = false;
  invalidateStampPreview(state);
  clearCreativeEditorPlacementFeedback(editor.interaction);
  receipt.reasonCode = receipt.changed
                           ? "creative_editor_terrain_stamp_cancelled"
                           : "creative_editor_terrain_stamp_already_inactive";
  return receipt;
}

bool processCreativeEditorTerrainStampQuickEdit(
    CreativeEditorState& editor,
    cr::CreativeInputActionId action) noexcept {
  CreativeTerrainStampPlacementState& state = editor.terrain.region.stamp;
  if (!state.active) {
    return false;
  }
  bool changed = false;
  switch (action) {
    case cr::CreativeInputActionId::QuickEditPrevious:
      changed = cycleTransformControl(state.selectedControl, -1);
      break;
    case cr::CreativeInputActionId::QuickEditNext:
      changed = cycleTransformControl(state.selectedControl, 1);
      break;
    case cr::CreativeInputActionId::QuickEditDecrease:
    case cr::CreativeInputActionId::QuickEditIncrease: {
      const int direction =
          action == cr::CreativeInputActionId::QuickEditIncrease ? 1 : -1;
      switch (state.selectedControl) {
        case CreativeTerrainStampTransformControl::Rotation:
          state.quarterTurns = static_cast<std::uint8_t>(
              (state.quarterTurns + (direction > 0 ? 1U : 3U)) % 4U);
          changed = true;
          break;
        case CreativeTerrainStampTransformControl::MirrorX:
          state.mirrorX = !state.mirrorX;
          changed = true;
          break;
        case CreativeTerrainStampTransformControl::MirrorZ:
          state.mirrorZ = !state.mirrorZ;
          changed = true;
          break;
        case CreativeTerrainStampTransformControl::HeightOffset: {
          const std::int32_t next =
              static_cast<std::int32_t>(state.heightOffsetCells) + direction;
          if (next >= cr::kCreativeTerrainStampMinimumHeightOffsetCells &&
              next <= cr::kCreativeTerrainStampMaximumHeightOffsetCells) {
            state.heightOffsetCells = static_cast<std::int16_t>(next);
            changed = true;
          }
          break;
        }
        case CreativeTerrainStampTransformControl::Count: return false;
      }
      break;
    }
    default:
      return false;
  }
  if (changed) {
    invalidateStampPreview(state);
  }
  return changed;
}

std::string creativeEditorTerrainStampQuickEditLabel(
    const CreativeEditorState& editor) {
  const CreativeTerrainStampPlacementState& state =
      editor.terrain.region.stamp;
  std::string output("STAMP ");
  output.append(cr::toString(editor.toolSettings.terrainStampMode));
  output.append(" | ");
  output.append(cr::toString(editor.toolSettings.terrainStampElevationMode));
  output.append(" | ");
  output.append(transformControlLabel(state.selectedControl));
  output.append(" | ROT ");
  output.append(std::to_string(static_cast<unsigned>(state.quarterTurns) * 90U));
  output.append(" | MX ");
  output.append(state.mirrorX ? "ON" : "OFF");
  output.append(" | MZ ");
  output.append(state.mirrorZ ? "ON" : "OFF");
  output.append(" | Y ");
  if (state.heightOffsetCells >= 0) {
    output.push_back('+');
  }
  output.append(std::to_string(state.heightOffsetCells));
  return output;
}

bool refreshCreativeEditorTerrainStampPreview(
    CreativeEditorTerrainState& terrain,
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    const cr::CreativeTerrainStamp& stamp) {
  CreativeTerrainStampPlacementState& state = terrain.region.stamp;
  if (!state.active || !cr::isValidCreativeTerrainStamp(stamp)) {
    invalidateStampPreview(state);
    return false;
  }
  cr::CreativeTerrainCoord2 targetMinimum{};
  if (!resolveCreativeEditorTerrainPointerCoord(editor, targetMinimum)) {
    invalidateStampPreview(state);
    return false;
  }
  const cr::CreativeTerrainStampRequest request =
      stampRequest(document, editor, stamp, targetMinimum);
  CreativeTerrainStampPreviewCache& cache = state.preview;
  if (previewKeyMatches(cache, document, editor, stamp, request)) {
    return false;
  }

  const std::uint64_t nextBuildCount = cache.buildCount + 1U;
  cache = {};
  cache.valid = true;
  cache.documentId = document.id();
  cache.terrainRevision = document.terrainField().revision();
  cache.stampSignature = stamp.contentSignature;
  cache.buildCount = nextBuildCount;
  cache.targetMinimum = targetMinimum;
  cache.quarterTurns = state.quarterTurns;
  cache.mirrorX = state.mirrorX;
  cache.mirrorZ = state.mirrorZ;
  cache.mode = request.mode;
  cache.elevationMode = request.elevationMode;
  cache.targetSurfacePresent = request.targetSurfacePresent;
  cache.targetSurfaceHeightCells = request.targetSurfaceHeightCells;
  cache.manualHeightOffsetCells = request.manualHeightOffsetCells;
  cache.plan = cr::buildCreativeTerrainStampPlan(request);
  if (!cache.plan.accepted) {
    return true;
  }

  const cr::CreativeGridSettings grid = document.gridSettings();
  const cr::CreativeTerrainMutationPreviewReceipt preview =
      cr::buildCreativeTerrainMutationPreview(
          document.terrainField(), cache.plan.items(), grid.origin,
          grid.cellSizeMeters);
  if (!preview.accepted) {
    return true;
  }

  std::uint16_t expansion = 1U;
  for (const cr::CreativeTerrainControlPoint& control : cache.plan.controls()) {
    expansion = std::max(expansion, control.radiusCells);
  }
  for (const cr::CreativeTerrainControlPoint& control :
       document.terrainField().controls()) {
    if (control.coord.x >= cache.plan.targetMinimum.x &&
        control.coord.x <= cache.plan.targetMaximum.x &&
        control.coord.z >= cache.plan.targetMinimum.z &&
        control.coord.z <= cache.plan.targetMaximum.z) {
      expansion = std::max(expansion, control.radiusCells);
    }
  }
  const std::int64_t minimumX =
      static_cast<std::int64_t>(cache.plan.targetMinimum.x) - expansion;
  const std::int64_t minimumZ =
      static_cast<std::int64_t>(cache.plan.targetMinimum.z) - expansion;
  const std::int64_t maximumX =
      static_cast<std::int64_t>(cache.plan.targetMaximum.x) + expansion;
  const std::int64_t maximumZ =
      static_cast<std::int64_t>(cache.plan.targetMaximum.z) + expansion;
  for (const cr::CreativeTerrainSurfacePatch& patch : preview.render.patches) {
    if (patch.coord.x >= minimumX && patch.coord.x <= maximumX &&
        patch.coord.z >= minimumZ && patch.coord.z <= maximumZ) {
      cache.patches.push_back(patch);
    }
  }
  cache.renderAccepted = true;
  return true;
}

void appendCreativeEditorTerrainStampOverlay(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines) {
  const CreativeTerrainStampPreviewCache& preview =
      editor.terrain.region.stamp.preview;
  if (!preview.valid) {
    return;
  }
  constexpr iggy3d::RenderLineColor admitted{0.20F, 1.0F, 0.35F, 1.0F};
  constexpr iggy3d::RenderLineColor removing{1.0F, 0.46F, 0.12F, 1.0F};
  constexpr iggy3d::RenderLineColor rejected{1.0F, 0.20F, 0.18F, 1.0F};
  const bool accepted = preview.plan.accepted && preview.renderAccepted;
  const cr::CreativeGridSettings grid = document.gridSettings();
  for (const cr::CreativeTerrainControlPoint& control :
       preview.plan.controls()) {
    appendCreativeEditorTerrainControlGuide(
        wireLines, grid, control, accepted ? admitted : rejected,
        wireThickness * 1.5F);
  }
  if (accepted && preview.mode == cr::CreativeTerrainStampMode::Replace) {
    for (const cr::CreativeTerrainControlEdit& edit : preview.plan.items()) {
      if (edit.kind == cr::CreativeTerrainEditKind::Remove) {
        appendCreativeEditorTerrainControlGuide(
            wireLines, grid, edit.control, removing, wireThickness * 1.5F);
      }
    }
  }
  if (!accepted) {
    return;
  }
  for (const cr::CreativeTerrainSurfacePatch& patch : preview.patches) {
    appendCreativeEditorTerrainPatchSlopeTriangles(
        wireLines, patch, wireThickness * 0.7F);
  }
}

}  // namespace iggy3d_creative_app
