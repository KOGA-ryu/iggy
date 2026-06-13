#include <cstdlib>
#include <vector>

#include "runtime/RuntimePlayerInputInteractionFrameReporter.hpp"
#include "runtime/RuntimePlayerInputInteractionFrameStep.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::ExpectPlayerAgent;
using iggy::test::Failures;
using iggy::test::MapFromRows;
using iggy::test::Near;
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

const iggy::ResourceId PlayerId { "player:input-interaction-pipeline" };

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

iggy::runtime::RuntimeSessionState SessionWithPlayer(iggy::Vec2 position)
{
	iggy::runtime::RuntimeSessionState session;
	session.level.map = MapFromRows({ "...", "..." });
	session.level.map.id = iggy::ResourceId { "level:input-interaction-pipeline" };
	session.level.map.playerStart = { 0, 0 };
	session.tickIndex = 19;
	session.hasPlayer = true;
	session.player = PlayerAgent(PlayerId, position, { 0, 0 }, iggy::PlayerMovementStatus::Idle, iggy::PlayerFacing2D::East);
	return session;
}

iggy::runtime::RuntimePlayerInputGatedFrameStepInput PlayerInput(
	iggy::runtime::RuntimeSessionState session,
	std::vector<iggy::PlayerInputIntent2D> intents,
	iggy::PlayerInputContext2D context = {})
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

iggy::InteractionTarget2D Target(
	const char *id,
	iggy::InteractionTarget2DKind kind,
	iggy::Vec2 position,
	float radius,
	bool enabled = true)
{
	return { iggy::ResourceId { id }, kind, position, radius, enabled };
}

iggy::InteractionTarget2DRegistry Registry(std::vector<iggy::InteractionTarget2D> targets)
{
	const iggy::InteractionTarget2DRegistryBuildResult result = iggy::InteractionTarget2DRegistryBuilder {}.build(targets);
	Expect(result.built, "player input interaction pipeline registry should build");
	return result.registry;
}

iggy::runtime::RuntimePlayerInputInteractionFrameInput Input(
	iggy::runtime::RuntimePlayerInputGatedFrameStepInput playerInput,
	iggy::InteractionTarget2DRegistry targets,
	iggy::InteractionReach2DConfig reach = {})
{
	return { playerInput, targets, reach };
}

bool SameEvents(
	const std::vector<iggy::runtime::RuntimePlayerInputInteractionFrameEvent> &actual,
	const std::vector<iggy::runtime::RuntimePlayerInputInteractionFrameEvent> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (actual[index] != expected[index])
			return false;
	}
	return true;
}

iggy::runtime::RuntimePlayerInputInteractionFrameReport Report(
	const iggy::runtime::RuntimePlayerInputInteractionFrameResult &result)
{
	return iggy::runtime::RuntimePlayerInputInteractionFrameReporter {}.report(result);
}

void ExpectRanOneFrame(const iggy::runtime::RuntimePlayerInputInteractionFrameResult &result, const char *message)
{
	Expect(result.playerInput.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, message);
	Expect(result.playerInput.command.runner.runner.ticks.size() == 1, message);
	Expect(result.playerInput.report.queuedFrameCount == 1, message);
	Expect(result.playerInput.report.tickResultCount == 1, message);
	Expect(result.queue.frames.empty(), message);
}

void TestReachableInteractionProducesReadyDiagnosticsAndReport()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const iggy::InteractionTarget2D target = Target("target:ready", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 0.25F);

	const iggy::runtime::RuntimePlayerInputInteractionFrameResult result = iggy::runtime::RuntimePlayerInputInteractionFrameStep {}.run(
		Input(PlayerInput(session, { iggy::playerInteractIntent(target.id) }), Registry({ target })));
	const iggy::runtime::RuntimePlayerInputInteractionFrameReport report = Report(result);

	ExpectRanOneFrame(result, "reachable interaction should run one player input frame");
	ExpectPlayerAgent(result.session.player, session.player, "reachable interaction");
	Expect(result.session.tickIndex == session.tickIndex + 1, "reachable interaction should advance only through existing command tick");
	Expect(result.interactions.interactions.size() == 1, "reachable interaction should produce one diagnostic entry");
	if (result.interactions.interactions.size() == 1) {
		const iggy::runtime::RuntimeInteractionCommandResult &interaction = result.interactions.interactions[0].result;
		Expect(interaction.status == iggy::runtime::RuntimeInteractionCommandStatus::Ready, "reachable interaction should be ready");
		Expect(interaction.plan.status == iggy::InteractionPlan2DStatus::Ready, "reachable interaction should preserve ready plan status");
		Expect(interaction.plan.reach.status == iggy::InteractionReach2DStatus::Reachable, "reachable interaction should preserve reachable status");
	}
	Expect(report.readyInteractionCount == 1, "reachable report should count ready interaction");
	Expect(report.blockedInteractionCount == 0, "reachable report should not count blocked interactions");
	Expect(report.acceptedCommandCount == 1, "reachable report should count accepted interact command");
	Expect(SameEvents(report.events, {
		iggy::runtime::RuntimePlayerInputInteractionFrameEvent::PlayerCommandAccepted,
		iggy::runtime::RuntimePlayerInputInteractionFrameEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputInteractionFrameEvent::CommandRunnerRan,
		iggy::runtime::RuntimePlayerInputInteractionFrameEvent::InteractionReady,
	}), "reachable report should emit accepted frame runner ready events");
}

