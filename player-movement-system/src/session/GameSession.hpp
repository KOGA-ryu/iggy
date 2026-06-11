#pragma once

#include <filesystem>

#include "events/MovementEventSink.hpp"
#include "save/SaveSlotService.hpp"
#include "simulation/SimulationClock.hpp"
#include "simulation/SimulationFrameRunner.hpp"
#include "simulation/SimulationWorld.hpp"
#include "world/Point.hpp"

namespace dev {

enum class GameSessionMode {
	Empty,
	Gameplay,
	Paused,
	Inventory,
};

struct NewGameSettings {
	Point playerStart;
	int playerHitPoints = 20;
};

class GameSession {
public:
	explicit GameSession(std::filesystem::path saveRoot);

	void startNewGame(const NewGameSettings &settings = {});
	[[nodiscard]] bool saveToSlot(SaveSlotId slotId) const;
	[[nodiscard]] bool loadFromSlot(SaveSlotId slotId);
	[[nodiscard]] SimulationFrameEvents update(float rawDeltaSeconds);

	void setMode(GameSessionMode mode);
	[[nodiscard]] GameSessionMode mode() const;
	[[nodiscard]] bool hasActiveWorld() const;

	[[nodiscard]] SimulationWorld &world();
	[[nodiscard]] const SimulationWorld &world() const;
	[[nodiscard]] SimulationClock &clock();
	[[nodiscard]] const SimulationClock &clock() const;
	[[nodiscard]] SaveSlotService &saveSlots();
	[[nodiscard]] const SaveSlotService &saveSlots() const;

private:
	void resetWorldPreservingSinks();
	[[nodiscard]] SimulationFramePolicy framePolicy() const;

	SimulationWorld world_;
	SimulationClock clock_;
	SaveSlotService saveSlots_;
	GameSessionMode mode_ = GameSessionMode::Empty;
};

} // namespace dev
