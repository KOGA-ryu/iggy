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

const iggy::ResourceId PlayerId { "player:input-interaction-effect-apply-frame" };

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
	session.level.map = MapFromRows({ "...", "..." });
	session.level.map.id = iggy::ResourceId { "level:input-interaction-effect-apply-frame" };
	session.level.map.playerStart = { 0, 0 };
	session.tickIndex = 53;
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
	iggy::InteractionTarget2DKind kind = iggy::InteractionTarget2DKind::Usable,
	iggy::Vec2 position = { 0.0F, 0.0F },
	float radius = 0.0F,
	bool enabled = true)
{
	return { iggy::ResourceId { id }, kind, position, radius, enabled };
}

iggy::InteractionTarget2DRegistry Registry(std::vector<iggy::InteractionTarget2D> targets)
{
	const iggy::InteractionTarget2DRegistryBuildResult result = iggy::InteractionTarget2DRegistryBuilder {}.build(targets);
	Expect(result.built, "player input interaction effect apply frame registry setup should build");
	return result.registry;
}

iggy::InteractionEffectEntry2D Entry(const char *targetId, std::vector<iggy::InteractionEffect2D> effects)
{
	return { iggy::ResourceId { targetId }, effects };
}

iggy::InteractionEffectCatalog2D Catalog(std::vector<iggy::InteractionEffectEntry2D> entries)
{
	const iggy::InteractionEffectCatalog2DBuildResult result = iggy::InteractionEffectCatalog2DBuilder {}.build(entries);
	Expect(result.built, "player input interaction effect apply frame catalog setup should build");
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

iggy::physics2d::CollisionObject2D Object(iggy::ResourceId id, iggy::Aabb2 bounds)
{
	return { id, iggy::physics2d::makeAabbShape(bounds), true };
}

iggy::physics2d::CollisionWorld2D World(std::vector<iggy::physics2d::CollisionObject2D> objects)
{
	const iggy::physics2d::CollisionWorldBuildResult build = iggy::physics2d::CollisionWorld2DBuilder {}.build(objects);
	Expect(build.built, "player input interaction effect apply frame collision fixture should build");
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

void ExpectCatalogPreserved(
	const iggy::InteractionEffectCatalog2D &actual,
	const iggy::InteractionEffectCatalog2D &expected,
	const char *message)
{
	Expect(actual.entries().size() == expected.entries().size(), message);
	if (actual.entries().size() != expected.entries().size())
		return;
	for (std::size_t entryIndex = 0; entryIndex < actual.entries().size(); ++entryIndex) {
		Expect(actual.entries()[entryIndex].targetId == expected.entries()[entryIndex].targetId, message);
		Expect(SameEffects(actual.entries()[entryIndex].effects, expected.entries()[entryIndex].effects), message);
	}
}

void TestNoInteractionCommandsStillRunsPlayerInputAndDoesNotMutateRegistry()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const iggy::InteractionTarget2D target = Target("target:no_interact");
	const iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameStep {}.run(
			Input(
				PlayerInput(session, { iggy::playerMoveToPointIntent({ 2.0F, 0.0F }) }),
				Registry({ target }),
				Catalog({ Entry("target:no_interact", { iggy::toggleTargetInteractionEffect(target.id, false) }) })));

	Expect(result.playerInput.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, "no-interaction apply adapter should run player input");
	Expect(result.application.status == iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::NoOp, "no-interaction apply adapter should return no-op application");
	Expect(!result.application.hasInteractions(), "no-interaction apply adapter should have no application entries");
	Expect(!result.application.mutated, "no-interaction apply adapter should not mutate interaction registry");
	ExpectRegistryTargets(result.interactionTargets, { target }, "no-interaction apply adapter should return original interaction registry");
	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "no-interaction apply adapter should preserve player movement result");
	Expect(SameQueue(result.queue, result.playerInput.queue), "no-interaction apply adapter queue should come from player input result");
}

