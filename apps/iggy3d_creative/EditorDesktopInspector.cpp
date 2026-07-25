#include "EditorDesktopInspectorInternal.hpp"

#include "EditorObjectActions.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;

void buildCreativeEditorDesktopInspectorPanel(
    CreativeEditorDesktopUiState& desktopUi,
    CreativeEditorState& editor,
    const cr::CreativeAppState& appState,
    const CreativeEditorUiInputFrame& input,
    CreativeDesktopCommandFrame& commands) {
  const cr::CreativeDocument& document = appState.facade.document();
  constexpr bool playModeActive = false;

  const CreativeDesktopLiveSelection live =
      creativeDesktopLiveSelection(
          appState.facade.selectionState());
  const CreativeDesktopSelectionResolution resolved =
      resolveCreativeDesktopSelection(
          document, live.objectIds, live.primaryObjectId);
  const CreativeDesktopObjectActionContext& actionContext =
      refreshCreativeEditorDesktopObjectActionContext(
          desktopUi, appState, &editor.worldLayout);

  appendCreativeDesktopMeasurementInspector(
      appState.facade.measurementState(),
      document.measurementAnnotationStore(), playModeActive, commands);
  appendCreativeDesktopActiveTransformInspector(
      editor, appState, playModeActive);
  appendCreativeDesktopVolumeInspector(
      editor, document, resolved.objectIds);

  if (resolved.objectIds.empty()) {
    ImGui::TextUnformatted("No object selected");
    return;
  }

  const cr::CreativeObject* object =
      resolved.objectIds.size() == 1U
          ? document.findObject(resolved.primaryObjectId)
          : nullptr;
  const CreativeDesktopInspectorSelectionModel selection{
      document, resolved, actionContext, object, playModeActive};
  if (resolved.objectIds.size() > 1U) {
    appendCreativeDesktopMultiSelectionInspector(selection, commands);
    return;
  }
  if (object == nullptr) {
    ImGui::TextUnformatted("No object selected");
    return;
  }

  appendCreativeDesktopSingleObjectInspector(
      desktopUi, selection, editor.worldLayout,
      editor.generatedSourceScopeCache,
      editor.movingPlatformPreview,
      editor.interaction.movingPlatformPathEdit, input, commands);
  appendCreativeDesktopLogicInspector(
      desktopUi, editor.logicLinks, selection, commands);
}

}  // namespace iggy3d_creative_app
