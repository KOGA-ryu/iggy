#include "RuntimeSessionModeTogglePolicy.hpp"

namespace dev {

GameSessionMode RuntimeSessionModeTogglePolicy::togglePause(GameSessionMode current) const
{
	return current == GameSessionMode::Paused ? GameSessionMode::Gameplay : GameSessionMode::Paused;
}

GameSessionMode RuntimeSessionModeTogglePolicy::toggleInventory(GameSessionMode current) const
{
	return current == GameSessionMode::Inventory ? GameSessionMode::Gameplay : GameSessionMode::Inventory;
}

} // namespace dev
