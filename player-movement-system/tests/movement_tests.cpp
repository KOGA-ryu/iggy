#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "actions/ActionExecutor.hpp"
#include "app/GameLoop.hpp"
#include "app/RuntimeDebugArtifactBundle.hpp"
#include "app/RuntimeExitCodePolicy.hpp"
#include "app/RuntimeFrameRunner.hpp"
#include "app/RuntimeInputContextBuilder.hpp"
#include "app/RuntimeInputRouter.hpp"
#include "app/RuntimeFrameTrace.hpp"
#include "app/RuntimeFrameTraceFileStore.hpp"
#include "app/RuntimeOutputFinalizer.hpp"
#include "app/RuntimeRawInputDrainer.hpp"
#include "app/RuntimeRunFinalizer.hpp"
#include "app/RuntimeRunRecorder.hpp"
#include "app/RuntimeSetupRunner.hpp"
#include "app/RuntimeSourceDrainer.hpp"
#include "app/RuntimeSourceDrainerSettingsBuilder.hpp"
#include "app/RuntimeTraceService.hpp"
#include "combat/CombatEventRecorder.hpp"
#include "combat/CombatResolver.hpp"
#include "combat/CombatSystem.hpp"
#include "commands/CommandDispatcher.hpp"
#include "effects/EffectApplier.hpp"
#include "effects/EffectRecorder.hpp"
#include "effects/EffectRouter.hpp"
#include "enemies/EnemyMovement.hpp"
#include "events/EventRecorder.hpp"
#include "focus/InputFocus.hpp"
#include "interaction/InteractionCommandBuilder.hpp"
#include "interaction/InteractionIntentBuilder.hpp"
#include "inventory/InventoryCommandCodec.hpp"
#include "inventory/InventoryCommandDispatcher.hpp"
#include "inventory/InventoryCommandLog.hpp"
#include "inventory/InventoryCommandLogCodec.hpp"
#include "inventory/InventoryCommandLogFileStore.hpp"
#include "inventory/InventoryCommandReplayer.hpp"
#include "inventory/InventoryCommandSource.hpp"
#include "inventory/InventoryEventRecorder.hpp"
#include "inventory/InventoryScriptRunner.hpp"
#include "inventory/InventoryScriptSource.hpp"
#include "inventory/EquipmentService.hpp"
#include "inventory/EquipmentStatsService.hpp"
#include "inventory/InventoryService.hpp"
#include "input/RawInputSource.hpp"
#include "network/MovementCodec.hpp"
#include "player/PlayerActionGate.hpp"
#include "player/PlayerController.hpp"
#include "player/PlayerMovement.hpp"
#include "replay/CommandLog.hpp"
#include "replay/CommandReplayer.hpp"
#include "save/SaveGameService.hpp"
#include "save/SaveSlotService.hpp"
#include "save/SnapshotCodec.hpp"
#include "save/SnapshotFileStore.hpp"
#include "save/SnapshotReader.hpp"
#include "save/SnapshotWriter.hpp"
#include "session/GameSession.hpp"
#include "session/SessionCommandCodec.hpp"
#include "session/SessionCommandDispatcher.hpp"
#include "session/SessionCommandLog.hpp"
#include "session/SessionCommandLogCodec.hpp"
#include "session/SessionCommandLogFileStore.hpp"
#include "session/SessionCommandReplayer.hpp"
#include "session/SessionEventRecorder.hpp"
#include "session/SessionScriptRunner.hpp"
#include "simulation/SimulationClock.hpp"
#include "simulation/SimulationCommandDrainer.hpp"
#include "simulation/SimulationEffectPipeline.hpp"
#include "simulation/SimulationFrameEventCapture.hpp"
#include "simulation/SimulationFrameFinalizer.hpp"
#include "simulation/SimulationFrameRunner.hpp"
#include "simulation/SimulationPlayerUpdater.hpp"
#include "simulation/SimulationTick.hpp"
#include "simulation/WorldEntityService.hpp"
#include "targeting/Target.hpp"
#include "targeting/TargetRegistry.hpp"
#include "targeting/TargetSynchronizer.hpp"
#include "world/Collision.hpp"
#include "world/PathFinder.hpp"
#include "world/TileMap.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
}

bool ContainsLineFragment(const std::vector<std::string> &lines, std::string_view fragment)
{
	for (const std::string &line : lines) {
		if (line.find(fragment) != std::string::npos)
			return true;
	}
	return false;
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

dev::Enemy MakeEnemy(dev::Point tile)
{
	dev::Enemy enemy;
	enemy.id = 1;
	enemy.position.tile = tile;
	enemy.position.future = tile;
	enemy.position.previous = tile;
	enemy.position.precise = tile;
	return enemy;
}

class FixedTargetResolver : public dev::TargetResolver {
public:
	explicit FixedTargetResolver(dev::Target target)
	    : target_(target)
	{
	}

	dev::Target resolveAtTile(dev::Point tile) const override
	{
		dev::Target target = target_;
		target.tile = tile;
		return target;
	}

private:
	dev::Target target_;
};

void TestInventoryFocusBlocksMovement()
{
	dev::FocusState focusState { .owner = dev::InputOwner::Inventory };
	dev::InputFocus focus { focusState };
	dev::PlayerActionContext context;
	dev::PlayerActionGate gate { focus, context };
	dev::Player player = MakePlayer();

	Expect(!gate.canMove(player), "inventory focus should block movement");
}

void TestStandGroundCreatesStandAndAct()
{
	dev::FocusState focusState;
	dev::InputFocus focus { focusState };
	dev::PlayerActionContext context;
	dev::PlayerActionGate gate { focus, context };
	dev::Player player = MakePlayer();
	player.movementModifiers.standGround = true;

	dev::Target target { .type = dev::TargetType::Enemy, .id = 1, .tile = { 1, 0 } };
	dev::InteractionIntent intent = dev::InteractionIntentBuilder {}.build(target, true);
	dev::MovementCommand command = dev::InteractionCommandBuilder {}.build(0, player, intent, gate);

	Expect(command.type == dev::MovementCommandType::StandAndAct, "stand-ground attack should create StandAndAct");
	Expect(command.destinationAction.has_value(), "stand-ground command should keep destination action");
	Expect(command.destinationAction->type == dev::DestinationActionType::Attack, "stand-ground destination action should be attack");
}

void TestMoveThenActExecutesAfterPath()
{
	dev::TileMap map;
	dev::Collision collision;
	dev::PathFinder pathFinder;
	std::vector<dev::Player> players { MakePlayer({ 0, 0 }) };
	dev::PlayerController controller { players, map, collision, pathFinder };
	dev::CommandDispatcher dispatcher { controller };
	dev::PlayerMovement movement { collision };

	dev::Target target { .type = dev::TargetType::Enemy, .id = 1, .tile = { 1, 0 } };
	dev::MovementCommand command {
		.type = dev::MovementCommandType::MoveThenAct,
		.playerId = 0,
		.destination = target.tile,
		.destinationAction = dev::DestinationAction { dev::DestinationActionType::Attack, target, 1 },
	};

	dispatcher.dispatch(command);
	Expect(players[0].moveState == dev::PlayerMoveState::Pathing, "MoveThenAct should start pathing");

	movement.update(players, 0.016F);
	Expect(players[0].position.tile == dev::Point { 1, 0 }, "movement should commit next step");
	Expect(players[0].destinationAction.type == dev::DestinationActionType::None, "action should clear after execution");
	Expect(players[0].animationLock.active, "executed attack should apply animation lock");
}

void TestDiagonalCornerPolicyBlocksCornerCutting()
{
	dev::TileMap map;
	dev::Collision collision;
	map.setBlocked({ 1, 0 });
	map.setBlocked({ 0, 1 });

	dev::PathFinder pathFinder;
	dev::WalkPath path = pathFinder.findPath({ 0, 0 }, { 1, 1 }, map, collision);

	Expect(path.empty(), "diagonal path should be blocked when both side corners are blocked");
}

void TestActionExecutorWaitsOutOfRange()
{
	dev::Player player = MakePlayer({ 0, 0 });
	dev::Target target { .type = dev::TargetType::Enemy, .id = 1, .tile = { 4, 0 } };
	player.destinationAction = { dev::DestinationActionType::Attack, target, 1 };
	player.moveState = dev::PlayerMoveState::Acting;

	dev::ActionResult result = dev::ActionExecutor {}.update(player);

	Expect(result.type == dev::ActionResultType::OutOfRange, "out-of-range action should wait");
	Expect(player.destinationAction.type == dev::DestinationActionType::Attack, "out-of-range action should stay queued");
}

void TestMoveThenActEventSequence()
{
	dev::EventRecorder events;
	dev::TileMap map;
	dev::Collision collision;
	dev::PathFinder pathFinder;
	std::vector<dev::Player> players { MakePlayer({ 0, 0 }) };
	dev::PlayerController controller { players, map, collision, pathFinder, &events };
	dev::CommandDispatcher dispatcher { controller, &events };
	dev::ActionExecutor actionExecutor { dev::ActionRules {}, &events };
	dev::PlayerMovement movement { collision, actionExecutor, &events };

	dev::Target target { .type = dev::TargetType::Enemy, .id = 1, .tile = { 1, 0 } };
	dev::MovementCommand command {
		.type = dev::MovementCommandType::MoveThenAct,
		.playerId = 0,
		.destination = target.tile,
		.destinationAction = dev::DestinationAction { dev::DestinationActionType::Attack, target, 1 },
	};

	dispatcher.dispatch(command);
	movement.update(players, 0.016F);

	const std::vector<dev::MovementEvent> &recorded = events.events();
	Expect(recorded.size() >= 6, "MoveThenAct should emit observable movement/action events");
	Expect(recorded[0].type == dev::MovementEventType::CommandAccepted, "first event should accept command");
	Expect(recorded[1].type == dev::MovementEventType::PathStarted, "second event should start path");
	Expect(recorded[2].type == dev::MovementEventType::StepCommitted, "third event should commit step");
	Expect(recorded[3].type == dev::MovementEventType::DestinationActionReady, "fourth event should make destination action ready");
	Expect(recorded[4].type == dev::MovementEventType::AnimationLocked, "fifth event should lock animation");
	Expect(recorded[5].type == dev::MovementEventType::ActionExecuted, "sixth event should execute action");
}

void TestCommandReplayProducesSameEventSequence()
{
	dev::Target target { .type = dev::TargetType::Enemy, .id = 1, .tile = { 1, 0 } };
	dev::MovementCommand command {
		.type = dev::MovementCommandType::MoveThenAct,
		.playerId = 0,
		.destination = target.tile,
		.destinationAction = dev::DestinationAction { dev::DestinationActionType::Attack, target, 1 },
	};

	dev::CommandLog log;
	log.record(command);
	Expect(!log.empty(), "command log should record command");

	dev::EventRecorder events;
	dev::TileMap map;
	dev::Collision collision;
	dev::PathFinder pathFinder;
	std::vector<dev::Player> players { MakePlayer({ 0, 0 }) };
	dev::PlayerController controller { players, map, collision, pathFinder, &events };
	dev::CommandDispatcher dispatcher { controller, &events };
	dev::CommandReplayer replayer { dispatcher };
	dev::ActionExecutor actionExecutor { dev::ActionRules {}, &events };
	dev::PlayerMovement movement { collision, actionExecutor, &events };

	replayer.replay(log);
	movement.update(players, 0.016F);

	const std::vector<dev::MovementEvent> &recorded = events.events();
	Expect(recorded.size() >= 6, "replayed command should produce movement/action events");
	Expect(recorded[0].type == dev::MovementEventType::CommandAccepted, "replay should accept command");
	Expect(recorded[1].type == dev::MovementEventType::PathStarted, "replay should start path");
	Expect(recorded[2].type == dev::MovementEventType::StepCommitted, "replay should commit step");
	Expect(recorded[5].type == dev::MovementEventType::ActionExecuted, "replay should execute action");
}

void TestMovementCodecRoundTrip()
{
	dev::Target target { .type = dev::TargetType::Enemy, .id = 42, .tile = { 10, 6 } };
	dev::MovementCommand original {
		.type = dev::MovementCommandType::MoveThenAct,
		.playerId = 2,
		.destination = target.tile,
		.destinationAction = dev::DestinationAction { dev::DestinationActionType::Attack, target, 1 },
	};

	dev::MovementCodec codec;
	dev::MovementPacket packet = codec.toPacket(original);
	dev::PacketBytes bytes = codec.encode(packet);
	std::optional<dev::MovementPacket> decodedPacket = codec.decode(bytes);
	Expect(decodedPacket.has_value(), "encoded packet should decode");

	std::optional<dev::MovementCommand> decoded = decodedPacket.has_value()
	    ? codec.fromPacket(*decodedPacket)
	    : std::nullopt;
	Expect(decoded.has_value(), "decoded packet should become command");
	if (!decoded.has_value())
		return;

	Expect(decoded->type == original.type, "codec should preserve command type");
	Expect(decoded->playerId == original.playerId, "codec should preserve player id");
	Expect(decoded->destination == original.destination, "codec should preserve destination");
	Expect(decoded->destinationAction.has_value(), "codec should preserve destination action");
	Expect(decoded->destinationAction->type == dev::DestinationActionType::Attack, "codec should preserve action type");
	Expect(decoded->destinationAction->target.type == dev::TargetType::Enemy, "codec should preserve target type");
	Expect(decoded->destinationAction->target.id == 42, "codec should preserve target id");
	Expect(decoded->destinationAction->target.tile == target.tile, "codec should preserve target tile");
	Expect(decoded->destinationAction->rangeTiles == 1, "codec should preserve range");
}

void TestEnemyPursuitObeysStepBudget()
{
	dev::TileMap map;
	dev::Collision collision;
	dev::Player player = MakePlayer({ 4, 0 });
	std::vector<dev::Enemy> enemies { MakeEnemy({ 0, 0 }) };
	enemies[0].tuning.maxStepsPerTick = 1;

	dev::EnemyMovement movement { map, collision };
	movement.update(enemies, player, 0.016F);

	Expect(enemies[0].position.tile == dev::Point { 1, 0 }, "enemy should move only one step toward player");
	Expect(enemies[0].moveState == dev::EnemyMoveState::Pursuing, "enemy should be pursuing after constrained movement");
}

void TestEnemyAttackWindupAndRecovery()
{
	dev::TileMap map;
	dev::Collision collision;
	dev::Player player = MakePlayer({ 1, 0 });
	std::vector<dev::Enemy> enemies { MakeEnemy({ 0, 0 }) };
	enemies[0].tuning.attackRangeTiles = 1;
	enemies[0].tuning.attackWindupSeconds = 0.25F;
	enemies[0].tuning.attackRecoverySeconds = 0.50F;

	dev::EnemyMovement movement { map, collision };
	movement.update(enemies, player, 0.016F);
	Expect(enemies[0].moveState == dev::EnemyMoveState::Attacking, "enemy in range should enter attack windup");

	movement.update(enemies, player, 0.25F);
	Expect(enemies[0].moveState == dev::EnemyMoveState::Recovering, "enemy should enter recovery after windup");

	movement.update(enemies, player, 0.25F);
	Expect(enemies[0].moveState == dev::EnemyMoveState::Recovering, "enemy should remain in recovery until recovery duration completes");
}

void TestCombatResolverDamageAndDefeat()
{
	dev::CombatStats attacker { .hitPoints = 20, .attackPower = 7, .defense = 1 };
	dev::Combatant target {
		.target = { .type = dev::TargetType::Enemy, .id = 99, .tile = { 1, 0 } },
		.stats = { .hitPoints = 5, .attackPower = 3, .defense = 2 },
	};

	dev::CombatResult result = dev::CombatResolver {}.resolveAttack(attacker, target);

	Expect(result.damage == 5, "combat damage should be attack minus defense");
	Expect(result.type == dev::CombatResultType::Defeated, "target should be defeated when hp reaches zero");
	Expect(target.stats.hitPoints == 0, "target hp should clamp to zero");
}

void TestActionExecutorAttackResolvesCombat()
{
	dev::CombatSystem combat;
	dev::Target target { .type = dev::TargetType::Enemy, .id = 7, .tile = { 1, 0 } };
	combat.registry().add({
		.target = target,
		.stats = { .hitPoints = 10, .attackPower = 3, .defense = 1 },
	});

	dev::Player player = MakePlayer({ 1, 0 });
	player.combatStats.attackPower = 6;
	player.destinationAction = { dev::DestinationActionType::Attack, target, 1 };
	player.moveState = dev::PlayerMoveState::Acting;

	dev::ActionResult result = dev::ActionExecutor { dev::ActionRules {}, nullptr, &combat }.update(player);
	const dev::Combatant *enemy = combat.registry().find(target);

	Expect(result.type == dev::ActionResultType::Executed, "attack action should execute through combat system");
	Expect(enemy != nullptr, "combat target should still be registered");
	Expect(enemy != nullptr && enemy->stats.hitPoints == 5, "attack action should damage combat target");
	Expect(player.destinationAction.type == dev::DestinationActionType::None, "executed combat action should clear destination action");
}

void TestCombatSystemEmitsHitEvent()
{
	dev::CombatEventRecorder combatEvents;
	dev::CombatSystem combat { &combatEvents };
	dev::Target target { .type = dev::TargetType::Enemy, .id = 8, .tile = { 1, 0 } };
	combat.registry().add({
		.target = target,
		.stats = { .hitPoints = 10, .attackPower = 3, .defense = 1 },
	});

	dev::Player player = MakePlayer({ 1, 0 });
	player.combatStats.attackPower = 6;
	player.destinationAction = { dev::DestinationActionType::Attack, target, 1 };
	player.moveState = dev::PlayerMoveState::Acting;

	dev::ActionExecutor { dev::ActionRules {}, nullptr, &combat }.update(player);

	const std::vector<dev::CombatEvent> &events = combatEvents.events();
	Expect(events.size() == 1, "combat hit should emit one combat event");
	Expect(events.size() == 1 && events[0].type == dev::CombatEventType::Hit, "combat event should be Hit");
	Expect(events.size() == 1 && events[0].damage == 5, "combat hit event should include damage");
	Expect(events.size() == 1 && events[0].remainingHitPoints == 5, "combat hit event should include remaining hp");
	Expect(events.size() == 1 && events[0].target.id == 8, "combat hit event should include target id");
}

void TestCombatSystemEmitsDefeatedEvent()
{
	dev::CombatEventRecorder combatEvents;
	dev::CombatSystem combat { &combatEvents };
	dev::Target target { .type = dev::TargetType::Enemy, .id = 9, .tile = { 1, 0 } };
	combat.registry().add({
		.target = target,
		.stats = { .hitPoints = 5, .attackPower = 3, .defense = 1 },
	});

	dev::Player player = MakePlayer({ 1, 0 });
	player.combatStats.attackPower = 6;
	player.destinationAction = { dev::DestinationActionType::Attack, target, 1 };
	player.moveState = dev::PlayerMoveState::Acting;

	dev::ActionExecutor { dev::ActionRules {}, nullptr, &combat }.update(player);

	const std::vector<dev::CombatEvent> &events = combatEvents.events();
	Expect(events.size() == 1, "defeat should emit one combat event");
	Expect(events.size() == 1 && events[0].type == dev::CombatEventType::Defeated, "combat event should be Defeated");
	Expect(events.size() == 1 && events[0].damage == 5, "defeated event should include damage");
	Expect(events.size() == 1 && events[0].remainingHitPoints == 0, "defeated event should include zero remaining hp");
}

void TestPlayerAttackUsesEquippedCombatModifiers()
{
	dev::CombatSystem combat;
	dev::Target target { .type = dev::TargetType::Enemy, .id = 10, .tile = { 1, 0 } };
	combat.registry().add({
		.target = target,
		.stats = { .hitPoints = 10, .attackPower = 3, .defense = 2 },
	});

	dev::Player player = MakePlayer({ 1, 0 });
	player.combatStats.attackPower = 5;
	player.inventory.items.push_back({
	    .id = 910,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	    .combatModifiers = { .attackPower = 99 },
	});
	player.inventory.equipment.weapon = dev::Item {
		.id = 911,
		.equipmentSlot = dev::EquipmentSlot::Weapon,
		.combatModifiers = { .attackPower = 4 },
	};
	player.destinationAction = { dev::DestinationActionType::Attack, target, 1 };
	player.moveState = dev::PlayerMoveState::Acting;

	dev::ActionResult result = dev::ActionExecutor { dev::ActionRules {}, nullptr, &combat }.update(player);
	const dev::Combatant *enemy = combat.registry().find(target);

	Expect(result.type == dev::ActionResultType::Executed, "equipped attack modifier should still allow action execution");
	Expect(enemy != nullptr && enemy->stats.hitPoints == 3, "equipped weapon modifier should increase player attack damage");
}

void TestEnemyAttackUsesEquippedDefenseModifiers()
{
	dev::TileMap map;
	dev::Collision collision;
	dev::CombatSystem combat;
	dev::Player player = MakePlayer({ 1, 0 });
	player.combatStats.hitPoints = 20;
	player.combatStats.defense = 1;
	player.inventory.items.push_back({
	    .id = 912,
	    .equipmentSlot = dev::EquipmentSlot::Armor,
	    .combatModifiers = { .defense = 99 },
	});
	player.inventory.equipment.armor = dev::Item {
		.id = 913,
		.equipmentSlot = dev::EquipmentSlot::Armor,
		.combatModifiers = { .defense = 4 },
	};
	std::vector<dev::Enemy> enemies { MakeEnemy({ 0, 0 }) };
	enemies[0].combatStats.attackPower = 8;
	enemies[0].tuning.attackRangeTiles = 1;
	enemies[0].tuning.attackWindupSeconds = 0.25F;

	dev::EnemyMovement movement { map, collision, nullptr, &combat };
	movement.update(enemies, player, 0.016F);
	movement.update(enemies, player, 0.25F);

	Expect(player.combatStats.hitPoints == 17, "equipped armor modifier should reduce enemy attack damage");
}

void TestEnemyAttackResolvesCombatAgainstPlayer()
{
	dev::TileMap map;
	dev::Collision collision;
	dev::CombatEventRecorder combatEvents;
	dev::CombatSystem combat { &combatEvents };
	dev::Player player = MakePlayer({ 1, 0 });
	player.combatStats.hitPoints = 20;
	player.combatStats.defense = 1;
	std::vector<dev::Enemy> enemies { MakeEnemy({ 0, 0 }) };
	enemies[0].combatStats.attackPower = 5;
	enemies[0].tuning.attackRangeTiles = 1;
	enemies[0].tuning.attackWindupSeconds = 0.25F;

	dev::EnemyMovement movement { map, collision, nullptr, &combat };
	movement.update(enemies, player, 0.016F);
	Expect(enemies[0].moveState == dev::EnemyMoveState::Attacking, "enemy should enter windup before damaging player");

	movement.update(enemies, player, 0.25F);
	Expect(player.combatStats.hitPoints == 16, "enemy attack should damage player after windup");
	Expect(enemies[0].moveState == dev::EnemyMoveState::Recovering, "enemy should recover after resolving attack");

	const std::vector<dev::CombatEvent> &events = combatEvents.events();
	Expect(events.size() == 1, "enemy attack should emit one combat event");
	Expect(events.size() == 1 && events[0].type == dev::CombatEventType::Hit, "enemy attack event should be Hit");
	Expect(events.size() == 1 && events[0].target.type == dev::TargetType::Player, "enemy attack event should target player");
	Expect(events.size() == 1 && events[0].damage == 4, "enemy attack event should include damage");
}

void TestSimulationCommandDrainerDispatchesQueuedMovementCommands()
{
	dev::SimulationWorld world;
	dev::EventRecorder movementEvents;
	world.movementEvents = &movementEvents;
	world.players.push_back(MakePlayer({ 0, 0 }));
	world.commandQueue.push({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	    .destinationAction = std::nullopt,
	});

	dev::SimulationCommandDrainer {}.drain(world);

	dev::MovementCommand command;
	Expect(!world.commandQueue.tryPop(command), "simulation command drainer should empty queued movement commands");
	Expect(world.players[0].moveState == dev::PlayerMoveState::Pathing, "simulation command drainer should dispatch movement commands through player controller");
	Expect(movementEvents.events().size() >= 2, "simulation command drainer should emit command and controller events");
	Expect(!movementEvents.events().empty() && movementEvents.events()[0].type == dev::MovementEventType::CommandAccepted, "simulation command drainer should emit command accepted event");
}

void TestSimulationPlayerUpdaterAdvancesPlayerMovement()
{
	dev::SimulationWorld world;
	dev::EventRecorder movementEvents;
	world.movementEvents = &movementEvents;
	world.players.push_back(MakePlayer({ 0, 0 }));
	world.players[0].path.pushStep({ 1, 0 });
	world.players[0].moveState = dev::PlayerMoveState::Pathing;

	dev::SimulationPlayerUpdater {}.update(world, 0.016F);

	Expect(world.players[0].position.tile == dev::Point { 1, 0 }, "simulation player updater should commit player path steps");
	Expect(world.players[0].moveState == dev::PlayerMoveState::Idle, "simulation player updater should settle player after final path step");
	Expect(!movementEvents.events().empty() && movementEvents.events()[0].type == dev::MovementEventType::StepCommitted, "simulation player updater should emit movement events");
}

void TestSimulationTickDispatchesMovementAndCombat()
{
	dev::SimulationWorld world;
	dev::EventRecorder movementEvents;
	dev::CombatEventRecorder combatEvents;
	world.movementEvents = &movementEvents;
	world.setCombatEventSink(&combatEvents);
	world.players.push_back(MakePlayer({ 0, 0 }));
	world.players[0].combatStats.attackPower = 6;

	dev::Target target { .type = dev::TargetType::Enemy, .id = 20, .tile = { 1, 0 } };
	world.combat.registry().add({
	    .target = target,
	    .stats = { .hitPoints = 10, .attackPower = 3, .defense = 1 },
	});
	world.commandQueue.push({
	    .type = dev::MovementCommandType::MoveThenAct,
	    .playerId = 0,
	    .destination = target.tile,
	    .destinationAction = dev::DestinationAction { dev::DestinationActionType::Attack, target, 1 },
	});

	dev::SimulationTick {}.update(world, 0.016F);

	const dev::Combatant *enemy = world.combat.registry().find(target);
	const std::vector<dev::CombatEvent> &combat = combatEvents.events();

	Expect(world.players[0].position.tile == dev::Point { 1, 0 }, "simulation tick should move player from queued command");
	Expect(enemy != nullptr && enemy->stats.hitPoints == 5, "simulation tick should resolve queued attack through combat");
	Expect(combat.size() == 1 && combat[0].type == dev::CombatEventType::Hit, "simulation tick should emit combat event");
	Expect(!movementEvents.events().empty(), "simulation tick should emit movement events");
}

void TestSimulationPolicyPausedDoesNotDrainCommands()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 0, 0 }));
	world.commandQueue.push({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	    .destinationAction = std::nullopt,
	});

	dev::SimulationTick tick;
	tick.update(world, 0.016F, dev::SimulationFramePolicy::forMode(dev::SimulationMode::Paused));
	Expect(world.players[0].position.tile == dev::Point { 0, 0 }, "paused simulation should not move player");

	tick.update(world, 0.016F);
	Expect(world.players[0].position.tile == dev::Point { 1, 0 }, "gameplay simulation should drain preserved command");
}

void TestSimulationClockHitStopFreezesActorUpdates()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 0, 0 }));
	world.commandQueue.push({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	    .destinationAction = std::nullopt,
	});

	dev::SimulationClock clock;
	clock.triggerHitStop(0.25F);
	dev::SimulationTick tick;

	tick.update(world, clock.step(0.10F));
	Expect(world.players[0].position.tile == dev::Point { 0, 0 }, "hit-stop should freeze player movement");

	tick.update(world, clock.step(0.20F));
	Expect(world.players[0].position.tile == dev::Point { 1, 0 }, "movement should resume after hit-stop remainder expires");
}

void TestSimulationClockScalesEnemyWindup()
{
	dev::SimulationWorld world;
	dev::CombatEventRecorder combatEvents;
	world.setCombatEventSink(&combatEvents);
	world.players.push_back(MakePlayer({ 1, 0 }));
	world.players[0].combatStats.hitPoints = 20;
	world.players[0].combatStats.defense = 1;
	world.enemies.push_back(MakeEnemy({ 0, 0 }));
	world.enemies[0].moveState = dev::EnemyMoveState::Attacking;
	world.enemies[0].combatStats.attackPower = 5;
	world.enemies[0].tuning.attackWindupSeconds = 1.0F;

	dev::SimulationClock clock;
	clock.setTimeScale(0.5F);
	dev::SimulationTick tick;

	tick.update(world, clock.step(1.0F));
	Expect(world.players[0].combatStats.hitPoints == 20, "half-speed enemy windup should not finish after one raw second");

	tick.update(world, clock.step(1.0F));
	Expect(world.players[0].combatStats.hitPoints == 16, "half-speed enemy windup should finish after two raw seconds");
	Expect(combatEvents.events().size() == 1, "scaled windup should emit one combat event when it completes");
}

