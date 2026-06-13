#include <cstdlib>
#include <vector>

#include "runtime/RuntimeInteractionCommandStep.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::ExpectPlayerAgent;
using iggy::test::Failures;
using iggy::test::Near;
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

const iggy::ResourceId PlayerId { "player:interaction" };
const iggy::ResourceId TargetId { "target:lever" };

iggy::runtime::RuntimeSessionState SessionWithPlayer(iggy::Vec2 position = { 0.0F, 0.0F })
{
	iggy::runtime::RuntimeSessionState session;
	session.hasPlayer = true;
	session.player = PlayerAgent(PlayerId, position, { 0, 0 }, iggy::PlayerMovementStatus::Moving, iggy::PlayerFacing2D::East);
	session.tickIndex = 17;
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
	Expect(result.built, "runtime interaction test registry setup should build");
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

void TestNonInteractCommandReturnsNotInteractWithoutPlayerOrTarget()
{
	const iggy::runtime::RuntimeSessionState session;
	const iggy::InteractionTarget2DRegistry registry = Registry({});
	const iggy::runtime::GameplayCommand2D command = iggy::runtime::GameplayCommand2DFactory {}.wait(PlayerId);

	const iggy::runtime::RuntimeInteractionCommandResult result = iggy::runtime::RuntimeInteractionCommandStep {}.evaluate(session, registry, command);

	Expect(result.status == iggy::runtime::RuntimeInteractionCommandStatus::NotInteractCommand, "non-interact command should return NotInteractCommand");
	Expect(!result.ready(), "non-interact command result should not be ready");
	ExpectCommand(result.command, command, "non-interact result should preserve command");
	Expect(result.plan.status == iggy::InteractionPlan2DStatus::TargetNotFound, "non-interact result should keep default plan");
}

void TestInteractWithMissingPlayerReturnsMissingPlayer()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	session.hasPlayer = false;
	session.player = {};
	const iggy::InteractionTarget2DRegistry registry = Registry({
		Target("target:lever", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 5.0F),
	});
	const iggy::runtime::GameplayCommand2D command = iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, TargetId);

	const iggy::runtime::RuntimeInteractionCommandResult result = iggy::runtime::RuntimeInteractionCommandStep {}.evaluate(session, registry, command);

	Expect(result.status == iggy::runtime::RuntimeInteractionCommandStatus::MissingPlayer, "interact command with missing player should return MissingPlayer");
	Expect(!result.ready(), "missing player interaction result should not be ready");
	ExpectCommand(result.command, command, "missing player interaction result should preserve command");
	Expect(result.plan.status == iggy::InteractionPlan2DStatus::TargetNotFound, "missing player interaction should keep default plan");
}

void TestInteractWithEmptyTargetIdReturnsMissingTargetId()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::InteractionTarget2DRegistry registry = Registry({});
	const iggy::runtime::GameplayCommand2D command = iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, {});

	const iggy::runtime::RuntimeInteractionCommandResult result = iggy::runtime::RuntimeInteractionCommandStep {}.evaluate(session, registry, command);

	Expect(result.status == iggy::runtime::RuntimeInteractionCommandStatus::MissingTargetId, "empty interact target id should return MissingTargetId");
	Expect(!result.ready(), "empty target id interaction result should not be ready");
	Expect(result.plan.status == iggy::InteractionPlan2DStatus::MissingTargetId, "empty target id interaction should preserve plan status");
	Expect(result.plan.query.status == iggy::InteractionTargetQuery2DStatus::MissingTargetId, "empty target id interaction should preserve query status");
	Expect(result.plan.reach.status == iggy::InteractionReach2DStatus::MissingTargetId, "empty target id interaction should preserve reach status");
}

