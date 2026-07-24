#include "EditorCapture.hpp"
#include "EditorDesktopCommands.hpp"
#include "EditorFrame.hpp"
#include "EditorObjectActionExecutor.hpp"
#include "EditorObjectActions.hpp"
#include "EditorState.hpp"
#include "EditorToolOptions.hpp"
#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutHistory.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/history/History.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <span>
#include <string>
#include <string_view>

namespace {

namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeAppState makeAppState(std::string_view name,
                                  cr::CreativeDocumentId id) {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create(std::string(name));
  static_cast<void>(document.assignId(id));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  return appState;
}

cr::CreativeObjectId createCrate(
    cr::Facade& facade,
    const cr::CreativeTransform& transform = {}) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Crate;
  request.transform = transform;
  request.hasTransformOverride = true;
  return facade.createDocumentObject(request).objectId;
}

void selectOne(cr::Facade& facade, cr::CreativeObjectId objectId) {
  static_cast<void>(facade.selectTargets(
      std::span<const cr::CreativeObjectId>{&objectId, 1U}, objectId));
}

std::string_view latestDocumentHistorySource(
    const cr::CreativeAppState& appState) {
  return appState.history.undoSnapshots.empty()
             ? std::string_view{}
             : std::string_view{
                   appState.history.undoSnapshots.back().source};
}

cr::CreativeObjectId addGeneratedWorldLayoutObject(
    cr::CreativeAppState& appState,
    app::CreativeEditorWorldLayoutState& worldLayout,
    std::string_view layoutKey) {
  app::resetCreativeEditorWorldLayout(worldLayout, std::string(layoutKey));
  cr::CreativeWorldLayoutObject source;
  source.kind = cr::CreativeObjectKind::Crate;
  source.stableKey = "adapter_crate";
  source.name = "Adapter Crate";
  source.assetId = "crate";
  source.boundsCells = {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}};
  worldLayout.source.objects.push_back(source);
  worldLayout.generatedBaseline =
      app::captureCreativeEditorWorldLayoutSnapshot(worldLayout);
  worldLayout.sourceHistory.current.snapshot =
      app::captureCreativeEditorWorldLayoutSnapshot(worldLayout);

  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Crate;
  request.name = "Compiled Adapter Crate";
  request.tags = {
      cr::creativeWorldLayoutTag(worldLayout.source.stableKey),
      cr::creativeWorldLayoutProvenanceTag(
          worldLayout.source, cr::CreativeWorldLayoutTable::Object, 0U)};
  return appState.facade.createDocumentObject(request).objectId;
}

cr::CreativeInputRouteResult routedAction(
    cr::CreativeInputActionId action,
    cr::CreativeInputKey trigger = cr::CreativeInputKey::Delete) {
  cr::CreativeInputRouteResult routed;
  routed.context = cr::CreativeInputContext::EditorViewport;
  routed.actions[0] = {action, trigger};
  routed.actionCount = 1U;
  return routed;
}

app::CreativeDesktopCommandResult dispatchDesktop(
    cr::CreativeAppState& appState,
    app::CreativeEditorState& editor,
    app::CreativeDesktopCommandId id,
    app::CreativeDesktopCommandPayload payload = std::monostate{}) {
  app::CreativeDesktopCommandFrame frame;
  const app::CreativeDesktopCommandEnqueueResult enqueued =
      frame.push(id, std::move(payload));
  if (enqueued != app::CreativeDesktopCommandEnqueueResult::Enqueued) {
    return {};
  }
  std::string saveId = "adapter_parity";
  return app::dispatchCreativeDesktopCommands(
      frame, {appState, editor, std::filesystem::path{}, &saveId});
}

bool activateToolOption(
    cr::CreativeAppState& appState,
    app::CreativeEditorState& editor,
    app::CreativeEditorToolOptionsCommandId command) {
  app::CreativeEditorToolOptionsState& state = editor.toolOptions;
  state = {};
  state.open = true;
  state.targetEntry = {cr::CreativeHeldItemKind::ObjectMove,
                       cr::CreativeObjectKind::Unknown};
  state.draft = editor.toolSettings;
  state.options =
      app::creativeEditorToolOptionsForEntry(state.targetEntry, state.draft);
  app::refreshCreativeEditorObjectActionContext(
      appState, editor.authoredAssets, state, &editor.worldLayout);
  state.commands = app::creativeEditorToolOptionCommandsForEntry(
      state.targetEntry, state.contextPrimaryObjectKind,
      state.contextPatternRecipeId, state.contextPatternRecipeKind);
  const auto begin = state.commands.ids.begin();
  const auto end = begin + state.commands.count;
  const auto found = std::find(begin, end, command);
  if (found == end) {
    return false;
  }
  state.selectedIndex =
      state.options.count + static_cast<std::size_t>(found - begin);
  return app::activateCreativeEditorToolOptionsSelection(appState, editor);
}

