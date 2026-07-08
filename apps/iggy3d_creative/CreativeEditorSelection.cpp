#include "CreativeEditorSelection.hpp"

#include "StandalonePreviewProxies.hpp"

namespace iggy3d_creative_app {

CreativeEditorSelectionFrame resolveCreativeEditorSelectionFrame(
    const iggy3d::creative::Facade& facade) {
  CreativeEditorSelectionFrame selection;
  selection.selectedId = facade.selectionState().selectedTarget.value;
  selection.selected =
      selection.selectedId != 0
          ? facade.findObject(static_cast<iggy3d::creative::CreativeObjectId>(
                selection.selectedId))
          : nullptr;
  selection.hasSelection =
      selection.selected != nullptr && selection.selected->visible;
  if (selection.hasSelection) {
    const VisualBounds selectedVisualBounds =
        visualBoundsForObject(*selection.selected);
    selection.boxMin = selectedVisualBounds.min;
    selection.boxMax = selectedVisualBounds.max;
  }
  return selection;
}

}  // namespace iggy3d_creative_app
