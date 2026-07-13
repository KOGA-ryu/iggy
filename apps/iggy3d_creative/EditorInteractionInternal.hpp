#pragma once

#include "EditorInteraction.hpp"
#include "app/iggy3d/creative/input/HeldItemRegistry.hpp"

namespace iggy3d_creative_app {

inline constexpr float kCreativeEditorReachMeters = 128.0F;

void dispatchCreativeEditorHeldItemWorldOperation(
    iggy3d::creative::CreativeHeldItemWorldOperation operation,
    const CreativeEditorWorldInteractionFrameRequest& request,
    const iggy3d::creative::CreativeHotbarEntry& held);

void processCreativeEditorMoveInteraction(
    const CreativeEditorWorldInteractionFrameRequest& request);

void processCreativeEditorHeldItemFrame(
    const CreativeEditorWorldInteractionFrameRequest& request);

}  // namespace iggy3d_creative_app
