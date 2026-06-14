#include <cstdlib>
#include <vector>

#include "scene/npc/NpcPlayControlApply2D.hpp"
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

iggy::NpcPlayControlProposal Proposal(
	iggy::NpcObjective objective = iggy::moveToNpcObjective({ 3.0F, 4.0F }),
	iggy::NpcBehaviorState behavior = iggy::seekingNpcBehaviorState({ 3.0F, 4.0F }),
	iggy::NpcMoveMode moveMode = iggy::NpcMoveMode::Walk)
{
	iggy::NpcPlayControlProposal proposal;
	proposal.status = iggy::NpcPlayControlProposalStatus::Proposed;
	proposal.actionTag = Id("action:proposal");
	proposal.requestedBehaviorState = behavior.type;
	proposal.objective = objective;
	proposal.behavior = behavior;
	proposal.moveMode = moveMode;
	return proposal;
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

bool SameProposal(const iggy::NpcPlayControlProposal &actual, const iggy::NpcPlayControlProposal &expected)
{
	return actual.status == expected.status
		&& actual.actionTag == expected.actionTag
		&& actual.requestedBehaviorState == expected.requestedBehaviorState
		&& SameObjective(actual.objective, expected.objective)
		&& SameBehavior(actual.behavior, expected.behavior)
		&& actual.moveMode == expected.moveMode;
}

iggy::NpcActorControlState2DRegistry Registry(std::vector<iggy::NpcActorControlState2D> entries)
{
	return { entries };
}

void ExpectUnchangedFailure(
	const iggy::NpcPlayControlApply2DResult &result,
	const iggy::NpcActorControlState2DRegistry &registry,
	iggy::NpcPlayControlApplyStatus status,
	const char *message)
{
	Expect(result.status == status, message);
	Expect(!result.applied(), "failed apply should not report applied");
	Expect(!result.changed, "failed apply should not report changed");
	Expect(!result.appended, "failed apply should not report appended");
	Expect(SameControls(result.registry.entries, registry.entries), "failed apply should preserve registry");
}

void TestNoProposalPreservesRegistry()
{
	const iggy::NpcActorControlState2DRegistry registry = Registry({
		Control("npc:guard"),
	});
	iggy::NpcPlayControlProposal proposal;
	proposal.status = iggy::NpcPlayControlProposalStatus::NoKeptPlay;

	const iggy::NpcPlayControlApply2DResult result =
		iggy::NpcPlayControlApplier2D {}.apply(registry, Id("npc:guard"), proposal);

	ExpectUnchangedFailure(result, registry, iggy::NpcPlayControlApplyStatus::NoProposal, "no proposal should return NoProposal");
	Expect(SameProposal(result.proposal, proposal), "NoProposal should preserve proposal");
}

void TestMissingNpcIdPreservesRegistry()
{
	const iggy::NpcActorControlState2DRegistry registry = Registry({
		Control("npc:guard"),
	});
	const iggy::NpcPlayControlProposal proposal = Proposal();

	const iggy::NpcPlayControlApply2DResult result =
		iggy::NpcPlayControlApplier2D {}.apply(registry, {}, proposal);

	ExpectUnchangedFailure(result, registry, iggy::NpcPlayControlApplyStatus::MissingNpcId, "missing npc id should return MissingNpcId");
	Expect(SameProposal(result.proposal, proposal), "MissingNpcId should preserve proposal");
}

void TestExistingControlIsReplacedInPlace()
{
	const iggy::NpcActorControlState2D first = Control("npc:first", iggy::waitNpcObjective(), iggy::idleNpcBehaviorState(), iggy::NpcMoveMode::Still);
	const iggy::NpcActorControlState2D second = Control("npc:second", iggy::guardNpcObjective(Id("anchor:old")), iggy::waitingNpcBehaviorState(), iggy::NpcMoveMode::Still);
	const iggy::NpcActorControlState2D third = Control("npc:third", iggy::noneNpcObjective(), iggy::disabledNpcBehaviorState(), iggy::NpcMoveMode::None);
	const iggy::NpcActorControlState2DRegistry registry = Registry({ first, second, third });
	const iggy::NpcPlayControlProposal proposal = Proposal(
		iggy::attackNpcObjective(Id("target:enemy")),
		iggy::attackingNpcBehaviorState(Id("target:enemy")),
		iggy::NpcMoveMode::Run);

	const iggy::NpcPlayControlApply2DResult result =
		iggy::NpcPlayControlApplier2D {}.apply(registry, Id("npc:second"), proposal);

	Expect(result.status == iggy::NpcPlayControlApplyStatus::Applied, "existing control apply should succeed");
	Expect(result.applied(), "existing control apply should report applied");
	Expect(result.changed, "existing control apply should report changed");
	Expect(!result.appended, "existing control apply should not append");
	Expect(result.controlIndex == 1, "existing control apply should preserve target index");
	Expect(result.registry.entries.size() == 3, "existing control apply should preserve entry count");
	if (result.registry.entries.size() == 3) {
		Expect(SameControl(result.registry.entries[0], first), "existing control apply should preserve earlier entry");
		Expect(result.registry.entries[1].npcId == Id("npc:second"), "existing control apply should preserve exact existing npc id");
		Expect(SameObjective(result.registry.entries[1].objective, proposal.objective), "existing control apply should replace objective");
		Expect(SameBehavior(result.registry.entries[1].behavior, proposal.behavior), "existing control apply should replace behavior");
		Expect(result.registry.entries[1].moveMode == proposal.moveMode, "existing control apply should replace move mode");
		Expect(SameControl(result.registry.entries[2], third), "existing control apply should preserve later entry");
	}
}

void TestExactNpcIdSemanticsPreserveNamespacedAndUnqualifiedIds()
{
	const iggy::NpcActorControlState2D unqualified = Control("guard", iggy::waitNpcObjective(), iggy::idleNpcBehaviorState(), iggy::NpcMoveMode::Still);
	const iggy::NpcActorControlState2D namespaced = Control("npc:guard", iggy::guardNpcObjective(Id("anchor:old")), iggy::waitingNpcBehaviorState(), iggy::NpcMoveMode::Still);
	const iggy::NpcActorControlState2DRegistry registry = Registry({ unqualified, namespaced });
	const iggy::NpcPlayControlProposal proposal = Proposal(
		iggy::moveToNpcObjective({ 8.0F, 9.0F }),
		iggy::seekingNpcBehaviorState({ 8.0F, 9.0F }),
		iggy::NpcMoveMode::Walk);

	const iggy::NpcPlayControlApply2DResult result =
		iggy::NpcPlayControlApplier2D {}.apply(registry, Id("npc:guard"), proposal);

	Expect(result.applied(), "namespaced control apply should succeed");
	Expect(result.controlIndex == 1, "namespaced control apply should update exact matching entry");
	Expect(result.registry.entries.size() == 2, "namespaced control apply should not append");
	if (result.registry.entries.size() == 2) {
		Expect(SameControl(result.registry.entries[0], unqualified), "namespaced control apply should leave unqualified id untouched");
		Expect(result.registry.entries[1].npcId == Id("npc:guard"), "namespaced control apply should preserve namespaced id");
		Expect(SameObjective(result.registry.entries[1].objective, proposal.objective), "namespaced control apply should update namespaced objective");
	}
}

void TestMissingControlAppendsAtEnd()
{
	const iggy::NpcActorControlState2D first = Control("npc:first");
	const iggy::NpcActorControlState2DRegistry registry = Registry({ first });
	const iggy::NpcPlayControlProposal proposal = Proposal(
		iggy::moveToNpcObjective({ 5.0F, 6.0F }),
		iggy::seekingNpcBehaviorState({ 5.0F, 6.0F }),
		iggy::NpcMoveMode::Jog);

	const iggy::NpcPlayControlApply2DResult result =
		iggy::NpcPlayControlApplier2D {}.apply(registry, Id("npc:new"), proposal);

	Expect(result.status == iggy::NpcPlayControlApplyStatus::Applied, "missing control apply should succeed");
	Expect(result.changed, "missing control apply should report changed");
	Expect(result.appended, "missing control apply should report appended");
	Expect(result.controlIndex == 1, "missing control apply should report appended index");
	Expect(result.registry.entries.size() == 2, "missing control apply should append one entry");
	if (result.registry.entries.size() == 2) {
		Expect(SameControl(result.registry.entries[0], first), "missing control apply should preserve existing entry");
		Expect(result.registry.entries[1].npcId == Id("npc:new"), "missing control apply should set appended npc id");
		Expect(SameObjective(result.registry.entries[1].objective, proposal.objective), "missing control apply should copy proposal objective");
		Expect(SameBehavior(result.registry.entries[1].behavior, proposal.behavior), "missing control apply should copy proposal behavior");
		Expect(result.registry.entries[1].moveMode == proposal.moveMode, "missing control apply should copy proposal move mode");
	}
}

void TestInvalidObjectivePreservesDiagnosticsAndRegistry()
{
	const iggy::NpcActorControlState2DRegistry registry = Registry({
		Control("npc:guard"),
	});
	const iggy::NpcPlayControlProposal proposal = Proposal(
		iggy::attackNpcObjective({}),
		iggy::idleNpcBehaviorState(),
		iggy::NpcMoveMode::Still);

	const iggy::NpcPlayControlApply2DResult result =
		iggy::NpcPlayControlApplier2D {}.apply(registry, Id("npc:guard"), proposal);

	ExpectUnchangedFailure(result, registry, iggy::NpcPlayControlApplyStatus::InvalidObjective, "invalid objective should fail apply");
	Expect(result.objectiveValidation.status == iggy::NpcObjectiveStatus::MissingTarget, "invalid objective apply should preserve nested status");
	Expect(SameObjective(result.objectiveValidation.objective, proposal.objective), "invalid objective apply should preserve nested objective payload");
	Expect(SameProposal(result.proposal, proposal), "invalid objective apply should preserve proposal");
}

void TestInvalidBehaviorPreservesDiagnosticsAndRegistry()
{
	const iggy::NpcActorControlState2DRegistry registry = Registry({
		Control("npc:guard"),
	});
	const iggy::NpcPlayControlProposal proposal = Proposal(
		iggy::waitNpcObjective(),
		iggy::attackingNpcBehaviorState({}),
		iggy::NpcMoveMode::Still);

	const iggy::NpcPlayControlApply2DResult result =
		iggy::NpcPlayControlApplier2D {}.apply(registry, Id("npc:guard"), proposal);

	ExpectUnchangedFailure(result, registry, iggy::NpcPlayControlApplyStatus::InvalidBehavior, "invalid behavior should fail apply");
	Expect(result.objectiveValidation.ok(), "invalid behavior apply should preserve valid objective diagnostics");
	Expect(result.behaviorValidation.status == iggy::NpcBehaviorStateStatus::MissingTarget, "invalid behavior apply should preserve nested status");
	Expect(SameBehavior(result.behaviorValidation.state, proposal.behavior), "invalid behavior apply should preserve nested behavior payload");
	Expect(SameProposal(result.proposal, proposal), "invalid behavior apply should preserve proposal");
}

void TestAppliedResultPreservesProposalAndValidationFacts()
{
	const iggy::NpcActorControlState2DRegistry registry = Registry({});
	const iggy::NpcPlayControlProposal proposal = Proposal(
		iggy::interactNpcObjective(Id("target:lever")),
		iggy::interactingNpcBehaviorState(Id("target:lever")),
		iggy::NpcMoveMode::Walk);

	const iggy::NpcPlayControlApply2DResult result =
		iggy::NpcPlayControlApplier2D {}.apply(registry, Id("npc:actor"), proposal);

	Expect(result.applied(), "valid apply should report applied");
	Expect(result.objectiveValidation.ok(), "valid apply should preserve valid objective diagnostics");
	Expect(result.behaviorValidation.ok(), "valid apply should preserve valid behavior diagnostics");
	Expect(SameObjective(result.objectiveValidation.objective, proposal.objective), "valid apply should preserve objective validation payload");
	Expect(SameBehavior(result.behaviorValidation.state, proposal.behavior), "valid apply should preserve behavior validation payload");
	Expect(SameProposal(result.proposal, proposal), "valid apply should preserve proposal");
}

void TestApplyDoesNotMutateInputs()
{
	iggy::NpcActorControlState2DRegistry registry = Registry({
		Control("npc:guard", iggy::waitNpcObjective(), iggy::idleNpcBehaviorState(), iggy::NpcMoveMode::Still),
	});
	iggy::NpcPlayControlProposal proposal = Proposal(
		iggy::moveToNpcObjective({ 2.0F, 3.0F }),
		iggy::seekingNpcBehaviorState({ 2.0F, 3.0F }),
		iggy::NpcMoveMode::Run);
	const iggy::NpcActorControlState2DRegistry registryBefore = registry;
	const iggy::NpcPlayControlProposal proposalBefore = proposal;

	const iggy::NpcPlayControlApply2DResult result =
		iggy::NpcPlayControlApplier2D {}.apply(registry, Id("npc:guard"), proposal);

	Expect(result.applied(), "immutability setup should apply");
	Expect(SameControls(registry.entries, registryBefore.entries), "apply should not mutate input registry");
	Expect(SameProposal(proposal, proposalBefore), "apply should not mutate input proposal");
}

} // namespace

int main()
{
	TestNoProposalPreservesRegistry();
	TestMissingNpcIdPreservesRegistry();
	TestExistingControlIsReplacedInPlace();
	TestExactNpcIdSemanticsPreserveNamespacedAndUnqualifiedIds();
	TestMissingControlAppendsAtEnd();
	TestInvalidObjectivePreservesDiagnosticsAndRegistry();
	TestInvalidBehaviorPreservesDiagnosticsAndRegistry();
	TestAppliedResultPreservesProposalAndValidationFacts();
	TestApplyDoesNotMutateInputs();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
