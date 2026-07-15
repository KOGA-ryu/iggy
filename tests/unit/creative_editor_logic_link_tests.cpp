#include "EditorLogicLinks.hpp"
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
                            std::string name) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = std::move(name);
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

bool overlayShowsLinksOnlyInConnectMode() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Overlay");
  static_cast<void>(document.assignId(702U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  const cr::CreativeObjectId source =
      create(appState.facade, cr::CreativeObjectKind::Switch, "Switch");
  const cr::CreativeObjectId target =
      create(appState.facade, cr::CreativeObjectKind::Door, "Door");
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
  cr::CreativeSpatialProjectionRequest projection;
  app::CreativeEditorOverlayFrame visible;
  app::CreativeEditorOverlayFrameRequest request{
      appState, editor, selection, gizmo, frame, projection};
  static_cast<void>(app::buildCreativeEditorWorldWireframes(request, visible));
  app::CreativeEditorOverlayFrame hidden;
  request.captureMode = true;
  static_cast<void>(app::buildCreativeEditorWorldWireframes(request, hidden));
  return expect(visible.logicLinkEdgeCount == 25U,
                "connect overlay draws one edge and two endpoint boxes") &&
         expect(hidden.logicLinkEdgeCount == 0U,
                "capture mode hides connect overlays");
}

}  // namespace

int main() {
  const bool ok = selectionLinkingAndHistoryAreOneKernel() &&
                  actionCycleAndDocumentSyncAreBounded() &&
                  connectInputMappingMatchesMouseAndControllerLanguage() &&
                  overlayShowsLinksOnlyInConnectMode();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
