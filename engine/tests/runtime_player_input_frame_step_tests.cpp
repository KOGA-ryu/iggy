#include <cstdlib>
#include <vector>

#include "runtime/RuntimePlayerInputFrameStep.hpp"
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

const iggy::ResourceId PlayerId { "player:input-frame-step" };
const iggy::ResourceId TargetId { "target:input-frame-step" };

iggy::runtime::RuntimePlayerCommandExecutionConfig PlayerConfig()
{
	iggy::runtime::RuntimePlayerCommandExecutionConfig config;
	config.movement.maxStep = 1.0F;
	return config;
}

iggy::npc_ai::NpcAgentTickConfig NpcConfig()
{
	iggy::npc_ai::NpcAgentTickConfig config;
	config.maxDistance = 0.25F;
	config.awareness = { 8.0F, 0 };
	return config;
}

iggy::runtime::RuntimeSessionState SessionWithPlayer(iggy::Vec2 position = { 0.0F, 0.0F })
{
	iggy::runtime::RuntimeSessionState session;
	session.level.map = MapFromRows({ "..", ".." });
	session.level.map.id = iggy::ResourceId("level:input-frame-step");
	session.level.map.playerStart = { 0, 0 };
	session.tickIndex = 4;
	session.hasPlayer = true;
	session.player = PlayerAgent(PlayerId, position, { 0, 0 }, iggy::PlayerMovementStatus::Idle, iggy::PlayerFacing2D::East);
	return session;
}

iggy::runtime::RuntimeSessionState SessionWithoutPlayer()
{
	iggy::runtime::RuntimeSessionState session;
	session.level.map = MapFromRows({ "..", ".." });
	session.level.map.id = iggy::ResourceId("level:input-frame-step");
	session.tickIndex = 9;
	return session;
}

iggy::runtime::GameplayCommandFrame2D MoveFrame(float x, float y)
{
	return CommandFrame({ iggy::runtime::GameplayCommand2DFactory {}.moveToPoint(PlayerId, { x, y }) });
}

iggy::runtime::RuntimeCommandQueueState Queue(std::vector<iggy::runtime::GameplayCommandFrame2D> frames)
{
	return { frames };
}

iggy::runtime::RuntimePlayerInputFrameStepInput Input(
	iggy::runtime::RuntimeSessionState session,
	iggy::runtime::RuntimeCommandQueueState queue,
	std::vector<iggy::PlayerInputIntent2D> intents,
	iggy::runtime::RuntimeCommandQueueConfig queueConfig = {})
{
	return {
		{
			session,
			queue,
			queueConfig,
			PlayerId,
			intents,
			{ 1.5F, 1.5F },
			PlayerConfig(),
			NpcConfig(),
		},
	};
}

iggy::physics2d::CollisionObject2D Object(iggy::ResourceId id, iggy::Aabb2 bounds)
{
	return { id, iggy::physics2d::makeAabbShape(bounds), true };
}

iggy::physics2d::CollisionWorld2D World(std::vector<iggy::physics2d::CollisionObject2D> objects)
{
	const iggy::physics2d::CollisionWorldBuildResult build = iggy::physics2d::CollisionWorld2DBuilder {}.build(objects);
	Expect(build.built, "player input frame step collision fixture should build");
	return build.world;
}

