#include <cstdlib>
#include <vector>

#include "scene/npc/NpcPlayControlFrameApply2D.hpp"
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

void ExpectEntry(
	const iggy::NpcPlayControlFrameApplyEntry2D &entry,
	std::size_t proposalIndex,
	const iggy::NpcPlayControlFrameProposal2D &request,
	iggy::NpcPlayControlApplyStatus status,
	const char *message)
{
	Expect(entry.proposalIndex == proposalIndex, message);
	Expect(SameRequest(entry.request, request), message);
	Expect(entry.apply.status == status, message);
	Expect(SameProposal(entry.apply.proposal, request.proposal), message);
}

void TestEmptyProposalListIsNoOp()
{
	const iggy::NpcActorControlState2DRegistry registry = Registry({
		Control("npc:guard"),
	});

	const iggy::NpcPlayControlFrameApply2DResult result =
		iggy::NpcPlayControlFrameApplier2D {}.apply(registry, {});

	Expect(result.status == iggy::NpcPlayControlFrameApplyStatus2D::NoProposalsApplied, "empty frame apply should report no proposals applied");
	Expect(!result.applied(), "empty frame apply should not report applied");
	Expect(!result.changed, "empty frame apply should not report changed");
	Expect(result.appliedCount == 0, "empty frame apply should have zero applied count");
	Expect(result.failedCount == 0, "empty frame apply should have zero failed count");
	Expect(result.entries.empty(), "empty frame apply should have no entries");
	Expect(SameControls(result.registry.entries, registry.entries), "empty frame apply should preserve registry");
}

void TestSingleValidProposalAppends()
{
	const iggy::NpcActorControlState2DRegistry registry = Registry({});
	const iggy::NpcPlayControlFrameProposal2D request = Request("npc:new", Proposal());

	const iggy::NpcPlayControlFrameApply2DResult result =
		iggy::NpcPlayControlFrameApplier2D {}.apply(registry, { request });

	Expect(result.status == iggy::NpcPlayControlFrameApplyStatus2D::Applied, "single valid frame apply should report Applied");
	Expect(result.applied(), "single valid frame apply should report applied helper");
	Expect(result.changed, "single valid frame apply should report changed");
	Expect(result.appliedCount == 1, "single valid frame apply should count one applied request");
	Expect(result.failedCount == 0, "single valid frame apply should count zero failed requests");
	Expect(result.entries.size() == 1, "single valid frame apply should record one entry");
	if (result.entries.size() == 1) {
		ExpectEntry(result.entries[0], 0, request, iggy::NpcPlayControlApplyStatus::Applied, "single valid entry should preserve request and nested apply");
		Expect(result.entries[0].apply.appended, "single valid nested apply should append");
		Expect(result.entries[0].apply.controlIndex == 0, "single valid nested apply should report appended index");
	}
	Expect(result.registry.entries.size() == 1, "single valid frame apply should append one control");
	if (result.registry.entries.size() == 1) {
		Expect(result.registry.entries[0].npcId == Id("npc:new"), "single valid frame apply should use request npc id");
		Expect(SameObjective(result.registry.entries[0].objective, request.proposal.objective), "single valid frame apply should copy objective");
	}
}

void TestSingleValidProposalUpdates()
{
	const iggy::NpcActorControlState2D existing = Control("npc:guard");
	const iggy::NpcActorControlState2DRegistry registry = Registry({ existing });
	const iggy::NpcPlayControlFrameProposal2D request = Request(
		"npc:guard",
		Proposal(iggy::attackNpcObjective(Id("target:enemy")), iggy::attackingNpcBehaviorState(Id("target:enemy")), iggy::NpcMoveMode::Run));

	const iggy::NpcPlayControlFrameApply2DResult result =
		iggy::NpcPlayControlFrameApplier2D {}.apply(registry, { request });

	Expect(result.applied(), "single update frame apply should report applied");
	Expect(result.entries.size() == 1, "single update frame apply should record one entry");
	if (result.entries.size() == 1) {
		Expect(!result.entries[0].apply.appended, "single update nested apply should not append");
		Expect(result.entries[0].apply.controlIndex == 0, "single update nested apply should report existing index");
	}
	Expect(result.registry.entries.size() == 1, "single update frame apply should preserve entry count");
	if (result.registry.entries.size() == 1) {
		Expect(result.registry.entries[0].npcId == Id("npc:guard"), "single update should preserve existing npc id");
		Expect(SameBehavior(result.registry.entries[0].behavior, request.proposal.behavior), "single update should replace behavior");
	}
}