void TestEffectRouterMapsMovementEventsToRequests()
{
	dev::EffectRecorder effects;
	dev::EffectRouter router { effects };

	router.route(dev::MovementEvent {
	    .type = dev::MovementEventType::StepCommitted,
	    .tile = { 2, 3 },
	});
	router.route(dev::MovementEvent {
	    .type = dev::MovementEventType::PathBlocked,
	    .tile = { 3, 3 },
	});

	const std::vector<dev::EffectRequest> &requests = effects.requests();
	Expect(requests.size() == 2, "movement effects should emit two requests");
	Expect(requests.size() == 2 && requests[0].type == dev::EffectRequestType::Footstep, "step should create footstep effect");
	Expect(requests.size() == 2 && requests[0].tile == dev::Point { 2, 3 }, "footstep should keep event tile");
	Expect(requests.size() == 2 && requests[1].type == dev::EffectRequestType::BlockedFeedback, "blocked path should create feedback effect");
}

void TestEffectRouterMapsCombatHitToRequests()
{
	dev::EffectRecorder effects;
	dev::EffectRouter router { effects };
	dev::Target target { .type = dev::TargetType::Enemy, .id = 30, .tile = { 4, 1 } };

	router.route(dev::CombatEvent {
	    .type = dev::CombatEventType::Hit,
	    .target = target,
	    .damage = 7,
	    .remainingHitPoints = 3,
	    .result = dev::CombatResultType::Hit,
	});

	const std::vector<dev::EffectRequest> &requests = effects.requests();
	Expect(requests.size() == 3, "combat hit should emit damage, impact, and hit-stop requests");
	Expect(requests.size() == 3 && requests[0].type == dev::EffectRequestType::DamageNumber, "combat hit should create damage number");
	Expect(requests.size() == 3 && requests[0].damage == 7, "damage number should preserve damage");
	Expect(requests.size() == 3 && requests[1].type == dev::EffectRequestType::HitImpact, "combat hit should create impact request");
	Expect(requests.size() == 3 && requests[2].type == dev::EffectRequestType::HitStop, "combat hit should create hit-stop request");
	Expect(requests.size() == 3 && requests[2].durationSeconds > 0.0F, "hit-stop request should include duration");
}

void TestEffectApplierAppliesHitStopToClock()
{
	dev::EffectRecorder effects;
	dev::EffectRouter router { effects };
	dev::SimulationClock clock;
	dev::EffectApplier applier { &clock };
	dev::Target target { .type = dev::TargetType::Enemy, .id = 31, .tile = { 2, 1 } };

	router.route(dev::CombatEvent {
	    .type = dev::CombatEventType::Hit,
	    .target = target,
	    .damage = 4,
	    .remainingHitPoints = 6,
	    .result = dev::CombatResultType::Hit,
	});

	for (const dev::EffectRequest &request : effects.requests()) {
		applier.apply(request);
	}

	Expect(clock.hitStopRemainingSeconds() > 0.0F, "hit-stop effect request should apply to simulation clock");
	dev::SimulationTimeStep step = clock.step(0.01F);
	Expect(step.playerDeltaSeconds == 0.0F, "applied hit-stop should freeze player actor time");
	Expect(step.enemyDeltaSeconds == 0.0F, "applied hit-stop should freeze enemy actor time");
}

void TestSimulationEffectPipelineRoutesAndAppliesEffects()
{
	dev::SimulationClock clock;
	dev::SimulationFrameEvents frameEvents;
	frameEvents.emit(dev::MovementEvent {
	    .type = dev::MovementEventType::StepCommitted,
	    .playerId = 0,
	    .tile = { 2, 0 },
	});
	frameEvents.emit(dev::CombatEvent {
	    .type = dev::CombatEventType::Hit,
	    .target = dev::Target { .type = dev::TargetType::Enemy, .id = 45, .tile = { 2, 0 } },
	    .damage = 5,
	    .remainingHitPoints = 4,
	    .result = dev::CombatResultType::Hit,
	});

	dev::SimulationEffectPipeline { &clock }.run(frameEvents);

	Expect(frameEvents.effectRequests().size() == 4, "simulation effect pipeline should route movement and combat effects");
	Expect(frameEvents.effectRequests().size() == 4 && frameEvents.effectRequests()[0].type == dev::EffectRequestType::Footstep, "simulation effect pipeline should preserve movement effect order");
	Expect(frameEvents.effectRequests().size() == 4 && frameEvents.effectRequests()[3].type == dev::EffectRequestType::HitStop, "simulation effect pipeline should route hit-stop request");
	Expect(clock.hitStopRemainingSeconds() > 0.0F, "simulation effect pipeline should apply hit-stop to clock");
}

void TestSimulationFrameEventCaptureCollectsForwardsAndRestoresSinks()
{
	dev::SimulationWorld world;
	dev::EventRecorder forwardedMovement;
	dev::CombatEventRecorder forwardedCombat;
	world.movementEvents = &forwardedMovement;
	world.setCombatEventSink(&forwardedCombat);

	{
		dev::SimulationFrameEventCapture capture { world };
		Expect(world.movementEvents == &capture.events(), "simulation frame event capture should install movement capture sink");
		Expect(world.combatEvents == &capture.events(), "simulation frame event capture should install combat capture sink");

		world.movementEvents->emit({
		    .type = dev::MovementEventType::StepCommitted,
		    .playerId = 0,
		    .tile = { 2, 0 },
		});
		world.combatEvents->emit({
		    .type = dev::CombatEventType::Hit,
		    .target = dev::Target { .type = dev::TargetType::Enemy, .id = 44, .tile = { 2, 0 } },
		    .damage = 2,
		    .remainingHitPoints = 3,
		    .result = dev::CombatResultType::Hit,
		});

		Expect(capture.events().movementEvents().size() == 1, "simulation frame event capture should collect movement events");
		Expect(capture.events().combatEvents().size() == 1, "simulation frame event capture should collect combat events");
		Expect(forwardedMovement.events().size() == 1, "simulation frame event capture should forward movement events");
		Expect(forwardedCombat.events().size() == 1, "simulation frame event capture should forward combat events");
	}

	Expect(world.movementEvents == &forwardedMovement, "simulation frame event capture should restore movement sink");
	Expect(world.combatEvents == &forwardedCombat, "simulation frame event capture should restore combat sink");
}

void TestSimulationFrameFinalizerAppliesConsequences()
{
	dev::SimulationWorld world;
	dev::SimulationClock clock;
	world.players.push_back(MakePlayer({ 1, 0 }));
	dev::WorldEntityService {}.spawnItem(world, {
	    .id = 620,
	    .tile = { 1, 0 },
	});
	dev::Enemy enemy = MakeEnemy({ 2, 0 });
	enemy.id = 41;
	world.enemies.push_back(enemy);

	dev::SimulationFrameEvents frameEvents;
	frameEvents.emit(dev::MovementEvent {
	    .type = dev::MovementEventType::ActionExecuted,
	    .playerId = 0,
	    .tile = { 1, 0 },
	    .actionType = dev::DestinationActionType::Pickup,
	    .actionResult = dev::ActionResultType::Executed,
	    .target = dev::Target { .type = dev::TargetType::Item, .id = 620, .tile = { 1, 0 } },
	});
	frameEvents.emit(dev::CombatEvent {
	    .type = dev::CombatEventType::Hit,
	    .target = dev::Target { .type = dev::TargetType::Enemy, .id = 41, .tile = { 2, 0 } },
	    .damage = 3,
	    .remainingHitPoints = 4,
	    .result = dev::CombatResultType::Hit,
	});

	dev::SimulationFrameFinalizer { &clock }.finalize(world, frameEvents);

	Expect(world.items.empty(), "simulation frame finalizer should apply pickup transfers");
	Expect(world.players[0].inventory.items.size() == 1 && world.players[0].inventory.items[0].id == 620, "simulation frame finalizer should preserve picked item identity");
	Expect(world.targets.resolveAtTile({ 1, 0 }).type == dev::TargetType::EmptyTile, "simulation frame finalizer should remove picked item target");
	Expect(world.targets.resolveAtTile({ 2, 0 }).type == dev::TargetType::Enemy, "simulation frame finalizer should publish current enemy targets");
	Expect(!frameEvents.effectRequests().empty(), "simulation frame finalizer should route events into effect requests");
	Expect(clock.hitStopRemainingSeconds() > 0.0F, "simulation frame finalizer should apply hit-stop effect requests");
}

void TestSimulationFrameRunnerProcessesConsequences()
{
	dev::SimulationWorld world;
	dev::EventRecorder forwardedMovement;
	dev::CombatEventRecorder forwardedCombat;
	dev::SimulationClock clock;
	world.movementEvents = &forwardedMovement;
	world.setCombatEventSink(&forwardedCombat);
	world.players.push_back(MakePlayer({ 0, 0 }));
	world.players[0].combatStats.attackPower = 6;

	dev::Target target { .type = dev::TargetType::Enemy, .id = 40, .tile = { 1, 0 } };
	world.combat.registry().add({
	    .target = target,
	    .stats = { .hitPoints = 10, .attackPower = 3, .defense = 1 },
	});
	world.commandQueue.push({
	    .type = dev::MovementCommandType::MoveThenAct,
	    .playerId = 0,
	    .destination = target.tile,
	    .destinationAction = dev::DestinationAction { dev::DestinationActionType::Attack, target, 1 },
	});

	dev::SimulationFrameRunner runner { &clock };
	dev::SimulationFrameEvents frame = runner.run(world, 0.016F);
	const dev::Combatant *enemy = world.combat.registry().find(target);

	Expect(enemy != nullptr && enemy->stats.hitPoints == 5, "frame runner should preserve combat registry while collecting events");
	Expect(!frame.movementEvents().empty(), "frame runner should collect movement events");
	Expect(frame.combatEvents().size() == 1, "frame runner should collect combat events");
	Expect(!frame.effectRequests().empty(), "frame runner should route frame events into effect requests");
	Expect(clock.hitStopRemainingSeconds() > 0.0F, "frame runner should apply hit-stop effect requests to clock");
	Expect(!forwardedMovement.events().empty(), "frame runner should forward movement events to existing sink");
	Expect(forwardedCombat.events().size() == 1, "frame runner should forward combat events to existing sink");

	world.commandQueue.push({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 2, 0 },
	    .destinationAction = std::nullopt,
	});
	dev::SimulationFrameEvents stoppedFrame = runner.run(world, 0.01F);
	Expect(world.players[0].position.tile == dev::Point { 1, 0 }, "runner should freeze actor updates while hit-stop remains");
	Expect(!stoppedFrame.movementEvents().empty(), "runner should still accept commands during hit-stop");
}

void TestTargetRegistryResolvesAndRemovesTargets()
{
	dev::TargetRegistry registry;
	registry.add({ .type = dev::TargetType::Item, .id = 10, .tile = { 2, 3 } });
	registry.add({ .type = dev::TargetType::Enemy, .id = 11, .tile = { 4, 5 } });

	dev::Target item = registry.resolveAtTile({ 2, 3 });
	dev::Target enemy = registry.resolveAtTile({ 4, 5 });
	dev::Target empty = registry.resolveAtTile({ 8, 8 });

	Expect(item.type == dev::TargetType::Item && item.id == 10, "target registry should resolve item target by tile");
	Expect(enemy.type == dev::TargetType::Enemy && enemy.id == 11, "target registry should resolve enemy target by tile");
	Expect(empty.type == dev::TargetType::EmptyTile && empty.tile == dev::Point { 8, 8 }, "target registry should return empty tile when no target is registered");
	Expect(registry.remove(dev::TargetType::Item, 10), "target registry should remove matching target");
	Expect(!registry.remove(dev::TargetType::Item, 10), "target registry should report missing target removal");
	Expect(registry.resolveAtTile({ 2, 3 }).type == dev::TargetType::EmptyTile, "removed target should no longer resolve");
	Expect(registry.removeAll(dev::TargetType::Enemy) == 1, "target registry should remove all targets of a type");
	Expect(registry.empty(), "target registry should be empty after removing remaining type");
}

void TestTargetSynchronizerSyncsEnemyTargetsWithoutRemovingObjects()
{
	dev::TargetRegistry registry;
	registry.add({ .type = dev::TargetType::Object, .id = 90, .tile = { 8, 8 } });
	registry.add({ .type = dev::TargetType::Enemy, .id = 1, .tile = { 0, 0 } });

	std::vector<dev::Enemy> enemies { MakeEnemy({ 3, 4 }) };
	enemies[0].id = 12;
	dev::CombatRegistry combat;
	combat.add({
	    .target = { .type = dev::TargetType::Enemy, .id = 12, .tile = { 3, 4 } },
	    .stats = { .hitPoints = 5, .attackPower = 2, .defense = 1 },
	});

	dev::TargetSynchronizer synchronizer;
	synchronizer.syncEnemyTargets(enemies, combat, registry);

	dev::Target enemy = registry.resolveAtTile({ 3, 4 });
	dev::Target stale = registry.resolveAtTile({ 0, 0 });
	dev::Target object = registry.resolveAtTile({ 8, 8 });
	Expect(enemy.type == dev::TargetType::Enemy && enemy.id == 12, "target synchronizer should publish current enemy target");
	Expect(stale.type == dev::TargetType::EmptyTile, "target synchronizer should remove stale enemy target");
	Expect(object.type == dev::TargetType::Object && object.id == 90, "target synchronizer should preserve non-enemy targets");
}

void TestTargetSynchronizerSkipsDefeatedEnemies()
{
	dev::TargetRegistry registry;
	std::vector<dev::Enemy> enemies { MakeEnemy({ 4, 4 }), MakeEnemy({ 5, 4 }) };
	enemies[0].id = 20;
	enemies[1].id = 21;

	dev::CombatRegistry combat;
	combat.add({
	    .target = { .type = dev::TargetType::Enemy, .id = 20, .tile = { 4, 4 } },
	    .stats = { .hitPoints = 0, .attackPower = 2, .defense = 1 },
	});
	combat.add({
	    .target = { .type = dev::TargetType::Enemy, .id = 21, .tile = { 5, 4 } },
	    .stats = { .hitPoints = 3, .attackPower = 2, .defense = 1 },
	});

	dev::TargetSynchronizer {}.syncEnemyTargets(enemies, combat, registry);

	Expect(registry.resolveAtTile({ 4, 4 }).type == dev::TargetType::EmptyTile, "target synchronizer should skip defeated combat registry enemies");
	Expect(registry.resolveAtTile({ 5, 4 }).type == dev::TargetType::Enemy, "target synchronizer should keep living combat registry enemies");
}

void TestFrameRunnerSynchronizesMovedEnemyTargets()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 3, 0 }));
	world.enemies.push_back(MakeEnemy({ 0, 0 }));
	world.enemies[0].id = 30;
	world.enemies[0].tuning.maxStepsPerTick = 1;
	world.enemies[0].tuning.attackRangeTiles = 0;
	world.targets.add({ .type = dev::TargetType::Enemy, .id = 30, .tile = { 0, 0 } });

	dev::SimulationFrameRunner {}.run(world, 1.0F);

	Expect(world.enemies[0].position.tile == dev::Point { 1, 0 }, "frame runner sync test should move enemy one step");
	Expect(world.targets.resolveAtTile({ 0, 0 }).type == dev::TargetType::EmptyTile, "frame runner should remove stale enemy target tile");
	Expect(world.targets.resolveAtTile({ 1, 0 }).type == dev::TargetType::Enemy, "frame runner should sync enemy target to moved tile");
}

void TestFrameRunnerRemovesDefeatedEnemyTargets()
{
	dev::SimulationWorld world;
	dev::Player player = MakePlayer({ 0, 0 });
	player.combatStats.attackPower = 10;
	world.players.push_back(player);
	world.enemies.push_back(MakeEnemy({ 1, 0 }));
	world.enemies[0].id = 40;
	world.enemies[0].tuning.attackRangeTiles = 0;

	dev::Target target { .type = dev::TargetType::Enemy, .id = 40, .tile = { 1, 0 } };
	world.targets.add(target);
	world.combat.registry().add({
	    .target = target,
	    .stats = { .hitPoints = 1, .attackPower = 1, .defense = 0 },
	});
	world.commandQueue.push({
	    .type = dev::MovementCommandType::MoveThenAct,
	    .playerId = 0,
	    .destination = target.tile,
	    .destinationAction = dev::DestinationAction { dev::DestinationActionType::Attack, target, 1 },
	});

	dev::SimulationFrameEvents frame = dev::SimulationFrameRunner {}.run(world, 1.0F);

	bool sawDefeated = false;
	for (const dev::CombatEvent &event : frame.combatEvents()) {
		if (event.type == dev::CombatEventType::Defeated && event.target.id == 40)
			sawDefeated = true;
	}
	Expect(sawDefeated, "frame runner defeat sync test should defeat enemy target");
	Expect(world.targets.resolveAtTile({ 1, 0 }).type == dev::TargetType::EmptyTile, "frame runner should remove defeated enemy target");
}

void TestWorldEntityServiceSpawnsEnemyStateAcrossRegistries()
{
	dev::SimulationWorld world;
	dev::WorldEntityService entities;

	dev::Enemy &enemy = entities.spawnEnemy(world, {
	    .id = 500,
	    .tile = { 6, 2 },
	    .tuning = { .maxStepsPerTick = 2, .attackRangeTiles = 1, .attackWindupSeconds = 0.25F, .attackRecoverySeconds = 0.75F },
	    .combatStats = { .hitPoints = 13, .attackPower = 6, .defense = 2 },
	});

	dev::Target target { .type = dev::TargetType::Enemy, .id = 500, .tile = { 6, 2 } };
	const dev::Combatant *combatant = world.combat.registry().find(target);
	dev::Target clickable = world.targets.resolveAtTile({ 6, 2 });

	Expect(world.enemies.size() == 1, "world entity service should add enemy to world");
	Expect(enemy.id == 500 && enemy.position.tile == dev::Point { 6, 2 }, "world entity service should initialize enemy identity and position");
	Expect(enemy.position.future == dev::Point { 6, 2 } && enemy.position.previous == dev::Point { 6, 2 }, "world entity service should initialize full actor position");
	Expect(enemy.tuning.maxStepsPerTick == 2 && enemy.tuning.attackRecoverySeconds == 0.75F, "world entity service should apply enemy tuning");
	Expect(enemy.combatStats.hitPoints == 13 && enemy.combatStats.attackPower == 6, "world entity service should apply enemy combat stats");
	Expect(combatant != nullptr && combatant->stats.hitPoints == 13, "world entity service should register enemy combatant");
	Expect(clickable.type == dev::TargetType::Enemy && clickable.id == 500, "world entity service should register enemy target");
}

void TestWorldEntityServiceDespawnsEnemyStateAcrossRegistries()
{
	dev::SimulationWorld world;
	dev::WorldEntityService entities;
	entities.spawnEnemy(world, {
	    .id = 501,
	    .tile = { 2, 6 },
	    .combatStats = { .hitPoints = 4, .attackPower = 2, .defense = 1 },
	});
	world.targets.add({ .type = dev::TargetType::Object, .id = 77, .tile = { 9, 9 } });
	world.combat.registry().add({
	    .target = { .type = dev::TargetType::Object, .id = 77, .tile = { 9, 9 } },
	    .stats = { .hitPoints = 10, .attackPower = 0, .defense = 0 },
	});

	Expect(entities.despawnEnemy(world, 501), "world entity service should report despawned enemy");

	dev::Target enemyTarget { .type = dev::TargetType::Enemy, .id = 501, .tile = { 2, 6 } };
	dev::Target objectTarget { .type = dev::TargetType::Object, .id = 77, .tile = { 9, 9 } };
	Expect(world.enemies.empty(), "world entity service should remove enemy from world");
	Expect(world.combat.registry().find(enemyTarget) == nullptr, "world entity service should remove enemy combatant");
	Expect(world.targets.resolveAtTile({ 2, 6 }).type == dev::TargetType::EmptyTile, "world entity service should remove enemy target");
	Expect(world.targets.resolveAtTile({ 9, 9 }).type == dev::TargetType::Object, "world entity service should preserve unrelated targets");
	Expect(world.combat.registry().find(objectTarget) != nullptr, "world entity service should preserve unrelated combatants");
	Expect(!entities.despawnEnemy(world, 501), "world entity service should report missing enemy on second despawn");
}

void TestWorldEntityServiceRespawnReplacesStaleEnemyState()
{
	dev::SimulationWorld world;
	dev::WorldEntityService entities;
	entities.spawnEnemy(world, {
	    .id = 502,
	    .tile = { 1, 1 },
	    .combatStats = { .hitPoints = 3, .attackPower = 1, .defense = 0 },
	});
	entities.spawnEnemy(world, {
	    .id = 502,
	    .tile = { 5, 1 },
	    .combatStats = { .hitPoints = 9, .attackPower = 4, .defense = 1 },
	});

	dev::Target newTarget { .type = dev::TargetType::Enemy, .id = 502, .tile = { 5, 1 } };
	const dev::Combatant *combatant = world.combat.registry().find(newTarget);

	Expect(world.enemies.size() == 1, "world entity service respawn should keep one enemy for duplicate id");
	Expect(world.enemies.size() == 1 && world.enemies[0].position.tile == dev::Point { 5, 1 }, "world entity service respawn should update enemy tile");
	Expect(world.targets.resolveAtTile({ 1, 1 }).type == dev::TargetType::EmptyTile, "world entity service respawn should remove old target tile");
	Expect(world.targets.resolveAtTile({ 5, 1 }).type == dev::TargetType::Enemy, "world entity service respawn should add new target tile");
	Expect(world.combat.registry().combatants().size() == 1, "world entity service respawn should keep one combatant for duplicate id");
	Expect(combatant != nullptr && combatant->stats.hitPoints == 9, "world entity service respawn should replace combat stats");
}

void TestWorldEntityServiceSpawnsAndDespawnsItems()
{
	dev::SimulationWorld world;
	dev::WorldEntityService entities;

	dev::Item &item = entities.spawnItem(world, {
	    .id = 600,
	    .tile = { 4, 7 },
	});

	dev::Target clickable = world.targets.resolveAtTile({ 4, 7 });
	Expect(world.items.size() == 1, "world entity service should add item to world");
	Expect(item.id == 600 && item.tile == dev::Point { 4, 7 }, "world entity service should initialize item identity and tile");
	Expect(clickable.type == dev::TargetType::Item && clickable.id == 600, "world entity service should register item target");

	Expect(entities.despawnItem(world, 600), "world entity service should report despawned item");
	Expect(world.items.empty(), "world entity service should remove item from world");
	Expect(world.targets.resolveAtTile({ 4, 7 }).type == dev::TargetType::EmptyTile, "world entity service should remove item target");
	Expect(!entities.despawnItem(world, 600), "world entity service should report missing item on second despawn");
}

void TestWorldEntityServiceRespawnReplacesStaleItemTarget()
{
	dev::SimulationWorld world;
	dev::WorldEntityService entities;
	entities.spawnItem(world, {
	    .id = 601,
	    .tile = { 2, 2 },
	});
	entities.spawnItem(world, {
	    .id = 601,
	    .tile = { 8, 2 },
	});

	Expect(world.items.size() == 1, "world entity service item respawn should keep one item for duplicate id");
	Expect(world.items.size() == 1 && world.items[0].tile == dev::Point { 8, 2 }, "world entity service item respawn should update tile");
	Expect(world.targets.resolveAtTile({ 2, 2 }).type == dev::TargetType::EmptyTile, "world entity service item respawn should remove old target tile");
	Expect(world.targets.resolveAtTile({ 8, 2 }).type == dev::TargetType::Item, "world entity service item respawn should add new target tile");
}

void TestInventoryServiceTransfersExecutedPickupToPlayerInventory()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 1, 0 }));
	dev::WorldEntityService {}.spawnItem(world, {
	    .id = 620,
	    .tile = { 1, 0 },
	});

	dev::MovementEvent pickup {
		.type = dev::MovementEventType::ActionExecuted,
		.playerId = 0,
		.tile = { 1, 0 },
		.actionType = dev::DestinationActionType::Pickup,
		.actionResult = dev::ActionResultType::Executed,
		.target = dev::Target { .type = dev::TargetType::Item, .id = 620, .tile = { 1, 0 } },
	};

	std::vector<dev::InventoryTransferResult> results = dev::InventoryService {}.applyPickupEvents(world, { pickup });

	Expect(results.size() == 1 && results[0].type == dev::InventoryTransferResultType::Transferred, "inventory service should report transferred pickup");
	Expect(world.items.empty(), "inventory service should remove picked item from world");
	Expect(world.targets.resolveAtTile({ 1, 0 }).type == dev::TargetType::EmptyTile, "inventory service should remove picked item target");
	Expect(world.players.size() == 1 && world.players[0].inventory.items.size() == 1, "inventory service should add picked item to player inventory");
	Expect(world.players.size() == 1 && world.players[0].inventory.items[0].id == 620, "inventory service should preserve picked item identity");
}

void TestInventoryServiceIgnoresInvalidPickupEvents()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 0, 0 }));
	dev::WorldEntityService {}.spawnItem(world, {
	    .id = 621,
	    .tile = { 2, 0 },
	});

	dev::MovementEvent missingItem {
		.type = dev::MovementEventType::ActionExecuted,
		.playerId = 0,
		.tile = { 0, 0 },
		.actionType = dev::DestinationActionType::Pickup,
		.actionResult = dev::ActionResultType::Executed,
		.target = dev::Target { .type = dev::TargetType::Item, .id = 999, .tile = { 9, 9 } },
	};
	dev::MovementEvent wrongPlayer {
		.type = dev::MovementEventType::ActionExecuted,
		.playerId = 2,
		.tile = { 0, 0 },
		.actionType = dev::DestinationActionType::Pickup,
		.actionResult = dev::ActionResultType::Executed,
		.target = dev::Target { .type = dev::TargetType::Item, .id = 621, .tile = { 2, 0 } },
	};

	std::vector<dev::InventoryTransferResult> results = dev::InventoryService {}.applyPickupEvents(world, { missingItem, wrongPlayer });

	Expect(results.size() == 2, "inventory service should report invalid pickup attempts");
	Expect(results.size() == 2 && results[0].type == dev::InventoryTransferResultType::MissingItem, "inventory service should report missing item");
	Expect(results.size() == 2 && results[1].type == dev::InventoryTransferResultType::InvalidEvent, "inventory service should report invalid player");
	Expect(world.items.size() == 1, "inventory service should keep world item when pickup event is invalid");
	Expect(world.players[0].inventory.items.empty(), "inventory service should not add invalid pickup to inventory");
	Expect(world.targets.resolveAtTile({ 2, 0 }).type == dev::TargetType::Item, "inventory service should keep target for invalid pickup");
}

void TestInventoryServiceRejectsPickupWhenInventoryIsFull()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 1, 0 }));
	world.players[0].inventory.capacity = 1;
	world.players[0].inventory.items.push_back({ .id = 700, .tile = { 0, 0 } });
	dev::WorldEntityService {}.spawnItem(world, {
	    .id = 701,
	    .tile = { 1, 0 },
	});

	dev::MovementEvent pickup {
		.type = dev::MovementEventType::ActionExecuted,
		.playerId = 0,
		.tile = { 1, 0 },
		.actionType = dev::DestinationActionType::Pickup,
		.actionResult = dev::ActionResultType::Executed,
		.target = dev::Target { .type = dev::TargetType::Item, .id = 701, .tile = { 1, 0 } },
	};

	std::vector<dev::InventoryTransferResult> results = dev::InventoryService {}.applyPickupEvents(world, { pickup });

	Expect(results.size() == 1 && results[0].type == dev::InventoryTransferResultType::RejectedFull, "inventory service should reject pickup when inventory is full");
	Expect(world.items.size() == 1 && world.items[0].id == 701, "full inventory pickup should leave item in world");
	Expect(world.targets.resolveAtTile({ 1, 0 }).type == dev::TargetType::Item, "full inventory pickup should leave item target");
	Expect(world.players[0].inventory.items.size() == 1 && world.players[0].inventory.items[0].id == 700, "full inventory pickup should preserve existing inventory");
}

