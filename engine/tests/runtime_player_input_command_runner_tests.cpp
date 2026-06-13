#include <cstdlib>
#include <vector>

#include "runtime/RuntimePlayerInputCommandRunner.hpp"
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

const iggy::ResourceId PlayerId { "player:input-command-runner" };
const iggy::ResourceId TargetId { "target:input-command-runner" };

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

iggy::runtime::RuntimeSessionState SessionWithPlayer(iggy::Vec2 position = { 0.0F, 0.0F })
{
	iggy::runtime::RuntimeSessionState session;
	session.level.map = MapFromRows({ "..", ".." });
	session.level.map.id = iggy::ResourceId("level:input-command-runner");
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
	session.level.map.id = iggy::ResourceId("level:input-command-runner");
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

iggy::runtime::RuntimePlayerInputGatedCommandRunnerInput GatedInput(
	iggy::runtime::RuntimeSessionState session,
	iggy::runtime::RuntimeCommandQueueState queue,
	iggy::PlayerInputContext2D context,
	std::vector<iggy::PlayerInputIntent2D> intents,
	iggy::runtime::RuntimeCommandQueueConfig queueConfig = {})
{
	return {
		session,
		queue,
		queueConfig,
		PlayerId,
		context,
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

iggy::physics2d::CollisionObject2D Object(iggy::ResourceId id, iggy::Aabb2 bounds)
{
	return { id, iggy::physics2d::makeAabbShape(bounds), true };
}

iggy::physics2d::CollisionWorld2D World(std::vector<iggy::physics2d::CollisionObject2D> objects)
{
	const iggy::physics2d::CollisionWorldBuildResult build = iggy::physics2d::CollisionWorld2DBuilder {}.build(objects);
	Expect(build.built, "player input command runner collision fixture should build");
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

void ExpectIntentEquals(const iggy::PlayerInputIntent2D &actual, const iggy::PlayerInputIntent2D &expected, const char *message)
{
	Expect(actual.type == expected.type, message);
	Expect(NearVec(actual.worldPoint, expected.worldPoint), message);
	Expect(actual.tile == expected.tile, message);
	Expect(actual.targetId == expected.targetId, message);
}

void TestEmptyIntentsRunOneQueuedEmptyFrame()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const iggy::runtime::RuntimePlayerInputCommandRunnerResult result = iggy::runtime::RuntimePlayerInputCommandRunner {}.run(Input(session, {}, {}));

	Expect(result.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, "empty intents should run after intake");
	Expect(result.intake.status == iggy::runtime::RuntimePlayerInputQueueStatus::Queued, "empty intents should be queued as an empty frame");
	Expect(result.intake.mapping.frame.commands.empty(), "empty intents should map to an empty command frame");
	Expect(result.runner.drained.frames.size() == 1, "queued runner should drain the queued empty frame");
	Expect(result.runner.drained.frames[0].commands.empty(), "drained empty input frame should have no commands");
	Expect(result.runner.runner.ticks.size() == 1, "empty queued frame should still run one command tick");
	Expect(result.queue.frames.empty(), "runner should empty the queue after draining");
	Expect(result.session.tickIndex == session.tickIndex + 1, "empty queued frame should advance through session command tick");
	ExpectPlayerAgent(result.session.player, session.player, "empty input command runner result");
}

void TestValidMoveToPointIntentMovesPlayerThroughQueuedRunner()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const iggy::runtime::RuntimePlayerInputCommandRunnerResult result = iggy::runtime::RuntimePlayerInputCommandRunner {}.run(
		Input(session, {}, { iggy::playerMoveToPointIntent({ 2.0F, 0.0F }) }));

	Expect(result.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, "valid intent should run");
	Expect(result.intake.mapping.issues.empty(), "valid intent should have no mapping issues");
	Expect(result.runner.runner.ticks.size() == 1, "valid intent should produce one runner tick");
	Expect(result.session.tickIndex == session.tickIndex + 1, "valid intent should advance tickIndex once");
	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "valid move intent should move player through queued runner");
	Expect(result.queue.frames.empty(), "valid intent runner should empty queue");
}

void TestMixedValidUnsupportedIntentsPreserveIssuesAndExecuteAcceptedCommands()
{
	const iggy::PlayerInputIntent2D validMove = iggy::playerMoveToPointIntent({ 2.0F, 0.0F });
	const iggy::PlayerInputIntent2D unsupportedInspect = iggy::playerInspectIntent(TargetId);
	const iggy::PlayerInputIntent2D unsupportedCancel = iggy::playerCancelIntent();
	const iggy::runtime::RuntimePlayerInputCommandRunnerResult result = iggy::runtime::RuntimePlayerInputCommandRunner {}.run(
		Input(SessionWithPlayer({ 0.0F, 0.0F }), {}, { validMove, unsupportedInspect, unsupportedCancel }));

	Expect(result.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, "mixed intents should run accepted commands");
	Expect(result.intake.mapping.frame.commands.size() == 1, "mixed intents should queue only accepted commands");
	Expect(result.intake.mapping.frame.commands[0].type == iggy::runtime::GameplayCommand2DType::MoveToPoint, "mixed intents should preserve accepted command");
	Expect(result.intake.mapping.issues.size() == 2, "unsupported intents should be preserved as mapping issues");
	Expect(result.intake.mapping.issues[0].intentIndex == 1, "unsupported inspect should preserve original index");
	Expect(result.intake.mapping.issues[0].map.status == iggy::PlayerInputCommandMapper2DStatus::UnsupportedIntent, "unsupported inspect should preserve mapper status");
	ExpectIntentEquals(result.intake.mapping.issues[0].intent, unsupportedInspect, "unsupported inspect should preserve original intent");
	Expect(result.intake.mapping.issues[1].intentIndex == 2, "unsupported cancel should preserve original index");
	Expect(result.intake.mapping.issues[1].map.status == iggy::PlayerInputCommandMapper2DStatus::UnsupportedIntent, "unsupported cancel should preserve mapper status");
	ExpectIntentEquals(result.intake.mapping.issues[1].intent, unsupportedCancel, "unsupported cancel should preserve original intent");
	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "accepted move should execute despite unsupported issues");
}

