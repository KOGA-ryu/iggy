#include <cstdlib>
#include <vector>

#include "runtime/RuntimeInteractionCommandFrameStep.hpp"
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

const iggy::ResourceId PlayerId { "player:interaction-frame" };

iggy::runtime::RuntimeSessionState SessionWithPlayer(iggy::Vec2 position = { 0.0F, 0.0F })
{
	iggy::runtime::RuntimeSessionState session;
	session.hasPlayer = true;
	session.player = PlayerAgent(PlayerId, position, { 0, 0 }, iggy::PlayerMovementStatus::Idle, iggy::PlayerFacing2D::South);
	session.tickIndex = 11;
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
	Expect(result.built, "runtime interaction frame test registry setup should build");
	return result.registry;
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

void ExpectEmptyFrameResult(const iggy::runtime::RuntimeInteractionCommandFrameResult &result, const char *message)
{
	Expect(result.interactions.empty(), message);
	Expect(result.readyCount == 0, message);
	Expect(result.blockedCount == 0, message);
	Expect(!result.hasInteractions(), message);
}

void TestEmptyFrameProducesNoInteractions()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::InteractionTarget2DRegistry registry = Registry({});
	const iggy::runtime::GameplayCommandFrame2D frame;

	const iggy::runtime::RuntimeInteractionCommandFrameResult result = iggy::runtime::RuntimeInteractionCommandFrameStep {}.evaluate(session, registry, frame);

	ExpectEmptyFrameResult(result, "empty command frame should produce no interaction diagnostics");
}

void TestOnlyNonInteractCommandsAreIgnored()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::InteractionTarget2DRegistry registry = Registry({
		Target("target:lever", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 1.0F),
	});
	const iggy::runtime::GameplayCommandFrame2D frame = CommandFrame({
		factory.wait(PlayerId),
		factory.moveToPoint(PlayerId, { 1.0F, 2.0F }),
		factory.none(PlayerId),
	});

	const iggy::runtime::RuntimeInteractionCommandFrameResult result = iggy::runtime::RuntimeInteractionCommandFrameStep {}.evaluate(session, registry, frame);

	ExpectEmptyFrameResult(result, "non-interact command frame should produce no interaction diagnostics");
}

void TestSingleReachableInteractProducesReadyEntryWithOriginalIndex()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const iggy::InteractionTarget2D target = Target("target:ready", iggy::InteractionTarget2DKind::Usable, { 3.0F, 4.0F }, 5.0F);
	const iggy::InteractionTarget2DRegistry registry = Registry({ target });
	const iggy::runtime::GameplayCommand2D command = factory.interact(PlayerId, target.id);
	const iggy::runtime::GameplayCommandFrame2D frame = CommandFrame({
		factory.wait(PlayerId),
		command,
	});

	const iggy::runtime::RuntimeInteractionCommandFrameResult result = iggy::runtime::RuntimeInteractionCommandFrameStep {}.evaluate(session, registry, frame);

	Expect(result.hasInteractions(), "reachable interact frame should report interactions");
	Expect(result.interactions.size() == 1, "reachable interact frame should produce one interaction entry");
	Expect(result.readyCount == 1, "reachable interact frame should count one ready interaction");
	Expect(result.blockedCount == 0, "reachable interact frame should count zero blocked interactions");
	if (result.interactions.size() == 1) {
		Expect(result.interactions[0].commandIndex == 1, "reachable interact entry should preserve original command index");
		Expect(result.interactions[0].result.status == iggy::runtime::RuntimeInteractionCommandStatus::Ready, "reachable interact entry should preserve ready status");
		Expect(result.interactions[0].result.ready(), "reachable interact entry should be ready");
		ExpectCommand(result.interactions[0].result.command, command, "reachable interact entry should preserve command");
		ExpectTarget(result.interactions[0].result.plan.query.target, target, "reachable interact entry should preserve target details");
	}
}