void TestDisabledTargetProducesBlockedDiagnosticsAndReport()
{
	const iggy::InteractionTarget2D target = Target("target:disabled", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 1.0F, false);
	const iggy::runtime::RuntimePlayerInputInteractionFrameResult result = iggy::runtime::RuntimePlayerInputInteractionFrameStep {}.run(
		Input(PlayerInput(SessionWithPlayer({ 0.0F, 0.0F }), { iggy::playerInteractIntent(target.id) }), Registry({ target })));
	const iggy::runtime::RuntimePlayerInputInteractionFrameReport report = Report(result);

	Expect(result.interactions.interactions.size() == 1, "disabled target should produce one interaction entry");
	if (result.interactions.interactions.size() == 1) {
		const iggy::runtime::RuntimeInteractionCommandResult &interaction = result.interactions.interactions[0].result;
		Expect(interaction.status == iggy::runtime::RuntimeInteractionCommandStatus::TargetDisabled, "disabled target should report runtime disabled status");
		Expect(interaction.plan.status == iggy::InteractionPlan2DStatus::TargetDisabled, "disabled target should preserve plan disabled status");
		Expect(interaction.plan.reach.status == iggy::InteractionReach2DStatus::TargetDisabled, "disabled target should preserve reach disabled status");
	}
	Expect(report.readyInteractionCount == 0, "disabled target report should have no ready interactions");
	Expect(report.blockedInteractionCount == 1, "disabled target report should count blocked interaction");
}

void TestOutOfRangeTargetPreservesDistanceAndAllowedDistance()
{
	const iggy::InteractionTarget2D target = Target("target:far", iggy::InteractionTarget2DKind::Talk, { 3.0F, 0.0F }, 0.5F);
	const iggy::runtime::RuntimePlayerInputInteractionFrameResult result = iggy::runtime::RuntimePlayerInputInteractionFrameStep {}.run(
		Input(PlayerInput(SessionWithPlayer({ 0.0F, 0.0F }), { iggy::playerInteractIntent(target.id) }), Registry({ target })));
	const iggy::runtime::RuntimePlayerInputInteractionFrameReport report = Report(result);

	Expect(result.interactions.interactions.size() == 1, "out-of-range target should produce one interaction entry");
	if (result.interactions.interactions.size() == 1) {
		const iggy::InteractionReach2DResult &reach = result.interactions.interactions[0].result.plan.reach;
		Expect(result.interactions.interactions[0].result.status == iggy::runtime::RuntimeInteractionCommandStatus::OutOfRange, "out-of-range target should report runtime out-of-range status");
		Expect(reach.status == iggy::InteractionReach2DStatus::OutOfRange, "out-of-range target should preserve reach status");
		Expect(Near(reach.distance, 3.0F), "out-of-range target should preserve computed distance");
		Expect(Near(reach.allowedDistance, 0.5F), "out-of-range target should preserve allowed distance");
	}
	Expect(report.blockedInteractionCount == 1, "out-of-range report should count blocked interaction");
}

