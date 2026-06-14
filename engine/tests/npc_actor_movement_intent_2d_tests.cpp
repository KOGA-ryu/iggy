#include <cstdlib>
#include <vector>

#include "scene/npc/NpcActorMovementIntent2D.hpp"
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
	iggy::NpcBehaviorState behavior = iggy::idleNpcBehaviorState(),
	iggy::NpcMoveMode moveMode = iggy::NpcMoveMode::Still,
	const char *npcId = "npc:guard")
{
	return {
		Id(npcId),
		iggy::waitNpcObjective(),
		behavior,
		moveMode,
	};
}

iggy::NpcActorFrameState2D Frame(
	const iggy::NpcActorState2D &actor,
	const iggy::NpcActorControlState2D &control,
	bool hasControl = true)
{
	return {
		actor,
		control,
		hasControl,
	};
}

bool SameActor(const iggy::NpcActorState2D &actual, const iggy::NpcActorState2D &expected)
{
	return actual.npcId == expected.npcId
		&& actual.aiProfileId == expected.aiProfileId
		&& actual.factionId == expected.factionId
		&& NearVec(actual.position, expected.position)
		&& actual.currentGoalId == expected.currentGoalId
		&& actual.present == expected.present;
}

bool SameBehavior(const iggy::NpcBehaviorState &actual, const iggy::NpcBehaviorState &expected)
{
	return actual.type == expected.type
		&& actual.targetId == expected.targetId
		&& NearVec(actual.targetPosition, expected.targetPosition);
}

bool SameControl(const iggy::NpcActorControlState2D &actual, const iggy::NpcActorControlState2D &expected)
{
	return actual.npcId == expected.npcId
		&& actual.objective.type == expected.objective.type
		&& actual.objective.targetId == expected.objective.targetId
		&& NearVec(actual.objective.targetPosition, expected.objective.targetPosition)
		&& SameBehavior(actual.behavior, expected.behavior)
		&& actual.moveMode == expected.moveMode;
}

bool SameFrame(const iggy::NpcActorFrameState2D &actual, const iggy::NpcActorFrameState2D &expected)
{
	return SameActor(actual.actor, expected.actor)
		&& SameControl(actual.control, expected.control)
		&& actual.hasControl == expected.hasControl;
}

void ExpectNoMovement(const iggy::NpcActorMovementIntent2D &intent, iggy::NpcActorMovementIntent2DStatus status, const char *message)
{
	Expect(intent.status == status, message);
	Expect(intent.type == iggy::NpcActorMovementIntent2DType::None, "no movement intent should preserve None type");
	Expect(!intent.requestsMovement, "no movement intent should not request movement");
	Expect(!intent.ready(), "no movement intent should not be ready");
}

void TestMissingControlReturnsMissingControlAndPreservesActorFacts()
{
	const iggy::NpcActorFrameState2D frame = Frame(
		Actor("npc:missing", { 3.0F, 4.0F }),
		Control(iggy::seekingNpcBehaviorState({ 9.0F, 9.0F }), iggy::NpcMoveMode::Walk),
		false);

	const iggy::NpcActorMovementIntent2D intent =
		iggy::NpcActorMovementIntentProjector2D {}.project(frame);

	ExpectNoMovement(intent, iggy::NpcActorMovementIntent2DStatus::MissingControl, "missing control should return MissingControl");
	Expect(intent.npcId == Id("npc:missing"), "missing control should preserve actor npc id");
	Expect(NearVec(intent.startPosition, { 3.0F, 4.0F }), "missing control should preserve actor start position");
	Expect(intent.moveMode == iggy::NpcMoveMode::Walk, "missing control should preserve copied control move mode");
	Expect(intent.speedMultiplier == iggy::npcMoveModeSpeedMultiplier(iggy::NpcMoveMode::Walk), "missing control should preserve speed multiplier");
	Expect(SameFrame(intent.frame, frame), "missing control should preserve copied frame");
}

