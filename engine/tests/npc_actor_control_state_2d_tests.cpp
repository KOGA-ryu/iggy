#include <cstdlib>
#include <vector>

#include "scene/npc/NpcActorControlState2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::NpcActorControlState2D Control(
	const char *npcId,
	iggy::NpcObjective objective = iggy::waitNpcObjective(),
	iggy::NpcBehaviorState behavior = iggy::idleNpcBehaviorState(),
	iggy::NpcMoveMode moveMode = iggy::NpcMoveMode::Still)
{
	return {
		Id(npcId),
		objective,
		behavior,
		moveMode,
	};
}

bool SameObjective(const iggy::NpcObjective &actual, const iggy::NpcObjective &expected)
{
	return actual.type == expected.type
		&& actual.targetId == expected.targetId
		&& NearVec(actual.targetPosition, expected.targetPosition);
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
		&& SameObjective(actual.objective, expected.objective)
		&& SameBehavior(actual.behavior, expected.behavior)
		&& actual.moveMode == expected.moveMode;
}

bool SameControls(
	const std::vector<iggy::NpcActorControlState2D> &actual,
	const std::vector<iggy::NpcActorControlState2D> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (!SameControl(actual[index], expected[index]))
			return false;
	}
	return true;
}

void ExpectControl(
	const iggy::NpcActorControlState2D &actual,
	const iggy::NpcActorControlState2D &expected,
	const char *message)
{
	Expect(actual.npcId == expected.npcId, message);
	Expect(SameObjective(actual.objective, expected.objective), message);
	Expect(SameBehavior(actual.behavior, expected.behavior), message);
	Expect(actual.moveMode == expected.moveMode, message);
}

void TestEmptyRegistryBuilds()
{
	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build({});

	Expect(result.built, "empty NPC actor control input should build");
	Expect(result.issues.empty(), "empty NPC actor control input should have no issues");
	Expect(result.registry.entries.empty(), "empty NPC actor control input should publish empty registry");
	Expect(!result.registry.contains(Id("npc:missing")), "empty NPC actor control registry should not contain missing id");
	Expect(result.registry.find(Id("npc:missing")) == nullptr, "empty NPC actor control registry should return null for missing id");
}

void TestSuccessfulBuildPreservesControlFacts()
{
	const std::vector<iggy::NpcActorControlState2D> controls {
		Control(
			"npc:guard",
			iggy::guardNpcObjective(Id("anchor:gate")),
			iggy::waitingNpcBehaviorState(),
			iggy::NpcMoveMode::Still),
		Control(
			"npc:scout",
			iggy::investigateNpcObjective({ 1.0F, 2.0F }),
			iggy::seekingNpcBehaviorState({ 3.0F, 4.0F }),
			iggy::NpcMoveMode::Run),
	};

	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);

	Expect(result.built, "valid NPC actor controls should build");
	Expect(result.issues.empty(), "valid NPC actor controls should have no issues");
	Expect(result.registry.entries.size() == controls.size(), "valid NPC actor control registry should preserve entry count");
	if (result.registry.entries.size() == controls.size()) {
		ExpectControl(result.registry.entries[0], controls[0], "first NPC actor control should preserve facts");
		ExpectControl(result.registry.entries[1], controls[1], "second NPC actor control should preserve facts");
	}
}

void TestFindAndContainsUseExactNpcIds()
{
	const std::vector<iggy::NpcActorControlState2D> controls {
		Control("npc:guard"),
		Control("npc:scout"),
	};
	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);

	Expect(result.built, "NPC actor control lookup setup should build");
	Expect(result.registry.contains(Id("npc:guard")), "NPC actor control registry should contain exact id");
	Expect(!result.registry.contains(Id("npc:missing")), "NPC actor control registry should not contain missing id");
	const iggy::NpcActorControlState2D *control = result.registry.find(Id("npc:scout"));
	Expect(control != nullptr, "NPC actor control registry should find exact id");
	if (control != nullptr)
		ExpectControl(*control, controls[1], "NPC actor control find should return matching payload");
	Expect(result.registry.find(Id("npc:missing")) == nullptr, "NPC actor control registry find should return null for missing id");
}

