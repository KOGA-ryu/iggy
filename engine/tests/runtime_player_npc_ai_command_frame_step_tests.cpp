#include <cstdlib>
#include <string_view>
#include <vector>

#include "runtime/RuntimePlayerNpcAiCommandFrameStep.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::LevelTileMap Grid(std::vector<std::string_view> rows)
{
	iggy::LevelTileMap map;
	map.height = static_cast<int>(rows.size());
	map.width = rows.empty() ? 0 : static_cast<int>(rows.front().size());
	for (std::string_view row : rows) {
		for (char cell : row)
			map.tiles.push_back({ cell != '#' });
	}
	return map;
}

iggy::AiMapNode2D Node(const char *id, iggy::Vec2 position, float radius, float patrolWeight)
{
	return {
		Id(id),
		position,
		radius,
		patrolWeight,
		0.0F,
		0.0F,
		0.0F,
		{},
		{},
		true,
	};
}

iggy::NpcAiProfile2D Profile(const char *npcId)
{
	return {
		Id(npcId),
		Id("faction:frame"),
		0.0F,
		0.0F,
		0.0F,
		4.0F,
		{},
	};
}

iggy::NpcAiCurrentState2D State(const char *npcId, iggy::Vec2 position = { 0.5F, 0.5F })
{
	return {
		Id(npcId),
		position,
		Id("goal:frame"),
		true,
	};
}

iggy::runtime::RuntimeNpcAiDecisionQueueNpcInput Npc(
	const char *npcId,
	iggy::Vec2 position = { 0.5F, 0.5F })
{
	return {
		Profile(npcId),
		State(npcId, position),
	};
}

iggy::runtime::RuntimeSessionState Session(iggy::Vec2 playerPosition = { 0.0F, 0.0F })
{
	iggy::runtime::RuntimeSessionState session;
	session.level.map = Grid({ "...." });
	session.level.map.id = Id("level:player-npc-ai-frame");
	session.tickIndex = 7;
	session.hasPlayer = true;
	session.player.id = Id("player:one");
	session.player.position = playerPosition;
	session.player.spawnTile = { 0, 0 };
	session.player.movementStatus = iggy::PlayerMovementStatus::Idle;
	session.player.facing = iggy::PlayerFacing2D::East;
	return session;
}

iggy::runtime::RuntimePlayerCommandExecutionConfig PlayerConfig(float maxStep = 1.0F)
{
	iggy::runtime::RuntimePlayerCommandExecutionConfig config;
	config.movement.maxStep = maxStep;
	return config;
}

iggy::npc_ai::NpcAgentTickConfig NpcConfig()
{
	iggy::npc_ai::NpcAgentTickConfig config;
	config.maxDistance = 0.25F;
	config.awareness = { 8.0F, 0 };
	return config;
}

iggy::runtime::GameplayCommandFrame2D ExistingFrame()
{
	return { { iggy::runtime::GameplayCommand2DFactory {}.wait(Id("actor:existing")) } };
}

iggy::runtime::RuntimePlayerNpcAiCommandFrameInput BaseInput(
	iggy::runtime::RuntimePlayerNpcAiQueueOrder order = iggy::runtime::RuntimePlayerNpcAiQueueOrder::PlayerThenNpcAi)
{
	iggy::runtime::RuntimePlayerNpcAiCommandFrameInput input;
	input.session = Session();
	input.intake.playerActorId = Id("player:one");
	input.intake.playerInputContext = {};
	input.intake.playerIntents = { iggy::playerMoveToPointIntent({ 9.0F, 0.0F }) };
	input.intake.aiMap = { { Node("ai:patrol", { 3.5F, 0.5F }, 4.0F, 5.0F) } };
	input.intake.levelMap = Grid({ "...." });
	input.intake.npcs = { Npc("npc:one") };
	input.intake.order = order;
	input.fallbackPlayerPosition = { 2.0F, 2.0F };
	input.playerCommandConfig = PlayerConfig();
	input.npcConfig = NpcConfig();
	return input;
}

iggy::physics2d::CollisionObject2D Object(iggy::ResourceId id, iggy::Aabb2 bounds)
{
	return { id, iggy::physics2d::makeAabbShape(bounds), true };
}