void TestExistingQueuedFramesRunBeforeNewInputFrame()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const iggy::runtime::GameplayCommandFrame2D existing = MoveFrame(2.0F, 0.0F);
	const iggy::runtime::RuntimePlayerInputCommandRunnerResult result = iggy::runtime::RuntimePlayerInputCommandRunner {}.run(
		Input(session, Queue({ existing }), { iggy::playerMoveToPointIntent({ 4.0F, 0.0F }) }));

	Expect(result.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, "existing queue plus input should run");
	Expect(result.runner.drained.frames.size() == 2, "runner should drain existing frame and appended input frame");
	if (result.runner.drained.frames.size() == 2) {
		Expect(SameFrame(result.runner.drained.frames[0], existing), "existing queued frame should drain first");
		Expect(SameFrame(result.runner.drained.frames[1], result.intake.mapping.frame), "new input frame should drain after existing frames");
	}
	Expect(result.runner.runner.ticks.size() == 2, "two drained frames should run two ticks");
	if (result.runner.runner.ticks.size() == 2) {
		Expect(NearVec(result.runner.runner.ticks[0].session.player.position, { 1.0F, 0.0F }), "existing frame should execute first");
		Expect(NearVec(result.runner.runner.ticks[1].session.player.position, { 2.0F, 0.0F }), "new input frame should execute second");
	}
	Expect(result.session.tickIndex == session.tickIndex + 2, "two frames should advance tickIndex twice");
	Expect(NearVec(result.session.player.position, { 2.0F, 0.0F }), "final session should come from queued runner");
}

