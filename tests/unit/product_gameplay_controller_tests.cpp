#include "app/iggy3d/gameplay/Controller.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>

#include "app/iggy3d/ascii_room/Activation.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
#include "app/iggy3d/gameplay/MovementTuning.hpp"
#include "app/iggy3d/gameplay/ProductRoomStore.hpp"
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
  window.asciiRoomDraft.text =
      "#######\n"
      "#.....#\n"
      "#..P..#\n"
      "#.....#\n"
      "#..$.E#\n"
      "#######\n";
  window.asciiRoomDraft.roomId = "gameplay_controller_step_room";
  window.asciiRoomDraft.sourceName = "unit/gameplay_controller_step_room.iggyroom.txt";
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
  return iggy3d::productActiveRoomCollisionSurfaces(iggy3d::activeRoomCollision(window));
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
  iggy3d::activeRoom(window).loaded = true;
  iggy3d::activeRoom(window).status = "loaded";
  iggy3d::activeRoom(window).reasonCode = "active_room_loaded";
  iggy3d::activeRoom(window).source = "unit";
  iggy3d::activeRoom(window).roomId = "product_clamber_test";
  iggy3d::activeRoom(window).sourceName = "unit/product_clamber_test";
  iggy3d::activeRoom(window).room = makeProductClamberRoom();
  iggy3d::activeRoom(window).staticMeshCount = iggy3d::activeRoom(window).room.staticMeshes.size();
  iggy3d::activeRoom(window).spatialSurfaceCount = iggy3d::activeRoom(window).room.spatialSurfaces.size();
  iggy3d::activeRoom(window).walkableSurfaceCount = 2U;
  iggy3d::activeRoom(window).actorBlockerSurfaceCount = 1U;
  iggy3d::activeRoomCollision(window) =
      iggy3d::buildProductActiveRoomCollision(iggy3d::activeRoom(window), session.state());
}

void setWallJumpActiveRoom(iggy3d::ProductAppWindowState& window,
                           const iggy3d::Session& session,
                           bool authoredWallJump = true) {
  iggy3d::activeRoom(window).loaded = true;
  iggy3d::activeRoom(window).status = "loaded";
  iggy3d::activeRoom(window).reasonCode = "active_room_loaded";
  iggy3d::activeRoom(window).source = "unit";
  iggy3d::activeRoom(window).roomId = "product_wall_jump_test";
  iggy3d::activeRoom(window).sourceName = "unit/product_wall_jump_test";
  iggy3d::activeRoom(window).room = makeProductWallJumpRoom(authoredWallJump);
  iggy3d::activeRoom(window).staticMeshCount = iggy3d::activeRoom(window).room.staticMeshes.size();
  iggy3d::activeRoom(window).spatialSurfaceCount = iggy3d::activeRoom(window).room.spatialSurfaces.size();
  iggy3d::activeRoom(window).walkableSurfaceCount = 1U;
  iggy3d::activeRoom(window).actorBlockerSurfaceCount = 1U;
  iggy3d::activeRoomCollision(window) =
      iggy3d::buildProductActiveRoomCollision(iggy3d::activeRoom(window), session.state());
}

