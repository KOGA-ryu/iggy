#include "app/iggy3d/gameplay/ProductGameplayController.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>

#include "app/iggy3d/ascii_room/ProductAsciiRoomActivation.hpp"
#include "app/iggy3d/gameplay/ProductActiveRoomCollision.hpp"
#include "app/input/ActionState.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/session/Session.hpp"
#include "runtime/world/WorldState.hpp"

namespace {

constexpr float kExpectedManualFirstPersonSpeedMetersPerSecond = 1.6F;
constexpr float kExpectedManualFirstPersonStepMeters =
    kExpectedManualFirstPersonSpeedMetersPerSecond / 60.0F;

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

const iggy3d::SpatialSurfaceSet* activeSurfaces(
    const iggy3d::ProductAppWindowState& window) {
  return iggy3d::productActiveRoomCollisionSurfaces(window.activeRoomCollision);
}

iggy3d::ActionState forwardMoveActions() {
  iggy3d::ActionState actions;
  iggy3d::recordAction(actions, iggy3d::InputAction::PlayerMoveY, true, false,
                       false, 1.0F);
  return actions;
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

  iggy3d::ActionState actions = forwardMoveActions();
  iggy3d::applyProductGameplayActions(*session, actions, window,
                                      "unit/gameplay_controller_step");

  const iggy3d::EntityState* after = playerEntity(*session);
  if (!expect(after != nullptr, "player after move")) {
    return false;
  }
  const iggy3d::Vec3 final = after->transform.position;

  return expect(window.gameplayCommandAccepted, "move accepted") &&
         expect(window.gameplayMovementStatus == "moved", "movement status") &&
         expect(!window.physicsMovementPlannerEnabled,
                "default physics planner disabled") &&
         expect(!window.physicsMovementPlannerRequested,
                "default physics planner not requested") &&
         expect(!window.physicsMovementPlannerUsed,
                "default physics planner not used") &&
         expect(window.physicsMovementPlannerStatus ==
                    "physics_movement_planner_disabled",
                "default physics planner status") &&
         expect(window.gameplayMovementProfile == "manual_first_person",
                "manual movement profile") &&
         expect(nearlyEqual(window.gameplayMovementMaxSpeedMetersPerSecond,
                            kExpectedManualFirstPersonSpeedMetersPerSecond),
                "manual movement speed") &&
         expect(nearlyEqual(window.gameplayMovementHorizontalDistanceMeters,
                            kExpectedManualFirstPersonStepMeters),
                "horizontal distance is profile step") &&
         expect(nearlyEqual(final.x, start.x), "x unchanged") &&
         expect(nearlyEqual(final.z - start.z, kExpectedManualFirstPersonStepMeters),
                "z moved profile step");
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
         expect(window.gameplayMovementProfile == "manual_first_person",
                "diagonal movement profile") &&
         expect(nearlyEqual(window.gameplayMovementHorizontalDistanceMeters,
                            kExpectedManualFirstPersonStepMeters),
                "diagonal movement normalizes to profile step");
}

bool defaultOffMoveWithCollisionSurfacesUsesLegacyPath() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "legacy surfaces session created")) {
    return false;
  }
  const iggy3d::SpatialSurfaceSet* surfaces = activeSurfaces(window);
  if (!expect(surfaces != nullptr, "legacy surfaces available")) {
    return false;
  }

  iggy3d::applyProductGameplayActions(*session, forwardMoveActions(), window,
                                      "unit/gameplay_controller_legacy_surfaces",
                                      surfaces);

  return expect(window.gameplayCommandAccepted, "legacy surfaces move accepted") &&
         expect(window.gameplayMovementStatus == "moved",
                "legacy surfaces movement status") &&
         expect(window.gameplayCollisionSurfacesUsed,
                "legacy surfaces collision surfaces used") &&
         expect(!window.physicsMovementPlannerEnabled,
                "legacy surfaces physics planner disabled") &&
         expect(!window.physicsMovementPlannerRequested,
                "legacy surfaces physics planner not requested") &&
         expect(!window.physicsMovementPlannerUsed,
                "legacy surfaces physics planner not used") &&
         expect(window.physicsMovementPlannerReasonCode ==
                    "physics_movement_planner_disabled",
                "legacy surfaces physics planner reason") &&
         expect(session->state().transient.lastMovementResultAvailable,
                "legacy surfaces movement result available") &&
         expect(!session->state()
                     .transient.lastMovementResult.physicsFrameStatsAvailable,
                "legacy surfaces has no physics stats");
}

bool optInMoveWithCollisionSurfacesUsesPhysicsPlanner() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "physics surfaces session created")) {
    return false;
  }
  const iggy3d::SpatialSurfaceSet* surfaces = activeSurfaces(window);
  if (!expect(surfaces != nullptr, "physics surfaces available")) {
    return false;
  }
  window.physicsMovementPlannerEnabled = true;

  iggy3d::applyProductGameplayActions(*session, forwardMoveActions(), window,
                                      "unit/gameplay_controller_physics_surfaces",
                                      surfaces);

  return expect(window.gameplayCommandAccepted, "physics move accepted") &&
         expect(window.physicsMovementPlannerEnabled,
                "physics planner enabled") &&
         expect(window.physicsMovementPlannerRequested,
                "physics planner requested") &&
         expect(window.physicsMovementPlannerUsed,
                "physics planner used") &&
         expect(window.physicsMovementPlannerStatus ==
                    "physics_movement_planner_used",
                "physics planner status used") &&
         expect(session->state().transient.lastMovementResultAvailable,
                "physics movement result available") &&
         expect(session->state()
                    .transient.lastMovementResult.physicsFrameStatsAvailable,
                "physics movement stats available") &&
         expect(window.gameplayMovementDebugAvailable,
                "physics movement debug available") &&
         expect(window.gameplayMovementCollisionSweepCount >= 1U,
                "physics movement sweep count");
}

bool optInMoveWithoutCollisionSurfacesRecordsNoSurfaces() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "physics no surfaces session created")) {
    return false;
  }
  window.physicsMovementPlannerEnabled = true;

  iggy3d::applyProductGameplayActions(*session, forwardMoveActions(), window,
                                      "unit/gameplay_controller_physics_no_surfaces");

  return expect(window.gameplayCommandAccepted,
                "physics no surfaces move accepted") &&
         expect(window.physicsMovementPlannerEnabled,
                "physics no surfaces planner enabled") &&
         expect(window.physicsMovementPlannerRequested,
                "physics no surfaces planner requested") &&
         expect(!window.physicsMovementPlannerUsed,
                "physics no surfaces planner not used") &&
         expect(window.physicsMovementPlannerStatus ==
                    "physics_movement_planner_no_collision_surfaces",
                "physics no surfaces planner status") &&
         expect(!session->state()
                     .transient.lastMovementResult.physicsFrameStatsAvailable,
                "physics no surfaces has no physics stats");
}

}  // namespace

int main() {
  const bool ok = productMoveUsesTunedManualStep() &&
                  productMoveNormalizesDiagonalToTunedStep() &&
                  defaultOffMoveWithCollisionSurfacesUsesLegacyPath() &&
                  optInMoveWithCollisionSurfacesUsesPhysicsPlanner() &&
                  optInMoveWithoutCollisionSurfacesRecordsNoSurfaces();
  if (!ok) {
    return EXIT_FAILURE;
  }
  std::cout << "product_gameplay_controller_tests=pass\n";
  return EXIT_SUCCESS;
}