void TestBoundedFullQueueRejectsWithoutRunning()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const iggy::runtime::RuntimeCommandQueueState queue = Queue({ MoveFrame(2.0F, 0.0F) });
	const iggy::runtime::RuntimePlayerInputCommandRunnerResult result = iggy::runtime::RuntimePlayerInputCommandRunner {}.run(
		Input(session, queue, { iggy::playerMoveToPointIntent({ 4.0F, 0.0F }) }, { 1 }));

	Expect(result.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::QueueRejected, "full queue should reject intake");
	Expect(result.intake.status == iggy::runtime::RuntimePlayerInputQueueStatus::RejectedFull, "intake should preserve rejected status");
	Expect(result.intake.mapping.frame.commands.size() == 1, "mapping should still run before queue rejection");
	Expect(result.runner.runner.ticks.empty(), "rejected intake should not call queued command runner");
	Expect(SameQueue(result.queue, queue), "rejected intake should return unchanged queue");
	Expect(result.session.tickIndex == session.tickIndex, "rejected intake should not advance tickIndex");
	ExpectPlayerAgent(result.session.player, session.player, "queue rejected input command runner result");
}

void TestExplicitWorldOverridesSessionCollisionCache()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	SetCollisionCache(session, BlockingWorld());
	const iggy::physics2d::CollisionWorld2D emptyWorld;

	const iggy::runtime::RuntimePlayerInputCommandRunnerResult result = iggy::runtime::RuntimePlayerInputCommandRunner {}.run(
		Input(session, {}, { iggy::playerMoveToPointIntent({ 2.0F, 0.0F }) }),
		emptyWorld);

	Expect(result.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, "explicit world run should succeed");
	Expect(result.runner.runner.ticks.size() == 1, "explicit world run should produce one tick");
	if (result.runner.runner.ticks.size() == 1) {
		Expect(result.runner.runner.ticks[0].playerCommands.execution.movementResults.size() == 1, "explicit world movement should preserve diagnostics");
		Expect(result.runner.runner.ticks[0].playerCommands.execution.movementResults[0].status == iggy::PlayerMovementExecutionStatus::Moved, "explicit empty world should override blocking session cache");
	}
	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "explicit empty world should allow movement");
}

void TestNoExplicitOverloadUsesSessionDerivedCollisionCache()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	SetCollisionCache(session, BlockingWorld());

	const iggy::runtime::RuntimePlayerInputCommandRunnerResult result = iggy::runtime::RuntimePlayerInputCommandRunner {}.run(
		Input(session, {}, { iggy::playerMoveToPointIntent({ 2.0F, 0.0F }) }));

	Expect(result.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, "session collision cache run should succeed");
	Expect(result.runner.runner.ticks.size() == 1, "session collision cache run should produce one tick");
	if (result.runner.runner.ticks.size() == 1) {
		Expect(result.runner.runner.ticks[0].playerCommands.execution.movementResults.size() == 1, "blocked movement should preserve diagnostics");
		Expect(result.runner.runner.ticks[0].playerCommands.execution.movementResults[0].status == iggy::PlayerMovementExecutionStatus::Blocked, "session collision cache should block movement");
	}
	Expect(NearVec(result.session.player.position, { 0.0F, 0.0F }), "session collision cache should keep player from moving");
}

void TestDefaultContextGatedRunnerBehavesLikeUngatedForSupportedMovement()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const std::vector<iggy::PlayerInputIntent2D> intents {
		iggy::playerMoveToPointIntent({ 2.0F, 0.0F }),
	};
	const iggy::runtime::RuntimePlayerInputCommandRunnerResult ungated = iggy::runtime::RuntimePlayerInputCommandRunner {}.run(
		Input(session, {}, intents));
	const iggy::runtime::RuntimePlayerInputGatedCommandRunnerResult gated = iggy::runtime::RuntimePlayerInputCommandRunner {}.runGated(
		GatedInput(session, {}, {}, intents));

	Expect(gated.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, "default gated supported movement should run");
	Expect(gated.intake.status == iggy::runtime::RuntimePlayerInputQueueStatus::Queued, "default gated movement should queue");
	Expect(gated.intake.mapping.gateIssues.empty(), "default gated movement should have no gate issues");
	Expect(gated.intake.mapping.mapping.issues.empty(), "default gated movement should have no nested mapping issues");
	Expect(gated.intake.mapping.frame.commands.size() == ungated.intake.mapping.frame.commands.size(), "default gated movement should map like ungated movement");
	Expect(gated.runner.runner.ticks.size() == 1, "default gated movement should tick once");
	Expect(gated.session.tickIndex == ungated.session.tickIndex, "default gated movement should match ungated tick index");
	Expect(NearVec(gated.session.player.position, ungated.session.player.position), "default gated movement should match ungated movement");
	Expect(gated.queue.frames.empty(), "default gated movement should drain queue");
}