void TestEquipmentServiceEquipsItemFromInventory()
{
	dev::Inventory inventory;
	inventory.items.push_back({ .id = 900, .equipmentSlot = dev::EquipmentSlot::Weapon });

	dev::EquipmentResult result = dev::EquipmentService {}.equip(inventory, 900);

	Expect(result.type == dev::EquipmentResultType::Equipped, "equipment service should equip equippable item");
	Expect(result.slot == dev::EquipmentSlot::Weapon, "equipment service should report equipped slot");
	Expect(inventory.items.empty(), "equipment service should remove equipped item from bag");
	Expect(inventory.equipment.weapon.has_value() && inventory.equipment.weapon->id == 900, "equipment service should place item in weapon slot");
}

void TestEquipmentServiceSwapsOccupiedSlot()
{
	dev::Inventory inventory;
	inventory.equipment.weapon = dev::Item { .id = 901, .equipmentSlot = dev::EquipmentSlot::Weapon };
	inventory.items.push_back({ .id = 902, .equipmentSlot = dev::EquipmentSlot::Weapon });

	dev::EquipmentResult result = dev::EquipmentService {}.equip(inventory, 902);

	Expect(result.type == dev::EquipmentResultType::Equipped, "equipment service should equip replacement item");
	Expect(inventory.equipment.weapon.has_value() && inventory.equipment.weapon->id == 902, "equipment service should replace occupied slot");
	Expect(inventory.items.size() == 1 && inventory.items[0].id == 901, "equipment service should return old equipment to bag");
}

void TestEquipmentServiceRejectsMissingAndNotEquippableItems()
{
	dev::Inventory inventory;
	inventory.items.push_back({ .id = 903 });

	dev::EquipmentResult missing = dev::EquipmentService {}.equip(inventory, 999);
	dev::EquipmentResult notEquippable = dev::EquipmentService {}.equip(inventory, 903);

	Expect(missing.type == dev::EquipmentResultType::MissingItem, "equipment service should reject missing item");
	Expect(notEquippable.type == dev::EquipmentResultType::NotEquippable, "equipment service should reject item without equipment slot");
	Expect(inventory.items.size() == 1 && inventory.items[0].id == 903, "equipment service should keep rejected item in bag");
	Expect(!inventory.equipment.weapon.has_value(), "equipment service should not equip rejected item");
}

void TestEquipmentServiceUnequipsWhenInventoryHasCapacity()
{
	dev::Inventory inventory;
	inventory.capacity = 2;
	inventory.equipment.armor = dev::Item { .id = 904, .equipmentSlot = dev::EquipmentSlot::Armor };

	dev::EquipmentResult result = dev::EquipmentService {}.unequip(inventory, dev::EquipmentSlot::Armor);

	Expect(result.type == dev::EquipmentResultType::Unequipped, "equipment service should unequip item when bag has capacity");
	Expect(!inventory.equipment.armor.has_value(), "equipment service should clear unequipped slot");
	Expect(inventory.items.size() == 1 && inventory.items[0].id == 904, "equipment service should return unequipped item to bag");
}

void TestEquipmentServiceRejectsUnequipWhenInventoryFull()
{
	dev::Inventory inventory;
	inventory.capacity = 1;
	inventory.items.push_back({ .id = 905 });
	inventory.equipment.accessory = dev::Item { .id = 906, .equipmentSlot = dev::EquipmentSlot::Accessory };

	dev::EquipmentResult result = dev::EquipmentService {}.unequip(inventory, dev::EquipmentSlot::Accessory);

	Expect(result.type == dev::EquipmentResultType::InventoryFull, "equipment service should reject unequip when bag is full");
	Expect(inventory.equipment.accessory.has_value() && inventory.equipment.accessory->id == 906, "equipment service should keep item equipped when unequip rejects");
	Expect(inventory.items.size() == 1 && inventory.items[0].id == 905, "equipment service should preserve full bag contents");
}

void TestEquipmentStatsServiceBuildsEffectiveCombatStats()
{
	dev::Player player = MakePlayer();
	player.combatStats.attackPower = 5;
	player.combatStats.defense = 1;
	player.inventory.items.push_back({
	    .id = 920,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	    .combatModifiers = { .attackPower = 99, .defense = 99 },
	});
	player.inventory.equipment.weapon = dev::Item {
		.id = 921,
		.equipmentSlot = dev::EquipmentSlot::Weapon,
		.combatModifiers = { .attackPower = 3 },
	};
	player.inventory.equipment.armor = dev::Item {
		.id = 922,
		.equipmentSlot = dev::EquipmentSlot::Armor,
		.combatModifiers = { .defense = 2 },
	};
	player.inventory.equipment.accessory = dev::Item {
		.id = 923,
		.equipmentSlot = dev::EquipmentSlot::Accessory,
		.combatModifiers = { .attackPower = 1, .defense = 1 },
	};

	dev::CombatStats effective = dev::EquipmentStatsService {}.effectiveCombatStats(player);

	Expect(effective.hitPoints == player.combatStats.hitPoints, "equipment stats service should preserve current hit points");
	Expect(effective.attackPower == 9, "equipment stats service should add equipped attack modifiers");
	Expect(effective.defense == 4, "equipment stats service should add equipped defense modifiers");
	Expect(player.combatStats.attackPower == 5, "equipment stats service should not mutate base combat stats");
}

void TestInventoryCommandDispatcherEquipsItem()
{
	dev::Player player = MakePlayer();
	player.inventory.items.push_back({
	    .id = 930,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});

	dev::InventoryCommandResult result = dev::InventoryCommandDispatcher { player }.dispatch({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 930,
	});

	Expect(result.type == dev::InventoryCommandResultType::Applied, "inventory command dispatcher should apply valid equip command");
	Expect(result.equipmentResult.type == dev::EquipmentResultType::Equipped, "inventory command dispatcher should expose equipment result");
	Expect(player.inventory.items.empty(), "inventory command dispatcher equip should remove item from bag");
	Expect(player.inventory.equipment.weapon.has_value() && player.inventory.equipment.weapon->id == 930, "inventory command dispatcher equip should fill equipment slot");
}

void TestInventoryCommandDispatcherUnequipsSlot()
{
	dev::Player player = MakePlayer();
	player.inventory.capacity = 2;
	player.inventory.equipment.armor = dev::Item {
		.id = 931,
		.equipmentSlot = dev::EquipmentSlot::Armor,
	};

	dev::InventoryCommandResult result = dev::InventoryCommandDispatcher { player }.dispatch({
	    .type = dev::InventoryCommandType::UnequipSlot,
	    .slot = dev::EquipmentSlot::Armor,
	});

	Expect(result.type == dev::InventoryCommandResultType::Applied, "inventory command dispatcher should apply valid unequip command");
	Expect(result.equipmentResult.type == dev::EquipmentResultType::Unequipped, "inventory command dispatcher should expose unequip result");
	Expect(!player.inventory.equipment.armor.has_value(), "inventory command dispatcher unequip should clear slot");
	Expect(player.inventory.items.size() == 1 && player.inventory.items[0].id == 931, "inventory command dispatcher unequip should return item to bag");
}

void TestInventoryCommandDispatcherRejectsInvalidCommands()
{
	dev::Player player = MakePlayer();
	player.inventory.items.push_back({ .id = 932 });

	dev::InventoryCommandResult missingPayload = dev::InventoryCommandDispatcher { player }.dispatch({
	    .type = dev::InventoryCommandType::EquipItem,
	});
	dev::InventoryCommandResult notEquippable = dev::InventoryCommandDispatcher { player }.dispatch({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 932,
	});
	dev::InventoryCommandResult emptySlot = dev::InventoryCommandDispatcher { player }.dispatch({
	    .type = dev::InventoryCommandType::UnequipSlot,
	    .slot = dev::EquipmentSlot::Accessory,
	});

	Expect(missingPayload.type == dev::InventoryCommandResultType::Rejected, "inventory command dispatcher should reject missing item payload");
	Expect(notEquippable.type == dev::InventoryCommandResultType::Rejected, "inventory command dispatcher should reject failed equip service result");
	Expect(notEquippable.equipmentResult.type == dev::EquipmentResultType::NotEquippable, "inventory command dispatcher should preserve rejected equipment reason");
	Expect(emptySlot.type == dev::InventoryCommandResultType::Rejected, "inventory command dispatcher should reject failed unequip service result");
	Expect(emptySlot.equipmentResult.type == dev::EquipmentResultType::EmptySlot, "inventory command dispatcher should preserve empty slot reason");
	Expect(player.inventory.items.size() == 1 && player.inventory.items[0].id == 932, "inventory command dispatcher should not mutate inventory for rejected commands");
}

void TestInventoryCommandDispatcherEmitsInventoryEvents()
{
	dev::Player player = MakePlayer();
	player.inventory.items.push_back({
	    .id = 933,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});
	player.inventory.items.push_back({ .id = 934 });
	dev::InventoryEventRecorder events;
	dev::InventoryCommandDispatcher dispatcher { player, &events };

	dev::InventoryCommandResult equipped = dispatcher.dispatch({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 933,
	});
	dev::InventoryCommandResult rejected = dispatcher.dispatch({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 934,
	});

	const std::vector<dev::InventoryEvent> &recorded = events.events();
	Expect(equipped.type == dev::InventoryCommandResultType::Applied, "inventory event test equip command should apply");
	Expect(rejected.type == dev::InventoryCommandResultType::Rejected, "inventory event test second equip command should reject");
	Expect(recorded.size() == 2, "inventory command dispatcher should emit one inventory event per command");
	Expect(recorded.size() == 2 && recorded[0].type == dev::InventoryEventType::Equipped, "inventory command dispatcher should emit equipped event");
	Expect(recorded.size() == 2 && recorded[0].commandType == dev::InventoryCommandType::EquipItem, "inventory equipped event should preserve command type");
	Expect(recorded.size() == 2 && recorded[0].commandResult == dev::InventoryCommandResultType::Applied, "inventory equipped event should report applied command");
	Expect(recorded.size() == 2 && recorded[0].equipmentResult == dev::EquipmentResultType::Equipped, "inventory equipped event should preserve equipment result");
	Expect(recorded.size() == 2 && recorded[0].itemId == 933, "inventory equipped event should preserve item id");
	Expect(recorded.size() == 2 && recorded[0].slot == dev::EquipmentSlot::Weapon, "inventory equipped event should preserve slot");
	Expect(recorded.size() == 2 && recorded[1].type == dev::InventoryEventType::Rejected, "inventory command dispatcher should emit rejected event");
	Expect(recorded.size() == 2 && recorded[1].equipmentResult == dev::EquipmentResultType::NotEquippable, "inventory rejected event should preserve rejection reason");
	Expect(recorded.size() == 2 && recorded[1].itemId == 934, "inventory rejected event should preserve rejected item id");
}

void TestInventoryCommandCodecRoundTripsCommands()
{
	dev::InventoryCommandCodec codec;
	std::vector<dev::InventoryCommand> commands {
		{
		    .type = dev::InventoryCommandType::EquipItem,
		    .itemId = 960,
		},
		{
		    .type = dev::InventoryCommandType::UnequipSlot,
		    .slot = dev::EquipmentSlot::Accessory,
		},
	};

	for (const dev::InventoryCommand &command : commands) {
		dev::InventoryCommandPacket packet = codec.toPacket(command);
		dev::InventoryCommandBytes bytes = codec.encode(packet);
		std::optional<dev::InventoryCommandPacket> decodedPacket = codec.decode(bytes);
		Expect(decodedPacket.has_value(), "inventory command packet should decode");
		std::optional<dev::InventoryCommand> decoded = decodedPacket.has_value()
		    ? codec.fromPacket(*decodedPacket)
		    : std::nullopt;
		Expect(decoded.has_value(), "inventory command packet should become command");
		if (!decoded.has_value())
			continue;
		Expect(decoded->type == command.type, "inventory command codec should preserve command type");
		Expect(decoded->itemId == command.itemId, "inventory command codec should preserve item id");
		Expect(decoded->slot == command.slot, "inventory command codec should preserve equipment slot");
	}
}

void TestInventoryCommandCodecRejectsInvalidPackets()
{
	dev::InventoryCommandCodec codec;

	dev::InventoryCommandPacket invalidType {
		.commandType = 99,
	};
	Expect(!codec.fromPacket(invalidType).has_value(), "inventory command codec should reject invalid command type");

	dev::InventoryCommandPacket equipWithoutItem {
		.commandType = static_cast<uint8_t>(dev::InventoryCommandType::EquipItem),
	};
	Expect(!codec.fromPacket(equipWithoutItem).has_value(), "inventory command codec should reject equip without item payload");

	dev::InventoryCommandPacket unequipWithoutSlot {
		.commandType = static_cast<uint8_t>(dev::InventoryCommandType::UnequipSlot),
	};
	Expect(!codec.fromPacket(unequipWithoutSlot).has_value(), "inventory command codec should reject unequip without slot payload");

	dev::InventoryCommandPacket invalidSlot {
		.commandType = static_cast<uint8_t>(dev::InventoryCommandType::UnequipSlot),
		.hasSlot = 1,
		.slot = 99,
	};
	Expect(!codec.fromPacket(invalidSlot).has_value(), "inventory command codec should reject invalid equipment slot");

	dev::InventoryCommandPacket extraPayload {
		.commandType = static_cast<uint8_t>(dev::InventoryCommandType::EquipItem),
		.hasItemId = 1,
		.itemId = 961,
		.hasSlot = 1,
		.slot = static_cast<uint8_t>(dev::EquipmentSlot::Weapon),
	};
	Expect(!codec.fromPacket(extraPayload).has_value(), "inventory command codec should reject unexpected payload fields");

	dev::InventoryCommandPacket invalidBoolean {
		.commandType = static_cast<uint8_t>(dev::InventoryCommandType::EquipItem),
		.hasItemId = 2,
		.itemId = 961,
	};
	Expect(!codec.fromPacket(invalidBoolean).has_value(), "inventory command codec should reject invalid payload flags");

	dev::InventoryCommandBytes shortBytes = codec.encode(codec.toPacket({
	    .type = dev::InventoryCommandType::UnequipSlot,
	    .slot = dev::EquipmentSlot::Armor,
	}));
	shortBytes.pop_back();
	Expect(!codec.decode(shortBytes).has_value(), "inventory command codec should reject wrong byte size");
}

void TestInventoryCommandLogReplaysThroughDispatcher()
{
	dev::Player player = MakePlayer();
	player.inventory.capacity = 3;
	player.inventory.items.push_back({
	    .id = 962,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});
	dev::InventoryEventRecorder events;
	dev::InventoryCommandDispatcher dispatcher { player, &events };
	dev::InventoryCommandReplayer replayer { dispatcher };
	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 962,
	});
	log.record({
	    .type = dev::InventoryCommandType::UnequipSlot,
	    .slot = dev::EquipmentSlot::Weapon,
	});

	std::vector<dev::InventoryCommandResult> results = replayer.replay(log);

	Expect(results.size() == 2, "inventory command log should replay every command");
	Expect(results.size() == 2 && results[0].type == dev::InventoryCommandResultType::Applied, "inventory command replay should apply equip command");
	Expect(results.size() == 2 && results[1].type == dev::InventoryCommandResultType::Applied, "inventory command replay should apply unequip command");
	Expect(!player.inventory.equipment.weapon.has_value(), "inventory command replay should leave weapon slot empty after unequip");
	Expect(player.inventory.items.size() == 1 && player.inventory.items[0].id == 962, "inventory command replay should return unequipped item to bag");
	Expect(events.events().size() == 2, "inventory command replay should emit inventory events");
	Expect(events.events().size() == 2 && events.events()[0].type == dev::InventoryEventType::Equipped, "inventory command replay should emit equipped event");
	Expect(events.events().size() == 2 && events.events()[1].type == dev::InventoryEventType::Unequipped, "inventory command replay should emit unequipped event");
}

void TestInventoryCommandLogCodecRoundTripsAndReplays()
{
	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 963,
	});
	log.record({
	    .type = dev::InventoryCommandType::UnequipSlot,
	    .slot = dev::EquipmentSlot::Weapon,
	});

	dev::InventoryCommandLogCodec codec;
	dev::InventoryCommandLogBytes bytes = codec.encode(log);
	std::optional<dev::InventoryCommandLog> decoded = codec.decode(bytes);
	Expect(decoded.has_value(), "inventory command log codec should decode its own bytes");
	Expect(decoded.has_value() && decoded->commands().size() == 2, "inventory command log codec should preserve command count");
	Expect(decoded.has_value() && decoded->commands()[0].itemId == std::optional<dev::TargetId> { 963 }, "inventory command log codec should preserve equip item id");
	Expect(decoded.has_value() && decoded->commands()[1].slot == std::optional<dev::EquipmentSlot> { dev::EquipmentSlot::Weapon }, "inventory command log codec should preserve unequip slot");

	dev::Player player = MakePlayer();
	player.inventory.capacity = 3;
	player.inventory.items.push_back({
	    .id = 963,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});
	dev::InventoryCommandDispatcher dispatcher { player };
	dev::InventoryCommandReplayer replayer { dispatcher };
	std::vector<dev::InventoryCommandResult> results = decoded.has_value()
	    ? replayer.replay(*decoded)
	    : std::vector<dev::InventoryCommandResult> {};

	Expect(results.size() == 2, "decoded inventory command log should replay");
	Expect(!player.inventory.equipment.weapon.has_value(), "decoded inventory command log should reproduce inventory state");
	Expect(player.inventory.items.size() == 1 && player.inventory.items[0].id == 963, "decoded inventory command log should preserve item ownership");
}

void TestInventoryCommandLogCodecRejectsInvalidBytes()
{
	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 964,
	});

	dev::InventoryCommandLogCodec codec;
	dev::InventoryCommandLogBytes bytes = codec.encode(log);

	dev::InventoryCommandLogBytes badMagic = bytes;
	badMagic[0] = 'X';
	Expect(!codec.decode(badMagic).has_value(), "inventory command log codec should reject bad magic");

	dev::InventoryCommandLogBytes badVersion = bytes;
	badVersion[4] = 2;
	Expect(!codec.decode(badVersion).has_value(), "inventory command log codec should reject bad version");

	dev::InventoryCommandLogBytes truncated = bytes;
	truncated.pop_back();
	Expect(!codec.decode(truncated).has_value(), "inventory command log codec should reject truncated bytes");

	dev::InventoryCommandLogBytes corrupted = bytes;
	corrupted[12] ^= 0x01U;
	Expect(!codec.decode(corrupted).has_value(), "inventory command log codec should reject checksum mismatch");
}

void TestInventoryCommandLogFileStoreSavesLoadsAndReplays()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_inventory_log_file_store_replay_test";
	const std::filesystem::path path = root / "inventory.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);
	std::filesystem::remove(path.string() + ".tmp");

	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 965,
	});
	log.record({
	    .type = dev::InventoryCommandType::UnequipSlot,
	    .slot = dev::EquipmentSlot::Weapon,
	});

	dev::InventoryCommandLogFileStore store;
	Expect(store.save(path, log), "inventory command log file store should save log");
	std::optional<dev::InventoryCommandLog> loaded = store.load(path);

	Expect(loaded.has_value(), "inventory command log file store should load saved log");
	Expect(loaded.has_value() && loaded->commands().size() == 2, "inventory command log file store should preserve command count");
	Expect(!std::filesystem::exists(path.string() + ".tmp"), "inventory command log file store should remove temp file after save");

	dev::Player player = MakePlayer();
	player.inventory.capacity = 3;
	player.inventory.items.push_back({
	    .id = 965,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});
	dev::InventoryEventRecorder events;
	dev::InventoryCommandDispatcher dispatcher { player, &events };
	dev::InventoryCommandReplayer replayer { dispatcher };
	std::vector<dev::InventoryCommandResult> results = loaded.has_value()
	    ? replayer.replay(*loaded)
	    : std::vector<dev::InventoryCommandResult> {};

	Expect(results.size() == 2, "loaded inventory command log should replay");
	Expect(!player.inventory.equipment.weapon.has_value(), "loaded inventory command log should reproduce inventory state");
	Expect(player.inventory.items.size() == 1 && player.inventory.items[0].id == 965, "loaded inventory command log should preserve item ownership");
	Expect(events.events().size() == 2, "loaded inventory command log replay should emit events");

	std::filesystem::remove_all(root);
}

void TestInventoryCommandLogFileStoreRejectsCorruptAndMissingFiles()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_inventory_log_file_store_corrupt_test";
	const std::filesystem::path missingPath = root / "missing.iicl";
	const std::filesystem::path corruptPath = root / "corrupt.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	{
		std::ofstream output { corruptPath, std::ios::binary | std::ios::trunc };
		output << "not an inventory command log";
	}

	dev::InventoryCommandLogFileStore store;
	Expect(!store.load(missingPath).has_value(), "inventory command log file store should reject missing file");
	Expect(!store.load(corruptPath).has_value(), "inventory command log file store should reject corrupt file");

	std::filesystem::remove_all(root);
}

void TestInventoryScriptRunnerRunsSavedInventoryScript()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_inventory_script_runner_test";
	const std::filesystem::path path = root / "inventory.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 966,
	});
	log.record({
	    .type = dev::InventoryCommandType::UnequipSlot,
	    .slot = dev::EquipmentSlot::Weapon,
	});

	dev::InventoryCommandLogFileStore store;
	Expect(store.save(path, log), "inventory script runner test should create script file");

	dev::Player player = MakePlayer();
	player.inventory.capacity = 3;
	player.inventory.items.push_back({
	    .id = 966,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});
	dev::InventoryEventRecorder events;
	dev::InventoryCommandDispatcher dispatcher { player, &events };
	dev::InventoryScriptRunner runner { dispatcher };
	dev::InventoryScriptRunResult result = runner.run(path);

	Expect(result.status == dev::InventoryScriptRunStatus::Completed, "inventory script runner should complete valid script files");
	Expect(result.commandResults.size() == 2, "inventory script runner should return per-command results");
	Expect(result.commandResults.size() == 2 && result.commandResults[0].type == dev::InventoryCommandResultType::Applied, "inventory script runner should apply equip command");
	Expect(result.commandResults.size() == 2 && result.commandResults[1].type == dev::InventoryCommandResultType::Applied, "inventory script runner should apply unequip command");
	Expect(!player.inventory.equipment.weapon.has_value(), "inventory script runner should reproduce unequipped state");
	Expect(player.inventory.items.size() == 1 && player.inventory.items[0].id == 966, "inventory script runner should preserve item ownership");
	Expect(events.events().size() == 2, "inventory script runner should still emit dispatcher events");

	std::filesystem::remove_all(root);
}

void TestInventoryScriptRunnerReportsLoadFailureAndCommandRejectionSeparately()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_inventory_script_runner_failure_test";
	const std::filesystem::path path = root / "inventory.iicl";
	const std::filesystem::path missingPath = root / "missing.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::Player player = MakePlayer();
	player.inventory.items.push_back({ .id = 967 });
	dev::InventoryCommandDispatcher dispatcher { player };
	dev::InventoryScriptRunner runner { dispatcher };

	dev::InventoryScriptRunResult missing = runner.run(missingPath);
	Expect(missing.status == dev::InventoryScriptRunStatus::LoadFailed, "inventory script runner should report missing file load failure");
	Expect(missing.commandResults.empty(), "missing inventory script should not dispatch commands");

	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 967,
	});
	dev::InventoryCommandLogFileStore store;
	Expect(store.save(path, log), "inventory script runner rejection test should create script file");

	dev::InventoryScriptRunResult rejected = runner.run(path);
	Expect(rejected.status == dev::InventoryScriptRunStatus::Completed, "inventory script runner should complete loadable scripts even when commands reject");
	Expect(rejected.commandResults.size() == 1, "inventory script runner should return rejected command result");
	Expect(rejected.commandResults.size() == 1 && rejected.commandResults[0].type == dev::InventoryCommandResultType::Rejected, "inventory script runner should preserve command-level rejection");
	Expect(player.inventory.items.size() == 1 && player.inventory.items[0].id == 967, "rejected inventory script command should not mutate inventory");

	std::filesystem::remove_all(root);
}

void TestSimulationSnapshotRestoresDurableState()
{
	dev::SimulationWorld world;
	dev::EventRecorder movementEvents;
	dev::CombatEventRecorder combatEvents;
	world.movementEvents = &movementEvents;
	world.setCombatEventSink(&combatEvents);
	world.players.push_back(MakePlayer({ 2, 2 }));
	world.players[0].combatStats.hitPoints = 18;
	world.players[0].inventory.capacity = 3;
	world.players[0].inventory.items.push_back({ .id = 52, .tile = { 0, 0 }, .combatModifiers = { .defense = 1 } });
	world.players[0].inventory.equipment.weapon = dev::Item { .id = 53, .equipmentSlot = dev::EquipmentSlot::Weapon, .combatModifiers = { .attackPower = 2 } };
	world.enemies.push_back(MakeEnemy({ 4, 4 }));
	world.enemies[0].moveState = dev::EnemyMoveState::Attacking;
	world.enemies[0].stateTimerSeconds = 0.50F;
	world.items.push_back({ .id = 51, .tile = { 3, 2 } });
	dev::Target target { .type = dev::TargetType::Enemy, .id = 50, .tile = { 4, 4 } };
	world.combat.registry().add({
	    .target = target,
	    .stats = { .hitPoints = 7, .attackPower = 3, .defense = 1 },
	});
	world.targets.add(target);
	world.targets.add({ .type = dev::TargetType::Item, .id = 51, .tile = { 3, 2 } });
	movementEvents.emit({ .type = dev::MovementEventType::StepCommitted, .tile = { 2, 2 } });
	combatEvents.emit({ .type = dev::CombatEventType::Hit, .target = target, .damage = 2, .remainingHitPoints = 7 });

	dev::SimulationSnapshot snapshot = dev::SnapshotWriter {}.write(world);

	world.players[0].position.tile = { 9, 9 };
	world.players[0].combatStats.hitPoints = 1;
	world.players[0].inventory.items.clear();
	world.enemies.clear();
	world.items.clear();
	world.combat.registry().replaceAll({});
	world.targets.clear();
	dev::SnapshotReader {}.read(snapshot, world);

	const dev::Combatant *combatant = world.combat.registry().find(target);
	dev::Target restoredTarget = world.targets.resolveAtTile({ 4, 4 });
	dev::Target restoredItem = world.targets.resolveAtTile({ 3, 2 });
	Expect(world.players.size() == 1 && world.players[0].position.tile == dev::Point { 2, 2 }, "snapshot should restore player position");
	Expect(world.players.size() == 1 && world.players[0].combatStats.hitPoints == 18, "snapshot should restore player combat stats");
	Expect(world.players.size() == 1 && world.players[0].inventory.capacity == 3, "snapshot should restore player inventory capacity");
	Expect(world.players.size() == 1 && world.players[0].inventory.items.size() == 1 && world.players[0].inventory.items[0].id == 52, "snapshot should restore player inventory");
	Expect(world.players.size() == 1 && world.players[0].inventory.equipment.weapon.has_value() && world.players[0].inventory.equipment.weapon->id == 53, "snapshot should restore player equipment");
	Expect(world.players.size() == 1 && world.players[0].inventory.equipment.weapon.has_value() && world.players[0].inventory.equipment.weapon->combatModifiers.attackPower == 2, "snapshot should restore equipment combat modifiers");
	Expect(world.enemies.size() == 1 && world.enemies[0].position.tile == dev::Point { 4, 4 }, "snapshot should restore enemy position");
	Expect(world.enemies.size() == 1 && world.enemies[0].moveState == dev::EnemyMoveState::Attacking, "snapshot should restore enemy state");
	Expect(world.items.size() == 1 && world.items[0].tile == dev::Point { 3, 2 }, "snapshot should restore item state");
	Expect(combatant != nullptr && combatant->stats.hitPoints == 7, "snapshot should restore combat registry state");
	Expect(restoredTarget.type == dev::TargetType::Enemy && restoredTarget.id == 50, "snapshot should restore target registry enemy");
	Expect(restoredItem.type == dev::TargetType::Item && restoredItem.id == 51, "snapshot should restore target registry item");
	Expect(snapshot.players.size() == 1 && snapshot.enemies.size() == 1 && snapshot.items.size() == 1 && snapshot.combatants.size() == 1 && snapshot.targets.size() == 2, "snapshot should contain durable state only");
	Expect(!movementEvents.events().empty() && !combatEvents.events().empty(), "snapshot restore should not manage transient event history");
}

