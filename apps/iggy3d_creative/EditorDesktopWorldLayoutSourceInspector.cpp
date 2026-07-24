#include "EditorDesktopWorldLayoutSourceInspectorInternal.hpp"

namespace iggy3d_creative_app {

void drawCreativeEditorWorldLayoutRoofApertureInspector(
    CreativeEditorWorldLayoutState& state,
    std::size_t apertureIndex,
    CreativeDesktopCommandFrame& commands) {
  detail::drawRoofApertureInspectorForIndex(state, apertureIndex, commands);
}

void drawCreativeEditorWorldLayoutSourceInspector(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document,
    CreativeDesktopCommandFrame& commands) {
  detail::drawLevelInspector(state, commands);
  if (state.selection.kind ==
      CreativeEditorWorldLayoutSelectionKind::RoofAperture) {
    drawCreativeEditorWorldLayoutRoofApertureInspector(
        state, state.selection.index, commands);
  } else {
    state.roofApertureSettingsDraft = {};
  }
  detail::drawTerrainProfileInspector(state, document, commands);
  detail::drawTerrainPathInspector(state, document, commands);
  detail::drawObjectInspector(state, commands);
}

}  // namespace iggy3d_creative_app
