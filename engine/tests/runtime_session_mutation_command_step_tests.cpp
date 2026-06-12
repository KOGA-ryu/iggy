#include <cstdlib>
#include <string_view>
#include <vector>

#include "runtime/RuntimeSessionMutationCommandStep.hpp"
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
	Expect(build.built, "mutation command fixture should build derived caches");
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

iggy::runtime::RuntimeSessionMutationCommandInput Input(
	iggy::runtime::RuntimeSessionState session,
	std::vector<iggy::LevelTileEdit> edits,
	iggy::runtime::GameplayCommandFrame2D frame)
{
	return {
		session,
		edits,
		frame,
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
	Expect(build.built, "mutation command fixture collision world should build");
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

void ExpectEditsSame(
	const std::vector<iggy::LevelTileEdit> &actual,
	const std::vector<iggy::LevelTileEdit> &expected,
	const char *message)
{
	Expect(actual.size() == expected.size(), message);
	for (std::size_t index = 0; index < actual.size() && index < expected.size(); ++index) {
		Expect(actual[index].tile == expected[index].tile, message);
		Expect(actual[index].walkable == expected[index].walkable, message);
	}
}

void ExpectFrameSame(
	const iggy::runtime::GameplayCommandFrame2D &actual,
	const iggy::runtime::GameplayCommandFrame2D &expected,
	const char *message)
{
	Expect(actual.commands.size() == expected.commands.size(), message);
	for (std::size_t index = 0; index < actual.commands.size() && index < expected.commands.size(); ++index) {
		Expect(actual.commands[index].type == expected.commands[index].type, message);
		Expect(actual.commands[index].actorId == expected.commands[index].actorId, message);
		Expect(actual.commands[index].targetId == expected.commands[index].targetId, message);
	}
}

void TestEmptyEditsAndEmptyCommandFrameTicksOnce()
{
	const iggy::LevelRuntimeState level = Level({ "...." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level));
	const iggy::runtime::RuntimeSessionMutationCommandInput input = Input(session, {}, {});

	const iggy::runtime::RuntimeSessionMutationCommandResult result = iggy::runtime::RuntimeSessionMutationCommandStep {}.run(input);

	Expect(result.status == iggy::runtime::RuntimeSessionMutationCommandStatus::Ticked, "empty mutation command step should still tick");
	Expect(result.mutation.status == iggy::runtime::RuntimeLevelMutationStatus::NoMutation, "empty mutation command step should preserve no-mutation status");
	Expect(result.session.tickIndex == session.tickIndex + 1, "empty mutation command step should tick once through command tick");
	Expect(!result.commandTick.playerCommands.execution.executedPlanCount, "empty command frame should execute no player plans");
	Expect(NearVec(result.session.player.position, session.player.position), "empty command frame should preserve player position");
}

void TestNoExplicitUsesRefreshedCollisionCacheAfterMutation()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::LevelRuntimeState level = Level({ "...." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level));
	const iggy::runtime::RuntimeSessionMutationCommandInput input = Input(
		session,
		{ Edit({ 2, 0 }, false) },
		CommandFrame({ factory.moveToPoint(PlayerId, { 4.0F, 0.0F }) }));

	const iggy::runtime::RuntimeSessionMutationCommandResult result = iggy::runtime::RuntimeSessionMutationCommandStep {}.run(input);

	Expect(result.status == iggy::runtime::RuntimeSessionMutationCommandStatus::Ticked, "mutated collision cache command step should tick");
	Expect(result.mutation.status == iggy::runtime::RuntimeLevelMutationStatus::Applied, "mutated collision cache command step should apply mutation");
	Expect(result.mutation.session.derivedCaches.collision.world.objects().size() == 1, "mutation should refresh session collision cache before command tick");
	Expect(PlayerBlocked(result), "no-explicit command tick should use refreshed collision cache and block movement");
	Expect(result.session.tickIndex == session.tickIndex + 1, "mutated collision cache command step should tick once");
	Expect(result.session.player.position.x < 4.0F, "refreshed collision cache should prevent full player movement");
}

void TestExplicitWorldOverridesRefreshedCollisionCache()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::LevelRuntimeState level = Level({ "...." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level));
	const iggy::runtime::RuntimeSessionMutationCommandInput input = Input(
		session,
		{ Edit({ 2, 0 }, false) },
		CommandFrame({ factory.moveToPoint(PlayerId, { 4.0F, 0.0F }) }));
	const iggy::physics2d::CollisionWorld2D explicitWorld = World({});

	const iggy::runtime::RuntimeSessionMutationCommandResult result = iggy::runtime::RuntimeSessionMutationCommandStep {}.run(input, explicitWorld);

	Expect(result.status == iggy::runtime::RuntimeSessionMutationCommandStatus::Ticked, "explicit override mutation command step should tick");
	Expect(result.mutation.session.derivedCaches.collision.world.objects().size() == 1, "explicit override setup should still refresh session collision cache");
	Expect(!PlayerBlocked(result), "explicit empty world should override refreshed collision cache");
	Expect(NearVec(result.session.player.position, { 4.0F, 0.0F }), "explicit empty world should allow full player movement");
}

