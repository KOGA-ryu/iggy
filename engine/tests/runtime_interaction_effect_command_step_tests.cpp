#include <cstdlib>
#include <vector>

#include "runtime/RuntimeInteractionEffectCommandStep.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::ExpectPlayerAgent;
using iggy::test::Failures;
using iggy::test::Near;
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

const iggy::ResourceId PlayerId { "player:interaction-effect" };

iggy::runtime::RuntimeSessionState SessionWithPlayer(iggy::Vec2 position = { 0.0F, 0.0F })
{
	iggy::runtime::RuntimeSessionState session;
	session.hasPlayer = true;
	session.player = PlayerAgent(PlayerId, position, { 0, 0 }, iggy::PlayerMovementStatus::Moving, iggy::PlayerFacing2D::East);
	session.tickIndex = 23;
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
	Expect(result.built, "runtime interaction effect test target registry setup should build");
	return result.registry;
}

iggy::InteractionEffectEntry2D Entry(const char *targetId, std::vector<iggy::InteractionEffect2D> effects)
{
	return { iggy::ResourceId { targetId }, effects };
}

iggy::InteractionEffectCatalog2D Catalog(std::vector<iggy::InteractionEffectEntry2D> entries)
{
	const iggy::InteractionEffectCatalog2DBuildResult result = iggy::InteractionEffectCatalog2DBuilder {}.build(entries);
	Expect(result.built, "runtime interaction effect test catalog setup should build");
	return result.catalog;
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

void TestNonInteractCommandReturnsNotInteractWithoutEffectPlan()
{
	const iggy::runtime::RuntimeSessionState session;
	const iggy::InteractionTarget2DRegistry targets = Registry({});
	const iggy::InteractionEffectCatalog2D effects = Catalog({});
	const iggy::runtime::GameplayCommand2D command = iggy::runtime::GameplayCommand2DFactory {}.wait(PlayerId);

	const iggy::runtime::RuntimeInteractionEffectCommandResult result =
		iggy::runtime::RuntimeInteractionEffectCommandStep {}.evaluate(session, targets, effects, command);

	Expect(result.status == iggy::runtime::RuntimeInteractionEffectCommandStatus::NotInteractCommand, "non-interact effect command should return NotInteractCommand");
	Expect(!result.ready(), "non-interact effect command should not be ready");
	Expect(result.interaction.status == iggy::runtime::RuntimeInteractionCommandStatus::NotInteractCommand, "non-interact effect command should preserve interaction status");
	ExpectCommand(result.interaction.command, command, "non-interact effect command should preserve command");
	Expect(result.effects.status == iggy::InteractionEffectPlan2DStatus::InteractionNotReady, "non-interact effect command should keep default effect plan");
	Expect(result.effects.effects.empty(), "non-interact effect command should have no effects");
}

void TestMissingPlayerReturnsMissingPlayerWithoutEffectPlan()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	session.hasPlayer = false;
	session.player = {};
	const iggy::InteractionTarget2D target = Target("target:lever", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 1.0F);
	const iggy::runtime::GameplayCommand2D command = iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, target.id);

	const iggy::runtime::RuntimeInteractionEffectCommandResult result =
		iggy::runtime::RuntimeInteractionEffectCommandStep {}.evaluate(
			session,
			Registry({ target }),
			Catalog({ Entry("target:lever", { iggy::inspectTextInteractionEffect({}, "Text") }) }),
			command);

	Expect(result.status == iggy::runtime::RuntimeInteractionEffectCommandStatus::MissingPlayer, "missing-player effect command should return MissingPlayer");
	Expect(!result.ready(), "missing-player effect command should not be ready");
	Expect(result.interaction.status == iggy::runtime::RuntimeInteractionCommandStatus::MissingPlayer, "missing-player effect command should preserve interaction status");
	Expect(result.effects.status == iggy::InteractionEffectPlan2DStatus::InteractionNotReady, "missing-player effect command should keep default effect plan");
	Expect(result.effects.effects.empty(), "missing-player effect command should have no effects");
}

