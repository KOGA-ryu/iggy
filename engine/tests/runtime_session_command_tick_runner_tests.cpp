#include <cstdlib>
#include <string_view>
#include <vector>

#include "runtime/RuntimeSessionCommandTickRunner.hpp"
#include "scene/level/LevelRenderCacheState.hpp"
#include "scene/level/LevelRuntimeBuilder.hpp"
#include "support/CommandFrameFixtures.hpp"
#include "support/GeometryAssertions.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::CommandFrame;
using iggy::test::Expect;
using iggy::test::ExpectBounds;
using iggy::test::ExpectPlayerAgent;
using iggy::test::Failures;
using iggy::test::Near;
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

const iggy::ResourceId PlayerId { "player:one" };
const iggy::ResourceId OtherActorId { "player:two" };
const iggy::ResourceId WalkableMaterial { "material:floor" };
const iggy::ResourceId BlockedMaterial { "material:wall" };

iggy::LevelBlueprint BlueprintWithNpc()
{
	iggy::LevelBlueprint blueprint;
	blueprint.id = iggy::ResourceId { "level:cellar_01" };
	blueprint.bounds = { 5, 3 };
	blueprint.tiles = {
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
	};
	blueprint.playerStarts.push_back({ 0, 0 });
	blueprint.entitySpawns.push_back({ iggy::ResourceId { "enemy:skeleton" }, 0, 1, iggy::ResourceId { "spawn:skeleton_01" } });
	return blueprint;
}

iggy::LevelRuntimeState StateFromBuild(const iggy::LevelRuntimeBuildResult &build)
{
	return { build.tileMap, build.npcAgents };
}

iggy::npc_ai::NpcAgentTickConfig NpcConfig(float maxDistance = 0.25F)
{
	iggy::npc_ai::NpcAgentTickConfig config;
	config.maxDistance = maxDistance;
	config.awareness = { 8.0F, 0 };
	return config;
}

iggy::LevelTileRenderChunkCacheConfig ChunkConfig()
{
	return { 2, 2, { { WalkableMaterial, BlockedMaterial }, 3 } };
}

iggy::LevelRenderCacheState BuildRenderCache(const iggy::LevelRuntimeState &level)
{
	const iggy::LevelRenderCacheBuildResult build = iggy::LevelRenderCacheBuilder {}.build(level.map, ChunkConfig());
	Expect(build.built, "render cache fixture should build");
	return build.state;
}

iggy::runtime::RuntimeSessionState SessionWithPlayer(iggy::Vec2 playerPosition = { 0.0F, 0.0F })
{
	iggy::runtime::RuntimeSessionState session;
	session.level = StateFromBuild(iggy::LevelRuntimeBuilder {}.build(BlueprintWithNpc()));
	session.player = PlayerAgent(PlayerId, playerPosition, { 0, 0 }, iggy::PlayerMovementStatus::Idle, iggy::PlayerFacing2D::North);
	session.hasPlayer = true;
	session.tickIndex = 6;
	return session;
}

iggy::runtime::RuntimePlayerCommandExecutionConfig PlayerCommandConfig(float maxStep = 1.0F)
{
	iggy::runtime::RuntimePlayerCommandExecutionConfig config;
	config.movement.maxStep = maxStep;
	return config;
}

iggy::runtime::RuntimeSessionCommandTickRunnerInput Input(
	iggy::runtime::RuntimeSessionState session,
	std::vector<iggy::runtime::GameplayCommandFrame2D> frames,
	iggy::Vec2 fallbackPlayerPosition = { 4.5F, 1.5F })
{
	return {
		session,
		frames,
		fallbackPlayerPosition,
		PlayerCommandConfig(),
		NpcConfig(),
	};
}

iggy::physics2d::CollisionObject2D Object(
	iggy::ResourceId id,
	iggy::Aabb2 bounds,
	bool solid = true)
{
	return { id, iggy::physics2d::makeAabbShape(bounds), solid };
}

iggy::physics2d::CollisionWorld2D World(std::vector<iggy::physics2d::CollisionObject2D> objects)
{
	const iggy::physics2d::CollisionWorldBuildResult result = iggy::physics2d::CollisionWorld2DBuilder {}.build(objects);
	Expect(result.built, "test fixture collision world should build");
	return result.world;
}

void ExpectObject(
	const iggy::physics2d::CollisionObject2D &actual,
	const iggy::physics2d::CollisionObject2D &expected,
	const char *message)
{
	Expect(actual.id == expected.id, message);
	Expect(actual.shape.type == expected.shape.type, message);
	ExpectBounds(actual.shape.bounds, expected.shape.bounds, message);
	Expect(actual.solid == expected.solid, message);
}