void TestSnapshotCodecRoundTripsVersionedBytes()
{
	dev::SimulationSnapshot snapshot;
	dev::Player player = MakePlayer({ 1, 2 });
	player.moveState = dev::PlayerMoveState::Pathing;
	player.path.pushStep({ 2, 2 });
	player.path.pushStep({ 3, 2 });
	player.destinationAction = {
	    dev::DestinationActionType::Attack,
	    { .type = dev::TargetType::Enemy, .id = 60, .tile = { 3, 2 } },
	    1,
	};
	player.movementModifiers.standGround = true;
	player.animationLock.active = true;
	player.animationLock.elapsedSeconds = 0.25F;
	player.animationLock.cancelAfterSeconds = 0.50F;
	player.combatStats.hitPoints = 11;
	player.inventory.capacity = 2;
	player.inventory.items.push_back({ .id = 62, .tile = { 0, 0 }, .combatModifiers = { .defense = 3 } });
	player.inventory.equipment.weapon = dev::Item { .id = 63, .equipmentSlot = dev::EquipmentSlot::Weapon, .combatModifiers = { .attackPower = 4 } };
	snapshot.players.push_back(player);

	dev::Enemy enemy = MakeEnemy({ 5, 5 });
	enemy.id = 60;
	enemy.moveState = dev::EnemyMoveState::Recovering;
	enemy.tuning.attackWindupSeconds = 0.75F;
	enemy.stateTimerSeconds = 0.25F;
	snapshot.enemies.push_back(enemy);
	snapshot.items.push_back({ .id = 61, .tile = { 6, 5 }, .equipmentSlot = dev::EquipmentSlot::Accessory, .combatModifiers = { .attackPower = 1, .defense = 1 } });
	snapshot.combatants.push_back({
	    .target = { .type = dev::TargetType::Enemy, .id = 60, .tile = { 5, 5 } },
	    .stats = { .hitPoints = 4, .attackPower = 7, .defense = 2 },
	});
	snapshot.targets.push_back({ .type = dev::TargetType::Enemy, .id = 60, .tile = { 5, 5 } });
	snapshot.targets.push_back({ .type = dev::TargetType::Object, .id = 61, .tile = { 6, 5 } });

	dev::SnapshotCodec codec;
	dev::SnapshotBytes bytes = codec.encode(snapshot);
	std::optional<dev::SimulationSnapshot> decoded = codec.decode(bytes);

	Expect(decoded.has_value(), "snapshot codec should decode its own bytes");
	if (!decoded.has_value())
		return;

	Expect(decoded->players.size() == 1, "snapshot codec should preserve player count");
	Expect(decoded->players.size() == 1 && decoded->players[0].position.tile == dev::Point { 1, 2 }, "snapshot codec should preserve player position");
	Expect(decoded->players.size() == 1 && decoded->players[0].path.size() == 2, "snapshot codec should preserve player path length");
	Expect(decoded->players.size() == 1 && decoded->players[0].path.peekNext() == std::optional<dev::Point> { { 2, 2 } }, "snapshot codec should preserve next path step");
	Expect(decoded->players.size() == 1 && decoded->players[0].destinationAction.target.id == 60, "snapshot codec should preserve destination action target");
	Expect(decoded->players.size() == 1 && decoded->players[0].animationLock.active, "snapshot codec should preserve animation lock");
	Expect(decoded->players.size() == 1 && decoded->players[0].inventory.capacity == 2, "snapshot codec should preserve player inventory capacity");
	Expect(decoded->players.size() == 1 && decoded->players[0].inventory.items.size() == 1 && decoded->players[0].inventory.items[0].id == 62, "snapshot codec should preserve player inventory");
	Expect(decoded->players.size() == 1 && decoded->players[0].inventory.items.size() == 1 && decoded->players[0].inventory.items[0].combatModifiers.defense == 3, "snapshot codec should preserve inventory item combat modifiers");
	Expect(decoded->players.size() == 1 && decoded->players[0].inventory.equipment.weapon.has_value() && decoded->players[0].inventory.equipment.weapon->id == 63, "snapshot codec should preserve player equipment");
	Expect(decoded->players.size() == 1 && decoded->players[0].inventory.equipment.weapon.has_value() && decoded->players[0].inventory.equipment.weapon->combatModifiers.attackPower == 4, "snapshot codec should preserve equipment combat modifiers");
	Expect(decoded->enemies.size() == 1 && decoded->enemies[0].moveState == dev::EnemyMoveState::Recovering, "snapshot codec should preserve enemy state");
	Expect(decoded->items.size() == 1 && decoded->items[0].tile == dev::Point { 6, 5 }, "snapshot codec should preserve item state");
	Expect(decoded->items.size() == 1 && decoded->items[0].equipmentSlot == std::optional<dev::EquipmentSlot> { dev::EquipmentSlot::Accessory }, "snapshot codec should preserve floor item equipment slot");
	Expect(decoded->items.size() == 1 && decoded->items[0].combatModifiers.attackPower == 1, "snapshot codec should preserve floor item combat modifiers");
	Expect(decoded->combatants.size() == 1 && decoded->combatants[0].stats.hitPoints == 4, "snapshot codec should preserve combatants");
	Expect(decoded->targets.size() == 2 && decoded->targets[0].type == dev::TargetType::Enemy, "snapshot codec should preserve target registry target type");
	Expect(decoded->targets.size() == 2 && decoded->targets[1].id == 61, "snapshot codec should preserve target registry target id");
}

void TestSnapshotCodecRejectsInvalidBytes()
{
	dev::SnapshotCodec codec;
	dev::SimulationSnapshot snapshot;
	snapshot.players.push_back(MakePlayer());
	dev::SnapshotBytes bytes = codec.encode(snapshot);

	dev::SnapshotBytes badMagic = bytes;
	badMagic[0] = 'X';
	Expect(!codec.decode(badMagic).has_value(), "snapshot codec should reject bad magic");

	dev::SnapshotBytes badVersion = bytes;
	badVersion[4] = 8;
	Expect(!codec.decode(badVersion).has_value(), "snapshot codec should reject unsupported version");

	dev::SnapshotBytes truncated = bytes;
	truncated.pop_back();
	Expect(!codec.decode(truncated).has_value(), "snapshot codec should reject truncated data");

	dev::SnapshotBytes corruptedPayload = bytes;
	corruptedPayload[12] ^= 0x01U;
	Expect(!codec.decode(corruptedPayload).has_value(), "snapshot codec should reject checksum mismatch");
}

void TestSnapshotFileStoreSavesAndLoadsVersionedBytes()
{
	const std::filesystem::path path = std::filesystem::temp_directory_path() / "iggy_snapshot_store_test.bin";
	std::filesystem::remove(path);
	std::filesystem::remove(path.string() + ".tmp");

	dev::SimulationSnapshot snapshot;
	dev::Player player = MakePlayer({ 6, 7 });
	player.combatStats.hitPoints = 13;
	snapshot.players.push_back(player);

	dev::SnapshotFileStore store;
	Expect(store.save(path, snapshot), "snapshot file store should save snapshot bytes");
	std::optional<dev::SimulationSnapshot> loaded = store.load(path);
	Expect(loaded.has_value(), "snapshot file store should load saved bytes");
	Expect(loaded.has_value() && loaded->players.size() == 1 && loaded->players[0].position.tile == dev::Point { 6, 7 }, "loaded snapshot should preserve player position");
	Expect(loaded.has_value() && loaded->players[0].combatStats.hitPoints == 13, "loaded snapshot should preserve player hp");

	std::filesystem::remove(path);
}

void TestSnapshotFileStoreRejectsCorruptFile()
{
	const std::filesystem::path path = std::filesystem::temp_directory_path() / "iggy_snapshot_store_corrupt_test.bin";
	std::filesystem::remove(path);

	{
		std::ofstream output { path, std::ios::binary | std::ios::trunc };
		output << "not a snapshot";
	}

	dev::SnapshotFileStore store;
	Expect(!store.load(path).has_value(), "snapshot file store should reject corrupt files");

	std::filesystem::remove(path);
}

void TestSaveGameServiceSavesAndLoadsWorld()
{
	const std::filesystem::path path = std::filesystem::temp_directory_path() / "iggy_save_game_service_test.bin";
	std::filesystem::remove(path);
	std::filesystem::remove(path.string() + ".tmp");

	dev::SimulationWorld world;
	dev::CombatEventRecorder combatEvents;
	world.setCombatEventSink(&combatEvents);
	world.players.push_back(MakePlayer({ 8, 2 }));
	world.players[0].combatStats.hitPoints = 17;
	world.players[0].inventory.items.push_back({ .id = 73, .tile = { 0, 0 } });
	dev::Target target { .type = dev::TargetType::Enemy, .id = 70, .tile = { 9, 2 } };
	world.combat.registry().add({
	    .target = target,
	    .stats = { .hitPoints = 6, .attackPower = 4, .defense = 1 },
	});
	world.items.push_back({ .id = 72, .tile = { 11, 2 } });
	world.targets.add(target);
	world.targets.add({ .type = dev::TargetType::Object, .id = 71, .tile = { 10, 2 } });
	world.targets.add({ .type = dev::TargetType::Item, .id = 72, .tile = { 11, 2 } });

	dev::SaveGameService saves;
	Expect(saves.saveWorld(path, world), "save game service should save world");

	world.players[0].position.tile = { 0, 0 };
	world.players[0].combatStats.hitPoints = 1;
	world.players[0].inventory.items.clear();
	world.items.clear();
	world.combat.registry().replaceAll({});
	world.targets.clear();
	Expect(saves.loadWorld(path, world), "save game service should load world");

	const dev::Combatant *combatant = world.combat.registry().find(target);
	dev::Target restoredEnemy = world.targets.resolveAtTile({ 9, 2 });
	dev::Target restoredObject = world.targets.resolveAtTile({ 10, 2 });
	dev::Target restoredItem = world.targets.resolveAtTile({ 11, 2 });
	Expect(world.players.size() == 1 && world.players[0].position.tile == dev::Point { 8, 2 }, "save game service should restore player position");
	Expect(world.players.size() == 1 && world.players[0].combatStats.hitPoints == 17, "save game service should restore player hp");
	Expect(world.players.size() == 1 && world.players[0].inventory.items.size() == 1 && world.players[0].inventory.items[0].id == 73, "save game service should restore player inventory");
	Expect(world.items.size() == 1 && world.items[0].id == 72, "save game service should restore item state");
	Expect(combatant != nullptr && combatant->stats.hitPoints == 6, "save game service should restore combat state");
	Expect(restoredEnemy.type == dev::TargetType::Enemy && restoredEnemy.id == 70, "save game service should restore enemy target");
	Expect(restoredObject.type == dev::TargetType::Object && restoredObject.id == 71, "save game service should restore object target");
	Expect(restoredItem.type == dev::TargetType::Item && restoredItem.id == 72, "save game service should restore item target");
	Expect(world.combatEvents == &combatEvents, "save game service should preserve world event sinks");

	std::filesystem::remove(path);
}

void TestSaveSlotServiceListsMetadata()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_save_slot_metadata_test";
	std::filesystem::remove_all(root);

	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 3, 9 }));
	world.players[0].combatStats.hitPoints = 14;
	world.enemies.push_back(MakeEnemy({ 4, 9 }));
	world.items.push_back({ .id = 90, .tile = { 5, 9 } });
	world.targets.add({ .type = dev::TargetType::Item, .id = 90, .tile = { 5, 9 } });

	dev::SaveSlotService slots { root };
	Expect(slots.saveSlot(1, world), "save slot service should save occupied slot");

	{
		std::ofstream corrupt { slots.pathForSlot(2), std::ios::binary | std::ios::trunc };
		corrupt << "corrupt";
	}

	std::vector<dev::SaveSlotMetadata> listed = slots.listSlots(1, 3);
	Expect(listed.size() == 3, "save slot service should list requested slot count");
	Expect(listed.size() == 3 && listed[0].slotId == 1 && listed[0].occupied && listed[0].valid, "saved slot should be occupied and valid");
	Expect(listed.size() == 3 && listed[0].playerTile == dev::Point { 3, 9 }, "slot metadata should include player tile");
	Expect(listed.size() == 3 && listed[0].playerHitPoints == 14, "slot metadata should include player hp");
	Expect(listed.size() == 3 && listed[0].enemyCount == 1, "slot metadata should include enemy count");
	Expect(listed.size() == 3 && listed[1].slotId == 2 && listed[1].occupied && !listed[1].valid, "corrupt slot should be occupied but invalid");
	Expect(listed.size() == 3 && listed[2].slotId == 3 && !listed[2].occupied && !listed[2].valid, "missing slot should be empty and invalid");

	std::filesystem::remove_all(root);
}

void TestSaveSlotServiceLoadsWorld()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_save_slot_load_test";
	std::filesystem::remove_all(root);

	dev::SimulationWorld source;
	source.players.push_back(MakePlayer({ 7, 1 }));
	source.players[0].combatStats.hitPoints = 19;
	source.players[0].inventory.items.push_back({ .id = 82, .tile = { 0, 0 } });
	dev::Target target { .type = dev::TargetType::Enemy, .id = 80, .tile = { 8, 1 } };
	source.combat.registry().add({
	    .target = target,
	    .stats = { .hitPoints = 8, .attackPower = 5, .defense = 1 },
	});
	source.items.push_back({ .id = 81, .tile = { 9, 1 } });
	source.targets.add(target);
	source.targets.add({ .type = dev::TargetType::Item, .id = 81, .tile = { 9, 1 } });

	dev::SaveSlotService slots { root };
	Expect(slots.saveSlot(4, source), "save slot service should save loadable slot");

	dev::SimulationWorld loaded;
	dev::CombatEventRecorder combatEvents;
	loaded.setCombatEventSink(&combatEvents);
	Expect(slots.loadSlot(4, loaded), "save slot service should load saved slot");

	const dev::Combatant *combatant = loaded.combat.registry().find(target);
	dev::Target loadedTarget = loaded.targets.resolveAtTile({ 8, 1 });
	dev::Target loadedItemTarget = loaded.targets.resolveAtTile({ 9, 1 });
	Expect(loaded.players.size() == 1 && loaded.players[0].position.tile == dev::Point { 7, 1 }, "slot load should restore player tile");
	Expect(loaded.players.size() == 1 && loaded.players[0].combatStats.hitPoints == 19, "slot load should restore player hp");
	Expect(loaded.players.size() == 1 && loaded.players[0].inventory.items.size() == 1 && loaded.players[0].inventory.items[0].id == 82, "slot load should restore player inventory");
	Expect(loaded.items.size() == 1 && loaded.items[0].id == 81, "slot load should restore item state");
	Expect(combatant != nullptr && combatant->stats.hitPoints == 8, "slot load should restore combat state");
	Expect(loadedTarget.type == dev::TargetType::Enemy && loadedTarget.id == 80, "slot load should restore target registry");
	Expect(loadedItemTarget.type == dev::TargetType::Item && loadedItemTarget.id == 81, "slot load should restore item target");
	Expect(loaded.combatEvents == &combatEvents, "slot load should preserve world event sinks");

	std::filesystem::remove_all(root);
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

void TestSessionCommandDispatcherAppliesLifecycleCommands()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_command_dispatch_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root };
	dev::SessionCommandDispatcher dispatcher { session };

	dev::SessionCommandResult start = dispatcher.dispatch({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 2, 6 }, .playerHitPoints = 15 },
	});
	Expect(start.type == dev::SessionCommandResultType::Applied, "session command should start new game");
	Expect(session.world().players.size() == 1 && session.world().players[0].position.tile == dev::Point { 2, 6 }, "new game command should apply settings");

	dev::SessionCommandResult save = dispatcher.dispatch({
	    .type = dev::SessionCommandType::SaveSlot,
	    .slotId = 1,
	});
	Expect(save.type == dev::SessionCommandResultType::Applied, "session command should save active slot");

	session.world().players[0].position.tile = { 9, 9 };
	dev::SessionCommandResult load = dispatcher.dispatch({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 1,
	});
	Expect(load.type == dev::SessionCommandResultType::Applied, "session command should load existing slot");
	Expect(session.world().players[0].position.tile == dev::Point { 2, 6 }, "load command should restore saved world");

	dev::SessionCommandResult pause = dispatcher.dispatch({
	    .type = dev::SessionCommandType::SetMode,
	    .mode = dev::GameSessionMode::Paused,
	});
	Expect(pause.type == dev::SessionCommandResultType::Applied, "session command should apply mode change");
	Expect(session.mode() == dev::GameSessionMode::Paused, "mode command should change session mode");

	std::filesystem::remove_all(root);
}

void TestSessionCommandDispatcherRejectsInvalidLifecycleCommands()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_command_reject_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root };
	dev::SessionCommandDispatcher dispatcher { session };

	dev::SessionCommandResult saveEmpty = dispatcher.dispatch({
	    .type = dev::SessionCommandType::SaveSlot,
	    .slotId = 1,
	});
	Expect(saveEmpty.type == dev::SessionCommandResultType::Rejected, "session command should reject saving empty session");

	dev::SessionCommandResult missingLoad = dispatcher.dispatch({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 99,
	});
	Expect(missingLoad.type == dev::SessionCommandResultType::Rejected, "session command should reject missing load slot");
	Expect(!session.hasActiveWorld(), "rejected missing load should leave empty session empty");

	dev::SessionCommandResult missingMode = dispatcher.dispatch({
	    .type = dev::SessionCommandType::SetMode,
	});
	Expect(missingMode.type == dev::SessionCommandResultType::Rejected, "session command should reject missing mode payload");

	std::filesystem::remove_all(root);
}

void TestSessionCommandDispatcherEmitsSuccessEvents()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_event_success_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root };
	dev::SessionEventRecorder events;
	dev::SessionCommandDispatcher dispatcher { session, &events };

	(void)dispatcher.dispatch({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 1, 1 } },
	});
	(void)dispatcher.dispatch({
	    .type = dev::SessionCommandType::SaveSlot,
	    .slotId = 2,
	});
	(void)dispatcher.dispatch({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 2,
	});
	(void)dispatcher.dispatch({
	    .type = dev::SessionCommandType::SetMode,
	    .mode = dev::GameSessionMode::Inventory,
	});

	const std::vector<dev::SessionEvent> &recorded = events.events();
	Expect(recorded.size() == 4, "session dispatcher should emit success lifecycle events");
	Expect(recorded.size() == 4 && recorded[0].type == dev::SessionEventType::GameStarted, "start command should emit GameStarted");
	Expect(recorded.size() == 4 && recorded[1].type == dev::SessionEventType::SaveCompleted, "save command should emit SaveCompleted");
	Expect(recorded.size() == 4 && recorded[1].slotId == std::optional<dev::SaveSlotId> { 2 }, "save event should include slot id");
	Expect(recorded.size() == 4 && recorded[2].type == dev::SessionEventType::LoadCompleted, "load command should emit LoadCompleted");
	Expect(recorded.size() == 4 && recorded[3].type == dev::SessionEventType::ModeChanged, "mode command should emit ModeChanged");
	Expect(recorded.size() == 4 && recorded[3].mode == std::optional<dev::GameSessionMode> { dev::GameSessionMode::Inventory }, "mode event should include target mode");

	std::filesystem::remove_all(root);
}

void TestSessionCommandDispatcherEmitsFailureEvents()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_event_failure_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root };
	dev::SessionEventRecorder events;
	dev::SessionCommandDispatcher dispatcher { session, &events };

	(void)dispatcher.dispatch({
	    .type = dev::SessionCommandType::SaveSlot,
	    .slotId = 3,
	});
	(void)dispatcher.dispatch({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 99,
	});
	(void)dispatcher.dispatch({
	    .type = dev::SessionCommandType::SetMode,
	});

	const std::vector<dev::SessionEvent> &recorded = events.events();
	Expect(recorded.size() == 3, "session dispatcher should emit failure lifecycle events");
	Expect(recorded.size() == 3 && recorded[0].type == dev::SessionEventType::SaveFailed, "empty save command should emit SaveFailed");
	Expect(recorded.size() == 3 && recorded[0].slotId == std::optional<dev::SaveSlotId> { 3 }, "save failure should include slot id");
	Expect(recorded.size() == 3 && recorded[1].type == dev::SessionEventType::LoadFailed, "missing load command should emit LoadFailed");
	Expect(recorded.size() == 3 && recorded[1].slotId == std::optional<dev::SaveSlotId> { 99 }, "load failure should include slot id");
	Expect(recorded.size() == 3 && recorded[2].type == dev::SessionEventType::ModeChangeRejected, "missing mode command should emit ModeChangeRejected");

	std::filesystem::remove_all(root);
}

void TestSessionCommandReplayAppliesLifecycleSequence()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_replay_sequence_test";
	std::filesystem::remove_all(root);

	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 3, 4 }, .playerHitPoints = 12 },
	});
	log.record({
	    .type = dev::SessionCommandType::SaveSlot,
	    .slotId = 1,
	});
	log.record({
	    .type = dev::SessionCommandType::SetMode,
	    .mode = dev::GameSessionMode::Paused,
	});
	log.record({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 1,
	});

	dev::GameSession session { root };
	dev::SessionEventRecorder events;
	dev::SessionCommandDispatcher dispatcher { session, &events };
	dev::SessionCommandReplayer replayer { dispatcher };
	std::vector<dev::SessionCommandResult> results = replayer.replay(log);

	Expect(!log.empty(), "session command log should record commands");
	Expect(results.size() == 4, "session replay should return one result per command");
	Expect(results.size() == 4 && results[0].type == dev::SessionCommandResultType::Applied, "replay should apply start command");
	Expect(results.size() == 4 && results[1].type == dev::SessionCommandResultType::Applied, "replay should apply save command");
	Expect(results.size() == 4 && results[2].type == dev::SessionCommandResultType::Applied, "replay should apply mode command");
	Expect(results.size() == 4 && results[3].type == dev::SessionCommandResultType::Applied, "replay should apply load command");
	Expect(session.world().players.size() == 1 && session.world().players[0].position.tile == dev::Point { 3, 4 }, "session replay should restore final player position");
	Expect(session.world().players.size() == 1 && session.world().players[0].combatStats.hitPoints == 12, "session replay should restore final player hp");
	Expect(session.mode() == dev::GameSessionMode::Gameplay, "load command in replay should return session to gameplay");
	Expect(events.events().size() == 4, "session replay should emit lifecycle events");
	Expect(events.events().size() == 4 && events.events()[0].type == dev::SessionEventType::GameStarted, "session replay should emit start event");
	Expect(events.events().size() == 4 && events.events()[3].type == dev::SessionEventType::LoadCompleted, "session replay should emit load event");

	std::filesystem::remove_all(root);
}

void TestSessionCommandReplayReportsRejectedCommands()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_replay_reject_test";
	std::filesystem::remove_all(root);

	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 99,
	});
	log.record({
	    .type = dev::SessionCommandType::SetMode,
	});

	dev::GameSession session { root };
	dev::SessionEventRecorder events;
	dev::SessionCommandDispatcher dispatcher { session, &events };
	dev::SessionCommandReplayer replayer { dispatcher };
	std::vector<dev::SessionCommandResult> results = replayer.replay(log);

	Expect(results.size() == 2, "session replay should report rejected command results");
	Expect(results.size() == 2 && results[0].type == dev::SessionCommandResultType::Rejected, "session replay should reject missing load");
	Expect(results.size() == 2 && results[1].type == dev::SessionCommandResultType::Rejected, "session replay should reject malformed mode command");
	Expect(!session.hasActiveWorld(), "replayed rejected lifecycle commands should not create active world");
	Expect(events.events().size() == 2, "replayed rejected lifecycle commands should emit failure events");
	Expect(events.events().size() == 2 && events.events()[0].type == dev::SessionEventType::LoadFailed, "replayed missing load should emit LoadFailed");
	Expect(events.events().size() == 2 && events.events()[1].type == dev::SessionEventType::ModeChangeRejected, "replayed malformed mode should emit ModeChangeRejected");

	std::filesystem::remove_all(root);
}

void TestSessionCommandCodecRoundTripsCommands()
{
	dev::SessionCommandCodec codec;
	std::vector<dev::SessionCommand> commands {
		{
		    .type = dev::SessionCommandType::StartNewGame,
		    .newGameSettings = dev::NewGameSettings { .playerStart = { 6, 8 }, .playerHitPoints = 17 },
		},
		{
		    .type = dev::SessionCommandType::SaveSlot,
		    .slotId = 3,
		},
		{
		    .type = dev::SessionCommandType::LoadSlot,
		    .slotId = 4,
		},
		{
		    .type = dev::SessionCommandType::SetMode,
		    .mode = dev::GameSessionMode::Inventory,
		},
	};

	for (const dev::SessionCommand &command : commands) {
		dev::SessionCommandPacket packet = codec.toPacket(command);
		dev::SessionCommandBytes bytes = codec.encode(packet);
		std::optional<dev::SessionCommandPacket> decodedPacket = codec.decode(bytes);
		Expect(decodedPacket.has_value(), "session command packet should decode");
		std::optional<dev::SessionCommand> decoded = decodedPacket.has_value()
		    ? codec.fromPacket(*decodedPacket)
		    : std::nullopt;
		Expect(decoded.has_value(), "session command packet should become command");
		if (!decoded.has_value())
			continue;
		Expect(decoded->type == command.type, "session command codec should preserve command type");
		Expect(decoded->newGameSettings.has_value() == command.newGameSettings.has_value(), "session command codec should preserve new-game payload presence");
		Expect(decoded->slotId == command.slotId, "session command codec should preserve slot id");
		Expect(decoded->mode == command.mode, "session command codec should preserve mode");
		if (command.newGameSettings.has_value()) {
			Expect(decoded->newGameSettings->playerStart == command.newGameSettings->playerStart, "session command codec should preserve player start");
			Expect(decoded->newGameSettings->playerHitPoints == command.newGameSettings->playerHitPoints, "session command codec should preserve player hp");
		}
	}
}

void TestSessionCommandCodecRejectsInvalidPackets()
{
	dev::SessionCommandCodec codec;

	dev::SessionCommandPacket invalidType {
		.commandType = 99,
	};
	Expect(!codec.fromPacket(invalidType).has_value(), "session command codec should reject invalid command type");

	dev::SessionCommandPacket saveWithoutSlot {
		.commandType = static_cast<uint8_t>(dev::SessionCommandType::SaveSlot),
	};
	Expect(!codec.fromPacket(saveWithoutSlot).has_value(), "session command codec should reject save command without slot");

	dev::SessionCommandPacket modeWithoutPayload {
		.commandType = static_cast<uint8_t>(dev::SessionCommandType::SetMode),
	};
	Expect(!codec.fromPacket(modeWithoutPayload).has_value(), "session command codec should reject mode command without mode payload");

	dev::SessionCommandPacket invalidMode {
		.commandType = static_cast<uint8_t>(dev::SessionCommandType::SetMode),
		.hasMode = 1,
		.mode = 99,
	};
	Expect(!codec.fromPacket(invalidMode).has_value(), "session command codec should reject invalid mode");

	dev::SessionCommandPacket extraPayload {
		.commandType = static_cast<uint8_t>(dev::SessionCommandType::LoadSlot),
		.hasNewGameSettings = 1,
		.hasSlotId = 1,
		.slotId = 5,
	};
	Expect(!codec.fromPacket(extraPayload).has_value(), "session command codec should reject unexpected payload fields");

	dev::SessionCommandBytes shortBytes = codec.encode(codec.toPacket({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 1,
	}));
	shortBytes.pop_back();
	Expect(!codec.decode(shortBytes).has_value(), "session command codec should reject wrong byte size");
}

void TestSessionCommandLogCodecRoundTripsAndReplays()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_log_codec_replay_test";
	std::filesystem::remove_all(root);

	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 10, 4 }, .playerHitPoints = 14 },
	});
	log.record({
	    .type = dev::SessionCommandType::SaveSlot,
	    .slotId = 5,
	});
	log.record({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 5,
	});

	dev::SessionCommandLogCodec codec;
	dev::SessionCommandLogBytes bytes = codec.encode(log);
	std::optional<dev::SessionCommandLog> decoded = codec.decode(bytes);
	Expect(decoded.has_value(), "session command log codec should decode its own bytes");
	Expect(decoded.has_value() && decoded->commands().size() == 3, "session command log codec should preserve command count");
	Expect(decoded.has_value() && decoded->commands()[0].newGameSettings->playerStart == dev::Point { 10, 4 }, "session command log codec should preserve new-game settings");
	Expect(decoded.has_value() && decoded->commands()[1].slotId == std::optional<dev::SaveSlotId> { 5 }, "session command log codec should preserve save slot");

	dev::GameSession session { root };
	dev::SessionCommandDispatcher dispatcher { session };
	dev::SessionCommandReplayer replayer { dispatcher };
	std::vector<dev::SessionCommandResult> results = decoded.has_value()
	    ? replayer.replay(*decoded)
	    : std::vector<dev::SessionCommandResult> {};

	Expect(results.size() == 3, "decoded session command log should replay");
	Expect(session.world().players.size() == 1 && session.world().players[0].position.tile == dev::Point { 10, 4 }, "decoded session command log should reproduce session state");
	Expect(session.world().players.size() == 1 && session.world().players[0].combatStats.hitPoints == 14, "decoded session command log should reproduce player hp");

	std::filesystem::remove_all(root);
}

