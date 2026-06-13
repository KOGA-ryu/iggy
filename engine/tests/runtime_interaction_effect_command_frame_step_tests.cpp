#include <cstdlib>
#include <vector>

#include "runtime/RuntimeInteractionEffectCommandFrameStep.hpp"
#include "support/CommandFrameFixtures.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::CommandFrame;
using iggy::test::Expect;
using iggy::test::ExpectPlayerAgent;
using iggy::test::Failures;
using iggy::test::Near;
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

const iggy::ResourceId PlayerId { "player:interaction-effect-frame" };

iggy::runtime::RuntimeSessionState SessionWithPlayer(iggy::Vec2 position = { 0.0F, 0.0F })
{
	iggy::runtime::RuntimeSessionState session;
	session.hasPlayer = true;
	session.player = PlayerAgent(PlayerId, position, { 0, 0 }, iggy::PlayerMovementStatus::Idle, iggy::PlayerFacing2D::East);
	session.tickIndex = 31;
	return session;
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
	Expect(result.built, "runtime interaction effect frame target registry setup should build");
	return result.registry;
}

iggy::InteractionEffectEntry2D Entry(const char *targetId, std::vector<iggy::InteractionEffect2D> effects)
{
	return { iggy::ResourceId { targetId }, effects };
}

iggy::InteractionEffectCatalog2D Catalog(std::vector<iggy::InteractionEffectEntry2D> entries)
{
	const iggy::InteractionEffectCatalog2DBuildResult result = iggy::InteractionEffectCatalog2DBuilder {}.build(entries);
	Expect(result.built, "runtime interaction effect frame catalog setup should build");
	return result.catalog;
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

void ExpectCommand(const iggy::runtime::GameplayCommand2D &actual, const iggy::runtime::GameplayCommand2D &expected, const char *message)
{
	Expect(actual.type == expected.type, message);
	Expect(actual.actorId == expected.actorId, message);
	Expect(NearVec(actual.targetPoint, expected.targetPoint), message);
	Expect(actual.targetTile == expected.targetTile, message);
	Expect(actual.targetId == expected.targetId, message);
}

void ExpectTarget(const iggy::InteractionTarget2D &actual, const iggy::InteractionTarget2D &expected, const char *message)
{
	Expect(actual.id == expected.id, message);
	Expect(actual.kind == expected.kind, message);
	Expect(NearVec(actual.position, expected.position), message);
	Expect(actual.radius == expected.radius, message);
	Expect(actual.enabled == expected.enabled, message);
}

void ExpectEmptyFrameResult(const iggy::runtime::RuntimeInteractionEffectCommandFrameResult &result, const char *message)
{
	Expect(result.interactions.empty(), message);
	Expect(result.readyCount == 0, message);
	Expect(result.noEffectCount == 0, message);
	Expect(result.blockedCount == 0, message);
	Expect(result.requestedEffectCount == 0, message);
	Expect(!result.hasInteractions(), message);
}

void TestEmptyFrameProducesNoInteractions()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::InteractionTarget2DRegistry targets = Registry({});
	const iggy::InteractionEffectCatalog2D effects = Catalog({});
	const iggy::runtime::GameplayCommandFrame2D frame;

	const iggy::runtime::RuntimeInteractionEffectCommandFrameResult result =
		iggy::runtime::RuntimeInteractionEffectCommandFrameStep {}.evaluate(session, targets, effects, frame);

	ExpectEmptyFrameResult(result, "empty effect command frame should produce no interactions");
}

void TestOnlyNonInteractCommandsAreIgnored()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::InteractionTarget2DRegistry targets = Registry({
		Target("target:ignored", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 1.0F),
	});
	const iggy::InteractionEffectCatalog2D effects = Catalog({
		Entry("target:ignored", { iggy::inspectTextInteractionEffect({}, "Ignored") }),
	});
	const iggy::runtime::GameplayCommandFrame2D frame = CommandFrame({
		factory.wait(PlayerId),
		factory.moveToPoint(PlayerId, { 2.0F, 3.0F }),
		factory.none(PlayerId),
	});

	const iggy::runtime::RuntimeInteractionEffectCommandFrameResult result =
		iggy::runtime::RuntimeInteractionEffectCommandFrameStep {}.evaluate(session, targets, effects, frame);

	ExpectEmptyFrameResult(result, "non-interact effect command frame should produce no interactions");
}