void TestEmptyNpcIdFails()
{
	const iggy::NpcActorControlState2D control = Control("");

	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build({ control });

	Expect(!result.built, "empty NPC actor control id should fail build");
	Expect(result.registry.entries.empty(), "failed empty NPC actor control id build should publish empty registry");
	Expect(result.issues.size() == 1, "empty NPC actor control id should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcActorControlState2DIssueCode::EmptyNpcId, "empty NPC actor control issue should use EmptyNpcId");
		Expect(result.issues[0].controlIndex == 0, "empty NPC actor control issue should preserve index");
		ExpectControl(result.issues[0].control, control, "empty NPC actor control issue should preserve payload");
	}
}

void TestDuplicateNpcIdFailsForLaterEntry()
{
	const std::vector<iggy::NpcActorControlState2D> controls {
		Control("npc:guard", iggy::waitNpcObjective()),
		Control("npc:scout"),
		Control("npc:guard", iggy::moveToNpcObjective({ 4.0F, 5.0F })),
	};

	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);

	Expect(!result.built, "duplicate NPC actor control id should fail build");
	Expect(result.registry.entries.empty(), "failed duplicate NPC actor control id build should publish empty registry");
	Expect(result.issues.size() == 1, "duplicate NPC actor control id should report one issue for later duplicate");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcActorControlState2DIssueCode::DuplicateNpcId, "duplicate NPC actor control issue should use DuplicateNpcId");
		Expect(result.issues[0].controlIndex == 2, "duplicate NPC actor control issue should point to later duplicate");
		ExpectControl(result.issues[0].control, controls[2], "duplicate NPC actor control issue should preserve later payload");
	}
}

void TestInvalidObjectiveFailsWithNestedDiagnostics()
{
	const iggy::NpcActorControlState2D control =
		Control("npc:guard", iggy::attackNpcObjective({}), iggy::idleNpcBehaviorState());

	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build({ control });

	Expect(!result.built, "invalid objective should fail NPC actor control build");
	Expect(result.registry.entries.empty(), "failed invalid objective build should publish empty registry");
	Expect(result.issues.size() == 1, "invalid objective should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcActorControlState2DIssueCode::InvalidObjective, "invalid objective issue should use InvalidObjective");
		Expect(result.issues[0].objectiveValidation.status == iggy::NpcObjectiveStatus::MissingTarget, "invalid objective issue should preserve nested objective status");
		Expect(SameObjective(result.issues[0].objectiveValidation.objective, control.objective), "invalid objective issue should preserve nested objective payload");
		Expect(result.issues[0].behaviorValidation.ok(), "invalid objective issue should preserve valid behavior diagnostics");
	}
}

void TestInvalidBehaviorFailsWithNestedDiagnostics()
{
	const iggy::NpcActorControlState2D control =
		Control("npc:guard", iggy::waitNpcObjective(), iggy::attackingNpcBehaviorState({}));

	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build({ control });

	Expect(!result.built, "invalid behavior should fail NPC actor control build");
	Expect(result.registry.entries.empty(), "failed invalid behavior build should publish empty registry");
	Expect(result.issues.size() == 1, "invalid behavior should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcActorControlState2DIssueCode::InvalidBehavior, "invalid behavior issue should use InvalidBehavior");
		Expect(result.issues[0].behaviorValidation.status == iggy::NpcBehaviorStateStatus::MissingTarget, "invalid behavior issue should preserve nested behavior status");
		Expect(SameBehavior(result.issues[0].behaviorValidation.state, control.behavior), "invalid behavior issue should preserve nested behavior payload");
		Expect(result.issues[0].objectiveValidation.ok(), "invalid behavior issue should preserve valid objective diagnostics");
	}
}

void TestMoveModesArePreservedAndValid()
{
	const std::vector<iggy::NpcActorControlState2D> controls {
		Control("npc:none", iggy::waitNpcObjective(), iggy::idleNpcBehaviorState(), iggy::NpcMoveMode::None),
		Control("npc:still", iggy::waitNpcObjective(), iggy::idleNpcBehaviorState(), iggy::NpcMoveMode::Still),
		Control("npc:run", iggy::moveToNpcObjective({ 3.0F, 0.0F }), iggy::seekingNpcBehaviorState({ 3.0F, 0.0F }), iggy::NpcMoveMode::Run),
	};

	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);

	Expect(result.built, "NPC actor control move modes should build");
	Expect(result.issues.empty(), "NPC actor control move modes should not overvalidate move mode");
	Expect(result.registry.entries.size() == controls.size(), "NPC actor control move modes should preserve entry count");
	if (result.registry.entries.size() == controls.size()) {
		Expect(result.registry.entries[0].moveMode == iggy::NpcMoveMode::None, "None move mode should be preserved");
		Expect(result.registry.entries[1].moveMode == iggy::NpcMoveMode::Still, "Still move mode should be preserved");
		Expect(result.registry.entries[2].moveMode == iggy::NpcMoveMode::Run, "Run move mode should be preserved");
	}
}