void TestReachableInteractWithToggleUpdatesReturnedRegistry()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:toggle", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 0.0F, true),
	};
	std::vector<iggy::InteractionTarget2D> expected = targets;
	expected[0].enabled = false;

	const iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameStep {}.run(
			Input(
				PlayerInput(SessionWithPlayer({ 0.0F, 0.0F }), { iggy::playerInteractIntent(targets[0].id) }),
				Registry(targets),
				Catalog({ Entry("target:toggle", { iggy::toggleTargetInteractionEffect(targets[0].id, false) }) })));

	Expect(result.application.status == iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::Applied, "reachable toggle apply adapter should return Applied");
	Expect(result.application.appliedCount == 1, "reachable toggle apply adapter should count applied interaction");
	Expect(result.application.mutated, "reachable toggle apply adapter should mark mutation");
	ExpectRegistryTargets(result.interactionTargets, expected, "reachable toggle apply adapter should return updated registry");
	ExpectRegistryTargets(result.application.registry, expected, "reachable toggle apply adapter should preserve application registry");
}

void TestDeferredOnlyEffectsDoNotMutateRegistry()
{
	const iggy::InteractionTarget2D target = Target("target:deferred", iggy::InteractionTarget2DKind::Inspectable, { 0.0F, 0.0F }, 0.0F);
	const std::vector<iggy::InteractionEffect2D> effects {
		iggy::inspectTextInteractionEffect(target.id, "Read"),
		iggy::emitInteractionEventEffect(target.id, iggy::ResourceId { "event:read" }),
	};

	const iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameStep {}.run(
			Input(
				PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(target.id) }),
				Registry({ target }),
				Catalog({ Entry("target:deferred", effects) })));

	Expect(result.application.status == iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::NoOp, "deferred apply adapter should return NoOp");
	Expect(result.application.noOpCount == 1, "deferred apply adapter should count no-op interaction");
	Expect(!result.application.mutated, "deferred apply adapter should not mutate");
	ExpectRegistryTargets(result.interactionTargets, { target }, "deferred apply adapter should return original registry");
	if (result.application.entries.size() == 1) {
		Expect(result.application.entries[0].result.application.deferredCount == 2, "deferred apply adapter should preserve deferred diagnostics");
		Expect(SameEffects(result.application.entries[0].result.command.effects.effects, effects), "deferred apply adapter should preserve effect order");
	}
}

void TestDisabledAndOutOfRangeTargetsDoNotMutateRegistry()
{
	const iggy::InteractionTarget2D disabled = Target("target:disabled", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 1.0F, false);
	const iggy::InteractionTarget2D far = Target("target:far", iggy::InteractionTarget2DKind::Usable, { 4.0F, 0.0F }, 0.5F);
	const iggy::InteractionEffectCatalog2D effects = Catalog({
		Entry("target:disabled", { iggy::toggleTargetInteractionEffect(disabled.id, true) }),
		Entry("target:far", { iggy::toggleTargetInteractionEffect(far.id, false) }),
	});

	const iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameResult disabledResult =
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameStep {}.run(
			Input(PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(disabled.id) }), Registry({ disabled, far }), effects));
	const iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameResult farResult =
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameStep {}.run(
			Input(PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(far.id) }), Registry({ disabled, far }), effects));

	Expect(disabledResult.application.notReadyCount == 1, "disabled target apply adapter should count not-ready interaction");
	Expect(farResult.application.notReadyCount == 1, "out-of-range target apply adapter should count not-ready interaction");
	Expect(!disabledResult.application.mutated && !farResult.application.mutated, "blocked target apply adapters should not mutate");
	ExpectRegistryTargets(disabledResult.interactionTargets, { disabled, far }, "disabled target apply adapter should return original registry");
	ExpectRegistryTargets(farResult.interactionTargets, { disabled, far }, "out-of-range target apply adapter should return original registry");
	if (disabledResult.application.entries.size() == 1)
		Expect(disabledResult.application.entries[0].result.command.interaction.status == iggy::runtime::RuntimeInteractionCommandStatus::TargetDisabled, "disabled target apply adapter should preserve disabled status");
	if (farResult.application.entries.size() == 1)
		Expect(farResult.application.entries[0].result.command.interaction.status == iggy::runtime::RuntimeInteractionCommandStatus::OutOfRange, "out-of-range target apply adapter should preserve out-of-range status");
}

