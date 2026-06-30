#include "app/iggy3d/gameplay/Controller.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>

#include "app/iggy3d/ascii_room/Activation.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
#include "app/iggy3d/gameplay/MovementTuning.hpp"
#include "app/input/ActionState.hpp"
#include "content/assets/RoomAsset.hpp"
#include "core/math/Transform3.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/replay/StateHash.hpp"
#include "runtime/session/Session.hpp"
#include "runtime/session/SessionState.hpp"
#include "runtime/world/WorldState.hpp"

namespace {

constexpr std::string_view kExpectedManualFirstPersonProfile =
    iggy3d::kProductGameplayMovementTuning.walkProfile;
constexpr std::string_view kExpectedManualFirstPersonSprintProfile =
    iggy3d::kProductGameplayMovementTuning.sprintProfile;
constexpr std::string_view kExpectedManualFirstPersonDashProfile =
    iggy3d::kProductGameplayMovementTuning.dashProfile;
constexpr float kExpectedManualFirstPersonSpeedMetersPerSecond =
    iggy3d::kProductGameplayMovementTuning.walkSpeedMetersPerSecond;
constexpr float kExpectedManualFirstPersonSprintSpeedMetersPerSecond =
    iggy3d::kProductGameplayMovementTuning.sprintSpeedMetersPerSecond;
constexpr float kExpectedManualFirstPersonStepMeters =
    kExpectedManualFirstPersonSpeedMetersPerSecond *
    iggy3d::kProductGameplayMovementTuning.inputStepSeconds;
constexpr float kExpectedManualFirstPersonSprintStepMeters =
    kExpectedManualFirstPersonSprintSpeedMetersPerSecond *
    iggy3d::kProductGameplayMovementTuning.inputStepSeconds;
constexpr float kExpectedManualFirstPersonJumpImpulseMetersPerSecond =
    iggy3d::kProductGameplayMovementTuning.jumpImpulseMetersPerSecond;
constexpr float kExpectedManualFirstPersonDashDistanceMeters =
    iggy3d::kProductGameplayMovementTuning.dashSpeedMetersPerSecond *
    iggy3d::kProductGameplayMovementTuning.dashDurationSeconds;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool nearlyEqual(float lhs, float rhs, float epsilon = 0.0001F) {
  return std::fabs(lhs - rhs) <= epsilon;
}

float horizontalDistance(iggy3d::Vec3 lhs, iggy3d::Vec3 rhs) {
  const float deltaX = rhs.x - lhs.x;
  const float deltaZ = rhs.z - lhs.z;
  return std::sqrt(deltaX * deltaX + deltaZ * deltaZ);
}

iggy3d::ProductAppWindowState makeGameplayWindow(
    std::optional<iggy3d::Session>& session) {
  iggy3d::ProductAppWindowState window;
  window.asciiRoomDraftText =
      "#######\n"
      "#.....#\n"
      "#..P..#\n"
      "#.....#\n"
      "#..$.E#\n"
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

iggy3d::RoomSpatialSurface clamberFloorSurface() {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "floor";
  surface.sourceStaticMeshId = "floor_mesh";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Plane;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Walkable;
  surface.pointsMeters = {
      {-10.0F, 0.0F, -10.0F},
      {10.0F, 0.0F, -10.0F},
      {10.0F, 0.0F, 10.0F},
      {-10.0F, 0.0F, 10.0F},
  };
  surface.normal = {0.0F, 1.0F, 0.0F};
  surface.traversalTags = {"walkable", "clamber"};
  surface.collisionMask = {"actor"};
  return surface;
}

iggy3d::RoomSpatialSurface clamberTopSurface() {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "clamber_top";
  surface.sourceStaticMeshId = "clamber_block";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Plane;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Walkable;
  surface.pointsMeters = {
      {2.0F, 1.0F, -1.5F},
      {4.0F, 1.0F, -1.5F},
      {4.0F, 1.0F, -0.5F},
      {2.0F, 1.0F, -0.5F},
  };
  surface.normal = {0.0F, 1.0F, 0.0F};
  surface.traversalTags = {"walkable"};
  surface.collisionMask = {"actor"};
  return surface;
}

iggy3d::RoomSpatialSurface clamberBlockerSurface() {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "clamber_blocker";
  surface.sourceStaticMeshId = "clamber_block";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Box;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Blocker;
  surface.pointsMeters = {
      {2.0F, 0.0F, -1.5F},
      {4.0F, 0.0F, -1.5F},
      {4.0F, 1.0F, -0.5F},
      {2.0F, 1.0F, -0.5F},
  };
  surface.normal = {0.0F, 0.0F, 1.0F};
  surface.traversalTags = {"blocker", "clamber"};
  surface.collisionMask = {"actor"};
  surface.blocksActor = true;
  return surface;
}

iggy3d::RoomSpatialSurface wallJumpBlockerSurface(bool authoredWallJump = true) {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "wall_jump_wall_actor_blocker";
  surface.sourceStaticMeshId = "wall_jump_wall";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Box;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Blocker;
  surface.pointsMeters = {
      {2.0F, 0.0F, -0.5F},
      {4.0F, 0.0F, -0.5F},
      {4.0F, 2.4F, 0.5F},
      {2.0F, 2.4F, 0.5F},
  };
  surface.normal = {0.0F, 0.0F, 1.0F};
  surface.collisionMask = {"actor"};
  surface.blocksActor = true;
  surface.runtimeOwnerStableName = "wall_jump_wall";
  if (authoredWallJump) {
    surface.traversalTags = {"blocker", "wall_jump"};
  } else {
    surface.traversalTags = {"blocker"};
  }
  return surface;
}

iggy3d::RoomSpatialSurface layeredWalkableFloorSurface(std::string_view id,
                                                       float y,
                                                       float minX,
                                                       float maxX,
                                                       float minZ,
                                                       float maxZ) {
  iggy3d::RoomSpatialSurface surface;
  surface.id = std::string(id);
  surface.sourceStaticMeshId = std::string(id) + "_mesh";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Plane;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Walkable;
  surface.pointsMeters = {
      {minX, y, minZ},
      {maxX, y, minZ},
      {maxX, y, maxZ},
      {minX, y, maxZ},
  };
  surface.normal = {0.0F, 1.0F, 0.0F};
  surface.traversalTags = {"walkable"};
  surface.collisionMask = {"actor"};
  return surface;
}

iggy3d::RoomAsset makeProductClamberRoom() {
  iggy3d::RoomAsset room;
  room.id = "product_clamber_test";

  iggy3d::RoomStaticMeshAsset floor;
  floor.id = "floor_mesh";
  floor.meshId = "floor";
  floor.role = "floor";
  floor.positionMeters = {0.0F, 0.0F, 0.0F};
  floor.sizeMeters = {20.0F, 0.1F, 20.0F};
  room.staticMeshes.push_back(floor);

  iggy3d::RoomStaticMeshAsset block;
  block.id = "clamber_block";
  block.meshId = "block";
  block.role = "ledge";
  block.positionMeters = {3.0F, 0.5F, -1.0F};
  block.sizeMeters = {2.0F, 1.0F, 1.0F};
  room.staticMeshes.push_back(block);

  room.spatialSurfaces.push_back(clamberFloorSurface());
  room.spatialSurfaces.push_back(clamberTopSurface());
  room.spatialSurfaces.push_back(clamberBlockerSurface());
  return room;
}

iggy3d::RoomAsset makeProductWallJumpRoom(bool authoredWallJump = true) {
  iggy3d::RoomAsset room;
  room.id = "product_wall_jump_test";

  iggy3d::RoomStaticMeshAsset floor;
  floor.id = "floor_mesh";
  floor.meshId = "floor";
  floor.role = "floor";
  floor.positionMeters = {0.0F, 0.0F, 0.0F};
  floor.sizeMeters = {20.0F, 0.1F, 20.0F};
  room.staticMeshes.push_back(floor);

  iggy3d::RoomStaticMeshAsset wall;
  wall.id = "wall_jump_wall";
  wall.meshId = "wall";
  wall.role = "wall";
  wall.positionMeters = {3.0F, 1.2F, 0.0F};
  wall.sizeMeters = {2.0F, 2.4F, 1.0F};
  room.staticMeshes.push_back(wall);

  room.spatialSurfaces.push_back(clamberFloorSurface());
  room.spatialSurfaces.push_back(wallJumpBlockerSurface(authoredWallJump));
  return room;
}

iggy3d::RoomAsset makeProductLayeredFloorRoom() {
  iggy3d::RoomAsset room;
  room.id = "product_layered_floor_test";

  iggy3d::RoomStaticMeshAsset lower;
  lower.id = "lower_floor_mesh";
  lower.meshId = "floor";
  lower.role = "floor";
  lower.positionMeters = {0.0F, 0.0F, 0.0F};
  lower.sizeMeters = {12.0F, 0.1F, 12.0F};
  room.staticMeshes.push_back(lower);

  iggy3d::RoomStaticMeshAsset upper;
  upper.id = "upper_floor_mesh";
  upper.meshId = "floor";
  upper.role = "floor";
  upper.positionMeters = {0.0F, 4.0F, 0.0F};
  upper.sizeMeters = {2.0F, 0.1F, 2.0F};
  room.staticMeshes.push_back(upper);

  room.spatialSurfaces.push_back(
      layeredWalkableFloorSurface("lower_floor", 0.0F, -6.0F, 6.0F, -6.0F, 6.0F));
  room.spatialSurfaces.push_back(
      layeredWalkableFloorSurface("upper_floor", 4.0F, -1.0F, 1.0F, -1.0F, 1.0F));

  iggy3d::RoomAnchorAsset spawn;
  spawn.id = "marker_player_spawn_r0_c0";
  spawn.kind = "spawn";
  spawn.runtimeStableName = spawn.id;
  spawn.positionMeters = {0.0F, 0.05F, 0.0F};
  room.anchors.push_back(spawn);

  iggy3d::RoomAnchorAsset reset;
  reset.id = "marker_reset_zone_r0_c1";
  reset.kind = "reset_zone";
  reset.runtimeStableName = reset.id;
  reset.positionMeters = {5.0F, 0.05F, 5.0F};
  room.anchors.push_back(reset);
  return room;
}

void setPlayerPosition(iggy3d::Session& session, iggy3d::Vec3 position) {
  iggy3d::SessionState& state = session.mutableStateForOwnedSystems();
  const iggy3d::EntityId actor = state.players.actorForSlot(0);
  const iggy3d::EntityState* player = state.world.findById(actor);
  if (!expect(player != nullptr, "player exists for reposition")) {
    return;
  }
  iggy3d::Transform3 transform = player->transform;
  transform.position = position;
  const iggy3d::WorldMutationResult mutation =
      state.world.updateTransform(actor, transform);
  expect(mutation.status == iggy3d::WorldStatus::Ok, "player reposition ok");
  state.currentStateHash = iggy3d::computeStateHash(state);
}

void setClamberActiveRoom(iggy3d::ProductAppWindowState& window,
                          const iggy3d::Session& session) {
  window.activeRoom.loaded = true;
  window.activeRoom.status = "loaded";
  window.activeRoom.reasonCode = "active_room_loaded";
  window.activeRoom.source = "unit";
  window.activeRoom.roomId = "product_clamber_test";
  window.activeRoom.sourceName = "unit/product_clamber_test";
  window.activeRoom.room = makeProductClamberRoom();
  window.activeRoom.staticMeshCount = window.activeRoom.room.staticMeshes.size();
  window.activeRoom.spatialSurfaceCount = window.activeRoom.room.spatialSurfaces.size();
  window.activeRoom.walkableSurfaceCount = 2U;
  window.activeRoom.actorBlockerSurfaceCount = 1U;
  window.activeRoomCollision =
      iggy3d::buildProductActiveRoomCollision(window.activeRoom, session.state());
}

void setWallJumpActiveRoom(iggy3d::ProductAppWindowState& window,
                           const iggy3d::Session& session,
                           bool authoredWallJump = true) {
  window.activeRoom.loaded = true;
  window.activeRoom.status = "loaded";
  window.activeRoom.reasonCode = "active_room_loaded";
  window.activeRoom.source = "unit";
  window.activeRoom.roomId = "product_wall_jump_test";
  window.activeRoom.sourceName = "unit/product_wall_jump_test";
  window.activeRoom.room = makeProductWallJumpRoom(authoredWallJump);
  window.activeRoom.staticMeshCount = window.activeRoom.room.staticMeshes.size();
  window.activeRoom.spatialSurfaceCount = window.activeRoom.room.spatialSurfaces.size();
  window.activeRoom.walkableSurfaceCount = 1U;
  window.activeRoom.actorBlockerSurfaceCount = 1U;
  window.activeRoomCollision =
      iggy3d::buildProductActiveRoomCollision(window.activeRoom, session.state());
}

void setLayeredFloorActiveRoom(iggy3d::ProductAppWindowState& window,
                               const iggy3d::Session& session) {
  window.activeRoom.loaded = true;
  window.activeRoom.status = "loaded";
  window.activeRoom.reasonCode = "active_room_loaded";
  window.activeRoom.source = "unit";
  window.activeRoom.roomId = "product_layered_floor_test";
  window.activeRoom.sourceName = "unit/product_layered_floor_test";
  window.activeRoom.room = makeProductLayeredFloorRoom();
  window.activeRoom.staticMeshCount = window.activeRoom.room.staticMeshes.size();
  window.activeRoom.spatialSurfaceCount = window.activeRoom.room.spatialSurfaces.size();
  window.activeRoom.walkableSurfaceCount = 2U;
  window.activeRoom.actorBlockerSurfaceCount = 0U;
  window.activeRoomCollision =
      iggy3d::buildProductActiveRoomCollision(window.activeRoom, session.state());
}

iggy3d::ActionState forwardMoveActions() {
  iggy3d::ActionState actions;
  iggy3d::recordAction(actions, iggy3d::InputAction::PlayerMoveY, true, false,
                       false, 1.0F);
  return actions;
}

iggy3d::ActionState manualMoveActions(float moveX,
                                      float moveY,
                                      bool sprinting = false) {
  iggy3d::ActionState actions;
  if (moveX != 0.0F) {
    iggy3d::recordAction(actions, iggy3d::InputAction::PlayerMoveX, true, false,
                         false, moveX);
  }
  if (moveY != 0.0F) {
    iggy3d::recordAction(actions, iggy3d::InputAction::PlayerMoveY, true, false,
                         false, moveY);
  }
  if (sprinting) {
    iggy3d::recordAction(actions,
                         iggy3d::InputAction::PlayerSprint,
                         true,
                         false,
                         false,
                         1.0F);
  }
  return actions;
}

iggy3d::ActionState jumpActions() {
  iggy3d::ActionState actions;
  iggy3d::recordAction(actions,
                       iggy3d::InputAction::PlayerJump,
                       true,
                       true,
                       false,
                       1.0F);
  return actions;
}

iggy3d::ActionState jumpForwardActions() {
  iggy3d::ActionState actions = forwardMoveActions();
  iggy3d::recordAction(actions,
                       iggy3d::InputAction::PlayerJump,
                       true,
                       true,
                       false,
                       1.0F);
  return actions;
}

iggy3d::ActionState dashActions(float moveX = 0.0F, float moveY = 0.0F) {
  iggy3d::ActionState actions = manualMoveActions(moveX, moveY);
  iggy3d::recordAction(actions,
                       iggy3d::InputAction::PlayerDash,
                       true,
                       true,
                       false,
                       1.0F);
  return actions;
}

iggy3d::ActionState noActions() {
  return {};
}

bool runManualMove(float moveX,
                   float moveY,
                   float yawDegrees,
                   iggy3d::Vec3& delta,
                   iggy3d::ProductAppWindowState* capturedWindow = nullptr) {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "session created for manual move")) {
    return false;
  }
  window.viewport.cameraYawDegrees = yawDegrees;
  const iggy3d::EntityState* before = playerEntity(*session);
  if (!expect(before != nullptr, "player before manual move")) {
    return false;
  }
  const iggy3d::Vec3 start = before->transform.position;

