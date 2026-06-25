#include "runtime/command/CommandAdmission.hpp"
#include "runtime/inventory/InventorySystem.hpp"

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

iggy3d::EntityState makeEntity(
    iggy3d::EntityId id,
    std::string_view name,
    iggy3d::EntityKind kind,
    iggy3d::Vec3 position) {
  iggy3d::EntityState entity;
  entity.id = id;
  entity.stableName = std::string(name);
  entity.kind = kind;
  entity.transform = transformAt(position.x, position.y, position.z);
  entity.localBounds = iggy3d::makeAabb3({-0.5F, 0.0F, -0.5F}, {0.5F, 1.0F, 0.5F});
  entity.active = true;
  return entity;
}

struct AdmissionFixture {
  iggy3d::WorldState world;
  iggy3d::PlayerRoster players;
  iggy3d::ClockState clock;
  iggy3d::CommandLog commandLog;
  iggy3d::RuntimeConfig config = iggy3d::makeDefaultRuntimeConfig();
  iggy3d::CombatState combat;
  iggy3d::InventoryState inventory;
};

iggy3d::EntityState makePlayer() {
  iggy3d::EntityState entity =
      makeEntity({1}, "player", iggy3d::EntityKind::Player, {0.0F, 0.0F, 0.0F});
  entity.targeting.targetable = true;
  entity.targeting.actions = {iggy3d::TargetAction::Attack, iggy3d::TargetAction::Inspect};
  return entity;
}

iggy3d::EntityState makeGoldKey() {
  iggy3d::EntityState entity =
      makeEntity({2}, "gold_key", iggy3d::EntityKind::Pickup, {3.0F, 0.0F, 0.0F});
  entity.targeting.targetable = true;
  entity.targeting.actions = {iggy3d::TargetAction::Interact, iggy3d::TargetAction::Inspect};
  entity.interaction.kind = iggy3d::InteractionKind::Pickup;
  entity.interaction.primaryEffect = iggy3d::InteractionEffectKind::AddItemToInventory;
  entity.interaction.itemId = "gold_key";
  entity.interaction.itemCount = 1;
  entity.interaction.objectiveId = "collect_gold_key";
  entity.interaction.deactivateTargetOnSuccess = true;
  return entity;
}

iggy3d::EntityState makeMarker() {
  iggy3d::EntityState entity = makeEntity({3}, "tactical_marker_alpha",
                                          iggy3d::EntityKind::Marker, {2.0F, 0.0F, 1.0F});
  entity.targeting.targetable = true;
  entity.targeting.actions = {iggy3d::TargetAction::Move, iggy3d::TargetAction::Inspect};
  return entity;
}

iggy3d::EntityState makeLockedSecretDoor() {
  iggy3d::EntityState entity =
      makeEntity({4}, "secret_door", iggy3d::EntityKind::Door, {2.5F, 0.0F, 0.0F});
  entity.targeting.targetable = true;
  entity.targeting.actions = {iggy3d::TargetAction::Interact,
                              iggy3d::TargetAction::Inspect};
  entity.interaction.kind = iggy3d::InteractionKind::OpenDoor;
  entity.interaction.primaryEffect = iggy3d::InteractionEffectKind::EmitEventOnly;
  entity.interaction.requiredItemId = "gold_key";
  entity.interaction.requiredItemCount = 1;
  entity.interaction.deactivateTargetOnSuccess = true;
  return entity;
}

iggy3d::EntityState makeTrainingNpc() {
  iggy3d::EntityState entity =
      makeEntity({5}, "training_npc", iggy3d::EntityKind::Npc, {1.0F, 0.0F, 0.0F});
  entity.targeting.targetable = true;
  entity.targeting.actions = {iggy3d::TargetAction::Attack, iggy3d::TargetAction::Inspect};
  return entity;
}

