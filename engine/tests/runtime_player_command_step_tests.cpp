#include <cstdlib>
#include <vector>

#include "runtime/RuntimePlayerCommandStep.hpp"
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
using iggy::test::Near;
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

const iggy::ResourceId PlayerId { "player:one" };
const iggy::ResourceId OtherActorId { "player:two" };
const iggy::ResourceId WalkableMaterial { "material:floor" };
const iggy::ResourceId BlockedMaterial { "material:wall" };

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

void TestMissingPlayerReturnsMissingPlayerWithoutExecution()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	session.hasPlayer = false;
	session.player = {};
	const iggy::runtime::RuntimeSessionState original = session;
	const iggy::runtime::GameplayCommandFrame2D frame = CommandFrame({
		factory.interact(PlayerId, {}),
		factory.moveToPoint(PlayerId, { 2.0F, 0.0F }),
	});

	const iggy::runtime::RuntimePlayerCommandStepResult result = iggy::runtime::RuntimePlayerCommandStep {}.run(
		session,
		frame,
		{},
		Config(2.0F));

	Expect(result.planning.status == iggy::runtime::RuntimePlayerCommandPlanningStatus::MissingPlayer, "missing player should preserve planning MissingPlayer");
	Expect(!result.planning.playerPlan.validation.hasInvalidCommands, "missing player should not validate command frame");
	Expect(result.planning.playerPlan.plans.empty(), "missing player should not produce plans");
	Expect(result.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::MissingPlayer, "missing player should synthesize execution MissingPlayer");
	Expect(result.execution.movementResults.empty(), "missing player should produce no movement results");
	Expect(result.execution.executedPlanCount == 0, "missing player should execute zero plans");
	ExpectSessionMetadataUnchanged(result.execution.session, original, "missing player execution session should remain unchanged");
	ExpectSessionMetadataUnchanged(result.session, original, "missing player result session should remain unchanged");
	ExpectPlayerAgent(result.session.player, original.player, "missing player result session");
}

void TestPresentPlayerEmptyFrameReturnsNoExecutablePlan()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::runtime::GameplayCommandFrame2D frame;

	const iggy::runtime::RuntimePlayerCommandStepResult result = iggy::runtime::RuntimePlayerCommandStep {}.run(
		session,
		frame,
		{},
		Config());

	Expect(result.planning.status == iggy::runtime::RuntimePlayerCommandPlanningStatus::Planned, "present player empty frame should plan");
	Expect(result.planning.playerPlan.plans.empty(), "empty frame should produce no plans");
	Expect(result.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::NoExecutablePlan, "empty frame should have no executable plan");
	Expect(result.execution.movementResults.empty(), "empty frame should produce no movement results");
	ExpectSessionMetadataUnchanged(result.session, session, "empty frame result session should preserve metadata");
	ExpectPlayerAgent(result.session.player, session.player, "empty frame result session");
}

void TestInvalidCommandDiagnosticsArePreserved()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::runtime::GameplayCommandFrame2D frame = CommandFrame({
		factory.interact(PlayerId, {}),
	});

	const iggy::runtime::RuntimePlayerCommandStepResult result = iggy::runtime::RuntimePlayerCommandStep {}.run(
		session,
		frame,
		{},
		Config());

	Expect(result.planning.status == iggy::runtime::RuntimePlayerCommandPlanningStatus::Planned, "invalid command frame should still plan for present player");
	Expect(result.planning.playerPlan.validation.hasInvalidCommands, "invalid command frame should preserve validation invalid flag");
	Expect(result.planning.playerPlan.validation.invalidCommands.size() == 1, "invalid command frame should preserve invalid diagnostics");
	if (result.planning.playerPlan.validation.invalidCommands.size() == 1) {
		Expect(result.planning.playerPlan.validation.invalidCommands[0].index == 0, "invalid diagnostic should preserve command index");
		Expect(result.planning.playerPlan.validation.invalidCommands[0].status == iggy::runtime::GameplayCommand2DStatus::MissingTarget, "invalid diagnostic should preserve MissingTarget status");
	}
	Expect(result.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::NoExecutablePlan, "invalid-only frame should have no executable plan");
	Expect(result.session.hasPlayer, "invalid-only frame should preserve player presence");
	ExpectPlayerAgent(result.session.player, session.player, "invalid-only frame result session");
}