void TestGatedContextBlockedMovementQueuesEmptyFrameAndDoesNotMove()
{
	iggy::PlayerInputContext2D context;
	context.worldInputEnabled = false;
	const iggy::PlayerInputIntent2D blockedMove = iggy::playerMoveToPointIntent({ 2.0F, 0.0F });
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });

	const iggy::runtime::RuntimePlayerInputGatedCommandRunnerResult result = iggy::runtime::RuntimePlayerInputCommandRunner {}.runGated(
		GatedInput(session, {}, context, { blockedMove }));

	Expect(result.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, "gated blocked movement should still run queued runner");
	Expect(result.intake.mapping.gateIssues.size() == 1, "gated blocked movement should report gate issue");
	Expect(result.intake.mapping.gateIssues[0].intentIndex == 0, "gated blocked movement should preserve original index");
	Expect(result.intake.mapping.gateIssues[0].gate.reason == iggy::PlayerInputIntentBlockReason::WorldInputDisabled, "gated blocked movement should preserve world disabled reason");
	ExpectIntentEquals(result.intake.mapping.gateIssues[0].gate.intent, blockedMove, "gated blocked movement should preserve intent");
	Expect(result.intake.mapping.mapping.issues.empty(), "gated blocked movement should not be nested mapping issue");
	Expect(result.intake.mapping.frame.commands.empty(), "gated blocked movement should queue empty frame");
	Expect(result.runner.drained.frames.size() == 1, "gated blocked movement should drain one empty frame");
	Expect(result.runner.drained.frames[0].commands.empty(), "gated blocked movement drained frame should be empty");
	Expect(result.runner.runner.ticks.size() == 1, "gated blocked movement should still tick once");
	Expect(NearVec(result.session.player.position, session.player.position), "gated blocked movement should not move player");
	Expect(result.session.tickIndex == session.tickIndex + 1, "gated blocked movement should still advance one tick");
}

void TestGatedInvalidIntentIsGateIssueOnly()
{
	const iggy::PlayerInputIntent2D invalidInteract = iggy::playerInteractIntent({});
	const iggy::runtime::RuntimePlayerInputGatedCommandRunnerResult result = iggy::runtime::RuntimePlayerInputCommandRunner {}.runGated(
		GatedInput(SessionWithPlayer({ 0.0F, 0.0F }), {}, {}, { invalidInteract }));

	Expect(result.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, "gated invalid intent should still run queued runner");
	Expect(result.intake.mapping.gateIssues.size() == 1, "gated invalid intent should be a gate issue");
	Expect(result.intake.mapping.gateIssues[0].gate.reason == iggy::PlayerInputIntentBlockReason::InvalidIntent, "gated invalid intent should preserve invalid reason");
	Expect(result.intake.mapping.gateIssues[0].gate.intentStatus == iggy::PlayerInputIntent2DStatus::MissingTarget, "gated invalid intent should preserve intent validation");
	Expect(result.intake.mapping.mapping.issues.empty(), "gated invalid intent should not be a nested mapping issue");
	Expect(result.intake.mapping.frame.commands.empty(), "gated invalid intent should queue empty command frame");
	Expect(result.runner.runner.ticks.size() == 1, "gated invalid intent should still tick once with empty frame");
}

