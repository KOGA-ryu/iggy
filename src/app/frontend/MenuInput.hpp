#pragma once

#include <cstdint>
#include <string_view>

namespace iggy3d {

enum class MenuInputAction : std::uint8_t {
  None,
  Up,
  Down,
  Left,
  Right,
  Confirm,
  Back,
  NextTab,
  PreviousTab,
};

enum class MenuInputSource : std::uint8_t {
  None,
  Keyboard,
  Gamepad,
  Codex,
  Scripted,
};

enum class MenuOwner : std::uint8_t {
  None,
  Starter,
  Pause,
  Settings,
  DevTools,
  Editor,
  Gameplay,
};

struct MenuOwnerState {
  bool starter = false;
  bool pause = false;
  bool settings = false;
  bool devTools = false;
  bool editor = false;
  bool gameplay = false;
};

std::string_view menuInputActionName(MenuInputAction action);
std::string_view menuInputSourceName(MenuInputSource source);
std::string_view menuOwnerName(MenuOwner owner);

MenuOwner chooseMenuOwner(const MenuOwnerState& state);
bool menuOwnerBlocksGameplay(MenuOwner owner);

}  // namespace iggy3d
