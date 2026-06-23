#pragma once

#include <string_view>

#include "app/iggy3d/ProductViewportState.hpp"

namespace iggy3d {

struct ActionState;

void applyProductCameraActions(const ActionState& actions,
                               ProductViewportState& viewport,
                               std::string_view source);

}  // namespace iggy3d
