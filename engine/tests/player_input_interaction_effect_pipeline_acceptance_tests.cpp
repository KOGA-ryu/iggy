#include <cstdlib>
#include <vector>

#include "runtime/RuntimePlayerInputInteractionEffectFrameReporter.hpp"
#include "runtime/RuntimePlayerInputInteractionEffectFrameStep.hpp"
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

const iggy::ResourceId PlayerId { "player:input-interaction-effect-pipeline" };

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
	session.level.map.id = iggy::ResourceId { "level:input-interaction-effect-pipeline" };
	session.level.map.playerStart = { 0, 0 };
	session.tickIndex = 41;
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
	Expect(result.built, "player input interaction effect acceptance registry should build");
	return result.registry;
}

iggy::InteractionEffectEntry2D Entry(const char *targetId, std::vector<iggy::InteractionEffect2D> effects)
{
	return { iggy::ResourceId { targetId }, effects };
}

iggy::InteractionEffectCatalog2D Catalog(std::vector<iggy::InteractionEffectEntry2D> entries)
{
	const iggy::InteractionEffectCatalog2DBuildResult result = iggy::InteractionEffectCatalog2DBuilder {}.build(entries);
	Expect(result.built, "player input interaction effect acceptance catalog should build");
	return result.catalog;
}

iggy::runtime::RuntimePlayerInputInteractionEffectFrameInput Input(
	iggy::runtime::RuntimePlayerInputGatedFrameStepInput playerInput,
	iggy::InteractionTarget2DRegistry targets,
	iggy::InteractionEffectCatalog2D effects,
	iggy::InteractionReach2DConfig reach = {})
{
	return { playerInput, targets, effects, reach };
}

iggy::runtime::RuntimePlayerInputInteractionEffectFrameReport Report(
	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameResult &result)
{
	return iggy::runtime::RuntimePlayerInputInteractionEffectFrameReporter {}.report(result);
}

bool SameEvents(
	const std::vector<iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent> &actual,
	const std::vector<iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (actual[index] != expected[index])
			return false;
	}
	return true;
}

bool SameEffect(const iggy::InteractionEffect2D &actual, const iggy::InteractionEffect2D &expected)
{
	return actual.type == expected.type
		&& actual.targetId == expected.targetId
		&& actual.eventId == expected.eventId
		&& actual.text == expected.text
		&& actual.enabledValue == expected.enabledValue;
}

bool SameEffects(const std::vector<iggy::InteractionEffect2D> &actual, const std::vector<iggy::InteractionEffect2D> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (!SameEffect(actual[index], expected[index]))
			return false;
	}
	return true;
}

void ExpectRanOneFrame(const iggy::runtime::RuntimePlayerInputInteractionEffectFrameResult &result, const char *message)
{
	Expect(result.playerInput.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, message);
	Expect(result.playerInput.command.runner.runner.ticks.size() == 1, message);
	Expect(result.playerInput.report.queuedFrameCount == 1, message);
	Expect(result.playerInput.report.tickResultCount == 1, message);
	Expect(result.queue.frames.empty(), message);
}