iggy::physics2d::CollisionWorld2D BlockingWorld()
{
	return World({
		Object(iggy::ResourceId("wall:east"), { { 0.5F, -0.5F }, { 1.5F, 0.5F } }),
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

bool SameEvents(
	const std::vector<iggy::runtime::RuntimePlayerInputCommandEvent> &actual,
	const std::vector<iggy::runtime::RuntimePlayerInputCommandEvent> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (actual[index] != expected[index])
			return false;
	}
	return true;
}

void ExpectIntentEquals(const iggy::PlayerInputIntent2D &actual, const iggy::PlayerInputIntent2D &expected, const char *message)
{
	Expect(actual.type == expected.type, message);
	Expect(NearVec(actual.worldPoint, expected.worldPoint), message);
	Expect(actual.tile == expected.tile, message);
	Expect(actual.targetId == expected.targetId, message);
}

void TestEmptyInputBatchReturnsCommandAndReport()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::runtime::RuntimePlayerInputFrameStepResult result = iggy::runtime::RuntimePlayerInputFrameStep {}.run(Input(session, {}, {}));

	Expect(result.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, "empty frame step should run command runner");
	Expect(result.command.intake.mapping.frame.commands.empty(), "empty frame step should preserve empty mapped frame");
	Expect(result.report.acceptedCommandCount == 0, "empty frame report should have zero accepted commands");
	Expect(result.report.rejectedIntentCount == 0, "empty frame report should have zero rejected intents");
	Expect(result.report.queuedFrameCount == 1, "empty frame report should count queued frame");
	Expect(result.report.tickResultCount == 1, "empty frame report should count runner tick");
	Expect(SameEvents(result.report.events, {
		iggy::runtime::RuntimePlayerInputCommandEvent::IntakeQueued,
		iggy::runtime::RuntimePlayerInputCommandEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputCommandEvent::CommandRunnerRan,
	}), "empty frame step should preserve queued frame and runner ran events");
	Expect(result.session.tickIndex == session.tickIndex + 1, "empty frame step should use runner session");
	Expect(result.queue.frames.empty(), "empty frame step should use runner queue");
}

void TestValidMoveToPointUpdatesSessionAndReport()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const iggy::runtime::RuntimePlayerInputFrameStepResult result = iggy::runtime::RuntimePlayerInputFrameStep {}.run(
		Input(session, {}, { iggy::playerMoveToPointIntent({ 2.0F, 0.0F }) }));

	Expect(result.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, "valid move frame step should run");
	Expect(result.report.acceptedCommandCount == 1, "valid move report should count accepted command");
	Expect(result.report.rejectedIntentCount == 0, "valid move report should have no rejected intents");
	Expect(result.report.tickResultCount == 1, "valid move report should count one tick");
	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "valid move frame step should update player through runner");
	Expect(result.session.tickIndex == session.tickIndex + 1, "valid move frame step should advance through runner");
	Expect(SameQueue(result.queue, result.command.queue), "frame step queue should come from command result");
}

void TestMixedValidUnsupportedIntentsPreserveIssuesAndReportCounts()
{
	const iggy::PlayerInputIntent2D validMove = iggy::playerMoveToPointIntent({ 2.0F, 0.0F });
	const iggy::PlayerInputIntent2D unsupportedInspect = iggy::playerInspectIntent(TargetId);
	const iggy::PlayerInputIntent2D unsupportedCancel = iggy::playerCancelIntent();
	const iggy::runtime::RuntimePlayerInputFrameStepResult result = iggy::runtime::RuntimePlayerInputFrameStep {}.run(
		Input(SessionWithPlayer(), {}, { validMove, unsupportedInspect, unsupportedCancel }));

	Expect(result.command.intake.mapping.frame.commands.size() == 1, "mixed frame step should preserve accepted command");
	Expect(result.command.intake.mapping.issues.size() == 2, "mixed frame step should preserve intake issues");
	Expect(result.report.acceptedCommandCount == 1, "mixed report should count accepted command");
	Expect(result.report.rejectedIntentCount == 2, "mixed report should count rejected intents");
	Expect(SameEvents(result.report.events, {
		iggy::runtime::RuntimePlayerInputCommandEvent::IntentMapped,
		iggy::runtime::RuntimePlayerInputCommandEvent::IntentRejected,
		iggy::runtime::RuntimePlayerInputCommandEvent::IntakeQueued,
		iggy::runtime::RuntimePlayerInputCommandEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputCommandEvent::CommandRunnerRan,
	}), "mixed frame step should preserve mapped/rejected/report events");
	Expect(result.command.intake.mapping.issues[0].intentIndex == 1, "mixed frame step should preserve unsupported inspect index");
	ExpectIntentEquals(result.command.intake.mapping.issues[0].intent, unsupportedInspect, "mixed frame step should preserve unsupported inspect intent");
	Expect(result.command.intake.mapping.issues[1].intentIndex == 2, "mixed frame step should preserve unsupported cancel index");
	ExpectIntentEquals(result.command.intake.mapping.issues[1].intent, unsupportedCancel, "mixed frame step should preserve unsupported cancel intent");
}