enum class DeleteAdapter {
  Executor,
  Desktop,
  Input,
  ToolOptions,
  Capture,
};

struct GeneratedDeleteFacts {
  std::size_t sourceObjectCount = 0U;
  std::size_t compiledObjectCount = 0U;
  std::uint64_t documentUndoDepth = 0U;
  std::uint64_t sourceUndoDepth = 0U;
  std::size_t selectedCount = 0U;
  bool adapterAccepted = false;
  bool sourceChangedImpact = false;
  std::string status;
  std::string reasonCode;
};

GeneratedDeleteFacts runGeneratedDeleteAdapter(DeleteAdapter adapter,
                                               cr::CreativeDocumentId id) {
  cr::CreativeAppState appState =
      makeAppState("Generated Delete Adapter", id);
  app::CreativeEditorState editor;
  const cr::CreativeObjectId target = addGeneratedWorldLayoutObject(
      appState, editor.worldLayout, "adapter_generated_delete");
  selectOne(appState.facade, target);
  appState.history = {};

  GeneratedDeleteFacts facts;
  switch (adapter) {
    case DeleteAdapter::Executor: {
      const app::CreativeEditorObjectActionExecution execution =
          app::executeCreativeEditorSceneObjectAction(
              {appState, &editor.worldLayout},
              {app::CreativeEditorDeleteSelectionAction{},
               "adapter_direct_delete"});
      facts.adapterAccepted =
          app::creativeEditorObjectActionOutcomeAccepted(execution.outcome);
      facts.sourceChangedImpact =
          app::creativeEditorObjectActionHasIntegrationImpact(
              execution,
              app::CreativeEditorObjectActionIntegrationImpact::
                  WorldLayoutSourceChanged);
      facts.status = execution.status;
      facts.reasonCode = execution.outcome.reasonCode;
      break;
    }
    case DeleteAdapter::Desktop: {
      const app::CreativeDesktopCommandResult result =
          dispatchDesktop(appState, editor,
                          app::CreativeDesktopCommandId::DeleteSelection);
      facts.adapterAccepted = result.accepted;
      facts.sourceChangedImpact = result.worldLayoutChanged;
      facts.status = result.message;
      facts.reasonCode = result.objectAction.reasonCode;
      break;
    }
    case DeleteAdapter::Input:
      app::applyCreativeEditorCommandInput(
          routedAction(cr::CreativeInputActionId::DeleteSelection),
          appState, editor, {}, "adapter_input_delete");
      facts.adapterAccepted = editor.worldLayout.source.objects.empty();
      facts.sourceChangedImpact = facts.adapterAccepted;
      break;
    case DeleteAdapter::ToolOptions:
      facts.adapterAccepted = activateToolOption(
          appState, editor,
          app::CreativeEditorToolOptionsCommandId::DeleteSelection);
      facts.sourceChangedImpact = facts.adapterAccepted;
      break;
    case DeleteAdapter::Capture:
      editor.frameIndex = app::StandaloneCaptureScript::kDeleteFrame;
      editor.captureScript.deleteTargetId = target;
      app::runCreativeEditorCaptureScenarioFrame(
          appState, editor, std::filesystem::path{}, "adapter_capture_delete",
          true);
      facts.adapterAccepted = editor.captureScript.deleteAttempted;
      facts.sourceChangedImpact = editor.worldLayout.source.objects.empty();
      break;
  }

  facts.sourceObjectCount = editor.worldLayout.source.objects.size();
  facts.compiledObjectCount = appState.facade.document().objectCount();
  facts.documentUndoDepth = cr::creativeUndoDepth(appState.history);
  facts.sourceUndoDepth =
      app::creativeEditorWorldLayoutSourceUndoDepth(editor.worldLayout);
  facts.selectedCount =
      cr::selectedTargetCount(appState.facade.selectionState());
  return facts;
}

