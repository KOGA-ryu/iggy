#include "app/frontend/SaveBrowser.hpp"

namespace iggy3d {

SaveBrowserModel buildSaveBrowserModel(const SaveSlotList& slots,
                                       std::string_view selectedSaveId) {
  SaveBrowserModel model;
  model.slots = slots;
  model.selectedSaveId = selectedSaveId.empty() ? "none" : std::string(selectedSaveId);

  for (const SaveSlotPreview& slot : model.slots.slots) {
    if (slot.id == model.selectedSaveId) {
      model.loadEnabled = slot.enabled;
      model.deleteEnabled = true;
      model.status = slot.enabled ? "save_browser_selection_ready"
                                  : "save_browser_selection_disabled";
      return model;
    }
  }

  model.status = model.slots.slots.empty() ? "save_browser_empty"
                                           : "save_browser_selection_missing";
  return model;
}

}  // namespace iggy3d
