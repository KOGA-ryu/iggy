#pragma once

#include "EditorInteraction.hpp"

namespace iggy3d_creative_app {

inline constexpr float kCreativeEditorReachMeters = 128.0F;

void processCreativeEditorHeldItemFrame(
    const CreativeEditorWorldInteractionFrameRequest& request);

}  // namespace iggy3d_creative_app
