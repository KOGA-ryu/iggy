#include "app/frontend/MenuInput.hpp"

namespace iggy3d {

std::string_view menuInputActionName(MenuInputAction action) {
  switch (action) {
    case MenuInputAction::None:
      return "none";
    case MenuInputAction::Up:
      return "up";
    case MenuInputAction::Down:
      return "down";
    case MenuInputAction::Left:
      return "left";
    case MenuInputAction::Right:
      return "right";
    case MenuInputAction::Confirm:
      return "confirm";
    case MenuInputAction::Back:
      return "back";
    case MenuInputAction::NextTab:
      return "next_tab";
    case MenuInputAction::PreviousTab:
      return "previous_tab";
  }
  return "none";
}

std::string_view menuInputSourceName(MenuInputSource source) {
  switch (source) {
    case MenuInputSource::None:
      return "none";
    case MenuInputSource::Keyboard:
      return "keyboard";
    case MenuInputSource::Gamepad:
      return "gamepad";
    case MenuInputSource::Codex:
      return "codex";
    case MenuInputSource::Scripted:
      return "scripted";
  }
  return "none";
}

std::string_view menuOwnerName(MenuOwner owner) {
  switch (owner) {
    case MenuOwner::None:
      return "none";
    case MenuOwner::Starter:
      return "starter";
    case MenuOwner::Pause:
      return "pause";
    case MenuOwner::Settings:
      return "settings";
    case MenuOwner::DevTools:
      return "dev_tools";
    case MenuOwner::Editor:
      return "editor";
    case MenuOwner::Gameplay:
      return "gameplay";
  }
  return "none";
}

MenuOwner chooseMenuOwner(const MenuOwnerState& state) {
  if (state.starter) {
    return MenuOwner::Starter;
  }
  if (state.pause) {
    return MenuOwner::Pause;
  }
  if (state.settings) {
    return MenuOwner::Settings;
  }
  if (state.devTools) {
    return MenuOwner::DevTools;
  }
  if (state.editor) {
    return MenuOwner::Editor;
  }
  if (state.gameplay) {
    return MenuOwner::Gameplay;
  }
  return MenuOwner::None;
}

bool menuOwnerBlocksGameplay(MenuOwner owner) {
  return owner == MenuOwner::Starter || owner == MenuOwner::Pause ||
         owner == MenuOwner::Settings || owner == MenuOwner::DevTools ||
         owner == MenuOwner::Editor;
}

}  // namespace iggy3d
