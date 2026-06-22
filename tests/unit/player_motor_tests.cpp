#include "runtime/player/PlayerMotor.hpp"

#include <initializer_list>
#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool approx(float lhs, float rhs, float epsilon = 0.001F) {
  return lhs > rhs - epsilon && lhs < rhs + epsilon;
}

iggy3d::Transform3 transformAt(float x, float y, float z) {
  iggy3d::Transform3 transform = iggy3d::identityTransform3();
  transform.position = {x, y, z};
  return transform;
}

iggy3d::WorldState makeWorldAt(iggy3d::Vec3 position, bool active = true) {
  iggy3d::WorldState world;
  iggy3d::EntityState player;
  player.id = {1};
  player.stableName = "player";
  player.kind = iggy3d::EntityKind::Player;
  player.transform = transformAt(position.x, position.y, position.z);
  player.localBounds = iggy3d::makeAabb3({-0.25F, 0.0F, -0.25F}, {0.25F, 1.8F, 0.25F});
  player.active = active;
  (void)world.seedEntity(player);
  return world;
}

iggy3d::RoomSpatialSurface floorSurface() {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "floor";
  surface.sourceStaticMeshId = "synthetic_floor";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Plane;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Walkable;
  surface.pointsMeters = {
      {-10.0F, 0.0F, -10.0F},
      {10.0F, 0.0F, -10.0F},
      {10.0F, 0.0F, 10.0F},
      {-10.0F, 0.0F, 10.0F},
  };
  surface.normal = {0.0F, 1.0F, 0.0F};
  surface.traversalTags = {"walkable"};
  surface.collisionMask = {"actor"};
  return surface;
}

iggy3d::RoomSpatialSurface wallSurface(std::string_view id = "wall") {
  iggy3d::RoomSpatialSurface surface;
  surface.id = std::string(id);
  surface.sourceStaticMeshId = "synthetic_wall";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Box;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Blocker;
  surface.pointsMeters = {
      {-10.0F, 0.0F, -0.10F},
      {10.0F, 0.0F, -0.10F},
      {10.0F, 3.0F, 0.10F},
      {-10.0F, 3.0F, 0.10F},
  };
  surface.normal = {0.0F, 0.0F, 1.0F};
  surface.traversalTags = {"blocker"};
  surface.collisionMask = {"actor"};
  surface.blocksActor = true;
  return surface;
}

iggy3d::SpatialSurfaceSet makeSurfaceSet(
    std::initializer_list<iggy3d::RoomSpatialSurface> surfaces = {floorSurface()}) {
  iggy3d::RoomAsset room;
  room.id = "synthetic_room";
  room.spatialSurfaces.assign(surfaces.begin(), surfaces.end());
  return iggy3d::buildSpatialSurfaceSet(room);
}

iggy3d::PlayerMotorState motorState() {
  iggy3d::PlayerMotorState state;
  state.actor = {1};
  return state;
}

bool groundedMotorReportsTerrainPolicy() {
  iggy3d::WorldState world = makeWorldAt({0.0F, 0.0F, 0.0F});
  const iggy3d::SpatialSurfaceSet surfaces = makeSurfaceSet();
  iggy3d::PlayerMotorContext context{&world, &surfaces};
  iggy3d::PlayerMotorState state = motorState();
  iggy3d::PlayerMotorInput input;
  input.seconds = 0.0F;

  const iggy3d::PlayerMotorResult result = iggy3d::updatePlayerMotor(context, state, input);
  return expect(iggy3d::playerMotorSucceeded(result), "terrain result ok") &&
         expect(result.groundSampleValid, "terrain sample valid") &&
         expect(result.groundContact, "terrain contact") &&
         expect(result.groundWalkable, "terrain walkable") &&
         expect(result.movementPolicyBand == "flat", "terrain flat band") &&
         expect(result.groundSurfaceId == "floor", "terrain ground surface") &&
         expect(result.hitSurfaceId.empty(), "terrain no hit surface") &&
         expect(iggy3d::nearlyEqual(result.groundNormal, {0.0F, 1.0F, 0.0F}),
                "terrain normal up") &&
         expect(approx(result.groundDistanceMeters, 0.0F), "terrain distance zero") &&
         expect(approx(result.slopeAngleDegrees, 0.0F), "terrain slope angle") &&
         expect(approx(result.slopeUpDot, 1.0F), "terrain up dot") &&
         expect(approx(result.speedMultiplier, 1.0F), "terrain speed multiplier") &&
         expect(approx(result.staminaCostMultiplier, 1.0F),
                "terrain stamina multiplier") &&
         expect(approx(result.stepPenaltyMultiplier, 1.0F),
                "terrain step multiplier") &&
         expect(!result.carefulFooting, "terrain no careful footing");
}

