#pragma once

#include <cstdint>

#include "core/ids/EntityId.hpp"
#include "runtime/combat/CombatState.hpp"
#include "runtime/command/Command.hpp"

namespace iggy3d {

enum class CombatStatus : std::uint8_t {
  Succeeded,
  InvalidCombatState,
  InvalidAttacker,
  InvalidTarget,
  AttackerDefeated,
  TargetDefeated,
  FriendlyFireBlocked,
  InvalidDamage,
};

struct CombatAttackRequest {
  EntityId attacker;
  EntityId target;
  std::int32_t damage = 0;
  CommandId sourceCommandId = kInvalidCommandId;
};

struct CombatAttackResult {
  CombatStatus status = CombatStatus::InvalidCombatState;
  EntityId attacker;
  EntityId target;
  std::int32_t damageApplied = 0;
  std::int32_t targetHitPoints = 0;
  bool targetDefeated = false;
  bool combatMutated = false;
};

CombatAttackResult previewAttack(const CombatState& combat, const CombatAttackRequest& request);
CombatAttackResult applyAttack(CombatState& combat, const CombatAttackRequest& request);

}  // namespace iggy3d
