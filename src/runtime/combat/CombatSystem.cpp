#include "runtime/combat/CombatSystem.hpp"

#include <algorithm>

namespace iggy3d {

namespace {

struct ValidationResult {
  CombatStatus status = CombatStatus::Succeeded;
  std::size_t attackerIndex = 0;
  std::size_t targetIndex = 0;
};

bool combatantInvariantValid(const CombatantState& combatant) {
  return isValid(combatant.entity) && combatant.maxHitPoints > 0 &&
         combatant.hitPoints >= 0 && combatant.hitPoints <= combatant.maxHitPoints &&
         combatant.defeated == (combatant.hitPoints == 0);
}

ValidationResult validateAttack(const CombatState& combat, const CombatAttackRequest& request) {
  for (std::size_t outer = 0; outer < combat.combatants.size(); ++outer) {
    if (!combatantInvariantValid(combat.combatants[outer])) {
      return {CombatStatus::InvalidCombatState, 0, 0};
    }
    for (std::size_t inner = outer + 1U; inner < combat.combatants.size(); ++inner) {
      if (combat.combatants[outer].entity == combat.combatants[inner].entity) {
        return {CombatStatus::InvalidCombatState, 0, 0};
      }
    }
  }

  const auto attackerIt = std::find_if(
      combat.combatants.begin(), combat.combatants.end(),
      [&](const CombatantState& combatant) { return combatant.entity == request.attacker; });
  if (attackerIt == combat.combatants.end()) {
    return {CombatStatus::InvalidAttacker, 0, 0};
  }

  const auto targetIt = std::find_if(
      combat.combatants.begin(), combat.combatants.end(),
      [&](const CombatantState& combatant) { return combatant.entity == request.target; });
  if (targetIt == combat.combatants.end()) {
    return {CombatStatus::InvalidTarget, 0, 0};
  }

  const std::size_t attackerIndex =
      static_cast<std::size_t>(std::distance(combat.combatants.begin(), attackerIt));
  const std::size_t targetIndex =
      static_cast<std::size_t>(std::distance(combat.combatants.begin(), targetIt));
  const CombatantState& attacker = combat.combatants[attackerIndex];
  const CombatantState& target = combat.combatants[targetIndex];

  if (attacker.defeated) {
    return {CombatStatus::AttackerDefeated, attackerIndex, targetIndex};
  }
  if (target.defeated) {
    return {CombatStatus::TargetDefeated, attackerIndex, targetIndex};
  }
  if (attacker.factionId != 0U && attacker.factionId == target.factionId) {
    return {CombatStatus::FriendlyFireBlocked, attackerIndex, targetIndex};
  }
  if (request.damage <= 0) {
    return {CombatStatus::InvalidDamage, attackerIndex, targetIndex};
  }

  return {CombatStatus::Succeeded, attackerIndex, targetIndex};
}

CombatAttackResult makeResult(const CombatState& combat,
                              const CombatAttackRequest& request,
                              const ValidationResult& validation) {
  CombatAttackResult result;
  result.status = validation.status;
  result.attacker = request.attacker;
  result.target = request.target;
  if (validation.targetIndex < combat.combatants.size()) {
    const CombatantState& target = combat.combatants[validation.targetIndex];
    result.targetHitPoints = target.hitPoints;
    result.targetDefeated = target.defeated;
  }
  if (validation.status == CombatStatus::Succeeded) {
    const CombatantState& target = combat.combatants[validation.targetIndex];
    result.damageApplied = std::min(request.damage, target.hitPoints);
    result.targetHitPoints = target.hitPoints - result.damageApplied;
    result.targetDefeated = result.targetHitPoints == 0;
  }
  return result;
}

}  // namespace

CombatAttackResult previewAttack(const CombatState& combat, const CombatAttackRequest& request) {
  const ValidationResult validation = validateAttack(combat, request);
  return makeResult(combat, request, validation);
}

CombatAttackResult applyAttack(CombatState& combat, const CombatAttackRequest& request) {
  const ValidationResult validation = validateAttack(combat, request);
  CombatAttackResult result = makeResult(combat, request, validation);
  if (validation.status != CombatStatus::Succeeded) {
    return result;
  }

  CombatantState& target = combat.combatants[validation.targetIndex];
  target.hitPoints = result.targetHitPoints;
  target.defeated = result.targetDefeated;
  result.combatMutated = true;
  return result;
}

}  // namespace iggy3d