bool jumpImpulseLeavesGround() {
  iggy3d::WorldState world = makeWorldAt({0.0F, 0.0F, 0.0F});
  const iggy3d::SpatialSurfaceSet surfaces = makeSurfaceSet();
  iggy3d::PlayerMotorContext context{&world, &surfaces};
  iggy3d::PlayerMotorState state = motorState();
  iggy3d::PlayerMotorInput input;
  input.jumpPressed = true;
  input.seconds = 0.10F;

  const iggy3d::PlayerMotorResult result = iggy3d::updatePlayerMotor(context, state, input);
  const iggy3d::EntityState* player = world.findById({1});
  return expect(iggy3d::playerMotorSucceeded(result), "jump result ok") &&
         expect(result.jumpRequested, "jump requested") &&
         expect(result.jumpAccepted, "jump accepted") &&
         expect(result.phase == iggy3d::PlayerMotorPhase::Airborne, "jump airborne") &&
         expect(!result.grounded, "jump not grounded") &&
         expect(result.verticalVelocityMetersPerSecond > 0.0F, "jump velocity positive") &&
         expect(player != nullptr && player->transform.position.y > 0.0F, "player rose");
}

bool doubleJumpRejectedWhileAirborne() {
  iggy3d::WorldState world = makeWorldAt({0.0F, 0.0F, 0.0F});
  const iggy3d::SpatialSurfaceSet surfaces = makeSurfaceSet();
  iggy3d::PlayerMotorContext context{&world, &surfaces};
  iggy3d::PlayerMotorState state = motorState();
  iggy3d::PlayerMotorInput input;
  input.jumpPressed = true;
  input.seconds = 0.10F;
  (void)iggy3d::updatePlayerMotor(context, state, input);

  const iggy3d::PlayerMotorResult second = iggy3d::updatePlayerMotor(context, state, input);
  return expect(iggy3d::playerMotorSucceeded(second), "second result ok") &&
         expect(second.jumpRequested, "second requested") &&
         expect(!second.jumpAccepted, "second rejected") &&
         expect(second.phase == iggy3d::PlayerMotorPhase::Airborne, "still airborne");
}

bool airControlMovesHorizontallyWhileAirborne() {
  iggy3d::WorldState world = makeWorldAt({0.0F, 0.0F, 0.0F});
  const iggy3d::SpatialSurfaceSet surfaces = makeSurfaceSet();
  iggy3d::PlayerMotorContext context{&world, &surfaces};
  iggy3d::PlayerMotorState state = motorState();
  iggy3d::PlayerMotorInput input;
  input.moveIntent = {1.0F, 0.0F, 0.0F};
  input.jumpPressed = true;
  input.seconds = 0.10F;

  const iggy3d::PlayerMotorResult jump = iggy3d::updatePlayerMotor(context, state, input);
  const iggy3d::EntityState* player = world.findById({1});
  return expect(iggy3d::playerMotorSucceeded(jump), "air control result ok") &&
         expect(jump.jumpAccepted, "air control jump accepted") &&
         expect(jump.airMoveIntent, "air intent observed") &&
         expect(jump.airControlActive, "air control active") &&
         expect(jump.horizontalSpeedMetersPerSecond > 0.0F, "horizontal speed positive") &&
         expect(jump.horizontalVelocityMetersPerSecond.x > 0.0F,
                "horizontal velocity positive x") &&
         expect(player != nullptr && player->transform.position.x > 0.0F,
                "player air strafed");
}