void TestValidMoveToPointUpdatesReturnedSessionPlayer()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::runtime::GameplayCommandFrame2D frame = CommandFrame({
		factory.moveToPoint(PlayerId, { 4.0F, 0.0F }),
	});

	const iggy::runtime::RuntimePlayerCommandStepResult result = iggy::runtime::RuntimePlayerCommandStep {}.run(
		session,
		frame,
		{},
		Config(2.0F));

	Expect(result.planning.status == iggy::runtime::RuntimePlayerCommandPlanningStatus::Planned, "valid move should plan");
	Expect(result.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::Executed, "valid move should execute");
	Expect(result.execution.movementResults.size() == 1, "valid move should produce one movement result");
	Expect(result.execution.executedPlanCount == 1, "valid move should execute one plan");
	Expect(NearVec(result.session.player.position, { 2.0F, 0.0F }), "valid move should update returned session player");
	Expect(NearVec(result.execution.session.player.position, result.session.player.position), "result session should mirror execution session");
}

void TestActorMismatchPreservesRejectedPlanAndDoesNotExecute()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::runtime::GameplayCommandFrame2D frame = CommandFrame({
		factory.moveToPoint(OtherActorId, { 4.0F, 0.0F }),
	});

	const iggy::runtime::RuntimePlayerCommandStepResult result = iggy::runtime::RuntimePlayerCommandStep {}.run(
		session,
		frame,
		{},
		Config(2.0F));

	Expect(result.planning.status == iggy::runtime::RuntimePlayerCommandPlanningStatus::Planned, "actor mismatch should still run planning");
	Expect(result.planning.playerPlan.rejectedPlans.size() == 1, "actor mismatch should preserve rejected plan diagnostic");
	if (result.planning.playerPlan.rejectedPlans.size() == 1) {
		Expect(result.planning.playerPlan.rejectedPlans[0].originalCommandIndex == 0, "actor mismatch should preserve original command index");
		Expect(result.planning.playerPlan.rejectedPlans[0].plan.rejectReason == iggy::PlayerCommandPlan2DRejectReason::ActorMismatch, "actor mismatch should preserve reject reason");
	}
	Expect(result.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::NoExecutablePlan, "actor mismatch should not execute movement");
	Expect(result.execution.movementResults.empty(), "actor mismatch should produce no movement result");
	ExpectPlayerAgent(result.session.player, session.player, "actor mismatch result session");
}

void TestMultipleMoveCommandsExecuteOnlyFirst()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::runtime::GameplayCommandFrame2D frame = CommandFrame({
		factory.moveToPoint(PlayerId, { 2.0F, 0.0F }),
		factory.moveToPoint(PlayerId, { 0.0F, 4.0F }),
	});

	const iggy::runtime::RuntimePlayerCommandStepResult result = iggy::runtime::RuntimePlayerCommandStep {}.run(
		session,
		frame,
		{},
		Config(1.0F));

	Expect(result.planning.playerPlan.plans.size() == 2, "multiple moves should preserve both planned commands");
	Expect(result.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::Executed, "multiple moves should execute first move");
	Expect(result.execution.executedPlanCount == 1, "multiple moves should execute exactly one plan");
	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "multiple moves should apply first movement policy");
}

