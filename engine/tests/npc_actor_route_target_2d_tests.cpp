#include <cstdlib>

#include "scene/npc/NpcActorRouteTarget2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::NpcActorState2D Actor(
	const char *npcId = "npc:guard",
	iggy::Vec2 position = { 1.0F, 2.0F },
	bool present = true)
{
	return {
		Id(npcId),
		Id("profile:guard"),
		Id("faction:town"),
		position,
		{},
		present,
	};
}

iggy::NpcActorControlState2D Control(
	iggy::NpcBehaviorState behavior = iggy::seekingNpcBehaviorState({ 8.0F, 9.0F }),
	iggy::NpcMoveMode moveMode = iggy::NpcMoveMode::Walk,
	const char *npcId = "npc:guard")
{
	return {
		Id(npcId),
		iggy::waitNpcObjective(),
		behavior,
		moveMode,
	};
}

iggy::NpcActorMovementIntent2D IntentFromFrame(
	const iggy::NpcActorState2D &actor,
	const iggy::NpcActorControlState2D &control,
	bool hasControl = true)
{
	return iggy::NpcActorMovementIntentProjector2D {}.project({
		actor,
		control,
		hasControl,
	});
}

bool SameIntent(const iggy::NpcActorMovementIntent2D &actual, const iggy::NpcActorMovementIntent2D &expected)
{
	return actual.status == expected.status
		&& actual.type == expected.type
		&& actual.npcId == expected.npcId
		&& NearVec(actual.startPosition, expected.startPosition)
		&& NearVec(actual.targetPosition, expected.targetPosition)
		&& actual.moveMode == expected.moveMode
		&& actual.speedMultiplier == expected.speedMultiplier
		&& actual.requestsMovement == expected.requestsMovement;
}

void ExpectNoRoute(
	const iggy::NpcActorRouteTarget2D &target,
	iggy::NpcActorRouteTarget2DStatus status,
	const char *message)
{
	Expect(target.status == status, message);
	Expect(!target.requestsRoute, "non-ready route target should not request a route");
	Expect(!target.ready(), "non-ready route target should not be ready");
}

void TestNonReadyIntentReturnsNoMovementIntent()
{
	const iggy::NpcActorMovementIntent2D intent = IntentFromFrame(
		Actor("npc:idle", { 3.0F, 4.0F }),
		Control(iggy::idleNpcBehaviorState(), iggy::NpcMoveMode::Still, "npc:idle"));

	const iggy::NpcActorRouteTarget2D target =
		iggy::NpcActorRouteTargetProjector2D {}.project(intent);

	ExpectNoRoute(target, iggy::NpcActorRouteTarget2DStatus::NoMovementIntent, "non-ready movement intent should return NoMovementIntent");
	Expect(SameIntent(target.intent, intent), "route target should preserve copied intent");
	Expect(target.npcId == Id("npc:idle"), "route target should preserve npc id");
	Expect(NearVec(target.startPosition, { 3.0F, 4.0F }), "route target should preserve start position");
	Expect(target.moveMode == iggy::NpcMoveMode::Still, "route target should preserve move mode");
}

void TestMoveToReadyIntentPassesThroughDestination()
{
	const iggy::NpcActorMovementIntent2D intent = IntentFromFrame(
		Actor("npc:seeker", { 1.0F, 2.0F }),
		Control(iggy::seekingNpcBehaviorState({ 8.0F, 9.0F }), iggy::NpcMoveMode::Run, "npc:seeker"));

	const iggy::NpcActorRouteTarget2D target =
		iggy::NpcActorRouteTargetProjector2D {}.project(intent);

	Expect(target.status == iggy::NpcActorRouteTarget2DStatus::Ready, "ready MoveTo intent should produce ready route target");
	Expect(target.ready() && target.requestsRoute, "ready MoveTo route target should request a route");
	Expect(target.type == iggy::NpcActorRouteTarget2DType::MoveTo, "ready MoveTo route target should preserve type");
	Expect(target.npcId == Id("npc:seeker"), "ready MoveTo route target should preserve npc id");
	Expect(NearVec(target.startPosition, { 1.0F, 2.0F }), "ready MoveTo route target should preserve start position");
	Expect(NearVec(target.targetPosition, { 8.0F, 9.0F }), "ready MoveTo route target should preserve destination");
	Expect(target.moveMode == iggy::NpcMoveMode::Run, "ready MoveTo route target should preserve move mode");
	Expect(target.speedMultiplier == iggy::npcMoveModeSpeedMultiplier(iggy::NpcMoveMode::Run), "ready MoveTo route target should preserve speed multiplier");
}

