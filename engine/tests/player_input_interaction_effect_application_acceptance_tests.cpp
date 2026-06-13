#include <cstdlib>
#include <vector>

#include "runtime/RuntimePlayerInputInteractionEffectApplyFrameStep.hpp"
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

const iggy::ResourceId PlayerId { "player:input-interaction-effect-application" };

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
	session.level.map.id = iggy::ResourceId { "level:input-interaction-effect-application" };
	session.level.map.playerStart = { 0, 0 };
	session.tickIndex = 67;
	session.hasPlayer = true;
	session.player = PlayerAgent(PlayerId, position, { 0, 0 }, iggy::PlayerMovementStatus::Idle, iggy::PlayerFacing2D::East);
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
	Expect(result.built, "player input interaction effect application registry should build");
	return result.registry;
}

iggy::InteractionEffectEntry2D Entry(const char *targetId, std::vector<iggy::InteractionEffect2D> effects)
{
	return { iggy::ResourceId { targetId }, effects };
}

iggy::InteractionEffectCatalog2D Catalog(std::vector<iggy::InteractionEffectEntry2D> entries)
{
	const iggy::InteractionEffectCatalog2DBuildResult result = iggy::InteractionEffectCatalog2DBuilder {}.build(entries);
	Expect(result.built, "player input interaction effect application catalog should build");
	return result.catalog;
}

iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameInput Input(
	iggy::runtime::RuntimePlayerInputGatedFrameStepInput playerInput,
	iggy::InteractionTarget2DRegistry targets,
	iggy::InteractionEffectCatalog2D effects,
	iggy::InteractionReach2DConfig reach = {})
{
	return { playerInput, targets, effects, reach };
}

iggy::runtime::RuntimeInteractionState InteractionState(
	iggy::InteractionTarget2DRegistry targets,
	iggy::InteractionEffectCatalog2D effects)
{
	return { targets, effects };
}

iggy::runtime::RuntimePlayerInputInteractionStateApplyFrameInput StateInput(
	iggy::runtime::RuntimePlayerInputGatedFrameStepInput playerInput,
	iggy::runtime::RuntimeInteractionState interaction,
	iggy::InteractionReach2DConfig reach = {})
{
	return { playerInput, interaction, reach };
}

iggy::physics2d::CollisionObject2D Object(iggy::ResourceId id, iggy::Aabb2 bounds)
{
	return { id, iggy::physics2d::makeAabbShape(bounds), true };
}

iggy::physics2d::CollisionWorld2D World(std::vector<iggy::physics2d::CollisionObject2D> objects)
{
	const iggy::physics2d::CollisionWorldBuildResult build = iggy::physics2d::CollisionWorld2DBuilder {}.build(objects);
	Expect(build.built, "player input interaction effect application collision fixture should build");
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
		&& actual.text == expected.text
		&& actual.enabledValue == expected.enabledValue;
}

bool SameEvent(const iggy::InteractionEvent2D &actual, const iggy::InteractionEvent2D &expected)
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

void ExpectEvents(
	const iggy::InteractionEventRecorder2D &recorder,
	const std::vector<iggy::InteractionEvent2D> &expected,
	const char *message)
{
	Expect(recorder.events.size() == expected.size(), message);
	if (recorder.events.size() != expected.size())
		return;
	for (std::size_t index = 0; index < expected.size(); ++index)
		Expect(SameEvent(recorder.events[index], expected[index]), message);
}

void ExpectTarget(const iggy::InteractionTarget2D &actual, const iggy::InteractionTarget2D &expected, const char *message)
{
	Expect(actual.id == expected.id, message);
	Expect(actual.kind == expected.kind, message);
	Expect(NearVec(actual.position, expected.position), message);
	Expect(actual.radius == expected.radius, message);
	Expect(actual.enabled == expected.enabled, message);
}

void ExpectRegistryTargets(
	const iggy::InteractionTarget2DRegistry &registry,
	const std::vector<iggy::InteractionTarget2D> &expected,
	const char *message)
{
	Expect(registry.targets().size() == expected.size(), message);
	if (registry.targets().size() != expected.size())
		return;
	for (std::size_t index = 0; index < expected.size(); ++index)
		ExpectTarget(registry.targets()[index], expected[index], message);
}

