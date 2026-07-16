#include "EditorStructuralPlacement.hpp"

#include <utility>

#include "EditorEdits.hpp"
#include "EditorGroup.hpp"
#include "EditorInteraction.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Geometry.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

[[nodiscard]] bool structuralSpanAnchorAllowed(
    const cr::CreativeHotbarEntry& held,
    const cr::CreativeGridTarget& target) noexcept {
  if (!creativeEditorUsesStructuralSpan(held) || !target.valid ||
      !cr::isFiniteCreativeVec3(target.placementAnchor)) {
    return false;
  }
  return cr::creativePlacementPolicyAllowsFace(
      cr::describeObject(held.objectKind).placementPolicy,
      target.faceNormal);
}

[[nodiscard]] CreativeBrushPlacementAdmissionStatus admissionStatusFor(
    cr::CreativeStructuralSpanStatus status) noexcept {
  switch (status) {
    case cr::CreativeStructuralSpanStatus::Ready:
      return CreativeBrushPlacementAdmissionStatus::Ready;
    case cr::CreativeStructuralSpanStatus::UnsupportedDescriptor:
      return CreativeBrushPlacementAdmissionStatus::UnsupportedBrush;
    case cr::CreativeStructuralSpanStatus::InvalidAnchor:
      return CreativeBrushPlacementAdmissionStatus::InvalidTarget;
    case cr::CreativeStructuralSpanStatus::DegenerateSpan:
    case cr::CreativeStructuralSpanStatus::SpanTooLong:
    case cr::CreativeStructuralSpanStatus::InvalidGeometry:
    case cr::CreativeStructuralSpanStatus::ArithmeticOverflow:
      return CreativeBrushPlacementAdmissionStatus::InvalidGeometry;
  }
  return CreativeBrushPlacementAdmissionStatus::InvalidGeometry;
}

void rejectStructuralSpan(CreativeEditorState& editor,
                          cr::CreativeObjectKind objectKind) noexcept {
  setCreativeEditorPlacementFeedback(
      editor.interaction, CreativeEditorPlacementFeedbackStatus::Rejected,
      editor.frameIndex, objectKind);
}

}  // namespace

bool creativeEditorUsesStructuralSpan(
    const cr::CreativeHotbarEntry& held) noexcept {
  return held.kind == cr::CreativeHeldItemKind::Material &&
         cr::creativeHotbarAssetId(held).empty() &&
         cr::descriptorSupportsCreativeStructuralSpan(
             cr::describeObject(held.objectKind));
}

CreativeBrushPlacementAdmission
resolveCreativeEditorStructuralSpanPlacement(
    const CreativeEditorStructuralSpanState& state,
    cr::CreativeDocumentId documentId,
    const cr::CreativeHotbarEntry& held,
    const cr::CreativeGridTarget& target) noexcept {
  CreativeBrushPlacementAdmission admission;
  admission.plan.brush = held.objectKind;
  if (!state.active || documentId == cr::kInvalidDocumentId ||
      state.documentId != documentId || state.objectKind != held.objectKind ||
      !structuralSpanAnchorAllowed(held, target)) {
    return admission;
  }

  const cr::CreativeStructuralSpanPlan structural =
      cr::planCreativeStructuralSpan(
          {held.objectKind, state.firstAnchor, target.placementAnchor});
  admission.status = admissionStatusFor(structural.status);
  if (!structural.accepted) {
    return admission;
  }

  CreativeBrushPlacementPlan& plan = admission.plan;
  const cr::CreativeObjectDescriptor& descriptor =
      cr::describeObject(held.objectKind);
  plan.status = CreativeBrushPlacementPlanStatus::Ready;
  plan.brush = held.objectKind;
  plan.shapeKind = descriptor.shapeKind;
  plan.transform = structural.transform;
  plan.authoredBounds = structural.authoredBounds;
  plan.previewBounds = structural.authoredBounds;
  plan.resolvedFace =
      cr::creativePlacementFaceFromNormal(target.faceNormal);
  plan.storagePolicy = descriptor.placementPolicy.storagePolicy;
  plan.hasTransformOverride = true;
  plan.hasBoundsOverride = true;
  plan.orientationResolved = true;
  plan.valid = true;
  admission.status = CreativeBrushPlacementAdmissionStatus::Ready;
  admission.allowed = true;
  return admission;
}

