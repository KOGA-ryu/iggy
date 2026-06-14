#include <cstdlib>
#include <vector>

#include "scene/npc/NpcActorFrameState2D.hpp"
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
	const char *npcId,
	const char *aiProfileId = "ai-profile:guard",
	const char *factionId = "faction:town",
	iggy::Vec2 position = { 0.0F, 0.0F },
	const char *currentGoalId = "",
	bool present = true)
{
	return {
		Id(npcId),
		Id(aiProfileId),
		Id(factionId),
		position,
		Id(currentGoalId),
		present,
	};
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

iggy::NpcActorState2DRegistry ActorRegistry(const std::vector<iggy::NpcActorState2D> &actors)
{
	return iggy::NpcActorState2DRegistryBuilder {}.build(actors).registry;
}

iggy::NpcActorControlState2DRegistry ControlRegistry(const std::vector<iggy::NpcActorControlState2D> &controls)
{
	return iggy::NpcActorControlState2DRegistryBuilder {}.build(controls).registry;
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

bool SameActor(const iggy::NpcActorState2D &actual, const iggy::NpcActorState2D &expected)
{
	return actual.npcId == expected.npcId
		&& actual.aiProfileId == expected.aiProfileId
		&& actual.factionId == expected.factionId
		&& NearVec(actual.position, expected.position)
		&& actual.currentGoalId == expected.currentGoalId
		&& actual.present == expected.present;
}

bool SameControl(const iggy::NpcActorControlState2D &actual, const iggy::NpcActorControlState2D &expected)
{
	return actual.npcId == expected.npcId
		&& SameObjective(actual.objective, expected.objective)
		&& SameBehavior(actual.behavior, expected.behavior)
		&& actual.moveMode == expected.moveMode;
}

bool SameActors(
	const std::vector<iggy::NpcActorState2D> &actual,
	const std::vector<iggy::NpcActorState2D> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (!SameActor(actual[index], expected[index]))
			return false;
	}
	return true;
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

void TestEmptyRegistriesProjectEmpty()
{
	const iggy::NpcActorFrameState2DProjectionResult result =
		iggy::NpcActorFrameStateProjector2D {}.project({}, {});

	Expect(result.entries.empty(), "empty actor/control registries should project no frame entries");
	Expect(result.issues.empty(), "empty actor/control registries should project no issues");
	Expect(!result.hasIssues(), "empty actor/control projection should not have issues");
}

void TestMatchingActorAndControlJoin()
{
	const std::vector<iggy::NpcActorState2D> actors {
		Actor("npc:guard", "ai-profile:guard", "faction:town", { 1.0F, 2.0F }, "goal:gate", true),
	};
	const std::vector<iggy::NpcActorControlState2D> controls {
		Control(
			"npc:guard",
			iggy::guardNpcObjective(Id("anchor:gate")),
			iggy::waitingNpcBehaviorState(),
			iggy::NpcMoveMode::Still),
	};

	const iggy::NpcActorFrameState2DProjectionResult result =
		iggy::NpcActorFrameStateProjector2D {}.project(ActorRegistry(actors), ControlRegistry(controls));

	Expect(result.entries.size() == 1, "matching actor/control projection should create one entry");
	Expect(result.issues.empty(), "matching actor/control projection should not produce issues");
	if (result.entries.size() == 1) {
		Expect(result.entries[0].hasControl, "matching actor/control projection should mark control present");
		Expect(SameActor(result.entries[0].actor, actors[0]), "matching actor/control projection should copy actor");
		Expect(SameControl(result.entries[0].control, controls[0]), "matching actor/control projection should copy control");
	}
}

void TestMultipleActorsPreserveActorOrder()
{
	const std::vector<iggy::NpcActorState2D> actors {
		Actor("npc:first", "ai-profile:first", "faction:a", { 1.0F, 0.0F }),
		Actor("npc:second", "ai-profile:second", "faction:b", { 2.0F, 0.0F }),
		Actor("npc:third", "ai-profile:third", "faction:c", { 3.0F, 0.0F }),
	};
	const std::vector<iggy::NpcActorControlState2D> controls {
		Control("npc:third", iggy::moveToNpcObjective({ 3.0F, 3.0F }), iggy::seekingNpcBehaviorState({ 3.0F, 3.0F }), iggy::NpcMoveMode::Run),
		Control("npc:first", iggy::waitNpcObjective(), iggy::idleNpcBehaviorState(), iggy::NpcMoveMode::Still),
		Control("npc:second", iggy::patrolNpcObjective(Id("patrol:two")), iggy::waitingNpcBehaviorState(), iggy::NpcMoveMode::Walk),
	};

	const iggy::NpcActorFrameState2DProjectionResult result =
		iggy::NpcActorFrameStateProjector2D {}.project(ActorRegistry(actors), ControlRegistry(controls));

	Expect(result.entries.size() == actors.size(), "frame projection should preserve actor entry count");
	Expect(result.issues.empty(), "complete frame projection should not have issues");
	if (result.entries.size() == actors.size()) {
		Expect(SameActor(result.entries[0].actor, actors[0]), "first frame entry should preserve first actor");
		Expect(SameActor(result.entries[1].actor, actors[1]), "second frame entry should preserve second actor");
		Expect(SameActor(result.entries[2].actor, actors[2]), "third frame entry should preserve third actor");
		Expect(SameControl(result.entries[0].control, controls[1]), "first frame entry should join matching control by id");
		Expect(SameControl(result.entries[1].control, controls[2]), "second frame entry should join matching control by id");
		Expect(SameControl(result.entries[2].control, controls[0]), "third frame entry should join matching control by id");
	}
}

void TestMissingControlCreatesIssueAndDefaultControl()
{
	const std::vector<iggy::NpcActorState2D> actors {
		Actor("npc:guard"),
	};

	const iggy::NpcActorFrameState2DProjectionResult result =
		iggy::NpcActorFrameStateProjector2D {}.project(ActorRegistry(actors), {});

	Expect(result.entries.size() == 1, "missing control projection should still create actor frame entry");
	Expect(result.issues.size() == 1, "missing control projection should create one issue");
	Expect(result.hasIssues(), "missing control projection should report issues");
	if (result.entries.size() == 1) {
		Expect(SameActor(result.entries[0].actor, actors[0]), "missing control projection should copy actor");
		Expect(!result.entries[0].hasControl, "missing control projection should mark control absent");
		Expect(result.entries[0].control.npcId.empty(), "missing control projection should use default control payload");
	}
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcActorFrameState2DIssueCode::MissingControlState, "missing control issue should use MissingControlState");
		Expect(result.issues[0].actorIndex == 0, "missing control issue should preserve actor index");
		Expect(SameActor(result.issues[0].actor, actors[0]), "missing control issue should preserve actor payload");
	}
}