void TestReachableInteractionWithEffects()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const iggy::InteractionTarget2D target = Target("target:effects", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 0.25F);
	const std::vector<iggy::InteractionEffect2D> effects {
		iggy::inspectTextInteractionEffect(target.id, "Panel opens."),
		iggy::emitInteractionEventEffect(target.id, iggy::ResourceId { "event:panel_opened" }),
		iggy::toggleTargetInteractionEffect(target.id, false),
	};
	const iggy::InteractionTarget2DRegistry registry = Registry({ target });

	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameStep {}.run(
			Input(PlayerInput(session, { iggy::playerInteractIntent(target.id) }), registry, Catalog({ Entry("target:effects", effects) })));
	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameReport report = Report(result);

	ExpectRanOneFrame(result, "reachable effects interaction should run one frame");
	ExpectPlayerAgent(result.session.player, session.player, "reachable effects interaction");
	Expect(result.session.tickIndex == session.tickIndex + 1, "reachable effects interaction should advance only through command tick");
	Expect(result.interactions.interactions.size() == 1, "reachable effects interaction should produce one effect entry");
	Expect(result.interactions.readyCount == 1, "reachable effects interaction should count one ready entry");
	Expect(result.interactions.requestedEffectCount == effects.size(), "reachable effects interaction should count catalog effects");
	if (result.interactions.interactions.size() == 1) {
		const iggy::runtime::RuntimeInteractionEffectCommandResult &interaction = result.interactions.interactions[0].result;
		Expect(interaction.status == iggy::runtime::RuntimeInteractionEffectCommandStatus::Ready, "reachable effects interaction should be ready");
		Expect(interaction.interaction.plan.status == iggy::InteractionPlan2DStatus::Ready, "reachable effects interaction should preserve ready plan");
		Expect(SameEffects(interaction.effects.effects, effects), "reachable effects interaction should preserve ordered effects");
	}
	Expect(registry.find(target.id)->enabled, "reachable effects interaction should not toggle target registry");
	Expect(report.readyInteractionCount == 1, "reachable effects report should count ready interaction");
	Expect(report.noEffectInteractionCount == 0, "reachable effects report should have zero no-effect interactions");
	Expect(report.blockedInteractionCount == 0, "reachable effects report should have zero blocked interactions");
	Expect(report.requestedEffectCount == effects.size(), "reachable effects report should count requested effects");
	Expect(report.hasRequestedEffects(), "reachable effects report should expose requested effects");
	Expect(SameEvents(report.events, {
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::PlayerCommandAccepted,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::CommandRunnerRan,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::InteractionReadyWithEffects,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::EffectsRequested,
	}), "reachable effects report should emit requested-effect events");
}

void TestReachableInteractionWithoutEffects()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const iggy::InteractionTarget2D target = Target("target:no-effects", iggy::InteractionTarget2DKind::Inspectable, { 0.0F, 0.0F }, 0.25F);

	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameStep {}.run(
			Input(PlayerInput(session, { iggy::playerInteractIntent(target.id) }), Registry({ target }), Catalog({})));
	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameReport report = Report(result);

	ExpectRanOneFrame(result, "reachable no-effects interaction should run one frame");
	Expect(result.interactions.interactions.size() == 1, "reachable no-effects interaction should produce one entry");
	Expect(result.interactions.noEffectCount == 1, "reachable no-effects interaction should count no effects");
	Expect(result.interactions.requestedEffectCount == 0, "reachable no-effects interaction should request zero effects");
	if (result.interactions.interactions.size() == 1)
		Expect(result.interactions.interactions[0].result.status == iggy::runtime::RuntimeInteractionEffectCommandStatus::NoEffects, "reachable no-effects interaction should report NoEffects");
	Expect(report.readyInteractionCount == 0, "reachable no-effects report should count zero ready-with-effects interactions");
	Expect(report.noEffectInteractionCount == 1, "reachable no-effects report should count ready-without-effects interaction");
	Expect(report.requestedEffectCount == 0, "reachable no-effects report should have zero requested effects");
	Expect(!report.hasRequestedEffects(), "reachable no-effects report should expose no requested effects");
	Expect(SameEvents(report.events, {
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::PlayerCommandAccepted,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::CommandRunnerRan,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::InteractionReadyWithoutEffects,
	}), "reachable no-effects report should emit ready-without-effects event");
}

