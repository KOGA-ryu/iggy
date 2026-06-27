#include "app/iggy3d/gameplay/ProductGameplayController.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>

#include "app/iggy3d/ascii_room/ProductAsciiRoomActivation.hpp"
#include "app/input/ActionState.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/session/Session.hpp"
#include "runtime/world/WorldState.hpp"

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool nearlyEqual(float lhs, float rhs, float epsilon = 0.0001F) {
  return std::fabs(lhs - rhs) <= epsilon;
}

iggy3d::ProductAppWindowState makeGameplayWindow(
    std::optional<iggy3d::Session>& session) {
  iggy3d::ProductAppWindowState window;
  window.asciiRoomDraftText =
      "#######\n"
      "#P..$.#\n"
      "#..E..#\n"
      "#######\n";
  window.asciiRoomDraftRoomId = "gameplay_controller_step_room";
  window.asciiRoomDraftSourceName = "unit/gameplay_controller_step_room.iggyroom.txt";
  const iggy3d::ProductAsciiRoomActivationResult activation =
      iggy3d::activateProductAsciiRoomPreview(session, window);
  expect(activation.ok, "ascii room activation ok");
  return window;
}

const iggy3d::EntityState* playerEntity(const iggy3d::Session& session) {
  const iggy3d::EntityId actor = session.state().players.actorForSlot(0);
  return session.state().world.findById(actor);
}

bool productMoveUsesTunedManualStep() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "session created")) {
    return false;
  }
  const iggy3d::EntityState* before = playerEntity(*session);
  if (!expect(before != nullptr, "player before move")) {
    return false;
  }
  const iggy3d::Vec3 start = before->transform.position;

  iggy3d::ActionState actions;
  iggy3d::recordAction(actions, iggy3d::InputAction::PlayerMoveY, true, false,
                       false, 1.0F);
  iggy3d::applyProductGameplayActions(*session, actions, window,
                                      "unit/gameplay_controller_step");

  const iggy3d::EntityState* after = playerEntity(*session);
  if (!expect(after != nullptr, "player after move")) {
    return false;
  }
  const iggy3d::Vec3 final = after->transform.position;

  return expect(window.gameplayCommandAccepted, "move accepted") &&
         expect(window.gameplayMovementStatus == "moved", "movement status") &&
         expect(nearlyEqual(window.gameplayMovementHorizontalDistanceMeters,
                            0.5F),
                "horizontal distance is tuned step") &&
         expect(nearlyEqual(final.x, start.x), "x unchanged") &&
         expect(nearlyEqual(final.z - start.z, 0.5F), "z moved tuned step");
}

bool productMoveNormalizesDiagonalToTunedStep() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "session created")) {
    return false;
  }

  iggy3d::ActionState actions;
  iggy3d::recordAction(actions, iggy3d::InputAction::PlayerMoveX, true, false,
                       false, 1.0F);
  iggy3d::recordAction(actions, iggy3d::InputAction::PlayerMoveY, true, false,
                       false, 1.0F);
  iggy3d::applyProductGameplayActions(*session, actions, window,
                                      "unit/gameplay_controller_step");

  return expect(window.gameplayCommandAccepted, "diagonal move accepted") &&
         expect(window.gameplayMovementStatus == "moved",
                "diagonal movement status") &&
         expect(nearlyEqual(window.gameplayMovementHorizontalDistanceMeters,
                            0.5F),
                "diagonal movement normalizes to tuned step");
}

}  // namespace

int main() {
  const bool ok = productMoveUsesTunedManualStep() &&
                  productMoveNormalizesDiagonalToTunedStep();
  if (!ok) {
    return EXIT_FAILURE;
  }
  std::cout << "product_gameplay_controller_tests=pass\n";
  return EXIT_SUCCESS;
}
