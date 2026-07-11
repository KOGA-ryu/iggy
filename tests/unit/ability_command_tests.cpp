#include "runtime/save/SaveCodec.hpp"
#include "runtime/save/SaveLoad.hpp"
#include "runtime/session/Session.hpp"

#include "content/assets/RoomAsset.hpp"
#include "runtime/ability/AbilitySystem.hpp"

#include <cstdint>
#include <iostream>
#include <string_view>
#include <utility>

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
  seed.kind = iggy3d::ScenarioEntityKind::Player;
  seed.transform = transformAt(0.0F, 0.0F, 0.0F);
  seed.localBounds = iggy3d::makeAabb3({-0.25F, 0.0F, -0.25F}, {0.25F, 1.8F, 0.25F});
  seed.active = true;
  seed.persistent = true;
  seed.combatantEnabled = true;
  seed.combatant.factionId = 1;
  seed.combatant.hitPoints = 10;
  seed.combatant.maxHitPoints = 10;
  return seed;
}

iggy3d::ScenarioEntitySeed trainingDummySeed() {
  iggy3d::ScenarioEntitySeed seed;
  seed.stableName = "training_dummy";
  seed.kind = iggy3d::ScenarioEntityKind::Npc;
  seed.transform = transformAt(0.0F, 0.0F, -4.0F);
  seed.localBounds = iggy3d::makeAabb3({-0.25F, 0.0F, -0.25F}, {0.25F, 1.8F, 0.25F});
  seed.active = true;
  seed.persistent = true;
  seed.targeting.targetable = true;
  seed.targeting.actions = {iggy3d::ScenarioTargetAction::Attack};
  seed.combatantEnabled = true;
  seed.combatant.factionId = 2;
  seed.combatant.hitPoints = 6;
  seed.combatant.maxHitPoints = 6;
  return seed;
}

iggy3d::FixtureScenarioSeed abilitySeed() {
  iggy3d::FixtureScenarioSeed seed;
  seed.scenarioId = "ability_command.runtime_loop";
  seed.config = iggy3d::makeDefaultRuntimeConfig();
  seed.initialClockMode = iggy3d::ScenarioClockMode::Normal;
  seed.defaultRealtimeCamera = iggy3d::ScenarioCameraMode::FirstPerson;
  seed.defaultTacticalCamera = iggy3d::ScenarioCameraMode::TacticalOverhead;
  seed.players.push_back({0, iggy3d::ScenarioPlayerSlotKind::Local, "player"});
  seed.entities = {playerSeed(), trainingDummySeed()};
  iggy3d::ScenarioObjectiveSeed objective;
  objective.id = "survive";
  objective.initialStatus = iggy3d::ObjectiveStatusSeed::Active;
  objective.condition = "None";
  seed.objectives.push_back(objective);
  return seed;
}

iggy3d::Session makeSession() {
  iggy3d::SessionCreateRequest request;
  request.packageId = "ability_command_tests";
  request.config = iggy3d::makeDefaultRuntimeConfig();
  request.seed = abilitySeed();
  return iggy3d::Session::create(request).value;
}

iggy3d::CommandRecord submittedArcaneBolt() {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::CastAbility;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.ability = iggy3d::CommandAbilityKind::ArcaneBolt;
  command.payload.abilityDirection = {0.0F, 0.0F, -1.0F};
  return command;
}

const iggy3d::CombatantState* combatantFor(const iggy3d::SessionState& state,
                                           iggy3d::EntityId entity) {
  for (const iggy3d::CombatantState& combatant : state.combat.combatants) {
    if (combatant.entity == entity) {
      return &combatant;
    }
  }
  return nullptr;
}

// NPC actors enqueue commands during ticks, so the player's cast is no longer
// guaranteed to be the last log record; locate it by kind instead.
template <typename Records>
auto findLastCastAbility(const Records& records) -> decltype(&records.front()) {
  for (auto it = records.rbegin(); it != records.rend(); ++it) {
    if (it->kind == iggy3d::CommandKind::CastAbility) {
      return &*it;
    }
  }
  return nullptr;
}

const iggy3d::CommandRecord* playerCastCommand(const iggy3d::Session& session) {
  return findLastCastAbility(session.state().commandLog.records());
}

iggy3d::SpatialSurfaceSet projectileWallSurfaceSet() {
  iggy3d::RoomSpatialSurface wall;
  wall.id = "test_projectile_wall";
  wall.shape = iggy3d::RoomSpatialSurfaceShape::Box;
  wall.role = iggy3d::RoomSpatialSurfaceRole::ProjectileBlocker;
  wall.pointsMeters = {{-1.0F, 0.0F, -2.60F}, {1.0F, 2.0F, -2.50F}};
  wall.normal = {0.0F, 0.0F, 1.0F};
  wall.blocksProjectile = true;
  iggy3d::RoomAsset room;
  room.spatialSurfaces.push_back(std::move(wall));
  return iggy3d::buildSpatialSurfaceSet(room);
}

