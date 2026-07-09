#include "CreativeEditorCaptureScenario.hpp"

#include "StandaloneCaptureScenario.hpp"
#include "StandaloneDelete.hpp"
#include "EditorGizmo.hpp"

namespace iggy3d_creative_app {

void runCreativeEditorCaptureScenarioFrame(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    const std::filesystem::path& saveRoot,
    const std::string& saveId,
    bool captureMode) {
  if (!captureMode) {
    return;
  }

  StandaloneCaptureScenarioStepRequest captureStep;
  captureStep.enabled = true;
  captureStep.frameIndex = editor.frameIndex;
  captureStep.appState = &appState;
  captureStep.undoStack = &editor.undoStack;
  captureStep.captureScript = &editor.captureScript;
  captureStep.placeBrush = &editor.placeBrush;
  captureStep.placeMode = &editor.placeMode;
  captureStep.placedCount = &editor.placedCount;
  captureStep.placeCellSize = editor.placeCellSize;
  captureStep.saveRoot = &saveRoot;
  captureStep.saveId = &saveId;
  captureStep.moveHeldAxisForX = heldAxisForGrabbedAxis(GizmoAxis::X);
  captureStep.moveHeldAxisForZ = heldAxisForGrabbedAxis(GizmoAxis::Z);
  captureStep.deleteSelected = [&](std::string_view source) {
    return deleteSelectedObject(appState, source, &editor.undoStack);
  };
  runStandaloneCaptureScenarioStep(captureStep);
}

}  // namespace iggy3d_creative_app
