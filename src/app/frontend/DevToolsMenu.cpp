#include "app/frontend/DevToolsMenu.hpp"

namespace iggy3d {

DevToolsMenuModel buildDevToolsMenuModel(FrontendDevToolsCategory selected) {
  DevToolsMenuModel model;
  model.categories = devToolsCategoryOrder();
  model.selected = selected == FrontendDevToolsCategory::None
                       ? FrontendDevToolsCategory::Session
                       : selected;
  return model;
}

std::string_view devToolsCategoryLabel(FrontendDevToolsCategory category) {
  switch (category) {
    case FrontendDevToolsCategory::Session:
      return "Session";
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

}  // namespace iggy3d
