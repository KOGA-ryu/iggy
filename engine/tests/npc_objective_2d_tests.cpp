#include <cstdlib>

#include "scene/ai/NpcObjective2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

bool SameObjective(const iggy::NpcObjective2D &actual, const iggy::NpcObjective2D &expected)
{
	return actual.type == expected.type
		&& actual.targetId == expected.targetId
		&& NearVec(actual.targetPosition, expected.targetPosition);
}

void ExpectObjective(
	const iggy::NpcObjective2D &actual,
	iggy::NpcObjective2DType type,
	iggy::ResourceId targetId,
	iggy::Vec2 targetPosition,
	const char *message)
{
	Expect(actual.type == type, message);
	Expect(actual.targetId == targetId, message);
	Expect(NearVec(actual.targetPosition, targetPosition), message);
}

void ExpectValid(const iggy::NpcObjective2D &objective, const char *message)
{
	const iggy::NpcObjective2DValidationResult result = iggy::validate(objective);
	Expect(result.status == iggy::NpcObjective2DStatus::Valid, message);
	Expect(result.ok(), message);
	Expect(SameObjective(result.objective, objective), message);
	Expect(iggy::valid(objective), message);
}

void ExpectMissingTarget(const iggy::NpcObjective2D &objective, const char *message)
{
	const iggy::NpcObjective2DValidationResult result = iggy::validate(objective);
	Expect(result.status == iggy::NpcObjective2DStatus::MissingTarget, message);
	Expect(!result.ok(), message);
	Expect(SameObjective(result.objective, objective), message);
	Expect(!iggy::valid(objective), message);
}

void TestDefaultObjectiveIsNoneAndValid()
{
	const iggy::NpcObjective2D objective;

	ExpectObjective(objective, iggy::NpcObjective2DType::None, {}, { 0.0F, 0.0F }, "default objective should be None with default payload");
	ExpectValid(objective, "default None objective should be valid");
}

void TestTargetlessObjectivesValidate()
{
	ExpectValid(iggy::noneNpcObjective2D(), "None objective should be valid without target");
	ExpectValid(iggy::waitNpcObjective2D(), "Wait objective should be valid without target");
	ExpectValid(iggy::patrolNpcObjective2D(), "Patrol objective should be valid without target");
	ExpectValid(iggy::guardNpcObjective2D(), "Guard objective should be valid without target");
}

void TestTargetRequiredObjectivesRejectEmptyTarget()
{
	ExpectMissingTarget(iggy::followNpcObjective2D({}), "Follow objective should reject empty target id");
	ExpectMissingTarget(iggy::attackNpcObjective2D({}), "Attack objective should reject empty target id");
	ExpectMissingTarget(iggy::interactNpcObjective2D({}), "Interact objective should reject empty target id");
}

void TestTargetRequiredObjectivesAcceptTarget()
{
	ExpectValid(iggy::followNpcObjective2D(Id("npc:leader")), "Follow objective should accept target id");
	ExpectValid(iggy::attackNpcObjective2D(Id("npc:enemy")), "Attack objective should accept target id");
	ExpectValid(iggy::interactNpcObjective2D(Id("interaction:lever")), "Interact objective should accept target id");
}

void TestPositionBasedObjectivesPreservePositions()
{
	const iggy::NpcObjective2D investigate = iggy::investigateNpcObjective2D({ 1.5F, -2.0F });
	const iggy::NpcObjective2D flee = iggy::fleeNpcObjective2D({ -4.0F, 3.25F });
	const iggy::NpcObjective2D moveTo = iggy::moveToNpcObjective2D({ 8.0F, 9.0F });

	ExpectObjective(investigate, iggy::NpcObjective2DType::Investigate, {}, { 1.5F, -2.0F }, "Investigate objective should preserve target position");
	ExpectObjective(flee, iggy::NpcObjective2DType::Flee, {}, { -4.0F, 3.25F }, "Flee objective should preserve target position");
	ExpectObjective(moveTo, iggy::NpcObjective2DType::MoveTo, {}, { 8.0F, 9.0F }, "MoveTo objective should preserve target position");
	ExpectValid(investigate, "Investigate objective should be valid without target id");
	ExpectValid(flee, "Flee objective should be valid without target id");
	ExpectValid(moveTo, "MoveTo objective should be valid without target id");
}

