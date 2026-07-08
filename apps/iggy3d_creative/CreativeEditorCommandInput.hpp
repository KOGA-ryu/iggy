#pragma once

#include <filesystem>
#include <string>

#include "app/iggy3d/creative/CreativeAppState.hpp"

#include "CreativeEditorState.hpp"

namespace iggy3d_creative_app {

void applyCreativeEditorCommandInput(
    const bool* keys,
    bool captureMode,
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    const std::filesystem::path& saveRoot,
    const std::string& saveId);

}  // namespace iggy3d_creative_app