void ExpectWorldUnchanged(
	const iggy::physics2d::CollisionWorld2D &world,
	const std::vector<iggy::physics2d::CollisionObject2D> &before,
	const char *message)
{
	Expect(world.objects().size() == before.size(), message);
	for (std::size_t index = 0; index < before.size(); ++index)
		ExpectObject(world.objects()[index], before[index], message);
}

void ExpectLevelMapUnchanged(
	const iggy::runtime::RuntimeSessionState &actual,
	const iggy::runtime::RuntimeSessionState &expected,
	const char *message)
{
	Expect(actual.level.map.id == expected.level.map.id, message);
	Expect(actual.level.map.width == expected.level.map.width && actual.level.map.height == expected.level.map.height, message);
	Expect(actual.level.map.tiles.size() == expected.level.map.tiles.size(), message);
}

void TestEmptyFramesReturnInitialSessionUnchanged()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 4.5F, 1.5F });
	const iggy::runtime::RuntimeSessionCommandTickRunnerResult result = iggy::runtime::RuntimeSessionCommandTickRunner {}.run(
		Input(session, {}),
		{});

	Expect(result.ticks.empty(), "empty command frame list should produce no tick results");
	Expect(result.session.tickIndex == session.tickIndex, "empty command frame list should preserve tickIndex");
	ExpectPlayerAgent(result.session.player, session.player, "empty command frame runner result");
	ExpectLevelMapUnchanged(result.session, session, "empty command frame runner should preserve level map");
}

void TestOneFrameMatchesSingleCommandTick()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::runtime::GameplayCommandFrame2D frame = CommandFrame({
		factory.moveToPoint(PlayerId, { 2.0F, 0.0F }),
	});
	const iggy::runtime::RuntimeSessionCommandTickRunnerInput input = Input(session, { frame });
	const iggy::runtime::RuntimeSessionCommandTickResult single = iggy::runtime::RuntimeSessionCommandTick {}.run(
		{ session, frame, input.fallbackPlayerPosition, input.playerCommandConfig, input.npcConfig },
		{});

	const iggy::runtime::RuntimeSessionCommandTickRunnerResult result = iggy::runtime::RuntimeSessionCommandTickRunner {}.run(input, {});

	Expect(result.ticks.size() == 1, "one frame runner should produce one tick result");
	if (result.ticks.size() == 1) {
		Expect(result.ticks[0].playerCommands.execution.status == single.playerCommands.execution.status, "one frame runner should preserve command execution status");
		Expect(NearVec(result.ticks[0].npcTargetPosition, single.npcTargetPosition), "one frame runner should preserve NPC target");
		Expect(result.ticks[0].tick.npcReports.size() == single.tick.npcReports.size(), "one frame runner should preserve NPC report count");
	}
	Expect(result.session.tickIndex == single.session.tickIndex, "one frame runner final session should match single tick session index");
	ExpectPlayerAgent(result.session.player, single.session.player, "one frame runner final session");
}

void TestMultipleFramesCarrySessionForward()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::runtime::RuntimeSessionCommandTickRunnerResult result = iggy::runtime::RuntimeSessionCommandTickRunner {}.run(
		Input(
			session,
			{
				CommandFrame({ factory.moveToPoint(PlayerId, { 2.0F, 0.0F }) }),
				CommandFrame({ factory.moveToPoint(PlayerId, { 4.0F, 0.0F }) }),
			}),
		{});

	Expect(result.ticks.size() == 2, "multiple frames should produce one tick result per frame");
	if (result.ticks.size() == 2) {
		Expect(NearVec(result.ticks[0].session.player.position, { 1.0F, 0.0F }), "first tick should move player by first command");
		Expect(result.ticks[0].session.tickIndex == session.tickIndex + 1, "first tick should advance tickIndex once");
		Expect(NearVec(result.ticks[1].session.player.position, { 2.0F, 0.0F }), "second tick should continue from first tick session");
		Expect(result.ticks[1].session.tickIndex == session.tickIndex + 2, "second tick should advance tickIndex again");
		Expect(NearVec(result.ticks[1].npcTargetPosition, { 2.0F, 0.0F }), "second tick should target post-command player position");
	}
	Expect(NearVec(result.session.player.position, { 2.0F, 0.0F }), "runner final session should be last tick session");
	Expect(result.session.tickIndex == session.tickIndex + 2, "runner final session should carry last tickIndex");
}