void setLayeredFloorActiveRoom(iggy3d::ProductAppWindowState& window,
                               const iggy3d::Session& session) {
  iggy3d::activeRoom(window).loaded = true;
  iggy3d::activeRoom(window).status = "loaded";
  iggy3d::activeRoom(window).reasonCode = "active_room_loaded";
  iggy3d::activeRoom(window).source = "unit";
  iggy3d::activeRoom(window).roomId = "product_layered_floor_test";
  iggy3d::activeRoom(window).sourceName = "unit/product_layered_floor_test";
  iggy3d::activeRoom(window).room = makeProductLayeredFloorRoom();
  iggy3d::activeRoom(window).staticMeshCount = iggy3d::activeRoom(window).room.staticMeshes.size();
  iggy3d::activeRoom(window).spatialSurfaceCount = iggy3d::activeRoom(window).room.spatialSurfaces.size();
  iggy3d::activeRoom(window).walkableSurfaceCount = 2U;
  iggy3d::activeRoom(window).actorBlockerSurfaceCount = 0U;
  iggy3d::activeRoomCollision(window) =
      iggy3d::buildProductActiveRoomCollision(iggy3d::activeRoom(window), session.state());
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

iggy3d::ActionState releaseJumpActions() {
  iggy3d::ActionState actions;
  iggy3d::recordAction(actions,
                       iggy3d::InputAction::PlayerJump,
                       false,
                       false,
                       true,
                       0.0F);
  return actions;
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
  return expect(window.gameplayCommand.accepted, "manual move accepted") &&
         expect(window.gameplayMovement.status == "moved",
                "manual movement status");
}

bool productMoveUsesTunedManualStep() {
  iggy3d::Vec3 delta;
  iggy3d::ProductAppWindowState window;
  if (!runManualMove(0.0F, 1.0F, 0.0F, delta, &window)) {
    return false;
  }

  return expect(window.gameplayCommand.accepted, "move accepted") &&
         expect(window.gameplayMovement.status == "moved", "movement status") &&
         expect(!window.physicsMovementPlanner.enabled,
                "default physics planner disabled") &&
         expect(!window.physicsMovementPlanner.requested,
                "default physics planner not requested") &&
         expect(!window.physicsMovementPlanner.used,
                "default physics planner not used") &&
         expect(window.physicsMovementPlanner.status ==
                    "physics_movement_planner_disabled",
                "default physics planner status") &&
         expect(window.gameplayMovement.profile == kExpectedManualFirstPersonProfile,
                "manual movement profile") &&
         expect(window.gameplayMovement.state ==
                    iggy3d::ProductGameplayMovementState::MovingGrounded,
                "manual movement state") &&
         expect(window.gameplayMovement.grounded, "manual movement grounded") &&
         expect(window.gameplayMovement.horizontalSpeedMetersPerSecond > 0.0F,
                "manual movement horizontal speed proof") &&
         expect(nearlyEqual(window.gameplayMovement.maxSpeedMetersPerSecond,
                            kExpectedManualFirstPersonSpeedMetersPerSecond),
                "manual movement speed") &&
         expect(nearlyEqual(window.gameplayMovement.horizontalDistanceMeters,
                            kExpectedManualFirstPersonStepMeters),
                "horizontal distance is profile step") &&
         expect(nearlyEqual(delta.x, 0.0F), "forward x unchanged") &&
         expect(nearlyEqual(delta.z, -kExpectedManualFirstPersonStepMeters),
                "W moves forward along camera -Z at yaw zero");
}

bool productMovementStateReportsIdleGroundedWithoutInput() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "idle state session created")) {
    return false;
  }

  iggy3d::applyProductGameplayActions(*session,
                                      noActions(),
                                      window,
                                      "unit/gameplay_controller_idle_state");

  return expect(window.gameplayMovement.state ==
                    iggy3d::ProductGameplayMovementState::IdleGrounded,
                "idle grounded state") &&
         expect(window.gameplayMovement.grounded, "idle grounded proof") &&
         expect(nearlyEqual(window.gameplayMovement.horizontalSpeedMetersPerSecond,
                            0.0F),
                "idle horizontal speed proof");
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

  return expect(window.gameplayCommand.accepted, "diagonal move accepted") &&
         expect(window.gameplayMovement.status == "moved",
                "diagonal movement status") &&
         expect(window.gameplayMovement.profile == kExpectedManualFirstPersonProfile,
                "diagonal movement profile") &&
         expect(nearlyEqual(window.gameplayMovement.horizontalDistanceMeters,
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

  return expect(window.gameplayCommand.accepted, "sprint move accepted") &&
         expect(window.gameplayMovement.status == "moved",
                "sprint movement status") &&
         expect(window.gameplayMovement.profile ==
                    kExpectedManualFirstPersonSprintProfile,
                "sprint movement profile") &&
         expect(nearlyEqual(window.gameplayMovement.maxSpeedMetersPerSecond,
                            kExpectedManualFirstPersonSprintSpeedMetersPerSecond),
                "sprint movement speed") &&
         expect(nearlyEqual(window.gameplayMovement.horizontalDistanceMeters,
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

  window.gameplayMovement.tuning.walkSpeedMetersPerSecond = 1.2F;
  const float expectedStep =
      window.gameplayMovement.tuning.walkSpeedMetersPerSecond *
      window.gameplayMovement.tuning.inputStepSeconds;
  const iggy3d::Vec3 start = playerEntity(*session)->transform.position;
  iggy3d::applyProductGameplayActions(*session,
                                      manualMoveActions(0.0F, 1.0F),
                                      window,
                                      "unit/gameplay_controller_runtime_tuning");
  const iggy3d::Vec3 final = playerEntity(*session)->transform.position;

  return expect(window.gameplayCommand.accepted, "runtime tuned move accepted") &&
         expect(nearlyEqual(window.gameplayMovement.maxSpeedMetersPerSecond, 1.2F),
                "runtime tuned speed proof") &&
         expect(nearlyEqual(window.gameplayMovement.horizontalDistanceMeters,
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

  window.gameplayMovement.tuning.groundAccelerationMetersPerSecondSquared = 33.0F;
  const float expectedStep =
      window.gameplayMovement.tuning.groundAccelerationMetersPerSecondSquared *
      window.gameplayMovement.tuning.inputStepSeconds *
      window.gameplayMovement.tuning.inputStepSeconds;
  const iggy3d::Vec3 start = playerEntity(*session)->transform.position;
  iggy3d::applyProductGameplayActions(*session,
                                      manualMoveActions(0.0F, 1.0F),
                                      window,
                                      "unit/gameplay_controller_accel_tuning");
  const iggy3d::Vec3 final = playerEntity(*session)->transform.position;

  return expect(window.gameplayCommand.accepted,
                "runtime acceleration tuned move accepted") &&
         expect(nearlyEqual(window.gameplayMovement.horizontalDistanceMeters,
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

  window.gameplayMovement.tuning.groundAccelerationMetersPerSecondSquared = 33.0F;
  iggy3d::applyProductGameplayActions(*session,
                                      forwardMoveActions(),
                                      window,
                                      "unit/gameplay_controller_accel_ramp_first");
  const float firstDistance = window.gameplayMovement.horizontalDistanceMeters;
  for (int frame = 0; frame < 8; ++frame) {
    iggy3d::applyProductGameplayActions(*session,
                                        forwardMoveActions(),
                                        window,
                                        "unit/gameplay_controller_accel_ramp_later");
  }
  const float laterDistance = window.gameplayMovement.horizontalDistanceMeters;

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

  slowWindow.gameplayMovement.tuning.groundAccelerationMetersPerSecondSquared =
      16.5F;
  fastWindow.gameplayMovement.tuning.groundAccelerationMetersPerSecondSquared =
      66.0F;
  iggy3d::applyProductGameplayActions(*slowSession,
                                      forwardMoveActions(),
                                      slowWindow,
                                      "unit/gameplay_controller_accel_slow");
  iggy3d::applyProductGameplayActions(*fastSession,
                                      forwardMoveActions(),
                                      fastWindow,
                                      "unit/gameplay_controller_accel_fast");

  return expect(fastWindow.gameplayMovement.horizontalDistanceMeters >
                    slowWindow.gameplayMovement.horizontalDistanceMeters,
                "higher acceleration moves farther on first frame") &&
         expect(std::fabs(fastWindow.gameplayMovement.groundVelocityZ) >
                    std::fabs(slowWindow.gameplayMovement.groundVelocityZ),
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
  const float startingSpeed = std::fabs(window.gameplayMovement.groundVelocityZ);
  window.gameplayMovement.tuning.groundDecelerationMetersPerSecondSquared = 33.0F;
  iggy3d::ActionState noInput;
  iggy3d::applyProductGameplayActions(*session,
                                      noInput,
                                      window,
                                      "unit/gameplay_controller_decel_release");
  const float decayedSpeed = std::fabs(window.gameplayMovement.groundVelocityZ);

  return expect(window.gameplayCommand.accepted,
                "deceleration submits retained movement") &&
         expect(window.gameplayMovement.horizontalDistanceMeters > 0.0F,
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
  slowWindow.gameplayMovement.tuning.groundDecelerationMetersPerSecondSquared =
      16.5F;
  fastWindow.gameplayMovement.tuning.groundDecelerationMetersPerSecondSquared =
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

  return expect(fastWindow.gameplayMovement.horizontalDistanceMeters <
                    slowWindow.gameplayMovement.horizontalDistanceMeters,
                "higher deceleration moves less after release") &&
         expect(std::fabs(fastWindow.gameplayMovement.groundVelocityZ) <
                    std::fabs(slowWindow.gameplayMovement.groundVelocityZ),
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

  return expect(window.gameplayJump.requested, "jump requested") &&
         expect(window.gameplayJump.accepted, "jump accepted") &&
         expect(window.gameplayJump.active, "jump remains active after first step") &&
         expect(window.gameplayMovement.state ==
                    iggy3d::ProductGameplayMovementState::Rising,
                "jump movement state rising") &&
         expect(!window.gameplayMovement.grounded, "jump airborne proof") &&
         expect(window.gameplayJump.status == "airborne", "jump airborne status") &&
         expect(window.gameplayJump.reasonCode == "gameplay_jump_airborne",
                "jump airborne reason") &&
         expect(window.gameplay.playerPositionChanged, "jump changed player position") &&
         expect(final.y > start.y, "jump raises player y") &&
         expect(nearlyEqual(window.gameplayJump.groundY, start.y), "jump ground y") &&
         expect(nearlyEqual(window.gameplayJump.startY, start.y), "jump start y") &&
         expect(nearlyEqual(window.gameplayJump.finalY, final.y), "jump final y") &&
         expect(window.gameplayJump.heightMeters > 0.0F, "jump height positive") &&
         expect(window.gameplayJump.velocityMetersPerSecond <
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

  return expect(window.gameplayJump.requested, "jump forward jump requested") &&
         expect(window.gameplayJump.accepted, "jump forward jump accepted") &&
         expect(window.gameplayJump.active, "jump forward remains airborne") &&
         expect(window.gameplayMovement.state ==
                    iggy3d::ProductGameplayMovementState::AirborneControl,
                "jump forward airborne control state") &&
         expect(!window.gameplayMovement.grounded,
                "jump forward airborne proof") &&
         expect(window.gameplayJump.status == "airborne",
                "jump forward jump airborne") &&
         expect(window.gameplayMovement.attempted,
                "jump forward movement attempted") &&
         expect(window.gameplayMovement.status == "moved",
                "jump forward movement status") &&
         expect(window.gameplayMovement.debugAvailable,
                "jump forward movement debug") &&
         expect(window.gameplayMovement.reasonCode == "airborne_manual_move",
                "jump forward movement reason") &&
         expect(window.gameplayMovement.policyBand == "airborne",
                "jump forward movement policy") &&
         expect(!window.gameplayCommand.submitted,
                "jump forward avoids grounded command") &&
         expect(nearlyEqual(window.gameplayMovement.horizontalDistanceMeters,
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

  window.gameplayMovement.tuning.airControlMultiplier = 0.25F;
  const float expectedStep =
      window.gameplayMovement.tuning.walkSpeedMetersPerSecond *
      window.gameplayMovement.tuning.inputStepSeconds *
      window.gameplayMovement.tuning.airControlMultiplier;
  const iggy3d::Vec3 start = playerEntity(*session)->transform.position;
  iggy3d::applyProductGameplayActions(*session,
                                      jumpForwardActions(),
                                      window,
                                      "unit/gameplay_controller_air_control");
  const iggy3d::Vec3 final = playerEntity(*session)->transform.position;

  return expect(window.gameplayJump.accepted,
                "air control tuned jump accepted") &&
         expect(window.gameplayMovement.attempted,
                "air control tuned movement attempted") &&
         expect(nearlyEqual(window.gameplayMovement.horizontalDistanceMeters,
                            expectedStep),
                "air control tuned horizontal distance") &&
         expect(nearlyEqual(final.z - start.z, -expectedStep),
                "air control tuned final z");
}

bool productJumpWithinCoyoteWindowSucceedsAfterLeavingGround() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "coyote jump session created")) {
    return false;
  }

  setLayeredFloorActiveRoom(window, *session);
  setPlayerPosition(*session, {1.34F, 4.0F, 0.0F});
  iggy3d::applyProductGameplayActions(*session,
                                      manualMoveActions(1.0F, 0.0F),
                                      window,
                                      "unit/gameplay_controller_coyote_leave",
                                      activeSurfaces(window));
  const bool falling = window.gameplayJump.active &&
                       window.gameplayJump.status == "falling";
  iggy3d::applyProductGameplayActions(*session,
                                      jumpActions(),
                                      window,
                                      "unit/gameplay_controller_coyote_jump",
                                      activeSurfaces(window));

  return expect(falling, "coyote setup starts falling") &&
         expect(window.gameplayJump.accepted, "coyote jump accepted") &&
         expect(window.gameplayJump.active, "coyote jump active") &&
         expect(window.gameplayJump.coyoteSecondsRemaining == 0.0F,
                "coyote jump consumes timer") &&
         expect(window.gameplayJump.velocityMetersPerSecond > 0.0F,
                "coyote jump has upward velocity");
}

bool productJumpOutsideCoyoteWindowRejectsAndBuffers() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "expired coyote session created")) {
    return false;
  }

  setLayeredFloorActiveRoom(window, *session);
  setPlayerPosition(*session, {1.34F, 4.0F, 0.0F});
  iggy3d::applyProductGameplayActions(*session,
                                      manualMoveActions(1.0F, 0.0F),
                                      window,
                                      "unit/gameplay_controller_coyote_expire_leave",
                                      activeSurfaces(window));
  for (int frame = 0; frame < 8; ++frame) {
    iggy3d::applyProductGameplayActions(
        *session,
        noActions(),
        window,
        "unit/gameplay_controller_coyote_expire_tick",
        activeSurfaces(window));
  }
  iggy3d::applyProductGameplayActions(*session,
                                      jumpActions(),
                                      window,
                                      "unit/gameplay_controller_coyote_expired",
                                      activeSurfaces(window));

  return expect(!window.gameplayJump.accepted,
                "expired coyote jump rejected") &&
         expect(window.gameplayJump.status == "already_airborne",
                "expired coyote jump status") &&
         expect(window.gameplayJump.bufferSecondsRemaining > 0.0F,
                "expired coyote jump is buffered");
}

bool productBufferedJumpFiresOnLanding() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "buffered jump session created")) {
    return false;
  }

  setPlayerPosition(*session, {0.0F, 0.01F, 0.0F});
  window.gameplayJump.active = true;
  window.gameplayJump.velocityMetersPerSecond = -1.0F;
  window.gameplayJump.groundY = 0.0F;
  window.gameplayJump.startY = 0.01F;
  iggy3d::applyProductGameplayActions(*session,
                                      jumpActions(),
                                      window,
                                      "unit/gameplay_controller_buffer_press");
  const bool buffered = window.gameplayJump.bufferSecondsRemaining > 0.0F;
  iggy3d::applyProductGameplayActions(*session,
                                      noActions(),
                                      window,
                                      "unit/gameplay_controller_buffer_land");

  return expect(buffered, "jump press buffered before landing") &&
         expect(window.gameplayJump.accepted, "buffered jump accepted on landing") &&
         expect(window.gameplayJump.active, "buffered jump leaves player airborne") &&
         expect(window.gameplayJump.bufferSecondsRemaining == 0.0F,
                "buffered jump consumes buffer") &&
         expect(window.gameplayJump.velocityMetersPerSecond > 0.0F,
                "buffered jump has upward velocity");
}

bool productEarlyJumpReleaseCutsJumpHeight() {
  std::optional<iggy3d::Session> heldSession;
  iggy3d::ProductAppWindowState heldWindow = makeGameplayWindow(heldSession);
  std::optional<iggy3d::Session> cutSession;
  iggy3d::ProductAppWindowState cutWindow = makeGameplayWindow(cutSession);
  if (!expect(heldSession.has_value() && cutSession.has_value(),
              "jump cut sessions created")) {
    return false;
  }

  iggy3d::applyProductGameplayActions(*heldSession,
                                      jumpActions(),
                                      heldWindow,
                                      "unit/gameplay_controller_jump_held");
  iggy3d::applyProductGameplayActions(*cutSession,
                                      jumpActions(),
                                      cutWindow,
                                      "unit/gameplay_controller_jump_cut_start");
  iggy3d::applyProductGameplayActions(*cutSession,
                                      releaseJumpActions(),
                                      cutWindow,
                                      "unit/gameplay_controller_jump_cut_release");
  float heldMaxY = playerEntity(*heldSession)->transform.position.y;
  float cutMaxY = playerEntity(*cutSession)->transform.position.y;
  for (int frame = 0; frame < 20; ++frame) {
    iggy3d::applyProductGameplayActions(*heldSession,
                                        noActions(),
                                        heldWindow,
                                        "unit/gameplay_controller_jump_held_tick");
    iggy3d::applyProductGameplayActions(*cutSession,
                                        noActions(),
                                        cutWindow,
                                        "unit/gameplay_controller_jump_cut_tick");
    heldMaxY = std::max(heldMaxY, playerEntity(*heldSession)->transform.position.y);
    cutMaxY = std::max(cutMaxY, playerEntity(*cutSession)->transform.position.y);
  }

  return expect(cutWindow.gameplayJump.cutApplied,
                "early release applies jump cut") &&
         expect(cutMaxY < heldMaxY, "early release produces lower jump");
}

bool productFallGravityMultiplierDescendsFaster() {
  std::optional<iggy3d::Session> normalSession;
  iggy3d::ProductAppWindowState normalWindow = makeGameplayWindow(normalSession);
  std::optional<iggy3d::Session> fastSession;
  iggy3d::ProductAppWindowState fastWindow = makeGameplayWindow(fastSession);
  if (!expect(normalSession.has_value() && fastSession.has_value(),
              "fall multiplier sessions created")) {
    return false;
  }

  setPlayerPosition(*normalSession, {0.0F, 3.0F, 0.0F});
  setPlayerPosition(*fastSession, {0.0F, 3.0F, 0.0F});
  normalWindow.gameplayJump.active = true;
  normalWindow.gameplayJump.velocityMetersPerSecond = 0.0F;
  normalWindow.gameplayJump.groundY = 0.0F;
  normalWindow.gameplayJump.startY = 3.0F;
  normalWindow.gameplayMovement.tuning.fallGravityMultiplier = 1.0F;
  fastWindow.gameplayJump.active = true;
  fastWindow.gameplayJump.velocityMetersPerSecond = 0.0F;
  fastWindow.gameplayJump.groundY = 0.0F;
  fastWindow.gameplayJump.startY = 3.0F;
  fastWindow.gameplayMovement.tuning.fallGravityMultiplier = 3.0F;

  iggy3d::applyProductGameplayActions(*normalSession,
                                      noActions(),
                                      normalWindow,
                                      "unit/gameplay_controller_fall_normal");
  iggy3d::applyProductGameplayActions(*fastSession,
                                      noActions(),
                                      fastWindow,
                                      "unit/gameplay_controller_fall_fast");

  return expect(playerEntity(*fastSession)->transform.position.y <
                    playerEntity(*normalSession)->transform.position.y,
                "higher fall multiplier descends farther") &&
         expect(normalWindow.gameplayMovement.state ==
                    iggy3d::ProductGameplayMovementState::Falling,
                "normal fall state") &&
         expect(fastWindow.gameplayMovement.state ==
                    iggy3d::ProductGameplayMovementState::Falling,
                "fast fall state") &&
         expect(fastWindow.gameplayJump.velocityMetersPerSecond <
                    normalWindow.gameplayJump.velocityMetersPerSecond,
                "higher fall multiplier has lower velocity");
}

bool productMovementStateReportsBlockedOrSlidingFromCollisionProof() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "blocked state session created")) {
    return false;
  }

  window.gameplayMovement.blocked = true;
  window.gameplayMovement.clamped = true;
  window.gameplayMovement.blockedReason = "blocked_by_collision";
  window.gameplayMovement.hitSurfaceId = "unit_wall_actor_blocker";
  iggy3d::applyProductGameplayActions(*session,
                                      noActions(),
                                      window,
                                      "unit/gameplay_controller_blocked_state");

  return expect(window.gameplayMovement.state ==
                    iggy3d::ProductGameplayMovementState::BlockedOrSliding,
                "blocked or sliding state") &&
         expect(window.gameplayMovement.grounded,
                "blocked state remains grounded") &&
         expect(window.gameplayMovement.hitSurfaceId == "unit_wall_actor_blocker",
                "blocked state keeps hit surface proof");
}

bool productWallRunCandidateReportsAirborneSideWallContact() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "wall-run candidate session created")) {
    return false;
  }

  setPlayerPosition(*session, {3.0F, 0.80F, 0.65F});
  setWallJumpActiveRoom(window, *session);
  window.gameplayJump.active = true;
  window.gameplayJump.velocityMetersPerSecond = 1.0F;
  window.gameplayJump.groundY = 0.0F;
  window.gameplayJump.startY = 0.80F;
  iggy3d::applyProductGameplayActions(*session,
                                      manualMoveActions(1.0F, 0.0F),
                                      window,
                                      "unit/gameplay_controller_wall_run_candidate",
                                      activeSurfaces(window));

  return expect(window.gameplayWallRun.candidateAvailable,
                "wall-run candidate available") &&
         expect(window.gameplayWallRun.candidateStatus == "wall_run_candidate",
                "wall-run candidate status") &&
         expect(window.gameplayWallRun.surfaceId ==
                    "wall_jump_wall_actor_blocker",
                "wall-run candidate surface") &&
         expect(window.gameplayWallRun.approachSpeedMetersPerSecond >=
                    window.gameplayMovement.tuning.wallRunMinSpeedMetersPerSecond,
                "wall-run candidate speed") &&
         expect(window.gameplayWallRun.side != "none",
                "wall-run candidate side proof");
}

