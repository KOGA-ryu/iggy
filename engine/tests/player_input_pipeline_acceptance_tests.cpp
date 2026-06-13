#include <cstdlib>
#include <string_view>
#include <vector>

#include "runtime/RuntimePlayerInputFrameStep.hpp"
#include "scene/level/LevelDerivedCacheState.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::ExpectPlayerAgent;
using iggy::test::Failures;
using iggy::test::MapFromRows;
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

const iggy::ResourceId PlayerId { "player:pipeline" };
const iggy::ResourceId TargetId { "target:pipeline" };

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

iggy::LevelRuntimeState LevelFromRows(std::vector<std::string_view> rows)
{
	iggy::LevelRuntimeState level;
	level.map = MapFromRows(rows);
	level.map.id = iggy::ResourceId("level:pipeline");
	level.map.playerStart = { 0, 0 };
	return level;
}

iggy::LevelDerivedCacheState BuildCollisionDerivedCache(const iggy::LevelRuntimeState &level)
{
	iggy::LevelDerivedCacheBuildConfig config;
	config.buildCollisionCache = true;
	const iggy::LevelDerivedCacheBuildResult result = iggy::LevelDerivedCacheBuilder {}.build(level, config);
	Expect(result.built, "acceptance collision cache should build");
	Expect(result.state.hasCollisionCache, "acceptance collision cache should be present");
	return result.state;
}

iggy::runtime::RuntimeSessionState Session(
	std::vector<std::string_view> rows,
	iggy::Vec2 playerPosition,
	bool buildCollisionCache = true)
{
	iggy::runtime::RuntimeSessionState session;
	session.level = LevelFromRows(rows);
	session.tickIndex = 11;
	session.hasPlayer = true;
	session.player = PlayerAgent(PlayerId, playerPosition, { 0, 0 }, iggy::PlayerMovementStatus::Idle, iggy::PlayerFacing2D::East);
	if (buildCollisionCache)
		session.derivedCaches = BuildCollisionDerivedCache(session.level);
	return session;
}