iggy::physics2d::CollisionWorld2D World(std::vector<iggy::physics2d::CollisionObject2D> objects)
{
	const iggy::physics2d::CollisionWorldBuildResult build = iggy::physics2d::CollisionWorld2DBuilder {}.build(objects);
	Expect(build.built, "collision fixture should build");
	return build.world;
}

iggy::physics2d::CollisionWorld2D BlockingWorld()
{
	return World({
		Object(Id("wall:east"), { { 0.5F, -0.5F }, { 1.5F, 0.5F } }),
	});
}

void SetCollisionCache(iggy::runtime::RuntimeSessionState &session, const iggy::physics2d::CollisionWorld2D &world)
{
	session.derivedCaches.hasCollisionCache = true;
	session.derivedCaches.collision.world = world;
}

bool SameCommand(const iggy::runtime::GameplayCommand2D &actual, const iggy::runtime::GameplayCommand2D &expected)
{
	return actual.type == expected.type
		&& actual.actorId == expected.actorId
		&& NearVec(actual.targetPoint, expected.targetPoint)
		&& actual.targetTile == expected.targetTile
		&& actual.targetId == expected.targetId;
}

bool SameFrame(const iggy::runtime::GameplayCommandFrame2D &actual, const iggy::runtime::GameplayCommandFrame2D &expected)
{
	if (actual.commands.size() != expected.commands.size())
		return false;
	for (std::size_t index = 0; index < actual.commands.size(); ++index) {
		if (!SameCommand(actual.commands[index], expected.commands[index]))
			return false;
	}
	return true;
}

bool SameQueue(const iggy::runtime::RuntimeCommandQueueState &actual, const iggy::runtime::RuntimeCommandQueueState &expected)
{
	if (actual.frames.size() != expected.frames.size())
		return false;
	for (std::size_t index = 0; index < actual.frames.size(); ++index) {
		if (!SameFrame(actual.frames[index], expected.frames[index]))
			return false;
	}
	return true;
}

bool SameGrid(const iggy::LevelTileMap &actual, const iggy::LevelTileMap &expected)
{
	if (actual.id != expected.id
		|| actual.width != expected.width
		|| actual.height != expected.height
		|| actual.tiles.size() != expected.tiles.size()) {
		return false;
	}
	for (std::size_t index = 0; index < actual.tiles.size(); ++index) {
		if (actual.tiles[index].walkable != expected.tiles[index].walkable)
			return false;
	}
	return true;
}

bool SameIntent(const iggy::PlayerInputIntent2D &actual, const iggy::PlayerInputIntent2D &expected)
{
	return actual.type == expected.type
		&& NearVec(actual.worldPoint, expected.worldPoint)
		&& actual.tile == expected.tile
		&& actual.targetId == expected.targetId;
}

bool SameInput(
	const iggy::runtime::RuntimePlayerNpcAiCommandFrameInput &actual,
	const iggy::runtime::RuntimePlayerNpcAiCommandFrameInput &expected)
{
	if (!(SameQueue(actual.intake.queue, expected.intake.queue)
		&& actual.intake.queueConfig.maxFrames == expected.intake.queueConfig.maxFrames
		&& actual.intake.playerActorId == expected.intake.playerActorId
		&& actual.intake.playerInputContext.playerControlEnabled == expected.intake.playerInputContext.playerControlEnabled
		&& actual.intake.playerInputContext.worldInputEnabled == expected.intake.playerInputContext.worldInputEnabled
		&& actual.intake.playerInputContext.interactionEnabled == expected.intake.playerInputContext.interactionEnabled
		&& actual.intake.playerInputContext.cancelEnabled == expected.intake.playerInputContext.cancelEnabled
		&& actual.intake.playerIntents.size() == expected.intake.playerIntents.size()
		&& actual.intake.aiMap.nodes.size() == expected.intake.aiMap.nodes.size()
		&& SameGrid(actual.intake.levelMap, expected.intake.levelMap)
		&& actual.intake.npcs.size() == expected.intake.npcs.size()
		&& actual.intake.order == expected.intake.order
		&& actual.session.tickIndex == expected.session.tickIndex
		&& actual.session.hasPlayer == expected.session.hasPlayer
		&& actual.session.player.id == expected.session.player.id
		&& NearVec(actual.session.player.position, expected.session.player.position)
		&& SameGrid(actual.session.level.map, expected.session.level.map)
		&& NearVec(actual.fallbackPlayerPosition, expected.fallbackPlayerPosition)
		&& actual.playerCommandConfig.movement.maxStep == expected.playerCommandConfig.movement.maxStep
		&& actual.npcConfig.maxDistance == expected.npcConfig.maxDistance)) {
		return false;
	}

	for (std::size_t index = 0; index < actual.intake.playerIntents.size(); ++index) {
		if (!SameIntent(actual.intake.playerIntents[index], expected.intake.playerIntents[index]))
			return false;
	}
	return true;
}