bool airControlClampsAgainstActorBlocker() {
  iggy3d::WorldState world = makeWorldAt({0.0F, 0.0F, 0.50F});
  const iggy3d::SpatialSurfaceSet surfaces = makeSurfaceSet({floorSurface(), wallSurface()});
  iggy3d::PlayerMotorContext context{&world, &surfaces};
  iggy3d::PlayerMotorState state = motorState();
  iggy3d::PlayerMotorParams params;
  params.airMaxSpeedMetersPerSecond = 8.0F;
  params.airAccelerationMetersPerSecondSquared = 24.0F;
  params.airLaunchSpeedMetersPerSecond = 8.0F;
  iggy3d::PlayerMotorInput input;
  input.moveIntent = {0.0F, 0.0F, -1.0F};
  input.jumpPressed = true;
  input.seconds = 0.20F;

  const iggy3d::PlayerMotorResult jump = iggy3d::updatePlayerMotor(context, state, input, params);
  const iggy3d::EntityState* player = world.findById({1});
  return expect(iggy3d::playerMotorSucceeded(jump), "air wall result ok") &&
         expect(jump.airMoveIntent, "air wall intent observed") &&
         expect(jump.airControlActive, "air wall control active") &&
         expect(jump.airMovementClamped, "air wall clamped") &&
         expect(jump.hitSurfaceId == "wall", "air wall hit id") &&
         expect(player != nullptr && player->transform.position.z > 0.10F,
                "player stayed before wall");
}

bool dashMovesHorizontallyOnGround() {
  iggy3d::WorldState world = makeWorldAt({0.0F, 0.0F, 0.0F});
  const iggy3d::SpatialSurfaceSet surfaces = makeSurfaceSet();
  iggy3d::PlayerMotorContext context{&world, &surfaces};
  iggy3d::PlayerMotorState state = motorState();
  iggy3d::PlayerMotorInput input;
  input.moveIntent = {1.0F, 0.0F, 0.0F};
  input.dashPressed = true;
  input.seconds = 0.10F;

  const iggy3d::PlayerMotorResult dash = iggy3d::updatePlayerMotor(context, state, input);
  const iggy3d::EntityState* player = world.findById({1});
  return expect(iggy3d::playerMotorSucceeded(dash), "dash result ok") &&
         expect(dash.dashRequested, "dash requested") &&
         expect(dash.dashAccepted, "dash accepted") &&
         expect(dash.dashActive, "dash active") &&
         expect(dash.phase == iggy3d::PlayerMotorPhase::Grounded, "dash remains grounded") &&
         expect(dash.dashCooldownRemainingSeconds > 0.0F, "dash cooldown set") &&
         expect(player != nullptr && player->transform.position.x > 0.50F,
                "player dashed forward");
}

bool dashRequiresIntentAndRejectsCooldown() {
  bool ok = true;
  iggy3d::WorldState world = makeWorldAt({0.0F, 0.0F, 0.0F});
  const iggy3d::SpatialSurfaceSet surfaces = makeSurfaceSet();
  iggy3d::PlayerMotorContext context{&world, &surfaces};
  iggy3d::PlayerMotorState state = motorState();
  iggy3d::PlayerMotorInput input;
  input.dashPressed = true;
  input.seconds = 0.10F;

  const iggy3d::PlayerMotorResult noIntent = iggy3d::updatePlayerMotor(context, state, input);
  ok = ok && expect(iggy3d::playerMotorSucceeded(noIntent), "dash no intent result ok") &&
       expect(noIntent.dashRejectedNoIntent, "dash rejected no intent") &&
       expect(!noIntent.dashAccepted, "dash no intent not accepted");

  input.moveIntent = {1.0F, 0.0F, 0.0F};
  const iggy3d::PlayerMotorResult accepted = iggy3d::updatePlayerMotor(context, state, input);
  const iggy3d::PlayerMotorResult rejected = iggy3d::updatePlayerMotor(context, state, input);
  return ok && expect(accepted.dashAccepted, "dash first accepted") &&
         expect(rejected.dashRejectedCooldown, "dash cooldown rejected") &&
         expect(!rejected.dashAccepted, "dash cooldown not accepted");
}

bool dashClampsAgainstActorBlocker() {
  iggy3d::WorldState world = makeWorldAt({0.0F, 0.0F, 0.50F});
  const iggy3d::SpatialSurfaceSet surfaces = makeSurfaceSet({floorSurface(), wallSurface()});
  iggy3d::PlayerMotorContext context{&world, &surfaces};
  iggy3d::PlayerMotorState state = motorState();
  iggy3d::PlayerMotorParams params;
  params.dashSpeedMetersPerSecond = 12.0F;
  params.dashDurationSeconds = 0.20F;
  iggy3d::PlayerMotorInput input;
  input.moveIntent = {0.0F, 0.0F, -1.0F};
  input.dashPressed = true;
  input.seconds = 0.20F;

  const iggy3d::PlayerMotorResult dash = iggy3d::updatePlayerMotor(context, state, input, params);
  const iggy3d::EntityState* player = world.findById({1});
  return expect(iggy3d::playerMotorSucceeded(dash), "dash wall result ok") &&
         expect(dash.dashAccepted, "dash wall accepted") &&
         expect(dash.dashMovementClamped, "dash wall clamped") &&
         expect(dash.hitSurfaceId == "wall", "dash wall hit id") &&
         expect(player != nullptr && player->transform.position.z > 0.10F,
                "dash player stayed before wall");
}