void TestGatedUnsupportedUnblockedIntentsAreNestedMappingIssues()
{
	const iggy::PlayerInputIntent2D inspect = iggy::playerInspectIntent(TargetId);
	const iggy::PlayerInputIntent2D cancel = iggy::playerCancelIntent();

	const iggy::runtime::RuntimePlayerInputGatedCommandRunnerResult result = iggy::runtime::RuntimePlayerInputCommandRunner {}.runGated(
		GatedInput(SessionWithPlayer({ 0.0F, 0.0F }), {}, {}, { inspect, cancel }));

	Expect(result.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, "gated unsupported intents should still run queued runner");
	Expect(result.intake.mapping.gateIssues.empty(), "gated unsupported unblocked intents should not be gate issues");
	Expect(result.intake.mapping.mapping.issues.size() == 2, "gated unsupported unblocked intents should be nested mapping issues");
	Expect(result.intake.mapping.mapping.issues[0].intentIndex == 0, "gated inspect issue should preserve accepted-intent index");
	Expect(result.intake.mapping.mapping.issues[0].map.status == iggy::PlayerInputCommandMapper2DStatus::UnsupportedIntent, "gated inspect should preserve unsupported mapper status");
	ExpectIntentEquals(result.intake.mapping.mapping.issues[0].intent, inspect, "gated inspect issue should preserve intent");
	Expect(result.intake.mapping.mapping.issues[1].intentIndex == 1, "gated cancel issue should preserve accepted-intent index");
	Expect(result.intake.mapping.mapping.issues[1].map.status == iggy::PlayerInputCommandMapper2DStatus::UnsupportedIntent, "gated cancel should preserve unsupported mapper status");
	ExpectIntentEquals(result.intake.mapping.mapping.issues[1].intent, cancel, "gated cancel issue should preserve intent");
	Expect(result.intake.mapping.frame.commands.empty(), "gated unsupported-only frame should have no commands");
	Expect(result.runner.runner.ticks.size() == 1, "gated unsupported-only frame should still run one tick");
}

void TestGatedBoundedFullQueueRejectsWithoutRunning()
{
	iggy::PlayerInputContext2D context;
	context.worldInputEnabled = false;
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const iggy::runtime::RuntimeCommandQueueState queue = Queue({ MoveFrame(2.0F, 0.0F) });
	const iggy::runtime::RuntimePlayerInputGatedCommandRunnerResult result = iggy::runtime::RuntimePlayerInputCommandRunner {}.runGated(
		GatedInput(session, queue, context, { iggy::playerMoveToPointIntent({ 4.0F, 0.0F }), iggy::playerWaitIntent() }, { 1 }));

	Expect(result.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::QueueRejected, "full gated queue should reject intake");
	Expect(result.intake.status == iggy::runtime::RuntimePlayerInputQueueStatus::RejectedFull, "full gated queue should preserve rejected intake");
	Expect(result.intake.mapping.gateIssues.size() == 1, "full gated queue should preserve gate diagnostics");
	Expect(result.intake.mapping.frame.commands.size() == 1, "full gated queue should preserve accepted command diagnostics");
	Expect(result.runner.runner.ticks.empty(), "full gated queue should not run queued runner");
	Expect(SameQueue(result.queue, queue), "full gated queue should return unchanged queue");
	Expect(result.session.tickIndex == session.tickIndex, "full gated queue should not advance tickIndex");
	ExpectPlayerAgent(result.session.player, session.player, "full gated queue rejected result");
}

void TestGatedExplicitWorldOverridesSessionCollisionCache()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	SetCollisionCache(session, BlockingWorld());
	const iggy::physics2d::CollisionWorld2D emptyWorld;

	const iggy::runtime::RuntimePlayerInputGatedCommandRunnerResult result = iggy::runtime::RuntimePlayerInputCommandRunner {}.runGated(
		GatedInput(session, {}, {}, { iggy::playerMoveToPointIntent({ 2.0F, 0.0F }) }),
		emptyWorld);

	Expect(result.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, "gated explicit world run should succeed");
	Expect(result.intake.mapping.gateIssues.empty(), "gated explicit world movement should have no gate issues");
	Expect(result.runner.runner.ticks.size() == 1, "gated explicit world run should produce one tick");
	if (result.runner.runner.ticks.size() == 1) {
		Expect(result.runner.runner.ticks[0].playerCommands.execution.movementResults.size() == 1, "gated explicit world movement should preserve diagnostics");
		Expect(result.runner.runner.ticks[0].playerCommands.execution.movementResults[0].status == iggy::PlayerMovementExecutionStatus::Moved, "gated explicit empty world should override blocking session cache");
	}
	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "gated explicit empty world should allow movement");
}

