#include "EnemyMovement.hpp"

namespace dev {

EnemyMovement::EnemyMovement(const TileMap &map, const Collision &collision, MovementEventSink *eventSink, CombatSystem *combatSystem)
    : map_(map)
    , collision_(collision)
    , attacks_(combatSystem)
    , pursuit_(map_, collision_, attacks_)
{
	(void)eventSink;
}

void EnemyMovement::update(std::vector<Enemy> &enemies, Player &target, float deltaSeconds) const
{
	for (Enemy &enemy : enemies) {
		updateEnemy(enemy, target, deltaSeconds);
	}
}

void EnemyMovement::updateEnemy(Enemy &enemy, Player &target, float deltaSeconds) const
{
	if (attacks_.update(enemy, target, deltaSeconds))
		return;

	pursuit_.pursue(enemy, target);
}

} // namespace dev