void TestBlockedTargetsWithCatalogEffectsDoNotSurfaceRequestedEffects()
{
	const iggy::InteractionTarget2D disabled = Target("target:disabled-effects", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 1.0F, false);
	const iggy::InteractionTarget2D far = Target("target:far-effects", iggy::InteractionTarget2DKind::Talk, { 4.0F, 0.0F }, 0.5F);
	const iggy::InteractionEffectCatalog2D effects = Catalog({
		Entry("target:disabled-effects", { iggy::inspectTextInteractionEffect(disabled.id, "Disabled") }),
		Entry("target:far-effects", { iggy::emitInteractionEventEffect(far.id, iggy::ResourceId { "event:far" }) }),
	});

	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameResult disabledResult =
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameStep {}.run(
			Input(PlayerInput(SessionWithPlayer({ 0.0F, 0.0F }), { iggy::playerInteractIntent(disabled.id) }), Registry({ disabled, far }), effects));
	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameResult farResult =
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameStep {}.run(
			Input(PlayerInput(SessionWithPlayer({ 0.0F, 0.0F }), { iggy::playerInteractIntent(far.id) }), Registry({ disabled, far }), effects));
	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameReport disabledReport = Report(disabledResult);
	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameReport farReport = Report(farResult);

	Expect(disabledResult.interactions.blockedCount == 1, "disabled target with catalog effects should be blocked");
	Expect(farResult.interactions.blockedCount == 1, "far target with catalog effects should be blocked");
	Expect(disabledResult.interactions.requestedEffectCount == 0, "disabled target should not surface catalog effects");
	Expect(farResult.interactions.requestedEffectCount == 0, "far target should not surface catalog effects");
	if (disabledResult.interactions.interactions.size() == 1)
		Expect(disabledResult.interactions.interactions[0].result.interaction.status == iggy::runtime::RuntimeInteractionCommandStatus::TargetDisabled, "disabled target should preserve disabled status");
	if (farResult.interactions.interactions.size() == 1) {
		const iggy::InteractionReach2DResult &reach = farResult.interactions.interactions[0].result.interaction.plan.reach;
		Expect(farResult.interactions.interactions[0].result.interaction.status == iggy::runtime::RuntimeInteractionCommandStatus::OutOfRange, "far target should preserve out-of-range status");
		Expect(Near(reach.distance, 4.0F), "far target should preserve distance");
		Expect(Near(reach.allowedDistance, 0.5F), "far target should preserve allowed distance");
	}
	Expect(disabledReport.blockedInteractionCount == 1 && farReport.blockedInteractionCount == 1, "blocked target reports should count blocked interactions");
	Expect(!disabledReport.hasRequestedEffects() && !farReport.hasRequestedEffects(), "blocked target reports should have no requested effects");
}

void TestContextBlockedInteractHasNoEffectDiagnostics()
{
	iggy::PlayerInputContext2D context;
	context.interactionEnabled = false;
	const iggy::InteractionTarget2D target = Target("target:context-blocked-effects", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 1.0F);

	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameStep {}.run(
			Input(
				PlayerInput(SessionWithPlayer({ 0.0F, 0.0F }), { iggy::playerInteractIntent(target.id) }, context),
				Registry({ target }),
				Catalog({ Entry("target:context-blocked-effects", { iggy::inspectTextInteractionEffect({}, "Blocked") }) })));
	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameReport report = Report(result);

	ExpectRanOneFrame(result, "context-blocked interact should still run queued empty frame");
	Expect(result.playerInput.command.intake.mapping.gateIssues.size() == 1, "context-blocked interact should preserve gate issue");
	Expect(result.playerInput.command.intake.mapping.frame.commands.empty(), "context-blocked interact should not enter mapped command frame");
	Expect(!result.interactions.hasInteractions(), "context-blocked interact should produce no effect-aware interaction entries");
	Expect(report.blockedIntentCount == 1, "context-blocked report should count blocked intent");
	Expect(report.readyInteractionCount == 0 && report.blockedInteractionCount == 0, "context-blocked report should have no interaction counts");
	Expect(report.requestedEffectCount == 0, "context-blocked report should have no requested effects");
	Expect(SameEvents(report.events, {
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::PlayerIntentBlocked,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::CommandFrameQueued,
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::CommandRunnerRan,
	}), "context-blocked report should emit blocked intent frame runner events");
}