void TestSingleReachableInteractWithEffectsProducesReadyEntry()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::InteractionTarget2D target = Target("target:ready", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 0.0F);
	const std::vector<iggy::InteractionEffect2D> requestedEffects {
		iggy::inspectTextInteractionEffect(target.id, "Ready"),
		iggy::emitInteractionEventEffect(target.id, iggy::ResourceId { "event:ready" }),
	};
	const iggy::runtime::GameplayCommand2D command = factory.interact(PlayerId, target.id);
	const iggy::runtime::GameplayCommandFrame2D frame = CommandFrame({
		factory.wait(PlayerId),
		command,
	});

	const iggy::runtime::RuntimeInteractionEffectCommandFrameResult result =
		iggy::runtime::RuntimeInteractionEffectCommandFrameStep {}.evaluate(
			SessionWithPlayer({ 0.0F, 0.0F }),
			Registry({ target }),
			Catalog({ Entry("target:ready", requestedEffects) }),
			frame);

	Expect(result.hasInteractions(), "ready effect command frame should report interactions");
	Expect(result.interactions.size() == 1, "ready effect command frame should produce one entry");
	Expect(result.readyCount == 1, "ready effect command frame should count one ready entry");
	Expect(result.noEffectCount == 0, "ready effect command frame should count zero no-effect entries");
	Expect(result.blockedCount == 0, "ready effect command frame should count zero blocked entries");
	Expect(result.requestedEffectCount == requestedEffects.size(), "ready effect command frame should count requested effects");
	if (result.interactions.size() == 1) {
		Expect(result.interactions[0].commandIndex == 1, "ready effect command frame should preserve original command index");
		Expect(result.interactions[0].result.status == iggy::runtime::RuntimeInteractionEffectCommandStatus::Ready, "ready effect command frame should preserve Ready status");
		ExpectCommand(result.interactions[0].result.interaction.command, command, "ready effect command frame should preserve command");
		Expect(SameEffects(result.interactions[0].result.effects.effects, requestedEffects), "ready effect command frame should preserve ordered effects");
	}
}

void TestReachableInteractWithNoEffectsCountsNoEffect()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::InteractionTarget2D target = Target("target:no_effects", iggy::InteractionTarget2DKind::Inspectable, { 0.0F, 0.0F }, 0.0F);
	const iggy::runtime::GameplayCommandFrame2D frame = CommandFrame({
		factory.interact(PlayerId, target.id),
	});

	const iggy::runtime::RuntimeInteractionEffectCommandFrameResult result =
		iggy::runtime::RuntimeInteractionEffectCommandFrameStep {}.evaluate(
			SessionWithPlayer(),
			Registry({ target }),
			Catalog({}),
			frame);

	Expect(result.interactions.size() == 1, "no-effect frame should produce one interaction entry");
	Expect(result.readyCount == 0, "no-effect frame should count zero ready entries");
	Expect(result.noEffectCount == 1, "no-effect frame should count one no-effect entry");
	Expect(result.blockedCount == 0, "no-effect frame should count zero blocked entries");
	Expect(result.requestedEffectCount == 0, "no-effect frame should count zero requested effects");
	if (result.interactions.size() == 1)
		Expect(result.interactions[0].result.status == iggy::runtime::RuntimeInteractionEffectCommandStatus::NoEffects, "no-effect frame should preserve NoEffects status");
}