bool productWallRunCandidateRejectsGroundedContact() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "grounded wall-run session created")) {
    return false;
  }

  setPlayerPosition(*session, {3.0F, 0.05F, 0.65F});
  setWallJumpActiveRoom(window, *session);
  iggy3d::applyProductGameplayActions(*session,
                                      noActions(),
                                      window,
                                      "unit/gameplay_controller_wall_run_grounded",
                                      activeSurfaces(window));

  return expect(!window.gameplayWallRun.candidateAvailable,
                "grounded wall-run candidate rejected") &&
         expect(window.gameplayWallRun.candidateReasonCode == "wall_run_grounded",
                "grounded wall-run reason");
}

bool productWallRunCandidateRejectsLowSpeed() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "low-speed wall-run session created")) {
    return false;
  }

  setPlayerPosition(*session, {3.0F, 0.80F, 0.65F});
  setWallJumpActiveRoom(window, *session);
  window.gameplayJump.active = true;
  window.gameplayJump.velocityMetersPerSecond = 1.0F;
  window.gameplayJump.groundY = 0.0F;
  window.gameplayJump.startY = 0.80F;
  window.gameplayMovement.tuning.wallRunMinSpeedMetersPerSecond = 10.0F;
  iggy3d::applyProductGameplayActions(*session,
                                      manualMoveActions(1.0F, 0.0F),
                                      window,
                                      "unit/gameplay_controller_wall_run_low_speed",
                                      activeSurfaces(window));

  return expect(!window.gameplayWallRun.candidateAvailable,
                "low-speed wall-run candidate rejected") &&
         expect(window.gameplayWallRun.candidateReasonCode == "wall_run_low_speed",
                "low-speed wall-run reason");
}

