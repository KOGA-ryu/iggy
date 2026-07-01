#pragma once

#include <cstdint>
#include <string_view>

#include "app/frontend/FrontendState.hpp"  // FrontendAction
#include "app/frontend/PauseMenu.hpp"       // PauseMenuModel, MenuRowModel
#include "app/iggy3d/menu/DrawList.hpp"      // ProductUiDrawList

namespace iggy3d {

struct ProductPauseUiRequest {
  const PauseMenuModel* model = nullptr;
  std::uint32_t virtualWidth = 1280;
  std::uint32_t virtualHeight = 720;
};

// Display label for a pause-menu action (the PauseMenuModel rows carry the action
// but no label). Falls back to the action's enum name so it is never empty.
std::string_view productPauseActionLabel(FrontendAction action);

// The in-game pause menu, built through the L1 widget layer and tagged with the
// Journal (Moleskine) theme — in the fiction, pausing opens the character's own
// journal. Pure request -> draw list, receipt-testable. Wiring the runtime to use
// this instead of the bespoke SDL pause draw is a separate step.
ProductUiDrawList buildProductPauseUiDrawList(const ProductPauseUiRequest& request);

}  // namespace iggy3d