void TestReachableToggleTargetAppliesToReturnedRegistryOnly()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:a", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 0.0F),
		Target("target:b", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 0.0F, true),
	};
	std::vector<iggy::InteractionTarget2D> expected = targets;
	expected[1].enabled = false;
	const iggy::InteractionTarget2DRegistry originalRegistry = Registry(targets);

	const iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameStep {}.run(
			Input(
				PlayerInput(SessionWithPlayer({ 0.0F, 0.0F }), { iggy::playerInteractIntent(targets[0].id) }),
				originalRegistry,
				Catalog({ Entry("target:a", { iggy::toggleTargetInteractionEffect(targets[1].id, false) }) })));

	Expect(result.application.status == iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::Applied, "reachable toggle acceptance should apply");
	Expect(result.application.appliedCount == 1, "reachable toggle acceptance should count applied interaction");
	Expect(result.application.noOpCount == 0 && result.application.notReadyCount == 0 && result.application.failedCount == 0, "reachable toggle acceptance should have no other counts");
	Expect(result.application.mutated, "reachable toggle acceptance should mark mutation");
	ExpectEvents(
		result.events,
		{ iggy::targetToggledInteractionEvent(targets[1].id, false) },
		"reachable toggle acceptance should surface top-level target toggled event");
	ExpectRegistryTargets(result.interactionTargets, expected, "reachable toggle acceptance should return updated registry");
	ExpectRegistryTargets(originalRegistry, targets, "reachable toggle acceptance should not mutate original registry");
}

void TestDeferredOnlyEffectsDoNotMutateRegistry()
{
	const iggy::InteractionTarget2D target = Target("target:deferred", iggy::InteractionTarget2DKind::Inspectable, { 0.0F, 0.0F }, 0.0F);
	const std::vector<iggy::InteractionEffect2D> effects {
		iggy::inspectTextInteractionEffect(target.id, "Read this."),
		iggy::emitInteractionEventEffect(target.id, iggy::ResourceId { "event:read" }),
	};

	const iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameStep {}.run(
			Input(
				PlayerInput(SessionWithPlayer({ 0.0F, 0.0F }), { iggy::playerInteractIntent(target.id) }),
				Registry({ target }),
				Catalog({ Entry("target:deferred", effects) })));

	Expect(result.application.status == iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::NoOp, "deferred-only acceptance should be no-op");
	Expect(result.application.noOpCount == 1, "deferred-only acceptance should count one no-op interaction");
	Expect(!result.application.mutated, "deferred-only acceptance should not mutate");
	ExpectEvents(
		result.events,
		{
			iggy::inspectTextRequestedInteractionEvent(target.id, "Read this."),
			iggy::interactionEventEmitted(target.id, iggy::ResourceId { "event:read" }),
		},
		"deferred-only acceptance should surface top-level deferred events");
	ExpectRegistryTargets(result.interactionTargets, { target }, "deferred-only acceptance should return original registry");
	if (result.application.entries.size() == 1) {
		Expect(result.application.entries[0].result.application.deferredCount == 2, "deferred-only acceptance should preserve deferred count");
		Expect(SameEffects(result.application.entries[0].result.command.effects.effects, effects), "deferred-only acceptance should preserve requested effects");
	}
}

