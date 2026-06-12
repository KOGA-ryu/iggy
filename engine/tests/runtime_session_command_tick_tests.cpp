#include <cstdlib>
#include <string_view>
#include <vector>

#include "runtime/RuntimeSessionCommandTick.hpp"
#include "scene/level/LevelDerivedCacheState.hpp"
#include "scene/level/LevelRenderCacheState.hpp"
#include "scene/level/LevelRuntimeBuilder.hpp"
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

iggy::LevelDerivedCacheState BuildDerivedCaches(const iggy::LevelRuntimeState &level)
{
	iggy::LevelDerivedCacheBuildConfig config;
	config.buildRenderCache = true;
	config.renderCacheConfig = ChunkConfig();
	config.buildCollisionCache = true;
	const iggy::LevelDerivedCacheBuildResult build = iggy::LevelDerivedCacheBuilder {}.build(level, config);
	Expect(build.built, "derived cache fixture should build");
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

void SetCollisionCache(iggy::runtime::RuntimeSessionState &session, const iggy::physics2d::CollisionWorld2D &world)
{
	session.derivedCaches.hasCollisionCache = true;
	session.derivedCaches.collision.world = world;
}

iggy::runtime::RuntimePlayerCommandExecutionConfig PlayerCommandConfig(float maxStep = 1.0F)
{
	iggy::runtime::RuntimePlayerCommandExecutionConfig config;
	config.movement.maxStep = maxStep;
	return config;
}

iggy::runtime::RuntimeSessionCommandTickInput Input(
	iggy::runtime::RuntimeSessionState session,
	iggy::runtime::GameplayCommandFrame2D frame,
	iggy::Vec2 fallbackPlayerPosition = { 4.5F, 1.5F })
{
	return {
		session,
		frame,
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

bool HasEvent(const iggy::npc_ai::NpcTickReport &report, iggy::npc_ai::NpcTickEventType type)
{
	for (const iggy::npc_ai::NpcTickEvent &event : report.events) {
		if (event.type == type)
			return true;
	}
	return false;
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

void TestMoveCommandRunsBeforeSessionTick()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	iggy::runtime::RuntimeSessionCommandTickInput input = Input(
		SessionWithPlayer(),
		CommandFrame({
			factory.moveToPoint(PlayerId, { 2.0F, 0.0F }),
		}));

	const iggy::runtime::RuntimeSessionCommandTickResult result = iggy::runtime::RuntimeSessionCommandTick {}.run(input, {});

	Expect(result.playerCommands.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::Executed, "move command should execute before session tick");
	Expect(NearVec(result.playerCommands.session.player.position, { 1.0F, 0.0F }), "player command step should move player before tick");
	Expect(NearVec(result.npcTargetPosition, { 1.0F, 0.0F }), "session tick should target post-command player position");
	Expect(result.tick.session.tickIndex == input.session.tickIndex + 1, "session command tick should increment tickIndex through RuntimeSessionTick");
	Expect(result.session.tickIndex == result.tick.session.tickIndex, "result session should come from session tick result");
	ExpectPlayerAgent(result.session.player, result.playerCommands.session.player, "session command tick final session");
}

void TestEmptyFrameStillTicksUsingCurrentPlayerPosition()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 4.5F, 1.5F });
	const iggy::runtime::RuntimeSessionCommandTickResult result = iggy::runtime::RuntimeSessionCommandTick {}.run(
		Input(session, {}),
		{});

	Expect(result.playerCommands.planning.status == iggy::runtime::RuntimePlayerCommandPlanningStatus::Planned, "empty frame should still plan for present player");
	Expect(result.playerCommands.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::NoExecutablePlan, "empty frame should have no executable player plan");
	Expect(NearVec(result.npcTargetPosition, session.player.position), "empty frame should use current player position for NPC target");
	Expect(result.session.tickIndex == session.tickIndex + 1, "empty frame should still tick session once");
	Expect(result.tick.npcReports.size() == 1, "empty frame should preserve NPC tick reports");
	if (result.tick.npcReports.size() == 1) {
		Expect(result.tick.npcReports[0].id == iggy::ResourceId { "spawn:skeleton_01" }, "NPC report id should be preserved");
		Expect(HasEvent(result.tick.npcReports[0].report, iggy::npc_ai::NpcTickEventType::PositionChanged), "NPC report should preserve position changed event");
	}
	Expect(result.session.level.npcAgents.size() == 1, "empty frame should preserve NPC count after tick");
	if (result.session.level.npcAgents.size() == 1)
		Expect(NearVec(result.session.level.npcAgents[0].state.position, { 0.75F, 1.5F }), "empty frame should tick NPC toward current player position");
}