void TestGatedNoExplicitOverloadUsesSessionDerivedCollisionCache()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	SetCollisionCache(session, BlockingWorld());

	const iggy::runtime::RuntimePlayerInputGatedCommandRunnerResult result = iggy::runtime::RuntimePlayerInputCommandRunner {}.runGated(
		GatedInput(session, {}, {}, { iggy::playerMoveToPointIntent({ 2.0F, 0.0F }) }));

	Expect(result.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, "gated session collision cache run should succeed");
	Expect(result.runner.runner.ticks.size() == 1, "gated session collision cache run should produce one tick");
	if (result.runner.runner.ticks.size() == 1) {
		Expect(result.runner.runner.ticks[0].playerCommands.execution.movementResults.size() == 1, "gated blocked movement should preserve diagnostics");
		Expect(result.runner.runner.ticks[0].playerCommands.execution.movementResults[0].status == iggy::PlayerMovementExecutionStatus::Blocked, "gated session collision cache should block movement");
	}
	Expect(NearVec(result.session.player.position, { 0.0F, 0.0F }), "gated session collision cache should keep player from moving");
}

void TestGatedMissingPlayerDiagnosticsFlowFromQueuedRunner()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithoutPlayer();
	const iggy::runtime::RuntimePlayerInputGatedCommandRunnerResult result = iggy::runtime::RuntimePlayerInputCommandRunner {}.runGated(
		GatedInput(session, {}, {}, { iggy::playerMoveToPointIntent({ 2.0F, 0.0F }) }));

	Expect(result.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, "gated missing player input should still run queued runner");
	Expect(result.intake.mapping.gateIssues.empty(), "gated missing player valid intent should have no gate issues");
	Expect(result.runner.runner.ticks.size() == 1, "gated missing player input should produce one tick");
	if (result.runner.runner.ticks.size() == 1) {
		Expect(result.runner.runner.ticks[0].playerCommands.planning.status == iggy::runtime::RuntimePlayerCommandPlanningStatus::MissingPlayer, "gated missing player should preserve planning diagnostics");
		Expect(result.runner.runner.ticks[0].playerCommands.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::MissingPlayer, "gated missing player should preserve execution diagnostics");
		Expect(NearVec(result.runner.runner.ticks[0].npcTargetPosition, { 1.5F, 1.5F }), "gated missing player should use fallback NPC target");
	}
	Expect(!result.session.hasPlayer, "gated missing player input runner should not invent a player");
	Expect(result.session.tickIndex == session.tickIndex + 1, "gated missing player input should still tick once");
}

void TestMissingPlayerDiagnosticsFlowFromQueuedRunner()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithoutPlayer();
	const iggy::runtime::RuntimePlayerInputCommandRunnerResult result = iggy::runtime::RuntimePlayerInputCommandRunner {}.run(
		Input(session, {}, { iggy::playerMoveToPointIntent({ 2.0F, 0.0F }) }));

	Expect(result.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, "missing player input should still run queued runner");
	Expect(result.runner.runner.ticks.size() == 1, "missing player input should produce one tick");
	if (result.runner.runner.ticks.size() == 1) {
		Expect(result.runner.runner.ticks[0].playerCommands.planning.status == iggy::runtime::RuntimePlayerCommandPlanningStatus::MissingPlayer, "missing player should preserve planning diagnostics");
		Expect(result.runner.runner.ticks[0].playerCommands.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::MissingPlayer, "missing player should preserve execution diagnostics");
		Expect(NearVec(result.runner.runner.ticks[0].npcTargetPosition, { 1.5F, 1.5F }), "missing player should use fallback NPC target");
	}
	Expect(!result.session.hasPlayer, "missing player input runner should not invent a player");
	Expect(result.session.tickIndex == session.tickIndex + 1, "missing player input should still tick once");
}