bool productWallRunCandidateRejectsNoWallContact() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "no-wall wall-run session created")) {
    return false;
  }

  window.gameplayJump.active = true;
  window.gameplayJump.velocityMetersPerSecond = 1.0F;
  window.gameplayJump.groundY = 0.0F;
  window.gameplayJump.startY = 0.80F;
  iggy3d::applyProductGameplayActions(*session,
                                      manualMoveActions(1.0F, 0.0F),
                                      window,
                                      "unit/gameplay_controller_wall_run_no_wall",
                                      activeSurfaces(window));

  return expect(!window.gameplayWallRun.candidateAvailable,
                "no-wall wall-run candidate rejected") &&
         expect(window.gameplayWallRun.candidateReasonCode ==
                    "wall_run_no_wall_contact",
                "no-wall wall-run reason");
}

bool productWallRunMinSpeedTuningControlsCandidateThreshold() {
  std::optional<iggy3d::Session> slowSession;
  iggy3d::ProductAppWindowState slowWindow = makeGameplayWindow(slowSession);
  std::optional<iggy3d::Session> fastSession;
  iggy3d::ProductAppWindowState fastWindow = makeGameplayWindow(fastSession);
  if (!expect(slowSession.has_value() && fastSession.has_value(),
              "wall-run threshold sessions created")) {
    return false;
  }

  setPlayerPosition(*slowSession, {3.0F, 0.80F, 0.65F});
  setPlayerPosition(*fastSession, {3.0F, 0.80F, 0.65F});
  setWallJumpActiveRoom(slowWindow, *slowSession);
  setWallJumpActiveRoom(fastWindow, *fastSession);
  slowWindow.gameplayJump.active = true;
  slowWindow.gameplayJump.velocityMetersPerSecond = 1.0F;
  slowWindow.gameplayJump.groundY = 0.0F;
  slowWindow.gameplayJump.startY = 0.80F;
  slowWindow.gameplayMovement.tuning.wallRunMinSpeedMetersPerSecond = 10.0F;
  fastWindow.gameplayJump.active = true;
  fastWindow.gameplayJump.velocityMetersPerSecond = 1.0F;
  fastWindow.gameplayJump.groundY = 0.0F;
  fastWindow.gameplayJump.startY = 0.80F;
  fastWindow.gameplayMovement.tuning.wallRunMinSpeedMetersPerSecond = 0.5F;

  iggy3d::applyProductGameplayActions(*slowSession,
                                      manualMoveActions(1.0F, 0.0F),
                                      slowWindow,
                                      "unit/gameplay_controller_wall_run_high_threshold",
                                      activeSurfaces(slowWindow));
  iggy3d::applyProductGameplayActions(*fastSession,
                                      manualMoveActions(1.0F, 0.0F),
                                      fastWindow,
                                      "unit/gameplay_controller_wall_run_low_threshold",
                                      activeSurfaces(fastWindow));

  return expect(!slowWindow.gameplayWallRun.candidateAvailable,
                "high wall-run speed threshold rejects") &&
         expect(fastWindow.gameplayWallRun.candidateAvailable,
                "low wall-run speed threshold accepts");
}