bool generatedDeleteAdaptersShareSourceEffects() {
  const GeneratedDeleteFacts direct =
      runGeneratedDeleteAdapter(DeleteAdapter::Executor, 7101U);
  const GeneratedDeleteFacts desktop =
      runGeneratedDeleteAdapter(DeleteAdapter::Desktop, 7102U);
  const GeneratedDeleteFacts input =
      runGeneratedDeleteAdapter(DeleteAdapter::Input, 7103U);
  const GeneratedDeleteFacts tool =
      runGeneratedDeleteAdapter(DeleteAdapter::ToolOptions, 7104U);
  const GeneratedDeleteFacts capture =
      runGeneratedDeleteAdapter(DeleteAdapter::Capture, 7105U);

  const auto stateEffectsMatch = [&](const GeneratedDeleteFacts& facts) {
    return facts.adapterAccepted && facts.sourceChangedImpact &&
           facts.sourceObjectCount == 0U &&
           facts.compiledObjectCount == 1U &&
           facts.documentUndoDepth == 0U && facts.sourceUndoDepth == 1U &&
           facts.selectedCount == 1U;
  };
  return expect(stateEffectsMatch(direct),
                "direct generated delete establishes canonical effects") &&
         expect(stateEffectsMatch(desktop) &&
                    desktop.status == direct.status &&
                    desktop.reasonCode == direct.reasonCode,
                "Desktop generated delete preserves outcome and effects") &&
         expect(stateEffectsMatch(input),
                "input generated delete preserves source effects") &&
         expect(stateEffectsMatch(tool),
                "Tool Options generated delete preserves source effects") &&
         expect(stateEffectsMatch(capture),
                "Capture generated delete preserves source effects");
}

enum class DuplicateAdapter {
  Executor,
  Desktop,
  Input,
  ToolOptions,
};

struct DuplicateFacts {
  std::size_t objectCount = 0U;
  std::uint64_t undoDepth = 0U;
  std::size_t selectedCount = 0U;
  bool primaryChanged = false;
  bool adapterAccepted = false;
  std::string historySource;
  std::string status;
  std::string reasonCode;
};

DuplicateFacts runDuplicateAdapter(DuplicateAdapter adapter,
                                   cr::CreativeDocumentId id) {
  cr::CreativeAppState appState = makeAppState("Duplicate Adapter", id);
  app::CreativeEditorState editor;
  const cr::CreativeObjectId original = createCrate(appState.facade);
  selectOne(appState.facade, original);
  appState.history = {};

  DuplicateFacts facts;
  switch (adapter) {
    case DuplicateAdapter::Executor: {
      const app::CreativeEditorObjectActionExecution execution =
          app::executeCreativeEditorSceneObjectAction(
              {appState, &editor.worldLayout},
              {app::CreativeEditorDuplicateSelectionAction{},
               "adapter_direct_duplicate"});
      facts.adapterAccepted =
          app::creativeEditorObjectActionOutcomeAccepted(execution.outcome);
      facts.status = execution.status;
      facts.reasonCode = execution.outcome.reasonCode;
      break;
    }
    case DuplicateAdapter::Desktop: {
      const app::CreativeDesktopCommandResult result =
          dispatchDesktop(appState, editor,
                          app::CreativeDesktopCommandId::DuplicateSelection);
      facts.adapterAccepted = result.accepted;
      facts.status = result.message;
      facts.reasonCode = result.objectAction.reasonCode;
      break;
    }
    case DuplicateAdapter::Input:
      app::applyCreativeEditorCommandInput(
          routedAction(cr::CreativeInputActionId::DuplicateSelection,
                       cr::CreativeInputKey::D),
          appState, editor, {}, "adapter_input_duplicate");
      facts.adapterAccepted = appState.facade.document().objectCount() == 2U;
      break;
    case DuplicateAdapter::ToolOptions:
      facts.adapterAccepted = activateToolOption(
          appState, editor,
          app::CreativeEditorToolOptionsCommandId::DuplicateSelection);
      break;
  }

  facts.objectCount = appState.facade.document().objectCount();
  facts.undoDepth = cr::creativeUndoDepth(appState.history);
  facts.selectedCount =
      cr::selectedTargetCount(appState.facade.selectionState());
  facts.primaryChanged =
      appState.facade.selectionState().selectedTarget.value !=
      static_cast<cr::Id>(original);
  facts.historySource = std::string(latestDocumentHistorySource(appState));
  return facts;
}