void TestBlockedMovePreservesMovementDiagnostics()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::physics2d::CollisionObject2D wall = Object(
		iggy::ResourceId("wall"),
		{ { 2.5F, -0.5F }, { 3.5F, 0.5F } });
	const iggy::physics2d::CollisionWorld2D world = World({ wall });
	const iggy::runtime::GameplayCommandFrame2D frame = CommandFrame({
		factory.moveToPoint(PlayerId, { 8.0F, 0.0F }),
	});

	const iggy::runtime::RuntimePlayerCommandStepResult result = iggy::runtime::RuntimePlayerCommandStep {}.run(
		session,
		frame,
		world,
		Config(4.0F));

	Expect(result.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::Executed, "blocked move should still execute the movement plan");
	Expect(result.execution.movementResults.size() == 1, "blocked move should preserve one movement diagnostic");
	if (result.execution.movementResults.size() == 1) {
		Expect(result.execution.movementResults[0].status == iggy::PlayerMovementExecutionStatus::Blocked, "blocked move should preserve blocked movement status");
		Expect(result.execution.movementResults[0].movement.motion.hit.objectIndex == 0, "blocked move should preserve hit object index");
		Expect(NearVec(result.execution.movementResults[0].state.position, { 2.0F, 0.0F }), "blocked move should preserve allowed player position");
	}
	Expect(NearVec(result.session.player.position, { 2.0F, 0.0F }), "blocked move should update returned session from execution result");
}

void TestSessionMetadataIsPreserved()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	session.tickIndex = 42;
	const iggy::runtime::RuntimeSessionState original = session;
	const iggy::runtime::GameplayCommandFrame2D frame = CommandFrame({
		factory.moveToPoint(PlayerId, { 1.0F, 0.0F }),
	});

	const iggy::runtime::RuntimePlayerCommandStepResult result = iggy::runtime::RuntimePlayerCommandStep {}.run(
		session,
		frame,
		{},
		Config(1.0F));

	Expect(result.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::Executed, "metadata preservation setup should execute");
	ExpectSessionMetadataUnchanged(result.session, original, "command step should preserve session metadata");
	Expect(result.session.tickIndex == 42, "command step should not advance tickIndex");
	Expect(result.session.level.npcAgents.size() == original.level.npcAgents.size(), "command step should not update NPCs");
}

void TestInputsAreNotMutated()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::runtime::RuntimeSessionState originalSession = session;
	iggy::runtime::GameplayCommandFrame2D frame = CommandFrame({
		factory.wait(PlayerId),
		factory.moveToPoint(PlayerId, { 2.0F, 0.0F }),
	});
	const iggy::runtime::GameplayCommandFrame2D originalFrame = frame;
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

	const iggy::runtime::RuntimePlayerCommandStepResult result = iggy::runtime::RuntimePlayerCommandStep {}.run(
		session,
		frame,
		world,
		config);

	Expect(result.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::Executed, "input mutation setup should execute");
	ExpectSessionMetadataUnchanged(session, originalSession, "command step should not mutate input session metadata");
	ExpectPlayerAgent(session.player, originalSession.player, "command step input session");
	Expect(frame.commands.size() == originalFrame.commands.size(), "command step should not mutate command frame count");
	Expect(frame.commands[0].type == originalFrame.commands[0].type, "command step should not mutate first command");
	Expect(frame.commands[1].type == originalFrame.commands[1].type && NearVec(frame.commands[1].targetPoint, originalFrame.commands[1].targetPoint), "command step should not mutate move command");
	ExpectWorldUnchanged(world, beforeObjects, "command step should not mutate collision world");
	Expect(Near(config.movement.maxStep, originalConfig.movement.maxStep), "command step should not mutate config maxStep");
	Expect(NearVec(config.movement.bodySize, originalConfig.movement.bodySize), "command step should not mutate config bodySize");
	Expect(NearVec(config.movement.bodyAnchor, originalConfig.movement.bodyAnchor), "command step should not mutate config bodyAnchor");
}

} // namespace

int main()
{
	TestMissingPlayerReturnsMissingPlayerWithoutExecution();
	TestPresentPlayerEmptyFrameReturnsNoExecutablePlan();
	TestInvalidCommandDiagnosticsArePreserved();
	TestValidMoveToPointUpdatesReturnedSessionPlayer();
	TestActorMismatchPreservesRejectedPlanAndDoesNotExecute();
	TestMultipleMoveCommandsExecuteOnlyFirst();
	TestBlockedMovePreservesMovementDiagnostics();
	TestSessionMetadataIsPreserved();
	TestInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
