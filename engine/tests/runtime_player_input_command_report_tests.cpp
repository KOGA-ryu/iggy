#include <cstdlib>
#include <vector>

#include "runtime/RuntimePlayerInputCommandReporter.hpp"
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

const iggy::ResourceId PlayerId { "player:input-command-report" };
const iggy::ResourceId TargetId { "target:input-command-report" };

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
	session.level.map.id = iggy::ResourceId("level:input-command-report");
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
	session.level.map.id = iggy::ResourceId("level:input-command-report");
	session.tickIndex = 9;
	return session;
}

iggy::runtime::RuntimePlayerInputCommandRunnerInput Input(
	iggy::runtime::RuntimeSessionState session,
	iggy::runtime::RuntimeCommandQueueState queue,
	std::vector<iggy::PlayerInputIntent2D> intents,
	iggy::runtime::RuntimeCommandQueueConfig queueConfig = {})
{
	return {
		session,
		queue,
		queueConfig,
		PlayerId,
		intents,
		{ 1.5F, 1.5F },
		PlayerConfig(),
		NpcConfig(),
	};
}

iggy::runtime::GameplayCommandFrame2D MoveFrame(float x, float y)
{
	return CommandFrame({ iggy::runtime::GameplayCommand2DFactory {}.moveToPoint(PlayerId, { x, y }) });
}