void TestInteractWithMissingTargetReturnsTargetNotFound()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::InteractionTarget2DRegistry registry = Registry({
		Target("target:known", iggy::InteractionTarget2DKind::Inspectable, { 0.0F, 0.0F }, 1.0F),
	});
	const iggy::runtime::GameplayCommand2D command = iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, iggy::ResourceId { "target:missing" });

	const iggy::runtime::RuntimeInteractionCommandResult result = iggy::runtime::RuntimeInteractionCommandStep {}.evaluate(session, registry, command);

	Expect(result.status == iggy::runtime::RuntimeInteractionCommandStatus::TargetNotFound, "missing interaction target should return TargetNotFound");
	Expect(!result.ready(), "missing target interaction result should not be ready");
	Expect(result.plan.status == iggy::InteractionPlan2DStatus::TargetNotFound, "missing target interaction should preserve plan status");
	Expect(result.plan.query.status == iggy::InteractionTargetQuery2DStatus::NotFound, "missing target interaction should preserve query status");
	Expect(result.plan.reach.status == iggy::InteractionReach2DStatus::TargetNotFound, "missing target interaction should preserve reach status");
}

void TestInteractWithDisabledTargetReturnsTargetDisabled()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::InteractionTarget2D target = Target("target:lever", iggy::InteractionTarget2DKind::Usable, { 0.0F, 0.0F }, 10.0F, false);
	const iggy::InteractionTarget2DRegistry registry = Registry({
		target,
	});
	const iggy::runtime::GameplayCommand2D command = iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, target.id);

	const iggy::runtime::RuntimeInteractionCommandResult result = iggy::runtime::RuntimeInteractionCommandStep {}.evaluate(session, registry, command);

	Expect(result.status == iggy::runtime::RuntimeInteractionCommandStatus::TargetDisabled, "disabled interaction target should return TargetDisabled");
	Expect(!result.ready(), "disabled target interaction result should not be ready");
	Expect(result.plan.status == iggy::InteractionPlan2DStatus::TargetDisabled, "disabled target interaction should preserve plan status");
	ExpectTarget(result.plan.query.target, target, "disabled target interaction should preserve query target");
	ExpectTarget(result.plan.reach.query.target, target, "disabled target interaction should preserve reach query target");
}

void TestInteractWithReachableTargetReturnsReady()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const iggy::InteractionTarget2D target = Target("target:lever", iggy::InteractionTarget2DKind::Usable, { 3.0F, 4.0F }, 5.0F);
	const iggy::InteractionTarget2DRegistry registry = Registry({
		target,
	});
	const iggy::runtime::GameplayCommand2D command = iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, target.id);

	const iggy::runtime::RuntimeInteractionCommandResult result = iggy::runtime::RuntimeInteractionCommandStep {}.evaluate(session, registry, command);

	Expect(result.status == iggy::runtime::RuntimeInteractionCommandStatus::Ready, "reachable interaction target should return Ready");
	Expect(result.ready(), "reachable interaction result should be ready");
	Expect(result.plan.ready(), "reachable interaction should preserve ready plan");
	Expect(result.plan.targetId == target.id, "reachable interaction should preserve target id");
	Expect(NearVec(result.plan.actorPosition, session.player.position), "reachable interaction should use session player position");
	Expect(result.plan.query.status == iggy::InteractionTargetQuery2DStatus::Found, "reachable interaction should preserve query status");
	Expect(result.plan.reach.status == iggy::InteractionReach2DStatus::Reachable, "reachable interaction should preserve reach status");
	Expect(Near(result.plan.reach.distance, 5.0F), "reachable interaction should preserve reach distance");
	Expect(Near(result.plan.reach.allowedDistance, 5.0F), "reachable interaction should preserve allowed distance");
	ExpectTarget(result.plan.query.target, target, "reachable interaction should preserve target details");
}