void TestNamespacedAndUnqualifiedNpcIdsAreDistinct()
{
	const std::vector<iggy::NpcActorControlState2D> controls {
		Control("guard"),
		Control("npc:guard"),
	};

	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);

	Expect(result.built, "namespaced and unqualified NPC control ids should build distinctly");
	Expect(result.registry.entries.size() == 2, "namespaced and unqualified NPC control ids should both be preserved");
	const iggy::NpcActorControlState2D *unqualified = result.registry.find(Id("guard"));
	const iggy::NpcActorControlState2D *namespaced = result.registry.find(Id("npc:guard"));
	Expect(unqualified != nullptr && namespaced != nullptr, "namespaced and unqualified NPC control ids should both be findable");
	if (unqualified != nullptr && namespaced != nullptr) {
		Expect(unqualified->npcId == Id("guard"), "unqualified NPC control lookup should return unqualified id");
		Expect(namespaced->npcId == Id("npc:guard"), "namespaced NPC control lookup should return namespaced id");
	}
}

void TestMultipleIssuesPreserveDeterministicOrder()
{
	const std::vector<iggy::NpcActorControlState2D> controls {
		Control("npc:valid"),
		Control("", iggy::attackNpcObjective({}), iggy::attackingNpcBehaviorState({})),
		Control("npc:valid", iggy::followNpcObjective({}), iggy::idleNpcBehaviorState()),
	};

	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);

	Expect(!result.built, "multiple NPC actor control issues should fail build");
	Expect(result.registry.entries.empty(), "failed multiple NPC actor control issue build should publish empty registry");
	Expect(result.issues.size() == 5, "multiple NPC actor control issues should preserve deterministic issue count");
	if (result.issues.size() == 5) {
		Expect(result.issues[0].code == iggy::NpcActorControlState2DIssueCode::EmptyNpcId && result.issues[0].controlIndex == 1, "empty npc id should be first issue for second control");
		Expect(result.issues[1].code == iggy::NpcActorControlState2DIssueCode::InvalidObjective && result.issues[1].controlIndex == 1, "invalid objective should follow empty id for second control");
		Expect(result.issues[2].code == iggy::NpcActorControlState2DIssueCode::InvalidBehavior && result.issues[2].controlIndex == 1, "invalid behavior should follow invalid objective for second control");
		Expect(result.issues[3].code == iggy::NpcActorControlState2DIssueCode::DuplicateNpcId && result.issues[3].controlIndex == 2, "duplicate npc id should be reported for third control");
		Expect(result.issues[4].code == iggy::NpcActorControlState2DIssueCode::InvalidObjective && result.issues[4].controlIndex == 2, "invalid objective should also be reported for third control");
	}
}

void TestBuildDoesNotMutateInputs()
{
	std::vector<iggy::NpcActorControlState2D> controls {
		Control(
			"npc:guard",
			iggy::guardNpcObjective(Id("anchor:gate")),
			iggy::waitingNpcBehaviorState(),
			iggy::NpcMoveMode::Still),
		Control(
			"npc:scout",
			iggy::moveToNpcObjective({ 5.0F, 6.0F }),
			iggy::seekingNpcBehaviorState({ 5.0F, 6.0F }),
			iggy::NpcMoveMode::Jog),
	};
	const std::vector<iggy::NpcActorControlState2D> before = controls;

	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);

	Expect(result.built, "immutability setup should build NPC actor controls");
	Expect(SameControls(controls, before), "NPC actor control builder should not mutate inputs");
}

} // namespace

int main()
{
	TestEmptyRegistryBuilds();
	TestSuccessfulBuildPreservesControlFacts();
	TestFindAndContainsUseExactNpcIds();
	TestEmptyNpcIdFails();
	TestDuplicateNpcIdFailsForLaterEntry();
	TestInvalidObjectiveFailsWithNestedDiagnostics();
	TestInvalidBehaviorFailsWithNestedDiagnostics();
	TestMoveModesArePreservedAndValid();
	TestNamespacedAndUnqualifiedNpcIdsAreDistinct();
	TestMultipleIssuesPreserveDeterministicOrder();
	TestBuildDoesNotMutateInputs();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