void TestMutationFailurePreventsCommandTick()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::LevelRuntimeState level = Level({ "...." });
	iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level, false, true));
	session.derivedCaches.hasRenderCache = true;
	session.derivedCaches.render.tileChunkConfig = RenderConfig(0, 1, 7);
	session.hasRenderCache = true;
	session.renderCache = session.derivedCaches.render;
	const iggy::runtime::RuntimeSessionState before = session;
	const iggy::runtime::RuntimeSessionMutationCommandInput input = Input(
		session,
		{ Edit({ 2, 0 }, false) },
		CommandFrame({ factory.moveToPoint(PlayerId, { 4.0F, 0.0F }) }));

	const iggy::runtime::RuntimeSessionMutationCommandResult result = iggy::runtime::RuntimeSessionMutationCommandStep {}.run(input);

	Expect(result.status == iggy::runtime::RuntimeSessionMutationCommandStatus::MutationFailed, "mutation failure should prevent command tick");
	Expect(result.mutation.status == iggy::runtime::RuntimeLevelMutationStatus::Failed, "mutation failure should preserve runtime mutation failure");
	Expect(result.mutation.levelMutation.cacheUpdate.render.dirtyChunks.issues.size() == 1, "mutation failure should preserve cache diagnostics");
	ExpectSessionSame(result.session, before, "mutation failure should leave final session unchanged");
	Expect(result.commandTick.session.tickIndex == 0, "mutation failure should not run command tick");
}

void TestInvalidCommandDiagnosticsArePreservedAndTickStillRuns()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::LevelRuntimeState level = Level({ "...." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level));
	const iggy::runtime::RuntimeSessionMutationCommandInput input = Input(
		session,
		{ Edit({ 1, 0 }, false) },
		CommandFrame({ factory.interact(PlayerId, {}) }));

	const iggy::runtime::RuntimeSessionMutationCommandResult result = iggy::runtime::RuntimeSessionMutationCommandStep {}.run(input);

	Expect(result.status == iggy::runtime::RuntimeSessionMutationCommandStatus::Ticked, "invalid command should not prevent session tick");
	Expect(result.commandTick.playerCommands.planning.playerPlan.validation.hasInvalidCommands, "invalid command diagnostics should be preserved");
	Expect(result.commandTick.playerCommands.planning.playerPlan.validation.invalidCommands.size() == 1, "invalid command diagnostics should include invalid command");
	Expect(result.commandTick.playerCommands.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::NoExecutablePlan, "invalid command should produce no executable plan");
	Expect(result.session.tickIndex == session.tickIndex + 1, "invalid command should still tick once");
}

void TestMissingPlayerUsesFallbackAfterMutationNoOp()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::LevelRuntimeState level = Level({ "...." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level), false);
	const iggy::runtime::RuntimeSessionMutationCommandInput input = Input(
		session,
		{},
		CommandFrame({ factory.moveToPoint({}, { 4.0F, 0.0F }) }));

	const iggy::runtime::RuntimeSessionMutationCommandResult result = iggy::runtime::RuntimeSessionMutationCommandStep {}.run(input);

	Expect(result.status == iggy::runtime::RuntimeSessionMutationCommandStatus::Ticked, "missing-player session should still tick");
	Expect(result.commandTick.playerCommands.planning.status == iggy::runtime::RuntimePlayerCommandPlanningStatus::MissingPlayer, "missing-player diagnostics should be preserved");
	Expect(!result.session.hasPlayer, "missing-player command step should not invent a player");
	Expect(result.session.tickIndex == session.tickIndex + 1, "missing-player command step should tick once");
}

