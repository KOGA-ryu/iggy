#include "content/PackageLoader.hpp"
#include "content/assets/RoomAsset.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/session/SessionRunner.hpp"

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

iggy3d::ScenarioEntitySeed playerSeed() {
  iggy3d::ScenarioEntitySeed seed;
  seed.stableName = "player";
  seed.kind = iggy3d::EntityKind::Player;
  seed.transform = transformAt(0.0F, 0.0F, 0.0F);
  seed.localBounds = iggy3d::makeAabb3({-0.25F, 0.0F, -0.25F}, {0.25F, 1.8F, 0.25F});
  seed.active = true;
  seed.persistent = true;
  return seed;
}

iggy3d::FixtureScenarioSeed makeRunnerPhysicsSeed() {
  iggy3d::FixtureScenarioSeed seed;
  seed.scenarioId = "session_runner.physics_movement";
  seed.config = iggy3d::makeDefaultRuntimeConfig();
  seed.initialClockMode = iggy3d::ClockMode::Normal;
  seed.defaultRealtimeCamera = iggy3d::CameraMode::ThirdPerson;
  seed.defaultTacticalCamera = iggy3d::CameraMode::TacticalOverhead;
  seed.players.push_back({0, iggy3d::PlayerSlotKind::Local, "player"});
  seed.entities = {playerSeed()};
  iggy3d::ScenarioObjectiveSeed objective;
  objective.id = "session_runner_objective";
  objective.initialStatus = iggy3d::ObjectiveStatusSeed::Active;
  objective.condition = "None";
  objective.completeStatus = iggy3d::ObjectiveStatusSeed::Complete;
  seed.objectives.push_back(objective);
  return seed;
}

iggy3d::Session makeSession() {
  const iggy3d::PackageLoadResult package =
      iggy3d::loadPackage(iggy3d::PackageLoadRequest{"fixtures/demos/first_room/package.iggy3d.toml"});
  iggy3d::SessionCreateRequest create;
  create.packageId = package.manifest.packageId;
  create.config = package.scenario.config;
  create.seed = package.scenario;
  return iggy3d::Session::create(create).value;
}

iggy3d::Session makeRunnerPhysicsSession() {
  iggy3d::SessionCreateRequest create;
  create.config = iggy3d::makeDefaultRuntimeConfig();
  create.seed = makeRunnerPhysicsSeed();
  return iggy3d::Session::create(create).value;
}

iggy3d::CommandRecord submittedMove(iggy3d::Vec3 point) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Move;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.target.hasPoint = true;
  command.payload.target.point = point;
  return command;
}

iggy3d::CommandRecord submittedWait() {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Wait;
  command.source = iggy3d::CommandSource::LocalPlayer;
  return command;
}

iggy3d::CommandRecord submittedControl(iggy3d::CommandKind kind) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.kind = kind;
  command.source = iggy3d::CommandSource::LocalPlayer;
  return command;
}

iggy3d::RoomSpatialSurface tickFloorSurface() {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "tick_floor";
  surface.sourceStaticMeshId = "tick_floor";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Plane;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Walkable;
  surface.pointsMeters = {
      {-10.0F, 0.0F, -10.0F},
      {10.0F, 0.0F, -10.0F},
      {10.0F, 0.0F, 10.0F},
      {-10.0F, 0.0F, 10.0F},
  };
  surface.normal = {0.0F, 1.0F, 0.0F};
  surface.collisionMask = {"actor"};
  return surface;
}

iggy3d::RoomSpatialSurface tickWallSurface() {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "tick_wall";
  surface.sourceStaticMeshId = "tick_wall";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Box;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Blocker;
  surface.pointsMeters = {
      {-10.0F, 0.0F, -1.10F},
      {10.0F, 0.0F, -1.10F},
      {10.0F, 3.0F, -0.90F},
      {-10.0F, 3.0F, -0.90F},
  };
  surface.normal = {0.0F, 0.0F, 1.0F};
  surface.collisionMask = {"actor"};
  surface.blocksActor = true;
  return surface;
}

iggy3d::SpatialSurfaceSet tickCollisionSurfaces() {
  iggy3d::RoomAsset room;
  room.id = "tick_collision_room";
  room.spatialSurfaces = {tickFloorSurface(), tickWallSurface()};
  return iggy3d::buildSpatialSurfaceSet(room);
}