bool tickUntilAbilityIdle(iggy3d::Session& session,
                          const iggy3d::SpatialSurfaceSet* surfaces,
                          std::uint32_t maxTicks) {
  for (std::uint32_t tick = 0; tick < maxTicks; ++tick) {
    if (session.state().transient.pendingExecutionSequences.empty() &&
        !iggy3d::abilityRuntimeHasActiveProjectile(session.state().transient.abilityRuntime)) {
      return true;
    }
    const iggy3d::StatusResult result = session.tick(surfaces);
    if (result.status != iggy3d::ResultStatus::Ok) {
      return false;
    }
  }
  return session.state().transient.pendingExecutionSequences.empty() &&
         !iggy3d::abilityRuntimeHasActiveProjectile(session.state().transient.abilityRuntime);
}

bool admissionRequiresNamedAbilityPayload() {
  iggy3d::Session session = makeSession();
  iggy3d::CommandRecord missingAbility = submittedArcaneBolt();
  missingAbility.payload.ability = iggy3d::CommandAbilityKind::None;
  const iggy3d::SessionCommandResult missing = session.submitCommand(missingAbility);

  iggy3d::CommandRecord zeroDirection = submittedArcaneBolt();
  zeroDirection.payload.abilityDirection = {};
  const iggy3d::SessionCommandResult zero = session.submitCommand(zeroDirection);

  const iggy3d::SessionCommandResult accepted = session.submitCommand(submittedArcaneBolt());

  return expect(missing.command.admission == iggy3d::CommandAdmissionStatus::Rejected,
                "missing ability rejected") &&
         expect(missing.command.rejection == iggy3d::CommandRejectionReason::InvalidCommand,
                "missing ability reason") &&
         expect(zero.command.admission == iggy3d::CommandAdmissionStatus::Rejected,
                "zero direction rejected") &&
         expect(zero.command.rejection == iggy3d::CommandRejectionReason::InvalidTargetPoint,
                "zero direction reason") &&
         expect(accepted.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                "arcane bolt accepted") &&
         expect(session.state().transient.pendingExecutionSequences.size() == 1U,
                "accepted ability queued") &&
         expect(iggy3d::requiresActor(iggy3d::CommandKind::CastAbility),
                "ability requires actor") &&
         expect(!iggy3d::requiresEntityTarget(iggy3d::CommandKind::CastAbility),
                "ability discovers entity target") &&
         expect(iggy3d::requiresAbilityPayload(iggy3d::CommandKind::CastAbility),
                "ability payload required");
}

bool sessionTickExecutesAbilityDamage() {
  iggy3d::Session session = makeSession();
  const iggy3d::SessionCommandResult submitted = session.submitCommand(submittedArcaneBolt());
  const iggy3d::SessionCommandResult busy = session.submitCommand(submittedArcaneBolt());
  const iggy3d::StatusResult firstTick = session.tick();
  const iggy3d::CombatantState* dummyAfterFirstTick = combatantFor(session.state(), {2});
  const std::int32_t firstTickHitPoints =
      dummyAfterFirstTick == nullptr ? -1 : dummyAfterFirstTick->hitPoints;
  const bool activeAfterFirstTick =
      iggy3d::abilityRuntimeHasActiveProjectile(session.state().transient.abilityRuntime);
  const bool drained = tickUntilAbilityIdle(session, nullptr, 16);
  const iggy3d::CommandLogCounts counts = session.state().commandLog.counts();
  const iggy3d::CombatantState* dummy = combatantFor(session.state(), {2});

  return expect(submitted.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                "cast accepted before tick") &&
         expect(busy.command.admission == iggy3d::CommandAdmissionStatus::Rejected,
                "busy cast rejected") &&
         expect(busy.command.rejection == iggy3d::CommandRejectionReason::AbilitySlotBusy,
                "busy cast rejection reason") &&
         expect(firstTick.status == iggy3d::ResultStatus::Ok, "ability first tick ok") &&
         expect(dummyAfterFirstTick != nullptr && firstTickHitPoints == 6,
                "first tick only starts flight") &&
         expect(activeAfterFirstTick, "projectile active after first tick") &&
         expect(drained, "ability drains to idle") &&
         expect(session.state().transient.pendingExecutionSequences.empty(),
                "ability queue consumed") &&
         expect(session.state().clock.tickIndex > 1U, "clock advanced across flight") &&
         expect(dummy != nullptr, "dummy combatant exists") &&
         expect(dummy != nullptr && dummy->hitPoints == 3, "arcane bolt damage applied") &&
         expect(!dummy->defeated, "dummy not defeated") &&
         expect(session.state().transient.metrics.abilityCasts == 1U, "ability cast metric") &&
         expect(session.state().transient.metrics.abilityImpacts == 1U,
                "ability impact metric") &&
         expect(session.state().transient.metrics.combatExecutions == 1U,
                "combat metric from ability") &&
         expect(counts.ability == 2U, "ability commands counted separately") &&
         expect(counts.combat == 0U, "ability command not counted as direct attack") &&
         expect(session.state().transient.events.size() >= 3U, "ability emitted events");
}

