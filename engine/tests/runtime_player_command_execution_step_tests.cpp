#include <cstdlib>
#include <vector>

#include "runtime/RuntimePlayerCommandExecutionStep.hpp"
#include "scene/level/LevelRenderCacheState.hpp"
#include "support/GeometryAssertions.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::ExpectBounds;
using iggy::test::ExpectPlayerAgent;
using iggy::test::Failures;
using iggy::test::MapFromRows;
using iggy::test::Near;
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

const iggy::ResourceId PlayerId { "player:one" };
const iggy::ResourceId WalkableMaterial { "material:floor" };
const iggy::ResourceId BlockedMaterial { "material:wall" };

iggy::PlayerCommandPlan2D MovePlan(iggy::Vec2 target)
{
	iggy::PlayerCommandPlan2D plan;
	plan.type = iggy::PlayerCommandPlan2DType::MoveToPoint;
	plan.actorId = PlayerId;
	plan.targetPoint = target;
	return plan;
}

iggy::PlayerCommandPlan2D Plan(iggy::PlayerCommandPlan2DType type)
{
	iggy::PlayerCommandPlan2D plan;
	plan.type = type;
	plan.actorId = PlayerId;
	if (type == iggy::PlayerCommandPlan2DType::Rejected)
		plan.rejectReason = iggy::PlayerCommandPlan2DRejectReason::InvalidCommand;
	return plan;
}

iggy::PlayerCommandFramePlan2DResult PlanResult(std::vector<iggy::PlayerCommandPlan2D> plans)
{
	iggy::PlayerCommandFramePlan2DResult result;
	result.plans = plans;
	return result;
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

iggy::runtime::RuntimeSessionState SessionWithPlayer()
{
	iggy::runtime::RuntimeSessionState session;
	session.level.map = MapFromRows({
		"..",
		"..",
	});
	session.level.npcAgents.push_back({ iggy::ResourceId("npc:one"), { { 4.0F, 5.0F }, { 1, 2 }, {}, {} } });
	session.player = PlayerAgent(PlayerId, { 0.0F, 0.0F }, { 0, 0 }, iggy::PlayerMovementStatus::Idle, iggy::PlayerFacing2D::North);
	session.hasPlayer = true;
	session.tickIndex = 9;
	session.hasRenderCache = true;
	session.renderCache.tileChunkConfig = { 2, 2, { { WalkableMaterial, BlockedMaterial }, 7 } };
	return session;
}

iggy::runtime::RuntimePlayerCommandExecutionConfig Config(float maxStep = 1.0F)
{
	iggy::runtime::RuntimePlayerCommandExecutionConfig config;
	config.movement.maxStep = maxStep;
	return config;
}

void ExpectSessionMetadataUnchanged(
	const iggy::runtime::RuntimeSessionState &actual,
	const iggy::runtime::RuntimeSessionState &expected,
	const char *message)
{
	Expect(actual.hasPlayer == expected.hasPlayer, message);
	Expect(actual.tickIndex == expected.tickIndex, message);
	Expect(actual.hasRenderCache == expected.hasRenderCache, message);
	Expect(actual.renderCache.tileChunkConfig.chunkWidth == expected.renderCache.tileChunkConfig.chunkWidth, message);
	Expect(actual.renderCache.tileChunkConfig.chunkHeight == expected.renderCache.tileChunkConfig.chunkHeight, message);
	Expect(actual.renderCache.tileChunkConfig.tileCommands.layer == expected.renderCache.tileChunkConfig.tileCommands.layer, message);
	Expect(actual.level.map.width == expected.level.map.width && actual.level.map.height == expected.level.map.height, message);
	Expect(actual.level.map.tiles.size() == expected.level.map.tiles.size(), message);
	Expect(actual.level.npcAgents.size() == expected.level.npcAgents.size(), message);
	if (!actual.level.npcAgents.empty() && !expected.level.npcAgents.empty()) {
		Expect(actual.level.npcAgents[0].id == expected.level.npcAgents[0].id, message);
		Expect(NearVec(actual.level.npcAgents[0].state.position, expected.level.npcAgents[0].state.position), message);
	}
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

void TestMissingPlayerReturnsMissingPlayerAndDoesNotExecute()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	session.hasPlayer = false;
	session.player = {};
	const iggy::runtime::RuntimeSessionState original = session;

	const iggy::runtime::RuntimePlayerCommandExecutionResult result = iggy::runtime::RuntimePlayerCommandExecutionStep {}.execute(
		session,
		PlanResult({ MovePlan({ 3.0F, 0.0F }) }),
		{},
		Config(3.0F));

	Expect(result.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::MissingPlayer, "missing player should return MissingPlayer");
	Expect(result.movementResults.empty(), "missing player should execute no movement");
	Expect(result.executedPlanCount == 0, "missing player should execute zero plans");
	ExpectSessionMetadataUnchanged(result.session, original, "missing player should copy session unchanged");
	ExpectPlayerAgent(result.session.player, original.player, "missing player result session");
}

void TestPresentPlayerEmptyPlanResultReturnsNoExecutablePlan()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();

	const iggy::runtime::RuntimePlayerCommandExecutionResult result = iggy::runtime::RuntimePlayerCommandExecutionStep {}.execute(
		session,
		{},
		{},
		Config());

	Expect(result.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::NoExecutablePlan, "empty plan result should have no executable plan");
	Expect(result.movementResults.empty(), "empty plan result should execute no movement");
	Expect(result.executedPlanCount == 0, "empty plan result should execute zero plans");
	ExpectPlayerAgent(result.session.player, session.player, "empty plan result session");
}