void TestOrphanControlCreatesIssueOnly()
{
	const std::vector<iggy::NpcActorControlState2D> controls {
		Control("npc:orphan", iggy::moveToNpcObjective({ 4.0F, 5.0F }), iggy::seekingNpcBehaviorState({ 4.0F, 5.0F }), iggy::NpcMoveMode::Jog),
	};

	const iggy::NpcActorFrameState2DProjectionResult result =
		iggy::NpcActorFrameStateProjector2D {}.project({}, ControlRegistry(controls));

	Expect(result.entries.empty(), "orphan control projection should not create actor frame entry");
	Expect(result.issues.size() == 1, "orphan control projection should create one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcActorFrameState2DIssueCode::OrphanControlState, "orphan control issue should use OrphanControlState");
		Expect(result.issues[0].controlIndex == 0, "orphan control issue should preserve control index");
		Expect(SameControl(result.issues[0].control, controls[0]), "orphan control issue should preserve control payload");
	}
}

void TestIssueOrderIsMissingThenOrphan()
{
	const std::vector<iggy::NpcActorState2D> actors {
		Actor("npc:missing-a"),
		Actor("npc:matched"),
		Actor("npc:missing-b"),
	};
	const std::vector<iggy::NpcActorControlState2D> controls {
		Control("npc:orphan-a"),
		Control("npc:matched"),
		Control("npc:orphan-b"),
	};

	const iggy::NpcActorFrameState2DProjectionResult result =
		iggy::NpcActorFrameStateProjector2D {}.project(ActorRegistry(actors), ControlRegistry(controls));

	Expect(result.entries.size() == 3, "mixed projection should preserve actor entry count");
	Expect(result.issues.size() == 4, "mixed projection should report missing controls then orphan controls");
	if (result.issues.size() == 4) {
		Expect(result.issues[0].code == iggy::NpcActorFrameState2DIssueCode::MissingControlState && result.issues[0].actorIndex == 0, "first issue should be first missing actor control");
		Expect(result.issues[1].code == iggy::NpcActorFrameState2DIssueCode::MissingControlState && result.issues[1].actorIndex == 2, "second issue should be second missing actor control");
		Expect(result.issues[2].code == iggy::NpcActorFrameState2DIssueCode::OrphanControlState && result.issues[2].controlIndex == 0, "third issue should be first orphan control");
		Expect(result.issues[3].code == iggy::NpcActorFrameState2DIssueCode::OrphanControlState && result.issues[3].controlIndex == 2, "fourth issue should be second orphan control");
	}
}

