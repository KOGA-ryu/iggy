#pragma once

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/tools/Group.hpp"

#include <string_view>

namespace iggy3d_creative_app {

[[nodiscard]] iggy3d::creative::CreativeGroupCommandReceipt
applyCreativeEditorGroupCommandWithHistory(
    iggy3d::creative::CreativeAppState& appState,
    std::string_view source);

}  // namespace iggy3d_creative_app