bool sessionAdmissionUsesDurableAbilityPolicy() {
  iggy3d::Session session = makeSession();
  const iggy3d::SessionCommandResult submitted = session.submitCommand(submittedArcaneBolt());
  const iggy3d::StatusResult firstTick = session.tick();

  iggy3d::SessionState& state = session.mutableStateForOwnedSystems();
  iggy3d::resetAbilityRuntime(state.transient.abilityRuntime);
  const iggy3d::SessionCommandResult cooldown = session.submitCommand(submittedArcaneBolt());

  state.abilities.actors[0].arcaneBoltReadyTick = state.clock.tickIndex;
  state.abilities.actors[0].arcaneFocus = 0U;
  const iggy3d::SessionCommandResult depleted = session.submitCommand(submittedArcaneBolt());
  const iggy3d::CommandLogCounts counts = session.state().commandLog.counts();

  return expect(submitted.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                "policy cast accepted") &&
         expect(firstTick.status == iggy3d::ResultStatus::Ok, "policy first tick ok") &&
         expect(session.state().abilities.actors.size() == 1U, "policy actor created") &&
         expect(session.state().abilities.actors[0].arcaneFocus == 0U,
                "policy focus depleted for test") &&
         expect(cooldown.command.admission == iggy3d::CommandAdmissionStatus::Rejected,
                "cooldown command rejected") &&
         expect(cooldown.command.rejection == iggy3d::CommandRejectionReason::AbilityOnCooldown,
                "cooldown rejection reason") &&
         expect(depleted.command.admission == iggy3d::CommandAdmissionStatus::Rejected,
                "resource command rejected") &&
         expect(depleted.command.rejection ==
                    iggy3d::CommandRejectionReason::AbilityInsufficientResource,
                "resource rejection reason") &&
         expect(counts.ability == 3U, "policy commands counted") &&
         expect(counts.rejected == 2U, "policy rejected count");
}

bool sessionTickAdvancesAbilityRechargeWithoutProjectile() {
  iggy3d::Session session = makeSession();
  const iggy3d::SessionCommandResult submitted = session.submitCommand(submittedArcaneBolt());
  const iggy3d::StatusResult firstTick = session.tick();
  iggy3d::SessionState& state = session.mutableStateForOwnedSystems();
  iggy3d::resetAbilityRuntime(state.transient.abilityRuntime);

  bool ticked = false;
  bool failed = false;
  for (std::uint32_t tick = 0; tick < 32U; ++tick) {
    if (!iggy3d::abilityStateHasPendingRecharge(session.state().abilities)) {
      break;
    }
    const iggy3d::StatusResult result = session.tick();
    ticked = true;
    failed = failed || result.status != iggy3d::ResultStatus::Ok;
  }

  return expect(submitted.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                "recharge cast accepted") &&
         expect(firstTick.status == iggy3d::ResultStatus::Ok, "recharge first tick ok") &&
         expect(ticked, "recharge ticked") &&
         expect(!failed, "recharge ticks ok") &&
         expect(session.state().clock.tickIndex >= 21U, "recharge clock advanced") &&
         expect(session.state().abilities.actors[0].arcaneFocus == 3U,
                "session recharge full") &&
         expect(session.state().abilities.actors[0].arcaneFocusNextRechargeTick == 0U,
                "session recharge timer cleared") &&
         expect(!iggy3d::abilityStateHasPendingRecharge(session.state().abilities),
                "session no pending recharge");
}

bool projectileWallBlocksSessionOwnedAbilityDamage() {
  const iggy3d::SpatialSurfaceSet surfaces = projectileWallSurfaceSet();
  iggy3d::Session session = makeSession();
  static_cast<void>(session.submitCommand(submittedArcaneBolt()));
  const bool drained = tickUntilAbilityIdle(session, &surfaces, 16);
  const iggy3d::AbilityProjectileState& projectile =
      session.state().transient.abilityRuntime.arcaneBolt;
  const iggy3d::CombatantState* dummy = combatantFor(session.state(), {2});

  return expect(drained, "wall ability drains") &&
         expect(projectile.impact, "wall projectile impact") &&
         expect(projectile.impactKind == iggy3d::AbilityImpactKind::Surface,
                "wall impact kind") &&
         expect(projectile.hitSurfaceId == "test_projectile_wall", "wall surface id") &&
         expect(dummy != nullptr && dummy->hitPoints == 6, "wall prevents entity damage") &&
         expect(session.state().transient.metrics.abilityImpacts == 1U,
                "wall impact metric") &&
         expect(session.state().transient.metrics.combatExecutions == 0U,
                "wall no combat metric");
}

