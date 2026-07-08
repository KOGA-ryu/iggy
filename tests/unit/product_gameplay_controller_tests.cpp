#include "app/iggy3d/gameplay/Controller.hpp"

#include "ProductAsciiRoomWindowTestSupport.hpp"
#include "ProductTestSupport.hpp"

#include <cmath>
#include <cstdlib>
#include <optional>
#include <string_view>

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

using iggy3d::test::expect;
using iggy3d::test::nearlyEqual;

float horizontalDistance(iggy3d::Vec3 lhs, iggy3d::Vec3 rhs) {
  const float deltaX = rhs.x - lhs.x;
  const float deltaZ = rhs.z - lhs.z;
  return std::sqrt(deltaX * deltaX + deltaZ * deltaZ);
}

iggy3d::ProductAppWindowState makeGameplayWindow(
    std::optional<iggy3d::Session>& session) {
  return iggy3d::test::activateAsciiRoomWindowForTest(
      session,
      {
          "#######\n"
          "#.....#\n"
          "#..P..#\n"
          "#.....#\n"
          "#..$.E#\n"
          "#######\n",
          "gameplay_controller_step_room",
          "unit/gameplay_controller_step_room.iggyroom.txt",
          "ascii room activation ok",
      });
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
  return expect(window.gameplay.gameplayCommand.accepted, "manual move accepted") &&
         expect(window.gameplay.gameplayMovement.status == "moved",
                "manual movement status");
}