bool productWallRunCandidateDoesNotChangeMovementOutput() {
  std::optional<iggy3d::Session> candidateSession;
  iggy3d::ProductAppWindowState candidateWindow =
      makeGameplayWindow(candidateSession);
  std::optional<iggy3d::Session> rejectedSession;
  iggy3d::ProductAppWindowState rejectedWindow =
      makeGameplayWindow(rejectedSession);
  if (!expect(candidateSession.has_value() && rejectedSession.has_value(),
              "wall-run non-invasive sessions created")) {
    return false;
  }

  setPlayerPosition(*candidateSession, {3.0F, 0.80F, 0.65F});
  setPlayerPosition(*rejectedSession, {3.0F, 0.80F, 0.65F});
  setWallJumpActiveRoom(candidateWindow, *candidateSession);
  setWallJumpActiveRoom(rejectedWindow, *rejectedSession);
  candidateWindow.gameplayJump.active = true;
  candidateWindow.gameplayJump.velocityMetersPerSecond = 1.0F;
  candidateWindow.gameplayJump.groundY = 0.0F;
  candidateWindow.gameplayJump.startY = 0.80F;
  candidateWindow.gameplayMovement.tuning.wallRunMinSpeedMetersPerSecond = 0.5F;
  rejectedWindow.gameplayJump.active = true;
  rejectedWindow.gameplayJump.velocityMetersPerSecond = 1.0F;
  rejectedWindow.gameplayJump.groundY = 0.0F;
  rejectedWindow.gameplayJump.startY = 0.80F;
  rejectedWindow.gameplayMovement.tuning.wallRunMinSpeedMetersPerSecond = 10.0F;

  iggy3d::applyProductGameplayActions(*candidateSession,
                                      manualMoveActions(1.0F, 0.0F),
                                      candidateWindow,
                                      "unit/gameplay_controller_wall_run_noninvasive_on",
                                      activeSurfaces(candidateWindow));
  iggy3d::applyProductGameplayActions(*rejectedSession,
                                      manualMoveActions(1.0F, 0.0F),
                                      rejectedWindow,
                                      "unit/gameplay_controller_wall_run_noninvasive_off",
                                      activeSurfaces(rejectedWindow));

  const iggy3d::Vec3 candidateFinal =
      playerEntity(*candidateSession)->transform.position;
  const iggy3d::Vec3 rejectedFinal =
      playerEntity(*rejectedSession)->transform.position;
  return expect(candidateWindow.gameplayWallRun.candidateAvailable,
                "candidate comparison enabled") &&
         expect(!rejectedWindow.gameplayWallRun.candidateAvailable,
                "candidate comparison rejected") &&
         expect(nearlyEqual(candidateFinal.x, rejectedFinal.x),
                "wall-run candidate does not change final x") &&
         expect(nearlyEqual(candidateFinal.y, rejectedFinal.y),
                "wall-run candidate does not change final y") &&
         expect(nearlyEqual(candidateFinal.z, rejectedFinal.z),
                "wall-run candidate does not change final z");
}

void seedAirborneWallRunSetup(iggy3d::Session& session,
                              iggy3d::ProductAppWindowState& window) {
  setPlayerPosition(session, {3.0F, 0.80F, 0.65F});
  setWallJumpActiveRoom(window, session);
  window.gameplayJump.active = true;
  window.gameplayJump.velocityMetersPerSecond = -1.0F;
  window.gameplayJump.groundY = 0.0F;
  window.gameplayJump.startY = 0.80F;
  window.gameplayMovement.tuning.wallRunMinSpeedMetersPerSecond = 0.5F;
}

bool enterWallRun(iggy3d::Session& session,
                  iggy3d::ProductAppWindowState& window) {
  iggy3d::applyProductGameplayActions(session,
                                      manualMoveActions(1.0F, 0.0F),
                                      window,
                                      "unit/gameplay_controller_wall_run_enter",
                                      activeSurfaces(window));
  return window.gameplayWallRun.active;
}

bool productWallRunCandidateEntersActiveState() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "wall-run active session created")) {
    return false;
  }

  seedAirborneWallRunSetup(*session, window);
  const bool entered = enterWallRun(*session, window);

  return expect(entered, "wall-run enters active state") &&
         expect(window.gameplayMovement.state ==
                    iggy3d::ProductGameplayMovementState::WallRunning,
                "wall-run movement state") &&
         expect(window.gameplayWallRun.status == "wall_run_active",
                "wall-run active status") &&
         expect(window.gameplayWallRun.reasonCode == "wall_run_started",
                "wall-run start reason") &&
         expect(window.gameplayWallRun.remainingSeconds > 0.0F,
                "wall-run remaining time");
}

bool productWallRunReducesFallingAgainstNormalAirborneFall() {
  std::optional<iggy3d::Session> wallSession;
  iggy3d::ProductAppWindowState wallWindow = makeGameplayWindow(wallSession);
  std::optional<iggy3d::Session> normalSession;
  iggy3d::ProductAppWindowState normalWindow = makeGameplayWindow(normalSession);
  if (!expect(wallSession.has_value() && normalSession.has_value(),
              "wall-run fall comparison sessions created")) {
    return false;
  }

  seedAirborneWallRunSetup(*wallSession, wallWindow);
  seedAirborneWallRunSetup(*normalSession, normalWindow);
  normalWindow.gameplayMovement.tuning.wallRunMinSpeedMetersPerSecond = 10.0F;
  const bool entered = enterWallRun(*wallSession, wallWindow);
  iggy3d::applyProductGameplayActions(*normalSession,
                                      manualMoveActions(1.0F, 0.0F),
                                      normalWindow,
                                      "unit/gameplay_controller_wall_run_normal_setup",
                                      activeSurfaces(normalWindow));
  const float wallStartY = playerEntity(*wallSession)->transform.position.y;
  const float normalStartY = playerEntity(*normalSession)->transform.position.y;
  iggy3d::applyProductGameplayActions(*wallSession,
                                      manualMoveActions(1.0F, 0.0F),
                                      wallWindow,
                                      "unit/gameplay_controller_wall_run_fall_scaled",
                                      activeSurfaces(wallWindow));
  iggy3d::applyProductGameplayActions(*normalSession,
                                      manualMoveActions(1.0F, 0.0F),
                                      normalWindow,
                                      "unit/gameplay_controller_wall_run_fall_normal",
                                      activeSurfaces(normalWindow));

  const float wallFall =
      wallStartY - playerEntity(*wallSession)->transform.position.y;
  const float normalFall =
      normalStartY - playerEntity(*normalSession)->transform.position.y;
  return expect(entered, "wall-run fall comparison enters") &&
         expect(wallFall < normalFall, "wall-run reduces falling") &&
         expect(wallWindow.gameplayJump.velocityMetersPerSecond >
                    normalWindow.gameplayJump.velocityMetersPerSecond,
                "wall-run keeps vertical velocity higher");
}

