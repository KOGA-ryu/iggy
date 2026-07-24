#include "EditorObjectActionExecutor.hpp"

#include <type_traits>
#include <utility>

#include "EditorEdits.hpp"
#include "EditorWorldLayout.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/document/Hierarchy.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;

namespace {

void addIntegrationImpact(
    CreativeEditorObjectActionExecution& execution,
    CreativeEditorObjectActionIntegrationImpact impact) noexcept {
  execution.impacts |= creativeEditorObjectActionIntegrationImpactFlag(impact);
}

void recordWorldLayoutMutation(
    CreativeEditorObjectActionExecution& execution,
    CreativeEditorObjectActionIntegrationImpact mutationImpact,
    bool previewWasActive) noexcept {
  addIntegrationImpact(
      execution,
      CreativeEditorObjectActionIntegrationImpact::WorldLayoutSourceChanged);
  addIntegrationImpact(execution, mutationImpact);
  if (previewWasActive) {
    addIntegrationImpact(
        execution,
        CreativeEditorObjectActionIntegrationImpact::SceneRefreshRequired);
  }
}

void synchronizeSelection(
    CreativeEditorObjectActionExecution& execution,
    const CreativeEditorObjectActionExecutionContext& context) {
  if (context.worldLayout == nullptr) {
    return;
  }
  execution.selectionSynchronizationAttempted = true;
  const CreativeEditorSelectionSynchronizationReceipt synchronized =
      synchronizeCreativeEditorWorldLayoutSelection(
          *context.worldLayout, context.appState.facade.document(),
          context.appState.facade.selectionState());
  execution.selectionSynchronizationAccepted = synchronized.accepted;
  execution.selectionSynchronizationChanged = synchronized.changed;
  if (synchronized.accepted) {
    addIntegrationImpact(
        execution,
        CreativeEditorObjectActionIntegrationImpact::SelectionSynchronized);
  }
}

void finishExecution(CreativeEditorObjectActionExecution& execution) {
  execution.status =
      formatCreativeEditorObjectActionOutcome(execution.outcome);
}

}  // namespace

CreativeEditorObjectActionExecution executeCreativeEditorSceneObjectAction(
    const CreativeEditorObjectActionExecutionContext& context,
    const CreativeEditorSceneObjectActionRequest& request) {
  CreativeEditorObjectActionExecution execution;
  const bool previewWasActive =
      context.worldLayout != nullptr &&
      creativeEditorWorldLayoutPreviewActive(*context.worldLayout);

  std::visit(
      [&](const auto& action) {
        using Action = std::decay_t<decltype(action)>;
        if constexpr (std::is_same_v<Action,
                                     CreativeEditorDeleteSelectionAction>) {
          const CreativeEditorDeleteReceipt receipt =
              deleteCreativeEditorSelectionWithUndo(
                  context.appState, request.historySource,
                  &context.appState.history, context.worldLayout);
          execution.outcome = makeCreativeEditorObjectActionOutcome(
              cr::CreativeSemanticObjectAction::Delete,
              CreativeEditorObjectActionTarget::Selection, receipt.accepted,
              receipt.changed, receipt.affectedObjectCount,
              receipt.reasonCode);
          if (receipt.worldLayoutSourceDeleted) {
            recordWorldLayoutMutation(
                execution,
                CreativeEditorObjectActionIntegrationImpact::
                    WorldLayoutSourceDeleted,
                previewWasActive);
          }
          if (receipt.accepted) {
            synchronizeSelection(execution, context);
          }
        } else if constexpr (
            std::is_same_v<Action,
                           CreativeEditorDuplicateSelectionAction>) {
          const CreativeEditorDuplicateReceipt receipt =
              duplicateCreativeEditorSelectionWithUndo(
                  context.appState, context.appState.history, action.request,
                  request.historySource, context.worldLayout);
          execution.outcome = makeCreativeEditorObjectActionOutcome(
              cr::CreativeSemanticObjectAction::Duplicate,
              CreativeEditorObjectActionTarget::Selection, receipt.accepted,
              receipt.changed, receipt.affectedObjectCount,
              receipt.reasonCode);
          if (receipt.worldLayoutSourceDuplicated) {
            recordWorldLayoutMutation(
                execution,
                CreativeEditorObjectActionIntegrationImpact::
                    WorldLayoutSourceDuplicated,
                previewWasActive);
          } else if (receipt.accepted) {
            synchronizeSelection(execution, context);
          }
        } else if constexpr (
            std::is_same_v<Action,
                           CreativeEditorTransformSelectionAction>) {
          const CreativeEditorSemanticEditReceipt receipt =
              transformCreativeEditorSelectionWithUndo(
                  context.appState, context.appState.history, action.request,
                  request.historySource, context.worldLayout);
          execution.outcome = makeCreativeEditorObjectActionOutcome(
              cr::CreativeSemanticObjectAction::TransformSelection,
              CreativeEditorObjectActionTarget::Selection, receipt.accepted,
              receipt.changed, receipt.affectedObjectCount,
              receipt.reasonCode);
          if (receipt.worldLayoutSourceChanged) {
            recordWorldLayoutMutation(
                execution,
                CreativeEditorObjectActionIntegrationImpact::
                    WorldLayoutSourceChanged,
                previewWasActive);
          }
        } else if constexpr (
            std::is_same_v<Action,
                           CreativeEditorSetObjectTransformAction>) {
          const cr::CreativeObject* object =
              context.appState.facade.findObject(action.objectId);
          if (object != nullptr &&
              cr::creativeObjectIsHierarchyContainer(object->kind)) {
            execution.outcome = rejectCreativeEditorObjectAction(
                cr::CreativeSemanticObjectAction::SetTransform,
                CreativeEditorObjectActionTarget::Object,
                CreativeEditorObjectActionOutcomeStatus::InvalidRequest,
                "creative_editor_transform_container_requires_selection_transform");
            return;
          }
          const CreativeEditorSemanticEditReceipt receipt =
              setCreativeEditorObjectTransformWithUndo(
                  context.appState, context.appState.history, action.objectId,
                  action.transform, action.setPosition, action.setRotation,
                  action.setScale, request.historySource,
                  context.worldLayout);
          execution.outcome = makeCreativeEditorObjectActionOutcome(
              cr::CreativeSemanticObjectAction::SetTransform,
              CreativeEditorObjectActionTarget::Object, receipt.accepted,
              receipt.changed, receipt.affectedObjectCount,
              receipt.reasonCode, receipt.requiresAdoption);
          if (receipt.requiresAdoption) {
            addIntegrationImpact(
                execution,
                CreativeEditorObjectActionIntegrationImpact::
                    WorldLayoutAdoptionRequired);
          }
        }
      },
      request.action);

  finishExecution(execution);
  return execution;
}

}  // namespace iggy3d_creative_app
