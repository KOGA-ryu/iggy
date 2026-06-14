#include <cstdlib>

#include "scene/ai/NpcObjective.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

bool SameObjective(const iggy::NpcObjective &actual, const iggy::NpcObjective &expected)
{
	return actual.type == expected.type
		&& actual.targetId == expected.targetId
		&& NearVec(actual.targetPosition, expected.targetPosition);
}

void ExpectObjective(
	const iggy::NpcObjective &actual,
	iggy::NpcObjectiveType type,
	iggy::ResourceId targetId,
	iggy::Vec2 targetPosition,
	const char *message)
{
	Expect(actual.type == type, message);
	Expect(actual.targetId == targetId, message);
	Expect(NearVec(actual.targetPosition, targetPosition), message);
}

void ExpectValid(const iggy::NpcObjective &objective, const char *message)
{
	const iggy::NpcObjectiveValidationResult result = iggy::validate(objective);
	Expect(result.status == iggy::NpcObjectiveStatus::Valid, message);
	Expect(result.ok(), message);
	Expect(SameObjective(result.objective, objective), message);
	Expect(iggy::valid(objective), message);
}

void ExpectMissingTarget(const iggy::NpcObjective &objective, const char *message)
{
	const iggy::NpcObjectiveValidationResult result = iggy::validate(objective);
	Expect(result.status == iggy::NpcObjectiveStatus::MissingTarget, message);
	Expect(!result.ok(), message);
	Expect(SameObjective(result.objective, objective), message);
	Expect(!iggy::valid(objective), message);
}

void TestDefaultObjectiveIsNoneAndValid()
{
	const iggy::NpcObjective objective;

	ExpectObjective(objective, iggy::NpcObjectiveType::None, {}, { 0.0F, 0.0F }, "default objective should be None with default payload");
	ExpectValid(objective, "default None objective should be valid");
}

void TestTargetlessObjectivesValidate()
{
	ExpectValid(iggy::noneNpcObjective(), "None objective should be valid without target");
	ExpectValid(iggy::waitNpcObjective(), "Wait objective should be valid without target");
	ExpectValid(iggy::patrolNpcObjective(), "Patrol objective should be valid without target");
	ExpectValid(iggy::guardNpcObjective(), "Guard objective should be valid without target");
}

void TestTargetRequiredObjectivesRejectEmptyTarget()
{
	ExpectMissingTarget(iggy::followNpcObjective({}), "Follow objective should reject empty target id");
	ExpectMissingTarget(iggy::attackNpcObjective({}), "Attack objective should reject empty target id");
	ExpectMissingTarget(iggy::interactNpcObjective({}), "Interact objective should reject empty target id");
}

void TestTargetRequiredObjectivesAcceptTarget()
{
	ExpectValid(iggy::followNpcObjective(Id("npc:leader")), "Follow objective should accept target id");
	ExpectValid(iggy::attackNpcObjective(Id("npc:enemy")), "Attack objective should accept target id");
	ExpectValid(iggy::interactNpcObjective(Id("interaction:lever")), "Interact objective should accept target id");
}

void TestPositionBasedObjectivesPreservePositions()
{
	const iggy::NpcObjective investigate = iggy::investigateNpcObjective({ 1.5F, -2.0F });
	const iggy::NpcObjective flee = iggy::fleeNpcObjective({ -4.0F, 3.25F });
	const iggy::NpcObjective moveTo = iggy::moveToNpcObjective({ 8.0F, 9.0F });

	ExpectObjective(investigate, iggy::NpcObjectiveType::Investigate, {}, { 1.5F, -2.0F }, "Investigate objective should preserve target position");
	ExpectObjective(flee, iggy::NpcObjectiveType::Flee, {}, { -4.0F, 3.25F }, "Flee objective should preserve target position");
	ExpectObjective(moveTo, iggy::NpcObjectiveType::MoveTo, {}, { 8.0F, 9.0F }, "MoveTo objective should preserve target position");
	ExpectValid(investigate, "Investigate objective should be valid without target id");
	ExpectValid(flee, "Flee objective should be valid without target id");
	ExpectValid(moveTo, "MoveTo objective should be valid without target id");
}

void TestFactoriesInitializeRelevantFieldsAndDefaultUnrelatedFields()
{
	ExpectObjective(iggy::noneNpcObjective(), iggy::NpcObjectiveType::None, {}, { 0.0F, 0.0F }, "none factory should leave payload defaulted");
	ExpectObjective(iggy::waitNpcObjective(), iggy::NpcObjectiveType::Wait, {}, { 0.0F, 0.0F }, "wait factory should leave payload defaulted");
	ExpectObjective(iggy::patrolNpcObjective(Id("path:alpha")), iggy::NpcObjectiveType::Patrol, Id("path:alpha"), { 0.0F, 0.0F }, "patrol factory should preserve optional target id");
	ExpectObjective(iggy::guardNpcObjective(Id("anchor:gate")), iggy::NpcObjectiveType::Guard, Id("anchor:gate"), { 0.0F, 0.0F }, "guard factory should preserve optional target id");
	ExpectObjective(iggy::followNpcObjective(Id("npc:leader")), iggy::NpcObjectiveType::Follow, Id("npc:leader"), { 0.0F, 0.0F }, "follow factory should preserve target id");
	ExpectObjective(iggy::attackNpcObjective(Id("npc:enemy")), iggy::NpcObjectiveType::Attack, Id("npc:enemy"), { 0.0F, 0.0F }, "attack factory should preserve target id");
	ExpectObjective(iggy::interactNpcObjective(Id("target:door")), iggy::NpcObjectiveType::Interact, Id("target:door"), { 0.0F, 0.0F }, "interact factory should preserve target id");
}

void TestNamespacedAndUnqualifiedTargetIdsRemainDistinct()
{
	const iggy::NpcObjective unqualified = iggy::attackNpcObjective(Id("guard"));
	const iggy::NpcObjective namespaced = iggy::attackNpcObjective(Id("npc:guard"));

	Expect(unqualified.targetId != namespaced.targetId, "unqualified and namespaced objective targets should be distinct");
	ExpectValid(unqualified, "unqualified target id should still validate when non-empty");
	ExpectValid(namespaced, "namespaced target id should validate when non-empty");
}

void TestValidationAndFactoriesDoNotMutateInputs()
{
	iggy::NpcObjective objective = iggy::attackNpcObjective(Id("npc:target"));
	const iggy::NpcObjective before = objective;

	const iggy::NpcObjectiveValidationResult result = iggy::validate(objective);
	const iggy::NpcObjective factoryResult = iggy::moveToNpcObjective({ 3.0F, 4.0F });

	Expect(result.ok(), "immutability setup should validate");
	Expect(SameObjective(objective, before), "objective validation should not mutate input");
	ExpectObjective(factoryResult, iggy::NpcObjectiveType::MoveTo, {}, { 3.0F, 4.0F }, "factory immutability setup should create independent objective");
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
