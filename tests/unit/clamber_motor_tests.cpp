// Clamber motor proof (flow feat v1). The stealth_blockout kit's clamber
// ladder is the fixture: synthetic box surfaces below replicate the kit
// pieces' exact dimensions (1x1 footprint blocks at 0.6 / 1.0 / 1.4 / 1.8,
// the 2.0 must-refuse block, the 0.25-riser stair_run step) so the same
// numbers are pinned in code that Ace walks against in the map demo.

#include "runtime/movement/ClamberMotor.hpp"

#include <cmath>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/movement/MovementSystem.hpp"
#include "runtime/save/SaveLoad.hpp"
#include "runtime/session/Session.hpp"
#include "runtime/session/SessionTick.hpp"

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool nearly(float a, float b, float tolerance = 0.02F) {
  return std::fabs(a - b) <= tolerance;
}

iggy3d::Transform3 transformAt(float x, float y, float z) {
  iggy3d::Transform3 transform;
  transform.position = {x, y, z};
  return transform;
}

iggy3d::WorldState makeWorldAt(iggy3d::Vec3 position) {
  iggy3d::WorldState world;
  iggy3d::EntityState player;
  player.id = {1};
  player.stableName = "player";
  player.kind = iggy3d::EntityKind::Player;
  player.transform = transformAt(position.x, position.y, position.z);
  player.localBounds =
      iggy3d::makeAabb3({-0.25F, 0.0F, -0.25F}, {0.25F, 1.8F, 0.25F});
  player.active = true;
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

// A stealth_blockout clamber block: 1x1 m footprint, kit-exact height,
// front face at x = 1.0 (player approaches along +X from the origin).
iggy3d::RoomSpatialSurface kitBlockSurface(
    std::string_view id, float heightMeters, float frontXMeters = 1.0F,
    std::vector<std::string> tags = {"blocker"}) {
  iggy3d::RoomSpatialSurface surface;
  surface.id = std::string(id);
  surface.sourceStaticMeshId = "stealth_blockout_fixture";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Box;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Blocker;
  surface.pointsMeters = {
      {frontXMeters, 0.0F, -0.5F},
      {frontXMeters + 1.0F, 0.0F, -0.5F},
      {frontXMeters + 1.0F, heightMeters, 0.5F},
      {frontXMeters, heightMeters, 0.5F},
  };
  surface.normal = {-1.0F, 0.0F, 0.0F};
  surface.traversalTags = std::move(tags);
  surface.collisionMask = {"actor"};
  surface.blocksActor = true;
  return surface;
}

// The walkable top of a kit block (the kit's iggy_walkable extras bake to
// exactly this kind of surface): needed to STAND on a piece after landing.
iggy3d::RoomSpatialSurface kitBlockTopSurface(std::string_view id,
                                              float heightMeters,
                                              float frontXMeters = 1.0F) {
  iggy3d::RoomSpatialSurface surface;
  surface.id = std::string(id);
  surface.sourceStaticMeshId = "stealth_blockout_fixture";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Plane;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Walkable;
  surface.pointsMeters = {
      {frontXMeters, heightMeters, -0.5F},
      {frontXMeters + 1.0F, heightMeters, -0.5F},
      {frontXMeters + 1.0F, heightMeters, 0.5F},
      {frontXMeters, heightMeters, 0.5F},
  };
  surface.normal = {0.0F, 1.0F, 0.0F};
  surface.traversalTags = {"walkable"};
  surface.collisionMask = {"actor"};
  return surface;
}

// Headroom blocker hovering above the ledge landing zone but recessed past
// the climb face, so the ground-level push still hits the block first. Its
// own top is far out of band, so it can never be a clamber target itself.
iggy3d::RoomSpatialSurface headroomBlockerSurface() {
  iggy3d::RoomSpatialSurface surface;
  surface.id = "headroom_blocker";
  surface.sourceStaticMeshId = "stealth_blockout_fixture";
  surface.shape = iggy3d::RoomSpatialSurfaceShape::Box;
  surface.role = iggy3d::RoomSpatialSurfaceRole::Blocker;
  surface.pointsMeters = {
      {1.3F, 1.6F, -0.5F},
      {2.0F, 1.6F, -0.5F},
      {2.0F, 2.6F, 0.5F},
      {1.3F, 2.6F, 0.5F},
  };
  surface.normal = {0.0F, -1.0F, 0.0F};
  surface.traversalTags = {"blocker"};
  surface.collisionMask = {"actor"};
  surface.blocksActor = true;
  return surface;
}

iggy3d::SpatialSurfaceSet makeSurfaceSet(
    std::vector<iggy3d::RoomSpatialSurface> surfaces) {
  iggy3d::RoomAsset room;
  room.id = "clamber_fixture_room";
  room.spatialSurfaces = std::move(surfaces);
  return iggy3d::buildSpatialSurfaceSet(room);
}

// Start at skin contact with the face at x = 1.0 (center = face - radius -
// skin) so the push is a FULL stop -- the engagement gesture. A player
// approaching from farther away simply closes the gap on the prior tick.
constexpr float kSkinContactStartX = 1.0F - 0.30F - 0.02F;

iggy3d::MovementRequest pushIntoWallRequest(bool allowClamber) {
  iggy3d::MovementRequest request;
  request.actor = {1};
  request.destination = {kSkinContactStartX + 0.2F, 0.0F,
                         0.0F};  // straight +X into the face
  request.mode = iggy3d::MovementMode::Walk;
  request.maxDistanceMeters = 3.0F;
  request.sourceCommandId = 7;
  request.allowClamber = allowClamber;
  return request;
}

iggy3d::MovementResult runPlannerMove(const iggy3d::SpatialSurfaceSet& surfaces,
                                      iggy3d::Vec3 startPosition,
                                      bool allowClamber) {
  iggy3d::WorldState world = makeWorldAt(startPosition);
  iggy3d::RuntimeConfig config = iggy3d::makeDefaultRuntimeConfig();
  iggy3d::MovementSystemContext context{&world, &config, &surfaces, true,
                                        nullptr};
  return iggy3d::executeMovement(context, pushIntoWallRequest(allowClamber));
}

// --- Pin 1: the kit ladder engages at every rung -------------------------

bool laddersEngageWithKitHeights() {
  const float kitHeights[] = {0.6F, 1.0F, 1.4F, 1.8F};
  bool ok = true;
  for (const float height : kitHeights) {
    const iggy3d::SpatialSurfaceSet surfaces = makeSurfaceSet(
        {floorSurface(), kitBlockSurface("clamber_block", height)});
    const iggy3d::MovementResult result =
        runPlannerMove(surfaces, {kSkinContactStartX, 0.0F, 0.0F}, true);
    const std::string label = "kit block " + std::to_string(height);
    ok = expect(result.clamberEvaluated && result.clamberEngaged,
                (label + " engages").c_str()) &&
         expect(result.clamberReasonCode == "clamber_engaged",
                (label + " reason").c_str()) &&
         expect(nearly(result.clamberTargetMeters.y, height),
                (label + " lands on top").c_str()) &&
         expect(result.clamberTargetMeters.x > 1.0F,
                (label + " lands past the face").c_str()) &&
         expect(result.clamberDurationTicks ==
                    iggy3d::MovementParams{}.clamberDurationTicks,
                (label + " fixed duration").c_str()) &&
         ok;
  }
  return ok;
}

// --- Pin 2: exact refusals ----------------------------------------------

bool bandRefusesKitFailBlock() {
  // The kit's clamber_fail_2p0: 0.2 above the band top. MUST refuse.
  const iggy3d::SpatialSurfaceSet surfaces =
      makeSurfaceSet({floorSurface(), kitBlockSurface("clamber_fail", 2.0F)});
  const iggy3d::MovementResult result =
      runPlannerMove(surfaces, {kSkinContactStartX, 0.0F, 0.0F}, true);
  return expect(result.clamberEvaluated && !result.clamberEngaged,
                "2.0 block refuses") &&
         expect(result.clamberReasonCode == "clamber_top_above_band",
                "2.0 block refusal is the band") &&
         expect(result.blocked == iggy3d::MovementBlockedReason::BlockedByCollision,
                "2.0 block leaves locomotion blocked as before");
}

bool clearanceRefusesBlockedHeadroom() {
  // Band-height ledge (1.0) with a hovering blocker 0.6 above its top: no
  // landing clearance for the 1.80 capsule.
  const iggy3d::SpatialSurfaceSet surfaces = makeSurfaceSet(
      {floorSurface(), kitBlockSurface("clamber_block", 1.0F),
       headroomBlockerSurface()});
  const iggy3d::MovementResult result =
      runPlannerMove(surfaces, {kSkinContactStartX, 0.0F, 0.0F}, true);
  return expect(result.clamberEvaluated && !result.clamberEngaged,
                "blocked headroom refuses") &&
         expect(result.clamberReasonCode == "clamber_no_landing_clearance",
                "blocked headroom refusal is clearance");
}

bool deniedTagRefuses() {
  const iggy3d::SpatialSurfaceSet surfaces = makeSurfaceSet(
      {floorSurface(),
       kitBlockSurface("no_player_block", 1.0F, 1.0F,
                       {"blocker", "no_player"})});
  const iggy3d::MovementResult result =
      runPlannerMove(surfaces, {kSkinContactStartX, 0.0F, 0.0F}, true);
  return expect(result.clamberEvaluated && !result.clamberEngaged &&
                    result.clamberReasonCode == "clamber_surface_denied",
                "no_player tag denies clamber");
}

bool npcIntentNeverEngages() {
  const iggy3d::SpatialSurfaceSet surfaces =
      makeSurfaceSet({floorSurface(), kitBlockSurface("clamber_block", 1.0F)});
  const iggy3d::MovementResult result =
      runPlannerMove(surfaces, {kSkinContactStartX, 0.0F, 0.0F}, false);
  return expect(!result.clamberEvaluated && !result.clamberEngaged,
                "clamber not evaluated without allowClamber (NPC path)");
}

// --- Pin 3: autoStep still steps, never clambers ------------------------

bool autoStepStillStepsNeverClambers() {
  // The kit stair_run riser (0.25, one snap step) walks via autoStep. Its
  // walkable tread top bakes from the kit's iggy_collision_part_walkable.
  const iggy3d::SpatialSurfaceSet surfaces = makeSurfaceSet(
      {floorSurface(), kitBlockSurface("stair_riser", 0.25F),
       kitBlockTopSurface("stair_riser_top", 0.25F)});
  const iggy3d::MovementResult result =
      runPlannerMove(surfaces, {kSkinContactStartX, 0.0F, 0.0F}, true);
  return expect(result.blocked == iggy3d::MovementBlockedReason::None,
                "0.25 riser walks") &&
         expect(!result.clamberEngaged && !result.clamberEvaluated,
                "0.25 riser never reaches clamber evaluation") &&
         expect(result.finalPosition.y > 0.2F, "0.25 riser stepped up");
}

// --- Session-level fixture ----------------------------------------------

iggy3d::ScenarioEntitySeed playerSeed() {
  iggy3d::ScenarioEntitySeed seed;
  seed.stableName = "player";
  seed.kind = iggy3d::ScenarioEntityKind::Player;
  seed.transform = transformAt(kSkinContactStartX, 0.0F, 0.0F);
  seed.localBounds =
      iggy3d::makeAabb3({-0.25F, 0.0F, -0.25F}, {0.25F, 1.8F, 0.25F});
  seed.active = true;
  seed.persistent = true;
  return seed;
}

iggy3d::Session makeLadderSession() {
  iggy3d::FixtureScenarioSeed seed;
  seed.scenarioId = "clamber_ladder.proof";
  seed.config = iggy3d::makeDefaultRuntimeConfig();
  seed.initialClockMode = iggy3d::ScenarioClockMode::Normal;
  seed.defaultRealtimeCamera = iggy3d::ScenarioCameraMode::ThirdPerson;
  seed.defaultTacticalCamera = iggy3d::ScenarioCameraMode::TacticalOverhead;
  seed.players.push_back({0, iggy3d::ScenarioPlayerSlotKind::Local, "player"});
  seed.entities = {playerSeed()};
  // Session creation requires at least one objective; this one never
  // completes (no gold key exists in the fixture), so it is inert.
  iggy3d::ScenarioObjectiveSeed objective;
  objective.id = "collect_gold_key";
  objective.initialStatus = iggy3d::ObjectiveStatusSeed::Active;
  objective.condition = "InventoryContains";
  objective.playerSlot = 0;
  objective.itemId = "gold_key";
  objective.itemCount = 1;
  objective.completeStatus = iggy3d::ObjectiveStatusSeed::Complete;
  seed.objectives.push_back(objective);
  iggy3d::SessionCreateRequest request;
  request.config = seed.config;
  request.seed = seed;
  return iggy3d::Session::create(request).value;
}

iggy3d::CommandRecord localMoveCommand(const iggy3d::Session& session,
                                       iggy3d::Vec3 point) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = session.state().world.entities().front().id;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.kind = iggy3d::CommandKind::Move;
  command.payload.target.hasPoint = true;
  command.payload.target.point = point;
  return command;
}