bool runUntilIdleExecutesQueuedMovement() {
  iggy3d::Session session = makeSession();
  const iggy3d::SessionCommandResult move = session.submitCommand(submittedMove({2.0F, 0.0F, 0.0F}));
  const iggy3d::SessionRunnerRunResult run =
      iggy3d::runSession(iggy3d::SessionRunnerRunRequest{&session, 4, true, true});
  const iggy3d::EntityState* player = session.state().world.findByStableName("player");
  return expect(move.command.admission == iggy3d::CommandAdmissionStatus::Accepted, "move accepted") &&
         expect(run.status == iggy3d::SessionRunnerStatus::Advanced, "runner advanced") &&
         expect(run.ticksAdvanced == 1U, "runner one tick") &&
         expect(session.state().transient.pendingExecutionSequences.empty(), "runner queue empty") &&
         expect(player != nullptr &&
                    iggy3d::nearlyEqual(player->transform.position, {2.0F, 0.0F, 0.0F}),
                "runner moved player");
}

bool runnerPhysicsMovePlannerDefaultsOffWithCollisionSurfaces() {
  iggy3d::Session session = makeRunnerPhysicsSession();
  const iggy3d::SpatialSurfaceSet surfaces = tickCollisionSurfaces();
  const iggy3d::SessionCommandResult move =
      session.submitCommand(submittedMove({0.0F, 0.0F, -2.0F}));
  const iggy3d::SessionRunnerRunResult run =
      iggy3d::runSession(iggy3d::SessionRunnerRunRequest{&session,
                                                          4,
                                                          true,
                                                          true,
                                                          &surfaces});

  const iggy3d::EntityState* player = session.state().world.findByStableName("player");
  const iggy3d::MovementResult& movement =
      session.state().transient.lastMovementResult;
  return expect(move.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                "runner default physics-off move accepted") &&
         expect(run.status == iggy3d::SessionRunnerStatus::Advanced,
                "runner default physics-off advanced") &&
         expect(session.state().transient.lastMovementResultAvailable,
                "runner default physics-off movement result") &&
         expect(movement.blocked == iggy3d::MovementBlockedReason::BlockedByCollision,
                "runner default physics-off legacy block") &&
         expect(movement.hitSurfaceId == "tick_wall",
                "runner default physics-off wall id") &&
         expect(movement.movementClamped, "runner default physics-off clamped") &&
         expect(player != nullptr &&
                    iggy3d::nearlyEqual(player->transform.position, {0.0F, 0.0F, 0.0F}),
                "runner default physics-off no mutation");
}

bool runnerPhysicsMovePlannerOptionPartiallyMovesAgainstWall() {
  iggy3d::Session session = makeRunnerPhysicsSession();
  const iggy3d::SpatialSurfaceSet surfaces = tickCollisionSurfaces();
  const iggy3d::SessionCommandResult move =
      session.submitCommand(submittedMove({0.0F, 0.0F, -2.0F}));
  const iggy3d::SessionRunnerRunResult run =
      iggy3d::runSession(iggy3d::SessionRunnerRunRequest{&session,
                                                          4,
                                                          true,
                                                          true,
                                                          &surfaces,
                                                          true});

  const iggy3d::EntityState* player = session.state().world.findByStableName("player");
  const iggy3d::MovementResult& movement =
      session.state().transient.lastMovementResult;
  const iggy3d::Vec3 finalPosition = player != nullptr ? player->transform.position
                                                       : iggy3d::Vec3{};
  return expect(move.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                "runner physics move accepted") &&
         expect(run.status == iggy3d::SessionRunnerStatus::Advanced,
                "runner physics advanced") &&
         expect(run.ticksAdvanced == 1U, "runner physics one tick") &&
         expect(session.state().transient.pendingExecutionSequences.empty(),
                "runner physics queue consumed") &&
         expect(session.state().transient.lastMovementResultAvailable,
                "runner physics movement result") &&
         expect(movement.blocked == iggy3d::MovementBlockedReason::None,
                "runner physics partial move succeeds") &&
         expect(movement.movementClamped, "runner physics clamped") &&
         expect(movement.hitSurfaceId == "tick_wall", "runner physics wall id") &&
         expect(movement.collisionSweepCount >= 1U, "runner physics sweep count") &&
         expect(finalPosition.z < -0.10F && finalPosition.z > -0.90F,
                "runner physics partial z before wall") &&
         expect(iggy3d::nearlyEqual(finalPosition, movement.finalPosition),
                "runner physics final position recorded");
}

