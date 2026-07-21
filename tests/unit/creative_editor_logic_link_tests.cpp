#include "EditorLogicLinks.hpp"
#include "app/iggy3d/creative/overlay/LogicLinkOverlay.hpp"
#include "EditorDesktopModel.hpp"
#include "EditorFrame.hpp"
#include "EditorGizmo.hpp"
#include "EditorPreviewFrameInternal.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/input/HeldItemRegistry.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"

#include <cmath>
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

cr::CreativeObjectId createGenerated(
    cr::Facade& facade,
    std::string name,
    cr::CreativeBounds bounds,
    std::vector<std::string> tags) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Crate;
  request.name = std::move(name);
  request.bounds = bounds;
  request.hasBoundsOverride = true;
  request.tags = std::move(tags);
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
                    afterPrevious == cr::CreativeLogicLinkAction::Reverse &&
                    next && afterNext == cr::CreativeLogicLinkAction::Toggle,
                "six-action cycle wraps in both directions") &&
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
  bool selectedToggleShaftVisible = false;
  for (const iggy3d::RenderCreativeWireframeDebugLine& line :
       inspected.combinedWireLines) {
    selectedToggleShaftVisible =
        selectedToggleShaftVisible ||
        (line.objectId == source && line.segmentKind == 0U &&
         line.color.b == 1.0F);
  }
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
                    selectedToggleShaftVisible,
                "selected circuit stays visible with directional Toggle styling") &&
         expect(hidden.logicLinkEdgeCount == 0U &&
                    hidden.logicLinkLabelGlyphCount == 0U,
                "capture mode hides connect overlays");
}