void TestNotPresentActorReturnsActorNotPresent()
{
	const iggy::NpcActorFrameState2D frame = Frame(
		Actor("npc:ghost", { 5.0F, 6.0F }, false),
		Control(iggy::seekingNpcBehaviorState({ 9.0F, 9.0F }), iggy::NpcMoveMode::Run));

	const iggy::NpcActorMovementIntent2D intent =
		iggy::NpcActorMovementIntentProjector2D {}.project(frame);

	ExpectNoMovement(intent, iggy::NpcActorMovementIntent2DStatus::ActorNotPresent, "not-present actor should return ActorNotPresent");
	Expect(intent.npcId == Id("npc:ghost"), "not-present actor should preserve npc id");
	Expect(NearVec(intent.startPosition, { 5.0F, 6.0F }), "not-present actor should preserve start position");
}

void TestTargetlessBehaviorsReturnNoMovement()
{
	const std::vector<iggy::NpcBehaviorState> behaviors {
		iggy::noneNpcBehaviorState(),
		iggy::idleNpcBehaviorState(),
		iggy::waitingNpcBehaviorState(),
		iggy::stunnedNpcBehaviorState(),
		iggy::disabledNpcBehaviorState(),
	};

	for (const iggy::NpcBehaviorState &behavior : behaviors) {
		const iggy::NpcActorFrameState2D frame = Frame(
			Actor(),
			Control(behavior, iggy::NpcMoveMode::Run));
		const iggy::NpcActorMovementIntent2D intent =
			iggy::NpcActorMovementIntentProjector2D {}.project(frame);

		ExpectNoMovement(intent, iggy::NpcActorMovementIntent2DStatus::NoMovement, "targetless behavior should return NoMovement");
	}
}

void TestSeekingWithMovingModeReturnsReadyMoveTo()
{
	const iggy::NpcActorFrameState2D walkFrame = Frame(
		Actor("npc:seeker", { 1.0F, 2.0F }),
		Control(iggy::seekingNpcBehaviorState({ 8.0F, 9.0F }), iggy::NpcMoveMode::Walk));
	const iggy::NpcActorFrameState2D runFrame = Frame(
		Actor("npc:runner", { 3.0F, 4.0F }),
		Control(iggy::seekingNpcBehaviorState({ 10.0F, 11.0F }), iggy::NpcMoveMode::Run, "npc:runner"));

	const iggy::NpcActorMovementIntent2D walk =
		iggy::NpcActorMovementIntentProjector2D {}.project(walkFrame);
	const iggy::NpcActorMovementIntent2D run =
		iggy::NpcActorMovementIntentProjector2D {}.project(runFrame);

	Expect(walk.status == iggy::NpcActorMovementIntent2DStatus::Ready, "seeking walk should be ready");
	Expect(walk.ready() && walk.requestsMovement, "seeking walk should request movement");
	Expect(walk.type == iggy::NpcActorMovementIntent2DType::MoveTo, "seeking walk should produce MoveTo intent");
	Expect(walk.npcId == Id("npc:seeker"), "seeking walk should preserve npc id");
	Expect(NearVec(walk.startPosition, { 1.0F, 2.0F }), "seeking walk should preserve start position");
	Expect(NearVec(walk.targetPosition, { 8.0F, 9.0F }), "seeking walk should preserve target destination");
	Expect(walk.moveMode == iggy::NpcMoveMode::Walk, "seeking walk should preserve move mode");
	Expect(walk.speedMultiplier == iggy::npcMoveModeSpeedMultiplier(iggy::NpcMoveMode::Walk), "seeking walk should preserve speed multiplier");

	Expect(run.status == iggy::NpcActorMovementIntent2DStatus::Ready, "seeking run should be ready");
	Expect(run.type == iggy::NpcActorMovementIntent2DType::MoveTo, "seeking run should produce MoveTo intent");
	Expect(run.moveMode == iggy::NpcMoveMode::Run, "seeking run should preserve run mode");
	Expect(run.speedMultiplier == iggy::npcMoveModeSpeedMultiplier(iggy::NpcMoveMode::Run), "seeking run should preserve run speed multiplier");
	Expect(NearVec(run.targetPosition, { 10.0F, 11.0F }), "seeking run should preserve target destination");
}

