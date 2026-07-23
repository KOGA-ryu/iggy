#include "EditorAssetReplacement.hpp"

#include "app/iggy3d/creative/document/Hierarchy.hpp"

#include <SDL3/SDL_log.h>

#include <algorithm>
#include <cstdio>
#include <utility>

#include "EditorAssets.hpp"
#include "EditorEdits.hpp"
#include "EditorPreviewProxies.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "render/debug/DebugHudText.hpp"

namespace iggy3d_creative_app {
namespace {

[[nodiscard]] bool replacementObjectKind(
    cr::CreativeObjectKind kind) noexcept {
  return kind == cr::CreativeObjectKind::Prop ||
         kind == cr::CreativeObjectKind::Rock ||
         kind == cr::CreativeObjectKind::Bridge;
}

void rejectPlan(CreativeAssetReplacementPlan& plan,
                CreativeAssetReplacementStatus status,
                std::string_view reasonCode) noexcept {
  plan.accepted = false;
  plan.status = status;
  plan.reasonCode = reasonCode;
}

[[nodiscard]] bool validCatalogBounds(
    const iggy3d::StaticMeshAssetCatalogEntry& entry) noexcept {
  const cr::CreativeBounds bounds{
      {entry.boundsMin.x, entry.boundsMin.y, entry.boundsMin.z},
      {entry.boundsMax.x, entry.boundsMax.y, entry.boundsMax.z}};
  const cr::CreativeBoundsMetrics metrics = cr::measureCreativeBounds(bounds);
  return metrics.valid && cr::isPositiveCreativeVec3(metrics.size);
}

void clearPreview(CreativeEditorAssetReplacementState& state) {
  state.active = false;
  state.plan = {};
  state.previewDocument = {};
}

void appendText(std::vector<iggy3d::DebugHudGlyphQuad>& glyphs,
                std::string_view text,
                std::int32_t x,
                std::int32_t y,
                std::uint32_t drawableWidth,
                std::uint32_t drawableHeight,
                float r,
                float g,
                float b) {
  iggy3d::DebugHudLayoutResult layout = iggy3d::layoutDebugHudTextAt(
      text, x, y, drawableWidth, drawableHeight);
  for (iggy3d::DebugHudGlyphQuad& quad : layout.quads) {
    quad.r = r;
    quad.g = g;
    quad.b = b;
    quad.a = 1.0F;
  }
  glyphs.insert(glyphs.end(), layout.quads.begin(), layout.quads.end());
}

}  // namespace

std::string_view toString(CreativeAssetReplacementStatus status) noexcept {
  switch (status) {
    case CreativeAssetReplacementStatus::NotRequested: return "NotRequested";
    case CreativeAssetReplacementStatus::Ready: return "Ready";
    case CreativeAssetReplacementStatus::InvalidDocument:
      return "InvalidDocument";
    case CreativeAssetReplacementStatus::InvalidSelection:
      return "InvalidSelection";
    case CreativeAssetReplacementStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeAssetReplacementStatus::InvalidTarget: return "InvalidTarget";
    case CreativeAssetReplacementStatus::UnsupportedObject:
      return "UnsupportedObject";
    case CreativeAssetReplacementStatus::MissingSourceAsset:
      return "MissingSourceAsset";
    case CreativeAssetReplacementStatus::LockedObject: return "LockedObject";
    case CreativeAssetReplacementStatus::CustomBounds: return "CustomBounds";
    case CreativeAssetReplacementStatus::NoChange: return "NoChange";
    case CreativeAssetReplacementStatus::StaleDocument: return "StaleDocument";
    case CreativeAssetReplacementStatus::MutationRejected:
      return "MutationRejected";
    case CreativeAssetReplacementStatus::Applied: return "Applied";
    case CreativeAssetReplacementStatus::Cancelled: return "Cancelled";
  }
  return "Unknown";
}

CreativeAssetReplacementPlan planCreativeAssetReplacement(
    const cr::CreativeDocument& document,
    std::span<const cr::CreativeObjectId> selectedObjectIds,
    const iggy3d::StaticMeshAssetCatalog& catalog,
    cr::CreativeObjectKind targetObjectKind,
    std::string_view targetAssetId) {
  CreativeAssetReplacementPlan plan;
  plan.sourceDocumentId = document.id();
  plan.sourceRevision = document.revision();
  plan.targetObjectKind = targetObjectKind;
  plan.targetAssetId = targetAssetId;
  if (!document.isValid() || document.id() == cr::kInvalidDocumentId) {
    rejectPlan(plan, CreativeAssetReplacementStatus::InvalidDocument,
               "creative_asset_replace_document_invalid");
    return plan;
  }
  if (selectedObjectIds.empty()) {
    rejectPlan(plan, CreativeAssetReplacementStatus::InvalidSelection,
               "creative_asset_replace_selection_empty");
    return plan;
  }
  if (selectedObjectIds.size() > kCreativeAssetReplacementCapacity) {
    rejectPlan(plan, CreativeAssetReplacementStatus::CapacityExceeded,
               "creative_asset_replace_selection_capacity_exceeded");
    return plan;
  }
  const iggy3d::StaticMeshAssetCatalogEntry* target =
      catalog.find(targetAssetId);
  if (!replacementObjectKind(targetObjectKind) ||
      !iggy3d::validStaticMeshAssetId(targetAssetId) || target == nullptr ||
      !validCatalogBounds(*target)) {
    rejectPlan(plan, CreativeAssetReplacementStatus::InvalidTarget,
               "creative_asset_replace_target_invalid");
    return plan;
  }

  plan.objectIds.reserve(selectedObjectIds.size());
  plan.mutations.reserve(selectedObjectIds.size());
  for (cr::CreativeObjectId objectId : selectedObjectIds) {
    if (objectId == cr::kInvalidObjectId ||
        std::find(plan.objectIds.begin(), plan.objectIds.end(), objectId) !=
            plan.objectIds.end()) {
      rejectPlan(plan, CreativeAssetReplacementStatus::InvalidSelection,
                 "creative_asset_replace_selection_invalid");
      return plan;
    }
    const cr::CreativeObject* object = document.findObject(objectId);
    if (object == nullptr) {
      rejectPlan(plan, CreativeAssetReplacementStatus::InvalidSelection,
                 "creative_asset_replace_selection_missing_object");
      return plan;
    }
    if (!replacementObjectKind(object->kind) || object->assetId.empty()) {
      rejectPlan(plan, CreativeAssetReplacementStatus::UnsupportedObject,
                 "creative_asset_replace_object_unsupported");
      return plan;
    }
    if (cr::creativeObjectEffectivelyLocked(document, object->id)) {
      rejectPlan(plan, CreativeAssetReplacementStatus::LockedObject,
                 "creative_asset_replace_object_locked");
      return plan;
    }
    const iggy3d::StaticMeshAssetCatalogEntry* source =
        catalog.find(object->assetId);
    if (!iggy3d::validStaticMeshAssetId(object->assetId) || source == nullptr ||
        !validCatalogBounds(*source)) {
      rejectPlan(plan, CreativeAssetReplacementStatus::MissingSourceAsset,
                 "creative_asset_replace_source_missing");
      return plan;
    }
    plan.objectIds.push_back(objectId);
    const cr::CreativeBounds naturalSource =
        creativeAssetBoundsAtPivot(*source, object->transform.position);
    const bool hasCustomBounds =
        !cr::creativeBoundsExactlyEqual(object->bounds, naturalSource);
    const cr::CreativeBounds replacementBounds =
        hasCustomBounds
            ? object->bounds
            : creativeAssetBoundsAtPivot(*target, object->transform.position);
    const bool variantRetained =
        object->assetMaterialVariant.empty() ||
        iggy3d::findStaticMeshMaterialVariantIndex(
            target->materialVariants, object->assetMaterialVariant)
            .has_value();
    const std::string replacementVariant =
        variantRetained ? object->assetMaterialVariant : std::string{};
    if (object->kind == targetObjectKind && object->assetId == targetAssetId &&
        object->assetContentHash == target->contentHash &&
        object->assetMaterialVariant == replacementVariant &&
        cr::creativeBoundsExactlyEqual(object->bounds, replacementBounds)) {
      continue;
    }
    cr::CreativeMutationRequest mutation;
    mutation.objectId = objectId;
    mutation.kind = cr::CreativeMutationKind::SetAsset;
    mutation.payload = cr::makeAssetPayload(
        targetObjectKind, std::string(targetAssetId), replacementBounds,
        target->contentHash, replacementVariant);
    plan.mutations.push_back(std::move(mutation));
  }
  if (plan.mutations.empty()) {
    rejectPlan(plan, CreativeAssetReplacementStatus::NoChange,
               "creative_asset_replace_no_change");
    return plan;
  }
  plan.accepted = true;
  plan.status = CreativeAssetReplacementStatus::Ready;
  plan.reasonCode = "creative_asset_replace_ready";
  return plan;
}

CreativeAssetReplacementBeginReceipt beginCreativeEditorAssetReplacement(
    const cr::CreativeAppState& appState,
    const iggy3d::StaticMeshAssetCatalog& catalog,
    cr::CreativeObjectKind targetObjectKind,
    std::string_view targetAssetId,
    CreativeEditorAssetReplacementState& state) {
  state = {};
  CreativeAssetReplacementBeginReceipt receipt;
  receipt.requested = true;
  std::vector<cr::CreativeObjectId> objectIds;
  const std::span<const cr::TargetRef> selected =
      cr::selectedTargetList(appState.facade.selectionState());
  objectIds.reserve(selected.size());
  for (cr::TargetRef target : selected) {
    objectIds.push_back(static_cast<cr::CreativeObjectId>(target.value));
  }
  state.plan = planCreativeAssetReplacement(
      appState.facade.document(), objectIds, catalog, targetObjectKind,
      targetAssetId);
  receipt.status = state.plan.status;
  receipt.objectCount = state.plan.objectIds.size();
  receipt.reasonCode = state.plan.reasonCode;
  if (!state.plan.accepted) {
    state.lastStatus = receipt.status;
    state.reasonCode = receipt.reasonCode;
    return receipt;
  }

  cr::CreativeDocument preview = appState.facade.document();
  const cr::CreativeDocumentBatchMutationReceipt mutation =
      cr::applyDocumentMutationsAtomically(preview, state.plan.mutations);
  if (!mutation.committed || !mutation.changed ||
      !cr::documentMutationSucceeded(mutation.status)) {
    state.plan.accepted = false;
    state.plan.status = CreativeAssetReplacementStatus::MutationRejected;
    state.plan.reasonCode = "creative_asset_replace_preview_rejected";
    state.lastStatus = state.plan.status;
    state.reasonCode = state.plan.reasonCode;
    receipt.status = state.plan.status;
    receipt.reasonCode = state.plan.reasonCode;
    return receipt;
  }

  state.active = true;
  state.previewDocument = std::move(preview);
  state.lastStatus = CreativeAssetReplacementStatus::Ready;
  state.reasonCode = "creative_asset_replace_ready";
  receipt.accepted = true;
  receipt.status = state.lastStatus;
  receipt.reasonCode = state.reasonCode;
  SDL_Log("iggy3d_creative: ASSET REPLACE preview asset='%s' objects=%zu",
          state.plan.targetAssetId.c_str(), state.plan.objectIds.size());
  return receipt;
}

CreativeAssetReplacementCommitReceipt commitCreativeEditorAssetReplacement(
    cr::CreativeAppState& appState,
    CreativeEditorAssetReplacementState& state) {
  CreativeAssetReplacementCommitReceipt receipt;
  receipt.requested = true;
  receipt.objectCount = state.plan.objectIds.size();
  const cr::CreativeDocument& live = appState.facade.document();
  receipt.revisionBefore = live.revision();
  receipt.revisionAfter = live.revision();
  if (!state.active || !state.plan.accepted) {
    receipt.status = CreativeAssetReplacementStatus::NotRequested;
    receipt.reasonCode = "creative_asset_replace_preview_inactive";
    return receipt;
  }
  if (live.id() != state.plan.sourceDocumentId ||
      live.revision() != state.plan.sourceRevision) {
    receipt.status = CreativeAssetReplacementStatus::StaleDocument;
    receipt.reasonCode = "creative_asset_replace_document_stale";
    state.lastStatus = receipt.status;
    state.reasonCode = receipt.reasonCode;
    clearPreview(state);
    return receipt;
  }

  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, "creative_asset_replace");
  const cr::CreativeDocumentBatchMutationReceipt mutation =
      appState.facade.mutateObjectsAtomically(state.plan.mutations);
  receipt.changed = mutation.committed && mutation.changed &&
                    cr::documentMutationSucceeded(mutation.status);
  receipt.historyReceipt = completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      receipt.changed,
      receipt.changed ? "creative_asset_replace_applied"
                      : "creative_asset_replace_mutation_rejected");
  receipt.revisionAfter = appState.facade.document().revision();
  receipt.accepted = receipt.changed;
  receipt.status = receipt.accepted
                       ? CreativeAssetReplacementStatus::Applied
                       : CreativeAssetReplacementStatus::MutationRejected;
  receipt.reasonCode = receipt.accepted
                           ? "creative_asset_replace_applied"
                           : "creative_asset_replace_mutation_rejected";
  state.lastStatus = receipt.status;
  state.reasonCode = receipt.reasonCode;
  clearPreview(state);
  SDL_Log("iggy3d_creative: ASSET REPLACE commit accepted=%d objects=%zu "
          "revision=%llu->%llu",
          receipt.accepted ? 1 : 0, receipt.objectCount,
          static_cast<unsigned long long>(receipt.revisionBefore),
          static_cast<unsigned long long>(receipt.revisionAfter));
  return receipt;
}