void ExpectPlayerFrame(const iggy::runtime::GameplayCommandFrame2D &frame, const char *message)
{
	Expect(frame.commands.size() == 1, message);
	if (frame.commands.empty())
		return;
	Expect(frame.commands[0].actorId == Id("player:one"), message);
	Expect(frame.commands[0].type == iggy::runtime::GameplayCommand2DType::MoveToPoint, message);
	Expect(NearVec(frame.commands[0].targetPoint, { 9.0F, 0.0F }), message);
}

void ExpectNpcFrame(const iggy::runtime::GameplayCommandFrame2D &frame, const char *message)
{
	Expect(frame.commands.size() == 1, message);
	if (frame.commands.empty())
		return;
	Expect(frame.commands[0].actorId == Id("npc:one"), message);
	Expect(frame.commands[0].type == iggy::runtime::GameplayCommand2DType::MoveToPoint, message);
	Expect(NearVec(frame.commands[0].targetPoint, { 1.5F, 0.5F }), message);
}

void TestPlayerThenNpcAiOrderingDrainsPlayerFrameBeforeNpcFrame()
{
	const iggy::runtime::RuntimePlayerNpcAiCommandFrameInput input =
		BaseInput(iggy::runtime::RuntimePlayerNpcAiQueueOrder::PlayerThenNpcAi);

	const iggy::runtime::RuntimePlayerNpcAiCommandFrameResult result =
		iggy::runtime::RuntimePlayerNpcAiCommandFrameStep {}.run(input);

	Expect(result.status == iggy::runtime::RuntimePlayerNpcAiCommandFrameStatus::Ran, "player-then-NPC frame should run");
	Expect(result.intake.status == iggy::runtime::RuntimePlayerNpcAiQueueStatus::Queued, "player-then-NPC intake should queue");
	Expect(result.runner.drained.frames.size() == 2, "player-then-NPC runner should drain two frames");
	if (result.runner.drained.frames.size() == 2) {
		ExpectPlayerFrame(result.runner.drained.frames[0], "player frame should drain first");
		ExpectNpcFrame(result.runner.drained.frames[1], "NPC frame should drain second");
	}
	Expect(result.runner.runner.ticks.size() == 2, "player-then-NPC runner should execute two ticks");
	if (result.runner.runner.ticks.size() == 2) {
		Expect(NearVec(result.runner.runner.ticks[0].npcTargetPosition, { 1.0F, 0.0F }), "first tick should target post-player position");
		Expect(NearVec(result.runner.runner.ticks[1].npcTargetPosition, { 1.0F, 0.0F }), "second tick should preserve player position after NPC actor mismatch");
	}
	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "player-then-NPC final session should preserve moved player");
	Expect(result.session.tickIndex == input.session.tickIndex + 2, "player-then-NPC should advance two ticks");
	Expect(result.queue.frames.empty(), "successful frame runner should drain queue");
}

