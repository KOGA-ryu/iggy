#include <cstdlib>

#include "scene/npc/NpcBehaviorState2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

bool SameState(const iggy::NpcBehaviorState2D &actual, const iggy::NpcBehaviorState2D &expected)
{
	return actual.type == expected.type
		&& actual.targetId == expected.targetId
		&& NearVec(actual.targetPosition, expected.targetPosition);
}

void ExpectState(
	const iggy::NpcBehaviorState2D &actual,
	iggy::NpcBehaviorState2DType type,
	iggy::ResourceId targetId,
	iggy::Vec2 targetPosition,
	const char *message)
{
	Expect(actual.type == type, message);
	Expect(actual.targetId == targetId, message);
	Expect(NearVec(actual.targetPosition, targetPosition), message);
}

void ExpectValid(const iggy::NpcBehaviorState2D &state, const char *message)
{
	const iggy::NpcBehaviorState2DValidationResult result = iggy::validate(state);
	Expect(result.status == iggy::NpcBehaviorState2DStatus::Valid, message);
	Expect(result.ok(), message);
	Expect(SameState(result.state, state), message);
	Expect(iggy::valid(state), message);
}

void ExpectMissingTarget(const iggy::NpcBehaviorState2D &state, const char *message)
{
	const iggy::NpcBehaviorState2DValidationResult result = iggy::validate(state);
	Expect(result.status == iggy::NpcBehaviorState2DStatus::MissingTarget, message);
	Expect(!result.ok(), message);
	Expect(SameState(result.state, state), message);
	Expect(!iggy::valid(state), message);
}

void TestDefaultStateIsNoneAndValid()
{
	const iggy::NpcBehaviorState2D state;

	ExpectState(state, iggy::NpcBehaviorState2DType::None, {}, { 0.0F, 0.0F }, "default behavior state should be None with default payload");
	ExpectValid(state, "default None behavior state should be valid");
}

void TestTargetlessStatesValidate()
{
	ExpectValid(iggy::noneNpcBehaviorState2D(), "None behavior state should validate without target");
	ExpectValid(iggy::idleNpcBehaviorState2D(), "Idle behavior state should validate without target");
	ExpectValid(iggy::waitingNpcBehaviorState2D(), "Waiting behavior state should validate without target");
	ExpectValid(iggy::stunnedNpcBehaviorState2D(), "Stunned behavior state should validate without target");
	ExpectValid(iggy::disabledNpcBehaviorState2D(), "Disabled behavior state should validate without target");
}

void TestTargetRequiredStatesRejectEmptyTarget()
{
	ExpectMissingTarget(iggy::attackingNpcBehaviorState2D({}), "Attacking behavior state should reject empty target id");
	ExpectMissingTarget(iggy::interactingNpcBehaviorState2D({}), "Interacting behavior state should reject empty target id");
}

void TestTargetRequiredStatesAcceptTarget()
{
	ExpectValid(iggy::attackingNpcBehaviorState2D(Id("npc:enemy")), "Attacking behavior state should accept target id");
	ExpectValid(iggy::interactingNpcBehaviorState2D(Id("interaction:lever")), "Interacting behavior state should accept target id");
}

void TestPositionBasedStatesPreservePositions()
{
	const iggy::NpcBehaviorState2D seeking = iggy::seekingNpcBehaviorState2D({ 1.5F, -2.0F });
	const iggy::NpcBehaviorState2D fleeing = iggy::fleeingNpcBehaviorState2D({ -4.0F, 3.25F });

	ExpectState(seeking, iggy::NpcBehaviorState2DType::Seeking, {}, { 1.5F, -2.0F }, "Seeking behavior state should preserve target position");
	ExpectState(fleeing, iggy::NpcBehaviorState2DType::Fleeing, {}, { -4.0F, 3.25F }, "Fleeing behavior state should preserve target position");
	ExpectValid(seeking, "Seeking behavior state should be valid without target id");
	ExpectValid(fleeing, "Fleeing behavior state should be valid without target id");
}

void TestFactoriesInitializeRelevantFieldsAndDefaultUnrelatedFields()
{
	ExpectState(iggy::noneNpcBehaviorState2D(), iggy::NpcBehaviorState2DType::None, {}, { 0.0F, 0.0F }, "none factory should leave payload defaulted");
	ExpectState(iggy::idleNpcBehaviorState2D(), iggy::NpcBehaviorState2DType::Idle, {}, { 0.0F, 0.0F }, "idle factory should leave payload defaulted");
	ExpectState(iggy::waitingNpcBehaviorState2D(), iggy::NpcBehaviorState2DType::Waiting, {}, { 0.0F, 0.0F }, "waiting factory should leave payload defaulted");
	ExpectState(iggy::stunnedNpcBehaviorState2D(), iggy::NpcBehaviorState2DType::Stunned, {}, { 0.0F, 0.0F }, "stunned factory should leave payload defaulted");
	ExpectState(iggy::disabledNpcBehaviorState2D(), iggy::NpcBehaviorState2DType::Disabled, {}, { 0.0F, 0.0F }, "disabled factory should leave payload defaulted");
	ExpectState(iggy::attackingNpcBehaviorState2D(Id("npc:enemy")), iggy::NpcBehaviorState2DType::Attacking, Id("npc:enemy"), { 0.0F, 0.0F }, "attacking factory should preserve target id");
	ExpectState(iggy::interactingNpcBehaviorState2D(Id("target:door")), iggy::NpcBehaviorState2DType::Interacting, Id("target:door"), { 0.0F, 0.0F }, "interacting factory should preserve target id");
}

void TestNamespacedAndUnqualifiedTargetIdsRemainDistinct()
{
	const iggy::NpcBehaviorState2D unqualified = iggy::attackingNpcBehaviorState2D(Id("guard"));
	const iggy::NpcBehaviorState2D namespaced = iggy::attackingNpcBehaviorState2D(Id("npc:guard"));

	Expect(unqualified.targetId != namespaced.targetId, "unqualified and namespaced behavior state targets should be distinct");
	ExpectValid(unqualified, "unqualified target id should validate when non-empty");
	ExpectValid(namespaced, "namespaced target id should validate when non-empty");
}

void TestValidationAndFactoriesDoNotMutateInputs()
{
	iggy::NpcBehaviorState2D state = iggy::attackingNpcBehaviorState2D(Id("npc:target"));
	const iggy::NpcBehaviorState2D before = state;

	const iggy::NpcBehaviorState2DValidationResult result = iggy::validate(state);
	const iggy::NpcBehaviorState2D factoryResult = iggy::seekingNpcBehaviorState2D({ 3.0F, 4.0F });

	Expect(result.ok(), "immutability setup should validate");
	Expect(SameState(state, before), "behavior state validation should not mutate input");
	ExpectState(factoryResult, iggy::NpcBehaviorState2DType::Seeking, {}, { 3.0F, 4.0F }, "factory immutability setup should create independent behavior state");
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
