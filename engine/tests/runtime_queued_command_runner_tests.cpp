#include <cstdlib>
#include <vector>

#include "runtime/RuntimeQueuedCommandRunner.hpp"
#include "scene/level/LevelRuntimeState.hpp"
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

const iggy::ResourceId PlayerId { "player:queue-runner" };

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
	session.level.map.id = iggy::ResourceId("level:queue-runner");
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
	session.level.map.id = iggy::ResourceId("level:queue-runner");
	session.tickIndex = 9;
	return session;
}

iggy::runtime::RuntimeQueuedCommandRunnerInput Input(
	iggy::runtime::RuntimeSessionState session,
	iggy::runtime::RuntimeCommandQueueState queue)
{
	return {
		session,
		queue,
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
	Expect(build.built, "queued command runner collision fixture should build");
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

void TestEmptyQueueReturnsUnchangedSessionAndEmptyQueue()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const iggy::runtime::RuntimeQueuedCommandRunnerResult result = iggy::runtime::RuntimeQueuedCommandRunner {}.run(Input(session, {}));

	Expect(result.drained.frames.empty(), "empty queued runner should drain no frames");
	Expect(result.queue.frames.empty(), "empty queued runner should return empty queue");
	Expect(result.runner.ticks.empty(), "empty queued runner should produce no runner ticks");
	Expect(result.session.tickIndex == session.tickIndex, "empty queued runner should preserve tickIndex");
	ExpectPlayerAgent(result.session.player, session.player, "empty queued runner result");
}

void TestOneQueuedFrameRunsOneTickAndEmptiesQueue()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const iggy::runtime::GameplayCommandFrame2D frame = MoveFrame(2.0F, 0.0F);

	const iggy::runtime::RuntimeQueuedCommandRunnerResult result = iggy::runtime::RuntimeQueuedCommandRunner {}.run(Input(session, Queue({ frame })));

	Expect(result.drained.frames.size() == 1 && SameFrame(result.drained.frames[0], frame), "queued runner should drain one queued frame");
	Expect(result.queue.frames.empty(), "queued runner should empty queue after drain");
	Expect(result.runner.ticks.size() == 1, "one queued frame should run one command tick");
	Expect(result.session.tickIndex == session.tickIndex + 1, "one queued frame should advance tickIndex once");
	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "one queued frame should move player through command tick runner");
}

void TestMultipleQueuedFramesPreserveFifoOrder()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const iggy::runtime::GameplayCommandFrame2D first = MoveFrame(2.0F, 0.0F);
	const iggy::runtime::GameplayCommandFrame2D second = MoveFrame(4.0F, 0.0F);

	const iggy::runtime::RuntimeQueuedCommandRunnerResult result = iggy::runtime::RuntimeQueuedCommandRunner {}.run(Input(session, Queue({ first, second })));

	Expect(result.drained.frames.size() == 2, "queued runner should drain all queued frames");
	if (result.drained.frames.size() == 2) {
		Expect(SameFrame(result.drained.frames[0], first), "queued runner should preserve first drained frame");
		Expect(SameFrame(result.drained.frames[1], second), "queued runner should preserve second drained frame");
	}
	Expect(result.runner.ticks.size() == 2, "two queued frames should produce two ticks");
	if (result.runner.ticks.size() == 2) {
		Expect(NearVec(result.runner.ticks[0].session.player.position, { 1.0F, 0.0F }), "first queued frame should execute first");
		Expect(NearVec(result.runner.ticks[1].session.player.position, { 2.0F, 0.0F }), "second queued frame should execute after first");
	}
	Expect(result.session.tickIndex == session.tickIndex + 2, "two queued frames should advance tickIndex twice");
	Expect(NearVec(result.session.player.position, { 2.0F, 0.0F }), "queued runner final session should be last runner session");
}

void TestInputQueueIsNotMutated()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	iggy::runtime::RuntimeCommandQueueState queue = Queue({ MoveFrame(2.0F, 0.0F) });
	const iggy::runtime::RuntimeCommandQueueState queueBefore = queue;

	const iggy::runtime::RuntimeQueuedCommandRunnerResult result = iggy::runtime::RuntimeQueuedCommandRunner {}.run(Input(session, queue));

	Expect(result.runner.ticks.size() == 1, "queue immutability setup should run one tick");
	Expect(SameQueue(queue, queueBefore), "queued runner should not mutate input queue");
}