void TestMixedValidAndInvalidProposalsContinueAfterFailures()
{
	const iggy::NpcActorControlState2DRegistry registry = Registry({});
	const iggy::NpcPlayControlFrameProposal2D first = Request("npc:first", Proposal());
	const iggy::NpcPlayControlFrameProposal2D invalid = Request("npc:invalid", NoProposal());
	const iggy::NpcPlayControlFrameProposal2D second = Request(
		"npc:second",
		Proposal(iggy::waitNpcObjective(), iggy::waitingNpcBehaviorState(), iggy::NpcMoveMode::Still, "action:wait"));

	const iggy::NpcPlayControlFrameApply2DResult result =
		iggy::NpcPlayControlFrameApplier2D {}.apply(registry, { first, invalid, second });

	Expect(result.status == iggy::NpcPlayControlFrameApplyStatus2D::Applied, "mixed frame apply should report Applied when any request applies");
	Expect(result.changed, "mixed frame apply should report changed when any request applies");
	Expect(result.appliedCount == 2, "mixed frame apply should count applied requests");
	Expect(result.failedCount == 1, "mixed frame apply should count failed requests");
	Expect(result.entries.size() == 3, "mixed frame apply should preserve one entry per attempt");
	if (result.entries.size() == 3) {
		ExpectEntry(result.entries[0], 0, first, iggy::NpcPlayControlApplyStatus::Applied, "mixed first entry should apply");
		ExpectEntry(result.entries[1], 1, invalid, iggy::NpcPlayControlApplyStatus::NoProposal, "mixed invalid entry should preserve diagnostics");
		ExpectEntry(result.entries[2], 2, second, iggy::NpcPlayControlApplyStatus::Applied, "mixed later valid entry should still apply");
		Expect(SameControls(result.entries[1].apply.registry.entries, result.entries[0].apply.registry.entries), "failed mixed entry should preserve carried registry");
	}
	Expect(result.registry.entries.size() == 2, "mixed frame apply should carry forward valid controls only");
	if (result.registry.entries.size() == 2) {
		Expect(result.registry.entries[0].npcId == Id("npc:first"), "mixed frame apply should preserve first valid append");
		Expect(result.registry.entries[1].npcId == Id("npc:second"), "mixed frame apply should continue after failed request");
	}
}

void TestDuplicateNpcProposalsApplySequentiallyAndLaterWins()
{
	const iggy::NpcActorControlState2DRegistry registry = Registry({});
	const iggy::NpcPlayControlFrameProposal2D first = Request(
		"npc:guard",
		Proposal(iggy::moveToNpcObjective({ 1.0F, 2.0F }), iggy::seekingNpcBehaviorState({ 1.0F, 2.0F }), iggy::NpcMoveMode::Walk, "action:first"));
	const iggy::NpcPlayControlFrameProposal2D second = Request(
		"npc:guard",
		Proposal(iggy::attackNpcObjective(Id("target:enemy")), iggy::attackingNpcBehaviorState(Id("target:enemy")), iggy::NpcMoveMode::Run, "action:second"));

	const iggy::NpcPlayControlFrameApply2DResult result =
		iggy::NpcPlayControlFrameApplier2D {}.apply(registry, { first, second });

	Expect(result.appliedCount == 2, "duplicate frame apply should count both successful requests");
	Expect(result.failedCount == 0, "duplicate frame apply should have no failures");
	Expect(result.registry.entries.size() == 1, "duplicate frame apply should keep one final control");
	if (result.entries.size() == 2) {
		Expect(result.entries[0].apply.appended, "first duplicate should append");
		Expect(result.entries[0].apply.controlIndex == 0, "first duplicate should target index zero");
		Expect(!result.entries[1].apply.appended, "second duplicate should update existing control");
		Expect(result.entries[1].apply.controlIndex == 0, "second duplicate should target same index");
	}
	if (result.registry.entries.size() == 1) {
		Expect(result.registry.entries[0].npcId == Id("npc:guard"), "duplicate final control should preserve npc id");
		Expect(SameObjective(result.registry.entries[0].objective, second.proposal.objective), "duplicate later successful objective should win");
		Expect(SameBehavior(result.registry.entries[0].behavior, second.proposal.behavior), "duplicate later successful behavior should win");
		Expect(result.registry.entries[0].moveMode == second.proposal.moveMode, "duplicate later successful move mode should win");
	}
}

