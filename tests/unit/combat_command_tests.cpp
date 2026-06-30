#include "content/PackageLoader.hpp"
#include "runtime/session/Session.hpp"
#include "runtime/session/SessionRunner.hpp"

#include <iostream>

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

iggy3d::Session createSession() {
  const iggy3d::PackageLoadResult package =
      iggy3d::loadPackage({"fixtures/demos/first_room/package.iggy3d.toml"});
  iggy3d::SessionCreateRequest create;
  create.packageId = package.manifest.packageId;
  create.config = package.scenario.config;
  create.seed = package.scenario;
  return iggy3d::Session::create(create).value;
}

iggy3d::CommandRecord attackCommand(iggy3d::CommandId id, std::int32_t damage) {
  iggy3d::CommandRecord command;
  command.commandId = id;
  command.playerSlot = 0;
  command.actor = iggy3d::EntityId{1};
  command.kind = iggy3d::CommandKind::Attack;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.target.hasEntity = true;
  command.payload.target.entity = iggy3d::EntityId{4};
  command.payload.attackDamage = damage;
  return command;
}

iggy3d::CommandRecord moveCommand(iggy3d::Vec3 point) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = iggy3d::EntityId{1};
  command.kind = iggy3d::CommandKind::Move;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.target.hasPoint = true;
  command.payload.target.point = point;
  return command;
}

const iggy3d::CombatantState* dummyCombatant(const iggy3d::Session& session) {
  const iggy3d::EntityState* dummy = session.state().world.findByStableName("training_dummy");
  if (dummy == nullptr) {
    return nullptr;
  }
  for (const iggy3d::CombatantState& combatant : session.state().combat.combatants) {
    if (combatant.entity == dummy->id) {
      return &combatant;
    }
  }
  return nullptr;
}

bool runOne(iggy3d::Session& session) {
  const iggy3d::SessionRunnerRunResult run =
      iggy3d::runSession({&session, 8, true, true});
  return run.status == iggy3d::SessionRunnerStatus::Advanced && run.ticksAdvanced == 1U;
}

bool moveIntoAttackRange(iggy3d::Session& session) {
  const iggy3d::SessionCommandResult move = session.submitCommand(moveCommand({2.0F, 0.0F, 1.0F}));
  return move.command.admission == iggy3d::CommandAdmissionStatus::Accepted && runOne(session);
}

bool shapeHelpersAndInvalidDamageLogging() {
  bool ok = expect(iggy3d::requiresActor(iggy3d::CommandKind::Attack), "attack actor") &&
            expect(iggy3d::requiresEntityTarget(iggy3d::CommandKind::Attack), "attack target") &&
            expect(!iggy3d::requiresPointTarget(iggy3d::CommandKind::Attack), "attack no point");

  iggy3d::Session session = createSession();
  ok = ok && expect(moveIntoAttackRange(session), "move into attack range");
  iggy3d::SessionCommandResult invalid = session.submitCommand(attackCommand(0, 0));
  ok = ok && expect(invalid.appendedToLog, "invalid damage logged") &&
       expect(invalid.command.admission == iggy3d::CommandAdmissionStatus::Rejected,
              "invalid damage rejected") &&
       expect(invalid.command.rejection == iggy3d::CommandRejectionReason::InvalidDamage,
              "invalid damage reason") &&
       expect(session.state().commandLog.counts().combat == 1U, "combat count rejected") &&
       expect(session.state().transient.pendingExecutionSequences.empty(), "rejected not queued");
  return ok;
}