void TestMissingPlayerTicksUsingFallback()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	session.hasPlayer = false;
	session.player = {};
	const iggy::Vec2 fallback { 4.5F, 1.5F };
	const iggy::runtime::RuntimeSessionCommandTickResult result = iggy::runtime::RuntimeSessionCommandTick {}.run(
		Input(
			session,
			CommandFrame({
				factory.moveToPoint(PlayerId, { 2.0F, 0.0F }),
			}),
			fallback),
		{});

	Expect(result.playerCommands.planning.status == iggy::runtime::RuntimePlayerCommandPlanningStatus::MissingPlayer, "missing player should preserve command planning MissingPlayer");
	Expect(result.playerCommands.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::MissingPlayer, "missing player should preserve command execution MissingPlayer");
	Expect(NearVec(result.npcTargetPosition, fallback), "missing player should use fallback NPC target position");
	Expect(result.session.tickIndex == session.tickIndex + 1, "missing player should still tick session once");
	Expect(!result.session.hasPlayer, "missing player should not invent player state");
	Expect(result.tick.npcReports.size() == 1, "missing player fallback should still preserve NPC tick reports");
}

void TestInvalidCommandDiagnosticsDoNotPreventTick()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 4.5F, 1.5F });
	const iggy::runtime::RuntimeSessionCommandTickResult result = iggy::runtime::RuntimeSessionCommandTick {}.run(
		Input(
			session,
			CommandFrame({
				factory.interact(PlayerId, {}),
			})),
		{});

	Expect(result.playerCommands.planning.playerPlan.validation.hasInvalidCommands, "invalid command diagnostics should be preserved");
	Expect(result.playerCommands.planning.playerPlan.validation.invalidCommands.size() == 1, "invalid command diagnostic count should be preserved");
	Expect(result.playerCommands.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::NoExecutablePlan, "invalid-only frame should not execute player movement");
	Expect(result.session.tickIndex == session.tickIndex + 1, "invalid command frame should still tick session once");
	Expect(NearVec(result.npcTargetPosition, session.player.position), "invalid command frame should tick against unchanged player position");
}

void TestActorMismatchDiagnosticsDoNotPreventTick()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 4.5F, 1.5F });
	const iggy::runtime::RuntimeSessionCommandTickResult result = iggy::runtime::RuntimeSessionCommandTick {}.run(
		Input(
			session,
			CommandFrame({
				factory.moveToPoint(OtherActorId, { 2.0F, 0.0F }),
			})),
		{});

	Expect(result.playerCommands.planning.playerPlan.rejectedPlans.size() == 1, "actor mismatch rejected plan diagnostic should be preserved");
	if (result.playerCommands.planning.playerPlan.rejectedPlans.size() == 1)
		Expect(result.playerCommands.planning.playerPlan.rejectedPlans[0].plan.rejectReason == iggy::PlayerCommandPlan2DRejectReason::ActorMismatch, "actor mismatch reject reason should be preserved");
	Expect(result.playerCommands.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::NoExecutablePlan, "actor mismatch should not execute player movement");
	Expect(result.session.tickIndex == session.tickIndex + 1, "actor mismatch frame should still tick session once");
	Expect(NearVec(result.npcTargetPosition, session.player.position), "actor mismatch frame should tick against unchanged player position");
}