bool saveLoadPreservesAbilityCommandPayload() {
  iggy3d::Session session = makeSession();
  static_cast<void>(session.submitCommand(submittedArcaneBolt()));
  static_cast<void>(session.tick());
  const bool activeBeforeSave =
      iggy3d::abilityRuntimeHasActiveProjectile(session.state().transient.abilityRuntime);

  const iggy3d::SaveStateResult saved = iggy3d::saveSessionStateEncoded(session.state());
  const iggy3d::SaveDecodeResult decoded = iggy3d::decodeSaveEnvelope(saved.encodedSaveText);
  iggy3d::Session loaded = makeSession();
  const iggy3d::SaveCompatibilityRequest compatibility{
      decoded.envelope, decoded.envelope.metadata.packageId, decoded.envelope.metadata.scenarioId};
  const iggy3d::LoadStateResult loadedResult =
      iggy3d::loadEncodedSaveIntoSession(loaded, saved.encodedSaveText, compatibility);
  const iggy3d::CommandRecord* loadedCommand = playerCastCommand(loaded);
  const auto* decodedCast = findLastCastAbility(decoded.envelope.commandLog.records);

  return expect(saved.status == iggy3d::SaveLoadStatus::Ok, "save ok") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok, "decode ok") &&
         expect(decodedCast != nullptr, "decoded command exists") &&
         expect(decodedCast != nullptr && decodedCast->kind == iggy3d::CommandKind::CastAbility,
                "decoded command kind") &&
         expect(decodedCast != nullptr &&
                    decodedCast->ability == iggy3d::CommandAbilityKind::ArcaneBolt,
                "decoded ability id") &&
         expect(decodedCast != nullptr &&
                    iggy3d::nearlyEqual(decodedCast->abilityDirection, {0.0F, 0.0F, -1.0F}),
                "decoded ability direction") &&
         expect(loadedResult.status == iggy3d::SaveLoadStatus::Ok, "load ok") &&
         expect(loadedCommand != nullptr, "loaded command exists") &&
         expect(loadedCommand != nullptr && loadedCommand->kind == iggy3d::CommandKind::CastAbility,
                "loaded command kind") &&
         expect(loadedCommand != nullptr &&
                    loadedCommand->payload.ability == iggy3d::CommandAbilityKind::ArcaneBolt,
                "loaded ability payload") &&
         expect(loadedCommand != nullptr &&
                    iggy3d::nearlyEqual(loadedCommand->payload.abilityDirection,
                                        {0.0F, 0.0F, -1.0F}),
                "loaded direction payload") &&
         expect(decoded.envelope.abilities.actors.size() == 1U, "decoded ability actor") &&
         expect(decoded.envelope.abilities.actors[0].actor == iggy3d::EntityId{1},
                "decoded ability actor id") &&
         expect(decoded.envelope.abilities.actors[0].arcaneFocus == 2U,
                "decoded ability focus") &&
         expect(decoded.envelope.abilities.actors[0].arcaneBoltReadyTick == 8U,
                "decoded ability cooldown") &&
         expect(decoded.envelope.abilities.actors[0].arcaneFocusNextRechargeTick == 20U,
                "decoded ability recharge") &&
         expect(loaded.state().abilities.actors.size() == 1U, "loaded ability actor") &&
         expect(loaded.state().abilities.actors[0].arcaneFocus == 2U,
                "loaded ability focus") &&
         expect(loaded.state().abilities.actors[0].arcaneBoltReadyTick == 8U,
                "loaded ability cooldown") &&
         expect(loaded.state().abilities.actors[0].arcaneFocusNextRechargeTick == 20U,
                "loaded ability recharge") &&
         expect(activeBeforeSave, "source save had active transient projectile") &&
         expect(!loaded.state().transient.abilityRuntime.arcaneBolt.spawned,
                "load clears transient projectile") &&
         expect(loaded.stateHash() == session.stateHash(), "load preserves state hash");
}

}  // namespace

int main() {
  const bool ok = admissionRequiresNamedAbilityPayload() &&
                  sessionTickExecutesAbilityDamage() &&
                  sessionAdmissionUsesDurableAbilityPolicy() &&
                  sessionTickAdvancesAbilityRechargeWithoutProjectile() &&
                  projectileWallBlocksSessionOwnedAbilityDamage() &&
                  saveLoadPreservesAbilityCommandPayload();
  return ok ? 0 : 1;
}