  iggy3d::applyProductGameplayActions(*session,
                                      manualMoveActions(moveX, moveY),
                                      window,
                                      "unit/gameplay_controller_manual_move");

  const iggy3d::EntityState* after = playerEntity(*session);
  if (!expect(after != nullptr, "player after manual move")) {
    return false;
  }
  delta = after->transform.position - start;
  if (capturedWindow != nullptr) {
    *capturedWindow = window;
  }
  return expect(window.gameplayCommandAccepted, "manual move accepted") &&
         expect(window.gameplayMovementStatus == "moved",
                "manual movement status");
}

bool productMoveUsesTunedManualStep() {
  iggy3d::Vec3 delta;
  iggy3d::ProductAppWindowState window;
  if (!runManualMove(0.0F, 1.0F, 0.0F, delta, &window)) {
    return false;
  }

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
         expect(window.gameplayMovementProfile == kExpectedManualFirstPersonProfile,
                "manual movement profile") &&
         expect(nearlyEqual(window.gameplayMovementMaxSpeedMetersPerSecond,
                            kExpectedManualFirstPersonSpeedMetersPerSecond),
                "manual movement speed") &&
         expect(nearlyEqual(window.gameplayMovementHorizontalDistanceMeters,
                            kExpectedManualFirstPersonStepMeters),
                "horizontal distance is profile step") &&
         expect(nearlyEqual(delta.x, 0.0F), "forward x unchanged") &&
         expect(nearlyEqual(delta.z, -kExpectedManualFirstPersonStepMeters),
                "W moves forward along camera -Z at yaw zero");
}

