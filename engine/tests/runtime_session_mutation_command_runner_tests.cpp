#include <cstdlib>
#include <string_view>
#include <vector>

#include "runtime/RuntimeSessionMutationCommandRunner.hpp"
#include "servers/physics2d/CollisionShape2D.hpp"
#include "support/CommandFrameFixtures.hpp"
#include "support/GeometryAssertions.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::CommandFrame;
using iggy::test::Expect;
using iggy::test::ExpectBounds;
using iggy::test::ExpectPlayerAgent;
using iggy::test::Failures;
using iggy::test::MapFromRows;
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

const iggy::ResourceId PlayerId { "player:one" };
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
	Expect(build.built, "mutation command runner fixture should build derived caches");
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
	session.tickIndex = 10;
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

iggy::runtime::RuntimeSessionMutationCommandRunnerInput Input(
	iggy::runtime::RuntimeSessionState session,
	std::vector<iggy::runtime::RuntimeSessionMutationCommandFrame> frames)
{
	return {
		session,
		frames,
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
	Expect(build.built, "mutation command runner fixture collision world should build");
	return build.world;
}

bool PlayerBlocked(const iggy::runtime::RuntimeSessionMutationCommandResult &result)
{
	return !result.commandTick.playerCommands.execution.movementResults.empty()
		&& result.commandTick.playerCommands.execution.movementResults[0].status == iggy::PlayerMovementExecutionStatus::Blocked;
}

void ExpectMapSame(const iggy::LevelTileMap &actual, const iggy::LevelTileMap &expected, const char *message)
{
	Expect(actual.id == expected.id, message);
	Expect(actual.width == expected.width && actual.height == expected.height, message);
	Expect(actual.tiles.size() == expected.tiles.size(), message);
	for (std::size_t index = 0; index < actual.tiles.size() && index < expected.tiles.size(); ++index)
		Expect(actual.tiles[index].walkable == expected.tiles[index].walkable, message);
}

void ExpectSessionSame(const iggy::runtime::RuntimeSessionState &actual, const iggy::runtime::RuntimeSessionState &expected, const char *message)
{
	ExpectMapSame(actual.level.map, expected.level.map, message);
	Expect(actual.tickIndex == expected.tickIndex, message);
	Expect(actual.hasPlayer == expected.hasPlayer, message);
	ExpectPlayerAgent(actual.player, expected.player, message);
	Expect(actual.hasRenderCache == expected.hasRenderCache, message);
	Expect(actual.derivedCaches.hasRenderCache == expected.derivedCaches.hasRenderCache, message);
	Expect(actual.derivedCaches.hasCollisionCache == expected.derivedCaches.hasCollisionCache, message);
	Expect(actual.derivedCaches.collision.world.objects().size() == expected.derivedCaches.collision.world.objects().size(), message);
}

void ExpectFramesSame(
	const std::vector<iggy::runtime::RuntimeSessionMutationCommandFrame> &actual,
	const std::vector<iggy::runtime::RuntimeSessionMutationCommandFrame> &expected,
	const char *message)
{
	Expect(actual.size() == expected.size(), message);
	for (std::size_t frameIndex = 0; frameIndex < actual.size() && frameIndex < expected.size(); ++frameIndex) {
		Expect(actual[frameIndex].levelEdits.size() == expected[frameIndex].levelEdits.size(), message);
		for (std::size_t editIndex = 0; editIndex < actual[frameIndex].levelEdits.size() && editIndex < expected[frameIndex].levelEdits.size(); ++editIndex) {
			Expect(actual[frameIndex].levelEdits[editIndex].tile == expected[frameIndex].levelEdits[editIndex].tile, message);
			Expect(actual[frameIndex].levelEdits[editIndex].walkable == expected[frameIndex].levelEdits[editIndex].walkable, message);
		}
		Expect(actual[frameIndex].commandFrame.commands.size() == expected[frameIndex].commandFrame.commands.size(), message);
	}
}

void TestEmptyFramesReturnInitialSession()
{
	const iggy::LevelRuntimeState level = Level({ "...." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level));
	const iggy::runtime::RuntimeSessionMutationCommandRunnerInput input = Input(session, {});

	const iggy::runtime::RuntimeSessionMutationCommandRunnerResult result = iggy::runtime::RuntimeSessionMutationCommandRunner {}.run(input);

	Expect(result.ticks.empty(), "empty mutation command runner should produce no ticks");
	ExpectSessionSame(result.session, session, "empty mutation command runner should return initial session unchanged");
}

void TestOneFrameMatchesStepBehavior()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::LevelRuntimeState level = Level({ "...." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level));
	const iggy::runtime::RuntimeSessionMutationCommandFrame frame = Frame(
		{ Edit({ 2, 0 }, false) },
		CommandFrame({ factory.moveToPoint(PlayerId, { 4.0F, 0.0F }) }));
	iggy::runtime::RuntimeSessionMutationCommandRunnerInput input = Input(session, { frame });

	const iggy::runtime::RuntimeSessionMutationCommandRunnerResult runner = iggy::runtime::RuntimeSessionMutationCommandRunner {}.run(input);
	const iggy::runtime::RuntimeSessionMutationCommandResult step = iggy::runtime::RuntimeSessionMutationCommandStep {}.run({
		session,
		frame.levelEdits,
		frame.commandFrame,
		input.fallbackPlayerPosition,
		input.playerCommandConfig,
		input.npcConfig,
	});

	Expect(runner.ticks.size() == 1, "one-frame mutation command runner should produce one tick");
	if (runner.ticks.size() == 1)
		Expect(runner.ticks[0].status == step.status, "one-frame runner should match step status");
	Expect(runner.session.tickIndex == step.session.tickIndex, "one-frame runner final tick index should match step");
	Expect(NearVec(runner.session.player.position, step.session.player.position), "one-frame runner final player position should match step");
}