AdmissionFixture makeFirstRoomAdmissionFixture() {
  AdmissionFixture fixture;
  (void)fixture.world.seedEntity(makePlayer());
  (void)fixture.world.seedEntity(makeGoldKey());
  (void)fixture.world.seedEntity(makeMarker());
  (void)fixture.world.seedEntity(makeLockedSecretDoor());
  (void)fixture.world.seedEntity(makeTrainingNpc());
  iggy3d::PlayerSlot slot;
  slot.id = 0;
  slot.kind = iggy3d::PlayerSlotKind::Local;
  slot.actor = {1};
  slot.stableName = "player0";
  (void)fixture.players.addSlot(slot);
  fixture.combat.combatants.push_back({{1}, 1, 10, 10, false});
  fixture.combat.combatants.push_back({{5}, 2, 3, 3, false});
  fixture.inventory.players.push_back({0, {}});
  return fixture;
}

iggy3d::CommandRecord makeCommand(iggy3d::CommandKind kind, iggy3d::CommandId id = 1) {
  iggy3d::CommandRecord command;
  command.commandId = id;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = kind;
  command.source = iggy3d::CommandSource::LocalPlayer;
  return command;
}

iggy3d::CommandRecord interactCommand(iggy3d::CommandId id = 1) {
  iggy3d::CommandRecord command = makeCommand(iggy3d::CommandKind::Interact, id);
  command.payload.target.hasEntity = true;
  command.payload.target.entity = {2};
  return command;
}

iggy3d::CommandRecord moveCommand(iggy3d::Vec3 point, iggy3d::CommandId id = 1) {
  iggy3d::CommandRecord command = makeCommand(iggy3d::CommandKind::Move, id);
  command.payload.target.hasPoint = true;
  command.payload.target.point = point;
  return command;
}

iggy3d::CommandRecord attackCommand(iggy3d::CommandId id = 1) {
  iggy3d::CommandRecord command = makeCommand(iggy3d::CommandKind::Attack, id);
  command.payload.target.hasEntity = true;
  command.payload.target.entity = {5};
  command.payload.attackDamage = 3;
  return command;
}

iggy3d::CommandRecord aiAttackCommand(iggy3d::CommandId id = 1) {
  iggy3d::CommandRecord command;
  command.commandId = id;
  command.actor = {5};
  command.kind = iggy3d::CommandKind::Attack;
  command.source = iggy3d::CommandSource::Ai;
  command.payload.target.hasEntity = true;
  command.payload.target.entity = {1};
  command.payload.attackDamage = 1;
  return command;
}

iggy3d::CommandRecord aiMoveCommand(iggy3d::Vec3 point, iggy3d::CommandId id = 1) {
  iggy3d::CommandRecord command;
  command.commandId = id;
  command.actor = {5};
  command.kind = iggy3d::CommandKind::Move;
  command.source = iggy3d::CommandSource::Ai;
  command.payload.target.hasPoint = true;
  command.payload.target.point = point;
  return command;
}

iggy3d::CommandRecord retryCommand(iggy3d::CommandId sourceId, iggy3d::CommandId id = 2) {
  iggy3d::CommandRecord command = makeCommand(iggy3d::CommandKind::Retry, id);
  command.payload.retrySourceCommandId = sourceId;
  return command;
}

iggy3d::CommandAdmissionResult admit(AdmissionFixture& fixture, iggy3d::CommandRecord command) {
  return iggy3d::admitCommand(
      {&fixture.world, &fixture.players, &fixture.clock, &fixture.commandLog, &fixture.config,
       &fixture.combat, &fixture.inventory},
      {command});
}

bool acceptRejectHelpersSetInvariantFields() {
  iggy3d::CommandRecord command = moveCommand({2.0F, 0.0F, 0.0F}, 99);
  command.rejection = iggy3d::CommandRejectionReason::InvalidCommand;
  const iggy3d::CommandAdmissionResult accepted = iggy3d::acceptCommand(command);
  bool ok = expect(accepted.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                   "accepted status") &&
            expect(accepted.command.rejection == iggy3d::CommandRejectionReason::None,
                   "accepted clears rejection") &&
            expect(accepted.command.commandId == 99U, "accepted preserves id");

  const iggy3d::CommandAdmissionResult rejected =
      iggy3d::rejectCommand(command, iggy3d::CommandRejectionReason::None);
  return ok && expect(rejected.command.admission == iggy3d::CommandAdmissionStatus::Rejected,
                      "rejected status") &&
         expect(rejected.command.rejection == iggy3d::CommandRejectionReason::InternalError,
                "none rejection repaired");
}

