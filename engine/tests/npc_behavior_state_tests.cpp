#include <cstdlib>

#include "scene/npc/NpcBehaviorState.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

bool SameState(const iggy::NpcBehaviorState &actual, const iggy::NpcBehaviorState &expected)
{
	return actual.type == expected.type
		&& actual.targetId == expected.targetId
		&& NearVec(actual.targetPosition, expected.targetPosition);
}

void ExpectState(
	const iggy::NpcBehaviorState &actual,
	iggy::NpcBehaviorStateType type,
	iggy::ResourceId targetId,
	iggy::Vec2 targetPosition,
	const char *message)
{
	Expect(actual.type == type, message);
	Expect(actual.targetId == targetId, message);
	Expect(NearVec(actual.targetPosition, targetPosition), message);
}

void ExpectValid(const iggy::NpcBehaviorState &state, const char *message)
{
	const iggy::NpcBehaviorStateValidationResult result = iggy::validate(state);
	Expect(result.status == iggy::NpcBehaviorStateStatus::Valid, message);
	Expect(result.ok(), message);
	Expect(SameState(result.state, state), message);
	Expect(iggy::valid(state), message);
}

void ExpectMissingTarget(const iggy::NpcBehaviorState &state, const char *message)
{
	const iggy::NpcBehaviorStateValidationResult result = iggy::validate(state);
	Expect(result.status == iggy::NpcBehaviorStateStatus::MissingTarget, message);
	Expect(!result.ok(), message);
	Expect(SameState(result.state, state), message);
	Expect(!iggy::valid(state), message);
}

void TestDefaultStateIsNoneAndValid()
{
	const iggy::NpcBehaviorState state;

	ExpectState(state, iggy::NpcBehaviorStateType::None, {}, { 0.0F, 0.0F }, "default behavior state should be None with default payload");
	ExpectValid(state, "default None behavior state should be valid");
}

void TestTargetlessStatesValidate()
{
	ExpectValid(iggy::noneNpcBehaviorState(), "None behavior state should validate without target");
	ExpectValid(iggy::idleNpcBehaviorState(), "Idle behavior state should validate without target");
	ExpectValid(iggy::waitingNpcBehaviorState(), "Waiting behavior state should validate without target");
	ExpectValid(iggy::stunnedNpcBehaviorState(), "Stunned behavior state should validate without target");
	ExpectValid(iggy::disabledNpcBehaviorState(), "Disabled behavior state should validate without target");
}

void TestTargetRequiredStatesRejectEmptyTarget()
{
	ExpectMissingTarget(iggy::attackingNpcBehaviorState({}), "Attacking behavior state should reject empty target id");
	ExpectMissingTarget(iggy::interactingNpcBehaviorState({}), "Interacting behavior state should reject empty target id");
}

void TestTargetRequiredStatesAcceptTarget()
{
	ExpectValid(iggy::attackingNpcBehaviorState(Id("npc:enemy")), "Attacking behavior state should accept target id");
	ExpectValid(iggy::interactingNpcBehaviorState(Id("interaction:lever")), "Interacting behavior state should accept target id");
}

void TestPositionBasedStatesPreservePositions()
{
	const iggy::NpcBehaviorState seeking = iggy::seekingNpcBehaviorState({ 1.5F, -2.0F });
	const iggy::NpcBehaviorState fleeing = iggy::fleeingNpcBehaviorState({ -4.0F, 3.25F });

	ExpectState(seeking, iggy::NpcBehaviorStateType::Seeking, {}, { 1.5F, -2.0F }, "Seeking behavior state should preserve target position");
	ExpectState(fleeing, iggy::NpcBehaviorStateType::Fleeing, {}, { -4.0F, 3.25F }, "Fleeing behavior state should preserve target position");
	ExpectValid(seeking, "Seeking behavior state should be valid without target id");
	ExpectValid(fleeing, "Fleeing behavior state should be valid without target id");
}

void TestFactoriesInitializeRelevantFieldsAndDefaultUnrelatedFields()
{
	ExpectState(iggy::noneNpcBehaviorState(), iggy::NpcBehaviorStateType::None, {}, { 0.0F, 0.0F }, "none factory should leave payload defaulted");
	ExpectState(iggy::idleNpcBehaviorState(), iggy::NpcBehaviorStateType::Idle, {}, { 0.0F, 0.0F }, "idle factory should leave payload defaulted");
	ExpectState(iggy::waitingNpcBehaviorState(), iggy::NpcBehaviorStateType::Waiting, {}, { 0.0F, 0.0F }, "waiting factory should leave payload defaulted");
	ExpectState(iggy::stunnedNpcBehaviorState(), iggy::NpcBehaviorStateType::Stunned, {}, { 0.0F, 0.0F }, "stunned factory should leave payload defaulted");
	ExpectState(iggy::disabledNpcBehaviorState(), iggy::NpcBehaviorStateType::Disabled, {}, { 0.0F, 0.0F }, "disabled factory should leave payload defaulted");
	ExpectState(iggy::attackingNpcBehaviorState(Id("npc:enemy")), iggy::NpcBehaviorStateType::Attacking, Id("npc:enemy"), { 0.0F, 0.0F }, "attacking factory should preserve target id");
	ExpectState(iggy::interactingNpcBehaviorState(Id("target:door")), iggy::NpcBehaviorStateType::Interacting, Id("target:door"), { 0.0F, 0.0F }, "interacting factory should preserve target id");
}

void TestNamespacedAndUnqualifiedTargetIdsRemainDistinct()
{
	const iggy::NpcBehaviorState unqualified = iggy::attackingNpcBehaviorState(Id("guard"));
	const iggy::NpcBehaviorState namespaced = iggy::attackingNpcBehaviorState(Id("npc:guard"));

	Expect(unqualified.targetId != namespaced.targetId, "unqualified and namespaced behavior state targets should be distinct");
	ExpectValid(unqualified, "unqualified target id should validate when non-empty");
	ExpectValid(namespaced, "namespaced target id should validate when non-empty");
}

void TestValidationAndFactoriesDoNotMutateInputs()
{
	iggy::NpcBehaviorState state = iggy::attackingNpcBehaviorState(Id("npc:target"));
	const iggy::NpcBehaviorState before = state;

	const iggy::NpcBehaviorStateValidationResult result = iggy::validate(state);
	const iggy::NpcBehaviorState factoryResult = iggy::seekingNpcBehaviorState({ 3.0F, 4.0F });

	Expect(result.ok(), "immutability setup should validate");
	Expect(SameState(state, before), "behavior state validation should not mutate input");
	ExpectState(factoryResult, iggy::NpcBehaviorStateType::Seeking, {}, { 3.0F, 4.0F }, "factory immutability setup should create independent behavior state");
	Expect(SameState(state, before), "behavior state factories should not mutate unrelated states");
}

} // namespace

int main()
{
	TestDefaultStateIsNoneAndValid();
	TestTargetlessStatesValidate();
	TestTargetRequiredStatesRejectEmptyTarget();
	TestTargetRequiredStatesAcceptTarget();
	TestPositionBasedStatesPreservePositions();
	TestFactoriesInitializeRelevantFieldsAndDefaultUnrelatedFields();
	TestNamespacedAndUnqualifiedTargetIdsRemainDistinct();
	TestValidationAndFactoriesDoNotMutateInputs();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
