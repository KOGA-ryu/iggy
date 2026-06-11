#include "TargetSynchronizer.hpp"

namespace dev {

void TargetSynchronizer::syncEnemyTargets(const std::vector<Enemy> &enemies, const CombatRegistry &combat, TargetRegistry &targets) const
{
	(void)targets.removeAll(TargetType::Enemy);

	for (const Enemy &enemy : enemies) {
		if (!enemyIsClickable(enemy, combat))
			continue;
		targets.add({
		    .type = TargetType::Enemy,
		    .id = enemy.id,
		    .tile = enemy.position.tile,
		});
	}
}

void TargetSynchronizer::removeDefeatedTargets(const std::vector<CombatEvent> &events, TargetRegistry &targets) const
{
	for (const CombatEvent &event : events) {
		if (event.type != CombatEventType::Defeated)
			continue;
		(void)targets.remove(event.target.type, event.target.id);
	}
}

bool TargetSynchronizer::enemyIsClickable(const Enemy &enemy, const CombatRegistry &combat) const
{
	if (!enemy.combatStats.alive())
		return false;

	const Target target {
		.type = TargetType::Enemy,
		.id = enemy.id,
		.tile = enemy.position.tile,
	};
	const Combatant *combatant = combat.find(target);
	return combatant == nullptr || combatant->stats.alive();
}

} // namespace dev