void TestFleeingWithMovingModeReturnsReadyMoveAwayFrom()
{
	const iggy::NpcActorFrameState2D runFrame = Frame(
		Actor("npc:flee", { 1.0F, 2.0F }),
		Control(iggy::fleeingNpcBehaviorState({ 4.0F, 5.0F }), iggy::NpcMoveMode::Run));
	const iggy::NpcActorFrameState2D sprintFrame = Frame(
		Actor("npc:sprint", { 6.0F, 7.0F }),
		Control(iggy::fleeingNpcBehaviorState({ 8.0F, 9.0F }), iggy::NpcMoveMode::Sprint, "npc:sprint"));

	const iggy::NpcActorMovementIntent2D run =
		iggy::NpcActorMovementIntentProjector2D {}.project(runFrame);
	const iggy::NpcActorMovementIntent2D sprint =
		iggy::NpcActorMovementIntentProjector2D {}.project(sprintFrame);

	Expect(run.status == iggy::NpcActorMovementIntent2DStatus::Ready, "fleeing run should be ready");
	Expect(run.ready() && run.requestsMovement, "fleeing run should request movement");
	Expect(run.type == iggy::NpcActorMovementIntent2DType::MoveAwayFrom, "fleeing run should produce MoveAwayFrom intent");
	Expect(NearVec(run.targetPosition, { 4.0F, 5.0F }), "fleeing run target should preserve threat/source position to move away from");
	Expect(run.speedMultiplier == iggy::npcMoveModeSpeedMultiplier(iggy::NpcMoveMode::Run), "fleeing run should preserve speed multiplier");

	Expect(sprint.status == iggy::NpcActorMovementIntent2DStatus::Ready, "fleeing sprint should be ready");
	Expect(sprint.type == iggy::NpcActorMovementIntent2DType::MoveAwayFrom, "fleeing sprint should produce MoveAwayFrom intent");
	Expect(sprint.moveMode == iggy::NpcMoveMode::Sprint, "fleeing sprint should preserve sprint mode");
	Expect(sprint.speedMultiplier == iggy::npcMoveModeSpeedMultiplier(iggy::NpcMoveMode::Sprint), "fleeing sprint should preserve speed multiplier");
	Expect(NearVec(sprint.targetPosition, { 8.0F, 9.0F }), "fleeing sprint target should preserve threat/source position");
}

void TestSeekingAndFleeingWithStillOrNoneReturnNoMovement()
{
	const std::vector<iggy::NpcActorFrameState2D> frames {
		Frame(Actor("npc:seek-still"), Control(iggy::seekingNpcBehaviorState({ 9.0F, 9.0F }), iggy::NpcMoveMode::Still)),
		Frame(Actor("npc:seek-none"), Control(iggy::seekingNpcBehaviorState({ 9.0F, 9.0F }), iggy::NpcMoveMode::None)),
		Frame(Actor("npc:flee-still"), Control(iggy::fleeingNpcBehaviorState({ 9.0F, 9.0F }), iggy::NpcMoveMode::Still)),
		Frame(Actor("npc:flee-none"), Control(iggy::fleeingNpcBehaviorState({ 9.0F, 9.0F }), iggy::NpcMoveMode::None)),
	};

	for (const iggy::NpcActorFrameState2D &frame : frames) {
		const iggy::NpcActorMovementIntent2D intent =
			iggy::NpcActorMovementIntentProjector2D {}.project(frame);
		ExpectNoMovement(intent, iggy::NpcActorMovementIntent2DStatus::NoMovement, "movement behavior with stationary mode should return NoMovement");
	}
}

void TestAttackingAndInteractingDefaultUnsupportedEvenWithMovingMode()
{
	const std::vector<iggy::NpcActorFrameState2D> frames {
		Frame(Actor("npc:attack"), Control(iggy::attackingNpcBehaviorState(Id("target:enemy")), iggy::NpcMoveMode::Run)),
		Frame(Actor("npc:interact"), Control(iggy::interactingNpcBehaviorState(Id("target:lever")), iggy::NpcMoveMode::Walk)),
	};

	for (const iggy::NpcActorFrameState2D &frame : frames) {
		const iggy::NpcActorMovementIntent2D intent =
			iggy::NpcActorMovementIntentProjector2D {}.project(frame);
		ExpectNoMovement(intent, iggy::NpcActorMovementIntent2DStatus::UnsupportedBehavior, "attacking/interacting should be unsupported by default");
	}
}

