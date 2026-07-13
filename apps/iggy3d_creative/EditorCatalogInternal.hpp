#pragma once

#include "EditorCatalog.hpp"
#include "app/iggy3d/creative/input/UiInput.hpp"

namespace iggy3d_creative_app {

inline constexpr iggy3d::creative::CreativeWheelProfile kCatalogWheelProfile{
    iggy3d::creative::CreativeWheelPolarity::Reversed,
    iggy3d::creative::CreativeWheelStepMode::RoundedMagnitude,
    1.0e-4F};

[[nodiscard]] bool beginToolWheelAssignment(
    CreativeEditorCatalogState& state) noexcept;
void finishToolWheelAssignment(CreativeEditorCatalogState& state,
                               bool reopenCatalog) noexcept;
void processToolWheelInput(
    const CreativeEditorCatalogFrameRequest& request,
    CreativeEditorCatalogFrameResult& result);

[[nodiscard]] bool creativeEditorCatalogActionAvailable(
    const iggy3d::creative::CreativeAppState& appState,
    const CreativeEditorState& editor,
    iggy3d::creative::CreativeInputActionId action) noexcept;

}  // namespace iggy3d_creative_app