void TestBoundedFullQueueReturnsUnchangedStateAndRejectedReport()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::runtime::RuntimeCommandQueueState queue = Queue({ MoveFrame(2.0F, 0.0F) });
	const iggy::runtime::RuntimePlayerInputFrameStepResult result = iggy::runtime::RuntimePlayerInputFrameStep {}.run(
		Input(session, queue, { iggy::playerMoveToPointIntent({ 4.0F, 0.0F }) }, { 1 }));

	Expect(result.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::QueueRejected, "full queue should reject frame step intake");
	Expect(result.command.intake.status == iggy::runtime::RuntimePlayerInputQueueStatus::RejectedFull, "full queue should preserve intake rejection");
	Expect(result.report.queuedFrameCount == 0, "full queue report should not count queued frame");
	Expect(result.report.tickResultCount == 0, "full queue report should not count ticks");
	Expect(SameEvents(result.report.events, {
		iggy::runtime::RuntimePlayerInputCommandEvent::IntentMapped,
		iggy::runtime::RuntimePlayerInputCommandEvent::QueueRejected,
		iggy::runtime::RuntimePlayerInputCommandEvent::CommandRunnerSkipped,
	}), "full queue frame step should report rejected and skipped events");
	Expect(result.session.tickIndex == session.tickIndex, "full queue frame step should not advance session");
	ExpectPlayerAgent(result.session.player, session.player, "full queue frame step result");
	Expect(SameQueue(result.queue, queue), "full queue frame step should preserve queue");
}

void TestExplicitWorldOverloadPreservesExplicitCollisionPrecedence()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	SetCollisionCache(session, BlockingWorld());
	const iggy::physics2d::CollisionWorld2D emptyWorld;

	const iggy::runtime::RuntimePlayerInputFrameStepResult result = iggy::runtime::RuntimePlayerInputFrameStep {}.run(
		Input(session, {}, { iggy::playerMoveToPointIntent({ 2.0F, 0.0F }) }),
		emptyWorld);

	Expect(result.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, "explicit world frame step should run");
	Expect(result.command.runner.runner.ticks.size() == 1, "explicit world frame step should produce one tick");
	if (result.command.runner.runner.ticks.size() == 1) {
		Expect(result.command.runner.runner.ticks[0].playerCommands.execution.movementResults.size() == 1, "explicit world frame step should preserve movement diagnostics");
		Expect(result.command.runner.runner.ticks[0].playerCommands.execution.movementResults[0].status == iggy::PlayerMovementExecutionStatus::Moved, "explicit empty world should override session collision cache");
	}
	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "explicit empty world should allow movement");
	Expect(result.report.acceptedCommandCount == 1, "explicit world report should count accepted command");
}

void TestNoExplicitOverloadUsesSessionDerivedCollisionCache()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	SetCollisionCache(session, BlockingWorld());

	const iggy::runtime::RuntimePlayerInputFrameStepResult result = iggy::runtime::RuntimePlayerInputFrameStep {}.run(
		Input(session, {}, { iggy::playerMoveToPointIntent({ 2.0F, 0.0F }) }));

	Expect(result.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, "session collision frame step should run");
	Expect(result.command.runner.runner.ticks.size() == 1, "session collision frame step should produce one tick");
	if (result.command.runner.runner.ticks.size() == 1) {
		Expect(result.command.runner.runner.ticks[0].playerCommands.execution.movementResults.size() == 1, "session collision frame step should preserve movement diagnostics");
		Expect(result.command.runner.runner.ticks[0].playerCommands.execution.movementResults[0].status == iggy::PlayerMovementExecutionStatus::Blocked, "session collision cache should block movement");
	}
	Expect(NearVec(result.session.player.position, { 0.0F, 0.0F }), "session collision cache should prevent movement");
	Expect(result.report.tickResultCount == 1, "session collision report should count runner tick");
}