void TestMissingDisabledAndOutOfRangeInteractionsProduceBlockedEntries()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::InteractionTarget2D disabled = Target("target:disabled", iggy::InteractionTarget2DKind::Talk, { 0.0F, 0.0F }, 2.0F, false);
	const iggy::InteractionTarget2D far = Target("target:far", iggy::InteractionTarget2DKind::Door, { 4.0F, 0.0F }, 3.0F);
	const iggy::runtime::GameplayCommandFrame2D frame = CommandFrame({
		factory.interact(PlayerId, {}),
		factory.interact(PlayerId, disabled.id),
		factory.interact(PlayerId, far.id),
	});

	const iggy::runtime::RuntimeInteractionEffectCommandFrameResult result =
		iggy::runtime::RuntimeInteractionEffectCommandFrameStep {}.evaluate(
			SessionWithPlayer({ 0.0F, 0.0F }),
			Registry({ disabled, far }),
			Catalog({
				Entry("target:disabled", { iggy::inspectTextInteractionEffect({}, "Disabled") }),
				Entry("target:far", { iggy::inspectTextInteractionEffect({}, "Far") }),
			}),
			frame);

	Expect(result.interactions.size() == 3, "blocked effect frame should produce one entry per interact command");
	Expect(result.readyCount == 0, "blocked effect frame should count zero ready entries");
	Expect(result.noEffectCount == 0, "blocked effect frame should count zero no-effect entries");
	Expect(result.blockedCount == 3, "blocked effect frame should count all interactions as blocked");
	Expect(result.requestedEffectCount == 0, "blocked effect frame should not request effects");
	if (result.interactions.size() == 3) {
		Expect(result.interactions[0].commandIndex == 0 && result.interactions[0].result.interaction.status == iggy::runtime::RuntimeInteractionCommandStatus::MissingTargetId, "first blocked effect entry should preserve MissingTargetId");
		Expect(result.interactions[1].commandIndex == 1 && result.interactions[1].result.interaction.status == iggy::runtime::RuntimeInteractionCommandStatus::TargetDisabled, "second blocked effect entry should preserve TargetDisabled");
		Expect(result.interactions[2].commandIndex == 2 && result.interactions[2].result.interaction.status == iggy::runtime::RuntimeInteractionCommandStatus::OutOfRange, "third blocked effect entry should preserve OutOfRange");
	}
}

void TestMixedCommandFramePreservesIndexesOrderAndCounts()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::InteractionTarget2D readyA = Target("target:ready_a", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 0.0F);
	const iggy::InteractionTarget2D noEffects = Target("target:no_effects", iggy::InteractionTarget2DKind::Inspectable, { 1.0F, 0.0F }, 1.0F);
	const iggy::InteractionTarget2D far = Target("target:far", iggy::InteractionTarget2DKind::Door, { 5.0F, 0.0F }, 1.0F);
	const iggy::InteractionTarget2D readyB = Target("target:ready_b", iggy::InteractionTarget2DKind::Pickup, { 0.0F, 1.0F }, 1.0F);
	const std::vector<iggy::InteractionEffect2D> readyAEffects {
		iggy::inspectTextInteractionEffect(readyA.id, "A"),
	};
	const std::vector<iggy::InteractionEffect2D> readyBEffects {
		iggy::toggleTargetInteractionEffect(readyB.id, false),
		iggy::emitInteractionEventEffect(readyB.id, iggy::ResourceId { "event:b" }),
	};
	const iggy::runtime::GameplayCommandFrame2D frame = CommandFrame({
		factory.wait(PlayerId),
		factory.interact(PlayerId, readyA.id),
		factory.moveToPoint(PlayerId, { 9.0F, 9.0F }),
		factory.interact(PlayerId, noEffects.id),
		factory.interact(PlayerId, far.id),
		factory.none(PlayerId),
		factory.interact(PlayerId, readyB.id),
	});

	const iggy::runtime::RuntimeInteractionEffectCommandFrameResult result =
		iggy::runtime::RuntimeInteractionEffectCommandFrameStep {}.evaluate(
			SessionWithPlayer({ 0.0F, 0.0F }),
			Registry({ readyA, noEffects, far, readyB }),
			Catalog({
				Entry("target:ready_a", readyAEffects),
				Entry("target:ready_b", readyBEffects),
			}),
			frame);

	Expect(result.interactions.size() == 4, "mixed effect frame should include only interact entries");
	Expect(result.readyCount == 2, "mixed effect frame should count ready entries");
	Expect(result.noEffectCount == 1, "mixed effect frame should count no-effect entries");
	Expect(result.blockedCount == 1, "mixed effect frame should count blocked entries");
	Expect(result.requestedEffectCount == readyAEffects.size() + readyBEffects.size(), "mixed effect frame should sum requested effects");
	if (result.interactions.size() == 4) {
		Expect(result.interactions[0].commandIndex == 1 && result.interactions[0].result.interaction.command.targetId == readyA.id, "first mixed effect entry should preserve index and order");
		Expect(result.interactions[1].commandIndex == 3 && result.interactions[1].result.interaction.command.targetId == noEffects.id, "second mixed effect entry should preserve index and order");
		Expect(result.interactions[2].commandIndex == 4 && result.interactions[2].result.interaction.command.targetId == far.id, "third mixed effect entry should preserve index and order");
		Expect(result.interactions[3].commandIndex == 6 && result.interactions[3].result.interaction.command.targetId == readyB.id, "fourth mixed effect entry should preserve index and order");
		Expect(SameEffects(result.interactions[0].result.effects.effects, readyAEffects), "mixed effect frame should preserve first ready effects");
		Expect(SameEffects(result.interactions[3].result.effects.effects, readyBEffects), "mixed effect frame should preserve second ready effects");
	}
}