bool productWallRunMovesAlongWallTangentOnly() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "wall-run tangent session created")) {
    return false;
  }

  seedAirborneWallRunSetup(*session, window);
  const bool entered = enterWallRun(*session, window);
  const iggy3d::Vec3 before = playerEntity(*session)->transform.position;
  iggy3d::applyProductGameplayActions(*session,
                                      manualMoveActions(1.0F, 0.0F),
                                      window,
                                      "unit/gameplay_controller_wall_run_tangent",
                                      activeSurfaces(window));
  const iggy3d::Vec3 after = playerEntity(*session)->transform.position;

  return expect(entered, "wall-run tangent enters") &&
         expect(after.x > before.x, "wall-run progresses along wall tangent") &&
         expect(nearlyEqual(after.z, before.z),
                "wall-run does not push into wall normal");
}

bool productWallRunTimerExpiryExits() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "wall-run timer session created")) {
    return false;
  }

  seedAirborneWallRunSetup(*session, window);
  window.gameplayMovement.tuning.wallRunDurationSeconds = 0.1F;
  const bool entered = enterWallRun(*session, window);
  for (int frame = 0; frame < 10 && window.gameplayWallRun.active; ++frame) {
    iggy3d::applyProductGameplayActions(
        *session,
        manualMoveActions(1.0F, 0.0F),
        window,
        "unit/gameplay_controller_wall_run_timer",
        activeSurfaces(window));
  }

  return expect(entered, "wall-run timer enters") &&
         expect(!window.gameplayWallRun.active, "wall-run timer exits") &&
         expect(window.gameplayWallRun.reasonCode == "wall_run_expired",
                "wall-run timer expiry reason");
}

bool productWallRunInputStopExits() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "wall-run input stop session created")) {
    return false;
  }

  seedAirborneWallRunSetup(*session, window);
  const bool entered = enterWallRun(*session, window);
  iggy3d::applyProductGameplayActions(*session,
                                      noActions(),
                                      window,
                                      "unit/gameplay_controller_wall_run_input_stop",
                                      activeSurfaces(window));

  return expect(entered, "wall-run input stop enters") &&
         expect(!window.gameplayWallRun.active, "wall-run input stop exits") &&
         expect(window.gameplayWallRun.reasonCode == "wall_run_input_stopped",
                "wall-run input stop reason");
}

bool productWallRunJumpInputExits() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "wall-run jump exit session created")) {
    return false;
  }

  seedAirborneWallRunSetup(*session, window);
  const bool entered = enterWallRun(*session, window);
  iggy3d::applyProductGameplayActions(*session,
                                      jumpActions(),
                                      window,
                                      "unit/gameplay_controller_wall_run_jump_exit",
                                      activeSurfaces(window));

  return expect(entered, "wall-run jump exit enters") &&
         expect(!window.gameplayWallRun.active, "wall-run jump exits") &&
         expect(window.gameplayWallRun.reasonCode == "wall_run_exit_jump",
                "wall-run jump exit reason");
}