void TestMissingPlayerDiagnosticsArePreserved()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithoutPlayer();
	const iggy::runtime::RuntimePlayerInputFrameStepResult result = iggy::runtime::RuntimePlayerInputFrameStep {}.run(
		Input(session, {}, { iggy::playerMoveToPointIntent({ 2.0F, 0.0F }) }));

	Expect(result.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, "missing player frame step should run queued runner");
	Expect(result.command.runner.runner.ticks.size() == 1, "missing player frame step should preserve runner tick");
	if (result.command.runner.runner.ticks.size() == 1) {
		Expect(result.command.runner.runner.ticks[0].playerCommands.planning.status == iggy::runtime::RuntimePlayerCommandPlanningStatus::MissingPlayer, "missing player frame step should preserve planning diagnostics");
		Expect(result.command.runner.runner.ticks[0].playerCommands.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::MissingPlayer, "missing player frame step should preserve execution diagnostics");
	}
	Expect(result.report.runner.runner.ticks.size() == result.command.runner.runner.ticks.size(), "missing player report should copy runner diagnostics");
	Expect(result.report.tickResultCount == 1, "missing player report should count tick");
	Expect(!result.session.hasPlayer, "missing player frame step should not invent player");
}

void TestInputsAreNotMutated()
{
	iggy::runtime::RuntimePlayerInputFrameStepInput input = Input(
		SessionWithPlayer({ 0.0F, 0.0F }),
		Queue({ MoveFrame(2.0F, 0.0F) }),
		{ iggy::playerMoveToPointIntent({ 4.0F, 0.0F }), iggy::playerCancelIntent() },
		{ 3 });
	const iggy::runtime::RuntimePlayerInputFrameStepInput before = input;

	const iggy::runtime::RuntimePlayerInputFrameStepResult result = iggy::runtime::RuntimePlayerInputFrameStep {}.run(input);

	Expect(result.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, "immutability setup should run");
	Expect(input.commandInput.session.tickIndex == before.commandInput.session.tickIndex, "frame step should not mutate input session tickIndex");
	ExpectPlayerAgent(input.commandInput.session.player, before.commandInput.session.player, "input frame step session after run");
	Expect(SameQueue(input.commandInput.queue, before.commandInput.queue), "frame step should not mutate input queue");
	Expect(input.commandInput.queueConfig.maxFrames == before.commandInput.queueConfig.maxFrames, "frame step should not mutate queue config");
	Expect(input.commandInput.actorId == before.commandInput.actorId, "frame step should not mutate actor id");
	Expect(input.commandInput.intents.size() == before.commandInput.intents.size(), "frame step should not mutate intents");
	for (std::size_t index = 0; index < input.commandInput.intents.size(); ++index) {
		ExpectIntentEquals(input.commandInput.intents[index], before.commandInput.intents[index], "frame step should not mutate intent data");
	}
}

} // namespace

int main()
{
	TestEmptyInputBatchReturnsCommandAndReport();
	TestValidMoveToPointUpdatesSessionAndReport();
	TestMixedValidUnsupportedIntentsPreserveIssuesAndReportCounts();
	TestBoundedFullQueueReturnsUnchangedStateAndRejectedReport();
	TestExplicitWorldOverloadPreservesExplicitCollisionPrecedence();
	TestNoExplicitOverloadUsesSessionDerivedCollisionCache();
	TestMissingPlayerDiagnosticsArePreserved();
	TestInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