bool missingContextRejectsInternalError() {
  AdmissionFixture fixture = makeFirstRoomAdmissionFixture();
  const iggy3d::CommandRecord command = moveCommand({2.0F, 0.0F, 0.0F});
  const iggy3d::CommandAdmissionResult missingWorld =
      iggy3d::admitCommand({nullptr, &fixture.players, &fixture.clock, &fixture.commandLog,
                            &fixture.config},
                           {command});
  const iggy3d::CommandAdmissionResult missingPlayers =
      iggy3d::admitCommand({&fixture.world, nullptr, &fixture.clock, &fixture.commandLog,
                            &fixture.config},
                           {command});
  const iggy3d::CommandAdmissionResult missingConfig =
      iggy3d::admitCommand({&fixture.world, &fixture.players, &fixture.clock, &fixture.commandLog,
                            nullptr},
                           {command});
  return expect(missingWorld.firstFailure == iggy3d::CommandRejectionReason::InternalError,
                "missing world") &&
         expect(missingPlayers.firstFailure == iggy3d::CommandRejectionReason::InternalError,
                "missing players") &&
         expect(missingConfig.firstFailure == iggy3d::CommandRejectionReason::InternalError,
                "missing config");
}

bool shapePlayerActorOrderIsDeterministic() {
  AdmissionFixture fixture = makeFirstRoomAdmissionFixture();
  iggy3d::CommandRecord none = makeCommand(iggy3d::CommandKind::None);
  bool ok = expect(admit(fixture, none).firstFailure ==
                       iggy3d::CommandRejectionReason::InvalidCommand,
                   "none invalid");

  iggy3d::CommandRecord missingPoint = makeCommand(iggy3d::CommandKind::Move);
  ok = ok && expect(admit(fixture, missingPoint).firstFailure ==
                        iggy3d::CommandRejectionReason::InvalidTargetPoint,
                    "move missing point");

  iggy3d::CommandRecord badSlot = interactCommand();
  badSlot.playerSlot = iggy3d::kInvalidPlayerSlotId;
  badSlot.actor = {99};
  ok = ok && expect(admit(fixture, badSlot).firstFailure ==
                        iggy3d::CommandRejectionReason::InvalidPlayerSlot,
                    "slot before actor");

  iggy3d::CommandRecord badActor = interactCommand();
  badActor.actor = {99};
  ok = ok && expect(admit(fixture, badActor).firstFailure ==
                        iggy3d::CommandRejectionReason::InvalidActor,
                    "actor before target");

  iggy3d::CommandRecord mismatch = interactCommand();
  iggy3d::PlayerSlot slot;
  slot.id = 1;
  slot.kind = iggy3d::PlayerSlotKind::Local;
  slot.actor = {2};
  slot.stableName = "player1";
  (void)fixture.players.addSlot(slot);
  mismatch.playerSlot = 1;
  return ok && expect(admit(fixture, mismatch).firstFailure ==
                          iggy3d::CommandRejectionReason::ActorNotControlledBySlot,
                      "actor binding");
}

bool clockRulesAreExplicit() {
  AdmissionFixture fixture = makeFirstRoomAdmissionFixture();
  fixture.clock.mode = iggy3d::ClockMode::Paused;
  bool ok = expect(admit(fixture, moveCommand({2.0F, 0.0F, 0.0F})).firstFailure ==
                       iggy3d::CommandRejectionReason::SessionPaused,
                   "move paused") &&
            expect(admit(fixture, makeCommand(iggy3d::CommandKind::Pause)).command.admission ==
                       iggy3d::CommandAdmissionStatus::Accepted,
                   "pause idempotent");

  iggy3d::CommandRecord step = makeCommand(iggy3d::CommandKind::StepTacticalTick);
  step.actor = {};
  ok = ok && expect(admit(fixture, step).command.admission ==
                        iggy3d::CommandAdmissionStatus::Accepted,
                    "step paused");
  fixture.clock.mode = iggy3d::ClockMode::Normal;
  return ok && expect(admit(fixture, step).firstFailure ==
                          iggy3d::CommandRejectionReason::StepRequiresPaused,
                      "step requires paused");
}

