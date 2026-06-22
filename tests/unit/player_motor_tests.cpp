#include "runtime/player/PlayerMotor.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
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

iggy3d::SpatialSurfaceSet makeSurfaceSet() {
  iggy3d::RoomAsset room;
  room.id = "synthetic_room";
  room.spatialSurfaces.push_back(floorSurface());
  return iggy3d::buildSpatialSurfaceSet(room);
}

iggy3d::PlayerMotorState motorState() {
  iggy3d::PlayerMotorState state;
  state.actor = {1};
  return state;
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
  const bool ok = jumpImpulseLeavesGround() && doubleJumpRejectedWhileAirborne() &&
                  gravityLandsAndRearmsJump() && missingAndInvalidInputsDoNotMutate();
  return ok ? 0 : 1;
}