bool productWasdUsesCameraRelativeYawZero() {
  iggy3d::Vec3 delta;
  bool ok = true;

  ok = runManualMove(0.0F, 1.0F, 0.0F, delta) && ok;
  ok = expect(nearlyEqual(delta.x, 0.0F), "W x unchanged") && ok;
  ok = expect(nearlyEqual(delta.z, -kExpectedManualFirstPersonStepMeters),
              "W moves forward") &&
       ok;

  ok = runManualMove(0.0F, -1.0F, 0.0F, delta) && ok;
  ok = expect(nearlyEqual(delta.x, 0.0F), "S x unchanged") && ok;
  ok = expect(nearlyEqual(delta.z, kExpectedManualFirstPersonStepMeters),
              "S moves back") &&
       ok;

  ok = runManualMove(-1.0F, 0.0F, 0.0F, delta) && ok;
  ok = expect(nearlyEqual(delta.x, -kExpectedManualFirstPersonStepMeters),
              "A moves left") &&
       ok;
  ok = expect(nearlyEqual(delta.z, 0.0F), "A z unchanged") && ok;

  ok = runManualMove(1.0F, 0.0F, 0.0F, delta) && ok;
  ok = expect(nearlyEqual(delta.x, kExpectedManualFirstPersonStepMeters),
              "D moves right") &&
       ok;
  ok = expect(nearlyEqual(delta.z, 0.0F), "D z unchanged") && ok;

  return ok;
}

bool productMoveUsesCameraYaw() {
  iggy3d::Vec3 delta;
  if (!runManualMove(0.0F, 1.0F, 90.0F, delta)) {
    return false;
  }
  return expect(nearlyEqual(delta.x, kExpectedManualFirstPersonStepMeters),
                "yaw ninety W moves camera forward along +X") &&
         expect(nearlyEqual(delta.z, 0.0F), "yaw ninety W z unchanged");
}

bool productMoveNormalizesDiagonalToTunedStep() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "session created")) {
    return false;
  }

  const iggy3d::Vec3 start = playerEntity(*session)->transform.position;
  iggy3d::applyProductGameplayActions(*session, manualMoveActions(1.0F, 1.0F), window,
                                      "unit/gameplay_controller_step");
  const iggy3d::Vec3 final = playerEntity(*session)->transform.position;

  return expect(window.gameplayCommandAccepted, "diagonal move accepted") &&
         expect(window.gameplayMovementStatus == "moved",
                "diagonal movement status") &&
         expect(window.gameplayMovementProfile == kExpectedManualFirstPersonProfile,
                "diagonal movement profile") &&
         expect(nearlyEqual(window.gameplayMovementHorizontalDistanceMeters,
                            kExpectedManualFirstPersonStepMeters),
                "diagonal movement normalizes to profile step") &&
         expect(final.x > start.x, "diagonal includes right movement") &&
         expect(final.z < start.z, "diagonal includes forward movement");
}

bool productSprintUsesSprintProfileAndStep() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "sprint session created")) {
    return false;
  }

  const iggy3d::Vec3 start = playerEntity(*session)->transform.position;
  iggy3d::applyProductGameplayActions(*session,
                                      manualMoveActions(0.0F, 1.0F, true),
                                      window,
                                      "unit/gameplay_controller_sprint");
  const iggy3d::Vec3 final = playerEntity(*session)->transform.position;

  return expect(window.gameplayCommandAccepted, "sprint move accepted") &&
         expect(window.gameplayMovementStatus == "moved",
                "sprint movement status") &&
         expect(window.gameplayMovementProfile ==
                    kExpectedManualFirstPersonSprintProfile,
                "sprint movement profile") &&
         expect(nearlyEqual(window.gameplayMovementMaxSpeedMetersPerSecond,
                            kExpectedManualFirstPersonSprintSpeedMetersPerSecond),
                "sprint movement speed") &&
         expect(nearlyEqual(window.gameplayMovementHorizontalDistanceMeters,
                            kExpectedManualFirstPersonSprintStepMeters),
                "sprint horizontal distance is sprint step") &&
         expect(nearlyEqual(final.x - start.x, 0.0F), "sprint x unchanged") &&
         expect(nearlyEqual(final.z - start.z,
                            -kExpectedManualFirstPersonSprintStepMeters),
                "sprint moves forward by sprint step");
}