bool productWallRunTuningChangesGravityAndDuration() {
  std::optional<iggy3d::Session> slowSession;
  iggy3d::ProductAppWindowState slowWindow = makeGameplayWindow(slowSession);
  std::optional<iggy3d::Session> fastSession;
  iggy3d::ProductAppWindowState fastWindow = makeGameplayWindow(fastSession);
  if (!expect(slowSession.has_value() && fastSession.has_value(),
              "wall-run tuning sessions created")) {
    return false;
  }

  seedAirborneWallRunSetup(*slowSession, slowWindow);
  seedAirborneWallRunSetup(*fastSession, fastWindow);
  slowWindow.gameplayMovement.tuning.wallRunGravityMultiplier = 0.1F;
  slowWindow.gameplayMovement.tuning.wallRunDurationSeconds = 0.5F;
  fastWindow.gameplayMovement.tuning.wallRunGravityMultiplier = 0.8F;
  fastWindow.gameplayMovement.tuning.wallRunDurationSeconds = 1.0F;
  const bool slowEntered = enterWallRun(*slowSession, slowWindow);
  const bool fastEntered = enterWallRun(*fastSession, fastWindow);
  const float slowStartY = playerEntity(*slowSession)->transform.position.y;
  const float fastStartY = playerEntity(*fastSession)->transform.position.y;
  iggy3d::applyProductGameplayActions(*slowSession,
                                      manualMoveActions(1.0F, 0.0F),
                                      slowWindow,
                                      "unit/gameplay_controller_wall_run_gravity_slow",
                                      activeSurfaces(slowWindow));
  iggy3d::applyProductGameplayActions(*fastSession,
                                      manualMoveActions(1.0F, 0.0F),
                                      fastWindow,
                                      "unit/gameplay_controller_wall_run_gravity_fast",
                                      activeSurfaces(fastWindow));
  const float slowFall =
      slowStartY - playerEntity(*slowSession)->transform.position.y;
  const float fastFall =
      fastStartY - playerEntity(*fastSession)->transform.position.y;

  return expect(slowEntered && fastEntered, "wall-run tuning enters") &&
         expect(slowFall < fastFall, "wall-run gravity tuning changes fall") &&
         expect(fastWindow.gameplayWallRun.durationSeconds >
                    slowWindow.gameplayWallRun.durationSeconds,
                "wall-run duration tuning recorded");
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
  return expect(window.gameplayJump.requested, "clamber jump requested") &&
         expect(!window.gameplayJump.accepted, "clamber skips jump arc") &&
         expect(!window.gameplayJump.active, "clamber leaves jump inactive") &&
         expect(window.gameplayJump.status == "traversal",
                "clamber jump status") &&
         expect(window.gameplayJump.reasonCode == "traversal_intent_applied",
                "clamber jump reason") &&
         expect(window.gameplayTraversal.requested, "clamber requested") &&
         expect(window.gameplayTraversal.consumed, "clamber consumed input") &&
         expect(window.gameplayTraversal.accepted, "clamber accepted") &&
         expect(!window.gameplayTraversal.fallbackJumpAllowed,
                "clamber no jump fallback") &&
         expect(window.gameplayTraversal.status == "traversal_intent_applied",
                "clamber traversal status") &&
         expect(window.gameplayTraversal.reasonCode == "traversal_intent_applied",
                "clamber traversal reason") &&
         expect(window.gameplayTraversal.mechanic == "clamber",
                "clamber mechanic proof") &&
         expect(window.gameplayTraversal.slotId == "clamber_block:clamber_top",
                "clamber slot proof") &&
         expect(window.gameplayTraversal.targetId == "clamber_block",
                "clamber target proof") &&
         expect(window.gameplayTraversal.landingSurfaceId == "clamber_top",
                "clamber landing proof") &&
         expect(window.gameplay.playerPositionChanged, "clamber changed player position") &&
         expect(nearlyEqual(window.gameplayTraversal.startX, start.x),
                "clamber start x") &&
         expect(nearlyEqual(window.gameplayTraversal.startY, start.y),
                "clamber start y") &&
         expect(nearlyEqual(window.gameplayTraversal.startZ, start.z),
                "clamber start z") &&
         expect(nearlyEqual(window.gameplayTraversal.finalX, final.x),
                "clamber final x") &&
         expect(nearlyEqual(window.gameplayTraversal.finalY, final.y),
                "clamber final y") &&
         expect(nearlyEqual(window.gameplayTraversal.finalZ, final.z),
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
  return expect(window.gameplayJump.requested, "wall jump requested") &&
         expect(window.gameplayJump.accepted, "wall jump accepted") &&
         expect(window.gameplayJump.active, "wall jump leaves jump active") &&
         expect(window.gameplayJump.status == "wall_jump",
                "wall jump status") &&
         expect(window.gameplayJump.reasonCode == "gameplay_jump_wall_jump",
                "wall jump reason") &&
         expect(window.gameplayTraversal.requested, "wall jump traversal requested") &&
         expect(window.gameplayTraversal.consumed, "wall jump consumed input") &&
         expect(window.gameplayTraversal.accepted, "wall jump traversal accepted") &&
         expect(!window.gameplayTraversal.fallbackJumpAllowed,
                "wall jump no fallback") &&
         expect(window.gameplayTraversal.mechanic == "wall_jump",
                "wall jump mechanic proof") &&
         expect(window.gameplayTraversal.slotId == "wall_jump_wall_actor_blocker",
                "wall jump slot proof") &&
         expect(window.gameplayTraversal.targetId == "wall_jump_wall",
                "wall jump target proof") &&
         expect(window.gameplayTraversal.landingSurfaceId ==
                    "wall_jump_wall_actor_blocker",
                "wall jump landing proof") &&
         expect(window.gameplay.playerPositionChanged, "wall jump changed player position") &&
         expect(nearlyEqual(window.gameplayTraversal.startX, start.x),
                "wall jump start x") &&
         expect(nearlyEqual(window.gameplayTraversal.startY, start.y),
                "wall jump start y") &&
         expect(nearlyEqual(window.gameplayTraversal.startZ, start.z),
                "wall jump start z") &&
         expect(nearlyEqual(window.gameplayTraversal.finalX, final.x),
                "wall jump final x") &&
         expect(nearlyEqual(window.gameplayTraversal.finalY, final.y),
                "wall jump final y") &&
         expect(nearlyEqual(window.gameplayTraversal.finalZ, final.z),
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
  window.gameplayJump.active = true;
  window.gameplayJump.velocityMetersPerSecond = 1.0F;
  const iggy3d::Vec3 start = playerEntity(*session)->transform.position;

  iggy3d::applyProductGameplayActions(*session,
                                      jumpActions(),
                                      window,
                                      "unit/gameplay_controller_generic_wall_jump");

  const iggy3d::Vec3 final = playerEntity(*session)->transform.position;
  return expect(window.gameplayJump.requested, "generic wall jump requested") &&
         expect(!window.gameplayJump.accepted, "generic wall jump rejected") &&
         expect(window.gameplayJump.status == "already_airborne",
                "generic wall jump status") &&
         expect(window.gameplayJump.reasonCode ==
                    "gameplay_jump_already_airborne",
                "generic wall jump reason") &&
         expect(!window.gameplayTraversal.accepted,
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

  return expect(window.gameplayJump.requested, "double jump requested") &&
         expect(!window.gameplayJump.accepted, "double jump rejected") &&
         expect(window.gameplayJump.active, "double jump still airborne") &&
         expect(window.gameplayJump.status == "already_airborne",
                "double jump status") &&
         expect(window.gameplayJump.reasonCode ==
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
  bool observedAirborneHeight = window.gameplayJump.heightMeters > 0.0F;
  const int landingTickBudget = static_cast<int>(std::ceil(
                                    2.0F *
                                    window.gameplayMovement.tuning
                                        .jumpImpulseMetersPerSecond /
                                    window.gameplayMovement.tuning
                                        .gravityMetersPerSecondSquared /
                                    window.gameplayMovement.tuning
                                        .inputStepSeconds)) +
                                10;
  for (int tick = 0; tick < landingTickBudget; ++tick) {
    iggy3d::applyProductGameplayActions(*session,
                                        noActions(),
                                        window,
                                        "unit/gameplay_controller_jump_tick");
    observedAirborneHeight =
        observedAirborneHeight || window.gameplayJump.heightMeters > 0.0F;
  }
  const float finalY = playerEntity(*session)->transform.position.y;

  return expect(observedAirborneHeight, "landing observed airborne height") &&
         expect(!window.gameplayJump.active, "jump no longer active") &&
         expect(window.gameplayJump.status == "landed", "jump landed status") &&
         expect(window.gameplayJump.reasonCode == "gameplay_jump_landed",
                "jump landed reason") &&
         expect(nearlyEqual(window.gameplayJump.velocityMetersPerSecond, 0.0F),
                "landed velocity zero") &&
         expect(nearlyEqual(window.gameplayJump.groundY, groundY), "land ground") &&
         expect(nearlyEqual(finalY, groundY), "landed player y") &&
         expect(nearlyEqual(window.gameplayJump.finalY, groundY),
                "landed final proof y") &&
         expect(nearlyEqual(window.gameplayJump.heightMeters, 0.0F),
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
  window.gameplayJump.active = true;
  window.gameplayJump.velocityMetersPerSecond = -1.0F;
  window.gameplayJump.groundY = 0.0F;
  window.gameplayJump.startY = 5.2F;

  for (int tick = 0; tick < 50 && window.gameplayJump.active; ++tick) {
    iggy3d::applyProductGameplayActions(*session,
                                        noActions(),
                                        window,
                                        "unit/gameplay_controller_elevated_land",
                                        activeSurfaces(window));
  }

  const float finalY = playerEntity(*session)->transform.position.y;
  return expect(!window.gameplayJump.active, "elevated landing inactive") &&
         expect(window.gameplayJump.status == "landed", "elevated landing status") &&
         expect(nearlyEqual(finalY, 4.0F), "elevated landing y") &&
         expect(nearlyEqual(window.gameplayJump.groundY, 4.0F),
                "elevated landing ground y") &&
         expect(nearlyEqual(window.gameplayJump.finalY, 4.0F),
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
    if (!window.gameplayJump.active &&
        nearlyEqual(playerEntity(*session)->transform.position.y, 0.0F)) {
      break;
    }
  }

  const float finalY = playerEntity(*session)->transform.position.y;
  return expect(observedFall, "hole fall observed downward motion") &&
         expect(!window.gameplayJump.active, "hole fall inactive after landing") &&
         expect(window.gameplayJump.status == "landed", "hole fall landed status") &&
         expect(nearlyEqual(finalY, 0.0F), "hole fall lands on lower floor") &&
         expect(nearlyEqual(window.gameplayJump.groundY, 0.0F),
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
  return expect(window.gameplayMovement.attempted, "move off floor attempted") &&
         expect(window.gameplay.playerPositionChanged, "move off floor moved") &&
         expect(final.x > 1.35F, "move off floor leaves upper footprint") &&
         expect(window.gameplayJump.active,
                "move off floor starts falling immediately") &&
         expect(window.gameplayJump.status == "falling",
                "move off floor falling status") &&
         expect(window.gameplayJump.reasonCode == "gameplay_jump_falling",
                "move off floor falling reason") &&
         expect(window.gameplayMovement.reasonCode == "grounded_ledge_fall",
                "move off floor movement reason") &&
         expect(nearlyEqual(window.gameplayJump.groundY, 0.0F),
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
  return expect(window.gameplayReset.triggered, "reset zone triggered") &&
         expect(window.gameplayReset.status == "reset",
                "reset zone status") &&
         expect(window.gameplayReset.reasonCode == "gameplay_reset_zone",
                "reset zone reason") &&
         expect(window.gameplayReset.spawnAnchorId == "marker_player_spawn_r0_c0",
                "reset zone spawn anchor") &&
         expect(window.gameplayReset.sourceAnchorId == "marker_reset_zone_r0_c1",
                "reset zone source anchor") &&
         expect(!window.gameplayJump.active, "reset zone clears jump") &&
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
  return expect(window.gameplayReset.triggered, "fall reset triggered") &&
         expect(window.gameplayReset.status == "reset", "fall reset status") &&
         expect(window.gameplayReset.reasonCode == "gameplay_reset_fall_out",
                "fall reset reason") &&
         expect(window.gameplayReset.spawnAnchorId == "marker_player_spawn_r0_c0",
                "fall reset spawn anchor") &&
         expect(window.gameplayReset.sourceAnchorId == "none",
                "fall reset source none") &&
         expect(nearlyEqual(window.gameplayReset.startY, -7.0F),
                "fall reset start y") &&
         expect(nearlyEqual(window.gameplayReset.finalY, 0.05F),
                "fall reset final proof y") &&
         expect(!window.gameplayJump.active, "fall reset clears jump") &&
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

  return expect(window.gameplayDash.requested, "dash requested") &&
         expect(window.gameplayDash.accepted, "dash accepted") &&
         expect(window.gameplayDash.status == "accepted", "dash status") &&
         expect(window.gameplayDash.reasonCode == "gameplay_dash_accepted",
                "dash reason") &&
         expect(window.gameplayMovement.profile == kExpectedManualFirstPersonDashProfile,
                "dash movement profile") &&
         expect(nearlyEqual(window.gameplayDash.distanceMeters,
                            kExpectedManualFirstPersonDashDistanceMeters),
                "dash distance proof") &&
         expect(nearlyEqual(window.gameplayMovement.horizontalDistanceMeters,
                            appliedDistance),
                "dash movement distance matches applied movement") &&
         expect(window.gameplayMovement.horizontalDistanceMeters <=
                    kExpectedManualFirstPersonDashDistanceMeters,
                "dash movement distance within requested dash") &&
         expect(window.gameplayDash.cooldownRemainingSeconds > 0.0F,
                "dash cooldown set") &&
         expect(nearlyEqual(window.gameplayDash.directionX, 0.0F),
                "dash direction x") &&
         expect(nearlyEqual(window.gameplayDash.directionZ, -1.0F),
                "dash direction z") &&
         expect(nearlyEqual(final.x - start.x, 0.0F), "dash x unchanged");
}

bool productDashUsesRuntimeTunedWindowDistance() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "runtime tuned dash session created")) {
    return false;
  }

  window.gameplayMovement.tuning.dashSpeedMetersPerSecond = 4.0F;
  window.gameplayMovement.tuning.dashDurationSeconds = 0.25F;
  iggy3d::applyProductGameplayActions(*session,
                                      dashActions(),
                                      window,
                                      "unit/gameplay_controller_runtime_dash");

  return expect(window.gameplayDash.accepted, "runtime tuned dash accepted") &&
         expect(nearlyEqual(window.gameplayDash.speedMetersPerSecond, 4.0F),
                "runtime tuned dash speed") &&
         expect(nearlyEqual(window.gameplayDash.distanceMeters, 1.0F),
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

  return expect(window.gameplayDash.accepted, "dash right accepted") &&
         expect(nearlyEqual(window.gameplayDash.directionX, 1.0F),
                "dash right direction x") &&
         expect(nearlyEqual(window.gameplayDash.directionZ, 0.0F),
                "dash right direction z") &&
         expect(nearlyEqual(window.gameplayMovement.horizontalDistanceMeters,
                            appliedDistance),
                "dash right distance matches applied movement") &&
         expect(window.gameplayMovement.horizontalDistanceMeters <=
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

  return expect(window.gameplayDash.requested, "cooldown dash requested") &&
         expect(!window.gameplayDash.accepted, "cooldown dash rejected") &&
         expect(window.gameplayDash.status == "cooldown", "cooldown dash status") &&
         expect(window.gameplayDash.reasonCode == "gameplay_dash_cooldown",
                "cooldown dash reason") &&
         expect(window.gameplayDash.cooldownRemainingSeconds > 0.0F,
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

  return expect(window.gameplayCommand.accepted, "legacy surfaces move accepted") &&
         expect(window.gameplayMovement.status == "moved",
                "legacy surfaces movement status") &&
         expect(window.gameplayCollision.surfacesUsed,
                "legacy surfaces collision surfaces used") &&
         expect(!window.physicsMovementPlanner.enabled,
                "legacy surfaces physics planner disabled") &&
         expect(!window.physicsMovementPlanner.requested,
                "legacy surfaces physics planner not requested") &&
         expect(!window.physicsMovementPlanner.used,
                "legacy surfaces physics planner not used") &&
         expect(window.physicsMovementPlanner.reasonCode ==
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
  window.physicsMovementPlanner.enabled = true;

  iggy3d::applyProductGameplayActions(*session, forwardMoveActions(), window,
                                      "unit/gameplay_controller_physics_surfaces",
                                      surfaces);

  return expect(window.gameplayCommand.accepted, "physics move accepted") &&
         expect(window.physicsMovementPlanner.enabled,
                "physics planner enabled") &&
         expect(window.physicsMovementPlanner.requested,
                "physics planner requested") &&
         expect(window.physicsMovementPlanner.used,
                "physics planner used") &&
         expect(window.physicsMovementPlanner.status ==
                    "physics_movement_planner_used",
                "physics planner status used") &&
         expect(session->state().transient.lastMovementResultAvailable,
                "physics movement result available") &&
         expect(session->state()
                    .transient.lastMovementResult.physicsFrameStatsAvailable,
                "physics movement stats available") &&
         expect(window.gameplayMovement.debugAvailable,
                "physics movement debug available") &&
         expect(window.gameplayMovement.collisionSweepCount >= 1U,
                "physics movement sweep count");
}

bool optInMoveWithoutCollisionSurfacesRecordsNoSurfaces() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "physics no surfaces session created")) {
    return false;
  }
  window.physicsMovementPlanner.enabled = true;

  iggy3d::applyProductGameplayActions(*session, forwardMoveActions(), window,
                                      "unit/gameplay_controller_physics_no_surfaces");

  return expect(window.gameplayCommand.accepted,
                "physics no surfaces move accepted") &&
         expect(window.physicsMovementPlanner.enabled,
                "physics no surfaces planner enabled") &&
         expect(window.physicsMovementPlanner.requested,
                "physics no surfaces planner requested") &&
         expect(!window.physicsMovementPlanner.used,
                "physics no surfaces planner not used") &&
         expect(window.physicsMovementPlanner.status ==
                    "physics_movement_planner_no_collision_surfaces",
                "physics no surfaces planner status") &&
         expect(!session->state()
                     .transient.lastMovementResult.physicsFrameStatsAvailable,
                "physics no surfaces has no physics stats");
}

}  // namespace

int main() {
  const bool ok = productMoveUsesTunedManualStep() &&
                  productMovementStateReportsIdleGroundedWithoutInput() &&
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
                  productJumpWithinCoyoteWindowSucceedsAfterLeavingGround() &&
                  productJumpOutsideCoyoteWindowRejectsAndBuffers() &&
                  productBufferedJumpFiresOnLanding() &&
                  productEarlyJumpReleaseCutsJumpHeight() &&
                  productFallGravityMultiplierDescendsFaster() &&
                  productMovementStateReportsBlockedOrSlidingFromCollisionProof() &&
                  productWallRunCandidateReportsAirborneSideWallContact() &&
                  productWallRunCandidateRejectsGroundedContact() &&
                  productWallRunCandidateRejectsLowSpeed() &&
                  productWallRunCandidateRejectsNoWallContact() &&
                  productWallRunMinSpeedTuningControlsCandidateThreshold() &&
                  productWallRunCandidateDoesNotChangeMovementOutput() &&
                  productWallRunCandidateEntersActiveState() &&
                  productWallRunReducesFallingAgainstNormalAirborneFall() &&
                  productWallRunMovesAlongWallTangentOnly() &&
                  productWallRunTimerExpiryExits() &&
                  productWallRunInputStopExits() &&
                  productWallRunJumpInputExits() &&
                  productWallRunTuningChangesGravityAndDuration() &&
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