bool targetReachAndPointRules() {
  AdmissionFixture fixture = makeFirstRoomAdmissionFixture();
  bool ok = expect(admit(fixture, interactCommand()).firstFailure ==
                       iggy3d::CommandRejectionReason::OutOfRange,
                   "initial interact out of range");

  iggy3d::CommandRecord marker = interactCommand();
  marker.payload.target.entity = {3};
  ok = ok && expect(admit(fixture, marker).firstFailure ==
                        iggy3d::CommandRejectionReason::InvalidTarget,
                    "marker not interactable");

  (void)fixture.world.setActive({2}, false);
  ok = ok && expect(admit(fixture, interactCommand()).firstFailure ==
                        iggy3d::CommandRejectionReason::TargetInactive,
                    "inactive target");
  (void)fixture.world.setActive({2}, true);

  iggy3d::CommandRecord nanMove =
      moveCommand({std::numeric_limits<float>::quiet_NaN(), 0.0F, 0.0F});
  ok = ok && expect(admit(fixture, nanMove).firstFailure ==
                        iggy3d::CommandRejectionReason::InvalidTargetPoint,
                    "nan point");
  ok = ok && expect(admit(fixture, moveCommand({4.0F, 0.0F, 0.0F})).firstFailure ==
                        iggy3d::CommandRejectionReason::MovementTooFar,
                    "move too far");
  ok = ok && expect(admit(fixture, moveCommand({2.0F, 0.0F, 0.0F})).command.admission ==
                        iggy3d::CommandAdmissionStatus::Accepted,
                    "move accepted");

  (void)fixture.world.updateTransform({1}, transformAt(2.0F, 0.0F, 0.0F));
  return ok && expect(admit(fixture, interactCommand()).command.admission ==
                          iggy3d::CommandAdmissionStatus::Accepted,
                      "interact after move");
}

bool aiCommandsDoNotRequirePlayerSlotBinding() {
  AdmissionFixture fixture = makeFirstRoomAdmissionFixture();
  const iggy3d::CommandAdmissionResult attack = admit(fixture, aiAttackCommand(40));
  const iggy3d::CommandAdmissionResult move =
      admit(fixture, aiMoveCommand({1.5F, 0.0F, 0.0F}, 41));
  return expect(attack.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                "ai attack accepted") &&
         expect(attack.command.playerSlot == iggy3d::kInvalidPlayerSlotId,
                "ai attack keeps invalid slot") &&
         expect(attack.command.source == iggy3d::CommandSource::Ai,
                "ai attack source") &&
         expect(move.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                "ai move accepted") &&
         expect(move.command.playerSlot == iggy3d::kInvalidPlayerSlotId,
                "ai move keeps invalid slot") &&
         expect(move.command.source == iggy3d::CommandSource::Ai, "ai move source");
}

bool localPlayerSlotPolicyIsUnchanged() {
  AdmissionFixture fixture = makeFirstRoomAdmissionFixture();
  iggy3d::CommandRecord badSlot = attackCommand();
  badSlot.playerSlot = iggy3d::kInvalidPlayerSlotId;
  bool ok = expect(admit(fixture, badSlot).firstFailure ==
                       iggy3d::CommandRejectionReason::InvalidPlayerSlot,
                   "local invalid slot rejected");

  iggy3d::CommandRecord mismatch = moveCommand({1.0F, 0.0F, 0.0F});
  iggy3d::PlayerSlot slot;
  slot.id = 1;
  slot.kind = iggy3d::PlayerSlotKind::Local;
  slot.actor = {2};
  slot.stableName = "player1";
  (void)fixture.players.addSlot(slot);
  mismatch.playerSlot = 1;
  return ok && expect(admit(fixture, mismatch).firstFailure ==
                          iggy3d::CommandRejectionReason::ActorNotControlledBySlot,
                      "local actor binding rejected");
}

