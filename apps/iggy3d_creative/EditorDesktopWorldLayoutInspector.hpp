#pragma once

#include "EditorDesktopCommands.hpp"
#include "EditorWorldLayout.hpp"

namespace iggy3d_creative_app {

void drawCreativeEditorWorldLayoutVerticalConnectorInspector(
    CreativeEditorWorldLayoutState& state,
    CreativeDesktopCommandFrame& commands);

void drawCreativeEditorWorldLayoutStructureInspector(
    CreativeEditorWorldLayoutState& state,
    CreativeDesktopCommandFrame& commands);

void drawCreativeEditorWorldLayoutSourceInspector(
    CreativeEditorWorldLayoutState& state,
    CreativeDesktopCommandFrame& commands);

}  // namespace iggy3d_creative_app