void TestReachableTargetWithMatchingEffectsReturnsReady()
{
	const iggy::InteractionTarget2D target = Target("target:ready", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 0.0F);
	const std::vector<iggy::InteractionEffect2D> expectedEffects {
		iggy::inspectTextInteractionEffect(target.id, "Ready"),
		iggy::emitInteractionEventEffect(target.id, iggy::ResourceId { "event:ready" }),
		iggy::toggleTargetInteractionEffect(target.id, false),
	};
	const iggy::runtime::GameplayCommand2D command = iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, target.id);

	const iggy::runtime::RuntimeInteractionEffectCommandResult result =
		iggy::runtime::RuntimeInteractionEffectCommandStep {}.evaluate(
			SessionWithPlayer({ 0.0F, 0.0F }),
			Registry({ target }),
			Catalog({ Entry("target:ready", expectedEffects) }),
			command);

	Expect(result.status == iggy::runtime::RuntimeInteractionEffectCommandStatus::Ready, "reachable target with effects should return Ready");
	Expect(result.ready(), "reachable target with effects should be ready");
	Expect(result.interaction.status == iggy::runtime::RuntimeInteractionCommandStatus::Ready, "ready effect command should preserve ready interaction");
	Expect(result.effects.status == iggy::InteractionEffectPlan2DStatus::Ready, "ready effect command should preserve ready effect plan");
	Expect(SameEffects(result.effects.effects, expectedEffects), "ready effect command should copy ordered effects");
}

void TestReachableTargetWithNoEffectsReturnsNoEffects()
{
	const iggy::InteractionTarget2D target = Target("target:no_effects", iggy::InteractionTarget2DKind::Inspectable, { 0.0F, 0.0F }, 0.0F);
	const iggy::runtime::GameplayCommand2D command = iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, target.id);

	const iggy::runtime::RuntimeInteractionEffectCommandResult result =
		iggy::runtime::RuntimeInteractionEffectCommandStep {}.evaluate(
			SessionWithPlayer({ 0.0F, 0.0F }),
			Registry({ target }),
			Catalog({}),
			command);

	Expect(result.status == iggy::runtime::RuntimeInteractionEffectCommandStatus::NoEffects, "reachable target without catalog entry should return NoEffects");
	Expect(!result.ready(), "NoEffects effect command should not be ready");
	Expect(result.interaction.ready(), "NoEffects effect command should preserve ready interaction");
	Expect(result.effects.status == iggy::InteractionEffectPlan2DStatus::NoEffects, "NoEffects effect command should preserve effect plan status");
	Expect(result.effects.effects.empty(), "NoEffects effect command should have no effects");
}

void TestNonReadyInteractionStatusesReturnInteractionNotReady()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const iggy::InteractionTarget2D disabled = Target("target:disabled", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 2.0F, false);
	const iggy::InteractionTarget2D far = Target("target:far", iggy::InteractionTarget2DKind::Talk, { 4.0F, 0.0F }, 1.0F);
	const iggy::InteractionTarget2DRegistry targets = Registry({ disabled, far });
	const iggy::InteractionEffectCatalog2D effects = Catalog({
		Entry("target:disabled", { iggy::inspectTextInteractionEffect({}, "Disabled") }),
		Entry("target:far", { iggy::inspectTextInteractionEffect({}, "Far") }),
	});
	const std::vector<iggy::runtime::GameplayCommand2D> commands {
		iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, {}),
		iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, iggy::ResourceId { "target:missing" }),
		iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, disabled.id),
		iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, far.id),
	};
	const std::vector<iggy::runtime::RuntimeInteractionCommandStatus> expectedInteractionStatuses {
		iggy::runtime::RuntimeInteractionCommandStatus::MissingTargetId,
		iggy::runtime::RuntimeInteractionCommandStatus::TargetNotFound,
		iggy::runtime::RuntimeInteractionCommandStatus::TargetDisabled,
		iggy::runtime::RuntimeInteractionCommandStatus::OutOfRange,
	};

	for (std::size_t index = 0; index < commands.size(); ++index) {
		const iggy::runtime::RuntimeInteractionEffectCommandResult result =
			iggy::runtime::RuntimeInteractionEffectCommandStep {}.evaluate(session, targets, effects, commands[index]);
		Expect(result.status == iggy::runtime::RuntimeInteractionEffectCommandStatus::InteractionNotReady, "non-ready interaction should map to InteractionNotReady");
		Expect(!result.ready(), "non-ready interaction effect command should not be ready");
		Expect(result.interaction.status == expectedInteractionStatuses[index], "non-ready interaction should preserve nested interaction status");
		Expect(result.effects.status == iggy::InteractionEffectPlan2DStatus::InteractionNotReady, "non-ready interaction should preserve effect plan not-ready status");
		Expect(result.effects.effects.empty(), "non-ready interaction should not request effects");
	}
}

