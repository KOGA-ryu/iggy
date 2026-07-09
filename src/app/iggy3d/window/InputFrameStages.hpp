#pragma once

#include <cstdint>

#include "app/iggy3d/menu/InputRouter.hpp"
#include "app/input/ActionState.hpp"
#include "app/input/MouseInput.hpp"

namespace iggy3d {

class SdlWindow;

MouseClick productWindowMenuClickForHitTest(
    MouseClick click,
    const SdlWindow* sdlWindow,
    std::uint32_t virtualWidth = 1280U,
    std::uint32_t virtualHeight = 720U);

void routeProductWindowMenuInput(InputAction inputAction,
                                 ActionState& actionState,
                                 ProductOpeningMenuInputContext context);

}  // namespace iggy3d
