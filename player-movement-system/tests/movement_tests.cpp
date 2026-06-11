#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

#include "actions/ActionExecutor.hpp"
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
#include "network/MovementCodec.hpp"
#include "player/PlayerActionGate.hpp"
#include "player/PlayerController.hpp"
#include "player/PlayerMovement.hpp"
#include "replay/CommandLog.hpp"
#include "replay/CommandReplayer.hpp"
#include "simulation/SimulationClock.hpp"
#include "simulation/SimulationFrameRunner.hpp"
#include "simulation/SimulationTick.hpp"
#include "targeting/Target.hpp"
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
	TestEnemyAttackResolvesCombatAgainstPlayer();
	TestSimulationTickDispatchesMovementAndCombat();
	TestSimulationPolicyPausedDoesNotDrainCommands();
	TestSimulationClockHitStopFreezesActorUpdates();
	TestSimulationClockScalesEnemyWindup();
	TestEffectRouterMapsMovementEventsToRequests();
	TestEffectRouterMapsCombatHitToRequests();
	TestEffectApplierAppliesHitStopToClock();
	TestSimulationFrameRunnerProcessesConsequences();

	if (Failures != 0)
		return EXIT_FAILURE;

	std::cout << "movement_tests passed\n";
	return EXIT_SUCCESS;
}