void TestMoveToAlreadyAtTargetReturnsAlreadyAtTarget()
{
	const iggy::NpcActorMovementIntent2D intent = IntentFromFrame(
		Actor("npc:arrived", { 5.0F, 5.0F }),
		Control(iggy::seekingNpcBehaviorState({ 5.0F, 5.0F }), iggy::NpcMoveMode::Walk, "npc:arrived"));

	const iggy::NpcActorRouteTarget2D target =
		iggy::NpcActorRouteTargetProjector2D {}.project(intent);

	ExpectNoRoute(target, iggy::NpcActorRouteTarget2DStatus::AlreadyAtTarget, "MoveTo intent at destination should return AlreadyAtTarget");
	Expect(target.type == iggy::NpcActorRouteTarget2DType::MoveTo, "already-at-target MoveTo should preserve route target type");
	Expect(NearVec(target.targetPosition, { 5.0F, 5.0F }), "already-at-target MoveTo should preserve destination");
}

void TestMoveAwayFromWithoutEscapeDestinationNeedsEscapeDestination()
{
	const iggy::NpcActorMovementIntent2D intent = IntentFromFrame(
		Actor("npc:flee", { 1.0F, 2.0F }),
		Control(iggy::fleeingNpcBehaviorState({ 8.0F, 9.0F }), iggy::NpcMoveMode::Run, "npc:flee"));

	const iggy::NpcActorRouteTarget2D target =
		iggy::NpcActorRouteTargetProjector2D {}.project(intent);

	ExpectNoRoute(target, iggy::NpcActorRouteTarget2DStatus::NeedsEscapeDestination, "MoveAwayFrom intent without explicit escape destination should need one");
	Expect(target.type == iggy::NpcActorRouteTarget2DType::None, "MoveAwayFrom without destination should not fake a route target type");
	Expect(NearVec(target.sourcePosition, { 8.0F, 9.0F }), "MoveAwayFrom should preserve threat/source position");
	Expect(NearVec(target.targetPosition, { 0.0F, 0.0F }), "MoveAwayFrom without escape destination should not use the threat as destination");
}

void TestMoveAwayFromWithEscapeDestinationReturnsReadyRoute()
{
	const iggy::NpcActorMovementIntent2D intent = IntentFromFrame(
		Actor("npc:flee", { 1.0F, 2.0F }),
		Control(iggy::fleeingNpcBehaviorState({ 8.0F, 9.0F }), iggy::NpcMoveMode::Sprint, "npc:flee"));
	iggy::NpcActorRouteTarget2DConfig config;
	config.hasEscapeDestination = true;
	config.escapeDestination = { -4.0F, -5.0F };

	const iggy::NpcActorRouteTarget2D target =
		iggy::NpcActorRouteTargetProjector2D {}.project(intent, config);

	Expect(target.status == iggy::NpcActorRouteTarget2DStatus::Ready, "MoveAwayFrom with explicit escape destination should be ready");
	Expect(target.ready() && target.requestsRoute, "MoveAwayFrom with explicit escape destination should request route");
	Expect(target.type == iggy::NpcActorRouteTarget2DType::MoveAwayFrom, "MoveAwayFrom with explicit escape destination should preserve route target type");
	Expect(NearVec(target.sourcePosition, { 8.0F, 9.0F }), "MoveAwayFrom with explicit destination should preserve source/threat position");
	Expect(NearVec(target.targetPosition, { -4.0F, -5.0F }), "MoveAwayFrom with explicit destination should route to escape destination");
	Expect(target.moveMode == iggy::NpcMoveMode::Sprint, "MoveAwayFrom with explicit destination should preserve move mode");
}

void TestMoveAwayFromEscapeDestinationAlreadyReached()
{
	const iggy::NpcActorMovementIntent2D intent = IntentFromFrame(
		Actor("npc:escaped", { -4.0F, -5.0F }),
		Control(iggy::fleeingNpcBehaviorState({ 8.0F, 9.0F }), iggy::NpcMoveMode::Run, "npc:escaped"));
	iggy::NpcActorRouteTarget2DConfig config;
	config.hasEscapeDestination = true;
	config.escapeDestination = { -4.0F, -5.0F };

	const iggy::NpcActorRouteTarget2D target =
		iggy::NpcActorRouteTargetProjector2D {}.project(intent, config);

	ExpectNoRoute(target, iggy::NpcActorRouteTarget2DStatus::AlreadyAtTarget, "MoveAwayFrom already at explicit escape destination should not request route");
	Expect(target.type == iggy::NpcActorRouteTarget2DType::MoveAwayFrom, "MoveAwayFrom already at escape destination should preserve route target type");
	Expect(NearVec(target.targetPosition, { -4.0F, -5.0F }), "MoveAwayFrom already at escape destination should preserve destination");
}

