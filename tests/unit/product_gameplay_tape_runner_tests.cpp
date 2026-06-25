#include "app/iggy3d/ProductAsciiRoomAuthoring.hpp"
#include "app/iggy3d/ProductGameplayTape.hpp"
#include "app/iggy3d/ProductGameplayTapeRunner.hpp"
#include "app/iggy3d/ProductPackageSessionSeed.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/session/Session.hpp"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>
#include <utility>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::ProductAsciiRoomAuthoringResult makeLoopRoom() {
  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceName = "unit/ascii_gameplay_loop.iggyroom.txt";
  request.roomId = "ascii_gameplay_loop";
  request.centerOnOrigin = false;
  request.emitAssetText = false;
  request.sourceText =
      "#######\n"
      "#PKs$E#\n"
      "#######\n";
  return iggy3d::buildProductAsciiRoomAuthoring(request);
}

iggy3d::PackageLoadResult makePackage(const iggy3d::RoomAsset& room) {
  iggy3d::PackageLoadResult package;
  package.status = iggy3d::PackageLoadStatus::Ok;
  package.manifest.packageId = "iggy3d.ascii_gameplay_loop";
  package.manifest.schemaVersion = 1;
  package.manifest.requiredRuntimeSchema = 1;
  package.manifest.scenarioPath = "inline_ascii_gameplay_loop";
  package.scenario.scenarioId = "ascii_gameplay_loop.runtime_loop";
  package.rooms.push_back(room);
  return package;
}

std::optional<iggy3d::Session> makeLoopSession() {
  const iggy3d::ProductAsciiRoomAuthoringResult room = makeLoopRoom();
  if (!room.ok) {
    return std::nullopt;
  }
  const iggy3d::ProductPackageSessionSeedResult seed =
      iggy3d::buildProductPackageSessionSeed(makePackage(room.roomAsset.room));
  if (!seed.ok) {
    return std::nullopt;
  }

  iggy3d::SessionCreateRequest create;
  create.packageId = "iggy3d.ascii_gameplay_loop";
  create.config = seed.seed.config;
  create.seed = seed.seed;
  iggy3d::Result<iggy3d::Session> session = iggy3d::Session::create(create);
  if (session.status != iggy3d::ResultStatus::Ok) {
    return std::nullopt;
  }
  return std::move(session.value);
}

std::string_view validLoopTape() {
  return "move marker_key_r1_c2\n"
         "expect_reject required_item_missing interact marker_secret_door_r1_c3\n"
         "interact marker_key_r1_c2\n"
         "interact marker_secret_door_r1_c3\n"
         "move marker_treasure_r1_c4\n"
         "expect_reject required_item_missing interact marker_exit_r1_c5\n"
         "interact marker_treasure_r1_c4\n"
         "interact marker_exit_r1_c5\n";
}

iggy3d::ProductAsciiRoomAuthoringResult makeBlockedWallRoom() {
  iggy3d::ProductAsciiRoomAuthoringRequest request;
  request.sourceName = "unit/ascii_blocked_wall.iggyroom.txt";
  request.roomId = "ascii_blocked_wall";
  request.centerOnOrigin = false;
  request.emitAssetText = false;
  request.sourceText =
      "#####\n"
      "#P#K#\n"
      "#####\n";
  return iggy3d::buildProductAsciiRoomAuthoring(request);
}

std::optional<iggy3d::Session> makeSessionFromRoom(const iggy3d::RoomAsset& room) {
  const iggy3d::ProductPackageSessionSeedResult seed =
      iggy3d::buildProductPackageSessionSeed(makePackage(room));
  if (!seed.ok) {
    return std::nullopt;
  }

  iggy3d::SessionCreateRequest create;
  create.packageId = "iggy3d.ascii_gameplay_loop";
  create.config = seed.seed.config;
  create.seed = seed.seed;
  iggy3d::Result<iggy3d::Session> session = iggy3d::Session::create(create);
  if (session.status != iggy3d::ResultStatus::Ok) {
    return std::nullopt;
  }
  return std::move(session.value);
}

