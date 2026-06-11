#include "GameSession.hpp"

#include "session/NewGameWorldBuilder.hpp"
#include "session/SessionFrameUpdater.hpp"
#include "session/SessionModeChanger.hpp"
#include "session/SessionModePolicy.hpp"
#include "session/SessionWorldSlotLoader.hpp"
#include "session/SessionWorldSlotSaver.hpp"

#include <utility>

namespace dev {

GameSession::GameSession(std::filesystem::path saveRoot)
    : saveSlots_(std::move(saveRoot))
{
}

void GameSession::startNewGame(const NewGameSettings &settings)
{
	MovementEventSink *movementEvents = world_.movementEvents;
	CombatEventSink *combatEvents = world_.combatEvents;
	world_ = NewGameWorldBuilder {}.build(settings, movementEvents, combatEvents);
	clock_.reset();
	mode_ = GameSessionMode::Gameplay;
}

bool GameSession::saveToSlot(SaveSlotId slotId) const
{
	return SessionWorldSlotSaver {}.save(saveSlots_, slotId, world_, mode_);
}

bool GameSession::loadFromSlot(SaveSlotId slotId)
{
	if (!SessionWorldSlotLoader {}.load(saveSlots_, slotId, world_))
		return false;

	clock_.reset();
	mode_ = GameSessionMode::Gameplay;
	return true;
}

SimulationFrameEvents GameSession::update(float rawDeltaSeconds)
{
	return SessionFrameUpdater {}.update(world_, clock_, mode_, rawDeltaSeconds);
}

void GameSession::setMode(GameSessionMode mode)
{
	(void)SessionModeChanger {}.change(mode_, mode);
}

GameSessionMode GameSession::mode() const
{
	return mode_;
}

bool GameSession::hasActiveWorld() const
{
	return SessionModePolicy {}.hasActiveWorld(mode_);
}

SimulationWorld &GameSession::world()
{
	return world_;
}

const SimulationWorld &GameSession::world() const
{
	return world_;
}

SimulationClock &GameSession::clock()
{
	return clock_;
}

const SimulationClock &GameSession::clock() const
{
	return clock_;
}

SaveSlotService &GameSession::saveSlots()
{
	return saveSlots_;
}

const SaveSlotService &GameSession::saveSlots() const
{
	return saveSlots_;
}

} // namespace dev
