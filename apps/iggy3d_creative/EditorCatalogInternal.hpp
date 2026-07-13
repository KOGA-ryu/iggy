#pragma once

#include "EditorCatalog.hpp"

namespace iggy3d_creative_app {

[[nodiscard]] bool creativeEditorCatalogActionAvailable(
    const iggy3d::creative::CreativeAppState& appState,
    const CreativeEditorState& editor,
    iggy3d::creative::CreativeInputActionId action) noexcept;

}  // namespace iggy3d_creative_app