bool aiCommandsStillUseNormalValidationGates() {
  AdmissionFixture fixture = makeFirstRoomAdmissionFixture();
  (void)fixture.world.setActive({5}, false);
  bool ok = expect(admit(fixture, aiAttackCommand(42)).firstFailure ==
                       iggy3d::CommandRejectionReason::InvalidActor,
                   "ai inactive actor rejected");
  (void)fixture.world.setActive({5}, true);

  iggy3d::CommandRecord missingTarget = aiAttackCommand(43);
  missingTarget.payload.target.entity = {99};
  ok = ok && expect(admit(fixture, missingTarget).firstFailure ==
                        iggy3d::CommandRejectionReason::InvalidTarget,
                    "ai invalid target rejected");

  iggy3d::CommandRecord badDamage = aiAttackCommand(44);
  badDamage.payload.attackDamage = 0;
  ok = ok && expect(admit(fixture, badDamage).firstFailure ==
                        iggy3d::CommandRejectionReason::InvalidDamage,
                    "ai invalid damage rejected");

  AdmissionFixture friendly = makeFirstRoomAdmissionFixture();
  friendly.combat.combatants[1].factionId = 1;
  ok = ok && expect(admit(friendly, aiAttackCommand(45)).firstFailure ==
                        iggy3d::CommandRejectionReason::FriendlyFireBlocked,
                    "ai friendly fire rejected");

  AdmissionFixture defeated = makeFirstRoomAdmissionFixture();
  defeated.combat.combatants[1].hitPoints = 0;
  defeated.combat.combatants[1].defeated = true;
  return ok && expect(admit(defeated, aiAttackCommand(46)).firstFailure ==
                          iggy3d::CommandRejectionReason::AttackerDefeated,
                      "ai defeated attacker rejected");
}

bool requiredItemGateRunsAfterReachAndUsesInventory() {
  AdmissionFixture fixture = makeFirstRoomAdmissionFixture();
  iggy3d::CommandRecord lockedDoor = interactCommand();
  lockedDoor.payload.target.entity = {4};
  bool ok = expect(admit(fixture, lockedDoor).firstFailure ==
                       iggy3d::CommandRejectionReason::OutOfRange,
                   "locked door still obeys reach first");

  (void)fixture.world.updateTransform({1}, transformAt(2.0F, 0.0F, 0.0F));
  ok = ok && expect(admit(fixture, lockedDoor).firstFailure ==
                        iggy3d::CommandRejectionReason::RequiredItemMissing,
                    "locked door missing key rejected");

  const iggy3d::InventoryOperationResult added =
      iggy3d::addItem(fixture.inventory, {0, "gold_key", 1});
  ok = ok && expect(added.status == iggy3d::InventoryStatus::Ok,
                    "key added to inventory");
  ok = ok && expect(admit(fixture, lockedDoor).command.admission ==
                        iggy3d::CommandAdmissionStatus::Accepted,
                    "locked door accepted with key");

  const iggy3d::CommandAdmissionResult missingInventory = iggy3d::admitCommand(
      {&fixture.world, &fixture.players, &fixture.clock, &fixture.commandLog, &fixture.config},
      {lockedDoor});
  return ok && expect(missingInventory.firstFailure ==
                          iggy3d::CommandRejectionReason::InternalError,
                      "required item without inventory is internal error");
}