void TestDisabledAndOutOfRangeInteractionsDoNotApplyEffects()
{
	const iggy::InteractionTarget2D disabled = Target("target:disabled", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 1.0F, false);
	const iggy::InteractionTarget2D far = Target("target:far", iggy::InteractionTarget2DKind::Usable, { 4.0F, 0.0F }, 0.5F, true);
	const iggy::InteractionEffectCatalog2D effects = Catalog({
		Entry("target:disabled", { iggy::toggleTargetInteractionEffect(disabled.id, true) }),
		Entry("target:far", { iggy::toggleTargetInteractionEffect(far.id, false) }),
	});

	const iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameResult disabledResult =
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameStep {}.run(
			Input(PlayerInput(SessionWithPlayer({ 0.0F, 0.0F }), { iggy::playerInteractIntent(disabled.id) }), Registry({ disabled, far }), effects));
	const iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameResult farResult =
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameStep {}.run(
			Input(PlayerInput(SessionWithPlayer({ 0.0F, 0.0F }), { iggy::playerInteractIntent(far.id) }), Registry({ disabled, far }), effects));

	Expect(disabledResult.application.notReadyCount == 1, "disabled acceptance should count not-ready");
	Expect(farResult.application.notReadyCount == 1, "out-of-range acceptance should count not-ready");
	Expect(!disabledResult.application.mutated && !farResult.application.mutated, "not-ready acceptance should not mutate");
	ExpectEvents(disabledResult.events, {}, "disabled acceptance should surface no top-level events");
	ExpectEvents(farResult.events, {}, "out-of-range acceptance should surface no top-level events");
	ExpectRegistryTargets(disabledResult.interactionTargets, { disabled, far }, "disabled acceptance should return original registry");
	ExpectRegistryTargets(farResult.interactionTargets, { disabled, far }, "out-of-range acceptance should return original registry");
}

void TestMoveThenInteractAppliesAfterMovement()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:after_move", iggy::InteractionTarget2DKind::Usable, { 1.0F, 0.0F }, 0.0F, true),
	};
	std::vector<iggy::InteractionTarget2D> expected = targets;
	expected[0].enabled = false;

	const iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameStep {}.run(
			Input(
				PlayerInput(SessionWithPlayer({ 0.0F, 0.0F }), {
					iggy::playerMoveToPointIntent({ 1.0F, 0.0F }),
					iggy::playerInteractIntent(targets[0].id),
				}),
				Registry(targets),
				Catalog({ Entry("target:after_move", { iggy::toggleTargetInteractionEffect(targets[0].id, false) }) })));

	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "move-then-interact acceptance should use post-move session");
	Expect(result.application.status == iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::Applied, "move-then-interact acceptance should apply effect");
	ExpectEvents(
		result.events,
		{ iggy::targetToggledInteractionEvent(targets[0].id, false) },
		"move-then-interact acceptance should surface event only when post-move application occurs");
	ExpectRegistryTargets(result.interactionTargets, expected, "move-then-interact acceptance should return updated registry");
	if (result.application.entries.size() == 1)
		Expect(NearVec(result.application.entries[0].result.command.interaction.plan.actorPosition, { 1.0F, 0.0F }), "move-then-interact acceptance should evaluate reach from post-move position");
}

void TestContextBlockedInteractDoesNotApply()
{
	iggy::PlayerInputContext2D context;
	context.interactionEnabled = false;
	const iggy::InteractionTarget2D target = Target("target:context", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 1.0F);

	const iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameStep {}.run(
			Input(
				PlayerInput(SessionWithPlayer({ 0.0F, 0.0F }), { iggy::playerInteractIntent(target.id) }, context),
				Registry({ target }),
				Catalog({ Entry("target:context", { iggy::toggleTargetInteractionEffect(target.id, false) }) })));

	Expect(result.playerInput.command.intake.mapping.gateIssues.size() == 1, "context-blocked acceptance should preserve gate issue");
	Expect(result.playerInput.command.intake.mapping.frame.commands.empty(), "context-blocked acceptance should map no command");
	Expect(!result.application.hasInteractions(), "context-blocked acceptance should have no application entries");
	Expect(!result.application.mutated, "context-blocked acceptance should not mutate");
	ExpectEvents(result.events, {}, "context-blocked acceptance should surface no top-level events");
	ExpectRegistryTargets(result.interactionTargets, { target }, "context-blocked acceptance should return original registry");
}