void TestContextBlockedInteractDoesNotEnterApplicationFrame()
{
	iggy::PlayerInputContext2D context;
	context.interactionEnabled = false;
	const iggy::InteractionTarget2D target = Target("target:context", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 1.0F);

	const iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameStep {}.run(
			Input(
				PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(target.id) }, context),
				Registry({ target }),
				Catalog({ Entry("target:context", { iggy::toggleTargetInteractionEffect(target.id, false) }) })));

	Expect(result.playerInput.command.intake.mapping.gateIssues.size() == 1, "context-blocked apply adapter should preserve gate issue");
	Expect(result.playerInput.command.intake.mapping.frame.commands.empty(), "context-blocked apply adapter should map no commands");
	Expect(!result.application.hasInteractions(), "context-blocked apply adapter should apply no interaction entries");
	Expect(!result.application.mutated, "context-blocked apply adapter should not mutate");
	ExpectRegistryTargets(result.interactionTargets, { target }, "context-blocked apply adapter should return original registry");
}

void TestMoveThenInteractUsesPostMovePositionAndAppliesToggle()
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

	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "move-then-interact apply adapter should preserve post-move session");
	Expect(result.application.status == iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::Applied, "move-then-interact apply adapter should apply effect");
	ExpectRegistryTargets(result.interactionTargets, expected, "move-then-interact apply adapter should return toggled registry");
	if (result.application.entries.size() == 1) {
		Expect(result.application.entries[0].commandIndex == 1, "move-then-interact apply adapter should preserve interact command index");
		Expect(NearVec(result.application.entries[0].result.command.interaction.plan.actorPosition, { 1.0F, 0.0F }), "move-then-interact apply adapter should use post-move actor position");
	}
}

void TestExplicitWorldOverloadAffectsMovementAndReachForApplication()
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

	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "explicit world apply adapter should preserve movement override result");
	Expect(result.application.status == iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::Applied, "explicit world apply adapter should apply after movement override");
	ExpectRegistryTargets(result.interactionTargets, expected, "explicit world apply adapter should return toggled registry");
	if (result.application.entries.size() == 1)
		Expect(NearVec(result.application.entries[0].result.command.interaction.plan.actorPosition, { 1.0F, 0.0F }), "explicit world apply adapter should use moved actor position");
}

void TestQueueRejectedSkipsEffectApplicationAndPreservesDiagnostics()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:queued", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 0.0F, true),
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
				Catalog({ Entry("target:queued", { iggy::toggleTargetInteractionEffect(targets[0].id, false) }) })));

	Expect(result.playerInput.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::QueueRejected, "queue-rejected apply adapter should preserve player input rejection");
	Expect(result.playerInput.report.tickResultCount == 0, "queue-rejected apply adapter should not execute command ticks");
	Expect(result.session.tickIndex == session.tickIndex, "queue-rejected apply adapter should not advance session");
	ExpectPlayerAgent(result.session.player, session.player, "queue-rejected apply adapter session");
	Expect(SameQueue(result.queue, fullQueue), "queue-rejected apply adapter should preserve input queue");
	Expect(result.playerInput.command.intake.mapping.frame.commands.size() == 1, "queue-rejected apply adapter should preserve mapped interact diagnostics");
	Expect(result.application.status == iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::NoOp, "queue-rejected apply adapter should return default no-op application");
	Expect(!result.application.hasInteractions(), "queue-rejected apply adapter should not apply mapped frame");
	Expect(result.application.appliedCount == 0 && result.application.noOpCount == 0 && result.application.notReadyCount == 0 && result.application.failedCount == 0, "queue-rejected apply adapter should have zero application counts");
	Expect(!result.application.mutated, "queue-rejected apply adapter should not mutate interaction registry");
	ExpectRegistryTargets(result.application.registry, targets, "queue-rejected apply adapter application should preserve original registry");
	ExpectRegistryTargets(result.interactionTargets, targets, "queue-rejected apply adapter should return original registry");
}