void TestMissingPlayerProducesBlockedEntries()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	session.hasPlayer = false;
	session.player = {};
	const iggy::InteractionTarget2D one = Target("target:one", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 1.0F);
	const iggy::InteractionTarget2D two = Target("target:two", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 1.0F);
	const iggy::runtime::GameplayCommandFrame2D frame = CommandFrame({
		factory.interact(PlayerId, one.id),
		factory.wait(PlayerId),
		factory.interact(PlayerId, two.id),
	});

	const iggy::runtime::RuntimeInteractionEffectCommandFrameResult result =
		iggy::runtime::RuntimeInteractionEffectCommandFrameStep {}.evaluate(
			session,
			Registry({ one, two }),
			Catalog({
				Entry("target:one", { iggy::inspectTextInteractionEffect({}, "One") }),
				Entry("target:two", { iggy::inspectTextInteractionEffect({}, "Two") }),
			}),
			frame);

	Expect(result.interactions.size() == 2, "missing-player effect frame should produce entries for interact commands");
	Expect(result.readyCount == 0, "missing-player effect frame should count zero ready entries");
	Expect(result.noEffectCount == 0, "missing-player effect frame should count zero no-effect entries");
	Expect(result.blockedCount == 2, "missing-player effect frame should count missing-player entries as blocked");
	Expect(result.requestedEffectCount == 0, "missing-player effect frame should request no effects");
	if (result.interactions.size() == 2) {
		Expect(result.interactions[0].result.status == iggy::runtime::RuntimeInteractionEffectCommandStatus::MissingPlayer, "first missing-player effect entry should preserve status");
		Expect(result.interactions[1].result.status == iggy::runtime::RuntimeInteractionEffectCommandStatus::MissingPlayer, "second missing-player effect entry should preserve status");
	}
}

void TestExtraReachConfigFlowsThroughEntries()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::InteractionTarget2D target = Target("target:extra", iggy::InteractionTarget2DKind::Pickup, { 4.0F, 0.0F }, 3.0F);
	const std::vector<iggy::InteractionEffect2D> effects {
		iggy::emitInteractionEventEffect(target.id, iggy::ResourceId { "event:pickup" }),
	};
	const iggy::runtime::GameplayCommandFrame2D frame = CommandFrame({
		factory.interact(PlayerId, target.id),
	});

	const iggy::runtime::RuntimeInteractionEffectCommandFrameResult result =
		iggy::runtime::RuntimeInteractionEffectCommandFrameStep {}.evaluate(
			SessionWithPlayer({ 0.0F, 0.0F }),
			Registry({ target }),
			Catalog({ Entry("target:extra", effects) }),
			frame,
			{ 1.0F });

	Expect(result.interactions.size() == 1, "extra reach effect frame should produce one interaction entry");
	Expect(result.readyCount == 1, "extra reach effect frame should count one ready entry");
	Expect(result.requestedEffectCount == effects.size(), "extra reach effect frame should count requested effect");
	if (result.interactions.size() == 1) {
		Expect(result.interactions[0].result.status == iggy::runtime::RuntimeInteractionEffectCommandStatus::Ready, "extra reach effect entry should be ready");
		Expect(Near(result.interactions[0].result.interaction.plan.reach.distance, 4.0F), "extra reach effect entry should preserve distance");
		Expect(Near(result.interactions[0].result.interaction.plan.reach.allowedDistance, 4.0F), "extra reach effect entry should preserve allowed distance");
		Expect(SameEffects(result.interactions[0].result.effects.effects, effects), "extra reach effect entry should preserve effects");
	}
}

