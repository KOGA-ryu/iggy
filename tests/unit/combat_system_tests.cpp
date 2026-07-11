#include "runtime/combat/CombatSystem.hpp"

#include <iostream>

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

iggy3d::CombatState makeCombat() {
  iggy3d::CombatState combat;
  combat.combatants.push_back({iggy3d::EntityId{1}, 1, 10, 10, false});
  combat.combatants.push_back({iggy3d::EntityId{2}, 2, 3, 3, false});
  combat.combatants.push_back({iggy3d::EntityId{3}, 0, 5, 5, false});
  return combat;
}

bool emptyCombatReportsMissingAttacker() {
  iggy3d::CombatState combat;
  const iggy3d::CombatAttackResult result =
      iggy3d::previewAttack(combat, {iggy3d::EntityId{1}, iggy3d::EntityId{2}, 1, 10});
  return expect(result.status == iggy3d::CombatStatus::InvalidAttacker,
                "empty combat missing attacker");
}

bool previewDoesNotMutateAndMatchesApplyProjection() {
  iggy3d::CombatState combat = makeCombat();
  const iggy3d::CombatAttackRequest request{iggy3d::EntityId{1}, iggy3d::EntityId{2}, 2, 7};
  const iggy3d::CombatAttackResult preview = iggy3d::previewAttack(combat, request);
  bool ok = expect(preview.status == iggy3d::CombatStatus::Succeeded, "preview succeeded") &&
            expect(preview.damageApplied == 2, "preview damage") &&
            expect(preview.targetHitPoints == 1, "preview target hp") &&
            expect(!preview.targetDefeated, "preview not defeated") &&
            expect(!preview.combatMutated, "preview not mutated") &&
            expect(combat.combatants[1].hitPoints == 3, "combat unchanged");
  const iggy3d::CombatAttackResult applied = iggy3d::applyAttack(combat, request);
  ok = ok && expect(applied.status == preview.status, "apply status matches") &&
       expect(applied.damageApplied == preview.damageApplied, "apply damage matches") &&
       expect(applied.targetHitPoints == preview.targetHitPoints, "apply hp matches") &&
       expect(applied.combatMutated, "apply mutates") &&
       expect(combat.combatants[1].hitPoints == 1, "target hp changed");
  return ok;
}

bool defeatClampsAndSetsFlag() {
  iggy3d::CombatState combat = makeCombat();
  const iggy3d::CombatAttackResult result =
      iggy3d::applyAttack(combat, {iggy3d::EntityId{1}, iggy3d::EntityId{2}, 9, 8});
  return expect(result.status == iggy3d::CombatStatus::Succeeded, "defeat succeeded") &&
         expect(result.damageApplied == 3, "damage clamped") &&
         expect(result.targetHitPoints == 0, "target hp zero") &&
         expect(result.targetDefeated, "target defeated") &&
         expect(combat.combatants[1].defeated, "state defeated");
}

