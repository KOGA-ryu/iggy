#include "app/frontend/DevToolsMenu.hpp"

namespace iggy3d {

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
