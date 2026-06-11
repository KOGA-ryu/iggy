#pragma once

#include <vector>

#include "combat/CombatEvent.hpp"
#include "combat/CombatRegistry.hpp"
#include "enemies/Enemy.hpp"
#include "targeting/TargetRegistry.hpp"

namespace dev {

class TargetSynchronizer {
public:
	void syncEnemyTargets(const std::vector<Enemy> &enemies, const CombatRegistry &combat, TargetRegistry &targets) const;
	void removeDefeatedTargets(const std::vector<CombatEvent> &events, TargetRegistry &targets) const;

private:
	[[nodiscard]] bool enemyIsClickable(const Enemy &enemy, const CombatRegistry &combat) const;
};

} // namespace dev
