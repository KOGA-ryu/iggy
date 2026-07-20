#pragma once

#include "EditorDesktopCommands.hpp"
#include "EditorWorldLayout.hpp"

namespace iggy3d_creative_app {

struct CreativeEditorDesktopUiState;

[[nodiscard]] const char* creativeEditorWorldLayoutVerticalConnectorKindLabel(
    iggy3d::creative::CreativeWorldLayoutVerticalConnectorKind kind) noexcept;
[[nodiscard]] const char*
creativeEditorWorldLayoutVerticalConnectorDirectionLabel(
    iggy3d::creative::CreativeWorldLayoutVerticalDirection direction) noexcept;
[[nodiscard]] iggy3d::creative::CreativeWorldLayoutVerticalDirection
oppositeCreativeEditorWorldLayoutVerticalConnectorDirection(
    iggy3d::creative::CreativeWorldLayoutVerticalDirection direction) noexcept;

void drawCreativeEditorWorldLayoutVerticalConnectorInspector(
    CreativeEditorWorldLayoutState& state,
    CreativeDesktopCommandFrame& commands);

void drawCreativeEditorWorldLayoutStructureInspector(
    CreativeEditorDesktopUiState& desktopUi,
    CreativeEditorWorldLayoutState& state,
    const iggy3d::creative::CreativeDocument& document,
    CreativeDesktopCommandFrame& commands);

void drawCreativeEditorWorldLayoutSourceInspector(
    CreativeEditorWorldLayoutState& state,
    const iggy3d::creative::CreativeDocument& document,
    CreativeDesktopCommandFrame& commands);

}  // namespace iggy3d_creative_app
