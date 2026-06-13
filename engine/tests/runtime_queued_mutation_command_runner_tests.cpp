#include <cstdlib>
#include <string_view>
#include <vector>

#include "runtime/RuntimeQueuedMutationCommandRunner.hpp"
#include "servers/physics2d/CollisionShape2D.hpp"
#include "support/CommandFrameFixtures.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::CommandFrame;
using iggy::test::Expect;
using iggy::test::ExpectPlayerAgent;
using iggy::test::Failures;
using iggy::test::MapFromRows;
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

const iggy::ResourceId PlayerId { "player:queued-mutation" };
const iggy::ResourceId WalkableMaterial { "material:floor" };
const iggy::ResourceId BlockedMaterial { "material:wall" };

iggy::LevelTileEdit Edit(iggy::TileCoord tile, bool walkable)
{
	return { tile, walkable };
}

iggy::LevelRuntimeState Level(std::initializer_list<std::string_view> rows)
{
	iggy::LevelRuntimeState level;
	level.map = MapFromRows(std::vector<std::string_view>(rows));
	level.map.id = iggy::ResourceId("level:queued-mutation");
	return level;
}

iggy::LevelTileRenderChunkCacheConfig RenderConfig(int chunkWidth = 2, int chunkHeight = 1, int layer = 4)
{
	return { chunkWidth, chunkHeight, { { WalkableMaterial, BlockedMaterial }, layer } };
}

iggy::LevelDerivedCacheState BuildCaches(
	const iggy::LevelRuntimeState &level,
	bool render = false,
	bool collision = true,
	iggy::LevelTileRenderChunkCacheConfig renderConfig = RenderConfig())
{
	iggy::LevelDerivedCacheBuildConfig config;
	config.buildRenderCache = render;
	config.renderCacheConfig = renderConfig;
	config.buildCollisionCache = collision;
	const iggy::LevelDerivedCacheBuildResult build = iggy::LevelDerivedCacheBuilder {}.build(level, config);
	Expect(build.built, "queued mutation command runner fixture should build derived caches");
	return build.state;
}

iggy::runtime::RuntimeSessionState Session(
	const iggy::LevelRuntimeState &level,
	const iggy::LevelDerivedCacheState &caches = {},
	bool hasPlayer = true)
{
	iggy::runtime::RuntimeSessionState session;
	session.level = level;
	session.derivedCaches = caches;
	session.tickIndex = 8;
	session.hasPlayer = hasPlayer;
	if (hasPlayer)
		session.player = PlayerAgent(PlayerId, { 0.0F, 0.0F }, { 0, 0 }, iggy::PlayerMovementStatus::Idle, iggy::PlayerFacing2D::East);
	session.hasRenderCache = caches.hasRenderCache;
	if (caches.hasRenderCache)
		session.renderCache = caches.render;
	return session;
}

iggy::runtime::RuntimePlayerCommandExecutionConfig PlayerCommandConfig(float maxStep = 4.0F)
{
	iggy::runtime::RuntimePlayerCommandExecutionConfig config;
	config.movement.maxStep = maxStep;
	return config;
}

iggy::runtime::RuntimeSessionMutationCommandFrame Frame(
	std::vector<iggy::LevelTileEdit> edits,
	iggy::runtime::GameplayCommandFrame2D commandFrame)
{
	return { edits, commandFrame };
}

iggy::runtime::RuntimeMutationCommandQueueState Queue(std::vector<iggy::runtime::RuntimeSessionMutationCommandFrame> frames)
{
	return { frames };
}

iggy::runtime::RuntimeQueuedMutationCommandRunnerInput Input(
	iggy::runtime::RuntimeSessionState session,
	iggy::runtime::RuntimeMutationCommandQueueState queue)
{
	return {
		session,
		queue,
		{ 4.0F, 4.0F },
		PlayerCommandConfig(),
		{},
	};
}

iggy::physics2d::CollisionObject2D Object(iggy::ResourceId id, iggy::Aabb2 bounds)
{
	return { id, iggy::physics2d::makeAabbShape(bounds), true };
}

iggy::physics2d::CollisionWorld2D World(std::vector<iggy::physics2d::CollisionObject2D> objects)
{
	const iggy::physics2d::CollisionWorldBuildResult build = iggy::physics2d::CollisionWorld2DBuilder {}.build(objects);
	Expect(build.built, "queued mutation command runner collision fixture should build");
	return build.world;
}

bool PlayerBlocked(const iggy::runtime::RuntimeSessionMutationCommandResult &result)
{
	return !result.commandTick.playerCommands.execution.movementResults.empty()
		&& result.commandTick.playerCommands.execution.movementResults[0].status == iggy::PlayerMovementExecutionStatus::Blocked;
}