void TestQueueRejectedExplicitWorldAlsoSkipsEffectApplication()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:queued_explicit", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 0.0F, true),
	};
	const iggy::runtime::RuntimeCommandQueueState fullQueue { { WaitFrame() } };
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const iggy::physics2d::CollisionWorld2D emptyWorld;

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
				Catalog({ Entry("target:queued_explicit", { iggy::toggleTargetInteractionEffect(targets[0].id, false) }) })),
			emptyWorld);

	Expect(result.playerInput.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::QueueRejected, "explicit queue-rejected apply adapter should preserve player input rejection");
	Expect(result.playerInput.command.intake.mapping.frame.commands.size() == 1, "explicit queue-rejected apply adapter should preserve mapped interact diagnostics");
	Expect(!result.application.hasInteractions(), "explicit queue-rejected apply adapter should not apply mapped frame");
	Expect(!result.application.mutated, "explicit queue-rejected apply adapter should not mutate interaction registry");
	ExpectRegistryTargets(result.interactionTargets, targets, "explicit queue-rejected apply adapter should return original registry");
}

void TestInputsAreNotMutated()
{
	const iggy::InteractionTarget2D target = Target("target:immutable", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 1.0F);
	iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameInput input = Input(
		PlayerInput(
			SessionWithPlayer({ 0.0F, 0.0F }),
			{ iggy::playerInteractIntent(target.id), iggy::playerCancelIntent() },
			{},
			{ { WaitFrame() } },
			{ 3 }),
		Registry({ target }),
		Catalog({ Entry("target:immutable", { iggy::toggleTargetInteractionEffect(target.id, false) }) }),
		{ 0.5F });
	const iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameInput before = input;

	const iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionEffectApplyFrameStep {}.run(input);

	Expect(result.playerInput.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, "apply adapter immutability setup should run");
	Expect(input.playerInput.commandInput.session.tickIndex == before.playerInput.commandInput.session.tickIndex, "apply adapter should not mutate input session tickIndex");
	ExpectPlayerAgent(input.playerInput.commandInput.session.player, before.playerInput.commandInput.session.player, "apply adapter input session");
	Expect(SameQueue(input.playerInput.commandInput.queue, before.playerInput.commandInput.queue), "apply adapter should not mutate input queue");
	Expect(input.playerInput.commandInput.intents.size() == before.playerInput.commandInput.intents.size(), "apply adapter should not mutate input intents");
	ExpectRegistryTargets(input.interactionTargets, before.interactionTargets.targets(), "apply adapter should not mutate target registry");
	ExpectCatalogPreserved(input.interactionEffects, before.interactionEffects, "apply adapter should not mutate effect catalog");
	Expect(input.interactionReach.extraReach == before.interactionReach.extraReach, "apply adapter should not mutate reach config");
}

} // namespace

int main()
{
	TestNoInteractionCommandsStillRunsPlayerInputAndDoesNotMutateRegistry();
	TestReachableInteractWithToggleUpdatesReturnedRegistry();
	TestDeferredOnlyEffectsDoNotMutateRegistry();
	TestDisabledAndOutOfRangeTargetsDoNotMutateRegistry();
	TestContextBlockedInteractDoesNotEnterApplicationFrame();
	TestMoveThenInteractUsesPostMovePositionAndAppliesToggle();
	TestExplicitWorldOverloadAffectsMovementAndReachForApplication();
	TestQueueRejectedSkipsEffectApplicationAndPreservesDiagnostics();
	TestQueueRejectedExplicitWorldAlsoSkipsEffectApplication();
	TestInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