void TestMultipleMoveCommandsUseExistingFirstMovePolicy()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::runtime::RuntimeSessionCommandTickResult result = iggy::runtime::RuntimeSessionCommandTick {}.run(
		Input(
			session,
			CommandFrame({
				factory.moveToPoint(PlayerId, { 2.0F, 0.0F }),
				factory.moveToPoint(PlayerId, { 0.0F, 4.0F }),
			})),
		{});

	Expect(result.playerCommands.planning.playerPlan.plans.size() == 2, "multiple move commands should preserve planned command count");
	Expect(result.playerCommands.execution.executedPlanCount == 1, "multiple move commands should execute one plan through existing policy");
	Expect(NearVec(result.playerCommands.session.player.position, { 1.0F, 0.0F }), "multiple move commands should execute only first MoveToPoint");
	Expect(NearVec(result.npcTargetPosition, { 1.0F, 0.0F }), "multiple move commands should tick NPCs against first move result");
}

void TestBlockedMoveStillTicksWithExecutionDiagnostics()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::physics2d::CollisionWorld2D world = World({
		Object(iggy::ResourceId("wall"), { { 2.5F, -0.5F }, { 3.5F, 0.5F } }),
	});
	iggy::runtime::RuntimeSessionCommandTickInput input = Input(
		session,
		CommandFrame({
			factory.moveToPoint(PlayerId, { 8.0F, 0.0F }),
		}));
	input.playerCommandConfig = PlayerCommandConfig(4.0F);

	const iggy::runtime::RuntimeSessionCommandTickResult result = iggy::runtime::RuntimeSessionCommandTick {}.run(input, world);

	Expect(result.playerCommands.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::Executed, "blocked player command should still execute command step");
	Expect(result.playerCommands.execution.movementResults.size() == 1, "blocked player command should preserve movement diagnostics");
	if (result.playerCommands.execution.movementResults.size() == 1)
		Expect(result.playerCommands.execution.movementResults[0].status == iggy::PlayerMovementExecutionStatus::Blocked, "blocked player command should preserve blocked movement status");
	Expect(NearVec(result.playerCommands.session.player.position, { 2.0F, 0.0F }), "blocked player command should preserve allowed player position");
	Expect(NearVec(result.npcTargetPosition, result.playerCommands.session.player.position), "blocked player command should tick NPCs against allowed player position");
	Expect(result.session.tickIndex == session.tickIndex + 1, "blocked player command should still tick session once");
}

void TestExplicitWorldWinsOverSessionCollisionCache()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	SetCollisionCache(session, World({}));
	const iggy::physics2d::CollisionWorld2D explicitWorld = World({
		Object(iggy::ResourceId("explicit:wall"), { { 2.5F, -0.5F }, { 3.5F, 0.5F } }),
	});
	iggy::runtime::RuntimeSessionCommandTickInput input = Input(
		session,
		CommandFrame({
			factory.moveToPoint(PlayerId, { 8.0F, 0.0F }),
		}));
	input.playerCommandConfig = PlayerCommandConfig(4.0F);

	const iggy::runtime::RuntimeSessionCommandTickResult result = iggy::runtime::RuntimeSessionCommandTick {}.run(input, explicitWorld);

	Expect(result.playerCommands.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::Executed, "explicit world precedence setup should execute command");
	Expect(result.playerCommands.execution.movementResults.size() == 1, "explicit world precedence should preserve movement diagnostic");
	if (result.playerCommands.execution.movementResults.size() == 1) {
		Expect(result.playerCommands.execution.movementResults[0].status == iggy::PlayerMovementExecutionStatus::Blocked, "explicit world should block even when session cache would not");
		Expect(result.playerCommands.execution.movementResults[0].movement.motion.hit.object.id == iggy::ResourceId("explicit:wall"), "explicit world should provide the blocking hit object");
	}
	Expect(NearVec(result.session.player.position, { 2.0F, 0.0F }), "explicit world should constrain command tick movement");
}

