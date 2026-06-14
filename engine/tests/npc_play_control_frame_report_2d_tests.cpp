#include <cstdlib>
#include <vector>

#include "scene/npc/NpcPlayControlFrameReport2D.hpp"
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
	iggy::NpcMoveMode moveMode = iggy::NpcMoveMode::Walk,
	const char *actionTag = "action:proposal")
{
	iggy::NpcPlayControlProposal proposal;
	proposal.status = iggy::NpcPlayControlProposalStatus::Proposed;
	proposal.actionTag = Id(actionTag);
	proposal.requestedBehaviorState = behavior.type;
	proposal.objective = objective;
	proposal.behavior = behavior;
	proposal.moveMode = moveMode;
	return proposal;
}

iggy::NpcPlayControlProposal NoProposal()
{
	iggy::NpcPlayControlProposal proposal;
	proposal.status = iggy::NpcPlayControlProposalStatus::NoKeptPlay;
	return proposal;
}

iggy::NpcPlayControlFrameProposal2D Request(const char *npcId, const iggy::NpcPlayControlProposal &proposal)
{
	return {
		Id(npcId),
		proposal,
	};
}

iggy::NpcActorControlState2DRegistry Registry(std::vector<iggy::NpcActorControlState2D> entries)
{
	return { entries };
}

