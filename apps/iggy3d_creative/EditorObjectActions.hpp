#pragma once

#include <string>
#include <string_view>

#include "EditorToolOptions.hpp"

namespace iggy3d_creative_app {

[[nodiscard]] bool creativeEditorCommandIsObjectAction(
    CreativeEditorToolOptionsCommandId command) noexcept;

void refreshCreativeEditorObjectActionContext(
    const iggy3d::creative::CreativeAppState& appState,
    CreativeEditorToolOptionsState& state) noexcept;

[[nodiscard]] bool creativeEditorObjectActionEnabled(
    const CreativeEditorState& editor,
    const CreativeEditorToolOptionsState& state,
    CreativeEditorToolOptionsCommandId command) noexcept;

[[nodiscard]] std::string_view creativeEditorObjectActionLabel(
    CreativeEditorToolOptionsCommandId command) noexcept;

[[nodiscard]] std::string creativeEditorObjectActionValueLabel(
    const CreativeEditorToolOptionsState& state,
    CreativeEditorToolOptionsCommandId command);

[[nodiscard]] bool activateCreativeEditorObjectAction(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    CreativeEditorToolOptionsCommandId command);

}  // namespace iggy3d_creative_app
