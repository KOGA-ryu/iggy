#include "app/iggy3d/window/InputFrame.hpp"

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/menu/InputRouter.hpp"
#include "app/iggy3d/save/SaveSlotOperations.hpp"
#include "app/iggy3d/view/OpeningMenuHitTest.hpp"
#include "app/input/ActionState.hpp"
#include "app/platform/SdlWindow.hpp"
#include "app/iggy3d/window/InputFrameStages.hpp"

#include <array>

namespace iggy3d {
namespace {

struct OpeningMenuNavigationHitRow {
  OpeningMenuHitArea area = OpeningMenuHitArea::None;
  InputAction action = InputAction::None;
};

static constexpr std::array kOpeningMenuNavigationHitRows{
    OpeningMenuNavigationHitRow{OpeningMenuHitArea::DevToolsBack,
                                InputAction::MenuBack},
    OpeningMenuNavigationHitRow{OpeningMenuHitArea::SettingsBack,
                                InputAction::MenuBack},
    OpeningMenuNavigationHitRow{OpeningMenuHitArea::NewWorldCreate,
                                InputAction::MenuConfirm},
    OpeningMenuNavigationHitRow{OpeningMenuHitArea::NewWorldBack,
                                InputAction::MenuBack},
    OpeningMenuNavigationHitRow{OpeningMenuHitArea::NewWorldPreviousDungeon,
                                InputAction::MenuLeft},
    OpeningMenuNavigationHitRow{OpeningMenuHitArea::NewWorldNextDungeon,
                                InputAction::MenuRight},
    OpeningMenuNavigationHitRow{OpeningMenuHitArea::LoadSaveBack,
                                InputAction::MenuBack},
    OpeningMenuNavigationHitRow{OpeningMenuHitArea::DeleteConfirmConfirm,
                                InputAction::MenuConfirm},
    OpeningMenuNavigationHitRow{OpeningMenuHitArea::DeleteConfirmBack,
                                InputAction::MenuBack},
};

InputAction openingMenuNavigationActionFor(OpeningMenuHitArea area) {
  for (const OpeningMenuNavigationHitRow& row : kOpeningMenuNavigationHitRows) {
    // branch-gate: BG-1029
    if (row.area == area) {
      return row.action;
    }
  }
  return InputAction::None;
}

MouseClick productWindowClickForHitTest(MouseClick click,
                                        const SdlWindow* sdlWindow,
                                        std::uint32_t virtualWidth,
                                        std::uint32_t virtualHeight) {
  // branch-gate: BG-1123
  if (sdlWindow == nullptr) {
    return click;
  }
  const SdlWindowEventState& eventState = sdlWindow->eventState();
  return normalizeProductWindowMenuClick(click,
                                         eventState.windowWidth,
                                         eventState.windowHeight,
                                         virtualWidth,
                                         virtualHeight);
}

}  // namespace

MouseClick productWindowMenuClickForHitTest(MouseClick click,
                                            const SdlWindow* sdlWindow,
                                            std::uint32_t virtualWidth,
                                            std::uint32_t virtualHeight) {
  return productWindowClickForHitTest(click,
                                      sdlWindow,
                                      virtualWidth,
                                      virtualHeight);
}

void routeProductWindowMenuInput(InputAction inputAction,
                                 ActionState& actionState,
                                 ProductOpeningMenuInputContext context) {
  // branch-gate: BG-1029
  if (inputAction == InputAction::MenuBack &&
      cancelProductRoomEditorPendingPreviewFromBack(context.frontend,
                                                    context.window)) {
    recordAction(actionState,
                 InputAction::EditorCancelPreview,
                 true,
                 true,
                 false,
                 1.0F);
    return;
  }
  routeProductOpeningMenuInput(inputAction, actionState, context);
}

void dispatchProductOpeningMenuMouseHit(
    const OpeningMenuHitTestResult& hit,
    const MouseClick& click,
    ActionState& actionState,
    ProductOpeningMenuInputContext context) {
  const InputAction navigationAction = openingMenuNavigationActionFor(hit.area);
  // branch-gate: BG-1029
  if (navigationAction != InputAction::None) {
    routeProductOpeningMenuInput(navigationAction, actionState, context);
    return;
  }

  // branch-gate: BG-1029
  switch (hit.area) {
    case OpeningMenuHitArea::StarterAction:
      context.frontend.selectedAction = hit.action;
      recordAction(actionState,
                   mouseClickAction(click),
                   true,
                   true,
                   false,
                   1.0F);
      routeProductOpeningMenuInput(InputAction::MenuConfirm, actionState,
                                   context);
      return;
    case OpeningMenuHitArea::DevToolsCategory:
      context.frontend.devToolsCategory = hit.devToolsCategory;
      context.frontend.status = "dev_tools_category_selected";
      return;
    case OpeningMenuHitArea::SettingsTab:
      context.settingsTab = hit.settingsTab;
      context.frontend.status = "settings_tab_selected";
      return;
    case OpeningMenuHitArea::LoadSaveSlot:
      // branch-gate: BG-1122
      if (hit.saveSlotIndex < context.saves.slots.slots.size()) {
        (void)selectProductSaveSlotById(
            context.saves.slots,
            context.saves.slots.slots[hit.saveSlotIndex].id,
            context.window);
        context.frontend.status = "load_save_selection_changed";
      }
      return;
    case OpeningMenuHitArea::LoadSaveLoad:
      context.frontend.saveBrowserMode = FrontendSaveBrowserMode::Load;
      context.window.saveSession.saveSlotBrowserMode =
          std::string(frontendSaveBrowserModeName(context.frontend.saveBrowserMode));
      routeProductOpeningMenuInput(InputAction::MenuConfirm, actionState,
                                   context);
      return;
    case OpeningMenuHitArea::LoadSaveDelete:
      context.frontend.saveBrowserMode = FrontendSaveBrowserMode::Delete;
      context.window.saveSession.saveSlotBrowserMode =
          std::string(frontendSaveBrowserModeName(context.frontend.saveBrowserMode));
      routeProductOpeningMenuInput(InputAction::MenuConfirm, actionState,
                                   context);
      return;
    default:
      context.frontend.status = "opening_menu_hit_area_unhandled";
      return;
  }
}

MouseClick normalizeProductWindowMenuClick(MouseClick click,
                                           std::uint32_t windowWidth,
                                           std::uint32_t windowHeight,
                                           std::uint32_t virtualWidth,
                                           std::uint32_t virtualHeight) {
  // branch-gate: BG-1123
  if (!click.clicked || windowWidth == 0U || windowHeight == 0U ||
      virtualWidth == 0U || virtualHeight == 0U) {
    return click;
  }
  click.x = click.x * static_cast<float>(virtualWidth) /
            static_cast<float>(windowWidth);
  click.y = click.y * static_cast<float>(virtualHeight) /
            static_cast<float>(windowHeight);
  return click;
}

MouseClick resolveProductWindowInputMouseClick(
    ProductWindowInputClickOverride clickOverride,
    MouseInputState& mouse) {
  if (clickOverride.enabled) {
    return clickOverride.click;
  }
  return pollMouseClick(mouse);
}

}  // namespace iggy3d
