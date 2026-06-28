#include "app/frontend/DevToolsMenu.hpp"

namespace iggy3d {
namespace {

bool validDevToolsParent(MenuOwner parentOwner) {
  return parentOwner == MenuOwner::Starter || parentOwner == MenuOwner::Gameplay ||
         parentOwner == MenuOwner::Pause;
}

FrontendScreen devToolsStayScreen(MenuOwner parentOwner) {
  if (parentOwner == MenuOwner::Starter) {
    return FrontendScreen::Starter;
  }
  return FrontendScreen::DevOverlay;
}

FrontendScreen devToolsStayChild(MenuOwner parentOwner) {
  if (parentOwner == MenuOwner::Starter) {
    return FrontendScreen::StarterDevTools;
  }
  return FrontendScreen::Gameplay;
}

FrontendRouteResult ignoredDevToolsRoute(MenuOwner parentOwner,
                                         FrontendAction action,
                                         std::string_view status) {
  FrontendRouteResult result = makeIgnoredFrontendRouteResult(
      MenuOwner::DevTools,
      devToolsStayScreen(parentOwner),
      devToolsStayChild(parentOwner),
      action);
  result.gameplayInputSuppressed = true;
  result.status = status;
  result.receiptReason = status;
  return result;
}

FrontendRouteResult acceptedDevToolsRoute(MenuOwner owner,
                                          FrontendScreen nextScreen,
                                          FrontendScreen nextChildScreen,
                                          bool gameplayInputSuppressed,
                                          std::string_view status,
                                          FrontendAction action) {
  return makeAcceptedFrontendRouteResult(owner,
                                         nextScreen,
                                         nextChildScreen,
                                         FrontendTransitionRequest::None,
                                         false,
                                         gameplayInputSuppressed,
                                         status,
                                         status,
                                         action);
}

}  // namespace

DevToolsMenuModel buildDevToolsMenuModel(FrontendDevToolsCategory selected) {
  DevToolsMenuModel model;
  model.categories = devToolsCategoryOrder();
  model.selected = selected == FrontendDevToolsCategory::None
                       ? FrontendDevToolsCategory::Session
                       : selected;
  model.selectedEnabled = true;
  model.selectedDisabledReason = "none";
  model.selectedAction = "none";
  model.runtimeReadoutCount = devToolsReadoutCount(model.selected);
  return model;
}

FrontendRouteResult routeDevToolsAction(const DevToolsMenuModel& model,
                                        MenuOwner parentOwner,
                                        FrontendAction action) {
  if (action != FrontendAction::Back && action != FrontendAction::Apply) {
    return ignoredDevToolsRoute(parentOwner, action, "not_dev_tools_action");
  }

  if (action == FrontendAction::Back) {
    switch (parentOwner) {
      case MenuOwner::Starter:
        return acceptedDevToolsRoute(MenuOwner::Starter,
                                     FrontendScreen::Starter,
                                     FrontendScreen::Gameplay,
                                     true,
                                     "dev_tools_closed_to_starter",
                                     action);
      case MenuOwner::Gameplay:
        return acceptedDevToolsRoute(MenuOwner::Gameplay,
                                     FrontendScreen::Gameplay,
                                     FrontendScreen::Gameplay,
                                     false,
                                     "dev_tools_closed_to_gameplay",
                                     action);
      case MenuOwner::Pause:
        return acceptedDevToolsRoute(MenuOwner::Pause,
                                     FrontendScreen::Pause,
                                     FrontendScreen::Gameplay,
                                     true,
                                     "dev_tools_closed_to_pause",
                                     action);
      case MenuOwner::None:
      case MenuOwner::Settings:
      case MenuOwner::DevTools:
      case MenuOwner::Editor:
        break;
    }
    return ignoredDevToolsRoute(parentOwner, action, "dev_tools_invalid_parent");
  }

  if (!validDevToolsParent(parentOwner)) {
    return ignoredDevToolsRoute(parentOwner, action, "dev_tools_invalid_parent");
  }
  if (!model.selectedEnabled) {
    return ignoredDevToolsRoute(parentOwner, action, model.selectedDisabledReason);
  }

  return acceptedDevToolsRoute(MenuOwner::DevTools,
                               devToolsStayScreen(parentOwner),
                               devToolsStayChild(parentOwner),
                               true,
                               "dev_tools_category_selected",
                               action);
}

std::string_view devToolsCategoryLabel(FrontendDevToolsCategory category) {
  switch (category) {
    case FrontendDevToolsCategory::Session:
      return "Session";
    case FrontendDevToolsCategory::Input:
      return "Input";
    case FrontendDevToolsCategory::Player:
      return "Player";
    case FrontendDevToolsCategory::Movement:
      return "Movement";
    case FrontendDevToolsCategory::WorldEditor:
      return "World Editor";
    case FrontendDevToolsCategory::Collision:
      return "Collision";
    case FrontendDevToolsCategory::Spells:
      return "Spells";
    case FrontendDevToolsCategory::Camera:
      return "Camera";
    case FrontendDevToolsCategory::Renderer:
      return "Renderer";
    case FrontendDevToolsCategory::Performance:
      return "Performance";
    case FrontendDevToolsCategory::None:
      return "None";
  }
  return "None";
}

std::string_view devToolsFunctionKeyHintLabel() {
  return "F1 CLOSE  F2 COLLISION  F3 DEBUG HUD  F4 TUNING";
}

std::uint64_t devToolsReadoutCount(FrontendDevToolsCategory category) {
  switch (category) {
    case FrontendDevToolsCategory::Session:
      return 5;
    case FrontendDevToolsCategory::Input:
      return 4;
    case FrontendDevToolsCategory::Player:
    case FrontendDevToolsCategory::Movement:
    case FrontendDevToolsCategory::WorldEditor:
    case FrontendDevToolsCategory::Collision:
    case FrontendDevToolsCategory::Spells:
    case FrontendDevToolsCategory::Camera:
    case FrontendDevToolsCategory::Renderer:
    case FrontendDevToolsCategory::Performance:
      return 3;
    case FrontendDevToolsCategory::None:
      return 0;
  }
  return 0;
}

}  // namespace iggy3d