iggy::runtime::RuntimeCommandQueueState Queue(std::vector<iggy::runtime::GameplayCommandFrame2D> frames)
{
	return { frames };
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

void TestEmptySuccessfulRunReportsQueuedFrameAndRunnerRan()
{
	const iggy::runtime::RuntimePlayerInputCommandRunnerResult result = iggy::runtime::RuntimePlayerInputCommandRunner {}.run(
		Input(SessionWithPlayer(), {}, {}));

	const iggy::runtime::RuntimePlayerInputCommandReport report = iggy::runtime::RuntimePlayerInputCommandReporter {}.report(result);

	Expect(report.status == result.status, "empty successful report should copy runner status");
	Expect(report.intake.status == result.intake.status, "empty successful report should copy intake status");
	Expect(report.acceptedCommandCount == 0, "empty successful report should have zero accepted commands");
	Expect(report.rejectedIntentCount == 0, "empty successful report should have zero rejected intents");
	Expect(report.queuedFrameCount == 1, "empty successful report should count queued empty frame");
	Expect(report.tickResultCount == 1, "empty successful report should count one tick result");
	Expect(SameEvents(report.events, {
		iggy::runtime::RuntimePlayerInputCommandEvent::IntakeQueued,
		iggy::runtime::RuntimePlayerInputCommandEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputCommandEvent::CommandRunnerRan,
	}), "empty successful report should emit queued and runner events");
}

void TestValidIntentsReportAcceptedCommandCountAndIntentMapped()
{
	const iggy::runtime::RuntimePlayerInputCommandRunnerResult result = iggy::runtime::RuntimePlayerInputCommandRunner {}.run(
		Input(SessionWithPlayer(), {}, {
			iggy::playerMoveToPointIntent({ 2.0F, 0.0F }),
			iggy::playerWaitIntent(),
		}));

	const iggy::runtime::RuntimePlayerInputCommandReport report = iggy::runtime::RuntimePlayerInputCommandReporter {}.report(result);

	Expect(report.acceptedCommandCount == 2, "valid report should count accepted commands");
	Expect(report.rejectedIntentCount == 0, "valid report should have no rejected intents");
	Expect(report.queuedFrameCount == 1, "valid report should count one queued frame");
	Expect(report.tickResultCount == 1, "valid report should count one tick result for one frame");
	Expect(SameEvents(report.events, {
		iggy::runtime::RuntimePlayerInputCommandEvent::IntentMapped,
		iggy::runtime::RuntimePlayerInputCommandEvent::IntakeQueued,
		iggy::runtime::RuntimePlayerInputCommandEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputCommandEvent::CommandRunnerRan,
	}), "valid report should emit mapped then queued runner events");
}

void TestMixedValidUnsupportedIntentsReportAcceptedRejectedCountsAndEvents()
{
	const iggy::runtime::RuntimePlayerInputCommandRunnerResult result = iggy::runtime::RuntimePlayerInputCommandRunner {}.run(
		Input(SessionWithPlayer(), {}, {
			iggy::playerMoveToPointIntent({ 2.0F, 0.0F }),
			iggy::playerInspectIntent(TargetId),
			iggy::playerCancelIntent(),
		}));

	const iggy::runtime::RuntimePlayerInputCommandReport report = iggy::runtime::RuntimePlayerInputCommandReporter {}.report(result);

	Expect(report.acceptedCommandCount == 1, "mixed report should count accepted command");
	Expect(report.rejectedIntentCount == 2, "mixed report should count rejected intents");
	Expect(report.queuedFrameCount == 1, "mixed report should count queued frame");
	Expect(report.tickResultCount == 1, "mixed report should count runner tick");
	Expect(report.intake.mapping.issues.size() == 2, "mixed report should preserve mapping issues");
	Expect(SameEvents(report.events, {
		iggy::runtime::RuntimePlayerInputCommandEvent::IntentMapped,
		iggy::runtime::RuntimePlayerInputCommandEvent::IntentRejected,
		iggy::runtime::RuntimePlayerInputCommandEvent::IntakeQueued,
		iggy::runtime::RuntimePlayerInputCommandEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputCommandEvent::CommandRunnerRan,
	}), "mixed report should emit mapped, rejected, queued, runner events");
}

void TestQueueRejectedReportsSkippedRunnerAndNoQueuedFrameOrTicks()
{
	const iggy::runtime::RuntimeCommandQueueState fullQueue = Queue({ MoveFrame(2.0F, 0.0F) });
	const iggy::runtime::RuntimePlayerInputCommandRunnerResult result = iggy::runtime::RuntimePlayerInputCommandRunner {}.run(
		Input(SessionWithPlayer(), fullQueue, { iggy::playerMoveToPointIntent({ 4.0F, 0.0F }) }, { 1 }));

	const iggy::runtime::RuntimePlayerInputCommandReport report = iggy::runtime::RuntimePlayerInputCommandReporter {}.report(result);

	Expect(report.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::QueueRejected, "queue rejected report should copy rejected status");
	Expect(report.intake.status == iggy::runtime::RuntimePlayerInputQueueStatus::RejectedFull, "queue rejected report should preserve intake rejection");
	Expect(report.acceptedCommandCount == 1, "queue rejected report should preserve mapping accepted count");
	Expect(report.rejectedIntentCount == 0, "queue rejected report should preserve zero mapping issues");
	Expect(report.queuedFrameCount == 0, "queue rejected report should not count queued frame");
	Expect(report.tickResultCount == 0, "queue rejected report should not count ticks");
	Expect(report.runner.runner.ticks.empty(), "queue rejected report should preserve skipped runner diagnostics");
	Expect(SameEvents(report.events, {
		iggy::runtime::RuntimePlayerInputCommandEvent::IntentMapped,
		iggy::runtime::RuntimePlayerInputCommandEvent::QueueRejected,
		iggy::runtime::RuntimePlayerInputCommandEvent::CommandRunnerSkipped,
	}), "queue rejected report should emit mapped, rejected, skipped events");
}

void TestMissingPlayerDiagnosticsArePreserved()
{
	const iggy::runtime::RuntimePlayerInputCommandRunnerResult result = iggy::runtime::RuntimePlayerInputCommandRunner {}.run(
		Input(SessionWithoutPlayer(), {}, { iggy::playerMoveToPointIntent({ 2.0F, 0.0F }) }));

	const iggy::runtime::RuntimePlayerInputCommandReport report = iggy::runtime::RuntimePlayerInputCommandReporter {}.report(result);

	Expect(report.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, "missing player report should still be a ran report");
	Expect(report.tickResultCount == 1, "missing player report should count underlying tick");
	Expect(report.runner.runner.ticks.size() == 1, "missing player report should preserve runner tick result");
	if (report.runner.runner.ticks.size() == 1) {
		Expect(report.runner.runner.ticks[0].playerCommands.planning.status == iggy::runtime::RuntimePlayerCommandPlanningStatus::MissingPlayer, "report should preserve missing-player planning diagnostics");
		Expect(report.runner.runner.ticks[0].playerCommands.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::MissingPlayer, "report should preserve missing-player execution diagnostics");
	}
}

void TestReporterDoesNotMutateInputResult()
{
	iggy::runtime::RuntimePlayerInputCommandRunnerResult result = iggy::runtime::RuntimePlayerInputCommandRunner {}.run(
		Input(SessionWithPlayer(), Queue({ MoveFrame(2.0F, 0.0F) }), {
			iggy::playerMoveToPointIntent({ 4.0F, 0.0F }),
			iggy::playerCancelIntent(),
		}));
	const iggy::runtime::RuntimePlayerInputCommandRunnerResult before = result;

	const iggy::runtime::RuntimePlayerInputCommandReport report = iggy::runtime::RuntimePlayerInputCommandReporter {}.report(result);

	Expect(report.acceptedCommandCount == 1, "immutability setup should produce accepted count");
	Expect(result.status == before.status, "reporter should not mutate runner status");
	Expect(result.intake.status == before.intake.status, "reporter should not mutate intake status");
	Expect(SameFrame(result.intake.mapping.frame, before.intake.mapping.frame), "reporter should not mutate intake mapping frame");
	Expect(result.intake.mapping.issues.size() == before.intake.mapping.issues.size(), "reporter should not mutate intake issues");
	Expect(result.runner.runner.ticks.size() == before.runner.runner.ticks.size(), "reporter should not mutate runner ticks");
	Expect(SameQueue(result.queue, before.queue), "reporter should not mutate result queue");
	Expect(result.session.tickIndex == before.session.tickIndex, "reporter should not mutate result session");
	ExpectPlayerAgent(result.session.player, before.session.player, "reported input result after reporter");
}

} // namespace

int main()
{
	TestEmptySuccessfulRunReportsQueuedFrameAndRunnerRan();
	TestValidIntentsReportAcceptedCommandCountAndIntentMapped();
	TestMixedValidUnsupportedIntentsReportAcceptedRejectedCountsAndEvents();
	TestQueueRejectedReportsSkippedRunnerAndNoQueuedFrameOrTicks();
	TestMissingPlayerDiagnosticsArePreserved();
	TestReporterDoesNotMutateInputResult();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