bool generatedScopeOverlaySharesHierarchyTruthAndFailsClosed() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Scope");
  static_cast<void>(document.assignId(704U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  cr::CreativeWorldLayout layout;
  layout.stableKey = "scope_layout";
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "house";
  building.name = "House";
  layout.buildings.push_back(std::move(building));
  layout.levels.push_back({0U, "ground", "Ground"});
  layout.levels.push_back({0U, "upper", "Upper", 3.0});
  layout.rooms.push_back(
      {0U, 0U, "west", "West", {{0, 0}, {4, 4}}, 0.25});
  layout.rooms.push_back(
      {0U, 0U, "east", "East", {{4, 0}, {8, 4}}, 0.25});
  layout.rooms.push_back(
      {0U, 1U, "upper", "Upper", {{0, 0}, {8, 4}}, 0.25});
  layout.verticalConnectors.push_back(
      {0U,
       0U,
       2U,
       cr::CreativeWorldLayoutVerticalConnectorKind::Stair,
       cr::CreativeWorldLayoutVerticalDirection::PositiveX,
       "stair",
       "Stair",
       {{1, 1}, {3, 3}}});
  const std::string layoutTag = cr::creativeWorldLayoutTag(layout.stableKey);
  const cr::CreativeObjectId west = createGenerated(
      appState.facade, "West", {{0.0, 0.0, 0.0}, {4.0, 0.2, 4.0}},
      {layoutTag, cr::creativeWorldLayoutProvenanceTag(
                      layout, cr::CreativeWorldLayoutTable::Room, 0U)});
  const cr::CreativeObjectId east = createGenerated(
      appState.facade, "East", {{4.0, 0.0, 0.0}, {8.0, 0.2, 4.0}},
      {layoutTag, cr::creativeWorldLayoutProvenanceTag(
                      layout, cr::CreativeWorldLayoutTable::Room, 1U)});
  const cr::CreativeObjectId shared = createGenerated(
      appState.facade, "Shared", {{3.8, 0.0, 0.0}, {4.2, 3.0, 4.0}},
      {layoutTag,
       cr::creativeWorldLayoutRoomEdgeProvenanceTag(
           layout, 0U, cr::CreativeWorldLayoutRoomEdge::East),
       cr::creativeWorldLayoutRoomEdgeProvenanceTag(
           layout, 1U, cr::CreativeWorldLayoutRoomEdge::West)});
  const cr::CreativeObjectId connector = createGenerated(
      appState.facade, "Stair", {{1.0, 0.0, 1.0}, {3.0, 3.0, 3.0}},
      {layoutTag, cr::creativeWorldLayoutProvenanceTag(
                      layout,
                      cr::CreativeWorldLayoutTable::VerticalConnector, 0U)});

  app::CreativeEditorState editor;
  editor.worldLayout.source = layout;
  editor.worldLayout.selection = {
      app::CreativeEditorWorldLayoutSelectionKind::Room, 1U};
  app::CreativeEditorSelectionFrame selection;
  selection.selectedId = static_cast<cr::Id>(east);
  selection.selected = appState.facade.findObject(east);
  selection.selectedObjectIds = {east};
  selection.selectionCount = 1U;
  selection.hasSelection = true;
  app::CreativeEditorGizmoFrame gizmo;
  iggy3d::FrameInput frame;
  frame.viewport.width = 800U;
  frame.viewport.height = 600U;
  cr::CreativeSpatialProjectionRequest projection;
  projection.gridSize = {32, 16, 32};
  app::CreativeEditorOverlayFrameRequest request{
      appState, editor, selection, gizmo, frame, projection};
  request.drawableWidth = 800U;
  request.drawableHeight = 600U;

  const auto hasColor = [](const app::CreativeEditorOverlayFrame& overlay,
                           cr::CreativeObjectId objectId, float r, float g,
                           float b) {
    for (const iggy3d::RenderCreativeWireframeDebugLine& line :
         overlay.combinedWireLines) {
      if (line.objectId == objectId && std::fabs(line.color.r - r) < 1.0e-6F &&
          std::fabs(line.color.g - g) < 1.0e-6F &&
          std::fabs(line.color.b - b) < 1.0e-6F) {
        return true;
      }
    }
    return false;
  };

  app::CreativeEditorOverlayFrame roomScope;
  static_cast<void>(
      app::buildCreativeEditorWorldWireframes(request, roomScope));
  editor.worldLayout.selection = {
      app::CreativeEditorWorldLayoutSelectionKind::Building, 0U};
  app::CreativeEditorOverlayFrame buildingScope;
  static_cast<void>(
      app::buildCreativeEditorWorldWireframes(request, buildingScope));
  editor.worldLayout.selection = {
      app::CreativeEditorWorldLayoutSelectionKind::Room, 1U};
  editor.worldLayout.generatedRevision = 0U;
  app::CreativeEditorOverlayFrame stale;
  static_cast<void>(app::buildCreativeEditorWorldWireframes(request, stale));

  return expect(roomScope.generatedScopeActive &&
                    roomScope.generatedScopeObjectCount == 2U &&
                    roomScope.generatedScopeVisibleObjectCount == 2U &&
                    roomScope.generatedScopeEdgeCount == 36U &&
                    !roomScope.architectureScaleGuideActive,
                "room scope renders two members plus one aggregate box") &&
         expect(hasColor(roomScope, east, 1.0F, 0.58F, 0.18F) &&
                    hasColor(roomScope, shared, 1.0F, 0.58F, 0.18F) &&
                    !hasColor(roomScope, west, 1.0F, 0.58F, 0.18F) &&
                    !hasColor(roomScope, connector, 1.0F, 0.58F, 0.18F),
                "room color covers its floor and shared edge only") &&
         expect(buildingScope.generatedScopeActive &&
                    buildingScope.generatedScopeObjectCount == 4U &&
                    buildingScope.generatedScopeEdgeCount == 60U &&
                    buildingScope.architectureScaleGuideActive &&
                    buildingScope.architectureScaleGuideLineCount == 13U &&
                    buildingScope.architecturalDimensions.accepted &&
                    buildingScope.architecturalDimensions.occupiedLevelCount ==
                        2U &&
                    std::fabs(buildingScope.architecturalDimensions
                                  .totalHeightMeters -
                              7.05) < 1.0e-9 &&
                    hasColor(buildingScope, west, 0.18F, 0.82F, 1.0F) &&
                    hasColor(buildingScope, connector, 0.18F, 0.82F, 1.0F),
                "building scope adds exact dimensions and a human guide") &&
         expect(!stale.generatedScopeActive &&
                    stale.generatedScopeEdgeCount == 0U &&
                    !stale.architectureScaleGuideActive &&
                    stale.architectureScaleGuideLineCount == 0U &&
                    hasColor(stale, east, 1.0F, 1.0F, 0.0F),
                "stale source disables scope and architectural scale claims");
}

}  // namespace

int main() {
  const bool ok = selectionLinkingAndHistoryAreOneKernel() &&
                  actionCycleAndDocumentSyncAreBounded() &&
                  explicitInspectorKernelsShareHistoryAndReceipts() &&
                  connectInputMappingMatchesMouseAndControllerLanguage() &&
                  directionalOverlayPlanClipsBoundsAndHandlesFailures() &&
                  overlayShowsLinksOnlyInConnectMode() &&
                  generatedScopeOverlaySharesHierarchyTruthAndFailsClosed();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