iggy::runtime::RuntimePlayerInputGatedFrameStepInput Input(
	iggy::runtime::RuntimeSessionState session,
	iggy::PlayerInputContext2D context,
	std::vector<iggy::PlayerInputIntent2D> intents)
{
	return {
		{
			session,
			{},
			{},
			PlayerId,
			context,
			intents,
			{ 1.5F, 1.5F },
			PlayerConfig(),
			NpcConfig(),
		},
	};
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

void ExpectRanOneTick(const iggy::runtime::RuntimePlayerInputGatedFrameStepResult &result, const char *message)
{
	Expect(result.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, message);
	Expect(result.command.runner.runner.ticks.size() == 1, message);
	Expect(result.report.queuedFrameCount == 1, message);
	Expect(result.report.tickResultCount == 1, message);
	Expect(result.queue.frames.empty(), message);
}

void TestValidMovement()
{
	const iggy::runtime::RuntimeSessionState session = Session({ "...", "..#" }, { 0.5F, 0.5F });

	const iggy::runtime::RuntimePlayerInputGatedFrameStepResult result = iggy::runtime::RuntimePlayerInputFrameStep {}.runGated(
		Input(session, {}, { iggy::playerMoveToPointIntent({ 0.5F, 1.5F }) }));

	ExpectRanOneTick(result, "valid movement should run one gated frame");
	Expect(NearVec(result.session.player.position, { 0.5F, 1.5F }), "valid movement should advance to open destination");
	Expect(result.session.tickIndex == session.tickIndex + 1, "valid movement should advance tick index");
	Expect(result.report.acceptedCommandCount == 1, "valid movement should report accepted command");
	Expect(result.report.blockedIntentCount == 0, "valid movement should have no blocked intents");
	Expect(result.report.rejectedIntentCount == 0, "valid movement should have no rejected intents");
	Expect(SameEvents(result.report.events, {
		iggy::runtime::RuntimePlayerInputCommandEvent::IntentMapped,
		iggy::runtime::RuntimePlayerInputCommandEvent::IntakeQueued,
		iggy::runtime::RuntimePlayerInputCommandEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputCommandEvent::CommandRunnerRan,
	}), "valid movement should report mapped queued runner events");
}

void TestContextBlockedMovement()
{
	iggy::PlayerInputContext2D context;
	context.worldInputEnabled = false;
	const iggy::runtime::RuntimeSessionState session = Session({ "...", "..#" }, { 0.5F, 0.5F });

	const iggy::runtime::RuntimePlayerInputGatedFrameStepResult result = iggy::runtime::RuntimePlayerInputFrameStep {}.runGated(
		Input(session, context, { iggy::playerMoveToPointIntent({ 0.5F, 1.5F }) }));

	ExpectRanOneTick(result, "context-blocked movement should still run empty gated frame");
	ExpectPlayerAgent(result.session.player, session.player, "context-blocked movement");
	Expect(result.session.tickIndex == session.tickIndex + 1, "context-blocked movement should advance empty-frame tick");
	Expect(result.command.intake.mapping.frame.commands.empty(), "context-blocked movement should queue empty command frame");
	Expect(result.report.acceptedCommandCount == 0, "context-blocked movement should have no accepted commands");
	Expect(result.report.blockedIntentCount == 1, "context-blocked movement should report blocked intent");
	Expect(result.report.rejectedIntentCount == 0, "context-blocked movement should have no mapping rejections");
	Expect(SameEvents(result.report.events, {
		iggy::runtime::RuntimePlayerInputCommandEvent::IntentBlocked,
		iggy::runtime::RuntimePlayerInputCommandEvent::IntakeQueued,
		iggy::runtime::RuntimePlayerInputCommandEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputCommandEvent::CommandRunnerRan,
	}), "context-blocked movement should report blocked queued runner events");
}

void TestCollisionBlockedMovementUsesSessionDerivedCache()
{
	const iggy::runtime::RuntimeSessionState session = Session({ ".#" }, { 0.5F, 0.5F });

	const iggy::runtime::RuntimePlayerInputGatedFrameStepResult result = iggy::runtime::RuntimePlayerInputFrameStep {}.runGated(
		Input(session, {}, { iggy::playerMoveToPointIntent({ 1.5F, 0.5F }) }));

	ExpectRanOneTick(result, "collision-blocked movement should run one gated frame");
	Expect(NearVec(result.session.player.position, session.player.position), "collision-blocked movement should keep player position");
	Expect(result.report.acceptedCommandCount == 1, "collision-blocked movement should still report accepted command");
	Expect(result.report.blockedIntentCount == 0, "collision-blocked movement should have no gate block");
	Expect(result.report.rejectedIntentCount == 0, "collision-blocked movement should have no mapping rejection");
	Expect(result.command.runner.runner.ticks[0].playerCommands.execution.movementResults.size() == 1, "collision-blocked movement should preserve movement diagnostics");
	if (result.command.runner.runner.ticks[0].playerCommands.execution.movementResults.size() == 1)
		Expect(result.command.runner.runner.ticks[0].playerCommands.execution.movementResults[0].status == iggy::PlayerMovementExecutionStatus::Blocked, "collision-blocked movement should be blocked by session derived collision cache");
}

void TestExplicitCollisionOverrideWins()
{
	const iggy::runtime::RuntimeSessionState session = Session({ ".#" }, { 0.5F, 0.5F });
	const iggy::physics2d::CollisionWorld2D emptyWorld;

	const iggy::runtime::RuntimePlayerInputGatedFrameStepResult result = iggy::runtime::RuntimePlayerInputFrameStep {}.runGated(
		Input(session, {}, { iggy::playerMoveToPointIntent({ 1.5F, 0.5F }) }),
		emptyWorld);

	ExpectRanOneTick(result, "explicit collision override should run one gated frame");
	Expect(NearVec(result.session.player.position, { 1.5F, 0.5F }), "explicit empty world should allow movement through cached blocked tile");
	Expect(result.report.acceptedCommandCount == 1, "explicit override should report accepted command");
	Expect(result.report.blockedIntentCount == 0, "explicit override should have no gate block");
	Expect(result.report.rejectedIntentCount == 0, "explicit override should have no mapping rejection");
	Expect(result.command.runner.runner.ticks[0].playerCommands.execution.movementResults.size() == 1, "explicit override should preserve movement diagnostics");
	if (result.command.runner.runner.ticks[0].playerCommands.execution.movementResults.size() == 1)
		Expect(result.command.runner.runner.ticks[0].playerCommands.execution.movementResults[0].status == iggy::PlayerMovementExecutionStatus::Moved, "explicit empty world should override session derived collision cache");
}

void TestUnsupportedIntentDiagnostic()
{
	const iggy::runtime::RuntimeSessionState session = Session({ "...", "..#" }, { 0.5F, 0.5F });

	const iggy::runtime::RuntimePlayerInputGatedFrameStepResult result = iggy::runtime::RuntimePlayerInputFrameStep {}.runGated(
		Input(session, {}, { iggy::playerInspectIntent(TargetId) }));

	ExpectRanOneTick(result, "unsupported intent should still run empty gated frame");
	ExpectPlayerAgent(result.session.player, session.player, "unsupported intent");
	Expect(result.command.intake.mapping.gateIssues.empty(), "unsupported inspect should not be gate-blocked by default context");
	Expect(result.command.intake.mapping.mapping.issues.size() == 1, "unsupported inspect should be a nested mapping issue");
	Expect(result.report.acceptedCommandCount == 0, "unsupported inspect should have no accepted commands");
	Expect(result.report.blockedIntentCount == 0, "unsupported inspect should have no gate block");
	Expect(result.report.rejectedIntentCount == 1, "unsupported inspect should report mapping rejection");
	Expect(SameEvents(result.report.events, {
		iggy::runtime::RuntimePlayerInputCommandEvent::IntentRejected,
		iggy::runtime::RuntimePlayerInputCommandEvent::IntakeQueued,
		iggy::runtime::RuntimePlayerInputCommandEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputCommandEvent::CommandRunnerRan,
	}), "unsupported inspect should report rejected queued runner events");
}

} // namespace

int main()
{
	TestValidMovement();
	TestContextBlockedMovement();
	TestCollisionBlockedMovementUsesSessionDerivedCache();
	TestExplicitCollisionOverrideWins();
	TestUnsupportedIntentDiagnostic();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