void TestNonMovePlansReturnNoExecutablePlan()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();

	const iggy::runtime::RuntimePlayerCommandExecutionResult result = iggy::runtime::RuntimePlayerCommandExecutionStep {}.execute(
		session,
		PlanResult({
			Plan(iggy::PlayerCommandPlan2DType::None),
			Plan(iggy::PlayerCommandPlan2DType::Wait),
			Plan(iggy::PlayerCommandPlan2DType::Rejected),
			Plan(iggy::PlayerCommandPlan2DType::Interact),
		}),
		{},
		Config());

	Expect(result.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::NoExecutablePlan, "non-move plans should have no executable movement plan");
	Expect(result.movementResults.empty(), "non-move plans should execute no movement");
	ExpectPlayerAgent(result.session.player, session.player, "non-move plans session");
}

void TestFirstMoveToPointExecutesInEmptyWorld()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();

	const iggy::runtime::RuntimePlayerCommandExecutionResult result = iggy::runtime::RuntimePlayerCommandExecutionStep {}.execute(
		session,
		PlanResult({ MovePlan({ 4.0F, 0.0F }) }),
		{},
		Config(2.0F));

	Expect(result.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::Executed, "move plan should execute");
	Expect(result.movementResults.size() == 1, "move plan should produce one movement result");
	Expect(result.executedPlanCount == 1, "move plan should execute one plan");
	Expect(NearVec(result.session.player.position, { 2.0F, 0.0F }), "move plan should update returned session player");
	Expect(result.session.player.movementStatus == iggy::PlayerMovementStatus::Moving, "move plan should preserve movement executor state");
	Expect(result.session.hasPlayer, "executed move should preserve hasPlayer true");
}

void TestMultipleMovePlansExecuteOnlyFirst()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();

	const iggy::runtime::RuntimePlayerCommandExecutionResult result = iggy::runtime::RuntimePlayerCommandExecutionStep {}.execute(
		session,
		PlanResult({
			MovePlan({ 2.0F, 0.0F }),
			MovePlan({ 0.0F, 4.0F }),
		}),
		{},
		Config(1.0F));

	Expect(result.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::Executed, "first of multiple move plans should execute");
	Expect(result.executedPlanCount == 1, "multiple move plans should still execute one plan");
	Expect(result.movementResults.size() == 1, "multiple move plans should produce one movement result");
	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "multiple move plans should apply only first move target");
}