bool cancelCreativeEditorAssetReplacement(
    CreativeEditorAssetReplacementState& state,
    std::string_view reasonCode) noexcept {
  if (!state.active) {
    return false;
  }
  state.lastStatus = CreativeAssetReplacementStatus::Cancelled;
  state.reasonCode = reasonCode;
  clearPreview(state);
  return true;
}

const cr::CreativeDocument& creativeEditorAssetReplacementRenderDocument(
    const CreativeEditorAssetReplacementState& state,
    const cr::CreativeDocument& liveDocument) noexcept {
  return state.active && state.previewDocument.isValid()
             ? state.previewDocument
             : liveDocument;
}

CreativeEditorAssetReplacementFrameResult
processCreativeEditorAssetReplacementFrame(
    const CreativeEditorAssetReplacementFrameRequest& request) {
  CreativeEditorAssetReplacementFrameResult result;
  const bool wasActive = request.state.active;
  result.blockWorldActions = wasActive;
  if (!wasActive || request.routedInput.context !=
                        cr::CreativeInputContext::AssetReplacementPreview) {
    return result;
  }
  for (const cr::CreativeInputActionEvent& event :
       request.routedInput.actionEvents()) {
    if (event.action == cr::CreativeInputActionId::ConfirmActiveTool) {
      result.commitReceipt =
          commitCreativeEditorAssetReplacement(request.appState, request.state);
      result.finished = true;
      break;
    }
    if (event.action == cr::CreativeInputActionId::CancelActiveTool) {
      static_cast<void>(cancelCreativeEditorAssetReplacement(
          request.state, "creative_asset_replace_cancelled"));
      result.finished = true;
      break;
    }
  }
  return result;
}

