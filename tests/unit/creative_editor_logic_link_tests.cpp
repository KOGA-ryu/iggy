#include "EditorLogicLinks.hpp"
#include "EditorLogicLinkOverlay.hpp"
#include "EditorFrame.hpp"
#include "EditorGizmo.hpp"
#include "EditorPreviewFrameInternal.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/input/HeldItemRegistry.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;
namespace app = iggy3d_creative_app;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeObjectId create(cr::Facade& facade,
                            cr::CreativeObjectKind kind,
                            std::string name,
                            cr::CreativeVec3 position = {}) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = std::move(name);
  request.transform.position = position;
  request.hasTransformOverride = true;
  return facade.createDocumentObject(request).objectId;
}

bool selectionLinkingAndHistoryAreOneKernel() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Logic UI");
  static_cast<void>(document.assignId(700U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId source =
      create(appState.facade, cr::CreativeObjectKind::Switch, "Switch");
  const cr::CreativeObjectId target =
      create(appState.facade, cr::CreativeObjectKind::Door, "Door");
  appState.history = {};
  app::CreativeEditorLogicLinkState state;

  const app::CreativeEditorLogicLinkReceipt selected =
      app::advanceCreativeEditorLogicLink(appState, state, source, "select");
  const app::CreativeEditorLogicLinkReceipt added =
      app::advanceCreativeEditorLogicLink(appState, state, target, "add");
  const app::CreativeEditorLogicLinkReceipt removed =
      app::advanceCreativeEditorLogicLink(appState, state, target, "remove");
  state.action = cr::CreativeLogicLinkAction::Open;
  const app::CreativeEditorLogicLinkReceipt reopened =
      app::advanceCreativeEditorLogicLink(appState, state, target, "open");
  const bool undone = app::undoLastEdit(appState, "undo_open");

  return expect(selected.accepted && !selected.changed &&
                    state.sourceObjectId == source,
                "control selection arms a source without history") &&
         expect(added.accepted && added.changed &&
                    added.status == app::CreativeEditorLogicLinkStatus::Added,
                "door activation adds a link") &&
         expect(removed.accepted && removed.changed &&
                    removed.status ==
                        app::CreativeEditorLogicLinkStatus::Removed,
                "same action on linked door removes it") &&
         expect(reopened.accepted && reopened.changed &&
                    reopened.action == cr::CreativeLogicLinkAction::Open &&
                    cr::creativeUndoDepth(appState.history) == 2U,
                "each document edit records exactly one history entry") &&
         expect(undone &&
                    appState.facade.document().findLogicLink(source, target) ==
                        nullptr,
                "undo restores link-free document snapshot");
}

bool actionCycleAndDocumentSyncAreBounded() {
  app::CreativeEditorLogicLinkState state;
  state.action = cr::CreativeLogicLinkAction::Toggle;
  const bool previous = app::cycleCreativeEditorLogicLinkAction(state, -1);
  const cr::CreativeLogicLinkAction afterPrevious = state.action;
  const bool next = app::cycleCreativeEditorLogicLinkAction(state, 1);
  const cr::CreativeLogicLinkAction afterNext = state.action;

  cr::CreativeDocument document = cr::CreativeDocument::create("Sync");
  static_cast<void>(document.assignId(701U));
  state.documentId = 999U;
  state.sourceObjectId = 22U;
  app::syncCreativeEditorLogicLinkState(state, document);
  return expect(previous &&
                    afterPrevious == cr::CreativeLogicLinkAction::Close &&
                    next && afterNext == cr::CreativeLogicLinkAction::Toggle,
                "three-action cycle wraps in both directions") &&
         expect(next && state.documentId == document.id() &&
                    state.sourceObjectId == cr::kInvalidObjectId,
                "document change clears stale source identity");
}

bool explicitInspectorKernelsShareHistoryAndReceipts() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Inspector Logic");
  static_cast<void>(document.assignId(703U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId source =
      create(appState.facade, cr::CreativeObjectKind::PressurePlate, "Plate");
  const cr::CreativeObjectId target =
      create(appState.facade, cr::CreativeObjectKind::Door, "Door");
  appState.history = {};
  app::CreativeEditorLogicLinkState state;

  const app::CreativeEditorLogicLinkReceipt selected =
      app::selectCreativeEditorLogicLinkSource(appState, state, source);
  const app::CreativeEditorLogicLinkReceipt added =
      app::setCreativeEditorLogicLink(
          appState, state, source, target,
          cr::CreativeLogicLinkAction::Toggle, "inspector_add");
  const app::CreativeEditorLogicLinkReceipt updated =
      app::setCreativeEditorLogicLink(
          appState, state, source, target,
          cr::CreativeLogicLinkAction::Open, "inspector_update");
  const app::CreativeEditorLogicLinkReceipt unchanged =
      app::setCreativeEditorLogicLink(
          appState, state, source, target,
          cr::CreativeLogicLinkAction::Open, "inspector_no_change");
  const std::size_t depthBeforeRemove =
      cr::creativeUndoDepth(appState.history);
  const app::CreativeEditorLogicLinkReceipt removed =
      app::removeCreativeEditorLogicLink(
          appState, state, source, target, "inspector_remove");
  const std::size_t depthAfterRemove =
      cr::creativeUndoDepth(appState.history);
  const bool undone = app::undoLastEdit(appState, "undo_remove");
  const cr::CreativeLogicLink* restored =
      appState.facade.document().findLogicLink(source, target);

  return expect(selected.accepted && state.sourceObjectId == source,
                "Inspector can arm the canonical logic source") &&
         expect(added.accepted && added.changed &&
                    added.status == app::CreativeEditorLogicLinkStatus::Added,
                "explicit set adds a link") &&
         expect(updated.accepted && updated.changed &&
                    updated.status ==
                        app::CreativeEditorLogicLinkStatus::Updated,
                "explicit set updates the action") &&
         expect(unchanged.accepted && !unchanged.changed &&
                    unchanged.status ==
                        app::CreativeEditorLogicLinkStatus::Unchanged &&
                    depthBeforeRemove == 2U,
                "same action is history-free") &&
         expect(removed.accepted && removed.changed &&
                    depthAfterRemove == 3U &&
                    cr::creativeUndoDepth(appState.history) == 2U,
                "remove adds one history entry and undo consumes it") &&
         expect(undone && restored != nullptr &&
                    restored->action == cr::CreativeLogicLinkAction::Open,
                "undo restores the exact updated link");
}

bool connectInputMappingMatchesMouseAndControllerLanguage() {
  const cr::CreativeHeldItemDefinition& definition =
      cr::describeCreativeHeldItem(cr::CreativeHeldItemKind::LogicLink);
  return expect(
      definition.worldOperations[0] ==
              cr::CreativeHeldItemWorldOperation::AdvanceLogicLink &&
          definition.worldOperations[1] ==
              cr::CreativeHeldItemWorldOperation::ClearLogicLinkSource &&
          definition.acceptOperation ==
              cr::CreativeHeldItemWorldOperation::AdvanceLogicLink &&
          definition.rejectOperation ==
              cr::CreativeHeldItemWorldOperation::ClearLogicLinkSource,
      "left click and X link while right click and Circle clear the source");
}

bool directionalOverlayPlanClipsBoundsAndHandlesFailures() {
  app::CreativeLogicLinkOverlayRequest horizontal;
  horizontal.source = {{-1.0F, -1.0F, -1.0F},
                       {1.0F, 1.0F, 1.0F}, true};
  horizontal.target = {{4.0F, -1.0F, -1.0F},
                       {6.0F, 1.0F, 1.0F}, true};
  horizontal.action = cr::CreativeLogicLinkAction::Open;
  horizontal.linkValid = true;
  const app::CreativeLogicLinkOverlayPlan planned =
      app::planCreativeLogicLinkOverlay(horizontal);

  app::CreativeLogicLinkOverlayRequest vertical = horizontal;
  vertical.target = {{-1.0F, 4.0F, -1.0F},
                     {1.0F, 6.0F, 1.0F}, true};
  vertical.action = cr::CreativeLogicLinkAction::Close;
  const app::CreativeLogicLinkOverlayPlan verticalPlan =
      app::planCreativeLogicLinkOverlay(vertical);

  app::CreativeLogicLinkOverlayRequest missing = horizontal;
  missing.target.available = false;
  const app::CreativeLogicLinkOverlayPlan missingPlan =
      app::planCreativeLogicLinkOverlay(missing);

  app::CreativeLogicLinkOverlayRequest invalid = horizontal;
  invalid.source.min = {2.0F, 0.0F, 0.0F};
  invalid.source.max = {1.0F, 1.0F, 1.0F};
  const app::CreativeLogicLinkOverlayPlan rejected =
      app::planCreativeLogicLinkOverlay(invalid);

  app::CreativeLogicLinkOverlayRequest invalidAction = horizontal;
  invalidAction.action = cr::CreativeLogicLinkAction::Count;
  const app::CreativeLogicLinkOverlayPlan invalidActionPlan =
      app::planCreativeLogicLinkOverlay(invalidAction);

  return expect(planned.accepted && planned.segmentCount == 3U &&
                    planned.status ==
                        app::CreativeLogicLinkOverlayStatus::Planned &&
                    planned.role ==
                        app::CreativeLogicLinkOverlayRole::Open,
                "valid link produces one shaft and two arrow edges") &&
         expect(iggy3d::nearlyEqual(planned.sourceAnchor,
                                    {1.0F, 0.0F, 0.0F}) &&
                    iggy3d::nearlyEqual(planned.targetAnchor,
                                        {4.0F, 0.0F, 0.0F}) &&
                    app::creativeLogicLinkOverlayLabel(planned.role) ==
                        "OPEN",
                "shaft clips to both object surfaces and exposes action text") &&
         expect(verticalPlan.accepted &&
                    iggy3d::isFinite(verticalPlan.segments[1].end) &&
                    iggy3d::isFinite(verticalPlan.segments[2].end) &&
                    !iggy3d::nearlyEqual(verticalPlan.segments[1].end,
                                         verticalPlan.segments[2].end),
                "vertical links choose a stable non-degenerate arrow plane") &&
         expect(missingPlan.accepted &&
                    missingPlan.status ==
                        app::CreativeLogicLinkOverlayStatus::MissingTarget &&
                    missingPlan.role ==
                        app::CreativeLogicLinkOverlayRole::Invalid &&
                    app::creativeLogicLinkOverlayLabel(missingPlan.role) ==
                        "INVALID",
                "missing targets remain visible as an invalid directional stub") &&
         expect(!rejected.accepted && rejected.segmentCount == 0U &&
                    rejected.status ==
                        app::CreativeLogicLinkOverlayStatus::InvalidSourceBounds,
                "invalid source bounds fail closed without geometry") &&
         expect(invalidActionPlan.accepted &&
                    invalidActionPlan.role ==
                        app::CreativeLogicLinkOverlayRole::Invalid &&
                    invalidActionPlan.status ==
                        app::CreativeLogicLinkOverlayStatus::InvalidLink,
                "invalid actions remain visible but cannot claim a valid style");
}

bool overlayShowsLinksOnlyInConnectMode() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Overlay");
  static_cast<void>(document.assignId(702U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId source =
      create(appState.facade, cr::CreativeObjectKind::Switch, "Switch",
             {-0.60F, 0.0F, 0.0F});
  const cr::CreativeObjectId target =
      create(appState.facade, cr::CreativeObjectKind::Door, "Door",
             {0.60F, 0.0F, 0.0F});
  static_cast<void>(appState.facade.setLogicLink(
      {source, target, cr::CreativeLogicLinkAction::Toggle}));

  app::CreativeEditorState editor;
  editor.interaction.hotbar.entries[0].kind = cr::CreativeHeldItemKind::LogicLink;
  editor.logicLinks.documentId = appState.facade.document().id();
  editor.logicLinks.sourceObjectId = source;
  editor.interaction.target.objectHit = true;
  editor.interaction.target.objectId = target;
  app::CreativeEditorSelectionFrame selection;
  app::CreativeEditorGizmoFrame gizmo;
  iggy3d::FrameInput frame;
  frame.viewport.width = 800U;
  frame.viewport.height = 600U;
  cr::CreativeSpatialProjectionRequest projection;
  app::CreativeEditorOverlayFrame visible;
  app::CreativeEditorOverlayFrameRequest request{
      appState, editor, selection, gizmo, frame, projection};
  request.drawableWidth = 800U;
  request.drawableHeight = 600U;
  static_cast<void>(app::buildCreativeEditorWorldWireframes(request, visible));
  editor.interaction.hotbar.entries[0].kind =
      cr::CreativeHeldItemKind::Material;
  app::CreativeEditorOverlayFrame inspected;
  static_cast<void>(
      app::buildCreativeEditorWorldWireframes(request, inspected));
  app::CreativeEditorOverlayFrame hidden;
  request.captureMode = true;
  static_cast<void>(app::buildCreativeEditorWorldWireframes(request, hidden));
  return expect(visible.logicLinkEdgeCount == 27U &&
                    visible.logicLinkShaftCount == 1U &&
                    visible.logicLinkArrowEdgeCount == 2U &&
                    visible.logicLinkEndpointEdgeCount == 24U &&
                    visible.logicLinkLabelGlyphCount == 6U,
                "connect overlay draws an arrow, action label, source, and "
                "hover boxes") &&
         expect(inspected.logicLinkEdgeCount == 27U &&
                    inspected.logicLinkShaftCount == 1U &&
                    inspected.logicLinkArrowEdgeCount == 2U &&
                    inspected.logicLinkEndpointEdgeCount == 24U &&
                    inspected.logicLinkLabelGlyphCount == 6U &&
                    inspected.invalidLogicLinkCount == 0U &&
                    inspected.combinedWireLines[
                        inspected.documentWireLineCount]
                            .color.b == 1.0F,
                "selected circuit stays visible with directional Toggle styling") &&
         expect(hidden.logicLinkEdgeCount == 0U &&
                    hidden.logicLinkLabelGlyphCount == 0U,
                "capture mode hides connect overlays");
}

}  // namespace

int main() {
  const bool ok = selectionLinkingAndHistoryAreOneKernel() &&
                  actionCycleAndDocumentSyncAreBounded() &&
                  explicitInspectorKernelsShareHistoryAndReceipts() &&
                  connectInputMappingMatchesMouseAndControllerLanguage() &&
                  directionalOverlayPlanClipsBoundsAndHandlesFailures() &&
                  overlayShowsLinksOnlyInConnectMode();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
