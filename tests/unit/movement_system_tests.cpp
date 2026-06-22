#include "runtime/movement/MovementSystem.hpp"

#include <iostream>
#include <initializer_list>
#include <limits>
#include <string>
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

iggy3d::RoomSpatialSurface floorSurface(std::string_view id = "floor",
                                        iggy3d::Vec3 normal = {0.0F, 1.0F, 0.0F}) {
  iggy3d::RoomSpatialSurface surface;
  surface.id = std::string(id);
  surface.sourceStaticMeshId = "synthetic_floor";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Plane;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Walkable;
  surface.pointsMeters = {
      {-10.0F, 0.0F, -10.0F},
      {10.0F, 0.0F, -10.0F},
      {10.0F, 0.0F, 10.0F},
      {-10.0F, 0.0F, 10.0F},
  };
  surface.normal = normal;
  surface.traversalTags = {"walkable"};
  surface.collisionMask = {"actor"};
  return surface;
}

iggy3d::RoomSpatialSurface slopeSurface(std::string_view id, float degrees) {
  const float radians = degrees * 3.14159265358979323846F / 180.0F;
  const float slope = std::tan(radians);
  iggy3d::RoomSpatialSurface surface = floorSurface(id, {0.0F, std::cos(radians), -std::sin(radians)});
  surface.pointsMeters = {
      {-10.0F, -10.0F * slope, -10.0F},
      {10.0F, -10.0F * slope, -10.0F},
      {10.0F, 10.0F * slope, 10.0F},
      {-10.0F, 10.0F * slope, 10.0F},
  };
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

iggy3d::RoomSpatialSurface openingSurface() {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "opening";
  surface.sourceStaticMeshId = "synthetic_opening";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Opening;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Opening;
  surface.pointsMeters = {
      {0.0F, 0.0F, -1.0F},
      {0.0F, 2.0F, -1.0F},
      {0.0F, 2.0F, 1.0F},
      {0.0F, 0.0F, 1.0F},
  };
  surface.normal = {1.0F, 0.0F, 0.0F};
  surface.traversalTags = {"opening"};
  return surface;
}

iggy3d::SpatialSurfaceSet makeSurfaceSet(std::initializer_list<iggy3d::RoomSpatialSurface> surfaces) {
  iggy3d::RoomAsset room;
  room.id = "synthetic_room";
  room.spatialSurfaces.assign(surfaces.begin(), surfaces.end());
  return iggy3d::buildSpatialSurfaceSet(room);
}

iggy3d::MovementRequest moveRequest(iggy3d::Vec3 destination,
                                    iggy3d::MovementMode mode = iggy3d::MovementMode::Walk) {
  iggy3d::MovementRequest request;
  request.actor = {1};
  request.destination = destination;
  request.mode = mode;
  request.maxDistanceMeters = 3.0F;
  request.sourceCommandId = 7;
  return request;
}

iggy3d::KinematicMovementRequest kinematicRequest(iggy3d::Vec3 intent, float seconds = 1.0F) {
  iggy3d::KinematicMovementRequest request;
  request.actor = {1};
  request.intent = intent;
  request.seconds = seconds;
  request.params.maxSpeedMetersPerSecond = 1.0F;
  request.params.groundSnapMeters = 0.75F;
  request.params.skinMeters = 0.02F;
  request.sourceCommandId = 11;
  return request;
}

iggy3d::CommandRecord acceptedMoveCommand() {
  iggy3d::CommandRecord command;
  command.commandId = 42;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Move;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.target.hasPoint = true;
  command.payload.target.point = {2.0F, 0.0F, 0.0F};
  command.admission = iggy3d::CommandAdmissionStatus::Accepted;
  return command;
}

bool acceptedMoveToKeyUpdatesPlayerPosition() {
  iggy3d::WorldState world = makeWorldAt({0.0F, 0.0F, 0.0F});
  iggy3d::RuntimeConfig config = iggy3d::makeDefaultRuntimeConfig();
  iggy3d::MovementSystemContext context{&world, &config};
  const iggy3d::MovementResult result =
      iggy3d::executeMovement(context, moveRequest({2.0F, 0.0F, 0.0F}));
  return expect(result.blocked == iggy3d::MovementBlockedReason::None, "move ok") &&
         expect(result.distanceMeters == 2.0F, "move distance") &&
         expect(iggy3d::nearlyEqual(result.finalPosition, {2.0F, 0.0F, 0.0F}), "final") &&
         expect(iggy3d::nearlyEqual(world.findById({1})->transform.position,
                                    {2.0F, 0.0F, 0.0F}),
                "world moved");
}

bool tacticalMoveUsesSameMutationPath() {
  iggy3d::WorldState world = makeWorldAt({2.0F, 0.0F, 0.0F});
  iggy3d::RuntimeConfig config = iggy3d::makeDefaultRuntimeConfig();
  iggy3d::MovementSystemContext context{&world, &config};
  const iggy3d::MovementResult result = iggy3d::executeMovement(
      context, moveRequest({2.0F, 0.0F, 1.0F}, iggy3d::MovementMode::Tactical));
  return expect(result.blocked == iggy3d::MovementBlockedReason::None, "tactical ok") &&
         expect(result.mode == iggy3d::MovementMode::Tactical, "tactical mode") &&
         expect(result.distanceMeters == 1.0F, "tactical distance") &&
         expect(iggy3d::nearlyEqual(world.findById({1})->transform.position,
                                    {2.0F, 0.0F, 1.0F}),
                "tactical moved");
}

bool acceptedCommandConversionPreservesPayload() {
  iggy3d::WorldState world = makeWorldAt({0.0F, 0.0F, 0.0F});
  const iggy3d::RuntimeConfig config = iggy3d::makeDefaultRuntimeConfig();
  const iggy3d::CommandRecord command = acceptedMoveCommand();
  const iggy3d::MovementRequest request =
      iggy3d::movementRequestFromAcceptedCommand(command, iggy3d::MovementMode::Walk, config);
  return expect(request.actor == iggy3d::EntityId{1}, "conversion actor") &&
         expect(request.sourceCommandId == 42U, "conversion command id") &&
         expect(request.maxDistanceMeters == 3.0F, "conversion limit") &&
         expect(iggy3d::nearlyEqual(request.destination, {2.0F, 0.0F, 0.0F}),
                "conversion point") &&
         expect(iggy3d::nearlyEqual(world.findById({1})->transform.position,
                                    {0.0F, 0.0F, 0.0F}),
                "conversion no mutation");
}

bool missingPointConversionBlocksAsNonfiniteDestination() {
  iggy3d::WorldState world = makeWorldAt({0.0F, 0.0F, 0.0F});
  const iggy3d::RuntimeConfig config = iggy3d::makeDefaultRuntimeConfig();
  iggy3d::CommandRecord command = acceptedMoveCommand();
  command.payload.target.hasPoint = false;
  const iggy3d::MovementRequest request =
      iggy3d::movementRequestFromAcceptedCommand(command, iggy3d::MovementMode::Walk, config);
  iggy3d::MovementSystemContext context{&world, &config};
  const iggy3d::MovementResult result = iggy3d::executeMovement(context, request);
  return expect(request.sourceCommandId == command.commandId, "missing point source preserved") &&
         expect(result.blocked == iggy3d::MovementBlockedReason::DestinationNotFinite,
                "missing point blocks") &&
         expect(iggy3d::nearlyEqual(world.findById({1})->transform.position,
                                    {0.0F, 0.0F, 0.0F}),
                "missing point no mutation");
}

bool blockedCasesDoNotMutateWorld() {
  bool ok = true;
  auto assertBlocked = [&ok](iggy3d::MovementRequest request,
                             iggy3d::MovementBlockedReason reason,
                             bool active = true) {
    iggy3d::WorldState world = makeWorldAt({0.0F, 0.0F, 0.0F}, active);
    const iggy3d::RuntimeConfig config = iggy3d::makeDefaultRuntimeConfig();
    iggy3d::MovementSystemContext context{&world, &config};
    const iggy3d::MovementResult result = iggy3d::executeMovement(context, request);
    ok = ok && expect(result.blocked == reason, "blocked reason") &&
         expect(iggy3d::nearlyEqual(world.findById({1})->transform.position,
                                    {0.0F, 0.0F, 0.0F}),
                "blocked no mutation");
  };

  iggy3d::MovementSystemContext missingContext;
  const iggy3d::MovementResult missingWorld =
      iggy3d::executeMovement(missingContext, moveRequest({1.0F, 0.0F, 0.0F}));
  ok = ok && expect(missingWorld.blocked == iggy3d::MovementBlockedReason::MissingWorld,
                    "missing world");

  iggy3d::MovementRequest invalidActor = moveRequest({1.0F, 0.0F, 0.0F});
  invalidActor.actor = {};
  assertBlocked(invalidActor, iggy3d::MovementBlockedReason::InvalidActor);

  assertBlocked(moveRequest({1.0F, 0.0F, 0.0F}), iggy3d::MovementBlockedReason::ActorInactive,
                false);

  iggy3d::MovementRequest nonfinite = moveRequest({1.0F, 0.0F, 0.0F});
  nonfinite.destination.x = std::numeric_limits<float>::infinity();
  assertBlocked(nonfinite, iggy3d::MovementBlockedReason::DestinationNotFinite);

  assertBlocked(moveRequest({4.0F, 0.0F, 0.0F}), iggy3d::MovementBlockedReason::MovementTooFar);
  return ok;
}

bool kinematicFlatMovementSnapsAndMutatesOnce() {
  iggy3d::WorldState world = makeWorldAt({0.0F, 0.40F, 0.0F});
  iggy3d::RuntimeConfig config = iggy3d::makeDefaultRuntimeConfig();
  const iggy3d::SpatialSurfaceSet surfaces = makeSurfaceSet({floorSurface()});
  iggy3d::MovementSystemContext context{&world, &config, &surfaces};
  const iggy3d::MovementResult result =
      iggy3d::executeKinematicMovement(context, kinematicRequest({1.0F, 0.0F, 0.0F}));
  return expect(result.blocked == iggy3d::MovementBlockedReason::None, "kinematic flat ok") &&
         expect(result.kinematic, "kinematic flag") &&
         expect(result.movementPolicyBand == "flat", "flat band") &&
         expect(result.groundSnapApplied, "ground snap applied") &&
         expect(result.slopeTravelDirection == "downhill", "flat snap direction") &&
         expect(result.horizontalDistanceMeters > 0.99F &&
                    result.horizontalDistanceMeters < 1.01F,
                "flat horizontal distance") &&
         expect(result.verticalDeltaMeters < -0.39F && result.verticalDeltaMeters > -0.41F,
                "flat snap vertical delta") &&
         expect(iggy3d::nearlyEqual(result.finalPosition, {1.0F, 0.0F, 0.0F}), "flat final") &&
         expect(iggy3d::nearlyEqual(world.findById({1})->transform.position,
                                    {1.0F, 0.0F, 0.0F}),
                "flat world moved");
}

bool kinematicWallClampPreventsCrossing() {
  iggy3d::WorldState world = makeWorldAt({0.0F, 0.0F, 1.0F});
  iggy3d::RuntimeConfig config = iggy3d::makeDefaultRuntimeConfig();
  const iggy3d::SpatialSurfaceSet surfaces = makeSurfaceSet({floorSurface(), wallSurface()});
  iggy3d::MovementSystemContext context{&world, &config, &surfaces};
  auto request = kinematicRequest({0.0F, 0.0F, -1.0F}, 2.0F);
  const iggy3d::MovementResult result = iggy3d::executeKinematicMovement(context, request);
  return expect(result.blocked == iggy3d::MovementBlockedReason::None, "wall clamp ok") &&
         expect(result.movementClamped, "movement clamped") &&
         expect(result.hitSurfaceId == "wall", "wall hit id") &&
         expect(result.collisionSweepCount >= 1U, "wall sweep count") &&
         expect(result.finalPosition.z > 0.09F, "wall not crossed");
}

bool kinematicAngledWallMovementSlides() {
  iggy3d::WorldState world = makeWorldAt({0.0F, 0.0F, 1.0F});
  iggy3d::RuntimeConfig config = iggy3d::makeDefaultRuntimeConfig();
  const iggy3d::SpatialSurfaceSet surfaces = makeSurfaceSet({floorSurface(), wallSurface()});
  iggy3d::MovementSystemContext context{&world, &config, &surfaces};
  const iggy3d::MovementResult result =
      iggy3d::executeKinematicMovement(context, kinematicRequest({1.0F, 0.0F, -1.0F}, 2.0F));
  return expect(result.blocked == iggy3d::MovementBlockedReason::None, "slide ok") &&
         expect(result.movementClamped, "slide clamped") &&
         expect(result.movementSlid, "movement slid") &&
         expect(result.finalPosition.x > 0.5F, "slide x advanced") &&
         expect(result.finalPosition.z > 0.09F, "slide wall not crossed");
}

bool kinematicOpeningDoesNotBlockActor() {
  iggy3d::WorldState world = makeWorldAt({-1.0F, 0.0F, 0.0F});
  iggy3d::RuntimeConfig config = iggy3d::makeDefaultRuntimeConfig();
  const iggy3d::SpatialSurfaceSet surfaces = makeSurfaceSet({floorSurface(), openingSurface()});
  iggy3d::MovementSystemContext context{&world, &config, &surfaces};
  const iggy3d::MovementResult result =
      iggy3d::executeKinematicMovement(context, kinematicRequest({1.0F, 0.0F, 0.0F}, 2.0F));
  return expect(result.blocked == iggy3d::MovementBlockedReason::None, "opening pass ok") &&
         expect(!result.movementClamped, "opening not clamped") &&
         expect(result.finalPosition.x > 0.9F, "opening crossed");
}

bool kinematicBlockedSlopeRejectsWithoutMutation() {
  iggy3d::WorldState world = makeWorldAt({0.0F, 0.0F, 0.0F});
  iggy3d::RuntimeConfig config = iggy3d::makeDefaultRuntimeConfig();
  const iggy3d::SpatialSurfaceSet surfaces = makeSurfaceSet({slopeSurface("blocked_slope", 45.0F)});
  iggy3d::MovementSystemContext context{&world, &config, &surfaces};
  const iggy3d::MovementResult result =
      iggy3d::executeKinematicMovement(context, kinematicRequest({1.0F, 0.0F, 0.0F}));
  return expect(result.blocked == iggy3d::MovementBlockedReason::SlopeRejected,
                "blocked slope reason") &&
         expect(result.movementPolicyBand == "blocked", "blocked slope band") &&
         expect(iggy3d::nearlyEqual(world.findById({1})->transform.position,
                                    {0.0F, 0.0F, 0.0F}),
                "blocked slope no mutation");
}

bool kinematicModerateSlopeAppliesSpeedMultiplier() {
  iggy3d::WorldState world = makeWorldAt({0.0F, 0.0F, 0.0F});
  iggy3d::RuntimeConfig config = iggy3d::makeDefaultRuntimeConfig();
  const iggy3d::SpatialSurfaceSet surfaces = makeSurfaceSet({slopeSurface("moderate_slope", 20.0F)});
  iggy3d::MovementSystemContext context{&world, &config, &surfaces};
  const iggy3d::MovementResult result =
      iggy3d::executeKinematicMovement(context, kinematicRequest({1.0F, 0.0F, 0.0F}));
  return expect(result.blocked == iggy3d::MovementBlockedReason::None, "moderate ok") &&
         expect(result.movementPolicyBand == "moderate", "moderate band") &&
         expect(result.carefulFooting, "careful footing") &&
         expect(result.speedMultiplier < 1.0F, "speed reduced") &&
         expect(result.slopeTravelDirection == "contour", "moderate contour direction") &&
         expect(result.horizontalDistanceMeters > 0.70F &&
                    result.horizontalDistanceMeters < 0.80F,
                "moderate horizontal distance") &&
         expect(result.finalPosition.x > 0.70F && result.finalPosition.x < 0.80F,
                "moderate distance reduced");
}

bool kinematicModerateSlopeReportsDirectionalGrade() {
  iggy3d::RuntimeConfig config = iggy3d::makeDefaultRuntimeConfig();
  const iggy3d::SpatialSurfaceSet surfaces = makeSurfaceSet({slopeSurface("moderate_slope", 20.0F)});

  iggy3d::WorldState uphillWorld = makeWorldAt({0.0F, 0.0F, 0.0F});
  iggy3d::MovementSystemContext uphillContext{&uphillWorld, &config, &surfaces};
  const iggy3d::MovementResult uphill =
      iggy3d::executeKinematicMovement(uphillContext, kinematicRequest({0.0F, 0.0F, 1.0F}));

  iggy3d::WorldState downhillWorld = makeWorldAt({0.0F, 0.0F, 0.0F});
  iggy3d::MovementSystemContext downhillContext{&downhillWorld, &config, &surfaces};
  const iggy3d::MovementResult downhill =
      iggy3d::executeKinematicMovement(downhillContext, kinematicRequest({0.0F, 0.0F, -1.0F}));

  return expect(uphill.blocked == iggy3d::MovementBlockedReason::None, "uphill ok") &&
         expect(uphill.slopeTravelDirection == "uphill", "uphill direction") &&
         expect(uphill.horizontalDistanceMeters > 0.60F, "uphill horizontal distance") &&
         expect(uphill.verticalDeltaMeters > 0.20F, "uphill vertical delta") &&
         expect(uphill.gradePercent > 30.0F, "uphill grade") &&
         expect(downhill.blocked == iggy3d::MovementBlockedReason::None, "downhill ok") &&
         expect(downhill.slopeTravelDirection == "downhill", "downhill direction") &&
         expect(downhill.horizontalDistanceMeters > 0.60F, "downhill horizontal distance") &&
         expect(downhill.verticalDeltaMeters < -0.20F, "downhill vertical delta") &&
         expect(downhill.gradePercent < -30.0F, "downhill grade");
}

bool kinematicMissingSurfacesAndInvalidParamsDoNotMutate() {
  bool ok = true;
  iggy3d::RuntimeConfig config = iggy3d::makeDefaultRuntimeConfig();
  {
    iggy3d::WorldState world = makeWorldAt({0.0F, 0.0F, 0.0F});
    iggy3d::MovementSystemContext context{&world, &config, nullptr};
    const iggy3d::MovementResult result =
        iggy3d::executeKinematicMovement(context, kinematicRequest({1.0F, 0.0F, 0.0F}));
    ok = ok && expect(result.blocked == iggy3d::MovementBlockedReason::MissingCollisionSurfaces,
                      "missing surfaces") &&
         expect(iggy3d::nearlyEqual(world.findById({1})->transform.position,
                                    {0.0F, 0.0F, 0.0F}),
                "missing surfaces no mutation");
  }
  {
    iggy3d::WorldState world = makeWorldAt({0.0F, 0.0F, 0.0F});
    const iggy3d::SpatialSurfaceSet surfaces = makeSurfaceSet({floorSurface()});
    iggy3d::MovementSystemContext context{&world, &config, &surfaces};
    auto request = kinematicRequest({1.0F, 0.0F, 0.0F});
    request.params.radiusMeters = -1.0F;
    const iggy3d::MovementResult result = iggy3d::executeKinematicMovement(context, request);
    ok = ok && expect(result.blocked == iggy3d::MovementBlockedReason::InvalidMovementParams,
                      "invalid params") &&
         expect(iggy3d::nearlyEqual(world.findById({1})->transform.position,
                                    {0.0F, 0.0F, 0.0F}),
                "invalid params no mutation");
  }
  return ok;
}

}  // namespace

int main() {
  const bool ok = acceptedMoveToKeyUpdatesPlayerPosition() && tacticalMoveUsesSameMutationPath() &&
                  acceptedCommandConversionPreservesPayload() &&
                  missingPointConversionBlocksAsNonfiniteDestination() &&
                  blockedCasesDoNotMutateWorld() && kinematicFlatMovementSnapsAndMutatesOnce() &&
                  kinematicWallClampPreventsCrossing() && kinematicAngledWallMovementSlides() &&
                  kinematicOpeningDoesNotBlockActor() &&
                  kinematicBlockedSlopeRejectsWithoutMutation() &&
                  kinematicModerateSlopeAppliesSpeedMultiplier() &&
                  kinematicModerateSlopeReportsDirectionalGrade() &&
                  kinematicMissingSurfacesAndInvalidParamsDoNotMutate();
  return ok ? 0 : 1;
}
