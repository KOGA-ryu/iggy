#pragma once

#include <string_view>

#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/ProductViewportState.hpp"

namespace iggy3d {

struct ActionState;

void applyProductCameraActions(const ActionState& actions,
                               ProductViewportState& viewport,
                               const FrontendSettings& settings,
                               std::string_view source);

}  // namespace iggy3d