bool duplicateAdaptersShareMutationAndSelectionEffects() {
  const DuplicateFacts direct =
      runDuplicateAdapter(DuplicateAdapter::Executor, 7111U);
  const DuplicateFacts desktop =
      runDuplicateAdapter(DuplicateAdapter::Desktop, 7112U);
  const DuplicateFacts input =
      runDuplicateAdapter(DuplicateAdapter::Input, 7113U);
  const DuplicateFacts tool =
      runDuplicateAdapter(DuplicateAdapter::ToolOptions, 7114U);
  const auto effectsMatch = [](const DuplicateFacts& facts) {
    return facts.adapterAccepted && facts.objectCount == 2U &&
           facts.undoDepth == 1U && facts.selectedCount == 1U &&
           facts.primaryChanged;
  };
  return expect(effectsMatch(direct) &&
                    direct.historySource == "adapter_direct_duplicate",
                "direct duplicate establishes canonical document effects") &&
         expect(effectsMatch(desktop) &&
                    desktop.historySource == "desktop_duplicate" &&
                    desktop.status == direct.status &&
                    desktop.reasonCode == direct.reasonCode,
                "Desktop duplicate preserves outcome and effects") &&
         expect(effectsMatch(input) &&
                    input.historySource == "keyboard_duplicate",
                "input duplicate preserves mutation and selection effects") &&
         expect(effectsMatch(tool) &&
                    tool.historySource == "object_actions_duplicate",
                "Tool Options duplicate preserves mutation and selection "
                "effects");
}

bool transformAdaptersShareAppliedAndResetEffects() {
  cr::CreativeAppState directRotate =
      makeAppState("Direct Rotate Adapter", 7121U);
  cr::CreativeAppState inputRotate =
      makeAppState("Input Rotate Adapter", 7122U);
  app::CreativeEditorState directRotateEditor;
  app::CreativeEditorState inputRotateEditor;
  const cr::CreativeObjectId directRotateId =
      createCrate(directRotate.facade);
  const cr::CreativeObjectId inputRotateId = createCrate(inputRotate.facade);
  selectOne(directRotate.facade, directRotateId);
  selectOne(inputRotate.facade, inputRotateId);
  directRotate.history = {};
  inputRotate.history = {};

  cr::CreativeTransformCommandRequest rotate;
  rotate.kind = cr::CreativeTransformCommandKind::RotateYaw;
  rotate.yawDegrees = cr::creativeRotationStepDegrees(
      inputRotateEditor.toolSettings.rotationStep);
  const app::CreativeEditorObjectActionExecution directRotated =
      app::executeCreativeEditorSceneObjectAction(
          {directRotate, &directRotateEditor.worldLayout},
          {app::CreativeEditorTransformSelectionAction{rotate},
           "adapter_direct_rotate"});
  app::applyCreativeEditorCommandInput(
      routedAction(cr::CreativeInputActionId::RotateYawPositive,
                   cr::CreativeInputKey::R),
      inputRotate, inputRotateEditor, {}, "adapter_input_rotate");

  cr::CreativeTransform seeded;
  seeded.rotationEulerRadians = {0.2, 0.4, 0.6};
  seeded.scale = {1.5, 0.75, 2.0};
  cr::CreativeAppState directReset =
      makeAppState("Direct Reset Adapter", 7123U);
  cr::CreativeAppState toolReset =
      makeAppState("Tool Reset Adapter", 7124U);
  app::CreativeEditorState directResetEditor;
  app::CreativeEditorState toolResetEditor;
  const cr::CreativeObjectId directResetId =
      createCrate(directReset.facade, seeded);
  const cr::CreativeObjectId toolResetId =
      createCrate(toolReset.facade, seeded);
  selectOne(directReset.facade, directResetId);
  selectOne(toolReset.facade, toolResetId);
  directReset.history = {};
  toolReset.history = {};
  cr::CreativeTransformCommandRequest reset;
  reset.kind = cr::CreativeTransformCommandKind::ResetRotationScale;
  const app::CreativeEditorObjectActionExecution directResetExecution =
      app::executeCreativeEditorSceneObjectAction(
          {directReset, &directResetEditor.worldLayout},
          {app::CreativeEditorTransformSelectionAction{reset},
           "adapter_direct_reset"});
  const bool toolResetAccepted = activateToolOption(
      toolReset, toolResetEditor,
      app::CreativeEditorToolOptionsCommandId::ResetSelectionTransform);

  const cr::CreativeObject* directRotatedObject =
      directRotate.facade.findObject(directRotateId);
  const cr::CreativeObject* inputRotatedObject =
      inputRotate.facade.findObject(inputRotateId);
  const cr::CreativeObject* directResetObject =
      directReset.facade.findObject(directResetId);
  const cr::CreativeObject* toolResetObject =
      toolReset.facade.findObject(toolResetId);
  return expect(
             app::creativeEditorObjectActionOutcomeAccepted(
                 directRotated.outcome) &&
                 directRotatedObject != nullptr &&
                 inputRotatedObject != nullptr &&
                 cr::creativeVec3ExactlyEqual(
                     directRotatedObject->transform.rotationEulerRadians,
                     inputRotatedObject->transform.rotationEulerRadians) &&
                 cr::creativeUndoDepth(directRotate.history) == 1U &&
                 cr::creativeUndoDepth(inputRotate.history) == 1U &&
                 latestDocumentHistorySource(inputRotate) ==
                     "keyboard_rotate_yaw_positive",
             "input rotate matches the typed executor mutation") &&
         expect(
             app::creativeEditorObjectActionOutcomeAccepted(
                 directResetExecution.outcome) &&
                 toolResetAccepted && directResetObject != nullptr &&
                 toolResetObject != nullptr &&
                 cr::creativeVec3ExactlyEqual(
                     directResetObject->transform.rotationEulerRadians,
                     toolResetObject->transform.rotationEulerRadians) &&
                 cr::creativeVec3ExactlyEqual(
                     directResetObject->transform.scale,
                     toolResetObject->transform.scale) &&
                 cr::creativeUndoDepth(directReset.history) == 1U &&
                 cr::creativeUndoDepth(toolReset.history) == 1U &&
                 latestDocumentHistorySource(toolReset) ==
                     "object_actions_reset_transform",
             "Tool Options reset matches the typed executor mutation");
}

