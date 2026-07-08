#pragma once

#include <string_view>

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/document/DocumentMutation.hpp"

#include "StandaloneUndo.hpp"

namespace iggy3d_creative_app {

[[nodiscard]] iggy3d::creative::CreativeDocumentRemoveReceipt
deleteSelectedObject(iggy3d::creative::CreativeAppState& appState,
                     std::string_view source,
                     StandaloneUndoStack* undoStack = nullptr);

}  // namespace iggy3d_creative_app