bool productMoveUsesRuntimeTunedWindowSpeed() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "runtime tuned session created")) {
    return false;
  }

  window.gameplayMovementTuning.walkSpeedMetersPerSecond = 1.2F;
  const float expectedStep =
      window.gameplayMovementTuning.walkSpeedMetersPerSecond *
      window.gameplayMovementTuning.inputStepSeconds;
  const iggy3d::Vec3 start = playerEntity(*session)->transform.position;
  iggy3d::applyProductGameplayActions(*session,
                                      manualMoveActions(0.0F, 1.0F),
                                      window,
                                      "unit/gameplay_controller_runtime_tuning");
  const iggy3d::Vec3 final = playerEntity(*session)->transform.position;

  return expect(window.gameplayCommandAccepted, "runtime tuned move accepted") &&
         expect(nearlyEqual(window.gameplayMovementMaxSpeedMetersPerSecond, 1.2F),
                "runtime tuned speed proof") &&
         expect(nearlyEqual(window.gameplayMovementHorizontalDistanceMeters,
                            expectedStep),
                "runtime tuned horizontal distance") &&
         expect(nearlyEqual(final.z - start.z, -expectedStep),
                "runtime tuned final z");
}

bool productMoveUsesRuntimeTunedGroundAcceleration() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "runtime tuned acceleration session created")) {
    return false;
  }

  window.gameplayMovementTuning.groundAccelerationMetersPerSecondSquared = 33.0F;
  const float expectedStep =
      window.gameplayMovementTuning.groundAccelerationMetersPerSecondSquared *
      window.gameplayMovementTuning.inputStepSeconds *
      window.gameplayMovementTuning.inputStepSeconds;
  const iggy3d::Vec3 start = playerEntity(*session)->transform.position;
  iggy3d::applyProductGameplayActions(*session,
                                      manualMoveActions(0.0F, 1.0F),
                                      window,
                                      "unit/gameplay_controller_accel_tuning");
  const iggy3d::Vec3 final = playerEntity(*session)->transform.position;

  return expect(window.gameplayCommandAccepted,
                "runtime acceleration tuned move accepted") &&
         expect(nearlyEqual(window.gameplayMovementHorizontalDistanceMeters,
                            expectedStep),
                "runtime acceleration tuned horizontal distance") &&
         expect(nearlyEqual(final.z - start.z, -expectedStep),
                "runtime acceleration tuned final z");
}

bool productGroundAccelerationApproachesMaxSpeed() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "accel ramp session created")) {
    return false;
  }

  window.gameplayMovementTuning.groundAccelerationMetersPerSecondSquared = 33.0F;
  iggy3d::applyProductGameplayActions(*session,
                                      forwardMoveActions(),
                                      window,
                                      "unit/gameplay_controller_accel_ramp_first");
  const float firstDistance = window.gameplayMovementHorizontalDistanceMeters;
  for (int frame = 0; frame < 8; ++frame) {
    iggy3d::applyProductGameplayActions(*session,
                                        forwardMoveActions(),
                                        window,
                                        "unit/gameplay_controller_accel_ramp_later");
  }
  const float laterDistance = window.gameplayMovementHorizontalDistanceMeters;

  return expect(firstDistance < kExpectedManualFirstPersonStepMeters,
                "first acceleration frame is below full speed") &&
         expect(laterDistance > firstDistance,
                "later acceleration frame moves farther") &&
         expect(nearlyEqual(laterDistance, kExpectedManualFirstPersonStepMeters),
                "acceleration reaches max speed step");
}

bool productHigherGroundAccelerationReachesSpeedFaster() {
  std::optional<iggy3d::Session> slowSession;
  iggy3d::ProductAppWindowState slowWindow = makeGameplayWindow(slowSession);
  std::optional<iggy3d::Session> fastSession;
  iggy3d::ProductAppWindowState fastWindow = makeGameplayWindow(fastSession);
  if (!expect(slowSession.has_value() && fastSession.has_value(),
              "accel comparison sessions created")) {
    return false;
  }

  slowWindow.gameplayMovementTuning.groundAccelerationMetersPerSecondSquared =
      16.5F;
  fastWindow.gameplayMovementTuning.groundAccelerationMetersPerSecondSquared =
      66.0F;
  iggy3d::applyProductGameplayActions(*slowSession,
                                      forwardMoveActions(),
                                      slowWindow,
                                      "unit/gameplay_controller_accel_slow");
  iggy3d::applyProductGameplayActions(*fastSession,
                                      forwardMoveActions(),
                                      fastWindow,
                                      "unit/gameplay_controller_accel_fast");

  return expect(fastWindow.gameplayMovementHorizontalDistanceMeters >
                    slowWindow.gameplayMovementHorizontalDistanceMeters,
                "higher acceleration moves farther on first frame") &&
         expect(std::fabs(fastWindow.gameplayMovementGroundVelocityZ) >
                    std::fabs(slowWindow.gameplayMovementGroundVelocityZ),
                "higher acceleration stores faster retained velocity");
}

bool productGroundDecelerationDecaysRetainedVelocity() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "decel session created")) {
    return false;
  }

  iggy3d::applyProductGameplayActions(*session,
                                      forwardMoveActions(),
                                      window,
                                      "unit/gameplay_controller_decel_prime");
  const float startingSpeed = std::fabs(window.gameplayMovementGroundVelocityZ);
  window.gameplayMovementTuning.groundDecelerationMetersPerSecondSquared = 33.0F;
  iggy3d::ActionState noInput;
  iggy3d::applyProductGameplayActions(*session,
                                      noInput,
                                      window,
                                      "unit/gameplay_controller_decel_release");
  const float decayedSpeed = std::fabs(window.gameplayMovementGroundVelocityZ);

  return expect(window.gameplayCommandAccepted,
                "deceleration submits retained movement") &&
         expect(window.gameplayMovementHorizontalDistanceMeters > 0.0F,
                "deceleration keeps moving after release") &&
         expect(decayedSpeed < startingSpeed,
                "deceleration reduces retained speed") &&
         expect(decayedSpeed > 0.0F,
                "deceleration does not stop instantly");
}