bool tapeCompletesAsciiLoop() {
  std::optional<iggy3d::Session> session = makeLoopSession();
  const iggy3d::ProductGameplayTapeParseResult parsed =
      iggy3d::parseProductGameplayTape(validLoopTape());
  if (!expect(session.has_value(), "session exists") ||
      !expect(parsed.ok, "tape parsed")) {
    return false;
  }

  const iggy3d::ProductGameplayTapeRunResult run =
      iggy3d::runProductGameplayTape({&*session, &parsed.tape});
  return expect(run.ok, "run ok") &&
         expect(run.status == "gameplay_tape_completed", "run status") &&
         expect(run.stepCount == 8U, "step count") &&
         expect(run.executedStepCount == 6U, "executed count") &&
         expect(run.expectedRejectedStepCount == 2U, "expected rejected count") &&
         expect(run.failedStepIndex == 0U, "no failed step") &&
         expect(run.keyCollected, "key collected") &&
         expect(run.secretDoorOpened, "secret door opened") &&
         expect(run.treasureCollected, "treasure collected") &&
         expect(run.exitObjectiveComplete, "exit objective complete") &&
         expect(run.sessionOutcome == "Victory", "victory outcome") &&
         expect(run.loopComplete, "loop complete") &&
         expect(session->state().clock.tickIndex == 6U, "six accepted ticks");
}

bool tapeAcceptsExpectedMovementBlock() {
  const iggy3d::ProductAsciiRoomAuthoringResult room = makeBlockedWallRoom();
  if (!expect(room.ok, "blocked wall room authored")) {
    return false;
  }
  std::optional<iggy3d::Session> session =
      makeSessionFromRoom(room.roomAsset.room);
  const iggy3d::SpatialSurfaceSet surfaces =
      iggy3d::buildSpatialSurfaceSet(room.roomAsset.room);
  const iggy3d::ProductGameplayTapeParseResult parsed =
      iggy3d::parseProductGameplayTape(
          "expect_blocked blocked_by_collision move marker_key_r1_c3\n");
  if (!expect(session.has_value(), "blocked wall session exists") ||
      !expect(parsed.ok, "blocked wall tape parsed")) {
    return false;
  }

  const iggy3d::ProductGameplayTapeRunResult run =
      iggy3d::runProductGameplayTape({&*session, &parsed.tape, &surfaces});
  return expect(run.ok, "expected movement block ok") &&
         expect(run.status == "gameplay_tape_completed",
                "expected movement block status") &&
         expect(run.stepCount == 1U, "expected movement block step count") &&
         expect(run.executedStepCount == 0U,
                "expected movement block executed count") &&
         expect(run.expectedBlockedStepCount == 1U,
                "expected movement block count") &&
         expect(run.lastMovementBlock == "blocked_by_collision",
                "expected movement block name") &&
         expect(session->state().transient.lastMovementResult.hitSurfaceId ==
                    "wall_r1_c2_actor_blocker",
                "expected movement block surface") &&
         expect(iggy3d::nearlyEqual(
                    session->state().world.findByStableName("player")->transform.position,
                    {1.0F, 0.05F, 1.0F}),
                "expected movement block no mutation");
}

bool tapeStopsOnUnexpectedRejection() {
  std::optional<iggy3d::Session> session = makeLoopSession();
  const iggy3d::ProductGameplayTapeParseResult parsed =
      iggy3d::parseProductGameplayTape(
          "move marker_key_r1_c2\n"
          "interact marker_secret_door_r1_c3\n"
          "interact marker_key_r1_c2\n");
  if (!expect(session.has_value(), "reject session exists") ||
      !expect(parsed.ok, "reject tape parsed")) {
    return false;
  }

  const iggy3d::ProductGameplayTapeRunResult run =
      iggy3d::runProductGameplayTape({&*session, &parsed.tape});
  return expect(!run.ok, "unexpected rejection fails") &&
         expect(run.status == "gameplay_tape_command_rejected",
                "unexpected rejection status") &&
         expect(run.failedStepIndex == 2U, "unexpected rejection step") &&
         expect(run.failedRejection == "required_item_missing",
                "unexpected rejection reason") &&
         expect(!run.keyCollected, "key not collected after stop") &&
         expect(run.sessionOutcome == "None", "no outcome after stop");
}

bool tapeReportsMissingTarget() {
  std::optional<iggy3d::Session> session = makeLoopSession();
  const iggy3d::ProductGameplayTapeParseResult parsed =
      iggy3d::parseProductGameplayTape("interact missing_marker\n");
  if (!expect(session.has_value(), "missing target session exists") ||
      !expect(parsed.ok, "missing target tape parsed")) {
    return false;
  }

  const iggy3d::ProductGameplayTapeRunResult run =
      iggy3d::runProductGameplayTape({&*session, &parsed.tape});
  return expect(!run.ok, "missing target fails") &&
         expect(run.status == "gameplay_tape_target_missing",
                "missing target status") &&
         expect(run.failedTarget == "missing_marker", "missing target name");
}

}  // namespace

int main() {
  const bool ok = tapeCompletesAsciiLoop() &&
                  tapeAcceptsExpectedMovementBlock() &&
                  tapeStopsOnUnexpectedRejection() &&
                  tapeReportsMissingTarget();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