void TestSessionCommandLogCodecRejectsInvalidBytes()
{
	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 1, 1 } },
	});

	dev::SessionCommandLogCodec codec;
	dev::SessionCommandLogBytes bytes = codec.encode(log);

	dev::SessionCommandLogBytes badMagic = bytes;
	badMagic[0] = 'X';
	Expect(!codec.decode(badMagic).has_value(), "session command log codec should reject bad magic");

	dev::SessionCommandLogBytes badVersion = bytes;
	badVersion[4] = 2;
	Expect(!codec.decode(badVersion).has_value(), "session command log codec should reject bad version");

	dev::SessionCommandLogBytes truncated = bytes;
	truncated.pop_back();
	Expect(!codec.decode(truncated).has_value(), "session command log codec should reject truncated bytes");

	dev::SessionCommandLogBytes corrupted = bytes;
	corrupted[12] ^= 0x01U;
	Expect(!codec.decode(corrupted).has_value(), "session command log codec should reject checksum mismatch");
}

void TestSessionCommandLogFileStoreSavesLoadsAndReplays()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_log_file_store_replay_test";
	const std::filesystem::path path = root / "boot.iscl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);
	std::filesystem::remove(path.string() + ".tmp");

	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 4, 11 }, .playerHitPoints = 16 },
	});
	log.record({
	    .type = dev::SessionCommandType::SetMode,
	    .mode = dev::GameSessionMode::Paused,
	});
	log.record({
	    .type = dev::SessionCommandType::SetMode,
	    .mode = dev::GameSessionMode::Gameplay,
	});

	dev::SessionCommandLogFileStore store;
	Expect(store.save(path, log), "session command log file store should save log bytes");
	std::optional<dev::SessionCommandLog> loaded = store.load(path);
	Expect(loaded.has_value(), "session command log file store should load saved log");
	Expect(loaded.has_value() && loaded->commands().size() == 3, "loaded session command log should preserve command count");

	dev::GameSession session { root / "saves" };
	dev::SessionCommandDispatcher dispatcher { session };
	dev::SessionCommandReplayer replayer { dispatcher };
	std::vector<dev::SessionCommandResult> results = loaded.has_value()
	    ? replayer.replay(*loaded)
	    : std::vector<dev::SessionCommandResult> {};

	Expect(results.size() == 3, "loaded session command log should replay");
	Expect(session.mode() == dev::GameSessionMode::Gameplay, "loaded session command log should reproduce final session mode");
	Expect(session.world().players.size() == 1 && session.world().players[0].position.tile == dev::Point { 4, 11 }, "loaded session command log should reproduce player start");
	Expect(session.world().players.size() == 1 && session.world().players[0].combatStats.hitPoints == 16, "loaded session command log should reproduce player hp");

	std::filesystem::remove_all(root);
}

void TestSessionCommandLogFileStoreRejectsCorruptAndMissingFiles()
{
	const std::filesystem::path path = std::filesystem::temp_directory_path() / "iggy_session_log_file_store_corrupt_test.iscl";
	const std::filesystem::path missingPath = std::filesystem::temp_directory_path() / "iggy_session_log_file_store_missing_test.iscl";
	std::filesystem::remove(path);
	std::filesystem::remove(missingPath);

	{
		std::ofstream output { path, std::ios::binary | std::ios::trunc };
		output << "not a session command log";
	}

	dev::SessionCommandLogFileStore store;
	Expect(!store.load(path).has_value(), "session command log file store should reject corrupt files");
	Expect(!store.load(missingPath).has_value(), "session command log file store should return empty for missing files");

	std::filesystem::remove(path);
}

void TestSessionScriptRunnerRunsSavedLifecycleScript()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_script_runner_test";
	const std::filesystem::path path = root / "script.iscl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 12, 6 }, .playerHitPoints = 21 },
	});
	log.record({
	    .type = dev::SessionCommandType::SaveSlot,
	    .slotId = 2,
	});
	log.record({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 2,
	});

	dev::SessionCommandLogFileStore store;
	Expect(store.save(path, log), "session script runner test should create script file");

	dev::GameSession session { root / "saves" };
	dev::SessionEventRecorder events;
	dev::SessionCommandDispatcher dispatcher { session, &events };
	dev::SessionScriptRunner runner { dispatcher };
	dev::SessionScriptRunResult result = runner.run(path);

	Expect(result.status == dev::SessionScriptRunStatus::Completed, "session script runner should complete valid script files");
	Expect(result.commandResults.size() == 3, "session script runner should return per-command results");
	Expect(result.commandResults.size() == 3 && result.commandResults[0].type == dev::SessionCommandResultType::Applied, "session script runner should apply new-game command");
	Expect(result.commandResults.size() == 3 && result.commandResults[1].type == dev::SessionCommandResultType::Applied, "session script runner should apply save command");
	Expect(result.commandResults.size() == 3 && result.commandResults[2].type == dev::SessionCommandResultType::Applied, "session script runner should apply load command");
	Expect(session.world().players.size() == 1 && session.world().players[0].position.tile == dev::Point { 12, 6 }, "session script runner should reproduce player position");
	Expect(session.world().players.size() == 1 && session.world().players[0].combatStats.hitPoints == 21, "session script runner should reproduce player hp");
	Expect(events.events().size() == 3, "session script runner should still emit dispatcher events");

	std::filesystem::remove_all(root);
}

void TestSessionScriptRunnerReportsLoadFailureAndCommandRejectionSeparately()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_script_runner_failure_test";
	const std::filesystem::path path = root / "script.iscl";
	const std::filesystem::path missingPath = root / "missing.iscl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::GameSession session { root / "saves" };
	dev::SessionCommandDispatcher dispatcher { session };
	dev::SessionScriptRunner runner { dispatcher };

	dev::SessionScriptRunResult missing = runner.run(missingPath);
	Expect(missing.status == dev::SessionScriptRunStatus::LoadFailed, "session script runner should report missing file load failure");
	Expect(missing.commandResults.empty(), "missing session script should not dispatch commands");

	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 99,
	});
	dev::SessionCommandLogFileStore store;
	Expect(store.save(path, log), "session script runner rejection test should create script file");

	dev::SessionScriptRunResult rejected = runner.run(path);
	Expect(rejected.status == dev::SessionScriptRunStatus::Completed, "session script runner should complete loadable scripts even when commands reject");
	Expect(rejected.commandResults.size() == 1, "session script runner should return rejected command result");
	Expect(rejected.commandResults.size() == 1 && rejected.commandResults[0].type == dev::SessionCommandResultType::Rejected, "session script runner should preserve command-level rejection");

	std::filesystem::remove_all(root);
}

void TestGameLoopRunsStartupScriptAndFrames()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_startup_script_test";
	const std::filesystem::path scriptPath = root / "startup.iscl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 2, 13 }, .playerHitPoints = 18 },
	});

	dev::SessionCommandLogFileStore store;
	Expect(store.save(scriptPath, log), "game loop startup test should create script file");

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .setup = { .startupScript = scriptPath },
		    .frame = { .maxFrames = 2, .fixedDeltaSeconds = 1.0F / 30.0F },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	Expect(result.setup.startupScriptRan, "game loop should run configured startup script");
	Expect(result.setup.startupScriptResult.status == dev::SessionScriptRunStatus::Completed, "game loop should report completed startup script");
	Expect(result.setup.startupScriptResult.commandResults.size() == 1, "game loop should expose startup command results");
	Expect(result.summary.framesRun == 2, "game loop should run configured frame count");
	Expect(result.finalMode == dev::GameSessionMode::Gameplay, "game loop should report final session mode");
	Expect(loop.session().world().players.size() == 1 && loop.session().world().players[0].position.tile == dev::Point { 2, 13 }, "game loop startup script should initialize session world");
	Expect(loop.session().world().players.size() == 1 && loop.session().world().players[0].combatStats.hitPoints == 18, "game loop startup script should initialize player hp");
	Expect(loop.sessionEvents().events().size() == 1, "game loop should keep startup session events observable");

	std::filesystem::remove_all(root);
}

void TestGameLoopReportsStartupScriptLoadFailure()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_startup_failure_test";
	const std::filesystem::path scriptPath = root / "missing.iscl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .setup = { .startupScript = scriptPath },
		    .frame = { .maxFrames = 3 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	Expect(result.setup.startupScriptRan, "game loop should attempt configured startup script");
	Expect(result.setup.startupScriptResult.status == dev::SessionScriptRunStatus::LoadFailed, "game loop should report startup script load failure");
	Expect(result.setup.startupScriptResult.commandResults.empty(), "failed startup script should not dispatch commands");
	Expect(result.summary.framesRun == 0, "game loop should not run frames after failed startup script");
	Expect(result.finalMode == dev::GameSessionMode::Empty, "failed startup script should leave session empty");
	Expect(loop.run() == 1, "game loop run should return failure exit code for missing startup script");

	std::filesystem::remove_all(root);
}

void TestGameLoopRunsInventoryScriptAgainstActivePlayer()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_inventory_script_test";
	const std::filesystem::path scriptPath = root / "inventory.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 970,
	});
	dev::InventoryCommandLogFileStore store;
	Expect(store.save(scriptPath, log), "game loop inventory script test should create script file");

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .setup = { .inventoryScript = scriptPath },
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	loop.session().world().players[0].inventory.items.push_back({
	    .id = 970,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});

	dev::GameLoopResult result = loop.runForResult();

	Expect(result.setup.inventoryScriptRan, "game loop should run configured inventory script");
	Expect(result.setup.inventoryScriptResult.status == dev::InventoryScriptRunStatus::Completed, "game loop should report completed inventory script");
	Expect(result.setup.inventoryScriptResult.commandResults.size() == 1, "game loop should expose inventory script command results");
	Expect(result.summary.inventoryCommandResults.size() == 1, "game loop should merge inventory script command results into loop results");
	Expect(loop.session().world().players[0].inventory.items.empty(), "game loop inventory script should remove equipped item from bag");
	Expect(loop.session().world().players[0].inventory.equipment.weapon.has_value() && loop.session().world().players[0].inventory.equipment.weapon->id == 970, "game loop inventory script should equip item");
	Expect(loop.inventoryEvents().events().size() == 1 && loop.inventoryEvents().events()[0].type == dev::InventoryEventType::Equipped, "game loop inventory script should emit inventory event");

	std::filesystem::remove_all(root);
}

void TestGameLoopRunsInventoryScriptAfterStartupScript()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_startup_then_inventory_script_test";
	const std::filesystem::path startupPath = root / "startup.iscl";
	const std::filesystem::path inventoryPath = root / "inventory.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::SessionCommandLog startup;
	startup.record({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 4, 9 }, .playerHitPoints = 16 },
	});
	dev::SessionCommandLogFileStore sessionStore;
	Expect(sessionStore.save(startupPath, startup), "startup then inventory test should create startup script");

	dev::InventoryCommandLog inventory;
	inventory.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 971,
	});
	dev::InventoryCommandLogFileStore inventoryStore;
	Expect(inventoryStore.save(inventoryPath, inventory), "startup then inventory test should create inventory script");

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .setup = { .startupScript = startupPath, .inventoryScript = inventoryPath },
		    .frame = { .maxFrames = 1 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	Expect(result.setup.startupScriptRan, "game loop should run startup script before inventory script");
	Expect(result.setup.startupScriptResult.status == dev::SessionScriptRunStatus::Completed, "startup then inventory test should complete startup script");
	Expect(result.setup.inventoryScriptRan, "game loop should run inventory script after startup creates world");
	Expect(result.setup.inventoryScriptResult.status == dev::InventoryScriptRunStatus::Completed, "inventory script should complete when startup created player");
	Expect(result.setup.inventoryScriptResult.commandResults.size() == 1 && result.setup.inventoryScriptResult.commandResults[0].type == dev::InventoryCommandResultType::Rejected, "inventory script should preserve command rejection after load");
	Expect(result.summary.framesRun == 1, "game loop should continue frames after loadable inventory script command rejects");
	Expect(loop.session().world().players.size() == 1 && loop.session().world().players[0].position.tile == dev::Point { 4, 9 }, "startup should initialize world before inventory script");
	Expect(loop.inventoryEvents().events().size() == 1 && loop.inventoryEvents().events()[0].equipmentResult == dev::EquipmentResultType::MissingItem, "inventory script rejection should emit event");

	std::filesystem::remove_all(root);
}

void TestGameLoopReportsInventoryScriptLoadFailure()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_inventory_script_load_failure_test";
	const std::filesystem::path scriptPath = root / "missing.iicl";
	std::filesystem::remove_all(root);

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .setup = { .inventoryScript = scriptPath },
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });

	dev::GameLoopResult result = loop.runForResult();

	Expect(result.setup.inventoryScriptRan, "game loop should attempt configured inventory script");
	Expect(result.setup.inventoryScriptResult.status == dev::InventoryScriptRunStatus::LoadFailed, "game loop should report inventory script load failure");
	Expect(result.setup.inventoryScriptResult.commandResults.empty(), "failed inventory script should not dispatch commands");
	Expect(result.summary.framesRun == 0, "game loop should stop before frames when configured inventory script cannot load");

	std::filesystem::remove_all(root);
}

void TestGameLoopReportsInventoryScriptWithoutActivePlayer()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_inventory_script_no_player_test";
	const std::filesystem::path scriptPath = root / "inventory.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::InventoryCommandLog inventory;
	inventory.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 972,
	});
	dev::InventoryCommandLogFileStore store;
	Expect(store.save(scriptPath, inventory), "no player inventory script test should create script file");

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .setup = { .inventoryScript = scriptPath },
		    .frame = { .maxFrames = 1 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	Expect(result.setup.inventoryScriptRan, "game loop should notice configured inventory script");
	Expect(result.setup.inventoryScriptResult.status == dev::InventoryScriptRunStatus::NoActivePlayer, "game loop should report inventory script without active player");
	Expect(result.setup.inventoryScriptResult.commandResults.empty(), "inventory script without player should not dispatch commands");
	Expect(result.summary.framesRun == 0, "game loop should stop before frames when inventory script has no active player");

	std::filesystem::remove_all(root);
}

void TestQueuedSessionCommandSourceDrainsCommandsOnce()
{
	dev::QueuedSessionCommandSource source;
	source.enqueue({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 5, 5 } },
	});
	source.enqueue({
	    .type = dev::SessionCommandType::SetMode,
	    .mode = dev::GameSessionMode::Paused,
	});

	Expect(source.size() == 2, "queued session command source should track queued command count");
	std::vector<dev::SessionCommand> drained = source.drain();
	Expect(drained.size() == 2, "queued session command source should drain queued commands");
	Expect(source.empty(), "queued session command source should be empty after drain");
	Expect(source.drain().empty(), "queued session command source should not drain commands twice");
	Expect(drained.size() == 2 && drained[0].newGameSettings->playerStart == dev::Point { 5, 5 }, "queued session command source should preserve command payloads");
}

void TestGameLoopDrainsRuntimeSessionCommandSources()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_runtime_command_source_test";
	std::filesystem::remove_all(root);

	dev::QueuedSessionCommandSource source;
	source.enqueue({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 9, 3 }, .playerHitPoints = 22 },
	});
	source.enqueue({
	    .type = dev::SessionCommandType::SetMode,
	    .mode = dev::GameSessionMode::Inventory,
	});

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = { .sessionCommandSources = { &source } },
		    .frame = { .maxFrames = 1 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	Expect(!result.setup.startupScriptRan, "runtime command source test should not run startup script");
	Expect(result.summary.sessionCommandResults.size() == 2, "game loop should dispatch runtime session commands");
	Expect(result.summary.sessionCommandResults.size() == 2 && result.summary.sessionCommandResults[0].type == dev::SessionCommandResultType::Applied, "game loop should apply runtime new-game command");
	Expect(result.summary.sessionCommandResults.size() == 2 && result.summary.sessionCommandResults[1].type == dev::SessionCommandResultType::Applied, "game loop should apply runtime mode command");
	Expect(result.summary.framesRun == 1, "game loop should still run frame after runtime commands");
	Expect(loop.session().world().players.size() == 1 && loop.session().world().players[0].position.tile == dev::Point { 9, 3 }, "runtime command source should initialize session world");
	Expect(loop.session().world().players.size() == 1 && loop.session().world().players[0].combatStats.hitPoints == 22, "runtime command source should preserve new-game settings");
	Expect(result.finalMode == dev::GameSessionMode::Inventory, "runtime command source should set final session mode");
	Expect(source.empty(), "game loop should drain runtime source commands once");
	Expect(loop.sessionEvents().events().size() == 2, "game loop should emit events for runtime source commands");

	std::filesystem::remove_all(root);
}

void TestGameLoopRunsStartupScriptBeforeRuntimeCommandSources()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_startup_then_source_test";
	const std::filesystem::path scriptPath = root / "startup.iscl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 1, 8 }, .playerHitPoints = 12 },
	});

	dev::SessionCommandLogFileStore store;
	Expect(store.save(scriptPath, log), "game loop startup/source test should create startup script");

	dev::QueuedSessionCommandSource source;
	source.enqueue({
	    .type = dev::SessionCommandType::SetMode,
	    .mode = dev::GameSessionMode::Paused,
	});

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .setup = { .startupScript = scriptPath },
		    .sources = { .sessionCommandSources = { &source } },
		    .frame = { .maxFrames = 1 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	Expect(result.setup.startupScriptResult.commandResults.size() == 1, "startup script should run before runtime sources");
	Expect(result.summary.sessionCommandResults.size() == 1, "runtime source should run after startup script");
	Expect(result.summary.sessionCommandResults.size() == 1 && result.summary.sessionCommandResults[0].type == dev::SessionCommandResultType::Applied, "runtime mode command should apply after startup creates a world");
	Expect(loop.session().world().players.size() == 1 && loop.session().world().players[0].position.tile == dev::Point { 1, 8 }, "startup script should initialize world before runtime mode command");
	Expect(result.finalMode == dev::GameSessionMode::Paused, "runtime command should be able to change mode after startup");
	Expect(loop.sessionEvents().events().size() == 2, "startup and runtime commands should share the same session event sink");

	std::filesystem::remove_all(root);
}

void TestQueuedMovementCommandSourceDrainsCommandsOnce()
{
	dev::QueuedMovementCommandSource source;
	source.enqueue({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 3, 4 },
	});
	source.enqueue({
	    .type = dev::MovementCommandType::Stop,
	    .playerId = 0,
	    .destination = { 0, 0 },
	});

	Expect(source.size() == 2, "queued movement command source should track queued command count");
	std::vector<dev::MovementCommand> drained = source.drain();
	Expect(drained.size() == 2, "queued movement command source should drain queued commands");
	Expect(source.empty(), "queued movement command source should be empty after drain");
	Expect(source.drain().empty(), "queued movement command source should not drain commands twice");
	Expect(drained.size() == 2 && drained[0].destination == dev::Point { 3, 4 }, "queued movement command source should preserve command payloads");
}

void TestQueuedInventoryCommandSourceDrainsCommandsOnce()
{
	dev::QueuedInventoryCommandSource source;
	source.enqueue({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 940,
	});
	source.enqueue({
	    .type = dev::InventoryCommandType::UnequipSlot,
	    .slot = dev::EquipmentSlot::Weapon,
	});

	Expect(source.size() == 2, "queued inventory command source should track queued command count");
	std::vector<dev::InventoryCommand> drained = source.drain();
	Expect(drained.size() == 2, "queued inventory command source should drain queued commands");
	Expect(source.empty(), "queued inventory command source should be empty after drain");
	Expect(source.drain().empty(), "queued inventory command source should not drain commands twice");
	Expect(drained.size() == 2 && drained[0].itemId == std::optional<dev::TargetId> { 940 }, "queued inventory command source should preserve equip payload");
	Expect(drained.size() == 2 && drained[1].slot == std::optional<dev::EquipmentSlot> { dev::EquipmentSlot::Weapon }, "queued inventory command source should preserve unequip payload");
}

void TestQueuedInventoryScriptSourceDrainsPathsOnce()
{
	dev::QueuedInventoryScriptSource source;
	const std::filesystem::path first = "equip.iicl";
	const std::filesystem::path second = "swap.iicl";
	source.enqueue(first);
	source.enqueue(second);

	Expect(source.size() == 2, "queued inventory script source should track queued path count");
	std::vector<std::filesystem::path> drained = source.drain();
	Expect(drained.size() == 2, "queued inventory script source should drain queued paths");
	Expect(source.empty(), "queued inventory script source should be empty after drain");
	Expect(source.drain().empty(), "queued inventory script source should not drain paths twice");
	Expect(drained.size() == 2 && drained[0] == first, "queued inventory script source should preserve first path");
	Expect(drained.size() == 2 && drained[1] == second, "queued inventory script source should preserve second path");
}

void TestRuntimeSourceDrainerSettingsBuilderMapsLoopSourcesAndPlayer()
{
	dev::QueuedRawInputSource rawInput;
	dev::QueuedSessionCommandSource sessionCommands;
	dev::QueuedMovementCommandSource movementCommands;
	dev::QueuedInventoryCommandSource inventoryCommands;
	dev::QueuedInventoryScriptSource inventoryScripts;

	dev::RuntimeSourceDrainerSettings settings = dev::RuntimeSourceDrainerSettingsBuilder {}.build(
	    dev::RuntimeSourceSettings {
	        .rawInputSources = { &rawInput },
	        .sessionCommandSources = { &sessionCommands },
	        .movementCommandSources = { &movementCommands },
	        .inventoryCommandSources = { &inventoryCommands },
	        .inventoryScriptSources = { &inventoryScripts },
	    },
	    dev::RuntimeInputSettings {
	        .playerId = 3,
	    });

	Expect(settings.sessionCommandSources.size() == 1 && settings.sessionCommandSources[0] == &sessionCommands, "runtime source drainer settings builder should copy session sources");
	Expect(settings.movementCommandSources.size() == 1 && settings.movementCommandSources[0] == &movementCommands, "runtime source drainer settings builder should copy movement sources");
	Expect(settings.inventoryCommandSources.size() == 1 && settings.inventoryCommandSources[0] == &inventoryCommands, "runtime source drainer settings builder should copy inventory command sources");
	Expect(settings.inventoryScriptSources.size() == 1 && settings.inventoryScriptSources[0] == &inventoryScripts, "runtime source drainer settings builder should copy inventory script sources");
	Expect(settings.inputPlayerId == 3, "runtime source drainer settings builder should map input player id to source-drainer player id");
}

void TestRuntimeSourceDrainerDrainsSessionBeforeMovement()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_source_drainer_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root / "saves" };
	dev::InventoryEventRecorder inventoryEvents;
	dev::QueuedSessionCommandSource routedSessionCommands;
	dev::QueuedMovementCommandSource routedMovementCommands;
	dev::QueuedSessionCommandSource sessionCommands;
	dev::QueuedMovementCommandSource movementCommands;

	sessionCommands.enqueue({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 0, 0 }, .playerHitPoints = 20 },
	});
	movementCommands.enqueue({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});

	dev::RuntimeSourceDrainer drainer {
		session,
		inventoryEvents,
		routedSessionCommands,
		routedMovementCommands,
		dev::RuntimeSourceDrainerSettings {
		    .sessionCommandSources = { &sessionCommands },
		    .movementCommandSources = { &movementCommands },
		},
	};
	dev::SessionCommandDispatcher dispatcher { session };

	std::vector<dev::SessionCommandResult> sessionResults = drainer.drainSessionCommands(dispatcher);
	int queuedMovement = drainer.drainMovementCommands();
	dev::SimulationFrameEvents frameEvents = session.update(1.0F / 60.0F);

	Expect(sessionResults.size() == 1 && sessionResults[0].type == dev::SessionCommandResultType::Applied, "runtime source drainer should dispatch session sources");
	Expect(session.hasActiveWorld(), "runtime source drainer session commands should create an active world");
	Expect(queuedMovement == 1, "runtime source drainer should queue movement after world exists");
	Expect(!frameEvents.movementEvents().empty(), "runtime source drainer test should produce movement frame events");
	Expect(session.world().players.size() == 1 && session.world().players[0].position.tile == dev::Point { 1, 0 }, "runtime source drainer movement queue should feed session update");
	Expect(sessionCommands.empty(), "runtime source drainer should drain session source");
	Expect(movementCommands.empty(), "runtime source drainer should drain movement source");

	std::filesystem::remove_all(root);
}

void TestGameLoopDrainsRuntimeMovementCommandSources()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_movement_source_test";
	std::filesystem::remove_all(root);

	dev::QueuedSessionCommandSource sessionSource;
	sessionSource.enqueue({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 0, 0 }, .playerHitPoints = 20 },
	});

	dev::QueuedMovementCommandSource movementSource;
	movementSource.enqueue({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = {
		        .sessionCommandSources = { &sessionSource },
		        .movementCommandSources = { &movementSource },
		    },
		    .frame = { .maxFrames = 1 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	bool sawCommandAccepted = false;
	for (const dev::MovementEvent &event : result.summary.lastFrameEvents.movementEvents()) {
		if (event.type == dev::MovementEventType::CommandAccepted && event.commandType == dev::MovementCommandType::WalkTo)
			sawCommandAccepted = true;
	}

	Expect(result.summary.sessionCommandResults.size() == 1, "game loop movement source test should dispatch session startup first");
	Expect(result.summary.movementCommandsQueued == 1, "game loop should queue runtime movement commands");
	Expect(result.summary.framesRun == 1, "game loop should run a frame after queueing movement");
	Expect(movementSource.empty(), "game loop should drain movement source commands once");
	Expect(loop.session().world().players.size() == 1 && loop.session().world().players[0].position.tile == dev::Point { 1, 0 }, "movement command source should move player through simulation");
	Expect(sawCommandAccepted, "movement command source should still pass through movement command dispatcher events");

	std::filesystem::remove_all(root);
}

void TestGameLoopDrainsRuntimeInventoryScriptSources()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_inventory_script_source_test";
	const std::filesystem::path scriptPath = root / "runtime_inventory.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 953,
	});
	dev::InventoryCommandLogFileStore store;
	Expect(store.save(scriptPath, log), "runtime inventory script source test should create script file");

	dev::QueuedInventoryScriptSource inventoryScripts;
	inventoryScripts.enqueue(scriptPath);

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = { .inventoryScriptSources = { &inventoryScripts } },
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	loop.session().world().players[0].inventory.items.push_back({
	    .id = 953,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});

	dev::GameLoopResult result = loop.runForResult();

	Expect(result.summary.runtimeInventoryScriptResults.size() == 1, "game loop should run runtime inventory script source");
	Expect(result.summary.runtimeInventoryScriptResults.size() == 1 && result.summary.runtimeInventoryScriptResults[0].status == dev::InventoryScriptRunStatus::Completed, "runtime inventory script source should complete valid script");
	Expect(result.summary.inventoryCommandResults.size() == 1 && result.summary.inventoryCommandResults[0].type == dev::InventoryCommandResultType::Applied, "runtime inventory script source should merge command results");
	Expect(loop.session().world().players[0].inventory.items.empty(), "runtime inventory script source should remove equipped item from bag");
	Expect(loop.session().world().players[0].inventory.equipment.weapon.has_value() && loop.session().world().players[0].inventory.equipment.weapon->id == 953, "runtime inventory script source should equip item");
	Expect(loop.inventoryEvents().events().size() == 1 && loop.inventoryEvents().events()[0].type == dev::InventoryEventType::Equipped, "runtime inventory script source should emit inventory event");
	Expect(inventoryScripts.empty(), "game loop should drain runtime inventory script source");

	std::filesystem::remove_all(root);
}