bool productHigherGroundDecelerationStopsFaster() {
  std::optional<iggy3d::Session> slowSession;
  iggy3d::ProductAppWindowState slowWindow = makeGameplayWindow(slowSession);
  std::optional<iggy3d::Session> fastSession;
  iggy3d::ProductAppWindowState fastWindow = makeGameplayWindow(fastSession);
  if (!expect(slowSession.has_value() && fastSession.has_value(),
              "decel comparison sessions created")) {
    return false;
  }

  iggy3d::applyProductGameplayActions(*slowSession,
                                      forwardMoveActions(),
                                      slowWindow,
                                      "unit/gameplay_controller_decel_slow_prime");
  iggy3d::applyProductGameplayActions(*fastSession,
                                      forwardMoveActions(),
                                      fastWindow,
                                      "unit/gameplay_controller_decel_fast_prime");
  slowWindow.gameplayMovementTuning.groundDecelerationMetersPerSecondSquared =
      16.5F;
  fastWindow.gameplayMovementTuning.groundDecelerationMetersPerSecondSquared =
      66.0F;
  iggy3d::ActionState noInput;
  iggy3d::applyProductGameplayActions(*slowSession,
                                      noInput,
                                      slowWindow,
                                      "unit/gameplay_controller_decel_slow");
  iggy3d::applyProductGameplayActions(*fastSession,
                                      noInput,
                                      fastWindow,
                                      "unit/gameplay_controller_decel_fast");

  return expect(fastWindow.gameplayMovementHorizontalDistanceMeters <
                    slowWindow.gameplayMovementHorizontalDistanceMeters,
                "higher deceleration moves less after release") &&
         expect(std::fabs(fastWindow.gameplayMovementGroundVelocityZ) <
                    std::fabs(slowWindow.gameplayMovementGroundVelocityZ),
                "higher deceleration stores lower retained velocity");
}

bool productJumpRaisesPlayerAndRecordsProof() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "jump session created")) {
    return false;
  }

  const iggy3d::Vec3 start = playerEntity(*session)->transform.position;
  iggy3d::applyProductGameplayActions(*session,
                                      jumpActions(),
                                      window,
                                      "unit/gameplay_controller_jump");
  const iggy3d::Vec3 final = playerEntity(*session)->transform.position;

  return expect(window.gameplayJumpRequested, "jump requested") &&
         expect(window.gameplayJumpAccepted, "jump accepted") &&
         expect(window.gameplayJumpActive, "jump remains active after first step") &&
         expect(window.gameplayJumpStatus == "airborne", "jump airborne status") &&
         expect(window.gameplayJumpReasonCode == "gameplay_jump_airborne",
                "jump airborne reason") &&
         expect(window.playerPositionChanged, "jump changed player position") &&
         expect(final.y > start.y, "jump raises player y") &&
         expect(nearlyEqual(window.gameplayJumpGroundY, start.y), "jump ground y") &&
         expect(nearlyEqual(window.gameplayJumpStartY, start.y), "jump start y") &&
         expect(nearlyEqual(window.gameplayJumpFinalY, final.y), "jump final y") &&
         expect(window.gameplayJumpHeightMeters > 0.0F, "jump height positive") &&
         expect(window.gameplayJumpVelocityMetersPerSecond <
                    kExpectedManualFirstPersonJumpImpulseMetersPerSecond,
                "jump velocity reduced by gravity");
}

bool productJumpCanMoveForwardInSameFrame() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "jump move session created")) {
    return false;
  }

  const iggy3d::Vec3 start = playerEntity(*session)->transform.position;
  iggy3d::applyProductGameplayActions(*session,
                                      jumpForwardActions(),
                                      window,
                                      "unit/gameplay_controller_jump_forward");
  const iggy3d::Vec3 final = playerEntity(*session)->transform.position;

  return expect(window.gameplayJumpRequested, "jump forward jump requested") &&
         expect(window.gameplayJumpAccepted, "jump forward jump accepted") &&
         expect(window.gameplayJumpActive, "jump forward remains airborne") &&
         expect(window.gameplayJumpStatus == "airborne",
                "jump forward jump airborne") &&
         expect(window.gameplayMovementAttempted,
                "jump forward movement attempted") &&
         expect(window.gameplayMovementStatus == "moved",
                "jump forward movement status") &&
         expect(window.gameplayMovementDebugAvailable,
                "jump forward movement debug") &&
         expect(window.gameplayMovementReasonCode == "airborne_manual_move",
                "jump forward movement reason") &&
         expect(window.gameplayMovementPolicyBand == "airborne",
                "jump forward movement policy") &&
         expect(!window.gameplayCommandSubmitted,
                "jump forward avoids grounded command") &&
         expect(nearlyEqual(window.gameplayMovementHorizontalDistanceMeters,
                            kExpectedManualFirstPersonStepMeters),
                "jump forward horizontal step") &&
         expect(final.y > start.y, "jump forward raises y") &&
         expect(final.z < start.z, "jump forward moves z") &&
         expect(nearlyEqual(final.x, start.x), "jump forward keeps x");
}

bool productJumpAirControlScalesAirborneMove() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "air control tuned session created")) {
    return false;
  }

  window.gameplayMovementTuning.airControlMultiplier = 0.25F;
  const float expectedStep =
      window.gameplayMovementTuning.walkSpeedMetersPerSecond *
      window.gameplayMovementTuning.inputStepSeconds *
      window.gameplayMovementTuning.airControlMultiplier;
  const iggy3d::Vec3 start = playerEntity(*session)->transform.position;
  iggy3d::applyProductGameplayActions(*session,
                                      jumpForwardActions(),
                                      window,
                                      "unit/gameplay_controller_air_control");
  const iggy3d::Vec3 final = playerEntity(*session)->transform.position;

  return expect(window.gameplayJumpAccepted,
                "air control tuned jump accepted") &&
         expect(window.gameplayMovementAttempted,
                "air control tuned movement attempted") &&
         expect(nearlyEqual(window.gameplayMovementHorizontalDistanceMeters,
                            expectedStep),
                "air control tuned horizontal distance") &&
         expect(nearlyEqual(final.z - start.z, -expectedStep),
                "air control tuned final z");
}

bool productJumpUsesClamberTraversalWhenCandidateIsLocal() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "clamber session created")) {
    return false;
  }

  setPlayerPosition(*session, {3.0F, 0.0F, 0.10F});
  setClamberActiveRoom(window, *session);
  window.viewport.cameraYawDegrees = 0.0F;
  const iggy3d::Vec3 start = playerEntity(*session)->transform.position;

  iggy3d::applyProductGameplayActions(*session,
                                      jumpActions(),
                                      window,
                                      "unit/gameplay_controller_clamber");

  const iggy3d::Vec3 final = playerEntity(*session)->transform.position;
  return expect(window.gameplayJumpRequested, "clamber jump requested") &&
         expect(!window.gameplayJumpAccepted, "clamber skips jump arc") &&
         expect(!window.gameplayJumpActive, "clamber leaves jump inactive") &&
         expect(window.gameplayJumpStatus == "traversal",
                "clamber jump status") &&
         expect(window.gameplayJumpReasonCode == "traversal_intent_applied",
                "clamber jump reason") &&
         expect(window.gameplayTraversalRequested, "clamber requested") &&
         expect(window.gameplayTraversalConsumed, "clamber consumed input") &&
         expect(window.gameplayTraversalAccepted, "clamber accepted") &&
         expect(!window.gameplayTraversalFallbackJumpAllowed,
                "clamber no jump fallback") &&
         expect(window.gameplayTraversalStatus == "traversal_intent_applied",
                "clamber traversal status") &&
         expect(window.gameplayTraversalReasonCode == "traversal_intent_applied",
                "clamber traversal reason") &&
         expect(window.gameplayTraversalMechanic == "clamber",
                "clamber mechanic proof") &&
         expect(window.gameplayTraversalSlotId == "clamber_block:clamber_top",
                "clamber slot proof") &&
         expect(window.gameplayTraversalTargetId == "clamber_block",
                "clamber target proof") &&
         expect(window.gameplayTraversalLandingSurfaceId == "clamber_top",
                "clamber landing proof") &&
         expect(window.playerPositionChanged, "clamber changed player position") &&
         expect(nearlyEqual(window.gameplayTraversalStartX, start.x),
                "clamber start x") &&
         expect(nearlyEqual(window.gameplayTraversalStartY, start.y),
                "clamber start y") &&
         expect(nearlyEqual(window.gameplayTraversalStartZ, start.z),
                "clamber start z") &&
         expect(nearlyEqual(window.gameplayTraversalFinalX, final.x),
                "clamber final x") &&
         expect(nearlyEqual(window.gameplayTraversalFinalY, final.y),
                "clamber final y") &&
         expect(nearlyEqual(window.gameplayTraversalFinalZ, final.z),
                "clamber final z") &&
         expect(final.y > start.y, "clamber raises player");
}