void TestMissingDisabledAndOutOfRangeInteractionsProduceBlockedEntries()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const iggy::InteractionTarget2D disabled = Target("target:disabled", iggy::InteractionTarget2DKind::Talk, { 0.0F, 0.0F }, 10.0F, false);
	const iggy::InteractionTarget2D far = Target("target:far", iggy::InteractionTarget2DKind::Door, { 4.0F, 0.0F }, 3.0F);
	const iggy::InteractionTarget2DRegistry registry = Registry({ disabled, far });
	const iggy::runtime::GameplayCommandFrame2D frame = CommandFrame({
		factory.interact(PlayerId, {}),
		factory.interact(PlayerId, disabled.id),
		factory.interact(PlayerId, far.id),
	});

	const iggy::runtime::RuntimeInteractionCommandFrameResult result = iggy::runtime::RuntimeInteractionCommandFrameStep {}.evaluate(session, registry, frame);

	Expect(result.interactions.size() == 3, "blocked interactions frame should produce one entry per interact command");
	Expect(result.readyCount == 0, "blocked interactions frame should count zero ready interactions");
	Expect(result.blockedCount == 3, "blocked interactions frame should count all blocked interactions");
	if (result.interactions.size() == 3) {
		Expect(result.interactions[0].commandIndex == 0 && result.interactions[0].result.status == iggy::runtime::RuntimeInteractionCommandStatus::MissingTargetId, "first blocked interaction should preserve MissingTargetId");
		Expect(result.interactions[1].commandIndex == 1 && result.interactions[1].result.status == iggy::runtime::RuntimeInteractionCommandStatus::TargetDisabled, "second blocked interaction should preserve TargetDisabled");
		Expect(result.interactions[2].commandIndex == 2 && result.interactions[2].result.status == iggy::runtime::RuntimeInteractionCommandStatus::OutOfRange, "third blocked interaction should preserve OutOfRange");
	}
}

void TestMixedCommandFramePreservesInteractionOrderAndIndexes()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const iggy::InteractionTarget2D readyA = Target("target:ready_a", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 0.0F);
	const iggy::InteractionTarget2D far = Target("target:far", iggy::InteractionTarget2DKind::Door, { 5.0F, 0.0F }, 1.0F);
	const iggy::InteractionTarget2D readyB = Target("target:ready_b", iggy::InteractionTarget2DKind::Pickup, { 1.0F, 0.0F }, 1.0F);
	const iggy::InteractionTarget2DRegistry registry = Registry({ readyA, far, readyB });
	const iggy::runtime::GameplayCommandFrame2D frame = CommandFrame({
		factory.wait(PlayerId),
		factory.interact(PlayerId, readyA.id),
		factory.moveToPoint(PlayerId, { 9.0F, 9.0F }),
		factory.interact(PlayerId, far.id),
		factory.none(PlayerId),
		factory.interact(PlayerId, readyB.id),
	});

	const iggy::runtime::RuntimeInteractionCommandFrameResult result = iggy::runtime::RuntimeInteractionCommandFrameStep {}.evaluate(session, registry, frame);

	Expect(result.interactions.size() == 3, "mixed frame should include only interaction entries");
	Expect(result.readyCount == 2, "mixed frame should count ready interactions");
	Expect(result.blockedCount == 1, "mixed frame should count blocked interactions");
	if (result.interactions.size() == 3) {
		Expect(result.interactions[0].commandIndex == 1 && result.interactions[0].result.command.targetId == readyA.id, "first interaction entry should preserve original index and order");
		Expect(result.interactions[1].commandIndex == 3 && result.interactions[1].result.command.targetId == far.id, "second interaction entry should preserve original index and order");
		Expect(result.interactions[2].commandIndex == 5 && result.interactions[2].result.command.targetId == readyB.id, "third interaction entry should preserve original index and order");
	}
}

void TestMissingPlayerResultsAppearPerInteraction()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	session.hasPlayer = false;
	session.player = {};
	const iggy::InteractionTarget2DRegistry registry = Registry({
		Target("target:one", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 1.0F),
		Target("target:two", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 1.0F),
	});
	const iggy::runtime::GameplayCommandFrame2D frame = CommandFrame({
		factory.interact(PlayerId, iggy::ResourceId { "target:one" }),
		factory.wait(PlayerId),
		factory.interact(PlayerId, iggy::ResourceId { "target:two" }),
	});

	const iggy::runtime::RuntimeInteractionCommandFrameResult result = iggy::runtime::RuntimeInteractionCommandFrameStep {}.evaluate(session, registry, frame);

	Expect(result.interactions.size() == 2, "missing player frame should produce one entry per interact command");
	Expect(result.readyCount == 0, "missing player frame should count zero ready interactions");
	Expect(result.blockedCount == 2, "missing player frame should count all interactions as blocked");
	if (result.interactions.size() == 2) {
		Expect(result.interactions[0].commandIndex == 0 && result.interactions[0].result.status == iggy::runtime::RuntimeInteractionCommandStatus::MissingPlayer, "first missing player entry should preserve status");
		Expect(result.interactions[1].commandIndex == 2 && result.interactions[1].result.status == iggy::runtime::RuntimeInteractionCommandStatus::MissingPlayer, "second missing player entry should preserve status");
	}
}