void TestInteractOutOfRangeReturnsOutOfRange()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const iggy::InteractionTarget2D target = Target("target:far", iggy::InteractionTarget2DKind::Door, { 4.0F, 0.0F }, 3.0F);
	const iggy::InteractionTarget2DRegistry registry = Registry({
		target,
	});
	const iggy::runtime::GameplayCommand2D command = iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, target.id);

	const iggy::runtime::RuntimeInteractionCommandResult result = iggy::runtime::RuntimeInteractionCommandStep {}.evaluate(session, registry, command);

	Expect(result.status == iggy::runtime::RuntimeInteractionCommandStatus::OutOfRange, "out-of-range interaction target should return OutOfRange");
	Expect(!result.ready(), "out-of-range interaction result should not be ready");
	Expect(result.plan.status == iggy::InteractionPlan2DStatus::OutOfRange, "out-of-range interaction should preserve plan status");
	Expect(result.plan.reach.status == iggy::InteractionReach2DStatus::OutOfRange, "out-of-range interaction should preserve reach status");
	Expect(Near(result.plan.reach.distance, 4.0F), "out-of-range interaction should preserve reach distance");
	Expect(Near(result.plan.reach.allowedDistance, 3.0F), "out-of-range interaction should preserve allowed distance");
}

void TestExtraReachCanMakeTargetReady()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const iggy::InteractionTarget2D target = Target("target:extra", iggy::InteractionTarget2DKind::Pickup, { 4.0F, 0.0F }, 3.0F);
	const iggy::InteractionTarget2DRegistry registry = Registry({
		target,
	});
	const iggy::runtime::GameplayCommand2D command = iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, target.id);

	const iggy::runtime::RuntimeInteractionCommandResult result = iggy::runtime::RuntimeInteractionCommandStep {}.evaluate(session, registry, command, { 1.0F });

	Expect(result.status == iggy::runtime::RuntimeInteractionCommandStatus::Ready, "extra reach should make interaction target ready");
	Expect(result.ready(), "extra reach interaction result should be ready");
	Expect(result.plan.status == iggy::InteractionPlan2DStatus::Ready, "extra reach interaction should preserve ready plan");
	Expect(Near(result.plan.reach.distance, 4.0F), "extra reach interaction should preserve distance");
	Expect(Near(result.plan.reach.allowedDistance, 4.0F), "extra reach interaction should preserve expanded allowed distance");
}

void TestInputsAreNotMutated()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 1.0F, 2.0F });
	const iggy::runtime::RuntimeSessionState sessionBefore = session;
	const iggy::InteractionTarget2D target = Target("target:immutable", iggy::InteractionTarget2DKind::Door, { 1.0F, 2.0F }, 1.0F);
	const iggy::InteractionTarget2DRegistry registry = Registry({
		target,
	});
	const std::vector<iggy::InteractionTarget2D> registryTargetsBefore = registry.targets();
	iggy::runtime::GameplayCommand2D command = iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, target.id);
	const iggy::runtime::GameplayCommand2D commandBefore = command;

	const iggy::runtime::RuntimeInteractionCommandResult result = iggy::runtime::RuntimeInteractionCommandStep {}.evaluate(session, registry, command);

	Expect(result.status == iggy::runtime::RuntimeInteractionCommandStatus::Ready, "immutability setup should evaluate ready interaction");
	Expect(session.hasPlayer == sessionBefore.hasPlayer, "interaction command step should not mutate session hasPlayer");
	Expect(session.tickIndex == sessionBefore.tickIndex, "interaction command step should not mutate session tickIndex");
	ExpectPlayerAgent(session.player, sessionBefore.player, "interaction command step input session");
	Expect(registry.targets().size() == registryTargetsBefore.size(), "interaction command step should not mutate registry target count");
	if (registry.targets().size() == registryTargetsBefore.size())
		ExpectTarget(registry.targets()[0], registryTargetsBefore[0], "interaction command step should not mutate registry target payload");
	ExpectCommand(command, commandBefore, "interaction command step should not mutate input command");
}

} // namespace

int main()
{
	TestNonInteractCommandReturnsNotInteractWithoutPlayerOrTarget();
	TestInteractWithMissingPlayerReturnsMissingPlayer();
	TestInteractWithEmptyTargetIdReturnsMissingTargetId();
	TestInteractWithMissingTargetReturnsTargetNotFound();
	TestInteractWithDisabledTargetReturnsTargetDisabled();
	TestInteractWithReachableTargetReturnsReady();
	TestInteractOutOfRangeReturnsOutOfRange();
	TestExtraReachCanMakeTargetReady();
	TestInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