void TestFactoriesInitializeRelevantFieldsAndDefaultUnrelatedFields()
{
	ExpectObjective(iggy::noneNpcObjective2D(), iggy::NpcObjective2DType::None, {}, { 0.0F, 0.0F }, "none factory should leave payload defaulted");
	ExpectObjective(iggy::waitNpcObjective2D(), iggy::NpcObjective2DType::Wait, {}, { 0.0F, 0.0F }, "wait factory should leave payload defaulted");
	ExpectObjective(iggy::patrolNpcObjective2D(Id("path:alpha")), iggy::NpcObjective2DType::Patrol, Id("path:alpha"), { 0.0F, 0.0F }, "patrol factory should preserve optional target id");
	ExpectObjective(iggy::guardNpcObjective2D(Id("anchor:gate")), iggy::NpcObjective2DType::Guard, Id("anchor:gate"), { 0.0F, 0.0F }, "guard factory should preserve optional target id");
	ExpectObjective(iggy::followNpcObjective2D(Id("npc:leader")), iggy::NpcObjective2DType::Follow, Id("npc:leader"), { 0.0F, 0.0F }, "follow factory should preserve target id");
	ExpectObjective(iggy::attackNpcObjective2D(Id("npc:enemy")), iggy::NpcObjective2DType::Attack, Id("npc:enemy"), { 0.0F, 0.0F }, "attack factory should preserve target id");
	ExpectObjective(iggy::interactNpcObjective2D(Id("target:door")), iggy::NpcObjective2DType::Interact, Id("target:door"), { 0.0F, 0.0F }, "interact factory should preserve target id");
}

void TestNamespacedAndUnqualifiedTargetIdsRemainDistinct()
{
	const iggy::NpcObjective2D unqualified = iggy::attackNpcObjective2D(Id("guard"));
	const iggy::NpcObjective2D namespaced = iggy::attackNpcObjective2D(Id("npc:guard"));

	Expect(unqualified.targetId != namespaced.targetId, "unqualified and namespaced objective targets should be distinct");
	ExpectValid(unqualified, "unqualified target id should still validate when non-empty");
	ExpectValid(namespaced, "namespaced target id should validate when non-empty");
}

void TestValidationAndFactoriesDoNotMutateInputs()
{
	iggy::NpcObjective2D objective = iggy::attackNpcObjective2D(Id("npc:target"));
	const iggy::NpcObjective2D before = objective;

	const iggy::NpcObjective2DValidationResult result = iggy::validate(objective);
	const iggy::NpcObjective2D factoryResult = iggy::moveToNpcObjective2D({ 3.0F, 4.0F });

	Expect(result.ok(), "immutability setup should validate");
	Expect(SameObjective(objective, before), "objective validation should not mutate input");
	ExpectObjective(factoryResult, iggy::NpcObjective2DType::MoveTo, {}, { 3.0F, 4.0F }, "factory immutability setup should create independent objective");
	Expect(SameObjective(objective, before), "objective factories should not mutate unrelated objectives");
}

} // namespace

int main()
{
	TestDefaultObjectiveIsNoneAndValid();
	TestTargetlessObjectivesValidate();
	TestTargetRequiredObjectivesRejectEmptyTarget();
	TestTargetRequiredObjectivesAcceptTarget();
	TestPositionBasedObjectivesPreservePositions();
	TestFactoriesInitializeRelevantFieldsAndDefaultUnrelatedFields();
	TestNamespacedAndUnqualifiedTargetIdsRemainDistinct();
	TestValidationAndFactoriesDoNotMutateInputs();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