std::size_t appendCreativeEditorAssetReplacementWireframes(
    const CreativeEditorAssetReplacementState& state,
    float thickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines) {
  if (!state.active) {
    return 0U;
  }
  const std::size_t before = wireLines.size();
  wireLines.reserve(wireLines.size() + state.plan.objectIds.size() * 12U);
  for (cr::CreativeObjectId objectId : state.plan.objectIds) {
    const cr::CreativeObject* object = state.previewDocument.findObject(objectId);
    if (object == nullptr) {
      continue;
    }
    const VisualBounds bounds = visualBoundsForObject(*object);
    const std::size_t objectBegin = wireLines.size();
    appendStandaloneWireframeBoxEdges(
        wireLines, bounds.min, bounds.max,
        iggy3d::RenderLineColor{0.24F, 1.0F, 0.40F, 1.0F},
        std::max(0.05F, thickness));
    for (std::size_t index = objectBegin; index < wireLines.size(); ++index) {
      wireLines[index].objectId = objectId;
    }
  }
  return wireLines.size() - before;
}

void appendCreativeEditorAssetReplacementOverlay(
    const CreativeEditorAssetReplacementState& state,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    std::vector<iggy3d::RenderUiRect>& uiRects,
    std::vector<iggy3d::DebugHudGlyphQuad>& glyphs) {
  if (!state.active || drawableWidth < 280U || drawableHeight < 80U) {
    return;
  }
  const std::uint32_t panelWidth = std::min(620U, drawableWidth - 16U);
  constexpr std::uint32_t panelHeight = 54U;
  const std::int32_t panelX =
      static_cast<std::int32_t>((drawableWidth - panelWidth) / 2U);
  constexpr std::int32_t panelY = 12;
  uiRects.push_back({panelX, panelY, panelWidth, panelHeight,
                     0.045F, 0.055F, 0.060F, 0.96F});
  char title[192];
  std::snprintf(title, sizeof(title), "REPLACE %zu OBJECT%s  >  %s",
                state.plan.objectIds.size(),
                state.plan.objectIds.size() == 1U ? "" : "S",
                state.plan.targetAssetId.c_str());
  appendText(glyphs, title, panelX + 12, panelY + 9, drawableWidth,
             drawableHeight, 0.30F, 1.0F, 0.46F);
  appendText(glyphs, "PREVIEW ONLY | X / ENTER APPLY | CIRCLE / ESC CANCEL",
             panelX + 12, panelY + 31, drawableWidth, drawableHeight, 0.76F,
             0.82F, 0.84F);
}

}  // namespace iggy3d_creative_app