bool validationOrderAndFailures() {
  bool ok = true;
  iggy3d::CombatState combat = makeCombat();
  combat.combatants.push_back({iggy3d::EntityId{2}, 4, 1, 1, false});
  ok = ok && expect(iggy3d::previewAttack(combat, {iggy3d::EntityId{1}, iggy3d::EntityId{2}, 1, 1})
                        .status == iggy3d::CombatStatus::InvalidCombatState,
                    "duplicate invalid");

  combat = makeCombat();
  combat.combatants[0].maxHitPoints = 0;
  ok = ok && expect(iggy3d::previewAttack(combat, {iggy3d::EntityId{1}, iggy3d::EntityId{2}, 1, 1})
                        .status == iggy3d::CombatStatus::InvalidCombatState,
                    "max hp invalid");

  combat = makeCombat();
  combat.combatants[0].maxHitPoints = -1;
  ok = ok && expect(iggy3d::previewAttack(combat, {iggy3d::EntityId{1}, iggy3d::EntityId{2}, 1, 1})
                        .status == iggy3d::CombatStatus::InvalidCombatState,
                    "negative max hp invalid");

  combat = makeCombat();
  combat.combatants[0].hitPoints = -1;
  ok = ok && expect(iggy3d::previewAttack(combat, {iggy3d::EntityId{1}, iggy3d::EntityId{2}, 1, 1})
                        .status == iggy3d::CombatStatus::InvalidCombatState,
                    "negative hp invalid");

  combat = makeCombat();
  combat.combatants[0].hitPoints = 11;
  ok = ok && expect(iggy3d::previewAttack(combat, {iggy3d::EntityId{1}, iggy3d::EntityId{2}, 1, 1})
                        .status == iggy3d::CombatStatus::InvalidCombatState,
                    "hp over max invalid");

  combat = makeCombat();
  combat.combatants[0].defeated = true;
  ok = ok && expect(iggy3d::previewAttack(combat, {iggy3d::EntityId{1}, iggy3d::EntityId{2}, 1, 1})
                        .status == iggy3d::CombatStatus::InvalidCombatState,
                    "defeated with hp invalid");

  combat = makeCombat();
  combat.combatants[0].hitPoints = 0;
  ok = ok && expect(iggy3d::previewAttack(combat, {iggy3d::EntityId{1}, iggy3d::EntityId{2}, 1, 1})
                        .status == iggy3d::CombatStatus::InvalidCombatState,
                    "zero hp not defeated invalid");

  combat = makeCombat();
  ok = ok && expect(iggy3d::previewAttack(combat, {iggy3d::EntityId{9}, iggy3d::EntityId{2}, 1, 1})
                        .status == iggy3d::CombatStatus::InvalidAttacker,
                    "missing attacker");
  ok = ok && expect(iggy3d::previewAttack(combat, {iggy3d::EntityId{1}, iggy3d::EntityId{9}, 1, 1})
                        .status == iggy3d::CombatStatus::InvalidTarget,
                    "missing target");

  combat = makeCombat();
  combat.combatants[0].hitPoints = 0;
  combat.combatants[0].defeated = true;
  ok = ok && expect(iggy3d::previewAttack(combat, {iggy3d::EntityId{1}, iggy3d::EntityId{2}, 1, 1})
                        .status == iggy3d::CombatStatus::AttackerDefeated,
                    "attacker defeated");

  combat = makeCombat();
  combat.combatants[1].hitPoints = 0;
  combat.combatants[1].defeated = true;
  ok = ok && expect(iggy3d::previewAttack(combat, {iggy3d::EntityId{1}, iggy3d::EntityId{2}, 1, 1})
                        .status == iggy3d::CombatStatus::TargetDefeated,
                    "target defeated");

  combat = makeCombat();
  combat.combatants[1].factionId = 1;
  ok = ok && expect(iggy3d::previewAttack(combat, {iggy3d::EntityId{1}, iggy3d::EntityId{2}, 1, 1})
                        .status == iggy3d::CombatStatus::FriendlyFireBlocked,
                    "friendly fire");

  combat = makeCombat();
  ok = ok && expect(iggy3d::previewAttack(combat, {iggy3d::EntityId{1}, iggy3d::EntityId{3}, 1, 1})
                        .status == iggy3d::CombatStatus::Succeeded,
                    "neutral target allowed");
  ok = ok && expect(iggy3d::previewAttack(combat, {iggy3d::EntityId{1}, iggy3d::EntityId{2}, 0, 1})
                        .status == iggy3d::CombatStatus::InvalidDamage,
                    "invalid damage");
  return ok;
}

}  // namespace

int main() {
  bool ok = true;
  ok = emptyCombatReportsMissingAttacker() && ok;
  ok = previewDoesNotMutateAndMatchesApplyProjection() && ok;
  ok = defeatClampsAndSetsFlag() && ok;
  ok = validationOrderAndFailures() && ok;
  return ok ? 0 : 1;
}