void TestNamespacedAndUnqualifiedIdsAreDistinct()
{
	const std::vector<iggy::NpcActorState2D> actors {
		Actor("guard", "ai-profile:plain"),
		Actor("npc:guard", "ai-profile:namespaced"),
	};
	const std::vector<iggy::NpcActorControlState2D> controls {
		Control("npc:guard", iggy::patrolNpcObjective(Id("patrol:namespaced")), iggy::waitingNpcBehaviorState(), iggy::NpcMoveMode::Walk),
		Control("guard", iggy::waitNpcObjective(), iggy::idleNpcBehaviorState(), iggy::NpcMoveMode::Still),
	};

	const iggy::NpcActorFrameState2DProjectionResult result =
		iggy::NpcActorFrameStateProjector2D {}.project(ActorRegistry(actors), ControlRegistry(controls));

	Expect(result.entries.size() == 2, "namespaced and unqualified projection should preserve both actors");
	Expect(result.issues.empty(), "namespaced and unqualified ids should not create projection issues");
	if (result.entries.size() == 2) {
		Expect(result.entries[0].actor.npcId == Id("guard"), "first actor should remain unqualified");
		Expect(result.entries[0].control.npcId == Id("guard"), "first control should match unqualified id");
		Expect(result.entries[1].actor.npcId == Id("npc:guard"), "second actor should remain namespaced");
		Expect(result.entries[1].control.npcId == Id("npc:guard"), "second control should match namespaced id");
	}
}

void TestProjectionDoesNotMutateInputs()
{
	std::vector<iggy::NpcActorState2D> actors {
		Actor("npc:guard", "ai-profile:guard", "faction:town", { 1.0F, 2.0F }, "goal:guard", true),
		Actor("npc:scout", "ai-profile:scout", "faction:town", { 3.0F, 4.0F }, "", false),
	};
	std::vector<iggy::NpcActorControlState2D> controls {
		Control("npc:guard", iggy::guardNpcObjective(Id("anchor:gate")), iggy::waitingNpcBehaviorState(), iggy::NpcMoveMode::Still),
		Control("npc:scout", iggy::moveToNpcObjective({ 8.0F, 9.0F }), iggy::seekingNpcBehaviorState({ 8.0F, 9.0F }), iggy::NpcMoveMode::Run),
	};
	const std::vector<iggy::NpcActorState2D> actorsBefore = actors;
	const std::vector<iggy::NpcActorControlState2D> controlsBefore = controls;
	const iggy::NpcActorState2DRegistry actorRegistry = ActorRegistry(actors);
	const iggy::NpcActorControlState2DRegistry controlRegistry = ControlRegistry(controls);

	const iggy::NpcActorFrameState2DProjectionResult result =
		iggy::NpcActorFrameStateProjector2D {}.project(actorRegistry, controlRegistry);

	Expect(result.entries.size() == 2, "immutability setup should project frame entries");
	Expect(SameActors(actors, actorsBefore), "frame projection should not mutate actor inputs");
	Expect(SameControls(controls, controlsBefore), "frame projection should not mutate control inputs");
	Expect(SameActors(actorRegistry.actors, actorsBefore), "frame projection should not mutate actor registry");
	Expect(SameControls(controlRegistry.entries, controlsBefore), "frame projection should not mutate control registry");
}

} // namespace

int main()
{
	TestEmptyRegistriesProjectEmpty();
	TestMatchingActorAndControlJoin();
	TestMultipleActorsPreserveActorOrder();
	TestMissingControlCreatesIssueAndDefaultControl();
	TestOrphanControlCreatesIssueOnly();
	TestIssueOrderIsMissingThenOrphan();
	TestNamespacedAndUnqualifiedIdsAreDistinct();
	TestProjectionDoesNotMutateInputs();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
