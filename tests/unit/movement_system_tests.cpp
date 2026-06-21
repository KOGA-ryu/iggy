#include "runtime/movement/MovementSystem.hpp"

#include <iostream>
#include <limits>
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

}  // namespace

int main() {
  const bool ok = acceptedMoveToKeyUpdatesPlayerPosition() && tacticalMoveUsesSameMutationPath() &&
                  acceptedCommandConversionPreservesPayload() &&
                  missingPointConversionBlocksAsNonfiniteDestination() &&
                  blockedCasesDoNotMutateWorld();
  return ok ? 0 : 1;
}
