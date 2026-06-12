#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string_view>

#include "combat/CombatEventRecorder.hpp"
#include "events/EventRecorder.hpp"
#include "player/Player.hpp"
#include "save/SaveSlotService.hpp"
#include "session/GameSession.hpp"
#include "session/NewGameWorldBuilder.hpp"
#include "session/SessionFrameUpdater.hpp"
#include "session/SessionModeChanger.hpp"
#include "session/SessionModePolicy.hpp"
#include "session/SessionWorldSlotLoader.hpp"
#include "session/SessionWorldSlotSaver.hpp"
#include "simulation/SimulationClock.hpp"
#include "simulation/SimulationFrameEvents.hpp"
#include "simulation/SimulationFramePolicy.hpp"
#include "simulation/SimulationWorld.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
}

dev::Player MakePlayer(dev::Point tile = { 0, 0 })
{
	dev::Player player;
	player.position.tile = tile;
	player.position.future = tile;
	player.position.previous = tile;
	player.position.precise = tile;
	return player;
}

void TestGameSessionStartsNewGameAndUpdates()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_session_new_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root };
	session.startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 16 });
	session.world().commandQueue.push({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	    .destinationAction = std::nullopt,
	});

	dev::SimulationFrameEvents frame = session.update(0.016F);

	Expect(session.mode() == dev::GameSessionMode::Gameplay, "new game should enter gameplay mode");
	Expect(session.world().players.size() == 1 && session.world().players[0].combatStats.hitPoints == 16, "new game should apply player settings");
	Expect(session.world().players.size() == 1 && session.world().players[0].position.tile == dev::Point { 1, 0 }, "game session update should advance world");
	Expect(!frame.movementEvents().empty(), "game session update should return frame events");

	std::filesystem::remove_all(root);
}

void TestNewGameWorldBuilderCreatesPlayerWorld()
{
	dev::EventRecorder movementEvents;
	dev::CombatEventRecorder combatEvents;

	dev::SimulationWorld world = dev::NewGameWorldBuilder {}.build(
	    { .playerStart = { 7, 3 }, .playerHitPoints = 24 },
	    &movementEvents,
	    &combatEvents);

	Expect(world.players.size() == 1, "new game world builder should create one starting player");
	Expect(world.players.size() == 1 && world.players[0].position.tile == dev::Point { 7, 3 }, "new game world builder should set player tile");
	Expect(world.players.size() == 1 && world.players[0].position.future == dev::Point { 7, 3 }, "new game world builder should initialize future tile");
	Expect(world.players.size() == 1 && world.players[0].position.previous == dev::Point { 7, 3 }, "new game world builder should initialize previous tile");
	Expect(world.players.size() == 1 && world.players[0].combatStats.hitPoints == 24, "new game world builder should apply starting hit points");
	Expect(world.movementEvents == &movementEvents, "new game world builder should preserve movement sink");
	Expect(world.combatEvents == &combatEvents, "new game world builder should preserve combat sink");
}

void TestSessionWorldSlotLoaderLoadsWorldPreservingSinks()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_world_slot_loader_test";
	std::filesystem::remove_all(root);

	dev::SimulationWorld saved;
	saved.players.push_back(MakePlayer({ 8, 8 }));
	saved.players[0].combatStats.hitPoints = 13;
	dev::SaveSlotService slots { root };
	Expect(slots.saveSlot(1, saved), "session world slot loader setup should save world");

	dev::EventRecorder movementEvents;
	dev::CombatEventRecorder combatEvents;
	dev::SimulationWorld current;
	current.movementEvents = &movementEvents;
	current.setCombatEventSink(&combatEvents);
	current.players.push_back(MakePlayer({ 1, 1 }));

	Expect(dev::SessionWorldSlotLoader {}.load(slots, 1, current), "session world slot loader should load saved world");
	Expect(current.players.size() == 1 && current.players[0].position.tile == dev::Point { 8, 8 }, "session world slot loader should replace world on success");
	Expect(current.players.size() == 1 && current.players[0].combatStats.hitPoints == 13, "session world slot loader should restore saved player state");
	Expect(current.movementEvents == &movementEvents && current.combatEvents == &combatEvents, "session world slot loader should preserve event sinks");

	current.players[0].position.tile = { 2, 2 };
	Expect(!dev::SessionWorldSlotLoader {}.load(slots, 99, current), "session world slot loader should reject missing slots");
	Expect(current.players.size() == 1 && current.players[0].position.tile == dev::Point { 2, 2 }, "session world slot loader should preserve current world on failed load");
	Expect(current.movementEvents == &movementEvents && current.combatEvents == &combatEvents, "session world slot loader should preserve sinks on failed load");

	std::filesystem::remove_all(root);
}