bool desktopAbsoluteTransformMatchesExecutor() {
  cr::CreativeAppState direct =
      makeAppState("Direct Absolute Adapter", 7131U);
  cr::CreativeAppState desktop =
      makeAppState("Desktop Absolute Adapter", 7132U);
  app::CreativeEditorState directEditor;
  app::CreativeEditorState desktopEditor;
  const cr::CreativeObjectId directId = createCrate(direct.facade);
  const cr::CreativeObjectId desktopId = createCrate(desktop.facade);
  direct.history = {};
  desktop.history = {};
  cr::CreativeTransform target;
  target.position = {4.0, 2.0, -3.0};
  target.rotationEulerRadians = {0.0, 0.75, 0.0};

  const app::CreativeEditorObjectActionExecution directExecution =
      app::executeCreativeEditorSceneObjectAction(
          {direct, &directEditor.worldLayout},
          {app::CreativeEditorSetObjectTransformAction{
               directId, target, true, true, false},
           "adapter_direct_absolute"});
  const app::CreativeDesktopCommandResult desktopResult = dispatchDesktop(
      desktop, desktopEditor,
      app::CreativeDesktopCommandId::SetObjectTransform,
      app::CreativeDesktopTransformPayload{
          desktopId, target, true, true, false});
  const cr::CreativeObject* directObject = direct.facade.findObject(directId);
  const cr::CreativeObject* desktopObject =
      desktop.facade.findObject(desktopId);

  return expect(
      app::creativeEditorObjectActionOutcomeAccepted(directExecution.outcome) &&
          desktopResult.accepted &&
          desktopResult.objectAction.status == directExecution.outcome.status &&
          desktopResult.objectAction.reasonCode ==
              directExecution.outcome.reasonCode &&
          desktopResult.message == directExecution.status &&
          directObject != nullptr && desktopObject != nullptr &&
          cr::creativeVec3ExactlyEqual(directObject->transform.position,
                                       desktopObject->transform.position) &&
          cr::creativeVec3ExactlyEqual(
              directObject->transform.rotationEulerRadians,
              desktopObject->transform.rotationEulerRadians) &&
          cr::creativeUndoDepth(direct.history) == 1U &&
          cr::creativeUndoDepth(desktop.history) == 1U &&
          latestDocumentHistorySource(desktop) == "desktop_set_transform",
      "Desktop absolute transform matches the typed executor outcome");
}

}  // namespace

int main() {
  const bool ok = generatedDeleteAdaptersShareSourceEffects() &&
                  duplicateAdaptersShareMutationAndSelectionEffects() &&
                  transformAdaptersShareAppliedAndResetEffects() &&
                  desktopAbsoluteTransformMatchesExecutor();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