void TestMultipleFramesCarrySessionForward()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::LevelRuntimeState level = Level({ "......." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level));
	const iggy::runtime::RuntimeSessionMutationCommandRunnerInput input = Input(session, {
		Frame({}, CommandFrame({ factory.moveToPoint(PlayerId, { 2.0F, 0.0F }) })),
		Frame({}, CommandFrame({ factory.moveToPoint(PlayerId, { 4.0F, 0.0F }) })),
	});

	const iggy::runtime::RuntimeSessionMutationCommandRunnerResult result = iggy::runtime::RuntimeSessionMutationCommandRunner {}.run(input);

	Expect(result.ticks.size() == 2, "multiple-frame runner should preserve per-tick results");
	Expect(result.session.tickIndex == session.tickIndex + 2, "multiple-frame runner should advance tick index across ticks");
	Expect(NearVec(result.session.player.position, { 4.0F, 0.0F }), "multiple-frame runner should carry player position forward");
}

void TestNoExplicitUsesPriorMutationCollisionCacheInLaterFrame()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::LevelRuntimeState level = Level({ "...." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level));
	const iggy::runtime::RuntimeSessionMutationCommandRunnerInput input = Input(session, {
		Frame({ Edit({ 2, 0 }, false) }, {}),
		Frame({}, CommandFrame({ factory.moveToPoint(PlayerId, { 4.0F, 0.0F }) })),
	});

	const iggy::runtime::RuntimeSessionMutationCommandRunnerResult result = iggy::runtime::RuntimeSessionMutationCommandRunner {}.run(input);

	Expect(result.ticks.size() == 2, "prior mutation collision cache setup should produce two ticks");
	if (result.ticks.size() == 2) {
		Expect(result.ticks[0].session.derivedCaches.collision.world.objects().size() == 1, "first tick should publish refreshed collision cache");
		Expect(PlayerBlocked(result.ticks[1]), "second tick should use carried collision cache and block movement");
	}
	Expect(result.session.player.position.x < 4.0F, "carried collision cache should prevent full movement");
}