bool processCreativeEditorStructuralSpanInput(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    const cr::CreativeWorldActionFrame& actions) {
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  if (!creativeEditorUsesStructuralSpan(held)) {
    return false;
  }

  CreativeEditorStructuralSpanState& state =
      editor.interaction.structuralSpan;
  const cr::CreativeDocumentId documentId = appState.facade.document().id();
  if (state.active &&
      (state.documentId != documentId || state.objectKind != held.objectKind)) {
    state = {};
  }

  const bool cancelPressed =
      cr::creativeWorldActionPressed(actions, cr::CreativeWorldActionId::Primary) ||
      cr::creativeWorldActionPressed(actions, cr::CreativeWorldActionId::Reject);
  if (cancelPressed) {
    if (!state.active) {
      return false;
    }
    state = {};
    clearCreativeEditorPlacementFeedback(editor.interaction);
    return true;
  }

  const bool confirmPressed =
      cr::creativeWorldActionPressed(actions, cr::CreativeWorldActionId::Secondary) ||
      cr::creativeWorldActionPressed(actions, cr::CreativeWorldActionId::Accept);
  if (!state.active) {
    if (!confirmPressed) {
      return false;
    }
    if (documentId == cr::kInvalidDocumentId ||
        !structuralSpanAnchorAllowed(held, editor.interaction.target.grid)) {
      rejectStructuralSpan(editor, held.objectKind);
      return true;
    }
    finalizeCreativeMaterialStroke(
        appState, editor, "creative_structural_span_started");
    state.documentId = documentId;
    state.objectKind = held.objectKind;
    state.firstAnchor = editor.interaction.target.grid.placementAnchor;
    state.active = true;
    clearCreativeEditorPlacementFeedback(editor.interaction);
    return true;
  }

  if (!confirmPressed) {
    return true;
  }

  const CreativeBrushPlacementAdmission admission =
      resolveCreativeEditorStructuralSpanPlacement(
          state, documentId, held, editor.interaction.target.grid);
  if (!admission.allowed ||
      creativeBrushPlacementAlreadyExists(appState.facade.document(),
                                           admission.plan)) {
    rejectStructuralSpan(editor, held.objectKind);
    return true;
  }

  StandaloneEditTransaction transaction = beginEditTransaction(
      appState.facade, "creative_structural_span_place");
  if (!transaction.active) {
    rejectStructuralSpan(editor, held.objectKind);
    return true;
  }
  const std::uint64_t ordinal = editor.placedCount + 1U;
  const CreativeBrushPlacementMutationReceipt receipt = applyBrushPlacement(
      appState.facade, admission.plan, ordinal,
      activeCreativeEditorGroupFocusId(editor.groupFocus));
  const bool changed = receipt.accepted && receipt.changed &&
                       receipt.objectCreated;
  static_cast<void>(completeEditTransaction(
      appState.history, std::move(transaction), appState.facade, changed,
      receipt.reasonCode));
  if (!changed) {
    rejectStructuralSpan(editor, held.objectKind);
    return true;
  }

  editor.placedCount = ordinal;
  setCreativeEditorPlacementFeedback(
      editor.interaction, CreativeEditorPlacementFeedbackStatus::Placed,
      editor.frameIndex, receipt.objectKind, receipt.objectId);
  state = {};
  return true;
}

void cancelCreativeEditorStructuralSpan(
    CreativeEditorState& editor) noexcept {
  editor.interaction.structuralSpan = {};
}

}  // namespace iggy3d_creative_app