bool SameEdit(const iggy::LevelTileEdit &actual, const iggy::LevelTileEdit &expected)
{
	return actual.tile == expected.tile && actual.walkable == expected.walkable;
}

bool SameCommand(const iggy::runtime::GameplayCommand2D &actual, const iggy::runtime::GameplayCommand2D &expected)
{
	return actual.type == expected.type
		&& actual.actorId == expected.actorId
		&& NearVec(actual.targetPoint, expected.targetPoint)
		&& actual.targetTile == expected.targetTile
		&& actual.targetId == expected.targetId;
}

bool SameCommandFrame(const iggy::runtime::GameplayCommandFrame2D &actual, const iggy::runtime::GameplayCommandFrame2D &expected)
{
	if (actual.commands.size() != expected.commands.size())
		return false;
	for (std::size_t index = 0; index < actual.commands.size(); ++index) {
		if (!SameCommand(actual.commands[index], expected.commands[index]))
			return false;
	}
	return true;
}

bool SameFrame(
	const iggy::runtime::RuntimeSessionMutationCommandFrame &actual,
	const iggy::runtime::RuntimeSessionMutationCommandFrame &expected)
{
	if (actual.levelEdits.size() != expected.levelEdits.size())
		return false;
	for (std::size_t index = 0; index < actual.levelEdits.size(); ++index) {
		if (!SameEdit(actual.levelEdits[index], expected.levelEdits[index]))
			return false;
	}
	return SameCommandFrame(actual.commandFrame, expected.commandFrame);
}

bool SameQueue(
	const iggy::runtime::RuntimeMutationCommandQueueState &actual,
	const iggy::runtime::RuntimeMutationCommandQueueState &expected)
{
	if (actual.frames.size() != expected.frames.size())
		return false;
	for (std::size_t index = 0; index < actual.frames.size(); ++index) {
		if (!SameFrame(actual.frames[index], expected.frames[index]))
			return false;
	}
	return true;
}

void ExpectMapSame(const iggy::LevelTileMap &actual, const iggy::LevelTileMap &expected, const char *message)
{
	Expect(actual.id == expected.id, message);
	Expect(actual.width == expected.width && actual.height == expected.height, message);
	Expect(actual.tiles.size() == expected.tiles.size(), message);
	for (std::size_t index = 0; index < actual.tiles.size() && index < expected.tiles.size(); ++index)
		Expect(actual.tiles[index].walkable == expected.tiles[index].walkable, message);
}

void TestEmptyQueueReturnsUnchangedSessionAndEmptyQueue()
{
	const iggy::LevelRuntimeState level = Level({ "...." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level));

	const iggy::runtime::RuntimeQueuedMutationCommandRunnerResult result = iggy::runtime::RuntimeQueuedMutationCommandRunner {}.run(Input(session, {}));

	Expect(result.drained.frames.empty(), "empty queued mutation runner should drain no frames");
	Expect(result.queue.frames.empty(), "empty queued mutation runner should return empty queue");
	Expect(result.runner.ticks.empty(), "empty queued mutation runner should produce no runner ticks");
	Expect(result.session.tickIndex == session.tickIndex, "empty queued mutation runner should preserve tickIndex");
	ExpectMapSame(result.session.level.map, session.level.map, "empty queued mutation runner should preserve map");
	ExpectPlayerAgent(result.session.player, session.player, "empty queued mutation runner result");
}

void TestOneQueuedFrameRunsOneMutationCommandStepAndEmptiesQueue()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::LevelRuntimeState level = Level({ "...." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level));
	const iggy::runtime::RuntimeSessionMutationCommandFrame frame = Frame(
		{ Edit({ 2, 0 }, false) },
		CommandFrame({ factory.moveToPoint(PlayerId, { 4.0F, 0.0F }) }));

	const iggy::runtime::RuntimeQueuedMutationCommandRunnerResult result = iggy::runtime::RuntimeQueuedMutationCommandRunner {}.run(Input(session, Queue({ frame })));

	Expect(result.drained.frames.size() == 1 && SameFrame(result.drained.frames[0], frame), "queued mutation runner should drain one frame");
	Expect(result.queue.frames.empty(), "queued mutation runner should empty queue after drain");
	Expect(result.runner.ticks.size() == 1, "one queued mutation frame should run one tick");
	Expect(result.session.tickIndex == session.tickIndex + 1, "one queued mutation frame should advance tickIndex once");
	Expect(result.session.level.map.tiles[2].walkable == false, "one queued mutation frame should apply tile edit through runner");
}