void TestTileMutationCanRemoveBlockedCollisionBeforeMovement()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::LevelRuntimeState level = Level({ "..#." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level));
	const iggy::runtime::RuntimeSessionMutationCommandInput input = Input(
		session,
		{ Edit({ 2, 0 }, true) },
		CommandFrame({ factory.moveToPoint(PlayerId, { 4.0F, 0.0F }) }));

	const iggy::runtime::RuntimeSessionMutationCommandResult result = iggy::runtime::RuntimeSessionMutationCommandStep {}.run(input);

	Expect(result.status == iggy::runtime::RuntimeSessionMutationCommandStatus::Ticked, "walkable mutation command step should tick");
	Expect(result.mutation.session.derivedCaches.collision.world.objects().empty(), "walkable mutation should remove blocked collision before command tick");
	Expect(!PlayerBlocked(result), "removed collision should allow player movement");
	Expect(NearVec(result.session.player.position, { 4.0F, 0.0F }), "removed collision should allow full movement");
}

void TestFinalSessionSources()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::LevelRuntimeState level = Level({ "...." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level));
	const iggy::runtime::RuntimeSessionMutationCommandInput input = Input(
		session,
		{ Edit({ 1, 0 }, false) },
		CommandFrame({ factory.wait(PlayerId) }));

	const iggy::runtime::RuntimeSessionMutationCommandResult result = iggy::runtime::RuntimeSessionMutationCommandStep {}.run(input);

	Expect(result.status == iggy::runtime::RuntimeSessionMutationCommandStatus::Ticked, "final session source setup should tick");
	Expect(result.session.tickIndex == result.commandTick.session.tickIndex, "successful final session should come from command tick");
	Expect(result.session.level.map.tileAt(1, 0) != nullptr && !result.session.level.map.tileAt(1, 0)->walkable, "successful final session should include mutation map output");
}

void TestInputsAreNotMutated()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::LevelRuntimeState level = Level({ "...." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level));
	const iggy::runtime::RuntimeSessionState sessionBefore = session;
	std::vector<iggy::LevelTileEdit> edits { Edit({ 1, 0 }, false) };
	const std::vector<iggy::LevelTileEdit> editsBefore = edits;
	iggy::runtime::GameplayCommandFrame2D frame = CommandFrame({ factory.moveToPoint(PlayerId, { 4.0F, 0.0F }) });
	const iggy::runtime::GameplayCommandFrame2D frameBefore = frame;
	iggy::runtime::RuntimeSessionMutationCommandInput input = Input(session, edits, frame);
	const iggy::runtime::RuntimePlayerCommandExecutionConfig configBefore = input.playerCommandConfig;
	const iggy::physics2d::CollisionWorld2D explicitWorld = World({
		Object(iggy::ResourceId("wall:explicit"), { { 3.0F, -0.5F }, { 4.0F, 0.5F } }),
	});
	const std::vector<iggy::physics2d::CollisionObject2D> worldObjectsBefore = explicitWorld.objects();

	const iggy::runtime::RuntimeSessionMutationCommandResult result = iggy::runtime::RuntimeSessionMutationCommandStep {}.run(input, explicitWorld);

	Expect(result.status == iggy::runtime::RuntimeSessionMutationCommandStatus::Ticked, "immutability setup should tick");
	ExpectSessionSame(session, sessionBefore, "mutation command step should not mutate input session object");
	ExpectEditsSame(edits, editsBefore, "mutation command step should not mutate caller edit vector");
	ExpectEditsSame(input.levelEdits, editsBefore, "mutation command step should not mutate input edit vector");
	ExpectFrameSame(input.commandFrame, frameBefore, "mutation command step should not mutate input command frame");
	Expect(input.playerCommandConfig.movement.maxStep == configBefore.movement.maxStep, "mutation command step should not mutate input player command config");
	Expect(explicitWorld.objects().size() == worldObjectsBefore.size(), "mutation command step should not mutate explicit world object count");
	if (explicitWorld.objects().size() == worldObjectsBefore.size()) {
		for (std::size_t index = 0; index < worldObjectsBefore.size(); ++index)
			ExpectBounds(explicitWorld.objects()[index].shape.bounds, worldObjectsBefore[index].shape.bounds, "mutation command step should not mutate explicit world bounds");
	}
}

} // namespace

int main()
{
	TestEmptyEditsAndEmptyCommandFrameTicksOnce();
	TestNoExplicitUsesRefreshedCollisionCacheAfterMutation();
	TestExplicitWorldOverridesRefreshedCollisionCache();
	TestMutationFailurePreventsCommandTick();
	TestInvalidCommandDiagnosticsArePreservedAndTickStillRuns();
	TestMissingPlayerUsesFallbackAfterMutationNoOp();
	TestTileMutationCanRemoveBlockedCollisionBeforeMovement();
	TestFinalSessionSources();
	TestInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
