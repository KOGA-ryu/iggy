#include "EditorSurfaceExtrude.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <utility>

#include "EditorEdits.hpp"
#include "EditorInteraction.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/tools/Volume.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

[[nodiscard]] bool sameCell(cr::CreativeGridCoord3 lhs,
                            cr::CreativeGridCoord3 rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

void setRejectedFeedback(CreativeEditorState& editor,
                         cr::CreativeObjectKind material) noexcept {
  editor.interaction.placementFeedback = {
      CreativeEditorPlacementFeedbackStatus::Rejected,
      cr::kInvalidObjectId,
      material,
      editor.frameIndex,
  };
}

[[nodiscard]] cr::CreativeBounds mutationBounds(
    const cr::CreativeSurfaceExtrudePlan& plan,
    const cr::CreativeGridSettings& grid) noexcept {
  const cr::CreativeBounds minimum = cr::creativeVolumeCellBounds(
      plan.minMutationCell, grid.cellSizeMeters, grid.origin);
  const cr::CreativeBounds maximum = cr::creativeVolumeCellBounds(
      plan.maxMutationCell, grid.cellSizeMeters, grid.origin);
  return {minimum.min, maximum.max};
}

}  // namespace

bool creativeSurfaceFaceOffset(
    cr::CreativeVec3 faceNormal,
    cr::CreativeGridCoord3& output) noexcept {
  output = {};
  if (!cr::isFiniteCreativeVec3(faceNormal)) {
    return false;
  }
  const std::array magnitudes{std::fabs(faceNormal.x),
                              std::fabs(faceNormal.y),
                              std::fabs(faceNormal.z)};
  const auto largest =
      std::max_element(magnitudes.begin(), magnitudes.end());
  if (largest == magnitudes.end() || *largest <= 0.0) {
    return false;
  }
  const std::size_t axis =
      static_cast<std::size_t>(std::distance(magnitudes.begin(), largest));
  const std::array components{faceNormal.x, faceNormal.y, faceNormal.z};
  const std::int32_t sign = components[axis] < 0.0 ? -1 : 1;
  if (axis == 0U) output.x = sign;
  if (axis == 1U) output.y = sign;
  if (axis == 2U) output.z = sign;
  return true;
}

const cr::CreativeSurfaceExtrudePlan& resolveCreativeEditorSurfaceExtrudePlan(
    CreativeEditorSurfaceExtrudeCache& cache,
    const cr::CreativeDocument& document,
    cr::CreativeGridCoord3 seedCell,
    cr::CreativeGridCoord3 outward,
    cr::CreativeSurfaceExtrudeKind kind,
    cr::CreativeSurfaceExtrudeDepth depth,
    cr::CreativeConnectedFillLimit affectedCellLimit) noexcept {
  if (cache.valid && cache.documentId == document.id() &&
      cache.documentRevision == document.revision() &&
      sameCell(cache.seedCell, seedCell) && sameCell(cache.outward, outward) &&
      cache.kind == kind && cache.depth == depth &&
      cache.affectedCellLimit == affectedCellLimit) {
    return cache.plan;
  }

  cache.plan = cr::planCreativeSurfaceExtrude(
      {&document.voxelField(), seedCell, outward, kind, depth,
       affectedCellLimit});
  cache.documentId = document.id();
  cache.documentRevision = document.revision();
  cache.seedCell = seedCell;
  cache.outward = outward;
  cache.kind = kind;
  cache.depth = depth;
  cache.affectedCellLimit = affectedCellLimit;
  ++cache.refreshCount;
  cache.valid = true;
  return cache.plan;
}

void invalidateCreativeEditorSurfaceExtrudeCache(
    CreativeEditorSurfaceExtrudeCache& cache) noexcept {
  cache.plan = {};
  cache.documentId = cr::kInvalidDocumentId;
  cache.documentRevision = 0U;
  cache.seedCell = {};
  cache.outward = {};
  cache.valid = false;
}

CreativeEditorSurfaceExtrudeReceipt
applyCreativeEditorSurfaceExtrudeWithHistory(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    cr::CreativeSurfaceExtrudeKind kind,
    std::string_view source) {
  CreativeEditorSurfaceExtrudeReceipt receipt;
  receipt.requested = true;
  receipt.kind = kind;

  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const CreativeEditorWorldTarget& target = editor.interaction.target;
  cr::CreativeGridCoord3 outward{};
  if (held.kind != cr::CreativeHeldItemKind::SurfaceExtrude ||
      !target.voxelHit ||
      !creativeSurfaceFaceOffset(target.grid.faceNormal, outward) ||
      kind >= cr::CreativeSurfaceExtrudeKind::Count) {
    receipt.reasonCode = "creative_surface_extrude_target_unavailable";
    setRejectedFeedback(editor, held.objectKind);
    return receipt;
  }

  const cr::CreativeSurfaceExtrudePlan& plan =
      resolveCreativeEditorSurfaceExtrudePlan(
          editor.interaction.surfaceExtrude, appState.facade.document(),
          target.voxelCell, outward, kind,
          editor.toolSettings.surfaceExtrudeDepth,
          editor.toolSettings.surfaceExtrudeLimit);
  receipt.planStatus = plan.status;
  receipt.surfaceCellCount = plan.surfaceCellCount;
  receipt.plannedCellCount = plan.mutationCellCount;
  if (!plan.accepted) {
    receipt.reasonCode = plan.reasonCode;
    setRejectedFeedback(editor, held.objectKind);
    return receipt;
  }

  const cr::CreativeObjectKind replacement =
      kind == cr::CreativeSurfaceExtrudeKind::Extrude
          ? held.objectKind
          : cr::CreativeObjectKind::Unknown;
  if (kind == cr::CreativeSurfaceExtrudeKind::Extrude &&
      !cr::creativeVolumeBrushSupported(replacement)) {
    receipt.reasonCode = "creative_surface_extrude_material_invalid";
    setRejectedFeedback(editor, replacement);
    return receipt;
  }

  std::array<cr::CreativeVoxelEdit, cr::kCreativeSurfaceExtrudeCapacity>
      edits{};
  for (std::size_t index = 0U; index < plan.mutationCellCount; ++index) {
    edits[index] = {plan.mutationCells[index], replacement};
  }

  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  const cr::CreativeVoxelMutationReceipt mutation =
      appState.facade.applyVoxelEdits(
          {edits.data(), plan.mutationCellCount});
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

  CreativeEditorPlacementFeedback feedback;
  feedback.status = CreativeEditorPlacementFeedbackStatus::Placed;
  feedback.objectKind = replacement == cr::CreativeObjectKind::Unknown
                            ? plan.sourceMaterial
                            : replacement;
  feedback.frameIndex = editor.frameIndex;
  feedback.voxelPlaced = true;
  feedback.voxelCell = plan.seedCell;
  feedback.voxelBounds =
      mutationBounds(plan, appState.facade.document().gridSettings());
  editor.interaction.placementFeedback = feedback;
  editor.interaction.surfaceExtrude.valid = false;
  return receipt;
}

}  // namespace iggy3d_creative_app