bool pausedStepRequestUsesPhysicsMovePlannerOption() {
  iggy3d::Session session = makeRunnerPhysicsSession();
  const iggy3d::SpatialSurfaceSet surfaces = tickCollisionSurfaces();
  const iggy3d::SessionCommandResult move =
      session.submitCommand(submittedMove({0.0F, 0.0F, -2.0F}));
  session.mutableStateForOwnedSystems().clock.mode = iggy3d::ClockMode::Paused;
  session.mutableStateForOwnedSystems().clock.timeScale = 0.0F;

  const iggy3d::SessionRunnerRunResult stepped =
      iggy3d::stepPausedOnce(iggy3d::SessionRunnerStepRequest{&session,
                                                              &surfaces,
                                                              true});
  const iggy3d::EntityState* player = session.state().world.findByStableName("player");
  const iggy3d::MovementResult& movement =
      session.state().transient.lastMovementResult;
  const iggy3d::Vec3 finalPosition = player != nullptr ? player->transform.position
                                                       : iggy3d::Vec3{};
  return expect(move.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                "paused runner physics move accepted") &&
         expect(stepped.status == iggy3d::SessionRunnerStatus::Advanced,
                "paused runner physics step advanced") &&
         expect(stepped.ticksAdvanced == 1U, "paused runner physics one tick") &&
         expect(session.state().transient.lastMovementResultAvailable,
                "paused runner physics movement result") &&
         expect(movement.movementClamped, "paused runner physics clamped") &&
         expect(movement.hitSurfaceId == "tick_wall", "paused runner physics wall id") &&
         expect(finalPosition.z < -0.10F && finalPosition.z > -0.90F,
                "paused runner physics partial z") &&
         expect(session.state().clock.mode == iggy3d::ClockMode::Paused,
                "paused runner remains paused") &&
         expect(!session.state().clock.stepRequested,
                "paused runner request consumed");
}

bool sessionRunUntilIdleWithOptionsUsesPhysicsMovePlanner() {
  iggy3d::Session session = makeRunnerPhysicsSession();
  const iggy3d::SpatialSurfaceSet surfaces = tickCollisionSurfaces();
  const iggy3d::SessionCommandResult move =
      session.submitCommand(submittedMove({0.0F, 0.0F, -2.0F}));
  const iggy3d::StatusResult run =
      session.runUntilIdleWithOptions(4, iggy3d::SessionTickOptions{&surfaces, true});

  const iggy3d::EntityState* player = session.state().world.findByStableName("player");
  const iggy3d::MovementResult& movement =
      session.state().transient.lastMovementResult;
  const iggy3d::Vec3 finalPosition = player != nullptr ? player->transform.position
                                                       : iggy3d::Vec3{};
  return expect(move.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                "run until idle physics move accepted") &&
         expect(run.status == iggy3d::ResultStatus::Ok,
                "run until idle physics status ok") &&
         expect(session.state().transient.lastMovementResultAvailable,
                "run until idle physics movement result") &&
         expect(movement.blocked == iggy3d::MovementBlockedReason::None,
                "run until idle physics partial success") &&
         expect(movement.movementClamped, "run until idle physics clamped") &&
         expect(movement.hitSurfaceId == "tick_wall",
                "run until idle physics wall id") &&
         expect(finalPosition.z < -0.10F && finalPosition.z > -0.90F,
                "run until idle physics partial z");
}

bool pausedAutoRunAndStepBehavior() {
  iggy3d::Session session = makeSession();
  const iggy3d::SessionCommandResult enter =
      session.submitCommand(submittedControl(iggy3d::CommandKind::ToggleTacticalMode));
  const iggy3d::SessionCommandResult pause =
      session.submitCommand(submittedControl(iggy3d::CommandKind::Pause));
  const iggy3d::CommandTick pausedTick = session.state().clock.tickIndex;
  const iggy3d::SessionRunnerRunResult idle =
      iggy3d::runSession(iggy3d::SessionRunnerRunRequest{&session, 4, true, true});
  const bool idlePreservedTick = session.state().clock.tickIndex == pausedTick;
  const iggy3d::SessionRunnerRunResult stepped = iggy3d::stepPausedOnce(session);
  return expect(enter.command.admission == iggy3d::CommandAdmissionStatus::Accepted, "enter accepted") &&
         expect(pause.command.admission == iggy3d::CommandAdmissionStatus::Accepted, "pause accepted") &&
         expect(session.state().clock.mode == iggy3d::ClockMode::Paused, "clock paused") &&
         expect(idle.status == iggy3d::SessionRunnerStatus::Idle, "paused idle") &&
         expect(idle.ticksAdvanced == 0U, "paused no auto advance") &&
         expect(idlePreservedTick, "paused tick unchanged") &&
         expect(stepped.status == iggy3d::SessionRunnerStatus::Advanced, "step advanced") &&
         expect(stepped.ticksAdvanced == 1U, "step one tick") &&
         expect(session.state().clock.tickIndex == pausedTick + 1U, "step tick index") &&
         expect(session.state().clock.mode == iggy3d::ClockMode::Paused, "step remains paused") &&
         expect(!session.state().clock.stepRequested, "step request consumed");
}