bool productJumpUsesWallJumpWhenAirborneNearWall() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "wall jump session created")) {
    return false;
  }

  setPlayerPosition(*session, {3.0F, 0.80F, 0.65F});
  setWallJumpActiveRoom(window, *session);
  const iggy3d::Vec3 start = playerEntity(*session)->transform.position;

  iggy3d::applyProductGameplayActions(*session,
                                      jumpActions(),
                                      window,
                                      "unit/gameplay_controller_wall_jump");

  const iggy3d::Vec3 final = playerEntity(*session)->transform.position;
  return expect(window.gameplayJumpRequested, "wall jump requested") &&
         expect(window.gameplayJumpAccepted, "wall jump accepted") &&
         expect(window.gameplayJumpActive, "wall jump leaves jump active") &&
         expect(window.gameplayJumpStatus == "wall_jump",
                "wall jump status") &&
         expect(window.gameplayJumpReasonCode == "gameplay_jump_wall_jump",
                "wall jump reason") &&
         expect(window.gameplayTraversalRequested, "wall jump traversal requested") &&
         expect(window.gameplayTraversalConsumed, "wall jump consumed input") &&
         expect(window.gameplayTraversalAccepted, "wall jump traversal accepted") &&
         expect(!window.gameplayTraversalFallbackJumpAllowed,
                "wall jump no fallback") &&
         expect(window.gameplayTraversalMechanic == "wall_jump",
                "wall jump mechanic proof") &&
         expect(window.gameplayTraversalSlotId == "wall_jump_wall_actor_blocker",
                "wall jump slot proof") &&
         expect(window.gameplayTraversalTargetId == "wall_jump_wall",
                "wall jump target proof") &&
         expect(window.gameplayTraversalLandingSurfaceId ==
                    "wall_jump_wall_actor_blocker",
                "wall jump landing proof") &&
         expect(window.playerPositionChanged, "wall jump changed player position") &&
         expect(nearlyEqual(window.gameplayTraversalStartX, start.x),
                "wall jump start x") &&
         expect(nearlyEqual(window.gameplayTraversalStartY, start.y),
                "wall jump start y") &&
         expect(nearlyEqual(window.gameplayTraversalStartZ, start.z),
                "wall jump start z") &&
         expect(nearlyEqual(window.gameplayTraversalFinalX, final.x),
                "wall jump final x") &&
         expect(nearlyEqual(window.gameplayTraversalFinalY, final.y),
                "wall jump final y") &&
         expect(nearlyEqual(window.gameplayTraversalFinalZ, final.z),
                "wall jump final z") &&
         expect(final.y > start.y, "wall jump raises player") &&
         expect(final.z > start.z, "wall jump pushes away from wall");
}

bool productJumpRejectsWallJumpNearGenericWall() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "generic wall jump session created")) {
    return false;
  }

  setPlayerPosition(*session, {3.0F, 0.80F, 0.65F});
  setWallJumpActiveRoom(window, *session, false);
  window.gameplayJumpActive = true;
  window.gameplayJumpVelocityMetersPerSecond = 1.0F;
  const iggy3d::Vec3 start = playerEntity(*session)->transform.position;

  iggy3d::applyProductGameplayActions(*session,
                                      jumpActions(),
                                      window,
                                      "unit/gameplay_controller_generic_wall_jump");

  const iggy3d::Vec3 final = playerEntity(*session)->transform.position;
  return expect(window.gameplayJumpRequested, "generic wall jump requested") &&
         expect(!window.gameplayJumpAccepted, "generic wall jump rejected") &&
         expect(window.gameplayJumpStatus == "already_airborne",
                "generic wall jump status") &&
         expect(window.gameplayJumpReasonCode ==
                    "gameplay_jump_already_airborne",
                "generic wall jump reason") &&
         expect(!window.gameplayTraversalAccepted,
                "generic wall traversal rejected") &&
         expect(nearlyEqual(final.x, start.x), "generic wall x unchanged") &&
         expect(nearlyEqual(final.y, start.y), "generic wall y unchanged") &&
         expect(nearlyEqual(final.z, start.z), "generic wall z unchanged");
}

bool productJumpRejectsDoubleJumpWhileAirborne() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "double jump session created")) {
    return false;
  }

  iggy3d::applyProductGameplayActions(*session,
                                      jumpActions(),
                                      window,
                                      "unit/gameplay_controller_jump");
  const float firstY = playerEntity(*session)->transform.position.y;
  iggy3d::applyProductGameplayActions(*session,
                                      jumpActions(),
                                      window,
                                      "unit/gameplay_controller_double_jump");
  const float secondY = playerEntity(*session)->transform.position.y;

  return expect(window.gameplayJumpRequested, "double jump requested") &&
         expect(!window.gameplayJumpAccepted, "double jump rejected") &&
         expect(window.gameplayJumpActive, "double jump still airborne") &&
         expect(window.gameplayJumpStatus == "already_airborne",
                "double jump status") &&
         expect(window.gameplayJumpReasonCode ==
                    "gameplay_jump_already_airborne",
                "double jump reason") &&
         expect(nearlyEqual(firstY, secondY), "double jump does not add height");
}

bool productJumpFallsAndLands() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "landing session created")) {
    return false;
  }

  const float groundY = playerEntity(*session)->transform.position.y;
  iggy3d::applyProductGameplayActions(*session,
                                      jumpActions(),
                                      window,
                                      "unit/gameplay_controller_jump");
  bool observedAirborneHeight = window.gameplayJumpHeightMeters > 0.0F;
  const int landingTickBudget = static_cast<int>(std::ceil(
                                    2.0F *
                                    window.gameplayMovementTuning
                                        .jumpImpulseMetersPerSecond /
                                    window.gameplayMovementTuning
                                        .gravityMetersPerSecondSquared /
                                    window.gameplayMovementTuning
                                        .inputStepSeconds)) +
                                10;
  for (int tick = 0; tick < landingTickBudget; ++tick) {
    iggy3d::applyProductGameplayActions(*session,
                                        noActions(),
                                        window,
                                        "unit/gameplay_controller_jump_tick");
    observedAirborneHeight =
        observedAirborneHeight || window.gameplayJumpHeightMeters > 0.0F;
  }
  const float finalY = playerEntity(*session)->transform.position.y;

  return expect(observedAirborneHeight, "landing observed airborne height") &&
         expect(!window.gameplayJumpActive, "jump no longer active") &&
         expect(window.gameplayJumpStatus == "landed", "jump landed status") &&
         expect(window.gameplayJumpReasonCode == "gameplay_jump_landed",
                "jump landed reason") &&
         expect(nearlyEqual(window.gameplayJumpVelocityMetersPerSecond, 0.0F),
                "landed velocity zero") &&
         expect(nearlyEqual(window.gameplayJumpGroundY, groundY), "land ground") &&
         expect(nearlyEqual(finalY, groundY), "landed player y") &&
         expect(nearlyEqual(window.gameplayJumpFinalY, groundY),
                "landed final proof y") &&
         expect(nearlyEqual(window.gameplayJumpHeightMeters, 0.0F),
                "landed height zero");
}

