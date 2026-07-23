#include "EditorStructuralPlacement.hpp"

#include <array>
#include <utility>

#include "EditorEdits.hpp"
#include "EditorGroup.hpp"
#include "EditorInteraction.hpp"
#include "EditorPlacementClearance.hpp"
#include "EditorPreviewProxies.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/Hierarchy.hpp"

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
  const cr::CreativeObjectPlacementPolicy& policy =
      cr::describeObject(held.objectKind).placementPolicy;
  const cr::CreativeVec3 surfaceNormal =
      cr::creativeGridTargetSurfaceNormal(target);
  return cr::creativePlacementPolicyAllowsFace(policy, surfaceNormal) &&
         cr::resolveCreativePlacementCompatibility(
             {policy.hostPolicy, target.targetFacts, surfaceNormal})
             .allowed;
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
    case cr::CreativeStructuralSpanStatus::UnsupportedTransform:
    case cr::CreativeStructuralSpanStatus::InvalidEndpoint:
    case cr::CreativeStructuralSpanStatus::ArithmeticOverflow:
      return CreativeBrushPlacementAdmissionStatus::InvalidGeometry;
  }
  return CreativeBrushPlacementAdmissionStatus::InvalidGeometry;
}

void rejectStructuralSpan(CreativeEditorState& editor,
                          cr::CreativeObjectKind objectKind) noexcept {
  setCreativeEditorPlacementRejectionFeedback(
      editor.interaction, editor.frameIndex, objectKind);
}

[[nodiscard]] bool sameTransform(const cr::CreativeTransform& lhs,
                                 const cr::CreativeTransform& rhs) noexcept {
  return lhs.position.x == rhs.position.x &&
         lhs.position.y == rhs.position.y &&
         lhs.position.z == rhs.position.z &&
         lhs.rotationEulerRadians.x == rhs.rotationEulerRadians.x &&
         lhs.rotationEulerRadians.y == rhs.rotationEulerRadians.y &&
         lhs.rotationEulerRadians.z == rhs.rotationEulerRadians.z &&
         lhs.scale.x == rhs.scale.x && lhs.scale.y == rhs.scale.y &&
         lhs.scale.z == rhs.scale.z;
}

[[nodiscard]] const cr::CreativeObject* selectedStructuralSpanObject(
    const cr::CreativeAppState& appState) noexcept {
  const cr::CreativeSelectionState& selection =
      appState.facade.selectionState();
  const std::span<const cr::TargetRef> selected =
      cr::selectedTargetList(selection);
  if (selected.size() != 1U || selected.front().value == cr::kInvalidId) {
    return nullptr;
  }
  const cr::CreativeObject* object = appState.facade.document().findObject(
      static_cast<cr::CreativeObjectId>(selected.front().value));
  if (object == nullptr ||
      !cr::creativeObjectEffectivelyVisible(appState.facade.document(),
                                            object->id) ||
      cr::creativeObjectEffectivelyLocked(appState.facade.document(),
                                          object->id) ||
      !object->assetId.empty() ||
      !cr::descriptorSupportsCreativeStructuralSpan(
          cr::describeObject(object->kind)) ||
      !cr::descriptorAllowsMutation(object->kind,
                                    cr::CreativeMutationKind::SetTransform) ||
      !cr::descriptorAllowsMutation(object->kind,
                                    cr::CreativeMutationKind::SetBounds)) {
    return nullptr;
  }
  return object;
}

void preserveAvailableStructuralSpanEdit(
    CreativeEditorStructuralSpanEditState& state) noexcept {
  const cr::CreativeDocumentId documentId = state.documentId;
  const cr::CreativeObjectId objectId = state.objectId;
  const cr::CreativeObjectKind objectKind = state.objectKind;
  state = {};
  state.documentId = documentId;
  state.objectId = objectId;
  state.objectKind = objectKind;
  state.available = objectId != cr::kInvalidObjectId;
}

