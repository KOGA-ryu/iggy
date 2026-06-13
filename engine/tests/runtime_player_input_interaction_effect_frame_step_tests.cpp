#include <cstdlib>
#include <vector>

#include "runtime/RuntimePlayerInputInteractionEffectFrameStep.hpp"
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
using iggy::test::Near;
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

const iggy::ResourceId PlayerId { "player:input-interaction-effect-frame" };

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
	session.level.map.id = iggy::ResourceId { "level:input-interaction-effect-frame" };
	session.hasPlayer = true;
	session.player = PlayerAgent(PlayerId, position, { 0, 0 }, iggy::PlayerMovementStatus::Idle, iggy::PlayerFacing2D::East);
	session.tickIndex = 37;
	return session;
}

iggy::runtime::GameplayCommandFrame2D WaitFrame()
{
	return CommandFrame({ iggy::runtime::GameplayCommand2DFactory {}.wait(PlayerId) });
}

iggy::runtime::RuntimePlayerInputGatedFrameStepInput PlayerInput(
	iggy::runtime::RuntimeSessionState session,
	std::vector<iggy::PlayerInputIntent2D> intents,
	iggy::PlayerInputContext2D context = {},
	iggy::runtime::RuntimeCommandQueueState queue = {},
	iggy::runtime::RuntimeCommandQueueConfig queueConfig = {})
{
	return {
		{
			session,
			queue,
			queueConfig,
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
	Expect(result.built, "player input interaction effect frame registry setup should build");
	return result.registry;
}

iggy::InteractionEffectEntry2D Entry(const char *targetId, std::vector<iggy::InteractionEffect2D> effects)
{
	return { iggy::ResourceId { targetId }, effects };
}

iggy::InteractionEffectCatalog2D Catalog(std::vector<iggy::InteractionEffectEntry2D> entries)
{
	const iggy::InteractionEffectCatalog2DBuildResult result = iggy::InteractionEffectCatalog2DBuilder {}.build(entries);
	Expect(result.built, "player input interaction effect frame catalog setup should build");
	return result.catalog;
}

iggy::runtime::RuntimePlayerInputInteractionEffectFrameInput Input(
	iggy::runtime::RuntimePlayerInputGatedFrameStepInput playerInput,
	iggy::InteractionTarget2DRegistry interactionTargets,
	iggy::InteractionEffectCatalog2D interactionEffects,
	iggy::InteractionReach2DConfig interactionReach = {})
{
	return { playerInput, interactionTargets, interactionEffects, interactionReach };
}

iggy::physics2d::CollisionObject2D Object(iggy::ResourceId id, iggy::Aabb2 bounds)
{
	return { id, iggy::physics2d::makeAabbShape(bounds), true };
}

iggy::physics2d::CollisionWorld2D World(std::vector<iggy::physics2d::CollisionObject2D> objects)
{
	const iggy::physics2d::CollisionWorldBuildResult build = iggy::physics2d::CollisionWorld2DBuilder {}.build(objects);
	Expect(build.built, "player input interaction effect frame collision fixture should build");
	return build.world;
}

iggy::physics2d::CollisionWorld2D BlockingWorld()
{
	return World({
		Object(iggy::ResourceId { "wall:east" }, { { 0.5F, -0.5F }, { 1.5F, 0.5F } }),
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

bool SameEffect(const iggy::InteractionEffect2D &actual, const iggy::InteractionEffect2D &expected)
{
	return actual.type == expected.type
		&& actual.targetId == expected.targetId
		&& actual.eventId == expected.eventId
		&& actual.dropId == expected.dropId
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

void TestNoInteractionCommandsStillRunsPlayerInput()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameStep {}.run(
			Input(
				PlayerInput(session, { iggy::playerMoveToPointIntent({ 2.0F, 0.0F }) }),
				Registry({}),
				Catalog({})));

	Expect(result.playerInput.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, "no-interaction effect adapter should run player input");
	Expect(!result.interactions.hasInteractions(), "no-interaction effect adapter should report no interactions");
	Expect(result.interactions.readyCount == 0, "no-interaction effect adapter should have zero ready interactions");
	Expect(result.interactions.noEffectCount == 0, "no-interaction effect adapter should have zero no-effect interactions");
	Expect(result.interactions.blockedCount == 0, "no-interaction effect adapter should have zero blocked interactions");
	Expect(result.interactions.requestedEffectCount == 0, "no-interaction effect adapter should have zero requested effects");
	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "no-interaction effect adapter should preserve player movement result");
	Expect(SameQueue(result.queue, result.playerInput.queue), "effect adapter queue should come from player input result");
}

void TestReachableInteractWithEffectsProducesReadyDiagnostics()
{
	const iggy::InteractionTarget2D target = Target("target:ready_effects", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 0.0F);
	const std::vector<iggy::InteractionEffect2D> effects {
		iggy::inspectTextInteractionEffect(target.id, "Ready"),
		iggy::emitInteractionEventEffect(target.id, iggy::ResourceId { "event:ready" }),
	};

	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameStep {}.run(
			Input(
				PlayerInput(SessionWithPlayer({ 0.0F, 0.0F }), { iggy::playerInteractIntent(target.id) }),
				Registry({ target }),
				Catalog({ Entry("target:ready_effects", effects) })));

	Expect(result.interactions.interactions.size() == 1, "reachable interact with effects should produce one entry");
	Expect(result.interactions.readyCount == 1, "reachable interact with effects should count ready entry");
	Expect(result.interactions.noEffectCount == 0, "reachable interact with effects should count zero no-effect entries");
	Expect(result.interactions.blockedCount == 0, "reachable interact with effects should count zero blocked entries");
	Expect(result.interactions.requestedEffectCount == effects.size(), "reachable interact with effects should count requested effects");
	if (result.interactions.interactions.size() == 1) {
		Expect(result.interactions.interactions[0].commandIndex == 0, "reachable interact with effects should preserve command index");
		Expect(result.interactions.interactions[0].result.status == iggy::runtime::RuntimeInteractionEffectCommandStatus::Ready, "reachable interact with effects should report Ready status");
		Expect(SameEffects(result.interactions.interactions[0].result.effects.effects, effects), "reachable interact with effects should preserve effect order");
	}
}

void TestReachableInteractWithNoCatalogEntryProducesNoEffects()
{
	const iggy::InteractionTarget2D target = Target("target:no_catalog", iggy::InteractionTarget2DKind::Inspectable, { 0.0F, 0.0F }, 0.0F);

	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameStep {}.run(
			Input(
				PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(target.id) }),
				Registry({ target }),
				Catalog({})));

	Expect(result.interactions.interactions.size() == 1, "reachable interact without catalog should produce one entry");
	Expect(result.interactions.readyCount == 0, "reachable interact without catalog should count zero ready entries");
	Expect(result.interactions.noEffectCount == 1, "reachable interact without catalog should count one no-effect entry");
	Expect(result.interactions.blockedCount == 0, "reachable interact without catalog should count zero blocked entries");
	Expect(result.interactions.requestedEffectCount == 0, "reachable interact without catalog should request no effects");
	if (result.interactions.interactions.size() == 1)
		Expect(result.interactions.interactions[0].result.status == iggy::runtime::RuntimeInteractionEffectCommandStatus::NoEffects, "reachable interact without catalog should preserve NoEffects status");
}

void TestDisabledAndOutOfRangeTargetsProduceBlockedDiagnostics()
{
	const iggy::InteractionTarget2D disabled = Target("target:disabled", iggy::InteractionTarget2DKind::Talk, { 0.0F, 0.0F }, 2.0F, false);
	const iggy::InteractionTarget2D far = Target("target:far", iggy::InteractionTarget2DKind::Door, { 5.0F, 0.0F }, 1.0F);
	const iggy::InteractionEffectCatalog2D catalog = Catalog({
		Entry("target:disabled", { iggy::inspectTextInteractionEffect({}, "Disabled") }),
		Entry("target:far", { iggy::inspectTextInteractionEffect({}, "Far") }),
	});

	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameResult disabledResult =
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameStep {}.run(
			Input(
				PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(disabled.id) }),
				Registry({ disabled, far }),
				catalog));
	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameResult farResult =
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameStep {}.run(
			Input(
				PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(far.id) }),
				Registry({ disabled, far }),
				catalog));

	Expect(disabledResult.interactions.interactions.size() == 1, "disabled target should produce one effect diagnostic");
	Expect(farResult.interactions.interactions.size() == 1, "out-of-range target should produce one effect diagnostic");
	if (disabledResult.interactions.interactions.size() == 1)
		Expect(disabledResult.interactions.interactions[0].result.interaction.status == iggy::runtime::RuntimeInteractionCommandStatus::TargetDisabled, "disabled target should preserve nested disabled status");
	if (farResult.interactions.interactions.size() == 1)
		Expect(farResult.interactions.interactions[0].result.interaction.status == iggy::runtime::RuntimeInteractionCommandStatus::OutOfRange, "out-of-range target should preserve nested out-of-range status");
	Expect(disabledResult.interactions.blockedCount == 1 && farResult.interactions.blockedCount == 1, "blocked target diagnostics should count blocked entries");
	Expect(disabledResult.interactions.requestedEffectCount == 0 && farResult.interactions.requestedEffectCount == 0, "blocked target diagnostics should request no effects");
}