bool maxTicksExceededAndStopWhenComplete() {
  iggy3d::Session session = makeSession();
  (void)session.submitCommand(submittedMove({2.0F, 0.0F, 0.0F}));
  const iggy3d::SessionRunnerRunResult exceeded =
      iggy3d::runSession(iggy3d::SessionRunnerRunRequest{&session, 0, true, true});

  iggy3d::Session complete = makeSession();
  iggy3d::SessionState& state = complete.mutableStateForOwnedSystems();
  state.lifecycle = iggy3d::SessionLifecycle::Complete;
  state.outcome = iggy3d::SessionOutcome::DemoComplete;
  const iggy3d::SessionRunnerRunResult stopped =
      iggy3d::runSession(iggy3d::SessionRunnerRunRequest{&complete, 4, true, true});
  return expect(exceeded.status == iggy3d::SessionRunnerStatus::MaxTicksExceeded,
                "max ticks exceeded") &&
         expect(stopped.status == iggy3d::SessionRunnerStatus::Complete, "stop complete");
}

bool immediateControlCommandExecutionAndPausedLegality() {
  iggy3d::Session session = makeSession();
  const iggy3d::SessionCommandResult enter =
      session.submitCommand(submittedControl(iggy3d::CommandKind::ToggleTacticalMode));
  const iggy3d::SessionCommandResult pause =
      session.submitCommand(submittedControl(iggy3d::CommandKind::Pause));
  const iggy3d::SessionCommandResult wait = session.submitCommand(submittedWait());
  const iggy3d::SessionCommandResult toggle =
      session.submitCommand(submittedControl(iggy3d::CommandKind::ToggleTacticalMode));
  const iggy3d::CommandTick tickBeforeStep = session.state().clock.tickIndex;
  const iggy3d::SessionCommandResult step =
      session.submitCommand(submittedControl(iggy3d::CommandKind::StepTacticalTick));
  const iggy3d::SessionCommandResult resume =
      session.submitCommand(submittedControl(iggy3d::CommandKind::Resume));

  return expect(enter.executedImmediately, "enter immediate") &&
         expect(session.state().camera.previousRealtimeMode == iggy3d::CameraMode::ThirdPerson,
                "previous realtime") &&
         expect(pause.executedImmediately, "pause immediate") &&
         expect(wait.command.admission == iggy3d::CommandAdmissionStatus::Rejected &&
                    wait.command.rejection == iggy3d::CommandRejectionReason::SessionPaused,
                "wait rejected paused") &&
         expect(toggle.command.admission == iggy3d::CommandAdmissionStatus::Rejected &&
                    toggle.command.rejection == iggy3d::CommandRejectionReason::SessionPaused,
                "toggle rejected paused") &&
         expect(step.command.admission == iggy3d::CommandAdmissionStatus::Accepted &&
                    step.executedImmediately,
                "step accepted immediate") &&
         expect(session.state().clock.tickIndex == tickBeforeStep + 1U, "step command tick") &&
         expect(resume.command.admission == iggy3d::CommandAdmissionStatus::Accepted &&
                    resume.executedImmediately,
                "resume accepted immediate") &&
         expect(session.state().clock.mode == iggy3d::ClockMode::Slow, "resume slow") &&
         expect(session.state().camera.activeMode == iggy3d::CameraMode::TacticalOverhead,
                "resume tactical camera");
}

}  // namespace

int main() {
  bool ok = true;
  ok = runUntilIdleExecutesQueuedMovement() && ok;
  ok = runnerPhysicsMovePlannerDefaultsOffWithCollisionSurfaces() && ok;
  ok = runnerPhysicsMovePlannerOptionPartiallyMovesAgainstWall() && ok;
  ok = pausedStepRequestUsesPhysicsMovePlannerOption() && ok;
  ok = sessionRunUntilIdleWithOptionsUsesPhysicsMovePlanner() && ok;
  ok = pausedAutoRunAndStepBehavior() && ok;
  ok = maxTicksExceededAndStopWhenComplete() && ok;
  ok = immediateControlCommandExecutionAndPausedLegality() && ok;
  return ok ? 0 : 1;
}