void TestFailedLaterDuplicateDoesNotEraseEarlierSuccess()
{
	const iggy::NpcActorControlState2DRegistry registry = Registry({});
	const iggy::NpcPlayControlFrameProposal2D first = Request("npc:guard", Proposal());
	const iggy::NpcPlayControlFrameProposal2D failedDuplicate = Request("npc:guard", NoProposal());

	const iggy::NpcPlayControlFrameApply2DResult result =
		iggy::NpcPlayControlFrameApplier2D {}.apply(registry, { first, failedDuplicate });

	Expect(result.appliedCount == 1, "failed duplicate frame apply should count earlier success");
	Expect(result.failedCount == 1, "failed duplicate frame apply should count later failure");
	Expect(result.changed, "failed duplicate frame apply should remain changed due to earlier success");
	Expect(result.entries.size() == 2, "failed duplicate frame apply should preserve both entries");
	if (result.entries.size() == 2) {
		ExpectEntry(result.entries[1], 1, failedDuplicate, iggy::NpcPlayControlApplyStatus::NoProposal, "failed duplicate entry should preserve failure");
		Expect(SameControls(result.entries[1].apply.registry.entries, result.entries[0].apply.registry.entries), "failed duplicate should preserve carried registry");
	}
	Expect(result.registry.entries.size() == 1, "failed duplicate frame apply should keep earlier control");
	if (result.registry.entries.size() == 1) {
		Expect(SameObjective(result.registry.entries[0].objective, first.proposal.objective), "failed duplicate should not erase earlier objective");
		Expect(SameBehavior(result.registry.entries[0].behavior, first.proposal.behavior), "failed duplicate should not erase earlier behavior");
	}
}

void TestDifferentNpcIdsPreserveRegistryOrderSemantics()
{
	const iggy::NpcActorControlState2D first = Control("npc:first", iggy::waitNpcObjective(), iggy::idleNpcBehaviorState(), iggy::NpcMoveMode::Still);
	const iggy::NpcActorControlState2D second = Control("npc:second", iggy::guardNpcObjective(Id("anchor:old")), iggy::waitingNpcBehaviorState(), iggy::NpcMoveMode::Still);
	const iggy::NpcActorControlState2DRegistry registry = Registry({ first, second });
	const iggy::NpcPlayControlFrameProposal2D newA = Request("npc:new-a", Proposal());
	const iggy::NpcPlayControlFrameProposal2D updateSecond = Request(
		"npc:second",
		Proposal(iggy::interactNpcObjective(Id("target:lever")), iggy::interactingNpcBehaviorState(Id("target:lever")), iggy::NpcMoveMode::Walk));
	const iggy::NpcPlayControlFrameProposal2D newB = Request(
		"npc:new-b",
		Proposal(iggy::waitNpcObjective(), iggy::waitingNpcBehaviorState(), iggy::NpcMoveMode::Still));

	const iggy::NpcPlayControlFrameApply2DResult result =
		iggy::NpcPlayControlFrameApplier2D {}.apply(registry, { newA, updateSecond, newB });

	Expect(result.appliedCount == 3, "different npc frame apply should apply all valid requests");
	Expect(result.registry.entries.size() == 4, "different npc frame apply should preserve existing plus appended entries");
	if (result.registry.entries.size() == 4) {
		Expect(SameControl(result.registry.entries[0], first), "different npc frame apply should preserve first existing entry");
		Expect(result.registry.entries[1].npcId == Id("npc:second"), "different npc frame apply should keep updated existing entry in place");
		Expect(SameBehavior(result.registry.entries[1].behavior, updateSecond.proposal.behavior), "different npc frame apply should update existing behavior in place");
		Expect(result.registry.entries[2].npcId == Id("npc:new-a"), "different npc frame apply should append first new npc in first-success order");
		Expect(result.registry.entries[3].npcId == Id("npc:new-b"), "different npc frame apply should append second new npc in first-success order");
	}
	if (result.entries.size() == 3) {
		Expect(result.entries[0].apply.appended && result.entries[0].apply.controlIndex == 2, "first new npc should append after existing entries");
		Expect(!result.entries[1].apply.appended && result.entries[1].apply.controlIndex == 1, "existing npc should update in place");
		Expect(result.entries[2].apply.appended && result.entries[2].apply.controlIndex == 3, "second new npc should append at final index");
	}
}