void TestContextBlockedInteractDoesNotReachInteractionDiagnostics()
{
	iggy::PlayerInputContext2D context;
	context.interactionEnabled = false;
	const iggy::InteractionTarget2D target = Target("target:blocked_context", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 1.0F);

	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameStep {}.run(
			Input(
				PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(target.id) }, context),
				Registry({ target }),
				Catalog({ Entry("target:blocked_context", { iggy::inspectTextInteractionEffect({}, "Blocked") }) })));

	Expect(result.playerInput.command.intake.mapping.gateIssues.size() == 1, "context-blocked interact should preserve gate issue");
	Expect(result.playerInput.command.intake.mapping.frame.commands.empty(), "context-blocked interact should not enter mapped command frame");
	Expect(!result.interactions.hasInteractions(), "context-blocked interact should produce no effect interaction entries");
	Expect(result.interactions.requestedEffectCount == 0, "context-blocked interact should request no effects");
}

void TestUnsupportedInspectIsMappingIssueNotInteractionDiagnostic()
{
	const iggy::InteractionTarget2D target = Target("target:inspect", iggy::InteractionTarget2DKind::Inspectable, { 0.0F, 0.0F }, 1.0F);

	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameStep {}.run(
			Input(
				PlayerInput(SessionWithPlayer(), { iggy::playerInspectIntent(target.id) }),
				Registry({ target }),
				Catalog({ Entry("target:inspect", { iggy::inspectTextInteractionEffect({}, "Inspect") }) })));

	Expect(result.playerInput.command.intake.mapping.gateIssues.empty(), "unsupported inspect should not be gate-blocked by default");
	Expect(result.playerInput.command.intake.mapping.mapping.issues.size() == 1, "unsupported inspect should preserve nested mapping issue");
	Expect(result.playerInput.command.intake.mapping.frame.commands.empty(), "unsupported inspect should not enter mapped command frame");
	Expect(!result.interactions.hasInteractions(), "unsupported inspect should produce no effect interaction entries");
}

