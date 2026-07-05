// TV1-H (TD-8): the creative Navigate tool feeds ONLY the fly-movement keys
// into the shared CreativeFly kernel. These tests pin the WASD gating decision
// (which actions the poll records, and just as importantly which it never
// records) plus that the recorded axes drive the reused fly kernel. Real SDL
// mouse-look / capture cannot run headless, so the live poll is not exercised;
// the pure record function IS the gating seam.

#include "app/input/KeyboardInput.hpp"

#include <cmath>
#include <iostream>
#include <string_view>

#include "app/input/ActionState.hpp"
#include "app/input/InputAction.hpp"
#include "app/iggy3d/creative/camera/Fly.hpp"

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(float lhs, float rhs) { return std::fabs(lhs - rhs) <= 0.0001F; }

bool flyPollRecordsOnlyFlyAxes() {
  iggy3d::KeyboardCreativeFlyInputSample sample;
  sample.forwardDown = true;
  sample.rightDown = true;
  sample.upDown = true;
  sample.sprintDown = true;
  iggy3d::ActionState actions;
  iggy3d::recordKeyboardCreativeFlyActions(sample, actions);

  const bool axesPresent =
      near(iggy3d::actionAxisValue(actions, iggy3d::InputAction::PlayerMoveY),
           1.0F) &&
      near(iggy3d::actionAxisValue(actions, iggy3d::InputAction::PlayerMoveX),
           1.0F) &&
      iggy3d::actionIsDown(actions, iggy3d::InputAction::PlayerJump) &&
      iggy3d::actionIsDown(actions, iggy3d::InputAction::PlayerSprint);

  // The general gameplay verbs must never appear — creative document mode keeps
  // the keyboard dead beyond the fly axes (no jump-press edge, no interact,
  // dash, or retry).
  bool leakedGameplayVerb = false;
  for (const iggy3d::ActionStateEntry& entry : actions.entries) {
    if (entry.action == iggy3d::InputAction::PlayerInteract ||
        entry.action == iggy3d::InputAction::PlayerDash ||
        entry.action == iggy3d::InputAction::PlayerRetryOrReset) {
      leakedGameplayVerb = true;
    }
    // PlayerJump is recorded as a HELD axis for fly-up, never as a pressed edge.
    if (entry.action == iggy3d::InputAction::PlayerJump && entry.pressed) {
      leakedGameplayVerb = true;
    }
  }

  return expect(axesPresent, "fly poll records the fly axes") &&
         expect(!leakedGameplayVerb,
                "fly poll records no general gameplay verb");
}

bool downKeyIsFlyDownNotCrouchVerb() {
  iggy3d::KeyboardCreativeFlyInputSample sample;
  sample.downDown = true;
  iggy3d::ActionState actions;
  iggy3d::recordKeyboardCreativeFlyActions(sample, actions);
  // Down is the fly vertical, carried on PlayerCrouch as a held axis.
  return expect(iggy3d::actionIsDown(actions, iggy3d::InputAction::PlayerCrouch),
                "down key drives fly-down via crouch axis");
}

bool idleRecordsNothing() {
  iggy3d::KeyboardCreativeFlyInputSample sample;
  iggy3d::ActionState actions;
  iggy3d::recordKeyboardCreativeFlyActions(sample, actions);
  return expect(actions.entries.empty(), "idle records no actions");
}

bool recordedAxesDriveTheReusedKernel() {
  // Prove the recorded fly actions move the SAME CreativeFly kernel map_maker
  // uses: forward (W) at yaw 0 walks -Z.
  iggy3d::KeyboardCreativeFlyInputSample sample;
  sample.forwardDown = true;
  iggy3d::ActionState actions;
  iggy3d::recordKeyboardCreativeFlyActions(sample, actions);

  iggy3d::ProductCreativeFlyConfig config;
  config.enabled = true;
  config.speedMetersPerSecond = 6.0F;
  config.inputStepSeconds = 0.5F;
  iggy3d::ProductCreativeFlyInput input;
  input.moveX = iggy3d::actionAxisValue(actions, iggy3d::InputAction::PlayerMoveX);
  input.moveY = iggy3d::actionAxisValue(actions, iggy3d::InputAction::PlayerMoveY);
  const float up =
      iggy3d::actionIsDown(actions, iggy3d::InputAction::PlayerJump) ? 1.0F : 0.0F;
  const float down =
      iggy3d::actionIsDown(actions, iggy3d::InputAction::PlayerCrouch) ? 1.0F
                                                                       : 0.0F;
  input.moveZ = up - down;
  input.sprinting =
      iggy3d::actionIsDown(actions, iggy3d::InputAction::PlayerSprint);
  input.cameraYawDegrees = 0.0F;
  const iggy3d::ProductCreativeFlyResult fly =
      iggy3d::applyProductCreativeFlyInput(config, input, {});

  return expect(fly.applied, "recorded forward applies through the kernel") &&
         expect(near(fly.finalPositionMeters.z, -3.0F),
                "recorded forward walks negative z");
}

}  // namespace

int main() {
  const bool ok = flyPollRecordsOnlyFlyAxes() && downKeyIsFlyDownNotCrouchVerb() &&
                  idleRecordsNothing() && recordedAxesDriveTheReusedKernel();
  if (!ok) {
    return 1;
  }
  std::cout << "product_creative_navigate_fly_tests=pass\n";
  return 0;
}