bool acceptedAttackMutatesOnlyCombat() {
  iggy3d::Session session = createSession();
  const iggy3d::StateHashValue before = session.stateHash();
  bool ok = expect(moveIntoAttackRange(session), "move accepted and ticked");
  const iggy3d::EntityState* dummy = session.state().world.findByStableName("training_dummy");
  const iggy3d::SessionCommandResult attack = session.submitCommand(attackCommand(0, 3));
  // The move (id/seq 1) is followed by one NPC behavior command enqueued during
  // moveIntoAttackRange's tick (id/seq 2), so the player attack is id/seq 3.
  ok = ok && expect(attack.command.commandId == 3U && attack.command.sequence == 3U,
                    "attack identity") &&
       expect(attack.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
              "attack accepted") &&
       expect(attack.command.payload.attackDamage == 3, "attack damage preserved") &&
       expect(session.state().transient.pendingExecutionSequences.size() == 1U, "attack queued") &&
       expect(runOne(session), "attack tick");
  const iggy3d::CombatantState* combatant = dummyCombatant(session);
  // NPC actors now also issue combat commands during ticks (the dummy's
  // counter-attack is logged-and-rejected), so the aggregate combat counter is
  // 2 here. Assert on the player's own accepted attack instead of the total.
  std::size_t playerAttacksAccepted = 0U;
  for (const iggy3d::CommandRecord& record : session.state().commandLog.records()) {
    if (record.source == iggy3d::CommandSource::LocalPlayer &&
        record.kind == iggy3d::CommandKind::Attack &&
        record.admission == iggy3d::CommandAdmissionStatus::Accepted) {
      ++playerAttacksAccepted;
    }
  }
  ok = ok && expect(combatant != nullptr, "dummy combatant") &&
       expect(combatant->hitPoints == 0, "dummy hp zero") &&
       expect(combatant->defeated, "dummy defeated") &&
       expect(dummy != nullptr && dummy->active, "dummy remains active") &&
       expect(playerAttacksAccepted == 1U, "combat count accepted") &&
       expect(session.stateHash() != before, "hash changed");
  return ok;
}

bool combatAdmissionFailures() {
  bool ok = true;
  iggy3d::Session session = createSession();
  iggy3d::CommandRecord command = attackCommand(0, 1);
  command.payload.target.entity = iggy3d::EntityId{2};
  iggy3d::SessionCommandResult unsupported = session.submitCommand(command);
  ok = ok && expect(unsupported.command.rejection == iggy3d::CommandRejectionReason::InvalidTarget,
                    "target action required");

  session = createSession();
  command = attackCommand(0, 1);
  command.payload.target.entity = iggy3d::EntityId{99};
  iggy3d::SessionCommandResult missingTarget = session.submitCommand(command);
  ok = ok && expect(missingTarget.command.rejection == iggy3d::CommandRejectionReason::InvalidTarget,
                    "missing target");

  session = createSession();
  command = attackCommand(0, 1);
  command.actor = iggy3d::EntityId{99};
  iggy3d::SessionCommandResult missingActor = session.submitCommand(command);
  ok = ok && expect(missingActor.command.rejection == iggy3d::CommandRejectionReason::InvalidActor,
                    "missing actor");

  session = createSession();
  ok = ok && expect(moveIntoAttackRange(session), "friendly setup move");
  session.mutableStateForOwnedSystems().combat.combatants[1].factionId = 1;
  command = attackCommand(0, 1);
  iggy3d::SessionCommandResult friendly = session.submitCommand(command);
  ok = ok && expect(friendly.command.rejection == iggy3d::CommandRejectionReason::FriendlyFireBlocked,
                    "friendly fire blocked");

  session = createSession();
  ok = ok && expect(moveIntoAttackRange(session), "defeated attacker setup move");
  session.mutableStateForOwnedSystems().combat.combatants[0].hitPoints = 0;
  session.mutableStateForOwnedSystems().combat.combatants[0].defeated = true;
  command = attackCommand(0, 1);
  iggy3d::SessionCommandResult defeatedAttacker = session.submitCommand(command);
  ok = ok && expect(defeatedAttacker.command.rejection ==
                        iggy3d::CommandRejectionReason::AttackerDefeated,
                    "defeated attacker");
  return ok;
}

}  // namespace

int main() {
  bool ok = true;
  ok = shapeHelpersAndInvalidDamageLogging() && ok;
  ok = acceptedAttackMutatesOnlyCombat() && ok;
  ok = combatAdmissionFailures() && ok;
  return ok ? 0 : 1;
}