iggy3d::Vec3 playerPosition(const iggy3d::Session& session) {
  return session.state().world.entities().front().transform.position;
}

struct PhaseRunResult {
  bool valid = false;
  std::vector<iggy3d::Vec3> perTickPositions;
  std::size_t engageSoundCount = 0;
  float engageLoudnessDb = 0.0F;
};

iggy3d::CommandRecord localWaitCommand(const iggy3d::Session& session) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = session.state().world.entities().front().id;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.kind = iggy3d::CommandKind::Wait;
  return command;
}

// Push into the block until the phase engages (the seed starts at skin
// contact, so the first push should engage; the loop absorbs float slack),
// then tick the phase to completion on Wait commands, Play-mode style.
PhaseRunResult runLadderPhase(const iggy3d::SpatialSurfaceSet& surfaces) {
  PhaseRunResult run;
  iggy3d::Session session = makeLadderSession();
  bool engaged = false;
  for (int push = 0; push < 3 && !engaged; ++push) {
    const iggy3d::Vec3 position = playerPosition(session);
    if (session
                .submitCommand(localMoveCommand(
                    session, position + iggy3d::Vec3{0.2F, 0.0F, 0.0F}))
                .command.admission !=
            iggy3d::CommandAdmissionStatus::Accepted ||
        session.tickWithOptions({&surfaces, true}).status !=
            iggy3d::ResultStatus::Ok) {
      return run;
    }
    engaged = session.state().clamberPhase.active;
    if (engaged) {
      // The engage tick's transient bus must carry exactly the clamber noise.
      for (const iggy3d::SoundEvent& sound :
           session.state().transient.soundEvents) {
        if (nearly(sound.loudnessDb,
                   iggy3d::MovementParams{}.clamberLoudnessDb)) {
          ++run.engageSoundCount;
          run.engageLoudnessDb = sound.loudnessDb;
        }
      }
    }
    run.perTickPositions.push_back(playerPosition(session));
  }
  if (!engaged) {
    return run;
  }
  const std::uint32_t duration = iggy3d::MovementParams{}.clamberDurationTicks;
  for (std::uint32_t tick = 0; tick < duration + 2U; ++tick) {
    if (session.submitCommand(localWaitCommand(session)).command.admission !=
            iggy3d::CommandAdmissionStatus::Accepted ||
        session.tickWithOptions({&surfaces, true}).status !=
            iggy3d::ResultStatus::Ok) {
      return run;
    }
    run.perTickPositions.push_back(playerPosition(session));
  }
  run.valid = !session.state().clamberPhase.active;
  return run;
}