void TestMoveAfterSkippedPlansExecutes()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();

	const iggy::runtime::RuntimePlayerCommandExecutionResult result = iggy::runtime::RuntimePlayerCommandExecutionStep {}.execute(
		session,
		PlanResult({
			Plan(iggy::PlayerCommandPlan2DType::Wait),
			Plan(iggy::PlayerCommandPlan2DType::Rejected),
			Plan(iggy::PlayerCommandPlan2DType::Interact),
			MovePlan({ 0.0F, 3.0F }),
		}),
		{},
		Config(2.0F));

	Expect(result.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::Executed, "move after skipped plans should execute");
	Expect(result.executedPlanCount == 1, "move after skipped plans should execute one plan");
	Expect(NearVec(result.session.player.position, { 0.0F, 2.0F }), "move after skipped plans should apply first executable move");
}

void TestBlockedMoveReturnsExecutedWithBlockedMovementResult()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::physics2d::CollisionObject2D wall = Object(
		iggy::ResourceId("wall"),
		{ { 2.5F, -0.5F }, { 3.5F, 0.5F } });
	const iggy::physics2d::CollisionWorld2D world = World({ wall });

	const iggy::runtime::RuntimePlayerCommandExecutionResult result = iggy::runtime::RuntimePlayerCommandExecutionStep {}.execute(
		session,
		PlanResult({ MovePlan({ 8.0F, 0.0F }) }),
		world,
		Config(4.0F));

	Expect(result.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::Executed, "blocked move plan should still count as executed");
	Expect(result.movementResults.size() == 1, "blocked move should produce one movement result");
	if (result.movementResults.size() == 1) {
		Expect(result.movementResults[0].status == iggy::PlayerMovementExecutionStatus::Blocked, "blocked move should preserve PlayerMovementExecutor blocked status");
		Expect(NearVec(result.movementResults[0].state.position, { 2.0F, 0.0F }), "blocked move should preserve allowed player position");
		Expect(result.movementResults[0].movement.motion.hit.objectIndex == 0, "blocked move should preserve hit index");
	}
	Expect(NearVec(result.session.player.position, { 2.0F, 0.0F }), "blocked move should update session player from movement result");
}

void TestExecutionPreservesSessionMetadata()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	session.tickIndex = 42;
	const iggy::runtime::RuntimeSessionState original = session;

	const iggy::runtime::RuntimePlayerCommandExecutionResult result = iggy::runtime::RuntimePlayerCommandExecutionStep {}.execute(
		session,
		PlanResult({ MovePlan({ 1.0F, 0.0F }) }),
		{},
		Config(1.0F));

	Expect(result.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::Executed, "metadata preservation setup should execute");
	ExpectSessionMetadataUnchanged(result.session, original, "execution should preserve non-player session metadata");
	Expect(result.session.tickIndex == 42, "execution should not advance tickIndex");
	Expect(result.session.level.npcAgents.size() == original.level.npcAgents.size(), "execution should not update NPCs");
}

void TestInputSessionIsNotMutated()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::runtime::RuntimeSessionState original = session;

	const iggy::runtime::RuntimePlayerCommandExecutionResult result = iggy::runtime::RuntimePlayerCommandExecutionStep {}.execute(
		session,
		PlanResult({ MovePlan({ 2.0F, 0.0F }) }),
		{},
		Config(1.0F));

	Expect(result.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::Executed, "input session mutation setup should execute");
	ExpectSessionMetadataUnchanged(session, original, "execution should not mutate input session metadata");
	ExpectPlayerAgent(session.player, original.player, "execution input session");
}