void TestMoveThenInteractWithEffectsUsesPostPlayerInputPosition()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const iggy::InteractionTarget2D target = Target("target:after-move-effects", iggy::InteractionTarget2DKind::Usable, { 1.0F, 0.0F }, 0.0F);
	const std::vector<iggy::InteractionEffect2D> effects {
		iggy::toggleTargetInteractionEffect(target.id, false),
	};

	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameStep {}.run(
			Input(
				PlayerInput(session, {
					iggy::playerMoveToPointIntent({ 1.0F, 0.0F }),
					iggy::playerInteractIntent(target.id),
				}),
				Registry({ target }),
				Catalog({ Entry("target:after-move-effects", effects) })));
	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameReport report = Report(result);

	ExpectRanOneFrame(result, "move-then-interact effects should run one frame");
	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "move-then-interact effects should move player before diagnostics");
	Expect(result.interactions.readyCount == 1, "move-then-interact effects should be ready after movement");
	Expect(result.interactions.requestedEffectCount == effects.size(), "move-then-interact effects should request effects");
	if (result.interactions.interactions.size() == 1) {
		const iggy::runtime::RuntimeInteractionEffectCommandResult &interaction = result.interactions.interactions[0].result;
		Expect(result.interactions.interactions[0].commandIndex == 1, "move-then-interact effects should preserve interact command index");
		Expect(NearVec(interaction.interaction.plan.actorPosition, { 1.0F, 0.0F }), "move-then-interact effects should use post-move actor position");
		Expect(Near(interaction.interaction.plan.reach.distance, 0.0F), "move-then-interact effects should compute reach after movement");
		Expect(SameEffects(interaction.effects.effects, effects), "move-then-interact effects should preserve requested effects");
	}
	Expect(report.acceptedCommandCount == 2, "move-then-interact report should count movement and interaction commands");
	Expect(report.readyInteractionCount == 1, "move-then-interact report should count ready interaction");
	Expect(report.requestedEffectCount == effects.size(), "move-then-interact report should count requested effects");
}

void TestUnsupportedInspectPreservesMappingRejectionWithoutEffectEntries()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const iggy::InteractionTarget2D target = Target("target:inspect-effects", iggy::InteractionTarget2DKind::Inspectable, { 0.0F, 0.0F }, 1.0F);

	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameStep {}.run(
			Input(
				PlayerInput(session, { iggy::playerInspectIntent(target.id) }),
				Registry({ target }),
				Catalog({ Entry("target:inspect-effects", { iggy::inspectTextInteractionEffect(target.id, "Inspect") }) })));
	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameReport report = Report(result);

	ExpectRanOneFrame(result, "unsupported inspect effects should still run queued empty frame");
	ExpectPlayerAgent(result.session.player, session.player, "unsupported inspect effects");
	Expect(result.playerInput.command.intake.mapping.gateIssues.empty(), "unsupported inspect effects should not be gate-blocked");
	Expect(result.playerInput.command.intake.mapping.mapping.issues.size() == 1, "unsupported inspect effects should preserve mapping rejection");
	Expect(!result.interactions.hasInteractions(), "unsupported inspect effects should produce no effect entries");
	Expect(report.rejectedIntentCount == 1, "unsupported inspect effects report should count rejected intent");
	Expect(report.readyInteractionCount == 0 && report.noEffectInteractionCount == 0 && report.blockedInteractionCount == 0, "unsupported inspect effects report should have no interaction counts");
	Expect(report.requestedEffectCount == 0, "unsupported inspect effects report should have zero requested effects");
}

} // namespace

int main()
{
	TestReachableInteractionWithEffects();
	TestReachableInteractionWithoutEffects();
	TestBlockedTargetsWithCatalogEffectsDoNotSurfaceRequestedEffects();
	TestContextBlockedInteractHasNoEffectDiagnostics();
	TestMoveThenInteractWithEffectsUsesPostPlayerInputPosition();
	TestUnsupportedInspectPreservesMappingRejectionWithoutEffectEntries();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