[[nodiscard]] bool beginStructuralSpanEndpointEdit(
    const cr::CreativeAppState& appState,
    CreativeEditorStructuralSpanEditState& state,
    cr::CreativeStructuralSpanEndpoint endpoint) noexcept {
  const cr::CreativeObject* object = selectedStructuralSpanObject(appState);
  if (object == nullptr || object->id != state.objectId ||
      (endpoint != cr::CreativeStructuralSpanEndpoint::First &&
       endpoint != cr::CreativeStructuralSpanEndpoint::Second)) {
    return false;
  }
  const cr::CreativeStructuralSpanInstance span =
      cr::resolveCreativeStructuralSpan(object->kind, object->transform,
                                        object->bounds);
  if (!span.accepted) {
    return false;
  }
  state.documentId = appState.facade.document().id();
  state.objectId = object->id;
  state.objectKind = object->kind;
  state.sourceRevision = appState.facade.document().revision();
  state.sourceTransform = object->transform;
  state.sourceBounds = object->bounds;
  state.selectedEndpoint = endpoint;
  state.preview = {};
  state.available = true;
  state.active = true;
  state.targetAvailable = false;
  return true;
}

void tagWireframeRange(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
    std::size_t begin,
    cr::CreativeObjectId objectId,
    std::uint32_t style) noexcept {
  for (std::size_t index = begin; index < lines.size(); ++index) {
    lines[index].objectId = objectId;
    lines[index].style = style;
  }
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
      cr::creativePlacementFaceFromNormal(
          cr::creativeGridTargetSurfaceNormal(target));
  plan.storagePolicy = descriptor.placementPolicy.storagePolicy;
  plan.compatibility = cr::resolveCreativePlacementCompatibility(
      {descriptor.placementPolicy.hostPolicy, target.targetFacts,
       cr::creativeGridTargetSurfaceNormal(target)});
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
    const cr::CreativeWorldActionFrame& actions,
    const CreativePlacementClearanceCache* clearanceCache) {
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

  CreativeBrushPlacementAdmission admission =
      resolveCreativeEditorStructuralSpanPlacement(
          state, documentId, held, editor.interaction.target.grid);
  applyCreativeBrushPlacementClearance(
      admission, appState.facade.document(), clearanceCache);
  if (!admission.allowed) {
    setCreativeEditorPlacementAdmissionRejectionFeedback(
        editor.interaction, editor.frameIndex, admission);
    return true;
  }
  if (creativeBrushPlacementAlreadyExists(appState.facade.document(),
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
      activeCreativeEditorGroupFocusId(editor.groupFocus), {},
      0U, {},
      clearanceCache);
  const bool changed = receipt.accepted && receipt.changed &&
                       receipt.objectCreated;
  static_cast<void>(completeEditTransaction(
      appState.history, std::move(transaction), appState.facade, changed,
      receipt.reasonCode));
  setCreativeEditorPlacementMutationFeedback(
      editor.interaction, editor.frameIndex, receipt);
  if (!changed) {
    return true;
  }

  editor.placedCount = ordinal;
  state = {};
  return true;
}

void cancelCreativeEditorStructuralSpan(
    CreativeEditorState& editor) noexcept {
  editor.interaction.structuralSpan = {};
}

void syncCreativeEditorStructuralSpanEditState(
    const cr::CreativeAppState& appState,
    CreativeEditorStructuralSpanEditState& state) noexcept {
  const cr::CreativeDocument& document = appState.facade.document();
  const cr::CreativeObject* selected = selectedStructuralSpanObject(appState);
  if (selected == nullptr) {
    state = {};
    return;
  }

  if (state.active) {
    const cr::CreativeObject* source = document.findObject(state.objectId);
    if (source == nullptr || selected->id != state.objectId ||
        document.id() != state.documentId ||
        document.revision() != state.sourceRevision ||
        source->kind != state.objectKind ||
        !sameTransform(source->transform, state.sourceTransform) ||
        !cr::creativeBoundsExactlyEqual(source->bounds, state.sourceBounds)) {
      state = {};
      return;
    }
    state.available = true;
    return;
  }

  state = {};
  state.documentId = document.id();
  state.objectId = selected->id;
  state.objectKind = selected->kind;
  state.available = cr::resolveCreativeStructuralSpan(
                        selected->kind, selected->transform, selected->bounds)
                        .accepted;
  if (!state.available) {
    state = {};
  }
}

void resetCreativeEditorStructuralSpanEdit(
    CreativeEditorStructuralSpanEditState& state) noexcept {
  state = {};
}

bool cancelCreativeEditorStructuralSpanEdit(
    CreativeEditorStructuralSpanEditState& state) noexcept {
  if (!state.active) {
    return false;
  }
  preserveAvailableStructuralSpanEdit(state);
  return true;
}

void refreshCreativeEditorStructuralSpanEditPreview(
    const cr::CreativeAppState& appState,
    CreativeEditorStructuralSpanEditState& state,
    bool targetAvailable,
    cr::CreativeVec3 targetAnchor) noexcept {
  if (!state.active) {
    return;
  }
  const cr::CreativeDocument& document = appState.facade.document();
  const cr::CreativeObject* object = document.findObject(state.objectId);
  if (document.id() != state.documentId ||
      document.revision() != state.sourceRevision || object == nullptr ||
      object->kind != state.objectKind ||
      !sameTransform(object->transform, state.sourceTransform) ||
      !cr::creativeBoundsExactlyEqual(object->bounds, state.sourceBounds)) {
    state = {};
    return;
  }
  state.targetAvailable =
      targetAvailable && cr::isFiniteCreativeVec3(targetAnchor);
  state.targetAnchor =
      state.targetAvailable ? targetAnchor : cr::CreativeVec3{};
  state.preview = {};
  if (!state.targetAvailable) {
    return;
  }
  state.preview = cr::planCreativeStructuralSpanEdit(
      {state.objectKind, state.sourceTransform, state.sourceBounds,
       state.selectedEndpoint, targetAnchor});
}

bool processCreativeEditorStructuralSpanEditInput(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    const cr::CreativeWorldActionFrame& actions,
    std::span<const PathPointHandleHit> endpointHandles,
    float targetPixelX,
    float targetPixelY) {
  CreativeEditorStructuralSpanEditState& state =
      editor.interaction.structuralSpanEdit;
  const bool wasActive = state.active;
  const cr::CreativeObjectKind activeObjectKind = state.objectKind;
  syncCreativeEditorStructuralSpanEditState(appState, state);
  if (wasActive && !state.active) {
    rejectStructuralSpan(editor, activeObjectKind);
    return true;
  }

  const bool rejectPressed = cr::creativeWorldActionPressed(
      actions, cr::CreativeWorldActionId::Reject);
  if (rejectPressed) {
    if (!cancelCreativeEditorStructuralSpanEdit(state)) {
      return false;
    }
    clearCreativeEditorPlacementFeedback(editor.interaction);
    return true;
  }

  const bool acceptPressed = cr::creativeWorldActionPressed(
                                 actions, cr::CreativeWorldActionId::Primary) ||
                             cr::creativeWorldActionPressed(
                                 actions, cr::CreativeWorldActionId::Accept);
  if (!state.active) {
    if (!acceptPressed || !state.available) {
      return false;
    }
    const PathPointHandlePickResult handle = pickPathPointHandleAtPixel(
        endpointHandles, targetPixelX, targetPixelY);
    if (!handle.hit || handle.objectId != state.objectId ||
        handle.pointIndex >= 2U ||
        !beginStructuralSpanEndpointEdit(
            appState, state,
            static_cast<cr::CreativeStructuralSpanEndpoint>(
                handle.pointIndex))) {
      return false;
    }
    refreshCreativeEditorStructuralSpanEditPreview(
        appState, state, editor.interaction.target.grid.valid,
        editor.interaction.target.grid.placementAnchor);
    clearCreativeEditorPlacementFeedback(editor.interaction);
    return true;
  }

  refreshCreativeEditorStructuralSpanEditPreview(
      appState, state, editor.interaction.target.grid.valid,
      editor.interaction.target.grid.placementAnchor);
  if (!acceptPressed) {
    return true;
  }
  if (!state.active || !state.targetAvailable || !state.preview.accepted ||
      !state.preview.changed) {
    rejectStructuralSpan(editor, state.objectKind);
    return true;
  }

  cr::CreativeDocument& document = appState.facade.documentForPersistence();
  const cr::CreativeObject* object = document.findObject(state.objectId);
  if (document.id() != state.documentId ||
      document.revision() != state.sourceRevision || object == nullptr ||
      object->kind != state.objectKind ||
      !sameTransform(object->transform, state.sourceTransform) ||
      !cr::creativeBoundsExactlyEqual(object->bounds, state.sourceBounds)) {
    rejectStructuralSpan(editor, state.objectKind);
    state = {};
    return true;
  }

  const cr::CreativeObjectId objectId = state.objectId;
  const cr::CreativeObjectKind objectKind = state.objectKind;
  StandaloneEditTransaction transaction = beginEditTransaction(
      appState.facade, "creative_structural_span_endpoint_edit");
  if (!transaction.active) {
    rejectStructuralSpan(editor, objectKind);
    return true;
  }
  const std::array mutations{
      cr::CreativeMutationRequest{
          0U, objectId, cr::CreativeMutationKind::SetTransform,
          cr::makeSetTransformPayload(state.preview.transform)},
      cr::CreativeMutationRequest{
          0U, objectId, cr::CreativeMutationKind::SetBounds,
          cr::makeBoundsPayload(state.preview.authoredBounds)},
  };
  const cr::CreativeDocumentBatchMutationReceipt receipt =
      cr::applyDocumentMutationsAtomically(document, mutations);
  const bool changed = receipt.committed &&
                       cr::documentMutationSucceeded(receipt.status) &&
                       receipt.changed;
  static_cast<void>(completeEditTransaction(
      appState.history, std::move(transaction), appState.facade, changed,
      receipt.message));
  if (!changed) {
    rejectStructuralSpan(editor, objectKind);
    state = {};
    return true;
  }

  setCreativeEditorPlacementFeedback(
      editor.interaction, CreativeEditorPlacementFeedbackStatus::Placed,
      editor.frameIndex, objectKind, objectId);
  state = {};
  syncCreativeEditorStructuralSpanEditState(appState, state);
  return true;
}

CreativeEditorStructuralSpanEndpointHandleFrame
buildCreativeEditorStructuralSpanEndpointHandles(
    const cr::CreativeObject& object,
    const iggy3d::Mat4& clipFromWorld,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight) noexcept {
  CreativeEditorStructuralSpanEndpointHandleFrame frame;
  const cr::CreativeStructuralSpanInstance span =
      cr::resolveCreativeStructuralSpan(object.kind, object.transform,
                                        object.bounds);
  if (!object.visible || !span.accepted || drawableWidth == 0U ||
      drawableHeight == 0U) {
    return frame;
  }
  for (std::size_t index = 0U; index < span.endpoints.size(); ++index) {
    const VisualBounds bounds = pathPointHandleBounds(span.endpoints[index]);
    PathPointHandleHit& handle = frame.handles[frame.count++];
    handle.objectId = object.id;
    handle.pointIndex = index;
    handle.position = span.endpoints[index];
    handle.aabb = cr::projectCreativeWorldBoundsToScreen(
        clipFromWorld, bounds.min, bounds.max, drawableWidth, drawableHeight);
  }
  return frame;
}

CreativeBrushPlacementPlan creativeEditorStructuralSpanEditPreviewPlan(
    const CreativeEditorStructuralSpanEditState& state) noexcept {
  CreativeBrushPlacementPlan plan;
  plan.brush = state.objectKind;
  if (!state.active || !state.targetAvailable) {
    return plan;
  }
  const cr::CreativeObjectDescriptor& descriptor =
      cr::describeObject(state.objectKind);
  plan.status = CreativeBrushPlacementPlanStatus::Ready;
  plan.shapeKind = descriptor.shapeKind;
  plan.transform = state.preview.accepted ? state.preview.transform
                                          : state.sourceTransform;
  plan.authoredBounds = state.preview.accepted ? state.preview.authoredBounds
                                               : state.sourceBounds;
  plan.previewBounds = plan.authoredBounds;
  plan.storagePolicy = descriptor.placementPolicy.storagePolicy;
  plan.hasTransformOverride = true;
  plan.hasBoundsOverride = true;
  plan.orientationResolved = true;
  plan.valid = true;
  return plan;
}

std::size_t appendCreativeEditorStructuralSpanEditWireframe(
    const cr::CreativeAppState& appState,
    const CreativeEditorStructuralSpanEditState& state,
    float thickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines) {
  if (!state.available || state.objectId == cr::kInvalidObjectId) {
    return 0U;
  }
  const cr::CreativeObject* object =
      appState.facade.document().findObject(state.objectId);
  if (object == nullptr ||
      !cr::creativeObjectEffectivelyVisible(appState.facade.document(),
                                            object->id)) {
    return 0U;
  }
  const cr::CreativeTransform transform =
      state.active ? state.sourceTransform : object->transform;
  const cr::CreativeBounds bounds =
      state.active ? state.sourceBounds : object->bounds;
  const cr::CreativeStructuralSpanInstance span =
      cr::resolveCreativeStructuralSpan(object->kind, transform, bounds);
  if (!span.accepted) {
    return 0U;
  }

  const std::size_t begin = wireLines.size();
  constexpr iggy3d::RenderLineColor kEndpointColor{
      0.20F, 0.82F, 1.0F, 1.0F};
  constexpr iggy3d::RenderLineColor kSelectedColor{
      1.0F, 0.88F, 0.16F, 1.0F};
  for (std::size_t index = 0U; index < span.endpoints.size(); ++index) {
    const VisualBounds handle = pathPointHandleBounds(span.endpoints[index]);
    const bool selected =
        state.active && static_cast<std::size_t>(state.selectedEndpoint) == index;
    const std::size_t boxBegin = wireLines.size();
    appendStandaloneWireframeBoxEdges(
        wireLines, handle.min, handle.max,
        selected ? kSelectedColor : kEndpointColor, thickness);
    tagWireframeRange(wireLines, boxBegin, object->id,
                      selected ? 1U : 0U);
  }

  if (state.active && state.targetAvailable && state.preview.accepted) {
    const cr::CreativeTransformedBounds preview =
        cr::resolveCreativeTransformedBounds(state.preview.authoredBounds,
                                             state.preview.transform);
    const cr::CreativeCoreVec3Conversion minimum =
        cr::creativeVec3ToCoreChecked(preview.worldBounds.min);
    const cr::CreativeCoreVec3Conversion maximum =
        cr::creativeVec3ToCoreChecked(preview.worldBounds.max);
    if (preview.valid && minimum.converted && maximum.converted) {
      const std::size_t boxBegin = wireLines.size();
      appendStandaloneWireframeBoxEdges(
          wireLines, minimum.value, maximum.value,
          state.preview.changed
              ? iggy3d::RenderLineColor{0.22F, 1.0F, 0.34F, 1.0F}
              : iggy3d::RenderLineColor{1.0F, 0.20F, 0.20F, 1.0F},
          thickness * 0.85F);
      tagWireframeRange(wireLines, boxBegin, object->id, 2U);
    }
  } else if (state.active && state.targetAvailable) {
    const VisualBounds target = pathPointHandleBounds(state.targetAnchor);
    const std::size_t boxBegin = wireLines.size();
    appendStandaloneWireframeBoxEdges(
        wireLines, target.min, target.max,
        {1.0F, 0.20F, 0.20F, 1.0F}, thickness);
    tagWireframeRange(wireLines, boxBegin, object->id, 3U);
  }
  return wireLines.size() - begin;
}

}  // namespace iggy3d_creative_app
