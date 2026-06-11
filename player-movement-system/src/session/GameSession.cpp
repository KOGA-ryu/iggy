#include "GameSession.hpp"

#include <utility>

namespace dev {

namespace {

Player MakeSessionPlayer(const NewGameSettings &settings)
{
	Player player;
	player.position.tile = settings.playerStart;
	player.position.future = settings.playerStart;
	player.position.previous = settings.playerStart;
	player.position.precise = settings.playerStart;
	player.combatStats.hitPoints = settings.playerHitPoints;
	return player;
}

} // namespace

GameSession::GameSession(std::filesystem::path saveRoot)
    : saveSlots_(std::move(saveRoot))
{
}

void GameSession::startNewGame(const NewGameSettings &settings)
{
	resetWorldPreservingSinks();
	clock_.reset();
	world_.players.push_back(MakeSessionPlayer(settings));
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
	MovementEventSink *movementEvents = world_.movementEvents;
	CombatEventSink *combatEvents = world_.combatEvents;
	SimulationWorld loadedWorld;
	loadedWorld.movementEvents = movementEvents;
	loadedWorld.setCombatEventSink(combatEvents);
	if (!saveSlots_.loadSlot(slotId, loadedWorld))
		return false;

	world_ = std::move(loadedWorld);
	world_.movementEvents = movementEvents;
	world_.setCombatEventSink(combatEvents);
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
	if (mode == GameSessionMode::Empty || hasActiveWorld())
		mode_ = mode;
}

GameSessionMode GameSession::mode() const
{
	return mode_;
}

bool GameSession::hasActiveWorld() const
{
	return mode_ != GameSessionMode::Empty;
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

void GameSession::resetWorldPreservingSinks()
{
	MovementEventSink *movementEvents = world_.movementEvents;
	CombatEventSink *combatEvents = world_.combatEvents;
	SimulationWorld resetWorld;
	world_ = std::move(resetWorld);
	world_.movementEvents = movementEvents;
	world_.setCombatEventSink(combatEvents);
}

SimulationFramePolicy GameSession::framePolicy() const
{
	switch (mode_) {
	case GameSessionMode::Gameplay:
		return SimulationFramePolicy::forMode(SimulationMode::Gameplay);
	case GameSessionMode::Paused:
		return SimulationFramePolicy::forMode(SimulationMode::Paused);
	case GameSessionMode::Inventory:
		return SimulationFramePolicy::forMode(SimulationMode::Inventory);
	case GameSessionMode::Empty:
		return SimulationFramePolicy::forMode(SimulationMode::Paused);
	}

	return SimulationFramePolicy::forMode(SimulationMode::Paused);
}

} // namespace dev