void TestQueueRejectedMappedInteractDoesNotApply()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:queue_rejected", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 0.0F, true),
	};
	const iggy::runtime::RuntimeCommandQueueState fullQueue { { WaitFrame() } };
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });

	const iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameStep {}.run(
			Input(
				PlayerInput(
					session,
					{ iggy::playerInteractIntent(targets[0].id) },
					{},
					fullQueue,
					{ 1 }),
				Registry(targets),
				Catalog({ Entry("target:queue_rejected", { iggy::toggleTargetInteractionEffect(targets[0].id, false) }) })));

	Expect(result.playerInput.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::QueueRejected, "queue-rejected acceptance should preserve player input rejection");
	Expect(result.playerInput.command.intake.mapping.frame.commands.size() == 1, "queue-rejected acceptance should preserve mapped frame diagnostics");
	Expect(result.playerInput.report.tickResultCount == 0, "queue-rejected acceptance should not execute command ticks");
	Expect(SameQueue(result.queue, fullQueue), "queue-rejected acceptance should preserve queue");
	Expect(result.application.status == iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::NoOp, "queue-rejected acceptance should have no-op application");
	Expect(!result.application.hasInteractions(), "queue-rejected acceptance should not apply effects");
	Expect(result.application.appliedCount == 0 && result.application.noOpCount == 0 && result.application.notReadyCount == 0 && result.application.failedCount == 0, "queue-rejected acceptance should have zero application counts");
	Expect(!result.application.mutated, "queue-rejected acceptance should not mutate");
	ExpectEvents(result.events, {}, "queue-rejected acceptance should surface no top-level events");
	ExpectRegistryTargets(result.interactionTargets, targets, "queue-rejected acceptance should return original registry");
}

void TestExplicitCollisionOverrideAllowsApplicationAfterMovement()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	SetCollisionCache(session, BlockingWorld());
	const iggy::physics2d::CollisionWorld2D emptyWorld;
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:explicit", iggy::InteractionTarget2DKind::Usable, { 1.0F, 0.0F }, 0.0F, true),
	};
	std::vector<iggy::InteractionTarget2D> expected = targets;
	expected[0].enabled = false;

	const iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameStep {}.run(
			Input(
				PlayerInput(session, {
					iggy::playerMoveToPointIntent({ 2.0F, 0.0F }),
					iggy::playerInteractIntent(targets[0].id),
				}),
				Registry(targets),
				Catalog({ Entry("target:explicit", { iggy::toggleTargetInteractionEffect(targets[0].id, false) }) })),
			emptyWorld);

	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "explicit collision acceptance should use explicit world movement");
	Expect(result.application.status == iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::Applied, "explicit collision acceptance should apply effect after movement");
	ExpectEvents(
		result.events,
		{ iggy::targetToggledInteractionEvent(targets[0].id, false) },
		"explicit collision acceptance should surface top-level target toggled event");
	ExpectRegistryTargets(result.interactionTargets, expected, "explicit collision acceptance should return updated registry");
}

void TestStateBasedOverloadCarriesTopLevelEvents()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:state_event", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 0.0F, true),
	};
	std::vector<iggy::InteractionTarget2D> expected = targets;
	expected[0].enabled = false;
	const iggy::InteractionEffectCatalog2D effects = Catalog({
		Entry("target:state_event", { iggy::toggleTargetInteractionEffect(targets[0].id, false) }),
	});
	const iggy::runtime::RuntimeInteractionState interaction = InteractionState(Registry(targets), effects);

	const iggy::runtime::RuntimePlayerInputInteractionStateApplyFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameStep {}.runWithInteractionState(
			StateInput(
				PlayerInput(SessionWithPlayer({ 0.0F, 0.0F }), { iggy::playerInteractIntent(targets[0].id) }),
				interaction));

	Expect(result.application.status == iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::Applied, "state event acceptance should apply");
	Expect(result.application.mutated, "state event acceptance should mark mutation");
	ExpectEvents(
		result.events,
		{ iggy::targetToggledInteractionEvent(targets[0].id, false) },
		"state event acceptance should surface top-level target toggled event");
	ExpectRegistryTargets(result.interaction.targets, expected, "state event acceptance should return updated interaction state targets");
	ExpectRegistryTargets(interaction.targets, targets, "state event acceptance should not mutate input interaction state");
}

} // namespace

int main()
{
	TestReachableToggleTargetAppliesToReturnedRegistryOnly();
	TestDeferredOnlyEffectsDoNotMutateRegistry();
	TestDisabledAndOutOfRangeInteractionsDoNotApplyEffects();
	TestMoveThenInteractAppliesAfterMovement();
	TestContextBlockedInteractDoesNotApply();
	TestQueueRejectedMappedInteractDoesNotApply();
	TestExplicitCollisionOverrideAllowsApplicationAfterMovement();
	TestStateBasedOverloadCarriesTopLevelEvents();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