bool retryUsesCommandIdAndCurrentState() {
  AdmissionFixture fixture = makeFirstRoomAdmissionFixture();
  const iggy3d::CommandAdmissionResult outOfRange = admit(fixture, interactCommand(10));
  bool ok = expect(outOfRange.firstFailure == iggy3d::CommandRejectionReason::OutOfRange,
                   "source rejected");
  ok = ok && expect(fixture.commandLog.append(outOfRange.command).status ==
                        iggy3d::CommandLogAppendStatus::Ok,
                    "append rejected source");

  ok = ok && expect(admit(fixture, retryCommand(10, 11)).firstFailure ==
                        iggy3d::CommandRejectionReason::OutOfRange,
                    "retry still out of range");

  (void)fixture.world.updateTransform({1}, transformAt(2.0F, 0.0F, 0.0F));
  const iggy3d::CommandAdmissionResult retryAccepted = admit(fixture, retryCommand(10, 11));
  ok = ok && expect(retryAccepted.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                    "retry accepted") &&
       expect(retryAccepted.command.kind == iggy3d::CommandKind::Retry, "retry remains retry") &&
       expect(fixture.world.findById({2})->active, "retry did not execute interaction");

  AdmissionFixture acceptedSourceFixture = makeFirstRoomAdmissionFixture();
  const iggy3d::CommandAdmissionResult moveAccepted =
      admit(acceptedSourceFixture, moveCommand({2.0F, 0.0F, 0.0F}, 20));
  ok = ok && expect(acceptedSourceFixture.commandLog.append(moveAccepted.command).status ==
                        iggy3d::CommandLogAppendStatus::Ok,
                    "append accepted source");
  ok = ok && expect(admit(acceptedSourceFixture, retryCommand(20, 21)).firstFailure ==
                        iggy3d::CommandRejectionReason::RetrySourceNotRejected,
                    "accepted source not retryable");

  AdmissionFixture unsupportedFixture = makeFirstRoomAdmissionFixture();
  iggy3d::CommandRecord reset = makeCommand(iggy3d::CommandKind::Reset, 30);
  const iggy3d::CommandRecord rejectedReset =
      iggy3d::rejectCommand(reset, iggy3d::CommandRejectionReason::ResetUnavailable).command;
  ok = ok && expect(unsupportedFixture.commandLog.append(rejectedReset).status ==
                        iggy3d::CommandLogAppendStatus::Ok,
                    "append unsupported source");
  return ok && expect(admit(unsupportedFixture, retryCommand(30, 31)).firstFailure ==
                          iggy3d::CommandRejectionReason::RetryUnsupportedKind,
                      "unsupported source");
}

bool admissionDoesNotMutateState() {
  AdmissionFixture fixture = makeFirstRoomAdmissionFixture();
  const iggy3d::EntityId nextBefore = fixture.world.nextEntityId();
  const std::size_t logSizeBefore = fixture.commandLog.size();
  const iggy3d::ClockMode clockBefore = fixture.clock.mode;
  const iggy3d::CommandAdmissionResult accepted = admit(fixture, moveCommand({2.0F, 0.0F, 0.0F}));
  return expect(accepted.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                "accepted") &&
         expect(fixture.world.nextEntityId() == nextBefore, "world cursor unchanged") &&
         expect(iggy3d::nearlyEqual(fixture.world.findById({1})->transform.position,
                                    {0.0F, 0.0F, 0.0F}),
                "world position unchanged") &&
         expect(fixture.commandLog.size() == logSizeBefore, "log unchanged") &&
         expect(fixture.clock.mode == clockBefore, "clock unchanged");
}

}  // namespace

int main() {
  const bool ok = acceptRejectHelpersSetInvariantFields() && missingContextRejectsInternalError() &&
                  shapePlayerActorOrderIsDeterministic() && clockRulesAreExplicit() &&
                  targetReachAndPointRules() &&
                  aiCommandsDoNotRequirePlayerSlotBinding() &&
                  localPlayerSlotPolicyIsUnchanged() &&
                  aiCommandsStillUseNormalValidationGates() &&
                  requiredItemGateRunsAfterReachAndUsesInventory() &&
                  retryUsesCommandIdAndCurrentState() &&
                  admissionDoesNotMutateState();
  return ok ? 0 : 1;
}
