#include "GameSession.hpp"

#include "session/NewGameWorldBuilder.hpp"
#include "session/SessionModePolicy.hpp"
#include "session/SessionWorldSlotLoader.hpp"

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
	if (!hasActiveWorld())
		return false;
	return saveSlots_.saveSlot(slotId, world_);
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
	if (!hasActiveWorld())
		return {};

	SimulationFrameRunner runner { &clock_ };
	return runner.run(world_, rawDeltaSeconds, framePolicy());
}

void GameSession::setMode(GameSessionMode mode)
{
	SessionModePolicy policy;
	if (policy.canTransition(mode_, mode))
		mode_ = mode;
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

SimulationFramePolicy GameSession::framePolicy() const
{
	return SessionModePolicy {}.framePolicyFor(mode_);
}

} // namespace dev