void TestGameLoopReportsRuntimeInventoryScriptLoadFailureWithoutStoppingFrames()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_inventory_script_source_load_failure_test";
	const std::filesystem::path missingPath = root / "missing.iicl";
	std::filesystem::remove_all(root);

	dev::QueuedInventoryScriptSource inventoryScripts;
	inventoryScripts.enqueue(missingPath);

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = { .inventoryScriptSources = { &inventoryScripts } },
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });

	dev::GameLoopResult result = loop.runForResult();

	Expect(result.summary.runtimeInventoryScriptResults.size() == 1, "game loop should report runtime inventory script source result");
	Expect(result.summary.runtimeInventoryScriptResults.size() == 1 && result.summary.runtimeInventoryScriptResults[0].status == dev::InventoryScriptRunStatus::LoadFailed, "runtime inventory script source should report load failure");
	Expect(result.summary.inventoryCommandResults.empty(), "failed runtime inventory script source should not dispatch commands");
	Expect(result.summary.framesRun == 1, "runtime inventory script load failure should not stop frame updates");
	Expect(inventoryScripts.empty(), "failed runtime inventory script source should still drain after attempted run");

	std::filesystem::remove_all(root);
}

void TestGameLoopDoesNotDrainInventoryScriptSourcesWithoutActiveWorld()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_no_world_inventory_script_source_test";
	const std::filesystem::path scriptPath = root / "inventory.iicl";
	std::filesystem::remove_all(root);

	dev::QueuedInventoryScriptSource inventoryScripts;
	inventoryScripts.enqueue(scriptPath);

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = { .inventoryScriptSources = { &inventoryScripts } },
		    .frame = { .maxFrames = 1 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	Expect(result.summary.runtimeInventoryScriptResults.empty(), "game loop should not run inventory script sources without an active world");
	Expect(inventoryScripts.size() == 1, "game loop should preserve inventory script paths until a world exists");

	std::filesystem::remove_all(root);
}

void TestGameLoopBuildsRuntimeFrameReports()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_frame_report_test";
	const std::filesystem::path scriptPath = root / "frame_inventory.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 954,
	});
	dev::InventoryCommandLogFileStore store;
	Expect(store.save(scriptPath, log), "frame report test should create inventory script");

	dev::QueuedInventoryScriptSource inventoryScripts;
	inventoryScripts.enqueue(scriptPath);
	dev::QueuedInventoryCommandSource inventoryCommands;
	inventoryCommands.enqueue({
	    .type = dev::InventoryCommandType::UnequipSlot,
	    .slot = dev::EquipmentSlot::Weapon,
	});
	dev::QueuedMovementCommandSource movementCommands;
	movementCommands.enqueue({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = {
		        .movementCommandSources = { &movementCommands },
		        .inventoryCommandSources = { &inventoryCommands },
		        .inventoryScriptSources = { &inventoryScripts },
		    },
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	loop.session().world().players[0].inventory.capacity = 2;
	loop.session().world().players[0].inventory.items.push_back({
	    .id = 954,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});

	dev::GameLoopResult result = loop.runForResult();

	Expect(result.frameReports.size() == 1, "game loop should record one runtime frame report per frame");
	if (result.frameReports.empty()) {
		std::filesystem::remove_all(root);
		return;
	}

	const dev::RuntimeFrameReport &report = result.frameReports[0];
	Expect(report.inventoryScriptResults.size() == 1, "runtime frame report should include inventory script results");
	Expect(report.inventoryScriptResults.size() == 1 && report.inventoryScriptResults[0].status == dev::InventoryScriptRunStatus::Completed, "runtime frame report should preserve inventory script status");
	Expect(report.inventoryCommandResults.size() == 2, "runtime frame report should include script and direct inventory command results");
	Expect(report.inventoryCommandResults.size() == 2 && report.inventoryCommandResults[0].type == dev::InventoryCommandResultType::Applied, "runtime frame report should include applied script command result");
	Expect(report.inventoryCommandResults.size() == 2 && report.inventoryCommandResults[1].type == dev::InventoryCommandResultType::Applied, "runtime frame report should include applied direct inventory command result");
	Expect(report.movementCommandsQueued == 1, "runtime frame report should include movement command count");
	Expect(!report.frameEvents.movementEvents().empty(), "runtime frame report should include simulation frame events");
	Expect(report.inventoryEvents.size() == 2, "runtime frame report should include inventory event deltas");
	Expect(report.inventoryEvents.size() == 2 && report.inventoryEvents[0].type == dev::InventoryEventType::Equipped, "runtime frame report should include equipped event");
	Expect(report.inventoryEvents.size() == 2 && report.inventoryEvents[1].type == dev::InventoryEventType::Unequipped, "runtime frame report should include unequipped event");
	Expect(result.summary.inventoryCommandResults.size() == report.inventoryCommandResults.size(), "game loop aggregate inventory command results should match frame report results");
	Expect(result.summary.movementCommandsQueued == report.movementCommandsQueued, "game loop aggregate movement count should match frame report count");
	Expect(result.summary.lastFrameEvents.movementEvents().size() == report.frameEvents.movementEvents().size(), "last frame events should mirror final runtime frame report");
	Expect(loop.session().world().players[0].inventory.items.size() == 1 && loop.session().world().players[0].inventory.items[0].id == 954, "frame report scenario should replay script then direct unequip");
	Expect(loop.session().world().players[0].position.tile == dev::Point { 1, 0 }, "frame report scenario should still run movement update");

	std::filesystem::remove_all(root);
}

void TestRuntimeFrameTraceFormatsReadableLines()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_frame_trace_test";
	const std::filesystem::path scriptPath = root / "trace_inventory.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 955,
	});
	dev::InventoryCommandLogFileStore store;
	Expect(store.save(scriptPath, log), "runtime frame trace test should create inventory script");

	dev::QueuedInventoryScriptSource inventoryScripts;
	inventoryScripts.enqueue(scriptPath);
	dev::QueuedInventoryCommandSource inventoryCommands;
	inventoryCommands.enqueue({
	    .type = dev::InventoryCommandType::UnequipSlot,
	    .slot = dev::EquipmentSlot::Weapon,
	});
	dev::QueuedMovementCommandSource movementCommands;
	movementCommands.enqueue({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = {
		        .movementCommandSources = { &movementCommands },
		        .inventoryCommandSources = { &inventoryCommands },
		        .inventoryScriptSources = { &inventoryScripts },
		    },
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	loop.session().world().players[0].inventory.capacity = 2;
	loop.session().world().players[0].inventory.items.push_back({
	    .id = 955,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});

	dev::GameLoopResult result = loop.runForResult();
	Expect(result.frameReports.size() == 1, "runtime frame trace test should create one frame report");
	if (result.frameReports.empty()) {
		std::filesystem::remove_all(root);
		return;
	}

	std::vector<std::string> lines = dev::RuntimeFrameTrace {}.format(result.frameReports[0]);

	Expect(!lines.empty(), "runtime frame trace should produce readable lines");
	Expect(ContainsLineFragment(lines, "frame rawInput=0"), "runtime frame trace should include summary line");
	Expect(ContainsLineFragment(lines, "inventoryScripts=1"), "runtime frame trace should include inventory script count");
	Expect(ContainsLineFragment(lines, "inventoryResults=2"), "runtime frame trace should include inventory result count");
	Expect(ContainsLineFragment(lines, "movementQueued=1"), "runtime frame trace should include movement queue count");
	Expect(ContainsLineFragment(lines, "inventoryScript[0] status=Completed results=1"), "runtime frame trace should include inventory script detail");
	Expect(ContainsLineFragment(lines, "inventoryResult[0] type=Applied command=EquipItem equipment=Equipped item=955 slot=Weapon"), "runtime frame trace should include equip result detail");
	Expect(ContainsLineFragment(lines, "inventoryResult[1] type=Applied command=UnequipSlot equipment=Unequipped item=955 slot=Weapon"), "runtime frame trace should include unequip result detail");
	Expect(ContainsLineFragment(lines, "inventoryEvent[0] type=Equipped command=EquipItem result=Applied equipment=Equipped item=955 slot=Weapon"), "runtime frame trace should include inventory event detail");
	Expect(ContainsLineFragment(lines, "movementEvent[0] type=CommandAccepted"), "runtime frame trace should include movement event detail");

	std::filesystem::remove_all(root);
}

void TestRuntimeFrameTraceFileStoreSavesAndLoadsLines()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_frame_trace_file_store_test";
	const std::filesystem::path path = root / "frame.trace";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);
	std::filesystem::remove(path.string() + ".tmp");

	std::vector<std::string> lines {
		"frame rawInput=0 sessionResults=0 inventoryScripts=1 inventoryResults=2 movementQueued=1",
		"inventoryResult[0] type=Applied command=EquipItem equipment=Equipped item=955 slot=Weapon",
		"movementEvent[0] type=CommandAccepted player=0 tile=(0,0) command=WalkTo",
	};

	dev::RuntimeFrameTraceFileStore store;
	Expect(store.save(path, lines), "runtime frame trace file store should save lines");
	std::optional<std::vector<std::string>> loaded = store.load(path);

	Expect(loaded.has_value(), "runtime frame trace file store should load saved lines");
	Expect(loaded.has_value() && *loaded == lines, "runtime frame trace file store should preserve exact lines");
	Expect(!std::filesystem::exists(path.string() + ".tmp"), "runtime frame trace file store should remove temp file after save");

	std::filesystem::remove_all(root);
}

void TestRuntimeFrameTraceFileStoreRejectsMissingFile()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_frame_trace_missing_test";
	const std::filesystem::path path = root / "missing.trace";
	std::filesystem::remove_all(root);

	dev::RuntimeFrameTraceFileStore store;
	Expect(!store.load(path).has_value(), "runtime frame trace file store should reject missing file");

	std::filesystem::remove_all(root);
}

void TestRuntimeTraceServiceFormatsAndSavesRunTrace()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_trace_service_test";
	const std::filesystem::path scriptPath = root / "trace_inventory.iicl";
	const std::filesystem::path tracePath = root / "run.trace";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::InventoryCommandLog log;
	log.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 956,
	});
	dev::InventoryCommandLogFileStore inventoryStore;
	Expect(inventoryStore.save(scriptPath, log), "runtime trace service test should create inventory script");

	dev::QueuedInventoryScriptSource inventoryScripts;
	inventoryScripts.enqueue(scriptPath);

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = { .inventoryScriptSources = { &inventoryScripts } },
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	loop.session().world().players[0].inventory.items.push_back({
	    .id = 956,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});
	dev::GameLoopResult result = loop.runForResult();

	dev::RuntimeTraceService service;
	std::vector<std::string> lines = service.formatRun(result);
	Expect(service.saveRunTrace(tracePath, result), "runtime trace service should save full run trace");
	std::optional<std::vector<std::string>> loaded = dev::RuntimeFrameTraceFileStore {}.load(tracePath);

	Expect(!lines.empty(), "runtime trace service should format run lines");
	Expect(!lines.empty() && lines[0] == "run frames=1 frameReports=1 rawInput=0 sessionResults=0 inventoryScripts=1 inventoryResults=1 movementQueued=0", "runtime trace service should include run summary");
	Expect(ContainsLineFragment(lines, "frame[0]"), "runtime trace service should include frame header");
	Expect(ContainsLineFragment(lines, "inventoryResult[0] type=Applied command=EquipItem equipment=Equipped item=956 slot=Weapon"), "runtime trace service should include frame trace detail");
	Expect(loaded.has_value() && *loaded == lines, "runtime trace service should persist exact formatted lines");

	std::filesystem::remove_all(root);
}

void TestRuntimeTraceServiceFormatsEmptyRun()
{
	dev::GameLoopResult result;
	std::vector<std::string> lines = dev::RuntimeTraceService {}.formatRun(result);

	Expect(lines.size() == 1, "runtime trace service should format empty run as summary only");
	Expect(lines.size() == 1 && lines[0] == "run frames=0 frameReports=0 rawInput=0 sessionResults=0 inventoryScripts=0 inventoryResults=0 movementQueued=0", "runtime trace service should preserve empty run counts");
}

void TestRuntimeOutputSettingsDefaultDisablesArtifacts()
{
	dev::RuntimeOutputSettings output;

	Expect(!output.runTracePath.has_value(), "runtime output settings should default to no run trace path");
	Expect(!output.debugBundlePath.has_value(), "runtime output settings should default to no debug bundle path");
}

void TestRuntimeOutputResultDefaultsToNoAttempts()
{
	dev::RuntimeOutputResult output;

	Expect(!output.runTraceSaveAttempted, "runtime output result should default to no trace attempt");
	Expect(!output.runTraceSaved, "runtime output result should default to unsaved trace");
	Expect(!output.debugBundleSaveAttempted, "runtime output result should default to no bundle attempt");
	Expect(!output.debugBundleSaved, "runtime output result should default to unsaved bundle");
	Expect(!dev::RuntimeOutputFinalizer::failed(output), "runtime output result should not fail when nothing was requested");
}

void TestRuntimeSetupSettingsDefaultsToNoScripts()
{
	dev::RuntimeSetupSettings setup;

	Expect(!setup.startupScript.has_value(), "runtime setup settings should default to no startup script");
	Expect(!setup.inventoryScript.has_value(), "runtime setup settings should default to no configured inventory script");
}

void TestRuntimeSetupResultDefaultsToNoSetupScripts()
{
	dev::RuntimeSetupResult setup;

	Expect(!setup.startupScriptRan, "runtime setup result should default to no startup script");
	Expect(setup.startupScriptResult.commandResults.empty(), "runtime setup result should default to no startup command results");
	Expect(!setup.inventoryScriptRan, "runtime setup result should default to no inventory script");
	Expect(setup.inventoryScriptResult.commandResults.empty(), "runtime setup result should default to no inventory command results");
}

void TestRuntimeSetupRunnerAllowsFramesWhenNoScriptsConfigured()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_setup_runner_empty_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root / "saves" };
	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	dev::QueuedSessionCommandSource routedSessionCommands;
	dev::QueuedMovementCommandSource routedMovementCommands;
	dev::SessionCommandDispatcher dispatcher { session, &sessionEvents };
	dev::RuntimeSourceDrainer drainer {
		session,
		inventoryEvents,
		routedSessionCommands,
		routedMovementCommands,
		{},
	};
	dev::RuntimeSetupRunResult result = dev::RuntimeSetupRunner { dispatcher, drainer }.run({});

	Expect(result.framesAllowed, "runtime setup runner should allow frames when no setup scripts are configured");
	Expect(!result.setup.startupScriptRan, "runtime setup runner should not invent startup script attempts");
	Expect(!result.setup.inventoryScriptRan, "runtime setup runner should not invent inventory script attempts");
	Expect(result.inventoryCommandResults.empty(), "runtime setup runner should report no setup inventory command results without scripts");

	std::filesystem::remove_all(root);
}

void TestRuntimeSetupRunnerStopsFramesAfterStartupLoadFailure()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_setup_runner_startup_failure_test";
	const std::filesystem::path missingStartup = root / "missing.iscl";
	const std::filesystem::path missingInventory = root / "missing.iicl";
	std::filesystem::remove_all(root);

	dev::GameSession session { root / "saves" };
	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	dev::QueuedSessionCommandSource routedSessionCommands;
	dev::QueuedMovementCommandSource routedMovementCommands;
	dev::SessionCommandDispatcher dispatcher { session, &sessionEvents };
	dev::RuntimeSourceDrainer drainer {
		session,
		inventoryEvents,
		routedSessionCommands,
		routedMovementCommands,
		{},
	};
	dev::RuntimeSetupRunResult result = dev::RuntimeSetupRunner { dispatcher, drainer }.run({
	    .startupScript = missingStartup,
	    .inventoryScript = missingInventory,
	});

	Expect(!result.framesAllowed, "runtime setup runner should stop frames after startup load failure");
	Expect(result.setup.startupScriptRan, "runtime setup runner should attempt configured startup script");
	Expect(result.setup.startupScriptResult.status == dev::SessionScriptRunStatus::LoadFailed, "runtime setup runner should report startup load failure");
	Expect(!result.setup.inventoryScriptRan, "runtime setup runner should not run inventory setup after startup load failure");
	Expect(result.inventoryCommandResults.empty(), "failed startup setup should not produce inventory command results");

	std::filesystem::remove_all(root);
}

void TestRuntimeSetupRunnerPreservesInventoryCommandRejections()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_setup_runner_inventory_rejection_test";
	const std::filesystem::path inventoryPath = root / "inventory.iicl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::InventoryCommandLog inventoryLog;
	inventoryLog.record({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 72,
	});
	dev::InventoryCommandLogFileStore inventoryStore;
	Expect(inventoryStore.save(inventoryPath, inventoryLog), "runtime setup runner inventory rejection test should create inventory script");

	dev::GameSession session { root / "saves" };
	session.startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	dev::QueuedSessionCommandSource routedSessionCommands;
	dev::QueuedMovementCommandSource routedMovementCommands;
	dev::SessionCommandDispatcher dispatcher { session, &sessionEvents };
	dev::RuntimeSourceDrainer drainer {
		session,
		inventoryEvents,
		routedSessionCommands,
		routedMovementCommands,
		{},
	};
	dev::RuntimeSetupRunResult result = dev::RuntimeSetupRunner { dispatcher, drainer }.run({
	    .inventoryScript = inventoryPath,
	});

	Expect(result.framesAllowed, "runtime setup runner should allow frames after loadable inventory script command rejection");
	Expect(result.setup.inventoryScriptRan, "runtime setup runner should attempt configured inventory script");
	Expect(result.setup.inventoryScriptResult.status == dev::InventoryScriptRunStatus::Completed, "runtime setup runner should complete loadable inventory scripts");
	Expect(result.setup.inventoryScriptResult.commandResults.size() == 1, "runtime setup runner should preserve inventory script command results");
	Expect(result.setup.inventoryScriptResult.commandResults.size() == 1 && result.setup.inventoryScriptResult.commandResults[0].type == dev::InventoryCommandResultType::Rejected, "runtime setup runner should preserve rejected inventory command result");
	Expect(result.inventoryCommandResults.size() == 1 && result.inventoryCommandResults[0].type == dev::InventoryCommandResultType::Rejected, "runtime setup runner should expose configured inventory command results for run summaries");
	Expect(inventoryEvents.events().size() == 1 && inventoryEvents.events()[0].type == dev::InventoryEventType::Rejected, "runtime setup runner should preserve inventory setup events");

	std::filesystem::remove_all(root);
}

void TestRuntimeSourceSettingsDefaultsToNoSources()
{
	dev::RuntimeSourceSettings sources;

	Expect(sources.rawInputSources.empty(), "runtime source settings should default to no raw input sources");
	Expect(sources.sessionCommandSources.empty(), "runtime source settings should default to no session command sources");
	Expect(sources.movementCommandSources.empty(), "runtime source settings should default to no movement command sources");
	Expect(sources.inventoryCommandSources.empty(), "runtime source settings should default to no inventory command sources");
	Expect(sources.inventoryScriptSources.empty(), "runtime source settings should default to no inventory script sources");
}

void TestRuntimeInputSettingsDefaultsToPrimaryGameplayInput()
{
	dev::RuntimeInputSettings input;

	Expect(input.bindings.pauseKey == 27, "runtime input settings should default pause binding to escape");
	Expect(input.bindings.inventoryKey == 'I', "runtime input settings should default inventory binding to I");
	Expect(input.bindings.stopKey == 'S', "runtime input settings should default stop binding to S");
	Expect(input.focusState.owner == dev::InputOwner::Gameplay, "runtime input settings should default focus to gameplay");
	Expect(!input.focusState.textEntryActive, "runtime input settings should default to no text entry");
	Expect(!input.actionContext.paused, "runtime input settings should default actions to unpaused");
	Expect(!input.actionContext.animationLocked, "runtime input settings should default actions to unlocked animation");
	Expect(input.playerId == 0, "runtime input settings should default to player zero");
	Expect(input.targetResolver == nullptr, "runtime input settings should default to world target resolver fallback");
}

void TestRuntimeInputContextBuilderHandlesMissingWorld()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_input_context_no_world_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root / "saves" };
	dev::RuntimeInputContext context = dev::RuntimeInputContextBuilder { session }.build({
	    .focusState = dev::FocusState { .owner = dev::InputOwner::Inventory },
	    .actionContext = dev::PlayerActionContext { .paused = true },
	    .playerId = 2,
	});

	Expect(context.world == nullptr, "runtime input context builder should not expose a world before one exists");
	Expect(context.playerId == 2, "runtime input context builder should preserve configured player id");
	Expect(context.focusState.owner == dev::InputOwner::Inventory, "runtime input context builder should preserve focus state");
	Expect(context.actionContext.paused, "runtime input context builder should preserve action context");
	Expect(context.sessionMode == dev::GameSessionMode::Empty, "runtime input context builder should expose current session mode");
	Expect(context.targetResolver == nullptr, "runtime input context builder should not invent a target resolver without a world");

	std::filesystem::remove_all(root);
}

void TestRuntimeInputContextBuilderUsesWorldTargetsUnlessOverridden()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_input_context_world_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root / "saves" };
	session.startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	dev::RuntimeInputContext worldContext = dev::RuntimeInputContextBuilder { session }.build({});

	FixedTargetResolver explicitTargets {
		dev::Target {
		    .type = dev::TargetType::Enemy,
		    .id = 91,
		    .tile = { 0, 0 },
		}
	};
	dev::RuntimeInputContext overrideContext = dev::RuntimeInputContextBuilder { session }.build({
	    .targetResolver = &explicitTargets,
	});

	Expect(worldContext.world == &session.world(), "runtime input context builder should expose active world");
	Expect(worldContext.sessionMode == dev::GameSessionMode::Gameplay, "runtime input context builder should expose gameplay session mode");
	Expect(worldContext.targetResolver == &session.world().targets, "runtime input context builder should use world targets by default");
	Expect(overrideContext.targetResolver == &explicitTargets, "runtime input context builder should preserve explicit target resolver override");

	std::filesystem::remove_all(root);
}

void TestRuntimeFrameSettingsDefaultsToNoFramesAtSixtyHz()
{
	dev::RuntimeFrameSettings frame;

	Expect(frame.maxFrames == 0, "runtime frame settings should default to zero bounded frames");
	Expect(frame.fixedDeltaSeconds == 1.0F / 60.0F, "runtime frame settings should default to sixty hertz timestep");
}

void TestRuntimeRunSummaryDefaultsToEmptyRun()
{
	dev::RuntimeRunSummary summary;

	Expect(summary.runtimeInventoryScriptResults.empty(), "runtime run summary should default to no runtime inventory script results");
	Expect(summary.rawInputEventsRouted == 0, "runtime run summary should default to no routed raw input");
	Expect(summary.sessionCommandResults.empty(), "runtime run summary should default to no session command results");
	Expect(summary.inventoryCommandResults.empty(), "runtime run summary should default to no inventory command results");
	Expect(summary.movementCommandsQueued == 0, "runtime run summary should default to no queued movement commands");
	Expect(summary.framesRun == 0, "runtime run summary should default to zero frames");
	Expect(summary.lastFrameEvents.movementEvents().empty(), "runtime run summary should default to no final movement events");
}

void TestRuntimeRunRecorderAggregatesSetupInventoryResultsWithoutFrame()
{
	dev::GameLoopResult result;
	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	dev::RuntimeRunRecorder recorder { result, sessionEvents, inventoryEvents };

	recorder.recordSetupInventoryCommandResults({
	    {
	        .type = dev::InventoryCommandResultType::Applied,
	        .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 73 },
	        .equipmentResult = { .type = dev::EquipmentResultType::Equipped, .itemId = 73 },
	    },
	});

	Expect(result.summary.inventoryCommandResults.size() == 1, "runtime run recorder should aggregate setup inventory command results");
	Expect(result.summary.inventoryCommandResults.size() == 1 && result.summary.inventoryCommandResults[0].command.itemId == std::optional<dev::TargetId> { 73 }, "runtime run recorder should preserve setup inventory result payload");
	Expect(result.summary.framesRun == 0, "runtime run recorder setup aggregation should not count a frame");
	Expect(result.frameReports.empty(), "runtime run recorder setup aggregation should not create a frame report");
}

void TestRuntimeRunRecorderAggregatesFrameReportsAndSummary()
{
	dev::GameLoopResult result;
	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	dev::RuntimeRunRecorder recorder { result, sessionEvents, inventoryEvents };

	sessionEvents.emit({
	    .type = dev::SessionEventType::GameStarted,
	    .commandType = dev::SessionCommandType::StartNewGame,
	});
	inventoryEvents.emit({
	    .type = dev::InventoryEventType::Rejected,
	    .commandType = dev::InventoryCommandType::EquipItem,
	    .commandResult = dev::InventoryCommandResultType::Rejected,
	    .equipmentResult = dev::EquipmentResultType::MissingItem,
	    .itemId = 70,
	});

	recorder.beginFrame();
	sessionEvents.emit({
	    .type = dev::SessionEventType::ModeChanged,
	    .commandType = dev::SessionCommandType::SetMode,
	    .mode = dev::GameSessionMode::Inventory,
	});
	inventoryEvents.emit({
	    .type = dev::InventoryEventType::Equipped,
	    .commandType = dev::InventoryCommandType::EquipItem,
	    .commandResult = dev::InventoryCommandResultType::Applied,
	    .equipmentResult = dev::EquipmentResultType::Equipped,
	    .itemId = 71,
	});

	recorder.recordRawInputEventsRouted(2);
	recorder.recordSessionCommandResults({
	    {
	        .type = dev::SessionCommandResultType::Applied,
	        .command = { .type = dev::SessionCommandType::SetMode, .mode = dev::GameSessionMode::Inventory },
	    },
	});
	recorder.recordInventoryScriptResults({
	    {
	        .status = dev::InventoryScriptRunStatus::Completed,
	        .commandResults = {
	            {
	                .type = dev::InventoryCommandResultType::Applied,
	                .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 71 },
	                .equipmentResult = { .type = dev::EquipmentResultType::Equipped, .itemId = 71 },
	            },
	        },
	    },
	});
	recorder.recordInventoryCommandResults({
	    {
	        .type = dev::InventoryCommandResultType::Applied,
	        .command = { .type = dev::InventoryCommandType::UnequipSlot, .slot = dev::EquipmentSlot::Weapon },
	        .equipmentResult = { .type = dev::EquipmentResultType::Unequipped, .itemId = 71 },
	    },
	});
	recorder.recordMovementCommandsQueued(1);
	dev::SimulationFrameEvents frameEvents;
	frameEvents.emit({
	    .type = dev::MovementEventType::CommandAccepted,
	    .playerId = 0,
	    .tile = { 1, 0 },
	    .commandType = dev::MovementCommandType::WalkTo,
	});
	recorder.recordFrameEvents(frameEvents);
	recorder.finishFrame();

	Expect(result.summary.framesRun == 1, "runtime run recorder should count finished frames");
	Expect(result.summary.rawInputEventsRouted == 2, "runtime run recorder should aggregate routed raw input");
	Expect(result.summary.sessionCommandResults.size() == 1, "runtime run recorder should aggregate session results");
	Expect(result.summary.runtimeInventoryScriptResults.size() == 1, "runtime run recorder should aggregate inventory script results");
	Expect(result.summary.inventoryCommandResults.size() == 2, "runtime run recorder should aggregate script and direct inventory results");
	Expect(result.summary.movementCommandsQueued == 1, "runtime run recorder should aggregate movement queue counts");
	Expect(result.summary.lastFrameEvents.movementEvents().size() == 1, "runtime run recorder should store final frame events");
	Expect(result.frameReports.size() == 1, "runtime run recorder should create one frame report");
	if (result.frameReports.empty())
		return;

	const dev::RuntimeFrameReport &report = result.frameReports[0];
	Expect(report.rawInputEventsRouted == 2, "runtime run recorder frame report should keep raw input count");
	Expect(report.sessionCommandResults.size() == 1, "runtime run recorder frame report should keep session results");
	Expect(report.inventoryScriptResults.size() == 1, "runtime run recorder frame report should keep inventory scripts");
	Expect(report.inventoryCommandResults.size() == 2, "runtime run recorder frame report should keep script and direct inventory results");
	Expect(report.movementCommandsQueued == 1, "runtime run recorder frame report should keep movement queue count");
	Expect(report.sessionEvents.size() == 1 && report.sessionEvents[0].type == dev::SessionEventType::ModeChanged, "runtime run recorder should capture session event deltas");
	Expect(report.inventoryEvents.size() == 1 && report.inventoryEvents[0].itemId == 71, "runtime run recorder should capture inventory event deltas");
	Expect(report.frameEvents.movementEvents().size() == 1, "runtime run recorder frame report should keep simulation frame events");
}