void TestSessionWorldSlotSaverRequiresActiveSession()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_world_slot_saver_test";
	std::filesystem::remove_all(root);

	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 6, 6 }));
	world.players[0].combatStats.hitPoints = 11;
	dev::SaveSlotService slots { root };

	dev::SessionWorldSlotSaver saver;
	Expect(!saver.save(slots, 1, world, dev::GameSessionMode::Empty), "session world slot saver should reject empty sessions");
	Expect(!slots.metadataForSlot(1).occupied, "rejected session save should not create a slot file");
	Expect(saver.save(slots, 2, world, dev::GameSessionMode::Inventory), "session world slot saver should save active inventory sessions");

	dev::SimulationWorld loaded;
	Expect(slots.loadSlot(2, loaded), "session world slot saver should write loadable slot data");
	Expect(loaded.players.size() == 1 && loaded.players[0].position.tile == dev::Point { 6, 6 }, "session world slot saver should save player position");
	Expect(loaded.players.size() == 1 && loaded.players[0].combatStats.hitPoints == 11, "session world slot saver should save player hp");

	std::filesystem::remove_all(root);
}

void TestSessionFrameUpdaterAppliesModePolicy()
{
	dev::SimulationWorld world;
	dev::SimulationClock clock;
	world.players.push_back(MakePlayer({ 0, 0 }));
	world.commandQueue.push({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	    .destinationAction = std::nullopt,
	});

	dev::SessionFrameUpdater updater;
	dev::SimulationFrameEvents emptyFrame = updater.update(world, clock, dev::GameSessionMode::Empty, 0.016F);
	Expect(world.players[0].position.tile == dev::Point { 0, 0 }, "session frame updater should not run empty sessions");
	Expect(emptyFrame.movementEvents().empty(), "session frame updater should not emit empty-session frame events");

	dev::SimulationFrameEvents inventoryFrame = updater.update(world, clock, dev::GameSessionMode::Inventory, 0.016F);
	Expect(world.players[0].position.tile == dev::Point { 0, 0 }, "session frame updater should freeze inventory sessions");
	Expect(inventoryFrame.movementEvents().empty(), "session frame updater should not drain movement in inventory mode");

	dev::SimulationFrameEvents gameplayFrame = updater.update(world, clock, dev::GameSessionMode::Gameplay, 0.016F);
	Expect(world.players[0].position.tile == dev::Point { 1, 0 }, "session frame updater should advance gameplay sessions");
	Expect(!gameplayFrame.movementEvents().empty(), "session frame updater should return gameplay frame events");
}

void TestGameSessionPausedModePreservesCommands()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_session_pause_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root };
	session.startNewGame({ .playerStart = { 0, 0 } });
	session.world().commandQueue.push({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	    .destinationAction = std::nullopt,
	});

	session.setMode(dev::GameSessionMode::Paused);
	(void)session.update(0.016F);
	Expect(session.world().players[0].position.tile == dev::Point { 0, 0 }, "paused session should not drain or move command");

	session.setMode(dev::GameSessionMode::Gameplay);
	(void)session.update(0.016F);
	Expect(session.world().players[0].position.tile == dev::Point { 1, 0 }, "gameplay session should resume preserved command");

	std::filesystem::remove_all(root);
}

void TestSessionModePolicyMapsModesToFramePolicy()
{
	dev::SessionModePolicy policy;

	dev::SimulationFramePolicy gameplay = policy.framePolicyFor(dev::GameSessionMode::Gameplay);
	dev::SimulationFramePolicy paused = policy.framePolicyFor(dev::GameSessionMode::Paused);
	dev::SimulationFramePolicy inventory = policy.framePolicyFor(dev::GameSessionMode::Inventory);
	dev::SimulationFramePolicy empty = policy.framePolicyFor(dev::GameSessionMode::Empty);

	Expect(gameplay.acceptCommands && gameplay.updatePlayers && gameplay.updateEnemies, "session mode policy should let gameplay update actors");
	Expect(!paused.acceptCommands && !paused.updatePlayers && !paused.updateEnemies, "session mode policy should freeze paused sessions");
	Expect(!inventory.acceptCommands && !inventory.updatePlayers && !inventory.updateEnemies, "session mode policy should freeze inventory sessions");
	Expect(!empty.acceptCommands && !empty.updatePlayers && !empty.updateEnemies, "session mode policy should freeze empty sessions");
	Expect(policy.simulationModeFor(dev::GameSessionMode::Gameplay) == dev::SimulationMode::Gameplay, "session mode policy should expose gameplay simulation mode");
	Expect(policy.simulationModeFor(dev::GameSessionMode::Paused) == dev::SimulationMode::Paused, "session mode policy should expose paused simulation mode");
	Expect(policy.simulationModeFor(dev::GameSessionMode::Inventory) == dev::SimulationMode::Inventory, "session mode policy should expose inventory simulation mode");
	Expect(policy.simulationModeFor(dev::GameSessionMode::Empty) == dev::SimulationMode::Paused, "session mode policy should describe empty sessions with paused frame policy");
}