void TestExplicitWorldPrecedenceAcrossFrames()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::LevelRuntimeState level = Level({ "...." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level));
	const iggy::runtime::RuntimeSessionMutationCommandRunnerInput input = Input(session, {
		Frame({ Edit({ 2, 0 }, false) }, {}),
		Frame({}, CommandFrame({ factory.moveToPoint(PlayerId, { 4.0F, 0.0F }) })),
	});
	const iggy::physics2d::CollisionWorld2D explicitWorld = World({});

	const iggy::runtime::RuntimeSessionMutationCommandRunnerResult result = iggy::runtime::RuntimeSessionMutationCommandRunner {}.run(input, explicitWorld);

	Expect(result.ticks.size() == 2, "explicit-world runner should produce two ticks");
	if (result.ticks.size() == 2)
		Expect(!PlayerBlocked(result.ticks[1]), "explicit empty world should override carried collision cache");
	Expect(NearVec(result.session.player.position, { 4.0F, 0.0F }), "explicit empty world should allow full movement across frames");
}

void TestMutationFailureCarriesUnchangedSessionAndLaterFramesRun()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::LevelRuntimeState level = Level({ "...." });
	iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level, false, true));
	session.derivedCaches.hasRenderCache = true;
	session.derivedCaches.render.tileChunkConfig = RenderConfig(0, 1, 7);
	session.hasRenderCache = true;
	session.renderCache = session.derivedCaches.render;
	const iggy::runtime::RuntimeSessionMutationCommandRunnerInput input = Input(session, {
		Frame({ Edit({ 2, 0 }, false) }, {}),
		Frame({}, CommandFrame({ factory.moveToPoint(PlayerId, { 1.0F, 0.0F }) })),
	});
	const iggy::physics2d::CollisionWorld2D explicitWorld = World({});

	const iggy::runtime::RuntimeSessionMutationCommandRunnerResult result = iggy::runtime::RuntimeSessionMutationCommandRunner {}.run(input, explicitWorld);

	Expect(result.ticks.size() == 2, "mutation failure runner should preserve failed and later ticks");
	if (result.ticks.size() == 2) {
		Expect(result.ticks[0].status == iggy::runtime::RuntimeSessionMutationCommandStatus::MutationFailed, "first tick should record mutation failure");
		Expect(result.ticks[0].session.tickIndex == session.tickIndex, "failed tick should carry unchanged session");
		Expect(result.ticks[1].status == iggy::runtime::RuntimeSessionMutationCommandStatus::Ticked, "later tick should still run after failure");
	}
	Expect(result.session.tickIndex == session.tickIndex + 1, "later successful tick should advance from unchanged failed session");
	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "later successful tick should update player from carried session");
}

void TestInvalidCommandStillRecordsDiagnosticsAndAdvancesTick()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::LevelRuntimeState level = Level({ "...." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level));
	const iggy::runtime::RuntimeSessionMutationCommandRunnerInput input = Input(session, {
		Frame({ Edit({ 1, 0 }, false) }, CommandFrame({ factory.interact(PlayerId, {}) })),
	});

	const iggy::runtime::RuntimeSessionMutationCommandRunnerResult result = iggy::runtime::RuntimeSessionMutationCommandRunner {}.run(input);

	Expect(result.ticks.size() == 1, "invalid command runner should record one tick");
	if (result.ticks.size() == 1) {
		Expect(result.ticks[0].commandTick.playerCommands.planning.playerPlan.validation.hasInvalidCommands, "invalid command runner should preserve command diagnostics");
		Expect(result.ticks[0].session.tickIndex == session.tickIndex + 1, "invalid command runner should still tick");
	}
}

