#include <cstdlib>
#include <vector>

#include "scene/ai/NpcMapPlayControlFrameStep2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::AiMapQuery2DResult Map(std::vector<iggy::ResourceId> tags = {})
{
	iggy::AiMapQuery2DResult map;
	map.status = tags.empty() ? iggy::AiMapQuery2DStatus::NoMatch : iggy::AiMapQuery2DStatus::Matched;
	map.tags = tags;
	return map;
}

iggy::NpcHandEnt Ent(
	const char *entryId,
	const char *actionTag,
	iggy::NpcBehaviorStateType behaviorState,
	float weight,
	std::vector<iggy::ResourceId> mapTags = {},
	iggy::NpcHandTraitSource source = iggy::NpcHandTraitSource::Strength,
	std::size_t drawEntryIndex = 0)
{
	return {
		source,
		Id(entryId),
		Id(actionTag),
		behaviorState,
		weight,
		mapTags,
		drawEntryIndex,
	};
}

iggy::NpcHand Hand(std::vector<iggy::NpcHandEnt> ents = {})
{
	return { ents, {} };
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

iggy::NpcMapPlayControlFrameStep2DRequest Request(
	const char *npcId,
	const iggy::NpcHand &hand,
	const iggy::AiMapQuery2DResult &map,
	const iggy::NpcPlayControlProposalContext &context = {})
{
	return {
		Id(npcId),
		hand,
		map,
		context,
	};
}

iggy::NpcPlayControlProposalContext TargetPosition(iggy::Vec2 position)
{
	iggy::NpcPlayControlProposalContext context;
	context.hasTargetPosition = true;
	context.targetPosition = position;
	return context;
}

iggy::NpcPlayControlProposalContext TargetId(const char *targetId)
{
	iggy::NpcPlayControlProposalContext context;
	context.targetId = Id(targetId);
	return context;
}

iggy::NpcMapPlayControlFrameStep2DConfig MapBoostConfig()
{
	iggy::NpcMapPlayControlFrameStep2DConfig config;
	config.mapRead.matchedMapTagBonus = 4.0F;
	return config;
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

bool SameEnt(const iggy::NpcHandEnt &actual, const iggy::NpcHandEnt &expected)
{
	return actual.source == expected.source
		&& actual.entryId == expected.entryId
		&& actual.actionTag == expected.actionTag
		&& actual.behaviorState == expected.behaviorState
		&& actual.weight == expected.weight
		&& actual.mapTags == expected.mapTags
		&& actual.drawEntryIndex == expected.drawEntryIndex;
}

bool SameHand(const iggy::NpcHand &actual, const iggy::NpcHand &expected)
{
	if (actual.ents.size() != expected.ents.size())
		return false;
	for (std::size_t index = 0; index < actual.ents.size(); ++index) {
		if (!SameEnt(actual.ents[index], expected.ents[index]))
			return false;
	}
	return actual.issues.size() == expected.issues.size();
}

void TestEmptyRequestsPreserveControls()
{
	const iggy::NpcActorControlState2DRegistry controls { { Control("npc:guard") } };

	const iggy::NpcMapPlayControlFrameStep2DResult result =
		iggy::NpcMapPlayControlFrameStepper2D {}.step(controls, {});

	Expect(result.status == iggy::NpcMapPlayControlFrameStep2DStatus::NoRequests, "empty map play frame step should report NoRequests");
	Expect(result.requestCount == 0 && result.proposalCount == 0, "empty map play frame step should count no requests");
	Expect(result.entries.empty(), "empty map play frame step should have no entries");
	Expect(!result.changed(), "empty map play frame step should report no change");
	Expect(SameControls(result.registry.entries, controls.entries), "empty map play frame step should preserve controls");
	Expect(result.report.events == std::vector<iggy::NpcPlayControlFrameEvent2D> { iggy::NpcPlayControlFrameEvent2D::NoControlChanged }, "empty map play frame step should preserve no-change report event");
}

void TestMapAwareRequestChangesSelectionAndUpdatesExistingControl()
{
	const iggy::NpcHand hand = Hand({
		Ent("strength:seek", "action:seek", iggy::NpcBehaviorStateType::Seeking, 2.0F, { Id("zone:cover") }),
		Ent("wisdom:idle", "action:idle", iggy::NpcBehaviorStateType::Idle, 5.0F, { Id("zone:loud") }, iggy::NpcHandTraitSource::Wisdom),
	});
	const iggy::NpcActorControlState2DRegistry controls { { Control("npc:guard") } };

	const iggy::NpcMapPlayControlFrameStep2DResult result =
		iggy::NpcMapPlayControlFrameStepper2D {}.step(
			controls,
			{ Request("npc:guard", hand, Map({ Id("zone:cover") }), TargetPosition({ 9.0F, 1.0F })) },
			MapBoostConfig());

	Expect(result.status == iggy::NpcMapPlayControlFrameStep2DStatus::Ran, "map selection update should run and change controls");
	Expect(result.entries.size() == 1, "map selection update should preserve one entry");
	if (result.entries.size() == 1) {
		Expect(result.entries[0].mapPlay.mapChangedSelection, "map selection update should preserve changed selection report");
		Expect(result.entries[0].mapPlay.rawSelectedActionTag == Id("action:idle"), "map selection update should preserve raw winner");
		Expect(result.entries[0].mapPlay.mapSelectedActionTag == Id("action:seek"), "map selection update should preserve map winner");
		Expect(result.entries[0].proposal.hasProposal(), "map selection update should produce proposal");
		Expect(result.entries[0].frameProposal.proposal.actionTag == Id("action:seek"), "map selection update should frame map-selected proposal");
	}
	Expect(result.mapChangedSelectionCount == 1, "map selection update should count changed selection");
	Expect(result.proposedCount == 1 && result.proposalFailedCount == 0, "map selection update should count proposed request");
	Expect(result.appliedCount == 1 && result.failedApplyCount == 0, "map selection update should apply one control");
	Expect(result.registry.entries.size() == 1, "map selection update should preserve one control");
	if (result.registry.entries.size() == 1) {
		Expect(result.registry.entries[0].npcId == Id("npc:guard"), "map selection update should preserve npc id");
		Expect(result.registry.entries[0].behavior.type == iggy::NpcBehaviorStateType::Seeking, "map selection update should use map-selected behavior");
		Expect(NearVec(result.registry.entries[0].behavior.targetPosition, { 9.0F, 1.0F }), "map selection update should preserve target position");
	}
}

void TestMapAwareRequestAppendsMissingControl()
{
	const iggy::NpcHand hand = Hand({
		Ent("strength:wait", "action:wait", iggy::NpcBehaviorStateType::Waiting, 2.0F),
	});

	const iggy::NpcMapPlayControlFrameStep2DResult result =
		iggy::NpcMapPlayControlFrameStepper2D {}.step({}, { Request("npc:new", hand, Map()) });

	Expect(result.status == iggy::NpcMapPlayControlFrameStep2DStatus::Ran, "missing control request should append and run");
	Expect(result.appliedCount == 1 && result.report.appendedCount == 1, "missing control request should append one control");
	Expect(result.registry.entries.size() == 1, "missing control request should return one control");
	if (result.registry.entries.size() == 1) {
		Expect(result.registry.entries[0].npcId == Id("npc:new"), "missing control request should preserve npc id");
		Expect(result.registry.entries[0].behavior.type == iggy::NpcBehaviorStateType::Waiting, "missing control request should preserve waiting behavior");
	}
}

void TestFoldedNoPlayRequestPreservesDiagnosticsAndNoChange()
{
	const iggy::NpcActorControlState2DRegistry controls { { Control("npc:guard") } };

	const iggy::NpcMapPlayControlFrameStep2DResult result =
		iggy::NpcMapPlayControlFrameStepper2D {}.step(
			controls,
			{ Request("npc:guard", {}, Map()) });

	Expect(result.status == iggy::NpcMapPlayControlFrameStep2DStatus::NoControlsChanged, "folded no-play request should report no controls changed");
	Expect(result.proposedCount == 0 && result.proposalFailedCount == 1, "folded no-play request should count failed proposal");
	Expect(result.appliedCount == 0 && result.failedApplyCount == 1, "folded no-play request should fail apply");
	Expect(SameControls(result.registry.entries, controls.entries), "folded no-play request should preserve controls");
	Expect(result.entries.size() == 1, "folded no-play request should preserve entry diagnostics");
	if (result.entries.size() == 1) {
		Expect(result.entries[0].mapPlay.folded(), "folded no-play request should preserve map play fold");
		Expect(result.entries[0].proposal.status == iggy::NpcPlayControlProposalStatus::NoKeptPlay, "folded no-play request should preserve no-kept proposal");
	}
	Expect(result.report.noProposalCount == 1, "folded no-play request should preserve frame report no-proposal count");
}

void TestProposalContextFailurePreservesDiagnosticsAndNoChange()
{
	const iggy::NpcActorControlState2DRegistry controls { { Control("npc:guard") } };
	const iggy::NpcHand hand = Hand({
		Ent("strength:seek", "action:seek", iggy::NpcBehaviorStateType::Seeking, 2.0F, { Id("zone:cover") }),
	});

	const iggy::NpcMapPlayControlFrameStep2DResult result =
		iggy::NpcMapPlayControlFrameStepper2D {}.step(
			controls,
			{ Request("npc:guard", hand, Map({ Id("zone:cover") })) },
			MapBoostConfig());

	Expect(result.status == iggy::NpcMapPlayControlFrameStep2DStatus::NoControlsChanged, "missing target context should not change controls");
	Expect(result.proposedCount == 0 && result.proposalFailedCount == 1, "missing target context should count failed proposal");
	Expect(result.failedApplyCount == 1, "missing target context should fail frame apply");
	Expect(SameControls(result.registry.entries, controls.entries), "missing target context should preserve controls");
	Expect(result.entries.size() == 1, "missing target context should preserve entry diagnostics");
	if (result.entries.size() == 1) {
		Expect(result.entries[0].mapPlay.kept(), "missing target context should still preserve kept map play");
		Expect(result.entries[0].proposal.status == iggy::NpcPlayControlProposalStatus::MissingTargetPosition, "missing target context should preserve proposal failure");
	}
	Expect(result.report.noProposalCount == 1, "missing target context should flow through frame apply diagnostics");
}

void TestMultipleNpcRequestsPreserveOrderAndCounts()
{
	const iggy::NpcHand first = Hand({
		Ent("strength:first", "action:first", iggy::NpcBehaviorStateType::Waiting, 1.0F),
	});
	const iggy::NpcHand second = Hand({
		Ent("strength:second", "action:second", iggy::NpcBehaviorStateType::Interacting, 1.0F),
	});

	const iggy::NpcMapPlayControlFrameStep2DResult result =
		iggy::NpcMapPlayControlFrameStepper2D {}.step(
			{},
			{
				Request("npc:first", first, Map()),
				Request("npc:second", second, Map(), TargetId("target:lever")),
			});

	Expect(result.requestCount == 2 && result.proposalCount == 2, "multi request should count requests and proposals");
	Expect(result.proposedCount == 2 && result.proposalFailedCount == 0, "multi request should count proposed entries");
	Expect(result.appliedCount == 2 && result.failedApplyCount == 0, "multi request should apply both controls");
	Expect(result.entries.size() == 2, "multi request should preserve entry count");
	if (result.entries.size() == 2) {
		Expect(result.entries[0].requestIndex == 0 && result.entries[0].request.npcId == Id("npc:first"), "multi request should preserve first order");
		Expect(result.entries[1].requestIndex == 1 && result.entries[1].request.npcId == Id("npc:second"), "multi request should preserve second order");
	}
	Expect(result.registry.entries.size() == 2, "multi request should return two controls");
	if (result.registry.entries.size() == 2) {
		Expect(result.registry.entries[0].npcId == Id("npc:first"), "multi request should append first npc first");
		Expect(result.registry.entries[1].npcId == Id("npc:second"), "multi request should append second npc second");
	}
}

void TestDuplicateNpcRequestsApplySequentiallyAndReportDuplicate()
{
	const iggy::NpcHand first = Hand({
		Ent("strength:first", "action:first", iggy::NpcBehaviorStateType::Waiting, 1.0F),
	});
	const iggy::NpcHand second = Hand({
		Ent("strength:second", "action:second", iggy::NpcBehaviorStateType::Attacking, 1.0F),
	});

	const iggy::NpcMapPlayControlFrameStep2DResult result =
		iggy::NpcMapPlayControlFrameStepper2D {}.step(
			{},
			{
				Request("npc:guard", first, Map()),
				Request("npc:guard", second, Map(), TargetId("target:enemy")),
			});

	Expect(result.appliedCount == 2 && result.failedApplyCount == 0, "duplicate step should apply both requests sequentially");
	Expect(result.report.duplicateNpcProposalCount == 1, "duplicate step should preserve nested duplicate count");
	Expect(result.registry.entries.size() == 1, "duplicate step should return one final control");
	if (result.registry.entries.size() == 1) {
		Expect(result.registry.entries[0].npcId == Id("npc:guard"), "duplicate step should preserve npc id");
		Expect(result.registry.entries[0].behavior.type == iggy::NpcBehaviorStateType::Attacking, "duplicate step later success should win");
		Expect(result.registry.entries[0].behavior.targetId == Id("target:enemy"), "duplicate step should preserve later target id");
	}
}

void TestReturnedResultPreservesNestedReportsAndRegistry()
{
	const iggy::NpcHand hand = Hand({
		Ent("strength:wait", "action:wait", iggy::NpcBehaviorStateType::Waiting, 1.0F),
	});

	const iggy::NpcMapPlayControlFrameStep2DResult result =
		iggy::NpcMapPlayControlFrameStepper2D {}.step({}, { Request("npc:one", hand, Map()) });

	Expect(result.entries.size() == 1, "preservation setup should produce one entry");
	if (result.entries.size() == 1) {
		Expect(SameHand(result.entries[0].request.hand, hand), "step entry should preserve request hand");
		Expect(result.entries[0].mapPlay.hasMapSelection, "step entry should preserve map play report");
		Expect(result.entries[0].proposal.actionTag == Id("action:wait"), "step entry should preserve proposal facts");
		Expect(result.entries[0].frameProposal.npcId == Id("npc:one"), "step entry should preserve frame proposal request");
	}
	Expect(result.apply.entries.size() == 1, "step result should preserve apply result");
	Expect(result.report.apply.entries.size() == 1, "step result should preserve frame report");
	Expect(SameControls(result.registry.entries, result.report.registry.entries), "step result registry should match frame report registry");
}

void TestInputsAreNotMutated()
{
	iggy::NpcActorControlState2DRegistry controls { { Control("npc:guard") } };
	std::vector<iggy::NpcMapPlayControlFrameStep2DRequest> requests {
		Request("npc:guard", Hand({
			Ent("strength:wait", "action:wait", iggy::NpcBehaviorStateType::Waiting, 1.0F, { Id("zone:quiet") }),
		}), Map({ Id("zone:quiet") })),
	};
	const iggy::NpcActorControlState2DRegistry controlsBefore = controls;
	const std::vector<iggy::NpcMapPlayControlFrameStep2DRequest> requestsBefore = requests;

	const iggy::NpcMapPlayControlFrameStep2DResult result =
		iggy::NpcMapPlayControlFrameStepper2D {}.step(controls, requests);

	Expect(result.changed(), "immutability setup should update control");
	Expect(SameControls(controls.entries, controlsBefore.entries), "map play control frame step should not mutate controls");
	Expect(requests.size() == requestsBefore.size(), "map play control frame step should not mutate request count");
	if (requests.size() == requestsBefore.size() && !requests.empty()) {
		Expect(requests[0].npcId == requestsBefore[0].npcId, "map play control frame step should preserve request npc id");
		Expect(SameHand(requests[0].hand, requestsBefore[0].hand), "map play control frame step should not mutate request hand");
		Expect(requests[0].map.tags == requestsBefore[0].map.tags, "map play control frame step should not mutate request map");
	}
}

} // namespace

int main()
{
	TestEmptyRequestsPreserveControls();
	TestMapAwareRequestChangesSelectionAndUpdatesExistingControl();
	TestMapAwareRequestAppendsMissingControl();
	TestFoldedNoPlayRequestPreservesDiagnosticsAndNoChange();
	TestProposalContextFailurePreservesDiagnosticsAndNoChange();
	TestMultipleNpcRequestsPreserveOrderAndCounts();
	TestDuplicateNpcRequestsApplySequentiallyAndReportDuplicate();
	TestReturnedResultPreservesNestedReportsAndRegistry();
	TestInputsAreNotMutated();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