void TestMultipleQueuedFramesPreserveFifoOrder()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::LevelRuntimeState level = Level({ "......." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level));
	const iggy::runtime::RuntimeSessionMutationCommandFrame first = Frame({}, CommandFrame({ factory.moveToPoint(PlayerId, { 2.0F, 0.0F }) }));
	const iggy::runtime::RuntimeSessionMutationCommandFrame second = Frame({}, CommandFrame({ factory.moveToPoint(PlayerId, { 4.0F, 0.0F }) }));

	const iggy::runtime::RuntimeQueuedMutationCommandRunnerResult result = iggy::runtime::RuntimeQueuedMutationCommandRunner {}.run(Input(session, Queue({ first, second })));

	Expect(result.drained.frames.size() == 2, "queued mutation runner should drain all frames");
	if (result.drained.frames.size() == 2) {
		Expect(SameFrame(result.drained.frames[0], first), "queued mutation runner should preserve first drained frame");
		Expect(SameFrame(result.drained.frames[1], second), "queued mutation runner should preserve second drained frame");
	}
	Expect(result.runner.ticks.size() == 2, "two queued mutation frames should run two ticks");
	if (result.runner.ticks.size() == 2) {
		Expect(NearVec(result.runner.ticks[0].session.player.position, { 2.0F, 0.0F }), "first queued mutation frame should execute first");
		Expect(NearVec(result.runner.ticks[1].session.player.position, { 4.0F, 0.0F }), "second queued mutation frame should execute after first");
	}
	Expect(result.session.tickIndex == session.tickIndex + 2, "two queued mutation frames should advance tickIndex twice");
	Expect(NearVec(result.session.player.position, { 4.0F, 0.0F }), "queued mutation runner final session should be last runner session");
}

void TestInputQueueIsNotMutated()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::LevelRuntimeState level = Level({ "...." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level));
	iggy::runtime::RuntimeMutationCommandQueueState queue = Queue({
		Frame({ Edit({ 1, 0 }, false) }, CommandFrame({ factory.moveToPoint(PlayerId, { 2.0F, 0.0F }) })),
	});
	const iggy::runtime::RuntimeMutationCommandQueueState queueBefore = queue;

	const iggy::runtime::RuntimeQueuedMutationCommandRunnerResult result = iggy::runtime::RuntimeQueuedMutationCommandRunner {}.run(Input(session, queue));

	Expect(result.runner.ticks.size() == 1, "queued mutation runner immutability setup should run one tick");
	Expect(SameQueue(queue, queueBefore), "queued mutation runner should not mutate input queue");
}

void TestNoExplicitOverloadUsesRefreshedCollisionCacheAcrossFrames()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::LevelRuntimeState level = Level({ "...." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level));
	const iggy::runtime::RuntimeMutationCommandQueueState queue = Queue({
		Frame({ Edit({ 2, 0 }, false) }, {}),
		Frame({}, CommandFrame({ factory.moveToPoint(PlayerId, { 4.0F, 0.0F }) })),
	});

	const iggy::runtime::RuntimeQueuedMutationCommandRunnerResult result = iggy::runtime::RuntimeQueuedMutationCommandRunner {}.run(Input(session, queue));

	Expect(result.runner.ticks.size() == 2, "refreshed collision cache test should produce two ticks");
	if (result.runner.ticks.size() == 2) {
		Expect(result.runner.ticks[0].session.derivedCaches.collision.world.objects().size() == 1, "first tick should refresh collision cache");
		Expect(PlayerBlocked(result.runner.ticks[1]), "second tick should use refreshed collision cache and block movement");
	}
	Expect(result.session.player.position.x < 4.0F, "refreshed collision cache should prevent full movement");
}

void TestExplicitWorldOverloadPreservesExplicitPrecedenceAcrossFrames()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::LevelRuntimeState level = Level({ "...." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level));
	const iggy::runtime::RuntimeMutationCommandQueueState queue = Queue({
		Frame({ Edit({ 2, 0 }, false) }, {}),
		Frame({}, CommandFrame({ factory.moveToPoint(PlayerId, { 4.0F, 0.0F }) })),
	});
	const iggy::physics2d::CollisionWorld2D explicitWorld = World({});

	const iggy::runtime::RuntimeQueuedMutationCommandRunnerResult result = iggy::runtime::RuntimeQueuedMutationCommandRunner {}.run(Input(session, queue), explicitWorld);

	Expect(result.runner.ticks.size() == 2, "explicit world queued mutation runner should produce two ticks");
	if (result.runner.ticks.size() == 2)
		Expect(!PlayerBlocked(result.runner.ticks[1]), "explicit empty world should override refreshed session collision cache");
	Expect(NearVec(result.session.player.position, { 4.0F, 0.0F }), "explicit empty world should allow full movement across frames");
}