void TestInputPlanResultIsNotMutated()
{
	iggy::PlayerCommandFramePlan2DResult planResult = PlanResult({
		Plan(iggy::PlayerCommandPlan2DType::Wait),
		MovePlan({ 2.0F, 0.0F }),
	});
	const iggy::PlayerCommandFramePlan2DResult original = planResult;

	const iggy::runtime::RuntimePlayerCommandExecutionResult result = iggy::runtime::RuntimePlayerCommandExecutionStep {}.execute(
		SessionWithPlayer(),
		planResult,
		{},
		Config(1.0F));

	Expect(result.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::Executed, "plan result mutation setup should execute");
	Expect(planResult.plans.size() == original.plans.size(), "execution should not mutate plan count");
	Expect(planResult.plans[0].type == original.plans[0].type, "execution should not mutate first plan");
	Expect(planResult.plans[1].type == original.plans[1].type && NearVec(planResult.plans[1].targetPoint, original.plans[1].targetPoint), "execution should not mutate move plan");
}

void TestWorldAndConfigAreNotMutated()
{
	const iggy::physics2d::CollisionObject2D wall = Object(
		iggy::ResourceId("wall"),
		{ { 5.0F, -1.0F }, { 6.0F, 1.0F } },
		false);
	const iggy::physics2d::CollisionWorld2D world = World({ wall });
	const std::vector<iggy::physics2d::CollisionObject2D> beforeObjects = world.objects();
	iggy::runtime::RuntimePlayerCommandExecutionConfig config = Config(2.0F);
	config.movement.bodySize = { 2.0F, 1.0F };
	config.movement.bodyAnchor = { 0.0F, 0.5F };
	const iggy::runtime::RuntimePlayerCommandExecutionConfig originalConfig = config;

	const iggy::runtime::RuntimePlayerCommandExecutionResult result = iggy::runtime::RuntimePlayerCommandExecutionStep {}.execute(
		SessionWithPlayer(),
		PlanResult({ MovePlan({ 2.0F, 0.0F }) }),
		world,
		config);

	Expect(result.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::Executed, "world/config mutation setup should execute");
	ExpectWorldUnchanged(world, beforeObjects, "execution should not mutate collision world");
	Expect(Near(config.movement.maxStep, originalConfig.movement.maxStep), "execution should not mutate config maxStep");
	Expect(NearVec(config.movement.bodySize, originalConfig.movement.bodySize), "execution should not mutate config bodySize");
	Expect(NearVec(config.movement.bodyAnchor, originalConfig.movement.bodyAnchor), "execution should not mutate config bodyAnchor");
}

void TestExecutionDoesNotRunRuntimeTick()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	session.tickIndex = 100;
	const iggy::Vec2 originalNpcPosition = session.level.npcAgents[0].state.position;

	const iggy::runtime::RuntimePlayerCommandExecutionResult result = iggy::runtime::RuntimePlayerCommandExecutionStep {}.execute(
		session,
		PlanResult({ MovePlan({ 1.0F, 0.0F }) }),
		{},
		Config(1.0F));

	Expect(result.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::Executed, "non-tick execution setup should execute");
	Expect(result.session.tickIndex == 100, "execution should not advance tickIndex");
	Expect(result.session.level.npcAgents.size() == 1, "execution should preserve NPC count");
	Expect(NearVec(result.session.level.npcAgents[0].state.position, originalNpcPosition), "execution should not update NPC position");
}

} // namespace

int main()
{
	TestMissingPlayerReturnsMissingPlayerAndDoesNotExecute();
	TestPresentPlayerEmptyPlanResultReturnsNoExecutablePlan();
	TestNonMovePlansReturnNoExecutablePlan();
	TestFirstMoveToPointExecutesInEmptyWorld();
	TestMultipleMovePlansExecuteOnlyFirst();
	TestMoveAfterSkippedPlansExecutes();
	TestBlockedMoveReturnsExecutedWithBlockedMovementResult();
	TestExecutionPreservesSessionMetadata();
	TestInputSessionIsNotMutated();
	TestInputPlanResultIsNotMutated();
	TestWorldAndConfigAreNotMutated();
	TestExecutionDoesNotRunRuntimeTick();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