bool gravityLandsAndRearmsJump() {
  iggy3d::WorldState world = makeWorldAt({0.0F, 0.0F, 0.0F});
  const iggy3d::SpatialSurfaceSet surfaces = makeSurfaceSet();
  iggy3d::PlayerMotorContext context{&world, &surfaces};
  iggy3d::PlayerMotorState state = motorState();
  iggy3d::PlayerMotorInput input;
  input.jumpPressed = true;
  input.seconds = 0.10F;
  (void)iggy3d::updatePlayerMotor(context, state, input);

  input.jumpPressed = false;
  bool landed = false;
  iggy3d::PlayerMotorResult last;
  for (int i = 0; i < 120; ++i) {
    last = iggy3d::updatePlayerMotor(context, state, input);
    landed = landed || last.landed;
    if (state.grounded) {
      break;
    }
  }

  const iggy3d::EntityState* player = world.findById({1});
  return expect(landed, "landed event") &&
         expect(last.phase == iggy3d::PlayerMotorPhase::Grounded, "landed grounded phase") &&
         expect(last.grounded, "landed grounded") && expect(state.jumpAvailable, "jump rearmed") &&
         expect(iggy3d::nearlyEqual(state.horizontalVelocityMetersPerSecond, {0.0F, 0.0F, 0.0F}),
                "landed horizontal velocity reset") &&
         expect(player != nullptr && iggy3d::nearlyEqual(player->transform.position,
                                                        {0.0F, 0.0F, 0.0F}),
                "landed on floor");
}

bool missingAndInvalidInputsDoNotMutate() {
  bool ok = true;
  iggy3d::WorldState world = makeWorldAt({0.0F, 0.0F, 0.0F});
  const iggy3d::SpatialSurfaceSet surfaces = makeSurfaceSet();
  iggy3d::PlayerMotorState state = motorState();
  iggy3d::PlayerMotorInput input;
  input.seconds = 0.10F;

  iggy3d::PlayerMotorContext missingWorld{nullptr, &surfaces};
  const iggy3d::PlayerMotorResult missing =
      iggy3d::updatePlayerMotor(missingWorld, state, input);
  ok = ok && expect(missing.status == iggy3d::PlayerMotorStatus::MissingWorld,
                    "missing world");

  iggy3d::PlayerMotorContext missingSurfaces{&world, nullptr};
  const iggy3d::PlayerMotorResult noSurfaces =
      iggy3d::updatePlayerMotor(missingSurfaces, state, input);
  ok = ok && expect(noSurfaces.status == iggy3d::PlayerMotorStatus::MissingCollisionSurfaces,
                    "missing surfaces");

  iggy3d::PlayerMotorContext context{&world, &surfaces};
  iggy3d::PlayerMotorParams badParams;
  badParams.gravityMetersPerSecondSquared = -1.0F;
  const iggy3d::PlayerMotorResult bad =
      iggy3d::updatePlayerMotor(context, state, input, badParams);
  ok = ok && expect(bad.status == iggy3d::PlayerMotorStatus::InvalidParameters,
                    "bad params");

  return ok && expect(iggy3d::nearlyEqual(world.findById({1})->transform.position,
                                          {0.0F, 0.0F, 0.0F}),
                      "invalid no mutation");
}

}  // namespace

int main() {
  const bool ok = groundedMotorReportsTerrainPolicy() && jumpImpulseLeavesGround() &&
                  doubleJumpRejectedWhileAirborne() &&
                  airControlMovesHorizontallyWhileAirborne() &&
                  airControlClampsAgainstActorBlocker() &&
                  dashMovesHorizontallyOnGround() &&
                  dashRequiresIntentAndRejectsCooldown() &&
                  dashClampsAgainstActorBlocker() &&
                  gravityLandsAndRearmsJump() && missingAndInvalidInputsDoNotMutate();
  return ok ? 0 : 1;
}
