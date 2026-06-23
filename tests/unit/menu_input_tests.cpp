#include "app/frontend/MenuInput.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool namesAreStable() {
  return expect(iggy3d::menuInputActionName(iggy3d::MenuInputAction::Confirm) ==
                    "confirm",
                "confirm action name") &&
         expect(iggy3d::menuInputActionName(iggy3d::MenuInputAction::PreviousTab) ==
                    "previous_tab",
                "previous tab action name") &&
         expect(iggy3d::menuInputSourceName(iggy3d::MenuInputSource::Gamepad) ==
                    "gamepad",
                "gamepad source name") &&
         expect(iggy3d::menuOwnerName(iggy3d::MenuOwner::DevTools) == "dev_tools",
                "dev tools owner name");
}

bool ownerPriorityIsExact() {
  return expect(iggy3d::chooseMenuOwner(
                    iggy3d::MenuOwnerState{true, true, true, true, true, true}) ==
                    iggy3d::MenuOwner::Starter,
                "starter has priority") &&
         expect(iggy3d::chooseMenuOwner(
                    iggy3d::MenuOwnerState{false, true, true, true, true, true}) ==
                    iggy3d::MenuOwner::Pause,
                "pause priority") &&
         expect(iggy3d::chooseMenuOwner(
                    iggy3d::MenuOwnerState{false, false, true, true, true, true}) ==
                    iggy3d::MenuOwner::Settings,
                "settings priority") &&
         expect(iggy3d::chooseMenuOwner(
                    iggy3d::MenuOwnerState{false, false, false, true, true, true}) ==
                    iggy3d::MenuOwner::DevTools,
                "dev tools priority") &&
         expect(iggy3d::chooseMenuOwner(
                    iggy3d::MenuOwnerState{false, false, false, false, true, true}) ==
                    iggy3d::MenuOwner::Editor,
                "editor priority") &&
         expect(iggy3d::chooseMenuOwner(
                    iggy3d::MenuOwnerState{false, false, false, false, false, true}) ==
                    iggy3d::MenuOwner::Gameplay,
                "gameplay fallback");
}

bool blockingPolicyIsStable() {
  return expect(iggy3d::menuOwnerBlocksGameplay(iggy3d::MenuOwner::Starter),
                "starter blocks gameplay") &&
         expect(iggy3d::menuOwnerBlocksGameplay(iggy3d::MenuOwner::Pause),
                "pause blocks gameplay") &&
         expect(iggy3d::menuOwnerBlocksGameplay(iggy3d::MenuOwner::Settings),
                "settings blocks gameplay") &&
         expect(iggy3d::menuOwnerBlocksGameplay(iggy3d::MenuOwner::DevTools),
                "dev tools blocks gameplay") &&
         expect(iggy3d::menuOwnerBlocksGameplay(iggy3d::MenuOwner::Editor),
                "editor blocks gameplay") &&
         expect(!iggy3d::menuOwnerBlocksGameplay(iggy3d::MenuOwner::Gameplay),
                "gameplay does not block itself") &&
         expect(!iggy3d::menuOwnerBlocksGameplay(iggy3d::MenuOwner::None),
                "none does not block");
}

}  // namespace

int main() {
  const bool ok = namesAreStable() && ownerPriorityIsExact() && blockingPolicyIsStable();
  return ok ? 0 : 1;
}