bool productJumpLandsOnElevatedWalkableFloor() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "elevated landing session created")) {
    return false;
  }

  setLayeredFloorActiveRoom(window, *session);
  setPlayerPosition(*session, {0.0F, 5.2F, 0.0F});
  window.gameplayJumpActive = true;
  window.gameplayJumpVelocityMetersPerSecond = -1.0F;
  window.gameplayJumpGroundY = 0.0F;
  window.gameplayJumpStartY = 5.2F;

  for (int tick = 0; tick < 50 && window.gameplayJumpActive; ++tick) {
    iggy3d::applyProductGameplayActions(*session,
                                        noActions(),
                                        window,
                                        "unit/gameplay_controller_elevated_land",
                                        activeSurfaces(window));
  }

  const float finalY = playerEntity(*session)->transform.position.y;
  return expect(!window.gameplayJumpActive, "elevated landing inactive") &&
         expect(window.gameplayJumpStatus == "landed", "elevated landing status") &&
         expect(nearlyEqual(finalY, 4.0F), "elevated landing y") &&
         expect(nearlyEqual(window.gameplayJumpGroundY, 4.0F),
                "elevated landing ground y") &&
         expect(nearlyEqual(window.gameplayJumpFinalY, 4.0F),
                "elevated landing proof y");
}

bool productFallThroughHoleLandsOnLowerWalkableFloor() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "hole fall session created")) {
    return false;
  }

  setLayeredFloorActiveRoom(window, *session);
  setPlayerPosition(*session, {3.0F, 4.0F, 0.0F});

  bool observedFall = false;
  for (int tick = 0; tick < 80; ++tick) {
    iggy3d::applyProductGameplayActions(*session,
                                        noActions(),
                                        window,
                                        "unit/gameplay_controller_hole_fall",
                                        activeSurfaces(window));
    observedFall = observedFall || playerEntity(*session)->transform.position.y < 4.0F;
    if (!window.gameplayJumpActive &&
        nearlyEqual(playerEntity(*session)->transform.position.y, 0.0F)) {
      break;
    }
  }

  const float finalY = playerEntity(*session)->transform.position.y;
  return expect(observedFall, "hole fall observed downward motion") &&
         expect(!window.gameplayJumpActive, "hole fall inactive after landing") &&
         expect(window.gameplayJumpStatus == "landed", "hole fall landed status") &&
         expect(nearlyEqual(finalY, 0.0F), "hole fall lands on lower floor") &&
         expect(nearlyEqual(window.gameplayJumpGroundY, 0.0F),
                "hole fall ground y");
}

bool productMoveOffUpperFloorStartsFallingImmediately() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "move off floor session created")) {
    return false;
  }

  setLayeredFloorActiveRoom(window, *session);
  setPlayerPosition(*session, {1.34F, 4.0F, 0.0F});
  iggy3d::applyProductGameplayActions(*session,
                                      manualMoveActions(1.0F, 0.0F),
                                      window,
                                      "unit/gameplay_controller_move_off_floor",
                                      activeSurfaces(window));

  const iggy3d::Vec3 final = playerEntity(*session)->transform.position;
  return expect(window.gameplayMovementAttempted, "move off floor attempted") &&
         expect(window.playerPositionChanged, "move off floor moved") &&
         expect(final.x > 1.35F, "move off floor leaves upper footprint") &&
         expect(window.gameplayJumpActive,
                "move off floor starts falling immediately") &&
         expect(window.gameplayJumpStatus == "falling",
                "move off floor falling status") &&
         expect(window.gameplayJumpReasonCode == "gameplay_jump_falling",
                "move off floor falling reason") &&
         expect(window.gameplayMovementReasonCode == "grounded_ledge_fall",
                "move off floor movement reason") &&
         expect(nearlyEqual(window.gameplayJumpGroundY, 0.0F),
                "move off floor targets lower floor") &&
         expect(nearlyEqual(final.y, 4.0F),
                "move off floor starts fall from upper y");
}

bool productResetZoneReturnsPlayerToSpawn() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "reset zone session created")) {
    return false;
  }

  setLayeredFloorActiveRoom(window, *session);
  setPlayerPosition(*session, {5.0F, 0.05F, 5.0F});
  iggy3d::applyProductGameplayActions(*session,
                                      noActions(),
                                      window,
                                      "unit/gameplay_controller_reset_zone",
                                      activeSurfaces(window));

  const iggy3d::Vec3 final = playerEntity(*session)->transform.position;
  return expect(window.gameplayResetTriggered, "reset zone triggered") &&
         expect(window.gameplayResetStatus == "reset",
                "reset zone status") &&
         expect(window.gameplayResetReasonCode == "gameplay_reset_zone",
                "reset zone reason") &&
         expect(window.gameplayResetSpawnAnchorId == "marker_player_spawn_r0_c0",
                "reset zone spawn anchor") &&
         expect(window.gameplayResetSourceAnchorId == "marker_reset_zone_r0_c1",
                "reset zone source anchor") &&
         expect(!window.gameplayJumpActive, "reset zone clears jump") &&
         expect(nearlyEqual(final.x, 0.0F), "reset zone final x") &&
         expect(nearlyEqual(final.y, 0.05F), "reset zone final y") &&
         expect(nearlyEqual(final.z, 0.0F), "reset zone final z");
}

bool productFallOutBelowLowestFloorReturnsPlayerToSpawn() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "fall reset session created")) {
    return false;
  }

  setLayeredFloorActiveRoom(window, *session);
  setPlayerPosition(*session, {2.0F, -7.0F, 0.0F});
  iggy3d::applyProductGameplayActions(*session,
                                      noActions(),
                                      window,
                                      "unit/gameplay_controller_fall_reset",
                                      activeSurfaces(window));

  const iggy3d::Vec3 final = playerEntity(*session)->transform.position;
  return expect(window.gameplayResetTriggered, "fall reset triggered") &&
         expect(window.gameplayResetStatus == "reset", "fall reset status") &&
         expect(window.gameplayResetReasonCode == "gameplay_reset_fall_out",
                "fall reset reason") &&
         expect(window.gameplayResetSpawnAnchorId == "marker_player_spawn_r0_c0",
                "fall reset spawn anchor") &&
         expect(window.gameplayResetSourceAnchorId == "none",
                "fall reset source none") &&
         expect(nearlyEqual(window.gameplayResetStartY, -7.0F),
                "fall reset start y") &&
         expect(nearlyEqual(window.gameplayResetFinalY, 0.05F),
                "fall reset final proof y") &&
         expect(!window.gameplayJumpActive, "fall reset clears jump") &&
         expect(nearlyEqual(final.x, 0.0F), "fall reset final x") &&
         expect(nearlyEqual(final.y, 0.05F), "fall reset final y") &&
         expect(nearlyEqual(final.z, 0.0F), "fall reset final z");
}