void TestExtraReachCanMakeEffectsReady()
{
	const iggy::InteractionTarget2D target = Target("target:extra", iggy::InteractionTarget2DKind::Pickup, { 4.0F, 0.0F }, 3.0F);
	const std::vector<iggy::InteractionEffect2D> expectedEffects {
		iggy::emitInteractionEventEffect(target.id, iggy::ResourceId { "event:pickup" }),
	};
	const iggy::runtime::GameplayCommand2D command = iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, target.id);

	const iggy::runtime::RuntimeInteractionEffectCommandResult result =
		iggy::runtime::RuntimeInteractionEffectCommandStep {}.evaluate(
			SessionWithPlayer({ 0.0F, 0.0F }),
			Registry({ target }),
			Catalog({ Entry("target:extra", expectedEffects) }),
			command,
			{ 1.0F });

	Expect(result.status == iggy::runtime::RuntimeInteractionEffectCommandStatus::Ready, "extra reach should make effect command ready");
	Expect(result.ready(), "extra reach effect command should be ready");
	Expect(result.interaction.status == iggy::runtime::RuntimeInteractionCommandStatus::Ready, "extra reach should preserve ready interaction");
	Expect(Near(result.interaction.plan.reach.allowedDistance, 4.0F), "extra reach should flow to nested interaction reach");
	Expect(result.effects.status == iggy::InteractionEffectPlan2DStatus::Ready, "extra reach should preserve ready effect plan");
	Expect(SameEffects(result.effects.effects, expectedEffects), "extra reach should preserve requested effects");
}

void TestInputsAreNotMutated()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 1.0F, 2.0F });
	const iggy::runtime::RuntimeSessionState sessionBefore = session;
	const iggy::InteractionTarget2D target = Target("target:immutable", iggy::InteractionTarget2DKind::Usable, { 1.0F, 2.0F }, 1.0F);
	iggy::InteractionTarget2DRegistry targets = Registry({ target });
	const std::vector<iggy::InteractionTarget2D> targetsBefore = targets.targets();
	const std::vector<iggy::InteractionEffect2D> expectedEffects {
		iggy::inspectTextInteractionEffect(target.id, "Text"),
	};
	iggy::InteractionEffectCatalog2D effects = Catalog({ Entry("target:immutable", expectedEffects) });
	const iggy::InteractionEffectCatalog2D effectsBefore = effects;
	iggy::runtime::GameplayCommand2D command = iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, target.id);
	const iggy::runtime::GameplayCommand2D commandBefore = command;

	const iggy::runtime::RuntimeInteractionEffectCommandResult result =
		iggy::runtime::RuntimeInteractionEffectCommandStep {}.evaluate(session, targets, effects, command);

	Expect(result.ready(), "immutability setup should produce ready effect command");
	Expect(session.hasPlayer == sessionBefore.hasPlayer, "effect command step should not mutate session hasPlayer");
	Expect(session.tickIndex == sessionBefore.tickIndex, "effect command step should not mutate session tickIndex");
	ExpectPlayerAgent(session.player, sessionBefore.player, "effect command step input session");
	Expect(targets.targets().size() == targetsBefore.size(), "effect command step should not mutate target count");
	if (targets.targets().size() == targetsBefore.size())
		ExpectTarget(targets.targets()[0], targetsBefore[0], "effect command step should not mutate target payload");
	Expect(effects.entries().size() == effectsBefore.entries().size(), "effect command step should not mutate catalog entry count");
	if (effects.entries().size() == effectsBefore.entries().size() && !effects.entries().empty()) {
		Expect(effects.entries()[0].targetId == effectsBefore.entries()[0].targetId, "effect command step should not mutate catalog target id");
		Expect(SameEffects(effects.entries()[0].effects, effectsBefore.entries()[0].effects), "effect command step should not mutate catalog effects");
	}
	ExpectCommand(command, commandBefore, "effect command step should not mutate input command");
}

} // namespace

int main()
{
	TestNonInteractCommandReturnsNotInteractWithoutEffectPlan();
	TestMissingPlayerReturnsMissingPlayerWithoutEffectPlan();
	TestReachableTargetWithMatchingEffectsReturnsReady();
	TestReachableTargetWithNoEffectsReturnsNoEffects();
	TestNonReadyInteractionStatusesReturnInteractionNotReady();
	TestExtraReachCanMakeEffectsReady();
	TestInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