void TestInputsAreNotMutated()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	iggy::runtime::RuntimePlayerInputCommandRunnerInput input = Input(
		session,
		Queue({ MoveFrame(2.0F, 0.0F) }),
		{ iggy::playerMoveToPointIntent({ 4.0F, 0.0F }), iggy::playerCancelIntent() },
		{ 3 });
	const iggy::runtime::RuntimePlayerInputCommandRunnerInput before = input;

	const iggy::runtime::RuntimePlayerInputCommandRunnerResult result = iggy::runtime::RuntimePlayerInputCommandRunner {}.run(input);

	Expect(result.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, "immutability setup should run");
	Expect(input.session.tickIndex == before.session.tickIndex, "runner should not mutate input session tickIndex");
	ExpectPlayerAgent(input.session.player, before.session.player, "input session after runner");
	Expect(SameQueue(input.queue, before.queue), "runner should not mutate input queue");
	Expect(input.queueConfig.maxFrames == before.queueConfig.maxFrames, "runner should not mutate queue config");
	Expect(input.actorId == before.actorId, "runner should not mutate actor id");
	Expect(input.intents.size() == before.intents.size(), "runner should not mutate intents vector");
	for (std::size_t index = 0; index < input.intents.size(); ++index) {
		ExpectIntentEquals(input.intents[index], before.intents[index], "runner should not mutate intents");
	}
}

void TestGatedInputsAreNotMutated()
{
	iggy::runtime::RuntimePlayerInputGatedCommandRunnerInput input = GatedInput(
		SessionWithPlayer({ 0.0F, 0.0F }),
		Queue({ MoveFrame(2.0F, 0.0F) }),
		{},
		{ iggy::playerMoveToPointIntent({ 4.0F, 0.0F }), iggy::playerCancelIntent() },
		{ 3 });
	input.context.worldInputEnabled = false;
	const iggy::runtime::RuntimePlayerInputGatedCommandRunnerInput before = input;

	const iggy::runtime::RuntimePlayerInputGatedCommandRunnerResult result = iggy::runtime::RuntimePlayerInputCommandRunner {}.runGated(input);

	Expect(result.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, "gated immutability setup should run");
	Expect(input.session.tickIndex == before.session.tickIndex, "gated runner should not mutate input session tickIndex");
	ExpectPlayerAgent(input.session.player, before.session.player, "gated input session after runner");
	Expect(SameQueue(input.queue, before.queue), "gated runner should not mutate input queue");
	Expect(input.queueConfig.maxFrames == before.queueConfig.maxFrames, "gated runner should not mutate queue config");
	Expect(input.actorId == before.actorId, "gated runner should not mutate actor id");
	Expect(input.context.playerControlEnabled == before.context.playerControlEnabled, "gated runner should not mutate player control flag");
	Expect(input.context.worldInputEnabled == before.context.worldInputEnabled, "gated runner should not mutate world input flag");
	Expect(input.context.interactionEnabled == before.context.interactionEnabled, "gated runner should not mutate interaction flag");
	Expect(input.context.cancelEnabled == before.context.cancelEnabled, "gated runner should not mutate cancel flag");
	Expect(input.intents.size() == before.intents.size(), "gated runner should not mutate intents vector");
	for (std::size_t index = 0; index < input.intents.size(); ++index) {
		ExpectIntentEquals(input.intents[index], before.intents[index], "gated runner should not mutate intents");
	}
}

} // namespace

int main()
{
	TestEmptyIntentsRunOneQueuedEmptyFrame();
	TestValidMoveToPointIntentMovesPlayerThroughQueuedRunner();
	TestMixedValidUnsupportedIntentsPreserveIssuesAndExecuteAcceptedCommands();
	TestExistingQueuedFramesRunBeforeNewInputFrame();
	TestBoundedFullQueueRejectsWithoutRunning();
	TestExplicitWorldOverridesSessionCollisionCache();
	TestNoExplicitOverloadUsesSessionDerivedCollisionCache();
	TestDefaultContextGatedRunnerBehavesLikeUngatedForSupportedMovement();
	TestGatedContextBlockedMovementQueuesEmptyFrameAndDoesNotMove();
	TestGatedInvalidIntentIsGateIssueOnly();
	TestGatedUnsupportedUnblockedIntentsAreNestedMappingIssues();
	TestGatedBoundedFullQueueRejectsWithoutRunning();
	TestGatedExplicitWorldOverridesSessionCollisionCache();
	TestGatedNoExplicitOverloadUsesSessionDerivedCollisionCache();
	TestGatedMissingPlayerDiagnosticsFlowFromQueuedRunner();
	TestMissingPlayerDiagnosticsFlowFromQueuedRunner();
	TestInputsAreNotMutated();
	TestGatedInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