// --- Pins 1+5: full phase ends standing on top; one engage sound ---------

bool phaseEndsStandingOnLedgeWithOneSound() {
  const iggy3d::SpatialSurfaceSet surfaces = makeSurfaceSet(
      {floorSurface(), kitBlockSurface("clamber_block", 1.0F),
       kitBlockTopSurface("clamber_block_top", 1.0F)});
  const PhaseRunResult run = runLadderPhase(surfaces);
  if (!expect(run.valid, "phase run completes")) {
    return false;
  }
  const iggy3d::Vec3 finalPosition = run.perTickPositions.back();
  return expect(nearly(finalPosition.y, 1.0F), "phase ends on ledge top") &&
         expect(finalPosition.x > 1.0F, "phase ends past the face") &&
         expect(run.engageSoundCount == 1U,
                "exactly one clamber engage sound") &&
         expect(nearly(run.engageLoudnessDb,
                       iggy3d::MovementParams{}.clamberLoudnessDb),
                "engage sound uses the clamber loudness constant");
}

// --- Pin 4: determinism --------------------------------------------------

bool phaseIsDeterministic() {
  const iggy3d::SpatialSurfaceSet surfaces = makeSurfaceSet(
      {floorSurface(), kitBlockSurface("clamber_block", 1.4F),
       kitBlockTopSurface("clamber_block_top", 1.4F)});
  const PhaseRunResult first = runLadderPhase(surfaces);
  const PhaseRunResult second = runLadderPhase(surfaces);
  if (!expect(first.valid && second.valid, "both determinism runs complete") ||
      !expect(first.perTickPositions.size() == second.perTickPositions.size(),
              "same tick count")) {
    return false;
  }
  for (std::size_t i = 0; i < first.perTickPositions.size(); ++i) {
    const iggy3d::Vec3 a = first.perTickPositions[i];
    const iggy3d::Vec3 b = second.perTickPositions[i];
    if (!expect(a.x == b.x && a.y == b.y && a.z == b.z,
                "identical positions per tick")) {
      return false;
    }
  }
  return true;
}

