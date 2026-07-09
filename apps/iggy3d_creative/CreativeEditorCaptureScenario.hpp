#pragma once

#include <filesystem>
#include <string>

#include "app/iggy3d/creative/CreativeAppState.hpp"

#include "CreativeEditorState.hpp"

namespace iggy3d_creative_app {

void runCreativeEditorCaptureScenarioFrame(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    const std::filesystem::path& saveRoot,
    const std::string& saveId,
    bool captureMode);

}  // namespace iggy3d_creative_app