void TestConfigFlagsDoNotInventApproachSemanticsYet()
{
	iggy::NpcActorMovementIntent2DConfig config;
	config.allowAttackingApproach = true;
	config.allowInteractingApproach = true;
	const iggy::NpcActorFrameState2D frame = Frame(
		Actor("npc:attack"),
		Control(iggy::attackingNpcBehaviorState(Id("target:enemy")), iggy::NpcMoveMode::Run));

	const iggy::NpcActorMovementIntent2D intent =
		iggy::NpcActorMovementIntentProjector2D {}.project(frame, config);

	ExpectNoMovement(intent, iggy::NpcActorMovementIntent2DStatus::UnsupportedBehavior, "approach config flags should not add hidden approach behavior in slice 1");
}

void TestInvalidMoveModeReportsInvalidMoveMode()
{
	const iggy::NpcMoveMode invalidMode = static_cast<iggy::NpcMoveMode>(999);
	const iggy::NpcActorFrameState2D frame = Frame(
		Actor("npc:invalid"),
		Control(iggy::seekingNpcBehaviorState({ 9.0F, 9.0F }), invalidMode));

	const iggy::NpcActorMovementIntent2D intent =
		iggy::NpcActorMovementIntentProjector2D {}.project(frame);

	ExpectNoMovement(intent, iggy::NpcActorMovementIntent2DStatus::InvalidMoveMode, "invalid move mode should report InvalidMoveMode");
	Expect(intent.moveMode == invalidMode, "invalid move mode should be preserved for diagnostics");
}

void TestNamespacedAndUnqualifiedNpcIdsArePreservedExactly()
{
	const iggy::NpcActorFrameState2D namespaced = Frame(
		Actor("npc:guard"),
		Control(iggy::seekingNpcBehaviorState({ 1.0F, 1.0F }), iggy::NpcMoveMode::Walk));
	const iggy::NpcActorFrameState2D unqualified = Frame(
		Actor("guard"),
		Control(iggy::seekingNpcBehaviorState({ 2.0F, 2.0F }), iggy::NpcMoveMode::Walk, "guard"));

	const iggy::NpcActorMovementIntent2D namespacedIntent =
		iggy::NpcActorMovementIntentProjector2D {}.project(namespaced);
	const iggy::NpcActorMovementIntent2D unqualifiedIntent =
		iggy::NpcActorMovementIntentProjector2D {}.project(unqualified);

	Expect(namespacedIntent.npcId == Id("npc:guard"), "namespaced npc id should be preserved exactly");
	Expect(unqualifiedIntent.npcId == Id("guard"), "unqualified npc id should be preserved exactly");
	Expect(namespacedIntent.npcId != unqualifiedIntent.npcId, "namespaced and unqualified npc ids should remain distinct");
}

void TestInputFrameIsNotMutated()
{
	iggy::NpcActorFrameState2D frame = Frame(
		Actor("npc:stable", { 1.0F, 2.0F }),
		Control(iggy::seekingNpcBehaviorState({ 8.0F, 9.0F }), iggy::NpcMoveMode::Walk));
	const iggy::NpcActorFrameState2D before = frame;

	const iggy::NpcActorMovementIntent2D intent =
		iggy::NpcActorMovementIntentProjector2D {}.project(frame);

	Expect(intent.ready(), "immutability setup should produce ready intent");
	Expect(SameFrame(frame, before), "movement intent projection should not mutate input frame");
}

} // namespace

int main()
{
	TestMissingControlReturnsMissingControlAndPreservesActorFacts();
	TestNotPresentActorReturnsActorNotPresent();
	TestTargetlessBehaviorsReturnNoMovement();
	TestSeekingWithMovingModeReturnsReadyMoveTo();
	TestFleeingWithMovingModeReturnsReadyMoveAwayFrom();
	TestSeekingAndFleeingWithStillOrNoneReturnNoMovement();
	TestAttackingAndInteractingDefaultUnsupportedEvenWithMovingMode();
	TestConfigFlagsDoNotInventApproachSemanticsYet();
	TestInvalidMoveModeReportsInvalidMoveMode();
	TestNamespacedAndUnqualifiedNpcIdsArePreservedExactly();
	TestInputFrameIsNotMutated();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