// --- Pin 6: transience ---------------------------------------------------

bool midPhaseSaveLoadDoesNotBrick() {
  const iggy3d::SpatialSurfaceSet surfaces = makeSurfaceSet(
      {floorSurface(), kitBlockSurface("clamber_block", 1.0F),
       kitBlockTopSurface("clamber_block_top", 1.0F)});
  iggy3d::Session session = makeLadderSession();
  const iggy3d::Vec3 start = playerPosition(session);
  if (session
              .submitCommand(localMoveCommand(
                  session, start + iggy3d::Vec3{0.2F, 0.0F, 0.0F}))
              .command.admission != iggy3d::CommandAdmissionStatus::Accepted ||
      session.tickWithOptions({&surfaces, true}).status !=
          iggy3d::ResultStatus::Ok ||
      !session.state().clamberPhase.active) {
    return expect(false, "mid-phase setup");
  }
  // Advance two phase ticks so we are genuinely mid-clamber.
  for (int i = 0; i < 2; ++i) {
    if (session.submitCommand(localWaitCommand(session)).command.admission !=
            iggy3d::CommandAdmissionStatus::Accepted ||
        session.tickWithOptions({&surfaces, true}).status !=
            iggy3d::ResultStatus::Ok) {
      return expect(false, "mid-phase advance");
    }
  }
  if (!expect(session.state().clamberPhase.active, "phase active mid-save")) {
    return false;
  }
  const iggy3d::SaveStateResult saved =
      iggy3d::saveSessionState(session.state());
  if (!expect(saved.status == iggy3d::SaveLoadStatus::Ok, "mid-phase save ok")) {
    return false;
  }
  iggy3d::Session loadTarget = makeLadderSession();
  const iggy3d::SaveCompatibilityRequest compatibility{
      saved.envelope, saved.envelope.session.packageId,
      saved.envelope.session.scenarioId};
  const iggy3d::LoadStateResult loaded =
      iggy3d::loadEnvelopeIntoSession(loadTarget, saved.envelope,
                                      compatibility);
  if (!expect(loaded.status == iggy3d::SaveLoadStatus::Ok,
              "mid-phase load ok") ||
      !expect(!loadTarget.state().clamberPhase.active,
              "loaded session has no clamber phase (never persisted)")) {
    return false;
  }
  const bool loadedTicks =
      loadTarget.submitCommand(localWaitCommand(loadTarget)).command.admission ==
          iggy3d::CommandAdmissionStatus::Accepted &&
      loadTarget.tickWithOptions({&surfaces, true}).status ==
          iggy3d::ResultStatus::Ok;
  return expect(loadedTicks, "loaded session still ticks") &&
         expect(session.state().clamberPhase.active,
                "saving did not disturb the live phase");
}

}  // namespace

int main() {
  const bool ok = laddersEngageWithKitHeights() &&
                  bandRefusesKitFailBlock() &&
                  clearanceRefusesBlockedHeadroom() &&
                  deniedTagRefuses() &&
                  npcIntentNeverEngages() &&
                  autoStepStillStepsNeverClambers() &&
                  phaseEndsStandingOnLedgeWithOneSound() &&
                  phaseIsDeterministic() &&
                  midPhaseSaveLoadDoesNotBrick();
  if (ok) {
    std::cout << "clamber_motor_tests passed\n";
  }
  return ok ? 0 : 1;
}