void TestRuntimeRunFinalizerCapturesFinalModeAndLeavesDisabledOutputsUntouched()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_run_finalizer_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root / "saves" };
	session.startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	session.setMode(dev::GameSessionMode::Inventory);
	dev::GameLoopResult result;

	dev::RuntimeRunFinalizer {}.finalize(session, dev::RuntimeOutputSettings {}, result);

	Expect(result.finalMode == dev::GameSessionMode::Inventory, "runtime run finalizer should capture final session mode");
	Expect(!result.output.runTraceSaveAttempted, "runtime run finalizer should leave disabled trace output untouched");
	Expect(!result.output.debugBundleSaveAttempted, "runtime run finalizer should leave disabled bundle output untouched");

	std::filesystem::remove_all(root);
}

void TestRuntimeExitCodePolicyReportsSuccessForCleanRun()
{
	dev::GameLoopResult result;

	Expect(!dev::RuntimeExitCodePolicy {}.failed(result), "runtime exit policy should not fail clean default run results");
	Expect(dev::RuntimeExitCodePolicy {}.exitCodeFor(result) == 0, "runtime exit policy should return success for clean run results");
}

void TestRuntimeExitCodePolicyFailsSetupErrors()
{
	dev::GameLoopResult startupFailure;
	startupFailure.setup.startupScriptRan = true;
	startupFailure.setup.startupScriptResult.status = dev::SessionScriptRunStatus::LoadFailed;

	dev::GameLoopResult inventoryFailure;
	inventoryFailure.setup.inventoryScriptRan = true;
	inventoryFailure.setup.inventoryScriptResult.status = dev::InventoryScriptRunStatus::LoadFailed;

	Expect(dev::RuntimeExitCodePolicy {}.failed(startupFailure), "runtime exit policy should fail startup script load failures");
	Expect(dev::RuntimeExitCodePolicy {}.exitCodeFor(startupFailure) == 1, "runtime exit policy should return failure for startup load failures");
	Expect(dev::RuntimeExitCodePolicy {}.failed(inventoryFailure), "runtime exit policy should fail configured inventory script failures");
	Expect(dev::RuntimeExitCodePolicy {}.exitCodeFor(inventoryFailure) == 1, "runtime exit policy should return failure for configured inventory script failures");
}

void TestRuntimeExitCodePolicyAllowsCommandRejections()
{
	dev::GameLoopResult result;
	result.setup.inventoryScriptRan = true;
	result.setup.inventoryScriptResult.status = dev::InventoryScriptRunStatus::Completed;
	result.setup.inventoryScriptResult.commandResults.push_back({
	    .type = dev::InventoryCommandResultType::Rejected,
	    .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 99 },
	});

	Expect(!dev::RuntimeExitCodePolicy {}.failed(result), "runtime exit policy should allow completed setup scripts with rejected commands");
	Expect(dev::RuntimeExitCodePolicy {}.exitCodeFor(result) == 0, "runtime exit policy should return success for command-level rejections");
}

void TestRuntimeExitCodePolicyFailsOutputErrors()
{
	dev::GameLoopResult result;
	result.output.runTraceSaveAttempted = true;
	result.output.runTraceSaved = false;

	Expect(dev::RuntimeExitCodePolicy {}.failed(result), "runtime exit policy should fail requested output write failures");
	Expect(dev::RuntimeExitCodePolicy {}.exitCodeFor(result) == 1, "runtime exit policy should return failure for output write failures");
}

void TestRuntimeOutputFinalizerLeavesDisabledOutputsUntouched()
{
	dev::GameLoopResult result;
	dev::RuntimeOutputFinalizer {}.finalize(dev::RuntimeOutputSettings {}, result);

	Expect(!result.output.runTraceSaveAttempted, "runtime output finalizer should not attempt trace without trace path");
	Expect(!result.output.debugBundleSaveAttempted, "runtime output finalizer should not attempt bundle without bundle path");
	Expect(!dev::RuntimeOutputFinalizer::failed(result.output), "runtime output finalizer should not fail when nothing was requested");
}

void TestRuntimeOutputFinalizerSavesTraceAndBundle()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_output_finalizer_test";
	const std::filesystem::path tracePath = root / "run.trace";
	const std::filesystem::path bundlePath = root / "bundle";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::GameLoopResult result;
	result.summary.framesRun = 1;
	result.finalMode = dev::GameSessionMode::Gameplay;

	dev::RuntimeOutputFinalizer {}.finalize(
	    dev::RuntimeOutputSettings {
	        .runTracePath = tracePath,
	        .debugBundlePath = bundlePath,
	    },
	    result);

	std::optional<std::vector<std::string>> trace = dev::RuntimeFrameTraceFileStore {}.load(tracePath);
	std::optional<std::vector<std::string>> bundleManifest = dev::RuntimeFrameTraceFileStore {}.load(bundlePath / "manifest.txt");
	std::optional<std::vector<std::string>> bundleTrace = dev::RuntimeFrameTraceFileStore {}.load(bundlePath / "run.trace");

	Expect(result.output.runTraceSaveAttempted, "runtime output finalizer should attempt configured trace save");
	Expect(result.output.runTraceSaved, "runtime output finalizer should report saved trace");
	Expect(result.output.debugBundleSaveAttempted, "runtime output finalizer should attempt configured bundle save");
	Expect(result.output.debugBundleSaved, "runtime output finalizer should report saved bundle");
	Expect(!dev::RuntimeOutputFinalizer::failed(result.output), "runtime output finalizer should report no failure after saving requested outputs");
	Expect(trace.has_value() && !trace->empty() && (*trace)[0] == "run frames=1 frameReports=0 rawInput=0 sessionResults=0 inventoryScripts=0 inventoryResults=0 movementQueued=0", "runtime output finalizer should save standalone trace");
	Expect(bundleManifest.has_value() && ContainsLineFragment(*bundleManifest, "trace=run.trace saved=true"), "runtime output finalizer should save bundle manifest");
	Expect(bundleTrace.has_value() && !bundleTrace->empty() && (*bundleTrace)[0] == "run frames=1 frameReports=0 rawInput=0 sessionResults=0 inventoryScripts=0 inventoryResults=0 movementQueued=0", "runtime output finalizer should save bundle trace");

	std::filesystem::remove_all(root);
}

void TestRuntimeOutputFinalizerReportsRequestedOutputFailure()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_output_finalizer_failure_test";
	const std::filesystem::path tracePath = root / "missing-parent" / "run.trace";
	std::filesystem::remove_all(root);

	dev::GameLoopResult result;
	dev::RuntimeOutputFinalizer {}.finalize(
	    dev::RuntimeOutputSettings {
	        .runTracePath = tracePath,
	    },
	    result);

	Expect(result.output.runTraceSaveAttempted, "runtime output finalizer should attempt requested trace even when path is invalid");
	Expect(!result.output.runTraceSaved, "runtime output finalizer should report failed trace save");
	Expect(dev::RuntimeOutputFinalizer::failed(result.output), "runtime output finalizer should report requested output failure");

	std::filesystem::remove_all(root);
}

void TestGameLoopSavesConfiguredRunTrace()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_run_trace_test";
	const std::filesystem::path tracePath = root / "run.trace";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .output = { .runTracePath = tracePath },
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });

	dev::GameLoopResult result = loop.runForResult();
	std::optional<std::vector<std::string>> loaded = dev::RuntimeFrameTraceFileStore {}.load(tracePath);

	Expect(result.output.runTraceSaveAttempted, "game loop should attempt configured run trace save");
	Expect(result.output.runTraceSaved, "game loop should report successful run trace save");
	Expect(loaded.has_value(), "game loop should persist configured run trace");
	Expect(loaded.has_value() && !loaded->empty() && (*loaded)[0] == "run frames=1 frameReports=1 rawInput=0 sessionResults=0 inventoryScripts=0 inventoryResults=0 movementQueued=0", "game loop run trace should include run summary");
	Expect(loaded.has_value() && ContainsLineFragment(*loaded, "frame[0]"), "game loop run trace should include frame trace header");

	std::filesystem::remove_all(root);
}

void TestGameLoopSavesRunTraceOnStartupFailure()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_failed_startup_trace_test";
	const std::filesystem::path tracePath = root / "failed-startup.trace";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .setup = { .startupScript = root / "missing.iscl" },
		    .output = { .runTracePath = tracePath },
		    .frame = { .maxFrames = 1 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();
	std::optional<std::vector<std::string>> loaded = dev::RuntimeFrameTraceFileStore {}.load(tracePath);

	Expect(result.setup.startupScriptRan, "failed startup trace test should attempt startup script");
	Expect(result.setup.startupScriptResult.status == dev::SessionScriptRunStatus::LoadFailed, "failed startup trace test should report load failure");
	Expect(result.output.runTraceSaveAttempted, "game loop should attempt run trace save after startup failure");
	Expect(result.output.runTraceSaved, "game loop should save run trace after startup failure");
	Expect(loaded.has_value() && !loaded->empty() && (*loaded)[0] == "run frames=0 frameReports=0 rawInput=0 sessionResults=0 inventoryScripts=0 inventoryResults=0 movementQueued=0", "failed startup trace should preserve zero-frame summary");

	std::filesystem::remove_all(root);
}

void TestGameLoopReportsRunTraceSaveFailure()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_run_trace_failure_test";
	const std::filesystem::path tracePath = root / "missing-parent" / "run.trace";
	std::filesystem::remove_all(root);

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .output = { .runTracePath = tracePath },
		    .frame = { .maxFrames = 0 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	Expect(result.output.runTraceSaveAttempted, "game loop should attempt configured run trace even when path is invalid");
	Expect(!result.output.runTraceSaved, "game loop should report failed run trace save");

	dev::GameLoop exitLoop {
		dev::GameLoopSettings {
		    .saveRoot = root / "other-saves",
		    .output = { .runTracePath = tracePath },
		    .frame = { .maxFrames = 0 },
		}
	};
	Expect(exitLoop.run() == 1, "game loop run should fail when requested trace cannot be saved");

	std::filesystem::remove_all(root);
}

void TestRuntimeDebugArtifactBundleSavesManifestAndTrace()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_debug_bundle_test";
	const std::filesystem::path bundleRoot = root / "bundle";
	std::filesystem::remove_all(root);

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	dev::GameLoopResult run = loop.runForResult();

	dev::RuntimeDebugArtifactBundle bundle;
	dev::RuntimeDebugArtifactBundleResult result = bundle.save(bundleRoot, run);
	std::optional<std::vector<std::string>> manifest = dev::RuntimeFrameTraceFileStore {}.load(bundleRoot / "manifest.txt");
	std::optional<std::vector<std::string>> trace = dev::RuntimeFrameTraceFileStore {}.load(bundleRoot / "run.trace");

	Expect(result.rootPrepared, "runtime debug bundle should create bundle root");
	Expect(result.traceSaved, "runtime debug bundle should save run trace");
	Expect(result.manifestSaved, "runtime debug bundle should save manifest");
	Expect(result.saved(), "runtime debug bundle should report complete save");
	Expect(result.manifestPath == bundleRoot / "manifest.txt", "runtime debug bundle should use stable manifest path");
	Expect(result.tracePath == bundleRoot / "run.trace", "runtime debug bundle should use stable trace path");
	Expect(manifest.has_value(), "runtime debug bundle manifest should be loadable text");
	Expect(trace.has_value(), "runtime debug bundle trace should be loadable text");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "bundle version=1"), "runtime debug bundle manifest should include version");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "trace=run.trace saved=true"), "runtime debug bundle manifest should index trace artifact");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "run frames=1 frameReports=1"), "runtime debug bundle manifest should summarize run frame counts");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "finalMode=Gameplay"), "runtime debug bundle manifest should include final mode");
	Expect(trace.has_value() && !trace->empty() && (*trace)[0] == "run frames=1 frameReports=1 rawInput=0 sessionResults=0 inventoryScripts=0 inventoryResults=0 movementQueued=0", "runtime debug bundle trace should preserve run trace summary");

	std::filesystem::remove_all(root);
}

void TestRuntimeDebugArtifactBundleFormatsManifestForFailedRun()
{
	dev::GameLoopResult run;
	run.setup.startupScriptRan = true;
	run.finalMode = dev::GameSessionMode::Empty;

	dev::RuntimeDebugArtifactBundleResult bundle {
		.rootPath = "debug/run-001",
		.manifestPath = "debug/run-001/manifest.txt",
		.tracePath = "debug/run-001/run.trace",
		.rootPrepared = true,
		.traceSaved = false,
		.manifestSaved = false,
	};

	std::vector<std::string> lines = dev::RuntimeDebugArtifactBundle {}.formatManifest(run, bundle);

	Expect(ContainsLineFragment(lines, "trace=run.trace saved=false"), "runtime debug bundle manifest should report trace save state");
	Expect(ContainsLineFragment(lines, "run frames=0 frameReports=0"), "runtime debug bundle manifest should summarize empty failed runs");
	Expect(ContainsLineFragment(lines, "finalMode=Empty"), "runtime debug bundle manifest should name empty final mode");
	Expect(ContainsLineFragment(lines, "setup startupScriptRan=true inventoryScriptRan=false"), "runtime debug bundle manifest should report setup attempts");
}

void TestRuntimeDebugArtifactBundleRejectsRootFile()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_debug_bundle_root_file_test";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root.parent_path());
	{
		std::ofstream output { root, std::ios::trunc };
		output << "not a directory\n";
	}

	dev::RuntimeDebugArtifactBundleResult result = dev::RuntimeDebugArtifactBundle {}.save(root, dev::GameLoopResult {});

	Expect(!result.rootPrepared, "runtime debug bundle should reject a root path that is already a file");
	Expect(!result.traceSaved, "runtime debug bundle should not save trace when root cannot be prepared");
	Expect(!result.manifestSaved, "runtime debug bundle should not save manifest when root cannot be prepared");
	Expect(!result.saved(), "runtime debug bundle should report incomplete save on root failure");

	std::filesystem::remove(root);
}

void TestGameLoopSavesConfiguredDebugBundle()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_debug_bundle_test";
	const std::filesystem::path bundleRoot = root / "debug-bundle";
	std::filesystem::remove_all(root);

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .output = { .debugBundlePath = bundleRoot },
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });

	dev::GameLoopResult result = loop.runForResult();
	std::optional<std::vector<std::string>> manifest = dev::RuntimeFrameTraceFileStore {}.load(bundleRoot / "manifest.txt");
	std::optional<std::vector<std::string>> trace = dev::RuntimeFrameTraceFileStore {}.load(bundleRoot / "run.trace");

	Expect(result.output.debugBundleSaveAttempted, "game loop should attempt configured debug bundle save");
	Expect(result.output.debugBundleSaved, "game loop should report successful debug bundle save");
	Expect(manifest.has_value(), "game loop debug bundle should write manifest");
	Expect(trace.has_value(), "game loop debug bundle should write run trace");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "trace=run.trace saved=true"), "game loop debug bundle manifest should index saved trace");
	Expect(trace.has_value() && !trace->empty() && (*trace)[0] == "run frames=1 frameReports=1 rawInput=0 sessionResults=0 inventoryScripts=0 inventoryResults=0 movementQueued=0", "game loop debug bundle trace should include run summary");

	std::filesystem::remove_all(root);
}

void TestGameLoopSavesDebugBundleOnStartupFailure()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_failed_startup_debug_bundle_test";
	const std::filesystem::path bundleRoot = root / "debug-bundle";
	std::filesystem::remove_all(root);

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .setup = { .startupScript = root / "missing.iscl" },
		    .output = { .debugBundlePath = bundleRoot },
		    .frame = { .maxFrames = 1 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();
	std::optional<std::vector<std::string>> manifest = dev::RuntimeFrameTraceFileStore {}.load(bundleRoot / "manifest.txt");
	std::optional<std::vector<std::string>> trace = dev::RuntimeFrameTraceFileStore {}.load(bundleRoot / "run.trace");

	Expect(result.setup.startupScriptRan, "failed startup debug bundle test should attempt startup script");
	Expect(result.setup.startupScriptResult.status == dev::SessionScriptRunStatus::LoadFailed, "failed startup debug bundle test should report load failure");
	Expect(result.output.debugBundleSaveAttempted, "game loop should attempt debug bundle save after startup failure");
	Expect(result.output.debugBundleSaved, "game loop should save debug bundle after startup failure");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "setup startupScriptRan=true inventoryScriptRan=false"), "failed startup debug bundle manifest should record setup attempt");
	Expect(trace.has_value() && !trace->empty() && (*trace)[0] == "run frames=0 frameReports=0 rawInput=0 sessionResults=0 inventoryScripts=0 inventoryResults=0 movementQueued=0", "failed startup debug bundle trace should preserve zero-frame summary");

	std::filesystem::remove_all(root);
}

void TestGameLoopReportsDebugBundleSaveFailure()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_debug_bundle_failure_test";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root.parent_path());
	{
		std::ofstream output { root, std::ios::trunc };
		output << "not a directory\n";
	}

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root.parent_path() / "saves",
		    .output = { .debugBundlePath = root },
		    .frame = { .maxFrames = 0 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	Expect(result.output.debugBundleSaveAttempted, "game loop should attempt configured debug bundle even when path is invalid");
	Expect(!result.output.debugBundleSaved, "game loop should report failed debug bundle save");

	dev::GameLoop exitLoop {
		dev::GameLoopSettings {
		    .saveRoot = root.parent_path() / "other-saves",
		    .output = { .debugBundlePath = root },
		    .frame = { .maxFrames = 0 },
		}
	};
	Expect(exitLoop.run() == 1, "game loop run should fail when requested debug bundle cannot be saved");

	std::filesystem::remove(root);
}

void TestGameLoopDrainsRuntimeInventoryCommandSources()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_inventory_source_test";
	std::filesystem::remove_all(root);

	dev::QueuedInventoryCommandSource inventoryCommands;
	inventoryCommands.enqueue({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 950,
	});

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = { .inventoryCommandSources = { &inventoryCommands } },
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	loop.session().world().players[0].inventory.items.push_back({
	    .id = 950,
	    .equipmentSlot = dev::EquipmentSlot::Weapon,
	});

	dev::GameLoopResult result = loop.runForResult();

	Expect(result.summary.inventoryCommandResults.size() == 1, "game loop should dispatch runtime inventory command source");
	Expect(result.summary.inventoryCommandResults.size() == 1 && result.summary.inventoryCommandResults[0].type == dev::InventoryCommandResultType::Applied, "game loop should apply valid runtime inventory command");
	Expect(loop.session().world().players[0].inventory.items.empty(), "runtime inventory equip should remove item from bag");
	Expect(loop.session().world().players[0].inventory.equipment.weapon.has_value() && loop.session().world().players[0].inventory.equipment.weapon->id == 950, "runtime inventory equip should fill equipment slot");
	Expect(inventoryCommands.empty(), "game loop should drain runtime inventory command source");
	Expect(loop.inventoryEvents().events().size() == 1, "game loop should record runtime inventory event");
	Expect(loop.inventoryEvents().events().size() == 1 && loop.inventoryEvents().events()[0].type == dev::InventoryEventType::Equipped, "game loop inventory event should report equipped item");
	Expect(loop.inventoryEvents().events().size() == 1 && loop.inventoryEvents().events()[0].itemId == 950, "game loop inventory event should preserve item id");

	std::filesystem::remove_all(root);
}

void TestGameLoopEmitsRejectedInventoryEventForMissingPlayer()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_missing_player_inventory_event_test";
	std::filesystem::remove_all(root);

	dev::QueuedInventoryCommandSource inventoryCommands;
	inventoryCommands.enqueue({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 952,
	});

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = { .inventoryCommandSources = { &inventoryCommands } },
		    .input = { .playerId = 3 },
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });

	dev::GameLoopResult result = loop.runForResult();

	Expect(result.summary.inventoryCommandResults.size() == 1, "game loop should reject inventory command for missing player");
	Expect(result.summary.inventoryCommandResults.size() == 1 && result.summary.inventoryCommandResults[0].type == dev::InventoryCommandResultType::Rejected, "missing player inventory command should be rejected");
	Expect(loop.inventoryEvents().events().size() == 1, "missing player inventory command should emit rejected event");
	Expect(loop.inventoryEvents().events().size() == 1 && loop.inventoryEvents().events()[0].type == dev::InventoryEventType::Rejected, "missing player inventory event should be rejected");
	Expect(loop.inventoryEvents().events().size() == 1 && loop.inventoryEvents().events()[0].commandType == dev::InventoryCommandType::EquipItem, "missing player inventory event should preserve command type");
	Expect(inventoryCommands.empty(), "missing player inventory command should still drain after rejection");

	std::filesystem::remove_all(root);
}

void TestGameLoopDoesNotDrainInventorySourcesWithoutActiveWorld()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_no_world_inventory_source_test";
	std::filesystem::remove_all(root);

	dev::QueuedInventoryCommandSource inventoryCommands;
	inventoryCommands.enqueue({
	    .type = dev::InventoryCommandType::EquipItem,
	    .itemId = 951,
	});

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = { .inventoryCommandSources = { &inventoryCommands } },
		    .frame = { .maxFrames = 1 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	Expect(result.summary.inventoryCommandResults.empty(), "game loop should not dispatch inventory commands without an active world");
	Expect(inventoryCommands.size() == 1, "game loop should preserve inventory commands until a world exists");

	std::filesystem::remove_all(root);
}

void TestGameLoopDoesNotDrainMovementSourcesWithoutActiveWorld()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_movement_source_empty_test";
	std::filesystem::remove_all(root);

	dev::QueuedMovementCommandSource movementSource;
	movementSource.enqueue({
	    .type = dev::MovementCommandType::WalkTo,
	    .playerId = 0,
	    .destination = { 1, 0 },
	});

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = { .movementCommandSources = { &movementSource } },
		    .frame = { .maxFrames = 1 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	Expect(result.summary.movementCommandsQueued == 0, "game loop should not queue movement commands without an active world");
	Expect(result.summary.framesRun == 1, "game loop should still run bounded frames without an active world");
	Expect(movementSource.size() == 1, "movement source should keep commands when no active world can receive them");
	Expect(result.finalMode == dev::GameSessionMode::Empty, "movement-only loop should leave session empty");

	std::filesystem::remove_all(root);
}

void TestRuntimeInputRouterMapsMouseClickToMovementCommand()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 0, 0 }));

	dev::QueuedSessionCommandSource sessionCommands;
	dev::QueuedMovementCommandSource movementCommands;
	dev::RuntimeInputRouter router { sessionCommands, movementCommands };

	dev::RuntimeInputRouteResult result = router.route(
	    dev::RawInputEvent {
	        .type = dev::RawInputType::MouseClick,
	        .screenPosition = { 96, 64 },
	        .pressed = true,
	    },
	    dev::RuntimeInputContext {
	        .world = &world,
	        .sessionMode = dev::GameSessionMode::Gameplay,
	    });

	Expect(result.handled, "runtime input router should handle gameplay mouse click");
	Expect(result.queuedMovementCommand, "runtime input router should queue movement for mouse click");
	Expect(!result.queuedSessionCommand, "runtime input router should not queue session command for mouse click");
	Expect(movementCommands.size() == 1, "runtime input router should enqueue one movement command");
	std::vector<dev::MovementCommand> drained = movementCommands.drain();
	Expect(drained.size() == 1 && drained[0].type == dev::MovementCommandType::WalkTo, "runtime input router should map mouse click to WalkTo");
	Expect(drained.size() == 1 && drained[0].destination == dev::Point { 3, 2 }, "runtime input router should map screen position through tile map");
	Expect(sessionCommands.empty(), "runtime input router should leave session queue empty for movement input");
}

void TestRuntimeInputRouterBlocksMovementWhenFocusDoesNotOwnGameplay()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 0, 0 }));

	dev::QueuedSessionCommandSource sessionCommands;
	dev::QueuedMovementCommandSource movementCommands;
	dev::RuntimeInputRouter router { sessionCommands, movementCommands };

	dev::RuntimeInputRouteResult inventoryResult = router.route(
	    dev::RawInputEvent {
	        .type = dev::RawInputType::MouseClick,
	        .screenPosition = { 32, 0 },
	        .pressed = true,
	    },
	    dev::RuntimeInputContext {
	        .world = &world,
	        .focusState = dev::FocusState { .owner = dev::InputOwner::Inventory },
	        .sessionMode = dev::GameSessionMode::Inventory,
	    });

	dev::RuntimeInputRouteResult textEntryResult = router.route(
	    dev::RawInputEvent {
	        .type = dev::RawInputType::MouseClick,
	        .screenPosition = { 64, 0 },
	        .pressed = true,
	    },
	    dev::RuntimeInputContext {
	        .world = &world,
	        .focusState = dev::FocusState { .owner = dev::InputOwner::Gameplay, .textEntryActive = true },
	        .sessionMode = dev::GameSessionMode::Gameplay,
	    });

	Expect(!inventoryResult.handled, "runtime input router should ignore movement while inventory owns focus");
	Expect(!textEntryResult.handled, "runtime input router should ignore movement while text entry is active");
	Expect(movementCommands.empty(), "runtime input router should not queue blocked movement input");
	Expect(sessionCommands.empty(), "runtime input router should not convert blocked movement into session commands");
}

void TestRuntimeInputRouterMapsHotkeysToSessionCommands()
{
	dev::QueuedSessionCommandSource sessionCommands;
	dev::QueuedMovementCommandSource movementCommands;
	dev::RuntimeInputRouter router {
		sessionCommands,
		movementCommands,
		dev::RuntimeInputBindings { .pauseKey = 'P', .inventoryKey = 'B', .stopKey = 'X' },
	};

	dev::RuntimeInputRouteResult pauseResult = router.route(
	    dev::RawInputEvent {
	        .type = dev::RawInputType::KeyPress,
	        .code = 'P',
	        .pressed = true,
	    },
	    dev::RuntimeInputContext {
	        .sessionMode = dev::GameSessionMode::Gameplay,
	    });

	dev::RuntimeInputRouteResult inventoryResult = router.route(
	    dev::RawInputEvent {
	        .type = dev::RawInputType::KeyPress,
	        .code = 'B',
	        .pressed = true,
	    },
	    dev::RuntimeInputContext {
	        .sessionMode = dev::GameSessionMode::Gameplay,
	    });

	Expect(pauseResult.handled && pauseResult.queuedSessionCommand, "runtime input router should map pause hotkey to session command");
	Expect(inventoryResult.handled && inventoryResult.queuedSessionCommand, "runtime input router should map inventory hotkey to session command");
	Expect(sessionCommands.size() == 2, "runtime input router should queue session hotkey commands");
	Expect(movementCommands.empty(), "runtime input router should not queue movement for session hotkeys");

	std::vector<dev::SessionCommand> commands = sessionCommands.drain();
	Expect(commands.size() == 2 && commands[0].mode == std::optional<dev::GameSessionMode> { dev::GameSessionMode::Paused }, "pause hotkey should request paused mode");
	Expect(commands.size() == 2 && commands[1].mode == std::optional<dev::GameSessionMode> { dev::GameSessionMode::Inventory }, "inventory hotkey should request inventory mode");
}

void TestRuntimeInputRouterMapsStopHotkeyToMovementCommand()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 7, 4 }));

	dev::QueuedSessionCommandSource sessionCommands;
	dev::QueuedMovementCommandSource movementCommands;
	dev::RuntimeInputRouter router {
		sessionCommands,
		movementCommands,
		dev::RuntimeInputBindings { .pauseKey = 'P', .inventoryKey = 'I', .stopKey = 'Q' },
	};

	dev::RuntimeInputRouteResult result = router.route(
	    dev::RawInputEvent {
	        .type = dev::RawInputType::KeyPress,
	        .code = 'Q',
	        .pressed = true,
	    },
	    dev::RuntimeInputContext {
	        .world = &world,
	        .sessionMode = dev::GameSessionMode::Gameplay,
	    });

	Expect(result.handled && result.queuedMovementCommand, "runtime input router should map stop hotkey to movement command");
	std::vector<dev::MovementCommand> commands = movementCommands.drain();
	Expect(commands.size() == 1 && commands[0].type == dev::MovementCommandType::Stop, "stop hotkey should request Stop movement command");
	Expect(commands.size() == 1 && commands[0].destination == dev::Point { 7, 4 }, "stop hotkey should use current player tile");
	Expect(sessionCommands.empty(), "stop hotkey should not queue session commands");
}