void TestNpcAiThenPlayerOrderingDrainsNpcFrameBeforePlayerFrame()
{
	const iggy::runtime::RuntimePlayerNpcAiCommandFrameInput input =
		BaseInput(iggy::runtime::RuntimePlayerNpcAiQueueOrder::NpcAiThenPlayer);

	const iggy::runtime::RuntimePlayerNpcAiCommandFrameResult result =
		iggy::runtime::RuntimePlayerNpcAiCommandFrameStep {}.run(input);

	Expect(result.status == iggy::runtime::RuntimePlayerNpcAiCommandFrameStatus::Ran, "NPC-then-player frame should run");
	Expect(result.runner.drained.frames.size() == 2, "NPC-then-player runner should drain two frames");
	if (result.runner.drained.frames.size() == 2) {
		ExpectNpcFrame(result.runner.drained.frames[0], "NPC frame should drain first");
		ExpectPlayerFrame(result.runner.drained.frames[1], "player frame should drain second");
	}
	Expect(result.runner.runner.ticks.size() == 2, "NPC-then-player runner should execute two ticks");
	if (result.runner.runner.ticks.size() == 2) {
		Expect(NearVec(result.runner.runner.ticks[0].npcTargetPosition, { 0.0F, 0.0F }), "first tick should target original player position");
		Expect(NearVec(result.runner.runner.ticks[1].npcTargetPosition, { 1.0F, 0.0F }), "second tick should target post-player position");
	}
	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "NPC-then-player final session should preserve moved player");
	Expect(result.queue.frames.empty(), "successful NPC-then-player runner should drain queue");
}

void TestPlayerRejectionSkipsQueuedRunner()
{
	iggy::runtime::RuntimePlayerNpcAiCommandFrameInput input =
		BaseInput(iggy::runtime::RuntimePlayerNpcAiQueueOrder::PlayerThenNpcAi);
	input.intake.queue.frames.push_back(ExistingFrame());
	input.intake.queueConfig.maxFrames = 1;
	const iggy::runtime::RuntimeCommandQueueState queueBefore = input.intake.queue;

	const iggy::runtime::RuntimePlayerNpcAiCommandFrameResult result =
		iggy::runtime::RuntimePlayerNpcAiCommandFrameStep {}.run(input);

	Expect(result.status == iggy::runtime::RuntimePlayerNpcAiCommandFrameStatus::PlayerRejected, "player rejection should surface at frame step");
	Expect(result.intake.status == iggy::runtime::RuntimePlayerNpcAiQueueStatus::PlayerRejected, "intake should preserve player rejection");
	Expect(result.runner.drained.frames.empty(), "rejected intake should skip queued runner drain");
	Expect(result.runner.runner.ticks.empty(), "rejected intake should skip queued runner ticks");
	Expect(result.session.tickIndex == input.session.tickIndex, "rejected intake should preserve input session");
	Expect(SameQueue(result.queue, queueBefore), "rejected intake should preserve intake result queue");
}

void TestNpcAiRejectionSkipsQueuedRunnerAndPreservesFirstFrame()
{
	iggy::runtime::RuntimePlayerNpcAiCommandFrameInput input =
		BaseInput(iggy::runtime::RuntimePlayerNpcAiQueueOrder::PlayerThenNpcAi);
	input.intake.queueConfig.maxFrames = 1;

	const iggy::runtime::RuntimePlayerNpcAiCommandFrameResult result =
		iggy::runtime::RuntimePlayerNpcAiCommandFrameStep {}.run(input);

	Expect(result.status == iggy::runtime::RuntimePlayerNpcAiCommandFrameStatus::NpcAiRejected, "NPC AI rejection should surface at frame step");
	Expect(result.intake.status == iggy::runtime::RuntimePlayerNpcAiQueueStatus::NpcAiRejected, "intake should preserve NPC AI rejection");
	Expect(result.runner.drained.frames.empty(), "NPC AI rejected intake should skip queued runner drain");
	Expect(result.runner.runner.ticks.empty(), "NPC AI rejected intake should skip queued runner ticks");
	Expect(result.session.tickIndex == input.session.tickIndex, "NPC AI rejection should preserve input session");
	Expect(result.queue.frames.size() == 1, "NPC AI rejection should preserve first queued player frame");
	ExpectPlayerFrame(result.queue.frames[0], "NPC AI rejection should leave player frame queued");
}

