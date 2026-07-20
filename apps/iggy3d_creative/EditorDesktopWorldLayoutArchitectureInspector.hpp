#pragma once

#include "EditorDesktopCommands.hpp"
#include "EditorWorldLayout.hpp"

#include "app/iggy3d/creative/world/WorldLayoutDimensions.hpp"

namespace iggy3d_creative_app {

struct CreativeEditorDesktopUiState;

void drawCreativeEditorWorldLayoutArchitectureInspector(
    CreativeEditorDesktopUiState& desktopUi,
    CreativeEditorWorldLayoutState& state,
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::creative::CreativeWorldLayoutBuildingDimensions& dimensions,
    std::size_t buildingIndex,
    const iggy3d::creative::CreativeWorldLayoutBuilding& building,
    CreativeDesktopCommandFrame& commands);

}  // namespace iggy3d_creative_app