void TestInvalidCommandFramePreservesDiagnosticsThroughRunner()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::LevelRuntimeState level = Level({ "...." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level));
	const iggy::runtime::RuntimeMutationCommandQueueState queue = Queue({
		Frame({ Edit({ 1, 0 }, false) }, CommandFrame({ factory.interact(PlayerId, {}) })),
	});

	const iggy::runtime::RuntimeQueuedMutationCommandRunnerResult result = iggy::runtime::RuntimeQueuedMutationCommandRunner {}.run(Input(session, queue));

	Expect(result.runner.ticks.size() == 1, "invalid command queued mutation frame should still run one tick");
	if (result.runner.ticks.size() == 1) {
		Expect(result.runner.ticks[0].commandTick.playerCommands.planning.playerPlan.validation.hasInvalidCommands, "invalid queued mutation command should preserve validation diagnostics");
		Expect(result.runner.ticks[0].session.tickIndex == session.tickIndex + 1, "invalid queued mutation command should still tick");
	}
}

void TestMutationFailurePreservesRunnerBehaviorAndLaterFramesRun()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::LevelRuntimeState level = Level({ "...." });
	iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level, false, true));
	session.derivedCaches.hasRenderCache = true;
	session.derivedCaches.render.tileChunkConfig = RenderConfig(0, 1, 7);
	session.hasRenderCache = true;
	session.renderCache = session.derivedCaches.render;
	const iggy::runtime::RuntimeMutationCommandQueueState queue = Queue({
		Frame({ Edit({ 2, 0 }, false) }, {}),
		Frame({}, CommandFrame({ factory.moveToPoint(PlayerId, { 1.0F, 0.0F }) })),
	});
	const iggy::physics2d::CollisionWorld2D explicitWorld = World({});

	const iggy::runtime::RuntimeQueuedMutationCommandRunnerResult result = iggy::runtime::RuntimeQueuedMutationCommandRunner {}.run(Input(session, queue), explicitWorld);

	Expect(result.runner.ticks.size() == 2, "mutation failure queued runner should preserve failed and later ticks");
	if (result.runner.ticks.size() == 2) {
		Expect(result.runner.ticks[0].status == iggy::runtime::RuntimeSessionMutationCommandStatus::MutationFailed, "first queued tick should record mutation failure");
		Expect(result.runner.ticks[0].session.tickIndex == session.tickIndex, "failed queued tick should carry unchanged session");
		Expect(result.runner.ticks[1].status == iggy::runtime::RuntimeSessionMutationCommandStatus::Ticked, "later queued tick should still run after failure");
	}
	Expect(result.session.tickIndex == session.tickIndex + 1, "later successful queued tick should advance from unchanged failed session");
	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "later successful queued tick should update player from carried session");
}

void TestMissingPlayerUsesFallbackThroughRunner()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::LevelRuntimeState level = Level({ "...." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level), false);
	const iggy::runtime::RuntimeMutationCommandQueueState queue = Queue({
		Frame({}, CommandFrame({ factory.moveToPoint({}, { 1.0F, 0.0F }) })),
	});

	const iggy::runtime::RuntimeQueuedMutationCommandRunnerResult result = iggy::runtime::RuntimeQueuedMutationCommandRunner {}.run(Input(session, queue));

	Expect(result.runner.ticks.size() == 1, "missing player queued mutation frame should still tick");
	if (result.runner.ticks.size() == 1) {
		Expect(result.runner.ticks[0].commandTick.playerCommands.planning.status == iggy::runtime::RuntimePlayerCommandPlanningStatus::MissingPlayer, "missing player should preserve planning diagnostics");
		Expect(result.runner.ticks[0].commandTick.playerCommands.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::MissingPlayer, "missing player should preserve execution diagnostics");
		Expect(NearVec(result.runner.ticks[0].commandTick.npcTargetPosition, { 4.0F, 4.0F }), "missing player should use fallback NPC target");
	}
	Expect(!result.session.hasPlayer, "missing player queued mutation runner should not invent player");
	Expect(result.session.tickIndex == session.tickIndex + 1, "missing player queued mutation frame should still advance session tick");
}

} // namespace

int main()
{
	TestEmptyQueueReturnsUnchangedSessionAndEmptyQueue();
	TestOneQueuedFrameRunsOneMutationCommandStepAndEmptiesQueue();
	TestMultipleQueuedFramesPreserveFifoOrder();
	TestInputQueueIsNotMutated();
	TestNoExplicitOverloadUsesRefreshedCollisionCacheAcrossFrames();
	TestExplicitWorldOverloadPreservesExplicitPrecedenceAcrossFrames();
	TestInvalidCommandFramePreservesDiagnosticsThroughRunner();
	TestMutationFailurePreservesRunnerBehaviorAndLaterFramesRun();
	TestMissingPlayerUsesFallbackThroughRunner();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