void TestMissingPlayerUsesFallbackEachFrame()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::LevelRuntimeState level = Level({ "...." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level), false);
	const iggy::runtime::RuntimeSessionMutationCommandRunnerInput input = Input(session, {
		Frame({}, CommandFrame({ factory.moveToPoint({}, { 1.0F, 0.0F }) })),
		Frame({}, CommandFrame({ factory.moveToPoint({}, { 2.0F, 0.0F }) })),
	});

	const iggy::runtime::RuntimeSessionMutationCommandRunnerResult result = iggy::runtime::RuntimeSessionMutationCommandRunner {}.run(input);

	Expect(result.ticks.size() == 2, "missing-player runner should preserve each tick");
	if (result.ticks.size() == 2) {
		Expect(result.ticks[0].commandTick.playerCommands.planning.status == iggy::runtime::RuntimePlayerCommandPlanningStatus::MissingPlayer, "first missing-player tick should preserve diagnostics");
		Expect(result.ticks[1].commandTick.playerCommands.planning.status == iggy::runtime::RuntimePlayerCommandPlanningStatus::MissingPlayer, "second missing-player tick should preserve diagnostics");
	}
	Expect(!result.session.hasPlayer, "missing-player runner should not invent a player");
	Expect(result.session.tickIndex == session.tickIndex + 2, "missing-player runner should still advance ticks");
}

void TestInputsAreNotMutated()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::LevelRuntimeState level = Level({ "...." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level));
	const iggy::runtime::RuntimeSessionState sessionBefore = session;
	std::vector<iggy::runtime::RuntimeSessionMutationCommandFrame> frames {
		Frame({ Edit({ 1, 0 }, false) }, CommandFrame({ factory.moveToPoint(PlayerId, { 4.0F, 0.0F }) })),
	};
	const std::vector<iggy::runtime::RuntimeSessionMutationCommandFrame> framesBefore = frames;
	iggy::runtime::RuntimeSessionMutationCommandRunnerInput input = Input(session, frames);
	const iggy::runtime::RuntimePlayerCommandExecutionConfig configBefore = input.playerCommandConfig;
	const iggy::physics2d::CollisionWorld2D explicitWorld = World({
		Object(iggy::ResourceId("wall:explicit"), { { 3.0F, -0.5F }, { 4.0F, 0.5F } }),
	});
	const std::vector<iggy::physics2d::CollisionObject2D> worldBefore = explicitWorld.objects();

	const iggy::runtime::RuntimeSessionMutationCommandRunnerResult result = iggy::runtime::RuntimeSessionMutationCommandRunner {}.run(input, explicitWorld);

	Expect(result.ticks.size() == 1, "runner immutability setup should tick");
	ExpectSessionSame(session, sessionBefore, "mutation command runner should not mutate input session object");
	ExpectFramesSame(frames, framesBefore, "mutation command runner should not mutate caller frame vector");
	ExpectFramesSame(input.frames, framesBefore, "mutation command runner should not mutate input frame vector");
	Expect(input.playerCommandConfig.movement.maxStep == configBefore.movement.maxStep, "mutation command runner should not mutate config");
	Expect(explicitWorld.objects().size() == worldBefore.size(), "mutation command runner should not mutate explicit world object count");
	if (explicitWorld.objects().size() == worldBefore.size()) {
		for (std::size_t index = 0; index < worldBefore.size(); ++index)
			ExpectBounds(explicitWorld.objects()[index].shape.bounds, worldBefore[index].shape.bounds, "mutation command runner should not mutate explicit world bounds");
	}
}

} // namespace

int main()
{
	TestEmptyFramesReturnInitialSession();
	TestOneFrameMatchesStepBehavior();
	TestMultipleFramesCarrySessionForward();
	TestNoExplicitUsesPriorMutationCollisionCacheInLaterFrame();
	TestExplicitWorldPrecedenceAcrossFrames();
	TestMutationFailureCarriesUnchangedSessionAndLaterFramesRun();
	TestInvalidCommandStillRecordsDiagnosticsAndAdvancesTick();
	TestMissingPlayerUsesFallbackEachFrame();
	TestInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