void TestNoExplicitOverloadUsesSessionCollisionCache()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	SetCollisionCache(session, BlockingWorld());

	const iggy::runtime::RuntimeQueuedCommandRunnerResult result = iggy::runtime::RuntimeQueuedCommandRunner {}.run(Input(session, Queue({ MoveFrame(2.0F, 0.0F) })));

	Expect(result.runner.ticks.size() == 1, "session collision cache test should run one tick");
	if (result.runner.ticks.size() == 1) {
		Expect(result.runner.ticks[0].playerCommands.execution.movementResults.size() == 1, "blocked movement should preserve movement diagnostics");
		Expect(result.runner.ticks[0].playerCommands.execution.movementResults[0].status == iggy::PlayerMovementExecutionStatus::Blocked, "session collision cache should block movement");
	}
	Expect(NearVec(result.session.player.position, { 0.0F, 0.0F }), "session collision cache should keep player from moving");
}

void TestExplicitWorldOverloadPreservesExplicitPrecedence()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	SetCollisionCache(session, BlockingWorld());
	const iggy::physics2d::CollisionWorld2D emptyWorld;

	const iggy::runtime::RuntimeQueuedCommandRunnerResult result = iggy::runtime::RuntimeQueuedCommandRunner {}.run(
		Input(session, Queue({ MoveFrame(2.0F, 0.0F) })),
		emptyWorld);

	Expect(result.runner.ticks.size() == 1, "explicit world precedence test should run one tick");
	if (result.runner.ticks.size() == 1) {
		Expect(result.runner.ticks[0].playerCommands.execution.movementResults.size() == 1, "explicit world movement should preserve diagnostics");
		Expect(result.runner.ticks[0].playerCommands.execution.movementResults[0].status == iggy::PlayerMovementExecutionStatus::Moved, "explicit empty world should override blocking session cache");
	}
	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "explicit empty world should allow movement");
}

void TestInvalidCommandFramePreservesDiagnosticsThroughRunner()
{
	const iggy::runtime::GameplayCommandFrame2D invalidFrame = CommandFrame({
		iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, {}),
	});

	const iggy::runtime::RuntimeQueuedCommandRunnerResult result = iggy::runtime::RuntimeQueuedCommandRunner {}.run(Input(SessionWithPlayer(), Queue({ invalidFrame })));

	Expect(result.runner.ticks.size() == 1, "invalid command frame should still run one tick");
	if (result.runner.ticks.size() == 1) {
		Expect(result.runner.ticks[0].playerCommands.planning.playerPlan.validation.hasInvalidCommands, "queued invalid command should preserve validation diagnostics");
		Expect(result.runner.ticks[0].playerCommands.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::NoExecutablePlan, "queued invalid command should preserve no executable status");
	}
}

void TestMissingPlayerUsesFallbackThroughRunner()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithoutPlayer();
	const iggy::runtime::RuntimeQueuedCommandRunnerResult result = iggy::runtime::RuntimeQueuedCommandRunner {}.run(Input(session, Queue({ MoveFrame(2.0F, 0.0F) })));

	Expect(result.runner.ticks.size() == 1, "missing player queued frame should still tick");
	if (result.runner.ticks.size() == 1) {
		Expect(result.runner.ticks[0].playerCommands.planning.status == iggy::runtime::RuntimePlayerCommandPlanningStatus::MissingPlayer, "missing player should preserve planning diagnostics");
		Expect(result.runner.ticks[0].playerCommands.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::MissingPlayer, "missing player should preserve execution diagnostics");
		Expect(NearVec(result.runner.ticks[0].npcTargetPosition, { 1.5F, 1.5F }), "missing player should use fallback NPC target");
	}
	Expect(!result.session.hasPlayer, "missing player runner should not invent player");
	Expect(result.session.tickIndex == session.tickIndex + 1, "missing player queued frame should still advance session tick");
}

} // namespace

int main()
{
	TestEmptyQueueReturnsUnchangedSessionAndEmptyQueue();
	TestOneQueuedFrameRunsOneTickAndEmptiesQueue();
	TestMultipleQueuedFramesPreserveFifoOrder();
	TestInputQueueIsNotMutated();
	TestNoExplicitOverloadUsesSessionCollisionCache();
	TestExplicitWorldOverloadPreservesExplicitPrecedence();
	TestInvalidCommandFramePreservesDiagnosticsThroughRunner();
	TestMissingPlayerUsesFallbackThroughRunner();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