void TestMoveThenInteractUsesPostMovePlayerPositionForEffects()
{
	const iggy::InteractionTarget2D target = Target("target:after_move", iggy::InteractionTarget2DKind::Usable, { 1.0F, 0.0F }, 0.0F);
	const std::vector<iggy::InteractionEffect2D> effects {
		iggy::toggleTargetInteractionEffect(target.id, false),
	};

	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameStep {}.run(
			Input(
				PlayerInput(SessionWithPlayer({ 0.0F, 0.0F }), {
					iggy::playerMoveToPointIntent({ 1.0F, 0.0F }),
					iggy::playerInteractIntent(target.id),
				}),
				Registry({ target }),
				Catalog({ Entry("target:after_move", effects) })));

	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "move-then-interact effect adapter should preserve post-move session");
	Expect(result.interactions.interactions.size() == 1, "move-then-interact effect adapter should produce one interaction entry");
	Expect(result.interactions.readyCount == 1, "move-then-interact effect adapter should count ready entry");
	Expect(result.interactions.requestedEffectCount == effects.size(), "move-then-interact effect adapter should count requested effects");
	if (result.interactions.interactions.size() == 1) {
		Expect(result.interactions.interactions[0].commandIndex == 1, "move-then-interact effect adapter should preserve interact command index");
		Expect(NearVec(result.interactions.interactions[0].result.interaction.plan.actorPosition, { 1.0F, 0.0F }), "move-then-interact effect plan should use post-move actor position");
		Expect(Near(result.interactions.interactions[0].result.interaction.plan.reach.distance, 0.0F), "move-then-interact effect reach should use post-move position");
		Expect(SameEffects(result.interactions.interactions[0].result.effects.effects, effects), "move-then-interact effect adapter should preserve requested effects");
	}
}