void TestPerTickDiagnosticsArePreservedInOrder()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 4.5F, 1.5F });
	const iggy::runtime::RuntimeSessionCommandTickRunnerResult result = iggy::runtime::RuntimeSessionCommandTickRunner {}.run(
		Input(
			session,
			{
				CommandFrame({ factory.interact(PlayerId, {}) }),
				CommandFrame({ factory.moveToPoint(PlayerId, { 5.5F, 1.5F }) }),
			}),
		{});

	Expect(result.ticks.size() == 2, "diagnostics should be preserved for each tick");
	if (result.ticks.size() == 2) {
		Expect(result.ticks[0].playerCommands.planning.playerPlan.validation.hasInvalidCommands, "first tick should preserve invalid command diagnostics");
		Expect(result.ticks[0].playerCommands.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::NoExecutablePlan, "first tick should preserve no executable status");
		Expect(result.ticks[0].tick.npcReports.size() == 1, "first tick should preserve NPC reports");
		Expect(result.ticks[1].playerCommands.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::Executed, "second tick should preserve execution status");
		Expect(result.ticks[1].playerCommands.execution.movementResults.size() == 1, "second tick should preserve movement diagnostics");
		Expect(result.ticks[1].tick.npcReports.size() == 1, "second tick should preserve NPC reports");
	}
}

void TestMissingPlayerUsesFallbackEachFrame()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	session.hasPlayer = false;
	session.player = {};
	const iggy::Vec2 fallback { 4.5F, 1.5F };

	const iggy::runtime::RuntimeSessionCommandTickRunnerResult result = iggy::runtime::RuntimeSessionCommandTickRunner {}.run(
		Input(
			session,
			{
				CommandFrame({ factory.moveToPoint(PlayerId, { 2.0F, 0.0F }) }),
				CommandFrame({ factory.wait(PlayerId) }),
			},
			fallback),
		{});

	Expect(result.ticks.size() == 2, "missing player should still run one tick per frame");
	for (const iggy::runtime::RuntimeSessionCommandTickResult &tick : result.ticks) {
		Expect(tick.playerCommands.planning.status == iggy::runtime::RuntimePlayerCommandPlanningStatus::MissingPlayer, "missing player runner should preserve MissingPlayer planning status");
		Expect(tick.playerCommands.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::MissingPlayer, "missing player runner should preserve MissingPlayer execution status");
		Expect(NearVec(tick.npcTargetPosition, fallback), "missing player runner should use fallback every tick");
		Expect(!tick.session.hasPlayer, "missing player runner should not invent player state");
	}
	Expect(result.session.tickIndex == session.tickIndex + 2, "missing player runner should advance tickIndex per frame");
	Expect(!result.session.hasPlayer, "missing player final session should remain without player");
}

void TestInvalidAndNoOpFramesStillAdvanceTicks()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 4.5F, 1.5F });

	const iggy::runtime::RuntimeSessionCommandTickRunnerResult result = iggy::runtime::RuntimeSessionCommandTickRunner {}.run(
		Input(
			session,
			{
				CommandFrame({ factory.interact(PlayerId, {}) }),
				CommandFrame({ factory.wait(PlayerId) }),
			}),
		{});

	Expect(result.ticks.size() == 2, "invalid/no-op frames should produce tick results");
	if (result.ticks.size() == 2) {
		Expect(result.ticks[0].playerCommands.planning.playerPlan.validation.hasInvalidCommands, "invalid frame should preserve invalid diagnostics");
		Expect(result.ticks[0].session.tickIndex == session.tickIndex + 1, "invalid frame should advance tickIndex through command tick");
		Expect(result.ticks[1].playerCommands.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::NoExecutablePlan, "wait frame should preserve no executable status");
		Expect(result.ticks[1].session.tickIndex == session.tickIndex + 2, "wait frame should advance tickIndex through command tick");
	}
	Expect(result.session.tickIndex == session.tickIndex + 2, "invalid/no-op runner should advance final tickIndex");
}

void TestRenderCacheIsPreservedAcrossMultipleTicks()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 4.5F, 1.5F });
	session.renderCache = BuildRenderCache(session.level);
	session.hasRenderCache = true;
	const std::size_t originalChunkCount = session.renderCache.tileChunks.chunks.size();
	const iggy::LevelTileRenderChunkCacheConfig originalConfig = session.renderCache.tileChunkConfig;

	const iggy::runtime::RuntimeSessionCommandTickRunnerResult result = iggy::runtime::RuntimeSessionCommandTickRunner {}.run(
		Input(
			session,
			{
				CommandFrame({ factory.wait(PlayerId) }),
				CommandFrame({ factory.wait(PlayerId) }),
			}),
		{});

	Expect(result.session.tickIndex == session.tickIndex + 2, "render cache runner should advance tickIndex per frame");
	Expect(result.session.hasRenderCache, "render cache runner should preserve hasRenderCache");
	Expect(result.session.renderCache.tileChunkConfig.chunkWidth == originalConfig.chunkWidth && result.session.renderCache.tileChunkConfig.chunkHeight == originalConfig.chunkHeight, "render cache runner should preserve chunk config");
	Expect(result.session.renderCache.tileChunkConfig.tileCommands.layer == originalConfig.tileCommands.layer, "render cache runner should preserve render cache layer");
	Expect(result.session.renderCache.tileChunks.chunks.size() == originalChunkCount, "render cache runner should preserve chunk count");
}