void TestInputsAreNotMutated()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 1.0F, 2.0F });
	const iggy::runtime::RuntimeSessionState sessionBefore = session;
	const iggy::InteractionTarget2D target = Target("target:immutable", iggy::InteractionTarget2DKind::Door, { 1.0F, 2.0F }, 1.0F);
	const iggy::InteractionTarget2DRegistry targets = Registry({ target });
	const std::vector<iggy::InteractionTarget2D> targetsBefore = targets.targets();
	const std::vector<iggy::InteractionEffect2D> requestedEffects {
		iggy::inspectTextInteractionEffect(target.id, "Text"),
	};
	const iggy::InteractionEffectCatalog2D effects = Catalog({ Entry("target:immutable", requestedEffects) });
	const iggy::InteractionEffectCatalog2D effectsBefore = effects;
	iggy::runtime::GameplayCommandFrame2D frame = CommandFrame({
		factory.wait(PlayerId),
		factory.interact(PlayerId, target.id),
	});
	const iggy::runtime::GameplayCommandFrame2D frameBefore = frame;

	const iggy::runtime::RuntimeInteractionEffectCommandFrameResult result =
		iggy::runtime::RuntimeInteractionEffectCommandFrameStep {}.evaluate(session, targets, effects, frame);

	Expect(result.interactions.size() == 1, "effect frame immutability setup should produce one interaction");
	Expect(session.hasPlayer == sessionBefore.hasPlayer, "effect frame step should not mutate session hasPlayer");
	Expect(session.tickIndex == sessionBefore.tickIndex, "effect frame step should not mutate session tickIndex");
	ExpectPlayerAgent(session.player, sessionBefore.player, "effect frame step input session");
	Expect(targets.targets().size() == targetsBefore.size(), "effect frame step should not mutate target count");
	if (targets.targets().size() == targetsBefore.size())
		ExpectTarget(targets.targets()[0], targetsBefore[0], "effect frame step should not mutate target payload");
	Expect(effects.entries().size() == effectsBefore.entries().size(), "effect frame step should not mutate catalog entry count");
	if (effects.entries().size() == effectsBefore.entries().size() && !effects.entries().empty()) {
		Expect(effects.entries()[0].targetId == effectsBefore.entries()[0].targetId, "effect frame step should not mutate catalog target id");
		Expect(SameEffects(effects.entries()[0].effects, effectsBefore.entries()[0].effects), "effect frame step should not mutate catalog effects");
	}
	Expect(frame.commands.size() == frameBefore.commands.size(), "effect frame step should not mutate frame command count");
	if (frame.commands.size() == frameBefore.commands.size()) {
		ExpectCommand(frame.commands[0], frameBefore.commands[0], "effect frame step should not mutate first command");
		ExpectCommand(frame.commands[1], frameBefore.commands[1], "effect frame step should not mutate second command");
	}
}

} // namespace

int main()
{
	TestEmptyFrameProducesNoInteractions();
	TestOnlyNonInteractCommandsAreIgnored();
	TestSingleReachableInteractWithEffectsProducesReadyEntry();
	TestReachableInteractWithNoEffectsCountsNoEffect();
	TestMissingDisabledAndOutOfRangeInteractionsProduceBlockedEntries();
	TestMixedCommandFramePreservesIndexesOrderAndCounts();
	TestMissingPlayerProducesBlockedEntries();
	TestExtraReachConfigFlowsThroughEntries();
	TestInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