void TestRuntimeInputRouterMapsTargetClickToMoveThenAct()
{
	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 0, 0 }));
	FixedTargetResolver targets {
		dev::Target {
		    .type = dev::TargetType::Enemy,
		    .id = 42,
		    .tile = { 0, 0 },
		}
	};

	dev::QueuedSessionCommandSource sessionCommands;
	dev::QueuedMovementCommandSource movementCommands;
	dev::RuntimeInputRouter router { sessionCommands, movementCommands };

	dev::RuntimeInputRouteResult result = router.route(
	    dev::RawInputEvent {
	        .type = dev::RawInputType::MouseClick,
	        .screenPosition = { 64, 0 },
	        .pressed = true,
	    },
	    dev::RuntimeInputContext {
	        .world = &world,
	        .sessionMode = dev::GameSessionMode::Gameplay,
	        .targetResolver = &targets,
	    });

	Expect(result.handled && result.queuedMovementCommand, "runtime input router should route target-aware click");
	std::vector<dev::MovementCommand> commands = movementCommands.drain();
	Expect(commands.size() == 1 && commands[0].type == dev::MovementCommandType::MoveThenAct, "enemy click should become MoveThenAct");
	Expect(commands.size() == 1 && commands[0].destination == dev::Point { 2, 0 }, "target-aware click should use clicked tile");
	Expect(commands.size() == 1 && commands[0].destinationAction.has_value(), "enemy click should carry destination action");
	Expect(commands.size() == 1 && commands[0].destinationAction->type == dev::DestinationActionType::Attack, "enemy click should carry attack action");
	Expect(commands.size() == 1 && commands[0].destinationAction->target.id == 42, "enemy click should preserve target id");
	Expect(sessionCommands.empty(), "target-aware click should not queue session commands");
}

void TestRuntimeInputRouterMapsStandGroundTargetClickToStandAndAct()
{
	dev::SimulationWorld world;
	dev::Player player = MakePlayer({ 0, 0 });
	player.movementModifiers.standGround = true;
	world.players.push_back(player);
	FixedTargetResolver targets {
		dev::Target {
		    .type = dev::TargetType::Enemy,
		    .id = 77,
		    .tile = { 0, 0 },
		}
	};

	dev::QueuedSessionCommandSource sessionCommands;
	dev::QueuedMovementCommandSource movementCommands;
	dev::RuntimeInputRouter router { sessionCommands, movementCommands };

	dev::RuntimeInputRouteResult result = router.route(
	    dev::RawInputEvent {
	        .type = dev::RawInputType::MouseClick,
	        .screenPosition = { 32, 0 },
	        .pressed = true,
	    },
	    dev::RuntimeInputContext {
	        .world = &world,
	        .sessionMode = dev::GameSessionMode::Gameplay,
	        .targetResolver = &targets,
	    });

	Expect(result.handled && result.queuedMovementCommand, "runtime input router should route stand-ground target click");
	std::vector<dev::MovementCommand> commands = movementCommands.drain();
	Expect(commands.size() == 1 && commands[0].type == dev::MovementCommandType::StandAndAct, "stand-ground enemy click should become StandAndAct");
	Expect(commands.size() == 1 && commands[0].destination == dev::Point { 1, 0 }, "stand-ground target click should preserve clicked tile");
	Expect(commands.size() == 1 && commands[0].destinationAction->type == dev::DestinationActionType::Attack, "stand-ground target click should carry attack action");
}

void TestQueuedRawInputSourceDrainsEventsOnce()
{
	dev::QueuedRawInputSource source;
	source.enqueue({
	    .type = dev::RawInputType::MouseClick,
	    .screenPosition = { 32, 64 },
	    .pressed = true,
	});
	source.enqueue({
	    .type = dev::RawInputType::KeyPress,
	    .code = 'P',
	    .pressed = true,
	});

	Expect(source.size() == 2, "queued raw input source should track queued event count");
	std::vector<dev::RawInputEvent> drained = source.drain();
	Expect(drained.size() == 2, "queued raw input source should drain queued events");
	Expect(source.empty(), "queued raw input source should be empty after drain");
	Expect(source.drain().empty(), "queued raw input source should not drain events twice");
	Expect(drained.size() == 2 && drained[0].screenPosition == dev::Point { 32, 64 }, "queued raw input source should preserve event payloads");
}

void TestRuntimeRawInputDrainerRoutesHandledEventsAndSkipsNullSources()
{
	dev::QueuedRawInputSource rawInput;
	rawInput.enqueue({
	    .type = dev::RawInputType::KeyPress,
	    .code = 'P',
	    .pressed = true,
	});
	rawInput.enqueue({
	    .type = dev::RawInputType::KeyPress,
	    .code = 'P',
	    .pressed = false,
	});

	dev::QueuedSessionCommandSource sessionCommands;
	dev::QueuedMovementCommandSource movementCommands;
	dev::RuntimeInputRouter router {
		sessionCommands,
		movementCommands,
		dev::RuntimeInputBindings { .pauseKey = 'P', .inventoryKey = 'I', .stopKey = 'S' },
	};
	dev::RuntimeRawInputDrainer drainer { router };

	const int routed = drainer.drain(
	    { nullptr, &rawInput },
	    dev::RuntimeInputContext {
	        .sessionMode = dev::GameSessionMode::Gameplay,
	    });

	Expect(routed == 1, "runtime raw input drainer should count handled routed events only");
	Expect(rawInput.empty(), "runtime raw input drainer should drain source events once");
	Expect(sessionCommands.size() == 1, "runtime raw input drainer should route handled hotkeys through session commands");
	Expect(movementCommands.empty(), "runtime raw input drainer should not invent movement commands for hotkeys");
}

void TestRuntimeFrameRunnerRoutesSourcesAndRecordsOneFrame()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_frame_runner_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root / "saves" };
	session.startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	dev::SessionEventRecorder sessionEvents;
	dev::InventoryEventRecorder inventoryEvents;
	dev::QueuedSessionCommandSource routedSessionCommands;
	dev::QueuedMovementCommandSource routedMovementCommands;
	dev::QueuedRawInputSource rawInput;
	rawInput.enqueue({
	    .type = dev::RawInputType::KeyPress,
	    .code = 'P',
	    .pressed = true,
	});

	dev::GameLoopResult result;
	dev::RuntimeRunRecorder recorder { result, sessionEvents, inventoryEvents };
	dev::SessionCommandDispatcher dispatcher { session, &sessionEvents };
	dev::RuntimeSourceDrainer sourceDrainer {
		session,
		inventoryEvents,
		routedSessionCommands,
		routedMovementCommands,
		{},
	};
	dev::RuntimeSourceSettings sources {
		.rawInputSources = { &rawInput },
	};
	dev::RuntimeInputSettings input {
		.bindings = dev::RuntimeInputBindings { .pauseKey = 'P', .inventoryKey = 'I', .stopKey = 'S' },
	};
	dev::RuntimeFrameRunner {
		session,
		routedSessionCommands,
		routedMovementCommands,
		sourceDrainer,
		recorder,
		dispatcher,
		sources,
		input,
		dev::RuntimeFrameSettings { .maxFrames = 1 },
	}.runFrame();

	Expect(result.summary.framesRun == 1, "runtime frame runner should finish one frame");
	Expect(result.summary.rawInputEventsRouted == 1, "runtime frame runner should route raw input during the frame");
	Expect(result.summary.sessionCommandResults.size() == 1, "runtime frame runner should drain routed session commands");
	Expect(result.summary.sessionCommandResults.size() == 1 && result.summary.sessionCommandResults[0].type == dev::SessionCommandResultType::Applied, "runtime frame runner should dispatch routed session commands");
	Expect(result.frameReports.size() == 1, "runtime frame runner should record one frame report");
	Expect(result.frameReports.size() == 1 && result.frameReports[0].rawInputEventsRouted == 1, "runtime frame runner report should include raw input count");
	Expect(result.frameReports.size() == 1 && result.frameReports[0].sessionCommandResults.size() == 1, "runtime frame runner report should include session command results");
	Expect(rawInput.empty(), "runtime frame runner should drain raw input sources once");
	Expect(routedSessionCommands.empty(), "runtime frame runner should drain routed session queue");
	Expect(session.mode() == dev::GameSessionMode::Paused, "runtime frame runner should apply routed pause command before simulation update");

	std::filesystem::remove_all(root);
}

void TestGameLoopRoutesRawInputHotkeysThroughSessionCommands()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_raw_session_input_test";
	const std::filesystem::path scriptPath = root / "startup.iscl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 0, 0 }, .playerHitPoints = 20 },
	});

	dev::SessionCommandLogFileStore store;
	Expect(store.save(scriptPath, log), "raw session input test should create startup script");

	dev::QueuedRawInputSource rawInput;
	rawInput.enqueue({
	    .type = dev::RawInputType::KeyPress,
	    .code = 'P',
	    .pressed = true,
	});

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .setup = { .startupScript = scriptPath },
		    .sources = { .rawInputSources = { &rawInput } },
		    .input = { .bindings = dev::RuntimeInputBindings { .pauseKey = 'P', .inventoryKey = 'I', .stopKey = 'S' } },
		    .frame = { .maxFrames = 1 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	Expect(result.summary.rawInputEventsRouted == 1, "game loop should route raw hotkey input");
	Expect(result.summary.sessionCommandResults.size() == 1, "game loop should dispatch routed session command");
	Expect(result.summary.sessionCommandResults.size() == 1 && result.summary.sessionCommandResults[0].type == dev::SessionCommandResultType::Applied, "routed pause hotkey should apply");
	Expect(result.finalMode == dev::GameSessionMode::Paused, "routed pause hotkey should update final mode");
	Expect(rawInput.empty(), "game loop should drain raw input source once");
	Expect(loop.sessionEvents().events().size() == 2, "startup script and routed hotkey should share session event sink");

	std::filesystem::remove_all(root);
}

void TestGameLoopRoutesRawMouseInputThroughMovementCommands()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_raw_movement_input_test";
	const std::filesystem::path scriptPath = root / "startup.iscl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 0, 0 }, .playerHitPoints = 20 },
	});

	dev::SessionCommandLogFileStore store;
	Expect(store.save(scriptPath, log), "raw movement input test should create startup script");

	dev::QueuedRawInputSource rawInput;
	rawInput.enqueue({
	    .type = dev::RawInputType::MouseClick,
	    .screenPosition = { 32, 0 },
	    .pressed = true,
	});

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .setup = { .startupScript = scriptPath },
		    .sources = { .rawInputSources = { &rawInput } },
		    .frame = { .maxFrames = 1 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	bool sawWalkCommand = false;
	for (const dev::MovementEvent &event : result.summary.lastFrameEvents.movementEvents()) {
		if (event.type == dev::MovementEventType::CommandAccepted && event.commandType == dev::MovementCommandType::WalkTo)
			sawWalkCommand = true;
	}

	Expect(result.summary.rawInputEventsRouted == 1, "game loop should route raw mouse input");
	Expect(result.summary.movementCommandsQueued == 1, "game loop should queue movement command from routed raw input");
	Expect(loop.session().world().players.size() == 1 && loop.session().world().players[0].position.tile == dev::Point { 1, 0 }, "routed raw mouse input should move player through simulation");
	Expect(sawWalkCommand, "routed raw mouse input should still emit movement command events");
	Expect(rawInput.empty(), "game loop should drain raw mouse input source once");

	std::filesystem::remove_all(root);
}

void TestGameLoopUsesWorldTargetRegistryForRawMouseInput()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_world_target_input_test";
	std::filesystem::remove_all(root);

	dev::QueuedRawInputSource rawInput;
	rawInput.enqueue({
	    .type = dev::RawInputType::MouseClick,
	    .screenPosition = { 32, 0 },
	    .pressed = true,
	});

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = { .rawInputSources = { &rawInput } },
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	loop.session().world().targets.add({ .type = dev::TargetType::Enemy, .id = 301, .tile = { 1, 0 } });

	dev::GameLoopResult result = loop.runForResult();

	bool sawMoveThenAct = false;
	bool sawActionRejected = false;
	for (const dev::MovementEvent &event : result.summary.lastFrameEvents.movementEvents()) {
		if (event.type == dev::MovementEventType::CommandAccepted && event.commandType == dev::MovementCommandType::MoveThenAct)
			sawMoveThenAct = true;
		if (event.type == dev::MovementEventType::ActionRejected)
			sawActionRejected = true;
	}

	Expect(result.summary.rawInputEventsRouted == 1, "game loop should route raw target-aware mouse input through world registry");
	Expect(result.summary.movementCommandsQueued == 1, "game loop should queue world-target movement command");
	Expect(sawMoveThenAct, "world target registry click should become MoveThenAct command");
	Expect(sawActionRejected, "unregistered combat target should still reject at action layer");
	Expect(loop.session().world().players.size() == 1 && loop.session().world().players[0].position.tile == dev::Point { 1, 0 }, "world target registry click should move player to target tile");

	std::filesystem::remove_all(root);
}

void TestGameLoopUsesWorldItemTargetForRawPickupInput()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_world_item_input_test";
	std::filesystem::remove_all(root);

	dev::QueuedRawInputSource rawInput;
	rawInput.enqueue({
	    .type = dev::RawInputType::MouseClick,
	    .screenPosition = { 32, 0 },
	    .pressed = true,
	});

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = { .rawInputSources = { &rawInput } },
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	dev::WorldEntityService {}.spawnItem(loop.session().world(), {
	    .id = 701,
	    .tile = { 1, 0 },
	});

	dev::GameLoopResult result = loop.runForResult();

	bool sawMoveThenAct = false;
	bool sawPickupExecuted = false;
	for (const dev::MovementEvent &event : result.summary.lastFrameEvents.movementEvents()) {
		if (event.type == dev::MovementEventType::CommandAccepted && event.commandType == dev::MovementCommandType::MoveThenAct)
			sawMoveThenAct = true;
		if (event.type == dev::MovementEventType::ActionExecuted && event.actionType == dev::DestinationActionType::Pickup)
			sawPickupExecuted = true;
	}

	Expect(result.summary.rawInputEventsRouted == 1, "game loop should route raw item mouse input through world registry");
	Expect(result.summary.movementCommandsQueued == 1, "game loop should queue world-item movement command");
	Expect(sawMoveThenAct, "world item target click should become MoveThenAct command");
	Expect(sawPickupExecuted, "world item target click should execute pickup action");
	Expect(loop.session().world().items.empty(), "pickup action should remove item from world");
	Expect(loop.session().world().targets.resolveAtTile({ 1, 0 }).type == dev::TargetType::EmptyTile, "pickup action should remove item target");
	Expect(loop.session().world().players.size() == 1 && loop.session().world().players[0].inventory.items.size() == 1, "pickup action should transfer item to player inventory");
	Expect(loop.session().world().players.size() == 1 && loop.session().world().players[0].inventory.items[0].id == 701, "pickup action should preserve transferred item identity");

	std::filesystem::remove_all(root);
}

void TestGameLoopLeavesItemWhenInventoryFull()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_full_inventory_pickup_test";
	std::filesystem::remove_all(root);

	dev::QueuedRawInputSource rawInput;
	rawInput.enqueue({
	    .type = dev::RawInputType::MouseClick,
	    .screenPosition = { 32, 0 },
	    .pressed = true,
	});

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .sources = { .rawInputSources = { &rawInput } },
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	loop.session().world().players[0].inventory.capacity = 1;
	loop.session().world().players[0].inventory.items.push_back({ .id = 800, .tile = { 0, 0 } });
	dev::WorldEntityService {}.spawnItem(loop.session().world(), {
	    .id = 801,
	    .tile = { 1, 0 },
	});

	dev::GameLoopResult result = loop.runForResult();

	bool sawPickupExecuted = false;
	for (const dev::MovementEvent &event : result.summary.lastFrameEvents.movementEvents()) {
		if (event.type == dev::MovementEventType::ActionExecuted && event.actionType == dev::DestinationActionType::Pickup)
			sawPickupExecuted = true;
	}

	Expect(sawPickupExecuted, "full inventory pickup should still execute pickup action");
	Expect(loop.session().world().items.size() == 1 && loop.session().world().items[0].id == 801, "full inventory pickup should leave item in world");
	Expect(loop.session().world().targets.resolveAtTile({ 1, 0 }).type == dev::TargetType::Item, "full inventory pickup should leave item target");
	Expect(loop.session().world().players[0].inventory.items.size() == 1 && loop.session().world().players[0].inventory.items[0].id == 800, "full inventory pickup should not alter inventory");

	std::filesystem::remove_all(root);
}

void TestGameLoopDoesNotRouteBlockedRawMovementInput()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_game_loop_raw_blocked_input_test";
	const std::filesystem::path scriptPath = root / "startup.iscl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 0, 0 }, .playerHitPoints = 20 },
	});

	dev::SessionCommandLogFileStore store;
	Expect(store.save(scriptPath, log), "blocked raw input test should create startup script");

	dev::QueuedRawInputSource rawInput;
	rawInput.enqueue({
	    .type = dev::RawInputType::MouseClick,
	    .screenPosition = { 32, 0 },
	    .pressed = true,
	});

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .setup = { .startupScript = scriptPath },
		    .sources = { .rawInputSources = { &rawInput } },
		    .input = { .focusState = dev::FocusState { .owner = dev::InputOwner::Gameplay, .textEntryActive = true } },
		    .frame = { .maxFrames = 1 },
		}
	};
	dev::GameLoopResult result = loop.runForResult();

	Expect(result.summary.rawInputEventsRouted == 0, "game loop should not count blocked raw movement as routed");
	Expect(result.summary.movementCommandsQueued == 0, "game loop should not queue blocked raw movement");
	Expect(loop.session().world().players.size() == 1 && loop.session().world().players[0].position.tile == dev::Point { 0, 0 }, "blocked raw movement should not move player");
	Expect(rawInput.empty(), "game loop should still drain inspected raw input");

	std::filesystem::remove_all(root);
}

} // namespace

int main()
{
	TestInventoryFocusBlocksMovement();
	TestStandGroundCreatesStandAndAct();
	TestMoveThenActExecutesAfterPath();
	TestDiagonalCornerPolicyBlocksCornerCutting();
	TestActionExecutorWaitsOutOfRange();
	TestMoveThenActEventSequence();
	TestCommandReplayProducesSameEventSequence();
	TestMovementCodecRoundTrip();
	TestEnemyPursuitObeysStepBudget();
	TestEnemyAttackWindupAndRecovery();
	TestCombatResolverDamageAndDefeat();
	TestActionExecutorAttackResolvesCombat();
	TestCombatSystemEmitsHitEvent();
	TestCombatSystemEmitsDefeatedEvent();
	TestPlayerAttackUsesEquippedCombatModifiers();
	TestEnemyAttackUsesEquippedDefenseModifiers();
	TestEnemyAttackResolvesCombatAgainstPlayer();
	TestSimulationCommandDrainerDispatchesQueuedMovementCommands();
	TestSimulationPlayerUpdaterAdvancesPlayerMovement();
	TestSimulationTickDispatchesMovementAndCombat();
	TestSimulationPolicyPausedDoesNotDrainCommands();
	TestSimulationClockHitStopFreezesActorUpdates();
	TestSimulationClockScalesEnemyWindup();
	TestEffectRouterMapsMovementEventsToRequests();
	TestEffectRouterMapsCombatHitToRequests();
	TestEffectApplierAppliesHitStopToClock();
	TestSimulationEffectPipelineRoutesAndAppliesEffects();
	TestSimulationFrameEventCaptureCollectsForwardsAndRestoresSinks();
	TestSimulationFrameFinalizerAppliesConsequences();
	TestSimulationFrameRunnerProcessesConsequences();
	TestTargetRegistryResolvesAndRemovesTargets();
	TestTargetSynchronizerSyncsEnemyTargetsWithoutRemovingObjects();
	TestTargetSynchronizerSkipsDefeatedEnemies();
	TestFrameRunnerSynchronizesMovedEnemyTargets();
	TestFrameRunnerRemovesDefeatedEnemyTargets();
	TestWorldEntityServiceSpawnsEnemyStateAcrossRegistries();
	TestWorldEntityServiceDespawnsEnemyStateAcrossRegistries();
	TestWorldEntityServiceRespawnReplacesStaleEnemyState();
	TestWorldEntityServiceSpawnsAndDespawnsItems();
	TestWorldEntityServiceRespawnReplacesStaleItemTarget();
	TestInventoryServiceTransfersExecutedPickupToPlayerInventory();
	TestInventoryServiceIgnoresInvalidPickupEvents();
	TestInventoryServiceRejectsPickupWhenInventoryIsFull();
	TestEquipmentServiceEquipsItemFromInventory();
	TestEquipmentServiceSwapsOccupiedSlot();
	TestEquipmentServiceRejectsMissingAndNotEquippableItems();
	TestEquipmentServiceUnequipsWhenInventoryHasCapacity();
	TestEquipmentServiceRejectsUnequipWhenInventoryFull();
	TestEquipmentStatsServiceBuildsEffectiveCombatStats();
	TestInventoryCommandDispatcherEquipsItem();
	TestInventoryCommandDispatcherUnequipsSlot();
	TestInventoryCommandDispatcherRejectsInvalidCommands();
	TestInventoryCommandDispatcherEmitsInventoryEvents();
	TestInventoryCommandCodecRoundTripsCommands();
	TestInventoryCommandCodecRejectsInvalidPackets();
	TestInventoryCommandLogReplaysThroughDispatcher();
	TestInventoryCommandLogCodecRoundTripsAndReplays();
	TestInventoryCommandLogCodecRejectsInvalidBytes();
	TestInventoryCommandLogFileStoreSavesLoadsAndReplays();
	TestInventoryCommandLogFileStoreRejectsCorruptAndMissingFiles();
	TestInventoryScriptRunnerRunsSavedInventoryScript();
	TestInventoryScriptRunnerReportsLoadFailureAndCommandRejectionSeparately();
	TestSimulationSnapshotRestoresDurableState();
	TestSnapshotCodecRoundTripsVersionedBytes();
	TestSnapshotCodecRejectsInvalidBytes();
	TestSnapshotFileStoreSavesAndLoadsVersionedBytes();
	TestSnapshotFileStoreRejectsCorruptFile();
	TestSaveGameServiceSavesAndLoadsWorld();
	TestSaveSlotServiceListsMetadata();
	TestSaveSlotServiceLoadsWorld();
	TestGameSessionStartsNewGameAndUpdates();
	TestGameSessionPausedModePreservesCommands();
	TestGameSessionSaveLoadPreservesSinksAndResetsClock();
	TestGameSessionMissingLoadKeepsCurrentWorld();
	TestSessionCommandDispatcherAppliesLifecycleCommands();
	TestSessionCommandDispatcherRejectsInvalidLifecycleCommands();
	TestSessionCommandDispatcherEmitsSuccessEvents();
	TestSessionCommandDispatcherEmitsFailureEvents();
	TestSessionCommandReplayAppliesLifecycleSequence();
	TestSessionCommandReplayReportsRejectedCommands();
	TestSessionCommandCodecRoundTripsCommands();
	TestSessionCommandCodecRejectsInvalidPackets();
	TestSessionCommandLogCodecRoundTripsAndReplays();
	TestSessionCommandLogCodecRejectsInvalidBytes();
	TestSessionCommandLogFileStoreSavesLoadsAndReplays();
	TestSessionCommandLogFileStoreRejectsCorruptAndMissingFiles();
	TestSessionScriptRunnerRunsSavedLifecycleScript();
	TestSessionScriptRunnerReportsLoadFailureAndCommandRejectionSeparately();
	TestGameLoopRunsStartupScriptAndFrames();
	TestGameLoopReportsStartupScriptLoadFailure();
	TestGameLoopRunsInventoryScriptAgainstActivePlayer();
	TestGameLoopRunsInventoryScriptAfterStartupScript();
	TestGameLoopReportsInventoryScriptLoadFailure();
	TestGameLoopReportsInventoryScriptWithoutActivePlayer();
	TestQueuedSessionCommandSourceDrainsCommandsOnce();
	TestGameLoopDrainsRuntimeSessionCommandSources();
	TestGameLoopRunsStartupScriptBeforeRuntimeCommandSources();
	TestQueuedMovementCommandSourceDrainsCommandsOnce();
	TestGameLoopDrainsRuntimeMovementCommandSources();
	TestQueuedInventoryCommandSourceDrainsCommandsOnce();
	TestQueuedInventoryScriptSourceDrainsPathsOnce();
	TestRuntimeSourceDrainerSettingsBuilderMapsLoopSourcesAndPlayer();
	TestRuntimeSourceDrainerDrainsSessionBeforeMovement();
	TestGameLoopDrainsRuntimeInventoryScriptSources();
	TestGameLoopReportsRuntimeInventoryScriptLoadFailureWithoutStoppingFrames();
	TestGameLoopDoesNotDrainInventoryScriptSourcesWithoutActiveWorld();
	TestGameLoopBuildsRuntimeFrameReports();
	TestRuntimeFrameTraceFormatsReadableLines();
	TestRuntimeFrameTraceFileStoreSavesAndLoadsLines();
	TestRuntimeFrameTraceFileStoreRejectsMissingFile();
	TestRuntimeTraceServiceFormatsAndSavesRunTrace();
	TestRuntimeTraceServiceFormatsEmptyRun();
	TestRuntimeOutputSettingsDefaultDisablesArtifacts();
	TestRuntimeOutputResultDefaultsToNoAttempts();
	TestRuntimeSetupSettingsDefaultsToNoScripts();
	TestRuntimeSetupResultDefaultsToNoSetupScripts();
	TestRuntimeSetupRunnerAllowsFramesWhenNoScriptsConfigured();
	TestRuntimeSetupRunnerStopsFramesAfterStartupLoadFailure();
	TestRuntimeSetupRunnerPreservesInventoryCommandRejections();
	TestRuntimeSourceSettingsDefaultsToNoSources();
	TestRuntimeInputSettingsDefaultsToPrimaryGameplayInput();
	TestRuntimeInputContextBuilderHandlesMissingWorld();
	TestRuntimeInputContextBuilderUsesWorldTargetsUnlessOverridden();
	TestRuntimeFrameSettingsDefaultsToNoFramesAtSixtyHz();
	TestRuntimeRunSummaryDefaultsToEmptyRun();
	TestRuntimeRunRecorderAggregatesSetupInventoryResultsWithoutFrame();
	TestRuntimeRunRecorderAggregatesFrameReportsAndSummary();
	TestRuntimeRunFinalizerCapturesFinalModeAndLeavesDisabledOutputsUntouched();
	TestRuntimeExitCodePolicyReportsSuccessForCleanRun();
	TestRuntimeExitCodePolicyFailsSetupErrors();
	TestRuntimeExitCodePolicyAllowsCommandRejections();
	TestRuntimeExitCodePolicyFailsOutputErrors();
	TestRuntimeOutputFinalizerLeavesDisabledOutputsUntouched();
	TestRuntimeOutputFinalizerSavesTraceAndBundle();
	TestRuntimeOutputFinalizerReportsRequestedOutputFailure();
	TestGameLoopSavesConfiguredRunTrace();
	TestGameLoopSavesRunTraceOnStartupFailure();
	TestGameLoopReportsRunTraceSaveFailure();
	TestRuntimeDebugArtifactBundleSavesManifestAndTrace();
	TestRuntimeDebugArtifactBundleFormatsManifestForFailedRun();
	TestRuntimeDebugArtifactBundleRejectsRootFile();
	TestGameLoopSavesConfiguredDebugBundle();
	TestGameLoopSavesDebugBundleOnStartupFailure();
	TestGameLoopReportsDebugBundleSaveFailure();
	TestGameLoopDrainsRuntimeInventoryCommandSources();
	TestGameLoopEmitsRejectedInventoryEventForMissingPlayer();
	TestGameLoopDoesNotDrainInventorySourcesWithoutActiveWorld();
	TestGameLoopDoesNotDrainMovementSourcesWithoutActiveWorld();
	TestRuntimeInputRouterMapsMouseClickToMovementCommand();
	TestRuntimeInputRouterBlocksMovementWhenFocusDoesNotOwnGameplay();
	TestRuntimeInputRouterMapsHotkeysToSessionCommands();
	TestRuntimeInputRouterMapsStopHotkeyToMovementCommand();
	TestRuntimeInputRouterMapsTargetClickToMoveThenAct();
	TestRuntimeInputRouterMapsStandGroundTargetClickToStandAndAct();
	TestQueuedRawInputSourceDrainsEventsOnce();
	TestRuntimeRawInputDrainerRoutesHandledEventsAndSkipsNullSources();
	TestRuntimeFrameRunnerRoutesSourcesAndRecordsOneFrame();
	TestGameLoopRoutesRawInputHotkeysThroughSessionCommands();
	TestGameLoopRoutesRawMouseInputThroughMovementCommands();
	TestGameLoopUsesWorldTargetRegistryForRawMouseInput();
	TestGameLoopUsesWorldItemTargetForRawPickupInput();
	TestGameLoopLeavesItemWhenInventoryFull();
	TestGameLoopDoesNotRouteBlockedRawMovementInput();

	if (Failures != 0)
		return EXIT_FAILURE;

	std::cout << "movement_tests passed\n";
	return EXIT_SUCCESS;
}