void TestExplicitWorldOverloadUsesResultingPlayerPositionForEffects()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	SetCollisionCache(session, BlockingWorld());
	const iggy::physics2d::CollisionWorld2D emptyWorld;
	const iggy::InteractionTarget2D target = Target("target:explicit", iggy::InteractionTarget2DKind::Usable, { 1.0F, 0.0F }, 0.0F);
	const std::vector<iggy::InteractionEffect2D> effects {
		iggy::emitInteractionEventEffect(target.id, iggy::ResourceId { "event:explicit" }),
	};

	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameStep {}.run(
			Input(
				PlayerInput(session, {
					iggy::playerMoveToPointIntent({ 2.0F, 0.0F }),
					iggy::playerInteractIntent(target.id),
				}),
				Registry({ target }),
				Catalog({ Entry("target:explicit", effects) })),
			emptyWorld);

	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "explicit world effect adapter should preserve movement override result");
	Expect(result.interactions.interactions.size() == 1, "explicit world effect adapter should produce interaction diagnostic");
	if (result.interactions.interactions.size() == 1) {
		Expect(result.interactions.interactions[0].result.status == iggy::runtime::RuntimeInteractionEffectCommandStatus::Ready, "explicit world effect adapter should evaluate interaction from moved position");
		Expect(NearVec(result.interactions.interactions[0].result.interaction.plan.actorPosition, { 1.0F, 0.0F }), "explicit world effect plan should use moved actor position");
		Expect(SameEffects(result.interactions.interactions[0].result.effects.effects, effects), "explicit world effect adapter should preserve requested effects");
	}
}

void TestQueueRejectedComputesDiagnosticsFromMappedFrameWithoutExecuting()
{
	const iggy::InteractionTarget2D target = Target("target:queued", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 0.0F);
	const std::vector<iggy::InteractionEffect2D> effects {
		iggy::inspectTextInteractionEffect(target.id, "Queued"),
	};
	const iggy::runtime::RuntimeCommandQueueState fullQueue { { WaitFrame() } };
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });

	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameStep {}.run(
			Input(
				PlayerInput(
					session,
					{ iggy::playerInteractIntent(target.id) },
					{},
					fullQueue,
					{ 1 }),
				Registry({ target }),
				Catalog({ Entry("target:queued", effects) })));

	Expect(result.playerInput.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::QueueRejected, "queue rejected effect adapter should preserve player input rejection");
	Expect(result.playerInput.report.tickResultCount == 0, "queue rejected effect adapter should not run command ticks");
	Expect(result.session.tickIndex == session.tickIndex, "queue rejected effect adapter should not advance session");
	ExpectPlayerAgent(result.session.player, session.player, "queue rejected effect adapter session");
	Expect(SameQueue(result.queue, fullQueue), "queue rejected effect adapter should preserve input queue");
	Expect(result.interactions.interactions.size() == 1, "queue rejected effect adapter should still evaluate mapped interact frame");
	Expect(result.interactions.readyCount == 1, "queue rejected effect adapter should count mapped ready interaction");
	Expect(result.interactions.requestedEffectCount == effects.size(), "queue rejected effect adapter should preserve requested effects");
}