void TestContextBlockedInteractDoesNotReachInteractionDiagnostics()
{
	iggy::PlayerInputContext2D context;
	context.interactionEnabled = false;
	const iggy::InteractionTarget2D target = Target("target:context-blocked", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 1.0F);

	const iggy::runtime::RuntimePlayerInputInteractionFrameResult result = iggy::runtime::RuntimePlayerInputInteractionFrameStep {}.run(
		Input(PlayerInput(SessionWithPlayer({ 0.0F, 0.0F }), { iggy::playerInteractIntent(target.id) }, context), Registry({ target })));
	const iggy::runtime::RuntimePlayerInputInteractionFrameReport report = Report(result);

	ExpectRanOneFrame(result, "context-blocked interact should still run queued empty frame");
	Expect(result.playerInput.report.blockedIntentCount == 1, "context-blocked interact should be reported by gate");
	Expect(result.playerInput.command.intake.mapping.frame.commands.empty(), "context-blocked interact should not enter mapped command frame");
	Expect(!result.interactions.hasInteractions(), "context-blocked interact should have no interaction diagnostics");
	Expect(report.blockedIntentCount == 1, "context-blocked report should count blocked intent");
	Expect(report.readyInteractionCount == 0 && report.blockedInteractionCount == 0, "context-blocked report should have no interaction counts");
}

void TestMoveThenInteractUsesPostPlayerInputSessionPosition()
{
	const iggy::InteractionTarget2D target = Target("target:after-move", iggy::InteractionTarget2DKind::Usable, { 1.0F, 0.0F }, 0.0F);
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });

	const iggy::runtime::RuntimePlayerInputInteractionFrameResult result = iggy::runtime::RuntimePlayerInputInteractionFrameStep {}.run(
		Input(
			PlayerInput(session, {
				iggy::playerMoveToPointIntent({ 1.0F, 0.0F }),
				iggy::playerInteractIntent(target.id),
			}),
			Registry({ target })));
	const iggy::runtime::RuntimePlayerInputInteractionFrameReport report = Report(result);

	ExpectRanOneFrame(result, "move-then-interact should run one frame");
	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "move-then-interact should move player before diagnostics");
	Expect(result.interactions.interactions.size() == 1, "move-then-interact should produce one interaction entry");
	if (result.interactions.interactions.size() == 1) {
		const iggy::InteractionPlan2DResult &plan = result.interactions.interactions[0].result.plan;
		Expect(result.interactions.interactions[0].commandIndex == 1, "move-then-interact should preserve interact command index");
		Expect(plan.status == iggy::InteractionPlan2DStatus::Ready, "move-then-interact should be ready after movement");
		Expect(NearVec(plan.actorPosition, { 1.0F, 0.0F }), "move-then-interact should use post-move actor position");
		Expect(Near(plan.reach.distance, 0.0F), "move-then-interact should compute zero distance after movement");
	}
	Expect(report.acceptedCommandCount == 2, "move-then-interact report should count both accepted commands");
	Expect(report.readyInteractionCount == 1, "move-then-interact report should count ready interaction");
}

void TestUnsupportedInspectPreservesMappingRejectionWithoutInteractionEntries()
{
	const iggy::InteractionTarget2D target = Target("target:inspect", iggy::InteractionTarget2DKind::Inspectable, { 0.0F, 0.0F }, 1.0F);
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });

	const iggy::runtime::RuntimePlayerInputInteractionFrameResult result = iggy::runtime::RuntimePlayerInputInteractionFrameStep {}.run(
		Input(PlayerInput(session, { iggy::playerInspectIntent(target.id) }), Registry({ target })));
	const iggy::runtime::RuntimePlayerInputInteractionFrameReport report = Report(result);

	ExpectRanOneFrame(result, "unsupported inspect should still run queued empty frame");
	ExpectPlayerAgent(result.session.player, session.player, "unsupported inspect");
	Expect(result.playerInput.command.intake.mapping.gateIssues.empty(), "unsupported inspect should not be gate blocked");
	Expect(result.playerInput.command.intake.mapping.mapping.issues.size() == 1, "unsupported inspect should preserve mapping rejection");
	Expect(!result.interactions.hasInteractions(), "unsupported inspect should produce no interaction entries");
	Expect(report.rejectedIntentCount == 1, "unsupported inspect report should count rejected intent");
	Expect(report.readyInteractionCount == 0 && report.blockedInteractionCount == 0, "unsupported inspect report should have no interaction counts");
}

} // namespace

int main()
{
	TestReachableInteractionProducesReadyDiagnosticsAndReport();
	TestDisabledTargetProducesBlockedDiagnosticsAndReport();
	TestOutOfRangeTargetPreservesDistanceAndAllowedDistance();
	TestContextBlockedInteractDoesNotReachInteractionDiagnostics();
	TestMoveThenInteractUsesPostPlayerInputSessionPosition();
	TestUnsupportedInspectPreservesMappingRejectionWithoutInteractionEntries();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