void TestArrivalToleranceIncludesEqualDistance()
{
	const iggy::NpcActorMovementIntent2D intent = IntentFromFrame(
		Actor("npc:tolerance", { 0.0F, 0.0F }),
		Control(iggy::seekingNpcBehaviorState({ 0.0F, 0.5F }), iggy::NpcMoveMode::Walk, "npc:tolerance"));
	iggy::NpcActorRouteTarget2DConfig config;
	config.arrivalTolerance = 0.5F;

	const iggy::NpcActorRouteTarget2D target =
		iggy::NpcActorRouteTargetProjector2D {}.project(intent, config);

	ExpectNoRoute(target, iggy::NpcActorRouteTarget2DStatus::AlreadyAtTarget, "arrival distance equal to tolerance should count as reached");
}

void TestInvalidIntentTypeReportsInvalidTarget()
{
	iggy::NpcActorMovementIntent2D intent = IntentFromFrame(
		Actor("npc:invalid", { 1.0F, 1.0F }),
		Control(iggy::seekingNpcBehaviorState({ 2.0F, 2.0F }), iggy::NpcMoveMode::Walk, "npc:invalid"));
	intent.type = static_cast<iggy::NpcActorMovementIntent2DType>(999);

	const iggy::NpcActorRouteTarget2D target =
		iggy::NpcActorRouteTargetProjector2D {}.project(intent);

	ExpectNoRoute(target, iggy::NpcActorRouteTarget2DStatus::InvalidTarget, "unknown ready intent type should report InvalidTarget");
	Expect(SameIntent(target.intent, intent), "invalid target should preserve copied malformed intent");
}

void TestNamespacedAndUnqualifiedNpcIdsArePreservedExactly()
{
	const iggy::NpcActorMovementIntent2D namespaced = IntentFromFrame(
		Actor("npc:guard"),
		Control(iggy::seekingNpcBehaviorState({ 2.0F, 2.0F }), iggy::NpcMoveMode::Walk, "npc:guard"));
	const iggy::NpcActorMovementIntent2D unqualified = IntentFromFrame(
		Actor("guard"),
		Control(iggy::seekingNpcBehaviorState({ 3.0F, 3.0F }), iggy::NpcMoveMode::Walk, "guard"));

	const iggy::NpcActorRouteTarget2D namespacedTarget =
		iggy::NpcActorRouteTargetProjector2D {}.project(namespaced);
	const iggy::NpcActorRouteTarget2D unqualifiedTarget =
		iggy::NpcActorRouteTargetProjector2D {}.project(unqualified);

	Expect(namespacedTarget.npcId == Id("npc:guard"), "namespaced npc id should be preserved exactly");
	Expect(unqualifiedTarget.npcId == Id("guard"), "unqualified npc id should be preserved exactly");
	Expect(namespacedTarget.npcId != unqualifiedTarget.npcId, "namespaced and unqualified npc ids should remain distinct");
}

void TestInputIntentIsNotMutated()
{
	iggy::NpcActorMovementIntent2D intent = IntentFromFrame(
		Actor("npc:stable", { 1.0F, 2.0F }),
		Control(iggy::fleeingNpcBehaviorState({ 8.0F, 9.0F }), iggy::NpcMoveMode::Run, "npc:stable"));
	const iggy::NpcActorMovementIntent2D before = intent;
	iggy::NpcActorRouteTarget2DConfig config;
	config.hasEscapeDestination = true;
	config.escapeDestination = { -4.0F, -5.0F };

	const iggy::NpcActorRouteTarget2D target =
		iggy::NpcActorRouteTargetProjector2D {}.project(intent, config);

	Expect(target.ready(), "immutability setup should produce ready route target");
	Expect(SameIntent(intent, before), "route target projection should not mutate input intent");
}

} // namespace

int main()
{
	TestNonReadyIntentReturnsNoMovementIntent();
	TestMoveToReadyIntentPassesThroughDestination();
	TestMoveToAlreadyAtTargetReturnsAlreadyAtTarget();
	TestMoveAwayFromWithoutEscapeDestinationNeedsEscapeDestination();
	TestMoveAwayFromWithEscapeDestinationReturnsReadyRoute();
	TestMoveAwayFromEscapeDestinationAlreadyReached();
	TestArrivalToleranceIncludesEqualDistance();
	TestInvalidIntentTypeReportsInvalidTarget();
	TestNamespacedAndUnqualifiedNpcIdsArePreservedExactly();
	TestInputIntentIsNotMutated();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