bool productMoveUsesTunedManualStep() {
  iggy3d::Vec3 delta;
  iggy3d::ProductAppWindowState window;
  if (!runManualMove(0.0F, 1.0F, 0.0F, delta, &window)) {
    return false;
  }

  return expect(window.gameplay.gameplayCommand.accepted, "move accepted") &&
         expect(window.gameplay.gameplayMovement.status == "moved", "movement status") &&
         expect(!window.gameplay.physicsMovementPlanner.enabled,
                "default physics planner disabled") &&
         expect(!window.gameplay.physicsMovementPlanner.requested,
                "default physics planner not requested") &&
         expect(!window.gameplay.physicsMovementPlanner.used,
                "default physics planner not used") &&
         expect(window.gameplay.physicsMovementPlanner.status ==
                    "physics_movement_planner_disabled",
                "default physics planner status") &&
         expect(window.gameplay.gameplayMovement.profile == kExpectedManualFirstPersonProfile,
                "manual movement profile") &&
         expect(window.gameplay.gameplayMovement.state ==
                    iggy3d::ProductGameplayMovementState::MovingGrounded,
                "manual movement state") &&
         expect(window.gameplay.gameplayMovement.grounded, "manual movement grounded") &&
         expect(window.gameplay.gameplayMovement.horizontalSpeedMetersPerSecond > 0.0F,
                "manual movement horizontal speed proof") &&
         expect(nearlyEqual(window.gameplay.gameplayMovement.maxSpeedMetersPerSecond,
                            kExpectedManualFirstPersonSpeedMetersPerSecond),
                "manual movement speed") &&
         expect(nearlyEqual(window.gameplay.gameplayMovement.horizontalDistanceMeters,
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

  return expect(window.gameplay.gameplayMovement.state ==
                    iggy3d::ProductGameplayMovementState::IdleGrounded,
                "idle grounded state") &&
         expect(window.gameplay.gameplayMovement.grounded, "idle grounded proof") &&
         expect(nearlyEqual(window.gameplay.gameplayMovement.horizontalSpeedMetersPerSecond,
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

  return expect(window.gameplay.gameplayCommand.accepted, "diagonal move accepted") &&
         expect(window.gameplay.gameplayMovement.status == "moved",
                "diagonal movement status") &&
         expect(window.gameplay.gameplayMovement.profile == kExpectedManualFirstPersonProfile,
                "diagonal movement profile") &&
         expect(nearlyEqual(window.gameplay.gameplayMovement.horizontalDistanceMeters,
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

  return expect(window.gameplay.gameplayCommand.accepted, "sprint move accepted") &&
         expect(window.gameplay.gameplayMovement.status == "moved",
                "sprint movement status") &&
         expect(window.gameplay.gameplayMovement.profile ==
                    kExpectedManualFirstPersonSprintProfile,
                "sprint movement profile") &&
         expect(nearlyEqual(window.gameplay.gameplayMovement.maxSpeedMetersPerSecond,
                            kExpectedManualFirstPersonSprintSpeedMetersPerSecond),
                "sprint movement speed") &&
         expect(nearlyEqual(window.gameplay.gameplayMovement.horizontalDistanceMeters,
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

  window.gameplay.gameplayMovement.tuning.walkSpeedMetersPerSecond = 1.2F;
  const float expectedStep =
      window.gameplay.gameplayMovement.tuning.walkSpeedMetersPerSecond *
      window.gameplay.gameplayMovement.tuning.inputStepSeconds;
  const iggy3d::Vec3 start = playerEntity(*session)->transform.position;
  iggy3d::applyProductGameplayActions(*session,
                                      manualMoveActions(0.0F, 1.0F),
                                      window,
                                      "unit/gameplay_controller_runtime_tuning");
  const iggy3d::Vec3 final = playerEntity(*session)->transform.position;

  return expect(window.gameplay.gameplayCommand.accepted, "runtime tuned move accepted") &&
         expect(nearlyEqual(window.gameplay.gameplayMovement.maxSpeedMetersPerSecond, 1.2F),
                "runtime tuned speed proof") &&
         expect(nearlyEqual(window.gameplay.gameplayMovement.horizontalDistanceMeters,
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

  window.gameplay.gameplayMovement.tuning.groundAccelerationMetersPerSecondSquared = 33.0F;
  const float expectedStep =
      window.gameplay.gameplayMovement.tuning.groundAccelerationMetersPerSecondSquared *
      window.gameplay.gameplayMovement.tuning.inputStepSeconds *
      window.gameplay.gameplayMovement.tuning.inputStepSeconds;
  const iggy3d::Vec3 start = playerEntity(*session)->transform.position;
  iggy3d::applyProductGameplayActions(*session,
                                      manualMoveActions(0.0F, 1.0F),
                                      window,
                                      "unit/gameplay_controller_accel_tuning");
  const iggy3d::Vec3 final = playerEntity(*session)->transform.position;

  return expect(window.gameplay.gameplayCommand.accepted,
                "runtime acceleration tuned move accepted") &&
         expect(nearlyEqual(window.gameplay.gameplayMovement.horizontalDistanceMeters,
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

  window.gameplay.gameplayMovement.tuning.groundAccelerationMetersPerSecondSquared = 33.0F;
  iggy3d::applyProductGameplayActions(*session,
                                      forwardMoveActions(),
                                      window,
                                      "unit/gameplay_controller_accel_ramp_first");
  const float firstDistance = window.gameplay.gameplayMovement.horizontalDistanceMeters;
  for (int frame = 0; frame < 8; ++frame) {
    iggy3d::applyProductGameplayActions(*session,
                                        forwardMoveActions(),
                                        window,
                                        "unit/gameplay_controller_accel_ramp_later");
  }
  const float laterDistance = window.gameplay.gameplayMovement.horizontalDistanceMeters;

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

  slowWindow.gameplay.gameplayMovement.tuning.groundAccelerationMetersPerSecondSquared =
      16.5F;
  fastWindow.gameplay.gameplayMovement.tuning.groundAccelerationMetersPerSecondSquared =
      66.0F;
  iggy3d::applyProductGameplayActions(*slowSession,
                                      forwardMoveActions(),
                                      slowWindow,
                                      "unit/gameplay_controller_accel_slow");
  iggy3d::applyProductGameplayActions(*fastSession,
                                      forwardMoveActions(),
                                      fastWindow,
                                      "unit/gameplay_controller_accel_fast");

  return expect(fastWindow.gameplay.gameplayMovement.horizontalDistanceMeters >
                    slowWindow.gameplay.gameplayMovement.horizontalDistanceMeters,
                "higher acceleration moves farther on first frame") &&
         expect(std::fabs(fastWindow.gameplay.gameplayMovement.groundVelocityZ) >
                    std::fabs(slowWindow.gameplay.gameplayMovement.groundVelocityZ),
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
  const float startingSpeed = std::fabs(window.gameplay.gameplayMovement.groundVelocityZ);
  window.gameplay.gameplayMovement.tuning.groundDecelerationMetersPerSecondSquared = 33.0F;
  iggy3d::ActionState noInput;
  iggy3d::applyProductGameplayActions(*session,
                                      noInput,
                                      window,
                                      "unit/gameplay_controller_decel_release");
  const float decayedSpeed = std::fabs(window.gameplay.gameplayMovement.groundVelocityZ);

  return expect(window.gameplay.gameplayCommand.accepted,
                "deceleration submits retained movement") &&
         expect(window.gameplay.gameplayMovement.horizontalDistanceMeters > 0.0F,
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
  slowWindow.gameplay.gameplayMovement.tuning.groundDecelerationMetersPerSecondSquared =
      16.5F;
  fastWindow.gameplay.gameplayMovement.tuning.groundDecelerationMetersPerSecondSquared =
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

  return expect(fastWindow.gameplay.gameplayMovement.horizontalDistanceMeters <
                    slowWindow.gameplay.gameplayMovement.horizontalDistanceMeters,
                "higher deceleration moves less after release") &&
         expect(std::fabs(fastWindow.gameplay.gameplayMovement.groundVelocityZ) <
                    std::fabs(slowWindow.gameplay.gameplayMovement.groundVelocityZ),
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

  return expect(window.gameplay.gameplayJump.requested, "jump requested") &&
         expect(window.gameplay.gameplayJump.accepted, "jump accepted") &&
         expect(window.gameplay.gameplayJump.active, "jump remains active after first step") &&
         expect(window.gameplay.gameplayMovement.state ==
                    iggy3d::ProductGameplayMovementState::Rising,
                "jump movement state rising") &&
         expect(!window.gameplay.gameplayMovement.grounded, "jump airborne proof") &&
         expect(window.gameplay.gameplayJump.status == "airborne", "jump airborne status") &&
         expect(window.gameplay.gameplayJump.reasonCode == "gameplay_jump_airborne",
                "jump airborne reason") &&
         expect(window.gameplay.playerPositionChanged, "jump changed player position") &&
         expect(final.y > start.y, "jump raises player y") &&
         expect(nearlyEqual(window.gameplay.gameplayJump.groundY, start.y), "jump ground y") &&
         expect(nearlyEqual(window.gameplay.gameplayJump.startY, start.y), "jump start y") &&
         expect(nearlyEqual(window.gameplay.gameplayJump.finalY, final.y), "jump final y") &&
         expect(window.gameplay.gameplayJump.heightMeters > 0.0F, "jump height positive") &&
         expect(window.gameplay.gameplayJump.velocityMetersPerSecond <
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

  return expect(window.gameplay.gameplayJump.requested, "jump forward jump requested") &&
         expect(window.gameplay.gameplayJump.accepted, "jump forward jump accepted") &&
         expect(window.gameplay.gameplayJump.active, "jump forward remains airborne") &&
         expect(window.gameplay.gameplayMovement.state ==
                    iggy3d::ProductGameplayMovementState::AirborneControl,
                "jump forward airborne control state") &&
         expect(!window.gameplay.gameplayMovement.grounded,
                "jump forward airborne proof") &&
         expect(window.gameplay.gameplayJump.status == "airborne",
                "jump forward jump airborne") &&
         expect(window.gameplay.gameplayMovement.attempted,
                "jump forward movement attempted") &&
         expect(window.gameplay.gameplayMovement.status == "moved",
                "jump forward movement status") &&
         expect(window.gameplay.gameplayMovement.debugAvailable,
                "jump forward movement debug") &&
         expect(window.gameplay.gameplayMovement.reasonCode == "airborne_manual_move",
                "jump forward movement reason") &&
         expect(window.gameplay.gameplayMovement.policyBand == "airborne",
                "jump forward movement policy") &&
         expect(!window.gameplay.gameplayCommand.submitted,
                "jump forward avoids grounded command") &&
         expect(nearlyEqual(window.gameplay.gameplayMovement.horizontalDistanceMeters,
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

  window.gameplay.gameplayMovement.tuning.airControlMultiplier = 0.25F;
  const float expectedStep =
      window.gameplay.gameplayMovement.tuning.walkSpeedMetersPerSecond *
      window.gameplay.gameplayMovement.tuning.inputStepSeconds *
      window.gameplay.gameplayMovement.tuning.airControlMultiplier;
  const iggy3d::Vec3 start = playerEntity(*session)->transform.position;
  iggy3d::applyProductGameplayActions(*session,
                                      jumpForwardActions(),
                                      window,
                                      "unit/gameplay_controller_air_control");
  const iggy3d::Vec3 final = playerEntity(*session)->transform.position;

  return expect(window.gameplay.gameplayJump.accepted,
                "air control tuned jump accepted") &&
         expect(window.gameplay.gameplayMovement.attempted,
                "air control tuned movement attempted") &&
         expect(nearlyEqual(window.gameplay.gameplayMovement.horizontalDistanceMeters,
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
  const bool falling = window.gameplay.gameplayJump.active &&
                       window.gameplay.gameplayJump.status == "falling";
  iggy3d::applyProductGameplayActions(*session,
                                      jumpActions(),
                                      window,
                                      "unit/gameplay_controller_coyote_jump",
                                      activeSurfaces(window));

  return expect(falling, "coyote setup starts falling") &&
         expect(window.gameplay.gameplayJump.accepted, "coyote jump accepted") &&
         expect(window.gameplay.gameplayJump.active, "coyote jump active") &&
         expect(window.gameplay.gameplayJump.coyoteSecondsRemaining == 0.0F,
                "coyote jump consumes timer") &&
         expect(window.gameplay.gameplayJump.velocityMetersPerSecond > 0.0F,
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

  return expect(!window.gameplay.gameplayJump.accepted,
                "expired coyote jump rejected") &&
         expect(window.gameplay.gameplayJump.status == "already_airborne",
                "expired coyote jump status") &&
         expect(window.gameplay.gameplayJump.bufferSecondsRemaining > 0.0F,
                "expired coyote jump is buffered");
}

bool productBufferedJumpFiresOnLanding() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "buffered jump session created")) {
    return false;
  }

  setPlayerPosition(*session, {0.0F, 0.01F, 0.0F});
  window.gameplay.gameplayJump.active = true;
  window.gameplay.gameplayJump.velocityMetersPerSecond = -1.0F;
  window.gameplay.gameplayJump.groundY = 0.0F;
  window.gameplay.gameplayJump.startY = 0.01F;
  iggy3d::applyProductGameplayActions(*session,
                                      jumpActions(),
                                      window,
                                      "unit/gameplay_controller_buffer_press");
  const bool buffered = window.gameplay.gameplayJump.bufferSecondsRemaining > 0.0F;
  iggy3d::applyProductGameplayActions(*session,
                                      noActions(),
                                      window,
                                      "unit/gameplay_controller_buffer_land");

  return expect(buffered, "jump press buffered before landing") &&
         expect(window.gameplay.gameplayJump.accepted, "buffered jump accepted on landing") &&
         expect(window.gameplay.gameplayJump.active, "buffered jump leaves player airborne") &&
         expect(window.gameplay.gameplayJump.bufferSecondsRemaining == 0.0F,
                "buffered jump consumes buffer") &&
         expect(window.gameplay.gameplayJump.velocityMetersPerSecond > 0.0F,
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

  return expect(cutWindow.gameplay.gameplayJump.cutApplied,
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
  normalWindow.gameplay.gameplayJump.active = true;
  normalWindow.gameplay.gameplayJump.velocityMetersPerSecond = 0.0F;
  normalWindow.gameplay.gameplayJump.groundY = 0.0F;
  normalWindow.gameplay.gameplayJump.startY = 3.0F;
  normalWindow.gameplay.gameplayMovement.tuning.fallGravityMultiplier = 1.0F;
  fastWindow.gameplay.gameplayJump.active = true;
  fastWindow.gameplay.gameplayJump.velocityMetersPerSecond = 0.0F;
  fastWindow.gameplay.gameplayJump.groundY = 0.0F;
  fastWindow.gameplay.gameplayJump.startY = 3.0F;
  fastWindow.gameplay.gameplayMovement.tuning.fallGravityMultiplier = 3.0F;

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
         expect(normalWindow.gameplay.gameplayMovement.state ==
                    iggy3d::ProductGameplayMovementState::Falling,
                "normal fall state") &&
         expect(fastWindow.gameplay.gameplayMovement.state ==
                    iggy3d::ProductGameplayMovementState::Falling,
                "fast fall state") &&
         expect(fastWindow.gameplay.gameplayJump.velocityMetersPerSecond <
                    normalWindow.gameplay.gameplayJump.velocityMetersPerSecond,
                "higher fall multiplier has lower velocity");
}

bool productMovementStateReportsBlockedOrSlidingFromCollisionProof() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "blocked state session created")) {
    return false;
  }

  window.gameplay.gameplayMovement.blocked = true;
  window.gameplay.gameplayMovement.clamped = true;
  window.gameplay.gameplayMovement.blockedReason = "blocked_by_collision";
  window.gameplay.gameplayMovement.hitSurfaceId = "unit_wall_actor_blocker";
  iggy3d::applyProductGameplayActions(*session,
                                      noActions(),
                                      window,
                                      "unit/gameplay_controller_blocked_state");

  return expect(window.gameplay.gameplayMovement.state ==
                    iggy3d::ProductGameplayMovementState::BlockedOrSliding,
                "blocked or sliding state") &&
         expect(window.gameplay.gameplayMovement.grounded,
                "blocked state remains grounded") &&
         expect(window.gameplay.gameplayMovement.hitSurfaceId == "unit_wall_actor_blocker",
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
  window.gameplay.gameplayJump.active = true;
  window.gameplay.gameplayJump.velocityMetersPerSecond = 1.0F;
  window.gameplay.gameplayJump.groundY = 0.0F;
  window.gameplay.gameplayJump.startY = 0.80F;
  iggy3d::applyProductGameplayActions(*session,
                                      manualMoveActions(1.0F, 0.0F),
                                      window,
                                      "unit/gameplay_controller_wall_run_candidate",
                                      activeSurfaces(window));

  return expect(window.gameplay.gameplayWallRun.candidateAvailable,
                "wall-run candidate available") &&
         expect(window.gameplay.gameplayWallRun.candidateStatus == "wall_run_candidate",
                "wall-run candidate status") &&
         expect(window.gameplay.gameplayWallRun.surfaceId ==
                    "wall_jump_wall_actor_blocker",
                "wall-run candidate surface") &&
         expect(window.gameplay.gameplayWallRun.approachSpeedMetersPerSecond >=
                    window.gameplay.gameplayMovement.tuning.wallRunMinSpeedMetersPerSecond,
                "wall-run candidate speed") &&
         expect(window.gameplay.gameplayWallRun.side != "none",
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

  return expect(!window.gameplay.gameplayWallRun.candidateAvailable,
                "grounded wall-run candidate rejected") &&
         expect(window.gameplay.gameplayWallRun.candidateReasonCode == "wall_run_grounded",
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
  window.gameplay.gameplayJump.active = true;
  window.gameplay.gameplayJump.velocityMetersPerSecond = 1.0F;
  window.gameplay.gameplayJump.groundY = 0.0F;
  window.gameplay.gameplayJump.startY = 0.80F;
  window.gameplay.gameplayMovement.tuning.wallRunMinSpeedMetersPerSecond = 10.0F;
  iggy3d::applyProductGameplayActions(*session,
                                      manualMoveActions(1.0F, 0.0F),
                                      window,
                                      "unit/gameplay_controller_wall_run_low_speed",
                                      activeSurfaces(window));

  return expect(!window.gameplay.gameplayWallRun.candidateAvailable,
                "low-speed wall-run candidate rejected") &&
         expect(window.gameplay.gameplayWallRun.candidateReasonCode == "wall_run_low_speed",
                "low-speed wall-run reason");
}

bool productWallRunCandidateRejectsNoWallContact() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "no-wall wall-run session created")) {
    return false;
  }

  window.gameplay.gameplayJump.active = true;
  window.gameplay.gameplayJump.velocityMetersPerSecond = 1.0F;
  window.gameplay.gameplayJump.groundY = 0.0F;
  window.gameplay.gameplayJump.startY = 0.80F;
  iggy3d::applyProductGameplayActions(*session,
                                      manualMoveActions(1.0F, 0.0F),
                                      window,
                                      "unit/gameplay_controller_wall_run_no_wall",
                                      activeSurfaces(window));

  return expect(!window.gameplay.gameplayWallRun.candidateAvailable,
                "no-wall wall-run candidate rejected") &&
         expect(window.gameplay.gameplayWallRun.candidateReasonCode ==
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
  slowWindow.gameplay.gameplayJump.active = true;
  slowWindow.gameplay.gameplayJump.velocityMetersPerSecond = 1.0F;
  slowWindow.gameplay.gameplayJump.groundY = 0.0F;
  slowWindow.gameplay.gameplayJump.startY = 0.80F;
  slowWindow.gameplay.gameplayMovement.tuning.wallRunMinSpeedMetersPerSecond = 10.0F;
  fastWindow.gameplay.gameplayJump.active = true;
  fastWindow.gameplay.gameplayJump.velocityMetersPerSecond = 1.0F;
  fastWindow.gameplay.gameplayJump.groundY = 0.0F;
  fastWindow.gameplay.gameplayJump.startY = 0.80F;
  fastWindow.gameplay.gameplayMovement.tuning.wallRunMinSpeedMetersPerSecond = 0.5F;

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

  return expect(!slowWindow.gameplay.gameplayWallRun.candidateAvailable,
                "high wall-run speed threshold rejects") &&
         expect(fastWindow.gameplay.gameplayWallRun.candidateAvailable,
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
  candidateWindow.gameplay.gameplayJump.active = true;
  candidateWindow.gameplay.gameplayJump.velocityMetersPerSecond = 1.0F;
  candidateWindow.gameplay.gameplayJump.groundY = 0.0F;
  candidateWindow.gameplay.gameplayJump.startY = 0.80F;
  candidateWindow.gameplay.gameplayMovement.tuning.wallRunMinSpeedMetersPerSecond = 0.5F;
  rejectedWindow.gameplay.gameplayJump.active = true;
  rejectedWindow.gameplay.gameplayJump.velocityMetersPerSecond = 1.0F;
  rejectedWindow.gameplay.gameplayJump.groundY = 0.0F;
  rejectedWindow.gameplay.gameplayJump.startY = 0.80F;
  rejectedWindow.gameplay.gameplayMovement.tuning.wallRunMinSpeedMetersPerSecond = 10.0F;

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
  return expect(candidateWindow.gameplay.gameplayWallRun.candidateAvailable,
                "candidate comparison enabled") &&
         expect(!rejectedWindow.gameplay.gameplayWallRun.candidateAvailable,
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
  window.gameplay.gameplayJump.active = true;
  window.gameplay.gameplayJump.velocityMetersPerSecond = -1.0F;
  window.gameplay.gameplayJump.groundY = 0.0F;
  window.gameplay.gameplayJump.startY = 0.80F;
  window.gameplay.gameplayMovement.tuning.wallRunMinSpeedMetersPerSecond = 0.5F;
}

bool enterWallRun(iggy3d::Session& session,
                  iggy3d::ProductAppWindowState& window) {
  iggy3d::applyProductGameplayActions(session,
                                      manualMoveActions(1.0F, 0.0F),
                                      window,
                                      "unit/gameplay_controller_wall_run_enter",
                                      activeSurfaces(window));
  return window.gameplay.gameplayWallRun.active;
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
         expect(window.gameplay.gameplayMovement.state ==
                    iggy3d::ProductGameplayMovementState::WallRunning,
                "wall-run movement state") &&
         expect(window.gameplay.gameplayWallRun.status == "wall_run_active",
                "wall-run active status") &&
         expect(window.gameplay.gameplayWallRun.reasonCode == "wall_run_started",
                "wall-run start reason") &&
         expect(window.gameplay.gameplayWallRun.remainingSeconds > 0.0F,
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
  normalWindow.gameplay.gameplayMovement.tuning.wallRunMinSpeedMetersPerSecond = 10.0F;
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
         expect(wallWindow.gameplay.gameplayJump.velocityMetersPerSecond >
                    normalWindow.gameplay.gameplayJump.velocityMetersPerSecond,
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
  window.gameplay.gameplayMovement.tuning.wallRunDurationSeconds = 0.1F;
  const bool entered = enterWallRun(*session, window);
  for (int frame = 0; frame < 10 && window.gameplay.gameplayWallRun.active; ++frame) {
    iggy3d::applyProductGameplayActions(
        *session,
        manualMoveActions(1.0F, 0.0F),
        window,
        "unit/gameplay_controller_wall_run_timer",
        activeSurfaces(window));
  }

  return expect(entered, "wall-run timer enters") &&
         expect(!window.gameplay.gameplayWallRun.active, "wall-run timer exits") &&
         expect(window.gameplay.gameplayWallRun.reasonCode == "wall_run_expired",
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
         expect(!window.gameplay.gameplayWallRun.active, "wall-run input stop exits") &&
         expect(window.gameplay.gameplayWallRun.reasonCode == "wall_run_input_stopped",
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
         expect(!window.gameplay.gameplayWallRun.active, "wall-run jump exits") &&
         expect(window.gameplay.gameplayWallRun.reasonCode == "wall_run_exit_jump",
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
  slowWindow.gameplay.gameplayMovement.tuning.wallRunGravityMultiplier = 0.1F;
  slowWindow.gameplay.gameplayMovement.tuning.wallRunDurationSeconds = 0.5F;
  fastWindow.gameplay.gameplayMovement.tuning.wallRunGravityMultiplier = 0.8F;
  fastWindow.gameplay.gameplayMovement.tuning.wallRunDurationSeconds = 1.0F;
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
         expect(fastWindow.gameplay.gameplayWallRun.durationSeconds >
                    slowWindow.gameplay.gameplayWallRun.durationSeconds,
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
  return expect(window.gameplay.gameplayJump.requested, "clamber jump requested") &&
         expect(!window.gameplay.gameplayJump.accepted, "clamber skips jump arc") &&
         expect(!window.gameplay.gameplayJump.active, "clamber leaves jump inactive") &&
         expect(window.gameplay.gameplayJump.status == "traversal",
                "clamber jump status") &&
         expect(window.gameplay.gameplayJump.reasonCode == "traversal_intent_applied",
                "clamber jump reason") &&
         expect(window.gameplay.gameplayTraversal.requested, "clamber requested") &&
         expect(window.gameplay.gameplayTraversal.consumed, "clamber consumed input") &&
         expect(window.gameplay.gameplayTraversal.accepted, "clamber accepted") &&
         expect(!window.gameplay.gameplayTraversal.fallbackJumpAllowed,
                "clamber no jump fallback") &&
         expect(window.gameplay.gameplayTraversal.status == "traversal_intent_applied",
                "clamber traversal status") &&
         expect(window.gameplay.gameplayTraversal.reasonCode == "traversal_intent_applied",
                "clamber traversal reason") &&
         expect(window.gameplay.gameplayTraversal.mechanic == "clamber",
                "clamber mechanic proof") &&
         expect(window.gameplay.gameplayTraversal.slotId == "clamber_block:clamber_top",
                "clamber slot proof") &&
         expect(window.gameplay.gameplayTraversal.targetId == "clamber_block",
                "clamber target proof") &&
         expect(window.gameplay.gameplayTraversal.landingSurfaceId == "clamber_top",
                "clamber landing proof") &&
         expect(window.gameplay.playerPositionChanged, "clamber changed player position") &&
         expect(nearlyEqual(window.gameplay.gameplayTraversal.startX, start.x),
                "clamber start x") &&
         expect(nearlyEqual(window.gameplay.gameplayTraversal.startY, start.y),
                "clamber start y") &&
         expect(nearlyEqual(window.gameplay.gameplayTraversal.startZ, start.z),
                "clamber start z") &&
         expect(nearlyEqual(window.gameplay.gameplayTraversal.finalX, final.x),
                "clamber final x") &&
         expect(nearlyEqual(window.gameplay.gameplayTraversal.finalY, final.y),
                "clamber final y") &&
         expect(nearlyEqual(window.gameplay.gameplayTraversal.finalZ, final.z),
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
  return expect(window.gameplay.gameplayJump.requested, "wall jump requested") &&
         expect(window.gameplay.gameplayJump.accepted, "wall jump accepted") &&
         expect(window.gameplay.gameplayJump.active, "wall jump leaves jump active") &&
         expect(window.gameplay.gameplayJump.status == "wall_jump",
                "wall jump status") &&
         expect(window.gameplay.gameplayJump.reasonCode == "gameplay_jump_wall_jump",
                "wall jump reason") &&
         expect(window.gameplay.gameplayTraversal.requested, "wall jump traversal requested") &&
         expect(window.gameplay.gameplayTraversal.consumed, "wall jump consumed input") &&
         expect(window.gameplay.gameplayTraversal.accepted, "wall jump traversal accepted") &&
         expect(!window.gameplay.gameplayTraversal.fallbackJumpAllowed,
                "wall jump no fallback") &&
         expect(window.gameplay.gameplayTraversal.mechanic == "wall_jump",
                "wall jump mechanic proof") &&
         expect(window.gameplay.gameplayTraversal.slotId == "wall_jump_wall_actor_blocker",
                "wall jump slot proof") &&
         expect(window.gameplay.gameplayTraversal.targetId == "wall_jump_wall",
                "wall jump target proof") &&
         expect(window.gameplay.gameplayTraversal.landingSurfaceId ==
                    "wall_jump_wall_actor_blocker",
                "wall jump landing proof") &&
         expect(window.gameplay.playerPositionChanged, "wall jump changed player position") &&
         expect(nearlyEqual(window.gameplay.gameplayTraversal.startX, start.x),
                "wall jump start x") &&
         expect(nearlyEqual(window.gameplay.gameplayTraversal.startY, start.y),
                "wall jump start y") &&
         expect(nearlyEqual(window.gameplay.gameplayTraversal.startZ, start.z),
                "wall jump start z") &&
         expect(nearlyEqual(window.gameplay.gameplayTraversal.finalX, final.x),
                "wall jump final x") &&
         expect(nearlyEqual(window.gameplay.gameplayTraversal.finalY, final.y),
                "wall jump final y") &&
         expect(nearlyEqual(window.gameplay.gameplayTraversal.finalZ, final.z),
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
  window.gameplay.gameplayJump.active = true;
  window.gameplay.gameplayJump.velocityMetersPerSecond = 1.0F;
  const iggy3d::Vec3 start = playerEntity(*session)->transform.position;

  iggy3d::applyProductGameplayActions(*session,
                                      jumpActions(),
                                      window,
                                      "unit/gameplay_controller_generic_wall_jump");

  const iggy3d::Vec3 final = playerEntity(*session)->transform.position;
  return expect(window.gameplay.gameplayJump.requested, "generic wall jump requested") &&
         expect(!window.gameplay.gameplayJump.accepted, "generic wall jump rejected") &&
         expect(window.gameplay.gameplayJump.status == "already_airborne",
                "generic wall jump status") &&
         expect(window.gameplay.gameplayJump.reasonCode ==
                    "gameplay_jump_already_airborne",
                "generic wall jump reason") &&
         expect(!window.gameplay.gameplayTraversal.accepted,
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

  return expect(window.gameplay.gameplayJump.requested, "double jump requested") &&
         expect(!window.gameplay.gameplayJump.accepted, "double jump rejected") &&
         expect(window.gameplay.gameplayJump.active, "double jump still airborne") &&
         expect(window.gameplay.gameplayJump.status == "already_airborne",
                "double jump status") &&
         expect(window.gameplay.gameplayJump.reasonCode ==
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
  bool observedAirborneHeight = window.gameplay.gameplayJump.heightMeters > 0.0F;
  const int landingTickBudget = static_cast<int>(std::ceil(
                                    2.0F *
                                    window.gameplay.gameplayMovement.tuning
                                        .jumpImpulseMetersPerSecond /
                                    window.gameplay.gameplayMovement.tuning
                                        .gravityMetersPerSecondSquared /
                                    window.gameplay.gameplayMovement.tuning
                                        .inputStepSeconds)) +
                                10;
  for (int tick = 0; tick < landingTickBudget; ++tick) {
    iggy3d::applyProductGameplayActions(*session,
                                        noActions(),
                                        window,
                                        "unit/gameplay_controller_jump_tick");
    observedAirborneHeight =
        observedAirborneHeight || window.gameplay.gameplayJump.heightMeters > 0.0F;
  }
  const float finalY = playerEntity(*session)->transform.position.y;

  return expect(observedAirborneHeight, "landing observed airborne height") &&
         expect(!window.gameplay.gameplayJump.active, "jump no longer active") &&
         expect(window.gameplay.gameplayJump.status == "landed", "jump landed status") &&
         expect(window.gameplay.gameplayJump.reasonCode == "gameplay_jump_landed",
                "jump landed reason") &&
         expect(nearlyEqual(window.gameplay.gameplayJump.velocityMetersPerSecond, 0.0F),
                "landed velocity zero") &&
         expect(nearlyEqual(window.gameplay.gameplayJump.groundY, groundY), "land ground") &&
         expect(nearlyEqual(finalY, groundY), "landed player y") &&
         expect(nearlyEqual(window.gameplay.gameplayJump.finalY, groundY),
                "landed final proof y") &&
         expect(nearlyEqual(window.gameplay.gameplayJump.heightMeters, 0.0F),
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
  window.gameplay.gameplayJump.active = true;
  window.gameplay.gameplayJump.velocityMetersPerSecond = -1.0F;
  window.gameplay.gameplayJump.groundY = 0.0F;
  window.gameplay.gameplayJump.startY = 5.2F;

  for (int tick = 0; tick < 50 && window.gameplay.gameplayJump.active; ++tick) {
    iggy3d::applyProductGameplayActions(*session,
                                        noActions(),
                                        window,
                                        "unit/gameplay_controller_elevated_land",
                                        activeSurfaces(window));
  }

  const float finalY = playerEntity(*session)->transform.position.y;
  return expect(!window.gameplay.gameplayJump.active, "elevated landing inactive") &&
         expect(window.gameplay.gameplayJump.status == "landed", "elevated landing status") &&
         expect(nearlyEqual(finalY, 4.0F), "elevated landing y") &&
         expect(nearlyEqual(window.gameplay.gameplayJump.groundY, 4.0F),
                "elevated landing ground y") &&
         expect(nearlyEqual(window.gameplay.gameplayJump.finalY, 4.0F),
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
    if (!window.gameplay.gameplayJump.active &&
        nearlyEqual(playerEntity(*session)->transform.position.y, 0.0F)) {
      break;
    }
  }

  const float finalY = playerEntity(*session)->transform.position.y;
  return expect(observedFall, "hole fall observed downward motion") &&
         expect(!window.gameplay.gameplayJump.active, "hole fall inactive after landing") &&
         expect(window.gameplay.gameplayJump.status == "landed", "hole fall landed status") &&
         expect(nearlyEqual(finalY, 0.0F), "hole fall lands on lower floor") &&
         expect(nearlyEqual(window.gameplay.gameplayJump.groundY, 0.0F),
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
  return expect(window.gameplay.gameplayMovement.attempted, "move off floor attempted") &&
         expect(window.gameplay.playerPositionChanged, "move off floor moved") &&
         expect(final.x > 1.35F, "move off floor leaves upper footprint") &&
         expect(window.gameplay.gameplayJump.active,
                "move off floor starts falling immediately") &&
         expect(window.gameplay.gameplayJump.status == "falling",
                "move off floor falling status") &&
         expect(window.gameplay.gameplayJump.reasonCode == "gameplay_jump_falling",
                "move off floor falling reason") &&
         expect(window.gameplay.gameplayMovement.reasonCode == "grounded_ledge_fall",
                "move off floor movement reason") &&
         expect(nearlyEqual(window.gameplay.gameplayJump.groundY, 0.0F),
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
  return expect(window.gameplay.gameplayReset.triggered, "reset zone triggered") &&
         expect(window.gameplay.gameplayReset.status == "reset",
                "reset zone status") &&
         expect(window.gameplay.gameplayReset.reasonCode == "gameplay_reset_zone",
                "reset zone reason") &&
         expect(window.gameplay.gameplayReset.spawnAnchorId == "marker_player_spawn_r0_c0",
                "reset zone spawn anchor") &&
         expect(window.gameplay.gameplayReset.sourceAnchorId == "marker_reset_zone_r0_c1",
                "reset zone source anchor") &&
         expect(!window.gameplay.gameplayJump.active, "reset zone clears jump") &&
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
  return expect(window.gameplay.gameplayReset.triggered, "fall reset triggered") &&
         expect(window.gameplay.gameplayReset.status == "reset", "fall reset status") &&
         expect(window.gameplay.gameplayReset.reasonCode == "gameplay_reset_fall_out",
                "fall reset reason") &&
         expect(window.gameplay.gameplayReset.spawnAnchorId == "marker_player_spawn_r0_c0",
                "fall reset spawn anchor") &&
         expect(window.gameplay.gameplayReset.sourceAnchorId == "none",
                "fall reset source none") &&
         expect(nearlyEqual(window.gameplay.gameplayReset.startY, -7.0F),
                "fall reset start y") &&
         expect(nearlyEqual(window.gameplay.gameplayReset.finalY, 0.05F),
                "fall reset final proof y") &&
         expect(!window.gameplay.gameplayJump.active, "fall reset clears jump") &&
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

  return expect(window.gameplay.gameplayDash.requested, "dash requested") &&
         expect(window.gameplay.gameplayDash.accepted, "dash accepted") &&
         expect(window.gameplay.gameplayDash.status == "accepted", "dash status") &&
         expect(window.gameplay.gameplayDash.reasonCode == "gameplay_dash_accepted",
                "dash reason") &&
         expect(window.gameplay.gameplayMovement.profile == kExpectedManualFirstPersonDashProfile,
                "dash movement profile") &&
         expect(nearlyEqual(window.gameplay.gameplayDash.distanceMeters,
                            kExpectedManualFirstPersonDashDistanceMeters),
                "dash distance proof") &&
         expect(nearlyEqual(window.gameplay.gameplayMovement.horizontalDistanceMeters,
                            appliedDistance),
                "dash movement distance matches applied movement") &&
         expect(window.gameplay.gameplayMovement.horizontalDistanceMeters <=
                    kExpectedManualFirstPersonDashDistanceMeters,
                "dash movement distance within requested dash") &&
         expect(window.gameplay.gameplayDash.cooldownRemainingSeconds > 0.0F,
                "dash cooldown set") &&
         expect(nearlyEqual(window.gameplay.gameplayDash.directionX, 0.0F),
                "dash direction x") &&
         expect(nearlyEqual(window.gameplay.gameplayDash.directionZ, -1.0F),
                "dash direction z") &&
         expect(nearlyEqual(final.x - start.x, 0.0F), "dash x unchanged");
}

bool productDashUsesRuntimeTunedWindowDistance() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "runtime tuned dash session created")) {
    return false;
  }

  window.gameplay.gameplayMovement.tuning.dashSpeedMetersPerSecond = 4.0F;
  window.gameplay.gameplayMovement.tuning.dashDurationSeconds = 0.25F;
  iggy3d::applyProductGameplayActions(*session,
                                      dashActions(),
                                      window,
                                      "unit/gameplay_controller_runtime_dash");

  return expect(window.gameplay.gameplayDash.accepted, "runtime tuned dash accepted") &&
         expect(nearlyEqual(window.gameplay.gameplayDash.speedMetersPerSecond, 4.0F),
                "runtime tuned dash speed") &&
         expect(nearlyEqual(window.gameplay.gameplayDash.distanceMeters, 1.0F),
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

  return expect(window.gameplay.gameplayDash.accepted, "dash right accepted") &&
         expect(nearlyEqual(window.gameplay.gameplayDash.directionX, 1.0F),
                "dash right direction x") &&
         expect(nearlyEqual(window.gameplay.gameplayDash.directionZ, 0.0F),
                "dash right direction z") &&
         expect(nearlyEqual(window.gameplay.gameplayMovement.horizontalDistanceMeters,
                            appliedDistance),
                "dash right distance matches applied movement") &&
         expect(window.gameplay.gameplayMovement.horizontalDistanceMeters <=
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

  return expect(window.gameplay.gameplayDash.requested, "cooldown dash requested") &&
         expect(!window.gameplay.gameplayDash.accepted, "cooldown dash rejected") &&
         expect(window.gameplay.gameplayDash.status == "cooldown", "cooldown dash status") &&
         expect(window.gameplay.gameplayDash.reasonCode == "gameplay_dash_cooldown",
                "cooldown dash reason") &&
         expect(window.gameplay.gameplayDash.cooldownRemainingSeconds > 0.0F,
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

  return expect(window.gameplay.gameplayCommand.accepted, "legacy surfaces move accepted") &&
         expect(window.gameplay.gameplayMovement.status == "moved",
                "legacy surfaces movement status") &&
         expect(window.gameplay.gameplayCollision.surfacesUsed,
                "legacy surfaces collision surfaces used") &&
         expect(!window.gameplay.physicsMovementPlanner.enabled,
                "legacy surfaces physics planner disabled") &&
         expect(!window.gameplay.physicsMovementPlanner.requested,
                "legacy surfaces physics planner not requested") &&
         expect(!window.gameplay.physicsMovementPlanner.used,
                "legacy surfaces physics planner not used") &&
         expect(window.gameplay.physicsMovementPlanner.reasonCode ==
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
  window.gameplay.physicsMovementPlanner.enabled = true;

  iggy3d::applyProductGameplayActions(*session, forwardMoveActions(), window,
                                      "unit/gameplay_controller_physics_surfaces",
                                      surfaces);

  return expect(window.gameplay.gameplayCommand.accepted, "physics move accepted") &&
         expect(window.gameplay.physicsMovementPlanner.enabled,
                "physics planner enabled") &&
         expect(window.gameplay.physicsMovementPlanner.requested,
                "physics planner requested") &&
         expect(window.gameplay.physicsMovementPlanner.used,
                "physics planner used") &&
         expect(window.gameplay.physicsMovementPlanner.status ==
                    "physics_movement_planner_used",
                "physics planner status used") &&
         expect(session->state().transient.lastMovementResultAvailable,
                "physics movement result available") &&
         expect(session->state()
                    .transient.lastMovementResult.physicsFrameStatsAvailable,
                "physics movement stats available") &&
         expect(window.gameplay.gameplayMovement.debugAvailable,
                "physics movement debug available") &&
         expect(window.gameplay.gameplayMovement.collisionSweepCount >= 1U,
                "physics movement sweep count");
}

bool optInMoveWithoutCollisionSurfacesRecordsNoSurfaces() {
  std::optional<iggy3d::Session> session;
  iggy3d::ProductAppWindowState window = makeGameplayWindow(session);
  if (!expect(session.has_value(), "physics no surfaces session created")) {
    return false;
  }
  window.gameplay.physicsMovementPlanner.enabled = true;

  iggy3d::applyProductGameplayActions(*session, forwardMoveActions(), window,
                                      "unit/gameplay_controller_physics_no_surfaces");

  return expect(window.gameplay.gameplayCommand.accepted,
                "physics no surfaces move accepted") &&
         expect(window.gameplay.physicsMovementPlanner.enabled,
                "physics no surfaces planner enabled") &&
         expect(window.gameplay.physicsMovementPlanner.requested,
                "physics no surfaces planner requested") &&
         expect(!window.gameplay.physicsMovementPlanner.used,
                "physics no surfaces planner not used") &&
         expect(window.gameplay.physicsMovementPlanner.status ==
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