void TestExtraReachConfigFlowsThroughEntries()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const iggy::InteractionTarget2D target = Target("target:extra", iggy::InteractionTarget2DKind::Inspectable, { 4.0F, 0.0F }, 3.0F);
	const iggy::InteractionTarget2DRegistry registry = Registry({ target });
	const iggy::runtime::GameplayCommandFrame2D frame = CommandFrame({
		factory.interact(PlayerId, target.id),
	});

	const iggy::runtime::RuntimeInteractionCommandFrameResult result = iggy::runtime::RuntimeInteractionCommandFrameStep {}.evaluate(session, registry, frame, { 1.0F });

	Expect(result.interactions.size() == 1, "extra reach frame should produce one interaction entry");
	Expect(result.readyCount == 1, "extra reach frame should count one ready interaction");
	Expect(result.blockedCount == 0, "extra reach frame should count zero blocked interactions");
	if (result.interactions.size() == 1) {
		Expect(result.interactions[0].result.status == iggy::runtime::RuntimeInteractionCommandStatus::Ready, "extra reach entry should be ready");
		Expect(Near(result.interactions[0].result.plan.reach.distance, 4.0F), "extra reach entry should preserve distance");
		Expect(Near(result.interactions[0].result.plan.reach.allowedDistance, 4.0F), "extra reach entry should preserve expanded allowed distance");
	}
}

void TestInputsAreNotMutated()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 1.0F, 2.0F });
	const iggy::runtime::RuntimeSessionState sessionBefore = session;
	const iggy::InteractionTarget2D target = Target("target:immutable", iggy::InteractionTarget2DKind::Door, { 1.0F, 2.0F }, 1.0F);
	const iggy::InteractionTarget2DRegistry registry = Registry({ target });
	const std::vector<iggy::InteractionTarget2D> registryTargetsBefore = registry.targets();
	iggy::runtime::GameplayCommandFrame2D frame = CommandFrame({
		factory.wait(PlayerId),
		factory.interact(PlayerId, target.id),
	});
	const iggy::runtime::GameplayCommandFrame2D frameBefore = frame;

	const iggy::runtime::RuntimeInteractionCommandFrameResult result = iggy::runtime::RuntimeInteractionCommandFrameStep {}.evaluate(session, registry, frame);

	Expect(result.interactions.size() == 1, "immutability setup should evaluate one interaction");
	Expect(session.hasPlayer == sessionBefore.hasPlayer, "interaction frame step should not mutate session hasPlayer");
	Expect(session.tickIndex == sessionBefore.tickIndex, "interaction frame step should not mutate session tickIndex");
	ExpectPlayerAgent(session.player, sessionBefore.player, "interaction frame step input session");
	Expect(registry.targets().size() == registryTargetsBefore.size(), "interaction frame step should not mutate registry target count");
	if (registry.targets().size() == registryTargetsBefore.size())
		ExpectTarget(registry.targets()[0], registryTargetsBefore[0], "interaction frame step should not mutate registry target payload");
	Expect(frame.commands.size() == frameBefore.commands.size(), "interaction frame step should not mutate frame command count");
	if (frame.commands.size() == frameBefore.commands.size()) {
		ExpectCommand(frame.commands[0], frameBefore.commands[0], "interaction frame step should not mutate first frame command");
		ExpectCommand(frame.commands[1], frameBefore.commands[1], "interaction frame step should not mutate second frame command");
	}
}

} // namespace

int main()
{
	TestEmptyFrameProducesNoInteractions();
	TestOnlyNonInteractCommandsAreIgnored();
	TestSingleReachableInteractProducesReadyEntryWithOriginalIndex();
	TestMissingDisabledAndOutOfRangeInteractionsProduceBlockedEntries();
	TestMixedCommandFramePreservesInteractionOrderAndIndexes();
	TestMissingPlayerResultsAppearPerInteraction();
	TestExtraReachConfigFlowsThroughEntries();
	TestInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