void TestNoExplicitWorldUsesSessionCollisionCache()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::physics2d::CollisionWorld2D cachedWorld = World({
		Object(iggy::ResourceId("session:wall"), { { 2.5F, -0.5F }, { 3.5F, 0.5F } }),
	});
	SetCollisionCache(session, cachedWorld);
	iggy::runtime::RuntimeSessionCommandTickInput input = Input(
		session,
		CommandFrame({
			factory.moveToPoint(PlayerId, { 8.0F, 0.0F }),
		}));
	input.playerCommandConfig = PlayerCommandConfig(4.0F);

	const iggy::runtime::RuntimeSessionCommandTickResult result = iggy::runtime::RuntimeSessionCommandTick {}.run(input);

	Expect(result.playerCommands.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::Executed, "session collision cache setup should execute command");
	Expect(result.playerCommands.execution.movementResults.size() == 1, "session collision cache should preserve movement diagnostic");
	if (result.playerCommands.execution.movementResults.size() == 1) {
		Expect(result.playerCommands.execution.movementResults[0].status == iggy::PlayerMovementExecutionStatus::Blocked, "session collision cache should block movement");
		Expect(result.playerCommands.execution.movementResults[0].movement.motion.hit.object.id == iggy::ResourceId("session:wall"), "session collision cache should provide blocking hit object");
	}
	Expect(NearVec(result.session.player.position, { 2.0F, 0.0F }), "session collision cache should constrain command tick movement");
}

void TestNoExplicitWorldAndNoSessionCollisionCacheUsesEmptyWorld()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	iggy::runtime::RuntimeSessionCommandTickInput input = Input(
		session,
		CommandFrame({
			factory.moveToPoint(PlayerId, { 4.0F, 0.0F }),
		}));
	input.playerCommandConfig = PlayerCommandConfig(4.0F);

	const iggy::runtime::RuntimeSessionCommandTickResult result = iggy::runtime::RuntimeSessionCommandTick {}.run(input);

	Expect(result.playerCommands.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::Executed, "empty collision fallback setup should execute command");
	Expect(result.playerCommands.execution.movementResults.size() == 1, "empty collision fallback should preserve movement diagnostic");
	if (result.playerCommands.execution.movementResults.size() == 1)
		Expect(result.playerCommands.execution.movementResults[0].status == iggy::PlayerMovementExecutionStatus::Moved, "empty collision fallback should allow movement");
	Expect(NearVec(result.session.player.position, { 4.0F, 0.0F }), "empty collision fallback should allow full requested movement");
}

void TestRenderCacheAndSessionMetadataArePreservedThroughTick()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 4.5F, 1.5F });
	session.level.map = iggy::test::MapFromRows({
		".#",
		"..",
	});
	session.renderCache = BuildRenderCache(session.level);
	session.derivedCaches = BuildDerivedCaches(session.level);
	session.hasRenderCache = true;
	const std::size_t originalChunkCount = session.renderCache.tileChunks.chunks.size();
	const std::size_t originalDerivedRenderChunkCount = session.derivedCaches.render.tileChunks.chunks.size();
	const std::size_t originalCollisionObjectCount = session.derivedCaches.collision.world.objects().size();
	const iggy::LevelTileRenderChunkCacheConfig originalConfig = session.renderCache.tileChunkConfig;

	const iggy::runtime::RuntimeSessionCommandTickResult result = iggy::runtime::RuntimeSessionCommandTick {}.run(
		Input(
			session,
			CommandFrame({
				factory.wait(PlayerId),
			})),
		{});

	Expect(result.session.tickIndex == session.tickIndex + 1, "metadata test should tick once");
	ExpectLevelMapUnchanged(result.session, session, "session command tick should preserve level map");
	Expect(result.session.hasRenderCache, "session command tick should preserve hasRenderCache");
	Expect(result.session.renderCache.tileChunkConfig.chunkWidth == originalConfig.chunkWidth && result.session.renderCache.tileChunkConfig.chunkHeight == originalConfig.chunkHeight, "session command tick should preserve render cache chunk config");
	Expect(result.session.renderCache.tileChunkConfig.tileCommands.layer == originalConfig.tileCommands.layer, "session command tick should preserve render cache layer config");
	Expect(result.session.renderCache.tileChunks.chunks.size() == originalChunkCount, "session command tick should preserve render cache chunks");
	Expect(result.session.derivedCaches.hasRenderCache, "session command tick should preserve derived render cache flag");
	Expect(result.session.derivedCaches.hasCollisionCache, "session command tick should preserve derived collision cache flag");
	Expect(result.session.derivedCaches.render.tileChunks.chunks.size() == originalDerivedRenderChunkCount, "session command tick should preserve derived render chunks");
	Expect(result.session.derivedCaches.collision.world.objects().size() == originalCollisionObjectCount, "session command tick should preserve derived collision objects");
}

