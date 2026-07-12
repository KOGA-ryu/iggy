#include "EditorConnectedFill.hpp"

#include <array>
#include <utility>

#include "EditorEdits.hpp"
#include "EditorInteraction.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/tools/Volume.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

void setRejectedFeedback(CreativeEditorState& editor,
                         cr::CreativeObjectKind material) noexcept {
  setCreativeEditorPlacementFeedback(
      editor.interaction, CreativeEditorPlacementFeedbackStatus::Rejected,
      editor.frameIndex, material);
}

[[nodiscard]] cr::CreativeBounds connectedFillBounds(
    const cr::CreativeConnectedFillPlan& plan,
    const cr::CreativeGridSettings& grid) noexcept {
  const cr::CreativeBounds minimum = cr::creativeVolumeCellBounds(
      plan.minCell, grid.cellSizeMeters, grid.origin);
  const cr::CreativeBounds maximum = cr::creativeVolumeCellBounds(
      plan.maxCell, grid.cellSizeMeters, grid.origin);
  return {minimum.min, maximum.max};
}

}  // namespace

const cr::CreativeConnectedFillPlan& resolveCreativeEditorConnectedFillPlan(
    CreativeEditorConnectedFillCache& cache,
    const cr::CreativeDocument& document,
    cr::CreativeGridCoord3 seedCell,
    cr::CreativeConnectedFillLimit limit) noexcept {
  if (cache.valid && cache.documentId == document.id() &&
      cache.documentRevision == document.revision() &&
      cache.seedCell == seedCell && cache.limit == limit) {
    return cache.plan;
  }

  cache.plan = cr::planCreativeConnectedFill(
      {&document.voxelField(), seedCell, limit});
  cache.documentId = document.id();
  cache.documentRevision = document.revision();
  cache.seedCell = seedCell;
  cache.limit = limit;
  ++cache.refreshCount;
  cache.valid = true;
  return cache.plan;
}

void invalidateCreativeEditorConnectedFillCache(
    CreativeEditorConnectedFillCache& cache) noexcept {
  cache.plan = {};
  cache.documentId = cr::kInvalidDocumentId;
  cache.documentRevision = 0U;
  cache.seedCell = {};
  cache.valid = false;
}

CreativeEditorConnectedFillReceipt
applyCreativeEditorConnectedFillWithHistory(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    CreativeConnectedFillEditKind editKind,
    std::string_view source) {
  CreativeEditorConnectedFillReceipt receipt;
  receipt.requested = true;
  receipt.editKind = editKind;

  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const CreativeEditorWorldTarget& target = editor.interaction.target;
  if (held.kind != cr::CreativeHeldItemKind::ConnectedFill ||
      !target.voxelHit ||
      editKind >= CreativeConnectedFillEditKind::Count) {
    receipt.reasonCode = "creative_connected_fill_target_unavailable";
    setRejectedFeedback(editor, held.objectKind);
    return receipt;
  }

  const cr::CreativeConnectedFillPlan& plan =
      resolveCreativeEditorConnectedFillPlan(
          editor.interaction.connectedFill, appState.facade.document(),
          target.voxelCell, editor.toolSettings.connectedFillLimit);
  receipt.planStatus = plan.status;
  receipt.plannedCellCount = plan.cellCount;
  if (!plan.accepted) {
    receipt.reasonCode = plan.reasonCode;
    setRejectedFeedback(editor, held.objectKind);
    return receipt;
  }

  const cr::CreativeObjectKind replacement =
      editKind == CreativeConnectedFillEditKind::Erase
          ? cr::CreativeObjectKind::Unknown
          : held.objectKind;
  if ((editKind == CreativeConnectedFillEditKind::Paint &&
       !cr::creativeVolumeBrushSupported(replacement)) ||
      replacement == plan.sourceMaterial) {
    receipt.reasonCode = replacement == plan.sourceMaterial
                             ? "creative_connected_fill_no_change"
                             : "creative_connected_fill_material_invalid";
    setRejectedFeedback(editor, replacement);
    return receipt;
  }

  std::array<cr::CreativeVoxelEdit, cr::kCreativeConnectedFillCapacity> edits{};
  for (std::size_t index = 0U; index < plan.cellCount; ++index) {
    edits[index] = {plan.cells[index], replacement};
  }

  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  const cr::CreativeVoxelMutationReceipt mutation =
      appState.facade.applyVoxelEdits({edits.data(), plan.cellCount});
  static_cast<void>(completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      mutation.accepted && mutation.changed, mutation.reasonCode));

  receipt.accepted = mutation.accepted;
  receipt.changed = mutation.changed;
  receipt.mutationStatus = mutation.status;
  receipt.changedCellCount = mutation.changedCellCount;
  receipt.reasonCode = mutation.reasonCode;
  if (!receipt.accepted || !receipt.changed) {
    setRejectedFeedback(editor, replacement);
    return receipt;
  }

  setCreativeEditorVoxelPlacementFeedback(
      editor.interaction, editor.frameIndex,
      replacement == cr::CreativeObjectKind::Unknown ? plan.sourceMaterial
                                                     : replacement,
      plan.seedCell,
      connectedFillBounds(plan, appState.facade.document().gridSettings()));
  editor.interaction.connectedFill.valid = false;
  return receipt;
}

}  // namespace iggy3d_creative_app