void TestEmptyInputsStillRunQueuedRunnerSemantics()
{
	iggy::runtime::RuntimePlayerNpcAiCommandFrameInput input = BaseInput();
	input.intake.playerIntents.clear();
	input.intake.npcs.clear();

	const iggy::runtime::RuntimePlayerNpcAiCommandFrameResult result =
		iggy::runtime::RuntimePlayerNpcAiCommandFrameStep {}.run(input);

	Expect(result.status == iggy::runtime::RuntimePlayerNpcAiCommandFrameStatus::Ran, "empty intake should still run");
	Expect(result.intake.queue.frames.size() == 2, "empty intake should queue two empty frames");
	Expect(result.runner.drained.frames.size() == 2, "empty intake runner should drain two frames");
	if (result.runner.drained.frames.size() == 2) {
		Expect(result.runner.drained.frames[0].commands.empty(), "first empty frame should drain");
		Expect(result.runner.drained.frames[1].commands.empty(), "second empty frame should drain");
	}
	Expect(result.runner.runner.ticks.size() == 2, "empty frames should still run queued runner ticks");
	Expect(result.session.tickIndex == input.session.tickIndex + 2, "empty frames should preserve runner tick semantics");
	Expect(NearVec(result.session.player.position, input.session.player.position), "empty frames should not move player");
	Expect(result.queue.frames.empty(), "empty frames should be drained");
}

void TestExplicitCollisionWorldOverloadPreservesExplicitPrecedence()
{
	iggy::runtime::RuntimePlayerNpcAiCommandFrameInput input = BaseInput();
	input.intake.npcs.clear();
	SetCollisionCache(input.session, BlockingWorld());
	const iggy::physics2d::CollisionWorld2D emptyWorld;

	const iggy::runtime::RuntimePlayerNpcAiCommandFrameResult implicitResult =
		iggy::runtime::RuntimePlayerNpcAiCommandFrameStep {}.run(input);
	const iggy::runtime::RuntimePlayerNpcAiCommandFrameResult explicitResult =
		iggy::runtime::RuntimePlayerNpcAiCommandFrameStep {}.run(input, emptyWorld);

	Expect(implicitResult.status == iggy::runtime::RuntimePlayerNpcAiCommandFrameStatus::Ran, "implicit collision frame should run");
	Expect(explicitResult.status == iggy::runtime::RuntimePlayerNpcAiCommandFrameStatus::Ran, "explicit collision frame should run");
	Expect(NearVec(implicitResult.session.player.position, { 0.0F, 0.0F }), "session collision cache should block player command");
	Expect(NearVec(explicitResult.session.player.position, { 1.0F, 0.0F }), "explicit empty world should override blocking session cache");
}

void TestInputsAreNotMutated()
{
	iggy::runtime::RuntimePlayerNpcAiCommandFrameInput input =
		BaseInput(iggy::runtime::RuntimePlayerNpcAiQueueOrder::NpcAiThenPlayer);
	input.intake.queue.frames.push_back(ExistingFrame());
	input.intake.queueConfig.maxFrames = 4;
	input.playerCommandConfig.movement.maxStep = 0.5F;
	input.npcConfig.maxDistance = 0.5F;
	const iggy::runtime::RuntimePlayerNpcAiCommandFrameInput inputBefore = input;

	const iggy::runtime::RuntimePlayerNpcAiCommandFrameResult result =
		iggy::runtime::RuntimePlayerNpcAiCommandFrameStep {}.run(input);

	Expect(result.status == iggy::runtime::RuntimePlayerNpcAiCommandFrameStatus::Ran, "immutability setup should run");
	Expect(SameInput(input, inputBefore), "command frame step should not mutate input");
}

} // namespace

int main()
{
	TestPlayerThenNpcAiOrderingDrainsPlayerFrameBeforeNpcFrame();
	TestNpcAiThenPlayerOrderingDrainsNpcFrameBeforePlayerFrame();
	TestPlayerRejectionSkipsQueuedRunner();
	TestNpcAiRejectionSkipsQueuedRunnerAndPreservesFirstFrame();
	TestEmptyInputsStillRunQueuedRunnerSemantics();
	TestExplicitCollisionWorldOverloadPreservesExplicitPrecedence();
	TestInputsAreNotMutated();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