void TestSessionModePolicyGuardsTransitions()
{
	dev::SessionModePolicy policy;

	Expect(policy.hasActiveWorld(dev::GameSessionMode::Gameplay), "session mode policy should treat gameplay as active");
	Expect(!policy.hasActiveWorld(dev::GameSessionMode::Empty), "session mode policy should treat empty as inactive");
	Expect(!policy.canTransition(dev::GameSessionMode::Empty, dev::GameSessionMode::Gameplay), "session mode policy should reject activating an empty session through mode change");
	Expect(policy.canTransition(dev::GameSessionMode::Gameplay, dev::GameSessionMode::Inventory), "session mode policy should allow active session mode changes");
	Expect(policy.canTransition(dev::GameSessionMode::Inventory, dev::GameSessionMode::Empty), "session mode policy should allow returning to empty mode");
}

void TestSessionModeChangerAppliesAllowedTransitionsOnly()
{
	dev::SessionModeChanger changer;
	dev::GameSessionMode empty = dev::GameSessionMode::Empty;
	dev::GameSessionMode active = dev::GameSessionMode::Gameplay;

	Expect(!changer.change(empty, dev::GameSessionMode::Inventory), "session mode changer should reject activating empty sessions");
	Expect(empty == dev::GameSessionMode::Empty, "session mode changer should preserve mode after rejected transition");
	Expect(changer.change(active, dev::GameSessionMode::Inventory), "session mode changer should allow active session mode changes");
	Expect(active == dev::GameSessionMode::Inventory, "session mode changer should apply allowed active transition");
	Expect(changer.change(active, dev::GameSessionMode::Empty), "session mode changer should allow returning to empty mode");
	Expect(active == dev::GameSessionMode::Empty, "session mode changer should apply empty transition");
}

void TestGameSessionSaveLoadPreservesSinksAndResetsClock()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_session_save_load_test";
	std::filesystem::remove_all(root);

	dev::EventRecorder movementEvents;
	dev::CombatEventRecorder combatEvents;
	dev::GameSession session { root };
	session.world().movementEvents = &movementEvents;
	session.world().setCombatEventSink(&combatEvents);
	session.startNewGame({ .playerStart = { 4, 4 }, .playerHitPoints = 18 });
	session.clock().setTimeScale(0.25F);
	session.clock().triggerHitStop(1.0F);
	Expect(session.saveToSlot(1), "game session should save active world");

	session.world().players[0].position.tile = { 9, 9 };
	session.world().players[0].combatStats.hitPoints = 1;
	Expect(session.loadFromSlot(1), "game session should load saved world");

	Expect(session.world().players[0].position.tile == dev::Point { 4, 4 }, "game session load should restore player position");
	Expect(session.world().players[0].combatStats.hitPoints == 18, "game session load should restore player hp");
	Expect(session.world().movementEvents == &movementEvents && session.world().combatEvents == &combatEvents, "game session load should preserve event sinks");
	Expect(session.clock().timeScale() == 1.0F && session.clock().hitStopRemainingSeconds() == 0.0F, "game session load should reset transient clock state");

	std::filesystem::remove_all(root);
}

void TestGameSessionMissingLoadKeepsCurrentWorld()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_session_missing_load_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root };
	session.startNewGame({ .playerStart = { 5, 5 }, .playerHitPoints = 12 });

	Expect(!session.loadFromSlot(99), "game session should reject missing slot load");
	Expect(session.hasActiveWorld(), "failed load should keep current active world");
	Expect(session.world().players.size() == 1 && session.world().players[0].position.tile == dev::Point { 5, 5 }, "failed load should preserve player position");
	Expect(session.world().players.size() == 1 && session.world().players[0].combatStats.hitPoints == 12, "failed load should preserve player hp");

	std::filesystem::remove_all(root);
}

} // namespace

int main()
{
	TestGameSessionStartsNewGameAndUpdates();
	TestNewGameWorldBuilderCreatesPlayerWorld();
	TestSessionWorldSlotLoaderLoadsWorldPreservingSinks();
	TestSessionWorldSlotSaverRequiresActiveSession();
	TestSessionFrameUpdaterAppliesModePolicy();
	TestGameSessionPausedModePreservesCommands();
	TestSessionModePolicyMapsModesToFramePolicy();
	TestSessionModePolicyGuardsTransitions();
	TestSessionModeChangerAppliesAllowedTransitionsOnly();
	TestGameSessionSaveLoadPreservesSinksAndResetsClock();
	TestGameSessionMissingLoadKeepsCurrentWorld();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