void TestInputsAreNotMutated()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	iggy::runtime::RuntimeSessionCommandTickRunnerInput input = Input(
		SessionWithPlayer({ 4.5F, 1.5F }),
		{
			CommandFrame({ factory.wait(PlayerId) }),
			CommandFrame({ factory.moveToPoint(PlayerId, { 5.5F, 1.5F }) }),
		});
	input.playerCommandConfig.movement.bodySize = { 2.0F, 1.0F };
	input.playerCommandConfig.movement.bodyAnchor = { 0.0F, 0.5F };
	input.npcConfig.maxDistance = 0.5F;
	input.npcConfig.awareness.visionRange = 9.0F;
	const iggy::runtime::RuntimeSessionCommandTickRunnerInput originalInput = input;
	const iggy::physics2d::CollisionObject2D wall = Object(
		iggy::ResourceId("wall"),
		{ { 5.0F, -1.0F }, { 6.0F, 1.0F } },
		false);
	const iggy::physics2d::CollisionWorld2D world = World({ wall });
	const std::vector<iggy::physics2d::CollisionObject2D> beforeObjects = world.objects();

	const iggy::runtime::RuntimeSessionCommandTickRunnerResult result = iggy::runtime::RuntimeSessionCommandTickRunner {}.run(input, world);

	Expect(result.ticks.size() == 2, "input mutation setup should run both ticks");
	Expect(input.session.tickIndex == originalInput.session.tickIndex, "runner should not mutate input session tickIndex");
	ExpectPlayerAgent(input.session.player, originalInput.session.player, "runner input session");
	Expect(input.commandFrames.size() == originalInput.commandFrames.size(), "runner should not mutate command frame vector size");
	if (input.commandFrames.size() == originalInput.commandFrames.size() && input.commandFrames.size() == 2) {
		Expect(input.commandFrames[0].commands.size() == originalInput.commandFrames[0].commands.size(), "runner should not mutate first frame command count");
		Expect(input.commandFrames[1].commands.size() == originalInput.commandFrames[1].commands.size(), "runner should not mutate second frame command count");
		Expect(input.commandFrames[1].commands[0].type == originalInput.commandFrames[1].commands[0].type && NearVec(input.commandFrames[1].commands[0].targetPoint, originalInput.commandFrames[1].commands[0].targetPoint), "runner should not mutate second frame move target");
	}
	Expect(Near(input.fallbackPlayerPosition.x, originalInput.fallbackPlayerPosition.x) && Near(input.fallbackPlayerPosition.y, originalInput.fallbackPlayerPosition.y), "runner should not mutate fallback position");
	Expect(Near(input.playerCommandConfig.movement.maxStep, originalInput.playerCommandConfig.movement.maxStep), "runner should not mutate player command maxStep");
	Expect(NearVec(input.playerCommandConfig.movement.bodySize, originalInput.playerCommandConfig.movement.bodySize), "runner should not mutate player bodySize");
	Expect(NearVec(input.playerCommandConfig.movement.bodyAnchor, originalInput.playerCommandConfig.movement.bodyAnchor), "runner should not mutate player bodyAnchor");
	Expect(Near(input.npcConfig.maxDistance, originalInput.npcConfig.maxDistance), "runner should not mutate NPC maxDistance");
	Expect(Near(input.npcConfig.awareness.visionRange, originalInput.npcConfig.awareness.visionRange), "runner should not mutate NPC awareness config");
	ExpectWorldUnchanged(world, beforeObjects, "runner should not mutate collision world");
	ExpectLevelMapUnchanged(input.session, originalInput.session, "runner should not mutate input level map");
}

} // namespace

int main()
{
	TestEmptyFramesReturnInitialSessionUnchanged();
	TestOneFrameMatchesSingleCommandTick();
	TestMultipleFramesCarrySessionForward();
	TestPerTickDiagnosticsArePreservedInOrder();
	TestMissingPlayerUsesFallbackEachFrame();
	TestInvalidAndNoOpFramesStillAdvanceTicks();
	TestRenderCacheIsPreservedAcrossMultipleTicks();
	TestInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