iggy::NpcPlayControlFrameApply2DResult Apply(
	const iggy::NpcActorControlState2DRegistry &registry,
	const std::vector<iggy::NpcPlayControlFrameProposal2D> &requests)
{
	return iggy::NpcPlayControlFrameApplier2D {}.apply(registry, requests);
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

bool SameRequest(const iggy::NpcPlayControlFrameProposal2D &actual, const iggy::NpcPlayControlFrameProposal2D &expected)
{
	return actual.npcId == expected.npcId
		&& SameProposal(actual.proposal, expected.proposal);
}

bool SameValidation(const iggy::NpcObjectiveValidationResult &actual, const iggy::NpcObjectiveValidationResult &expected)
{
	return actual.status == expected.status
		&& SameObjective(actual.objective, expected.objective);
}

bool SameValidation(const iggy::NpcBehaviorStateValidationResult &actual, const iggy::NpcBehaviorStateValidationResult &expected)
{
	return actual.status == expected.status
		&& SameBehavior(actual.state, expected.state);
}

bool SameSingleApply(const iggy::NpcPlayControlApply2DResult &actual, const iggy::NpcPlayControlApply2DResult &expected)
{
	return SameControls(actual.registry.entries, expected.registry.entries)
		&& SameProposal(actual.proposal, expected.proposal)
		&& actual.status == expected.status
		&& actual.changed == expected.changed
		&& actual.appended == expected.appended
		&& actual.controlIndex == expected.controlIndex
		&& SameValidation(actual.objectiveValidation, expected.objectiveValidation)
		&& SameValidation(actual.behaviorValidation, expected.behaviorValidation);
}

bool SameEntry(const iggy::NpcPlayControlFrameApplyEntry2D &actual, const iggy::NpcPlayControlFrameApplyEntry2D &expected)
{
	return actual.proposalIndex == expected.proposalIndex
		&& SameRequest(actual.request, expected.request)
		&& SameSingleApply(actual.apply, expected.apply);
}

bool SameApply(const iggy::NpcPlayControlFrameApply2DResult &actual, const iggy::NpcPlayControlFrameApply2DResult &expected)
{
	if (!SameControls(actual.registry.entries, expected.registry.entries)
		|| actual.entries.size() != expected.entries.size()
		|| actual.status != expected.status
		|| actual.appliedCount != expected.appliedCount
		|| actual.failedCount != expected.failedCount
		|| actual.changed != expected.changed)
		return false;

	for (std::size_t index = 0; index < actual.entries.size(); ++index) {
		if (!SameEntry(actual.entries[index], expected.entries[index]))
			return false;
	}
	return true;
}

bool SameEvents(
	const std::vector<iggy::NpcPlayControlFrameEvent2D> &actual,
	const std::vector<iggy::NpcPlayControlFrameEvent2D> &expected)
{
	return actual == expected;
}

void TestEmptyNoChangeReportsNoControlChanged()
{
	const iggy::NpcActorControlState2DRegistry registry = Registry({
		Control("npc:guard"),
	});
	const iggy::NpcPlayControlFrameApply2DResult apply = Apply(registry, {});

	const iggy::NpcPlayControlFrameReport2D report =
		iggy::NpcPlayControlFrameReporter2D {}.report(apply);

	Expect(report.proposalCount == 0, "empty report should count zero proposals");
	Expect(report.appliedCount == 0, "empty report should count zero applied");
	Expect(report.failedCount == 0, "empty report should count zero failed");
	Expect(report.appendedCount == 0, "empty report should count zero appended");
	Expect(report.updatedCount == 0, "empty report should count zero updated");
	Expect(report.duplicateNpcProposalCount == 0, "empty report should count zero duplicate proposals");
	Expect(!report.changed, "empty report should not report changed");
	Expect(report.hasEvents(), "empty report should still emit final changed fact");
	Expect(SameEvents(report.events, { iggy::NpcPlayControlFrameEvent2D::NoControlChanged }), "empty report event order should contain NoControlChanged only");
	Expect(SameApply(report.apply, apply), "empty report should copy full apply result");
	Expect(SameControls(report.registry.entries, apply.registry.entries), "empty report should copy returned registry");
}

void TestAllSuccessCountsAppliedAppendedUpdatedAndChanged()
{
	const iggy::NpcActorControlState2D existing = Control("npc:guard");
	const iggy::NpcActorControlState2DRegistry registry = Registry({ existing });
	const iggy::NpcPlayControlFrameProposal2D update = Request(
		"npc:guard",
		Proposal(iggy::attackNpcObjective(Id("target:enemy")), iggy::attackingNpcBehaviorState(Id("target:enemy")), iggy::NpcMoveMode::Run));
	const iggy::NpcPlayControlFrameProposal2D append = Request("npc:new", Proposal());
	const iggy::NpcPlayControlFrameApply2DResult apply = Apply(registry, { update, append });

	const iggy::NpcPlayControlFrameReport2D report =
		iggy::NpcPlayControlFrameReporter2D {}.report(apply);

	Expect(report.proposalCount == 2, "all-success report should count proposals");
	Expect(report.appliedCount == 2, "all-success report should count applied proposals");
	Expect(report.failedCount == 0, "all-success report should count zero failures");
	Expect(report.appendedCount == 1, "all-success report should count appended proposals");
	Expect(report.updatedCount == 1, "all-success report should count updated proposals");
	Expect(report.changed, "all-success report should copy changed flag");
	Expect(SameEvents(report.events, {
		iggy::NpcPlayControlFrameEvent2D::ProposalApplied,
		iggy::NpcPlayControlFrameEvent2D::ProposalApplied,
		iggy::NpcPlayControlFrameEvent2D::ProposalUpdated,
		iggy::NpcPlayControlFrameEvent2D::ProposalAppended,
		iggy::NpcPlayControlFrameEvent2D::ControlChanged,
	}), "all-success report should emit deterministic event order");
	Expect(SameApply(report.apply, apply), "all-success report should preserve apply payload");
	Expect(SameControls(report.registry.entries, apply.registry.entries), "all-success report should preserve registry");
}

void TestMixedFailuresAreGroupedByNestedStatus()
{
	const iggy::NpcActorControlState2DRegistry registry = Registry({});
	const iggy::NpcPlayControlFrameProposal2D applied = Request("npc:ok", Proposal());
	const iggy::NpcPlayControlFrameProposal2D noProposal = Request("npc:no-proposal", NoProposal());
	const iggy::NpcPlayControlFrameProposal2D missingNpcId = { {}, Proposal() };
	const iggy::NpcPlayControlFrameProposal2D invalidObjective = Request(
		"npc:bad-objective",
		Proposal(iggy::attackNpcObjective({}), iggy::idleNpcBehaviorState(), iggy::NpcMoveMode::Still));
	const iggy::NpcPlayControlFrameProposal2D invalidBehavior = Request(
		"npc:bad-behavior",
		Proposal(iggy::waitNpcObjective(), iggy::attackingNpcBehaviorState({}), iggy::NpcMoveMode::Still));
	const iggy::NpcPlayControlFrameApply2DResult apply = Apply(
		registry,
		{ applied, noProposal, missingNpcId, invalidObjective, invalidBehavior });

	const iggy::NpcPlayControlFrameReport2D report =
		iggy::NpcPlayControlFrameReporter2D {}.report(apply);

	Expect(report.proposalCount == 5, "mixed report should count all proposals");
	Expect(report.appliedCount == 1, "mixed report should count applied proposals");
	Expect(report.failedCount == 4, "mixed report should count failed proposals");
	Expect(report.noProposalCount == 1, "mixed report should count NoProposal failures");
	Expect(report.missingNpcIdCount == 1, "mixed report should count MissingNpcId failures");
	Expect(report.invalidObjectiveCount == 1, "mixed report should count InvalidObjective failures");
	Expect(report.invalidBehaviorCount == 1, "mixed report should count InvalidBehavior failures");
	Expect(report.appendedCount == 1, "mixed report should count successful append");
	Expect(report.updatedCount == 0, "mixed report should count no updates");
	Expect(SameEvents(report.events, {
		iggy::NpcPlayControlFrameEvent2D::ProposalApplied,
		iggy::NpcPlayControlFrameEvent2D::ProposalFailed,
		iggy::NpcPlayControlFrameEvent2D::ProposalFailed,
		iggy::NpcPlayControlFrameEvent2D::ProposalFailed,
		iggy::NpcPlayControlFrameEvent2D::ProposalFailed,
		iggy::NpcPlayControlFrameEvent2D::ProposalAppended,
		iggy::NpcPlayControlFrameEvent2D::ControlChanged,
	}), "mixed report should order proposal/fact/final events deterministically");
}

void TestDuplicateNpcProposalAttemptsAreCountedInEntryOrder()
{
	const iggy::NpcActorControlState2DRegistry registry = Registry({});
	const iggy::NpcPlayControlFrameProposal2D first = Request("npc:guard", Proposal());
	const iggy::NpcPlayControlFrameProposal2D duplicate = Request("npc:guard", NoProposal());
	const iggy::NpcPlayControlFrameProposal2D emptyFirst = { {}, Proposal() };
	const iggy::NpcPlayControlFrameProposal2D emptySecond = { {}, NoProposal() };
	const iggy::NpcPlayControlFrameProposal2D unqualified = Request("guard", Proposal());
	const iggy::NpcPlayControlFrameProposal2D duplicateAgain = Request("npc:guard", Proposal());
	const iggy::NpcPlayControlFrameApply2DResult apply = Apply(
		registry,
		{ first, duplicate, emptyFirst, emptySecond, unqualified, duplicateAgain });

	const iggy::NpcPlayControlFrameReport2D report =
		iggy::NpcPlayControlFrameReporter2D {}.report(apply);

	Expect(report.duplicateNpcProposalCount == 2, "duplicate report should count later non-empty exact duplicate ids");
	Expect(SameEvents(report.events, {
		iggy::NpcPlayControlFrameEvent2D::ProposalApplied,
		iggy::NpcPlayControlFrameEvent2D::ProposalFailed,
		iggy::NpcPlayControlFrameEvent2D::ProposalFailed,
		iggy::NpcPlayControlFrameEvent2D::ProposalFailed,
		iggy::NpcPlayControlFrameEvent2D::ProposalApplied,
		iggy::NpcPlayControlFrameEvent2D::ProposalApplied,
		iggy::NpcPlayControlFrameEvent2D::ProposalAppended,
		iggy::NpcPlayControlFrameEvent2D::ProposalAppended,
		iggy::NpcPlayControlFrameEvent2D::ProposalUpdated,
		iggy::NpcPlayControlFrameEvent2D::DuplicateNpcProposalObserved,
		iggy::NpcPlayControlFrameEvent2D::DuplicateNpcProposalObserved,
		iggy::NpcPlayControlFrameEvent2D::ControlChanged,
	}), "duplicate report should place duplicate observations after append/update facts");
}

void TestNamespacedAndUnqualifiedNpcIdsAreDistinctForDuplicates()
{
	const iggy::NpcActorControlState2DRegistry registry = Registry({});
	const iggy::NpcPlayControlFrameApply2DResult apply = Apply(registry, {
		Request("guard", Proposal()),
		Request("npc:guard", Proposal()),
		Request("guard", Proposal()),
	});

	const iggy::NpcPlayControlFrameReport2D report =
		iggy::NpcPlayControlFrameReporter2D {}.report(apply);

	Expect(report.duplicateNpcProposalCount == 1, "namespaced report should count only exact duplicate ids");
	Expect(report.appliedCount == 3, "namespaced report should still count all applied proposals");
}

void TestAllFailureReportHasNoControlChanged()
{
	const iggy::NpcActorControlState2D existing = Control("npc:existing");
	const iggy::NpcActorControlState2DRegistry registry = Registry({ existing });
	const iggy::NpcPlayControlFrameApply2DResult apply = Apply(registry, {
		Request("npc:first", NoProposal()),
		{ {}, Proposal() },
	});

	const iggy::NpcPlayControlFrameReport2D report =
		iggy::NpcPlayControlFrameReporter2D {}.report(apply);

	Expect(report.proposalCount == 2, "all-failure report should count proposals");
	Expect(report.appliedCount == 0, "all-failure report should count zero applied");
	Expect(report.failedCount == 2, "all-failure report should count failures");
	Expect(report.appendedCount == 0, "all-failure report should count zero appended");
	Expect(report.updatedCount == 0, "all-failure report should count zero updated");
	Expect(!report.changed, "all-failure report should copy unchanged flag");
	Expect(SameEvents(report.events, {
		iggy::NpcPlayControlFrameEvent2D::ProposalFailed,
		iggy::NpcPlayControlFrameEvent2D::ProposalFailed,
		iggy::NpcPlayControlFrameEvent2D::NoControlChanged,
	}), "all-failure report should emit failure events then NoControlChanged");
	Expect(SameControls(report.registry.entries, registry.entries), "all-failure report should preserve original registry");
}

void TestReportDoesNotMutateInput()
{
	iggy::NpcPlayControlFrameApply2DResult apply = Apply(Registry({ Control("npc:guard") }), {
		Request("npc:guard", Proposal(iggy::waitNpcObjective(), iggy::waitingNpcBehaviorState(), iggy::NpcMoveMode::Still)),
		Request("npc:new", Proposal()),
	});
	const iggy::NpcPlayControlFrameApply2DResult before = apply;

	const iggy::NpcPlayControlFrameReport2D report =
		iggy::NpcPlayControlFrameReporter2D {}.report(apply);

	Expect(report.changed, "immutability setup should report changed");
	Expect(SameApply(apply, before), "report should not mutate input apply result");
	Expect(SameApply(report.apply, before), "report should copy apply result by value");
}

} // namespace

int main()
{
	TestEmptyNoChangeReportsNoControlChanged();
	TestAllSuccessCountsAppliedAppendedUpdatedAndChanged();
	TestMixedFailuresAreGroupedByNestedStatus();
	TestDuplicateNpcProposalAttemptsAreCountedInEntryOrder();
	TestNamespacedAndUnqualifiedNpcIdsAreDistinctForDuplicates();
	TestAllFailureReportHasNoControlChanged();
	TestReportDoesNotMutateInput();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