void TestInputsAreNotMutated()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	iggy::runtime::RuntimeSessionCommandTickInput input = Input(
		SessionWithPlayer({ 4.5F, 1.5F }),
		CommandFrame({
			factory.wait(PlayerId),
			factory.moveToPoint(PlayerId, { 2.0F, 0.0F }),
		}));
	input.playerCommandConfig.movement.bodySize = { 2.0F, 1.0F };
	input.playerCommandConfig.movement.bodyAnchor = { 0.0F, 0.5F };
	input.npcConfig.maxDistance = 0.5F;
	input.npcConfig.awareness.visionRange = 9.0F;
	const iggy::runtime::RuntimeSessionCommandTickInput originalInput = input;
	const iggy::physics2d::CollisionObject2D wall = Object(
		iggy::ResourceId("wall"),
		{ { 5.0F, -1.0F }, { 6.0F, 1.0F } },
		false);
	const iggy::physics2d::CollisionWorld2D world = World({ wall });
	const std::vector<iggy::physics2d::CollisionObject2D> beforeObjects = world.objects();

	const iggy::runtime::RuntimeSessionCommandTickResult result = iggy::runtime::RuntimeSessionCommandTick {}.run(input, world);

	Expect(result.session.tickIndex == input.session.tickIndex + 1, "input mutation setup should tick once");
	Expect(input.session.tickIndex == originalInput.session.tickIndex, "session command tick should not mutate input session tickIndex");
	ExpectPlayerAgent(input.session.player, originalInput.session.player, "session command tick input session");
	Expect(input.commandFrame.commands.size() == originalInput.commandFrame.commands.size(), "session command tick should not mutate command frame count");
	Expect(input.commandFrame.commands[1].type == originalInput.commandFrame.commands[1].type && NearVec(input.commandFrame.commands[1].targetPoint, originalInput.commandFrame.commands[1].targetPoint), "session command tick should not mutate command frame move target");
	Expect(Near(input.fallbackPlayerPosition.x, originalInput.fallbackPlayerPosition.x) && Near(input.fallbackPlayerPosition.y, originalInput.fallbackPlayerPosition.y), "session command tick should not mutate fallback position");
	Expect(Near(input.playerCommandConfig.movement.maxStep, originalInput.playerCommandConfig.movement.maxStep), "session command tick should not mutate player command maxStep");
	Expect(NearVec(input.playerCommandConfig.movement.bodySize, originalInput.playerCommandConfig.movement.bodySize), "session command tick should not mutate player bodySize");
	Expect(NearVec(input.playerCommandConfig.movement.bodyAnchor, originalInput.playerCommandConfig.movement.bodyAnchor), "session command tick should not mutate player bodyAnchor");
	Expect(Near(input.npcConfig.maxDistance, originalInput.npcConfig.maxDistance), "session command tick should not mutate NPC maxDistance");
	Expect(Near(input.npcConfig.awareness.visionRange, originalInput.npcConfig.awareness.visionRange), "session command tick should not mutate NPC awareness config");
	ExpectWorldUnchanged(world, beforeObjects, "session command tick should not mutate collision world");
}

} // namespace

int main()
{
	TestMoveCommandRunsBeforeSessionTick();
	TestEmptyFrameStillTicksUsingCurrentPlayerPosition();
	TestMissingPlayerTicksUsingFallback();
	TestInvalidCommandDiagnosticsDoNotPreventTick();
	TestActorMismatchDiagnosticsDoNotPreventTick();
	TestMultipleMoveCommandsUseExistingFirstMovePolicy();
	TestBlockedMoveStillTicksWithExecutionDiagnostics();
	TestExplicitWorldWinsOverSessionCollisionCache();
	TestNoExplicitWorldUsesSessionCollisionCache();
	TestNoExplicitWorldAndNoSessionCollisionCacheUsesEmptyWorld();
	TestRenderCacheAndSessionMetadataArePreservedThroughTick();
	TestInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