bool productDashMovesForwardAndRecordsProof() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "dash session created")) {
    return false;
  }

  const iggy3d::Vec3 start = playerEntity(*session)->transform.position;
  iggy3d::applyProductGameplayActions(*session,
                                      dashActions(),
                                      window,
                                      "unit/gameplay_controller_dash");
  const iggy3d::Vec3 final = playerEntity(*session)->transform.position;
  const float appliedDistance = horizontalDistance(start, final);

  return expect(window.gameplayDashRequested, "dash requested") &&
         expect(window.gameplayDashAccepted, "dash accepted") &&
         expect(window.gameplayDashStatus == "accepted", "dash status") &&
         expect(window.gameplayDashReasonCode == "gameplay_dash_accepted",
                "dash reason") &&
         expect(window.gameplayMovementProfile == kExpectedManualFirstPersonDashProfile,
                "dash movement profile") &&
         expect(nearlyEqual(window.gameplayDashDistanceMeters,
                            kExpectedManualFirstPersonDashDistanceMeters),
                "dash distance proof") &&
         expect(nearlyEqual(window.gameplayMovementHorizontalDistanceMeters,
                            appliedDistance),
                "dash movement distance matches applied movement") &&
         expect(window.gameplayMovementHorizontalDistanceMeters <=
                    kExpectedManualFirstPersonDashDistanceMeters,
                "dash movement distance within requested dash") &&
         expect(window.gameplayDashCooldownRemainingSeconds > 0.0F,
                "dash cooldown set") &&
         expect(nearlyEqual(window.gameplayDashDirectionX, 0.0F),
                "dash direction x") &&
         expect(nearlyEqual(window.gameplayDashDirectionZ, -1.0F),
                "dash direction z") &&
         expect(nearlyEqual(final.x - start.x, 0.0F), "dash x unchanged");
}

bool productDashUsesRuntimeTunedWindowDistance() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "runtime tuned dash session created")) {
    return false;
  }

  window.gameplayMovementTuning.dashSpeedMetersPerSecond = 4.0F;
  window.gameplayMovementTuning.dashDurationSeconds = 0.25F;
  iggy3d::applyProductGameplayActions(*session,
                                      dashActions(),
                                      window,
                                      "unit/gameplay_controller_runtime_dash");

  return expect(window.gameplayDashAccepted, "runtime tuned dash accepted") &&
         expect(nearlyEqual(window.gameplayDashSpeedMetersPerSecond, 4.0F),
                "runtime tuned dash speed") &&
         expect(nearlyEqual(window.gameplayDashDistanceMeters, 1.0F),
                "runtime tuned dash distance");
}

bool productDashUsesMoveIntentDirection() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "dash right session created")) {
    return false;
  }

  const iggy3d::Vec3 start = playerEntity(*session)->transform.position;
  iggy3d::applyProductGameplayActions(*session,
                                      dashActions(1.0F, 0.0F),
                                      window,
                                      "unit/gameplay_controller_dash_right");
  const iggy3d::Vec3 final = playerEntity(*session)->transform.position;
  const float appliedDistance = horizontalDistance(start, final);

  return expect(window.gameplayDashAccepted, "dash right accepted") &&
         expect(nearlyEqual(window.gameplayDashDirectionX, 1.0F),
                "dash right direction x") &&
         expect(nearlyEqual(window.gameplayDashDirectionZ, 0.0F),
                "dash right direction z") &&
         expect(nearlyEqual(window.gameplayMovementHorizontalDistanceMeters,
                            appliedDistance),
                "dash right distance matches applied movement") &&
         expect(window.gameplayMovementHorizontalDistanceMeters <=
                    kExpectedManualFirstPersonDashDistanceMeters,
                "dash right distance within requested dash") &&
         expect(nearlyEqual(final.z - start.z, 0.0F), "dash right z unchanged");
}

bool productDashRejectsDuringCooldown() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "dash cooldown session created")) {
    return false;
  }

  iggy3d::applyProductGameplayActions(*session,
                                      dashActions(),
                                      window,
                                      "unit/gameplay_controller_dash");
  const iggy3d::Vec3 afterFirst = playerEntity(*session)->transform.position;
  iggy3d::applyProductGameplayActions(*session,
                                      dashActions(),
                                      window,
                                      "unit/gameplay_controller_dash_again");
  const iggy3d::Vec3 afterSecond = playerEntity(*session)->transform.position;

  return expect(window.gameplayDashRequested, "cooldown dash requested") &&
         expect(!window.gameplayDashAccepted, "cooldown dash rejected") &&
         expect(window.gameplayDashStatus == "cooldown", "cooldown dash status") &&
         expect(window.gameplayDashReasonCode == "gameplay_dash_cooldown",
                "cooldown dash reason") &&
         expect(window.gameplayDashCooldownRemainingSeconds > 0.0F,
                "cooldown remains") &&
         expect(nearlyEqual(afterFirst.x, afterSecond.x), "cooldown no x move") &&
         expect(nearlyEqual(afterFirst.z, afterSecond.z), "cooldown no z move");
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
                  productWasdUsesCameraRelativeYawZero() &&
                  productMoveUsesCameraYaw() &&
                  productMoveNormalizesDiagonalToTunedStep() &&
                  productSprintUsesSprintProfileAndStep() &&
                  productMoveUsesRuntimeTunedWindowSpeed() &&
                  productMoveUsesRuntimeTunedGroundAcceleration() &&
                  productGroundAccelerationApproachesMaxSpeed() &&
                  productHigherGroundAccelerationReachesSpeedFaster() &&
                  productGroundDecelerationDecaysRetainedVelocity() &&
                  productHigherGroundDecelerationStopsFaster() &&
                  productJumpRaisesPlayerAndRecordsProof() &&
                  productJumpCanMoveForwardInSameFrame() &&
                  productJumpAirControlScalesAirborneMove() &&
                  productJumpUsesClamberTraversalWhenCandidateIsLocal() &&
                  productJumpUsesWallJumpWhenAirborneNearWall() &&
                  productJumpRejectsWallJumpNearGenericWall() &&
                  productJumpRejectsDoubleJumpWhileAirborne() &&
                  productJumpFallsAndLands() &&
                  productJumpLandsOnElevatedWalkableFloor() &&
                  productFallThroughHoleLandsOnLowerWalkableFloor() &&
                  productMoveOffUpperFloorStartsFallingImmediately() &&
                  productResetZoneReturnsPlayerToSpawn() &&
                  productFallOutBelowLowestFloorReturnsPlayerToSpawn() &&
                  productDashMovesForwardAndRecordsProof() &&
                  productDashUsesRuntimeTunedWindowDistance() &&
                  productDashUsesMoveIntentDirection() &&
                  productDashRejectsDuringCooldown() &&
                  defaultOffMoveWithCollisionSurfacesUsesLegacyPath() &&
                  optInMoveWithCollisionSurfacesUsesPhysicsPlanner() &&
                  optInMoveWithoutCollisionSurfacesRecordsNoSurfaces();
  if (!ok) {
    return EXIT_FAILURE;
  }
  std::cout << "product_gameplay_controller_tests=pass\n";
  return EXIT_SUCCESS;
}