void TestInputsAreNotMutated()
{
	const iggy::InteractionTarget2D target = Target("target:immutable", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 1.0F);
	const std::vector<iggy::InteractionEffect2D> effects {
		iggy::inspectTextInteractionEffect(target.id, "Text"),
	};
	iggy::runtime::RuntimePlayerInputInteractionEffectFrameInput input = Input(
		PlayerInput(
			SessionWithPlayer({ 0.0F, 0.0F }),
			{ iggy::playerInteractIntent(target.id), iggy::playerCancelIntent() },
			{},
			{ { WaitFrame() } },
			{ 3 }),
		Registry({ target }),
		Catalog({ Entry("target:immutable", effects) }),
		{ 0.5F });
	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameInput before = input;

	const iggy::runtime::RuntimePlayerInputInteractionEffectFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionEffectFrameStep {}.run(input);

	Expect(result.playerInput.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, "effect adapter immutability setup should run");
	Expect(input.playerInput.commandInput.session.tickIndex == before.playerInput.commandInput.session.tickIndex, "effect adapter should not mutate input session tickIndex");
	ExpectPlayerAgent(input.playerInput.commandInput.session.player, before.playerInput.commandInput.session.player, "effect adapter input session");
	Expect(SameQueue(input.playerInput.commandInput.queue, before.playerInput.commandInput.queue), "effect adapter should not mutate input queue");
	Expect(input.playerInput.commandInput.intents.size() == before.playerInput.commandInput.intents.size(), "effect adapter should not mutate input intents");
	Expect(input.interactionTargets.targets().size() == before.interactionTargets.targets().size(), "effect adapter should not mutate target registry");
	Expect(input.interactionEffects.entries().size() == before.interactionEffects.entries().size(), "effect adapter should not mutate effect catalog");
	if (input.interactionTargets.targets().size() == before.interactionTargets.targets().size())
		Expect(input.interactionTargets.targets()[0].id == before.interactionTargets.targets()[0].id, "effect adapter should preserve target id");
	if (input.interactionEffects.entries().size() == before.interactionEffects.entries().size() && !input.interactionEffects.entries().empty())
		Expect(SameEffects(input.interactionEffects.entries()[0].effects, before.interactionEffects.entries()[0].effects), "effect adapter should preserve catalog effects");
	Expect(input.interactionReach.extraReach == before.interactionReach.extraReach, "effect adapter should not mutate reach config");
}

} // namespace

int main()
{
	TestNoInteractionCommandsStillRunsPlayerInput();
	TestReachableInteractWithEffectsProducesReadyDiagnostics();
	TestReachableInteractWithNoCatalogEntryProducesNoEffects();
	TestDisabledAndOutOfRangeTargetsProduceBlockedDiagnostics();
	TestContextBlockedInteractDoesNotReachInteractionDiagnostics();
	TestUnsupportedInspectIsMappingIssueNotInteractionDiagnostic();
	TestMoveThenInteractUsesPostMovePlayerPositionForEffects();
	TestExplicitWorldOverloadUsesResultingPlayerPositionForEffects();
	TestQueueRejectedComputesDiagnosticsFromMappedFrameWithoutExecuting();
	TestInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