void TestAllFailureBatchReportsNoProposalsApplied()
{
	const iggy::NpcActorControlState2D existing = Control("npc:existing");
	const iggy::NpcActorControlState2DRegistry registry = Registry({ existing });
	const iggy::NpcPlayControlFrameProposal2D noProposal = Request("npc:missing", NoProposal());
	const iggy::NpcPlayControlFrameProposal2D missingNpcId = { {}, Proposal() };

	const iggy::NpcPlayControlFrameApply2DResult result =
		iggy::NpcPlayControlFrameApplier2D {}.apply(registry, { noProposal, missingNpcId });

	Expect(result.status == iggy::NpcPlayControlFrameApplyStatus2D::NoProposalsApplied, "all-failure frame apply should report no proposals applied");
	Expect(!result.applied(), "all-failure frame apply should not report applied");
	Expect(!result.changed, "all-failure frame apply should not report changed");
	Expect(result.appliedCount == 0, "all-failure frame apply should count zero applied");
	Expect(result.failedCount == 2, "all-failure frame apply should count both failures");
	Expect(SameControls(result.registry.entries, registry.entries), "all-failure frame apply should preserve original registry");
	if (result.entries.size() == 2) {
		ExpectEntry(result.entries[0], 0, noProposal, iggy::NpcPlayControlApplyStatus::NoProposal, "all-failure first entry should preserve NoProposal");
		ExpectEntry(result.entries[1], 1, missingNpcId, iggy::NpcPlayControlApplyStatus::MissingNpcId, "all-failure second entry should preserve MissingNpcId");
	}
}

void TestEntryPayloadsPreserveNestedApplyResults()
{
	const iggy::NpcActorControlState2DRegistry registry = Registry({});
	const iggy::NpcPlayControlFrameProposal2D request = Request("npc:entry", Proposal());

	const iggy::NpcPlayControlFrameApply2DResult result =
		iggy::NpcPlayControlFrameApplier2D {}.apply(registry, { request });

	Expect(result.entries.size() == 1, "entry preservation setup should record one entry");
	if (result.entries.size() == 1) {
		const iggy::NpcPlayControlFrameApplyEntry2D &entry = result.entries[0];
		Expect(entry.proposalIndex == 0, "entry should preserve proposal index");
		Expect(SameRequest(entry.request, request), "entry should preserve request payload");
		Expect(entry.apply.applied(), "entry should preserve nested applied helper");
		Expect(entry.apply.changed, "entry should preserve nested changed flag");
		Expect(entry.apply.appended, "entry should preserve nested appended flag");
		Expect(entry.apply.controlIndex == 0, "entry should preserve nested control index");
		Expect(SameProposal(entry.apply.proposal, request.proposal), "entry should preserve nested proposal");
		Expect(SameControls(entry.apply.registry.entries, result.registry.entries), "entry should preserve nested apply registry");
	}
}

void TestFrameApplyDoesNotMutateInputs()
{
	iggy::NpcActorControlState2DRegistry registry = Registry({
		Control("npc:guard"),
	});
	std::vector<iggy::NpcPlayControlFrameProposal2D> proposals {
		Request("npc:new", Proposal()),
		Request("npc:guard", Proposal(iggy::waitNpcObjective(), iggy::waitingNpcBehaviorState(), iggy::NpcMoveMode::Still)),
	};
	const iggy::NpcActorControlState2DRegistry registryBefore = registry;
	const std::vector<iggy::NpcPlayControlFrameProposal2D> proposalsBefore = proposals;

	const iggy::NpcPlayControlFrameApply2DResult result =
		iggy::NpcPlayControlFrameApplier2D {}.apply(registry, proposals);

	Expect(result.applied(), "immutability setup should apply at least one proposal");
	Expect(SameControls(registry.entries, registryBefore.entries), "frame apply should not mutate input registry");
	Expect(proposals.size() == proposalsBefore.size(), "frame apply should not mutate proposal vector size");
	if (proposals.size() == proposalsBefore.size()) {
		for (std::size_t index = 0; index < proposals.size(); ++index)
			Expect(SameRequest(proposals[index], proposalsBefore[index]), "frame apply should not mutate proposal payloads");
	}
}

} // namespace

int main()
{
	TestEmptyProposalListIsNoOp();
	TestSingleValidProposalAppends();
	TestSingleValidProposalUpdates();
	TestMixedValidAndInvalidProposalsContinueAfterFailures();
	TestDuplicateNpcProposalsApplySequentiallyAndLaterWins();
	TestFailedLaterDuplicateDoesNotEraseEarlierSuccess();
	TestDifferentNpcIdsPreserveRegistryOrderSemantics();
	TestAllFailureBatchReportsNoProposalsApplied();
	TestEntryPayloadsPreserveNestedApplyResults();
	TestFrameApplyDoesNotMutateInputs();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
