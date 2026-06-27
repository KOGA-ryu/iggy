#include "app/iggy3d/menu/FrontendActionExecutor.hpp"

namespace iggy3d {

ProductFrontendActionResult executeProductFrontendAction(FrontendState& state,
                                                         FrontendAction action) {
  ProductFrontendActionResult result;
  result.action = action;
  state.selectedAction = action;

  switch (action) {
    case FrontendAction::Continue:
    case FrontendAction::CreateAndEnter:
    case FrontendAction::Load:
      result.launchRequested = true;
      result.status = "frontend_launch_requested";
      enterFrontendGameplay(state, action);
      break;
    case FrontendAction::ReturnToTitle:
      result.returnToTitleRequested = true;
      result.status = "frontend_return_to_title_requested";
      state.returnToTitleRequested = true;
      state.screen = FrontendScreen::Starter;
      break;
    case FrontendAction::Exit:
    case FrontendAction::ExitGame:
      result.quitRequested = true;
      result.status = "frontend_quit_requested";
      state.screen = FrontendScreen::ExitConfirm;
      break;
    case FrontendAction::Settings:
      result.status = "frontend_settings_opened";
      state.childScreen = FrontendScreen::Settings;
      break;
    case FrontendAction::DevTools:
      result.status = "frontend_dev_tools_opened";
      state.devToolsOpen = true;
      state.childScreen = FrontendScreen::StarterDevTools;
      break;
    case FrontendAction::Back:
      result.status = "frontend_back_requested";
      state.childScreen = FrontendScreen::Gameplay;
      break;
    default:
      result.status = "frontend_action_deferred";
      break;
  }

  return result;
}

}  // namespace iggy3d
