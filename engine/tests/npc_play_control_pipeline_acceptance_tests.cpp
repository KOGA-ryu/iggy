#include <cstdlib>
#include <vector>

#include "scene/ai/NpcFold.hpp"
#include "scene/ai/NpcHand.hpp"
#include "scene/ai/NpcPlayControlProposal.hpp"
#include "scene/ai/NpcRead.hpp"
#include "scene/ai/NpcStrengthDraw.hpp"
#include "scene/ai/NpcStrengthPool.hpp"
#include "scene/ai/NpcTell.hpp"
#include "scene/npc/NpcPlayControlFrameApply2D.hpp"
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

iggy::NpcStrengthEnt StrengthEnt(
	const char *entryId,
	const char *actionTag,
	iggy::NpcBehaviorStateType behaviorState,
	float weight = 4.0F,
	std::uint32_t minimumStrength = 0)
{
	return {
		Id(entryId),
		minimumStrength,
		behaviorState,
		Id(actionTag),
		weight,
		{ Id("map:acceptance") },
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

iggy::NpcActorControlState2DRegistry Registry(std::vector<iggy::NpcActorControlState2D> entries)
{
	return { entries };
}

struct CardChainResult {
	iggy::NpcStrengthPoolBuildResult poolBuild;
	iggy::NpcStrengthDrawResult strengthDraw;
	iggy::NpcHand hand;
	iggy::NpcRead read;
	iggy::NpcPlay play;
	iggy::NpcTell tell;
	iggy::NpcFold fold;
	iggy::NpcPlayControlProposal proposal;
};

CardChainResult RunCardChain(
	const iggy::NpcStrengthEnt &ent,
	const iggy::NpcPlayControlProposalContext &context = {},
	const iggy::NpcPlayControlProposalConfig &proposalConfig = {},
	const iggy::NpcFoldConfig &foldConfig = {},
	const iggy::NpcReadConfig &readConfig = {})
{
	CardChainResult result;
	result.poolBuild = iggy::NpcStrengthPoolBuilder {}.build({ ent });
	Expect(result.poolBuild.built, "acceptance fixture strength pool should build");
	if (!result.poolBuild.built)
		return result;

	iggy::NpcTraitSet traits;
	traits.strength = 12;
	result.strengthDraw = iggy::NpcStrengthDraw {}.draw(result.poolBuild.pool, traits, ent.behaviorState);
	result.hand = iggy::NpcHandAssembler {}.assemble(result.strengthDraw, {}, {}, {}, {}, {});
	result.read = iggy::NpcReader {}.read(result.hand, readConfig);
	result.play = iggy::NpcPlaySelector {}.play(result.read);
	result.tell = iggy::NpcTeller {}.tell(result.play);
	result.fold = iggy::NpcFolder {}.fold(result.tell, foldConfig);
	result.proposal = iggy::NpcPlayControlProjector {}.project(result.fold, context, proposalConfig);
	return result;
}

iggy::NpcPlayControlFrameReport2D ApplyAndReport(
	const iggy::NpcActorControlState2DRegistry &registry,
	const std::vector<iggy::NpcPlayControlFrameProposal2D> &requests)
{
	const iggy::NpcPlayControlFrameApply2DResult apply =
		iggy::NpcPlayControlFrameApplier2D {}.apply(registry, requests);
	return iggy::NpcPlayControlFrameReporter2D {}.report(apply);
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

bool SameProposalCore(const iggy::NpcPlayControlProposal &actual, const iggy::NpcPlayControlProposal &expected)
{
	return actual.status == expected.status
		&& actual.actionTag == expected.actionTag
		&& actual.requestedBehaviorState == expected.requestedBehaviorState
		&& SameObjective(actual.objective, expected.objective)
		&& SameBehavior(actual.behavior, expected.behavior)
		&& actual.moveMode == expected.moveMode;
}

bool SameEvents(
	const std::vector<iggy::NpcPlayControlFrameEvent2D> &actual,
	const std::vector<iggy::NpcPlayControlFrameEvent2D> &expected)
{
	return actual == expected;
}

bool SameStrengthEnt(const iggy::NpcStrengthEnt &actual, const iggy::NpcStrengthEnt &expected)
{
	return actual.entryId == expected.entryId
		&& actual.minimumStrength == expected.minimumStrength
		&& actual.behaviorState == expected.behaviorState
		&& actual.actionTag == expected.actionTag
		&& actual.weight == expected.weight
		&& actual.mapTags == expected.mapTags;
}

bool SameStrengthEnts(
	const std::vector<iggy::NpcStrengthEnt> &actual,
	const std::vector<iggy::NpcStrengthEnt> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (!SameStrengthEnt(actual[index], expected[index]))
			return false;
	}
	return true;
}

void TestValidTraitCardUpdatesExistingControlAndReportsUpdated()
{
	const CardChainResult chain = RunCardChain(
		StrengthEnt("strength:idle", "action:idle", iggy::NpcBehaviorStateType::Idle));
	const iggy::NpcActorControlState2D existing = Control(
		"npc:guard",
		iggy::guardNpcObjective(Id("anchor:old")),
		iggy::waitingNpcBehaviorState(),
		iggy::NpcMoveMode::Still);

	const iggy::NpcPlayControlFrameReport2D report = ApplyAndReport(
		Registry({ existing }),
		{ { Id("npc:guard"), chain.proposal } });

	Expect(chain.poolBuild.built, "valid update chain should build strength pool");
	Expect(chain.strengthDraw.hasEntries(), "valid update chain should draw strength entry");
	Expect(chain.hand.hasEnts(), "valid update chain should assemble hand");
	Expect(chain.read.hasRankedEnts(), "valid update chain should read ranked ent");
	Expect(chain.play.hasPlay(), "valid update chain should play selected ent");
	Expect(chain.fold.kept(), "valid update chain should keep play");
	Expect(chain.proposal.hasProposal(), "valid update chain should project proposal");
	Expect(chain.proposal.actionTag == Id("action:idle"), "valid update chain should preserve action tag");
	Expect(report.appliedCount == 1, "valid update report should count applied proposal");
	Expect(report.updatedCount == 1, "valid update report should count update");
	Expect(report.appendedCount == 0, "valid update report should count no append");
	Expect(report.changed, "valid update report should report control changed");
	Expect(SameEvents(report.events, {
		iggy::NpcPlayControlFrameEvent2D::ProposalApplied,
		iggy::NpcPlayControlFrameEvent2D::ProposalUpdated,
		iggy::NpcPlayControlFrameEvent2D::ControlChanged,
	}), "valid update report should preserve event order");
	Expect(report.registry.entries.size() == 1, "valid update should preserve one control");
	if (report.registry.entries.size() == 1) {
		Expect(report.registry.entries[0].npcId == Id("npc:guard"), "valid update should preserve npc id");
		Expect(report.registry.entries[0].objective.type == iggy::NpcObjectiveType::Wait, "valid update should set Wait objective");
		Expect(report.registry.entries[0].behavior.type == iggy::NpcBehaviorStateType::Idle, "valid update should set Idle behavior");
		Expect(report.registry.entries[0].moveMode == iggy::NpcMoveMode::Still, "valid update should set Still move mode");
	}
}

void TestValidTraitCardAppendsMissingControlAndReportsAppended()
{
	const CardChainResult chain = RunCardChain(
		StrengthEnt("strength:wait", "action:wait", iggy::NpcBehaviorStateType::Waiting));

	const iggy::NpcPlayControlFrameReport2D report = ApplyAndReport(
		Registry({}),
		{ { Id("npc:new"), chain.proposal } });

	Expect(chain.proposal.hasProposal(), "valid append chain should project proposal");
	Expect(report.appliedCount == 1, "valid append report should count applied proposal");
	Expect(report.appendedCount == 1, "valid append report should count appended proposal");
	Expect(report.updatedCount == 0, "valid append report should count no updates");
	Expect(report.registry.entries.size() == 1, "valid append should create one control");
	if (report.registry.entries.size() == 1) {
		Expect(report.registry.entries[0].npcId == Id("npc:new"), "valid append should preserve npc id");
		Expect(report.registry.entries[0].objective.type == iggy::NpcObjectiveType::Wait, "valid append should set Wait objective");
		Expect(report.registry.entries[0].behavior.type == iggy::NpcBehaviorStateType::Waiting, "valid append should set Waiting behavior");
	}
	Expect(SameEvents(report.events, {
		iggy::NpcPlayControlFrameEvent2D::ProposalApplied,
		iggy::NpcPlayControlFrameEvent2D::ProposalAppended,
		iggy::NpcPlayControlFrameEvent2D::ControlChanged,
	}), "valid append report should preserve event order");
}

void TestFoldedPlayDoesNotMutateControlsAndReportsNoProposal()
{
	const CardChainResult chain = RunCardChain(
		StrengthEnt("strength:low", "action:low", iggy::NpcBehaviorStateType::Idle, 1.0F),
		{},
		{},
		{ 2.0F });
	const iggy::NpcActorControlState2D existing = Control("npc:guard");

	const iggy::NpcPlayControlFrameReport2D report = ApplyAndReport(
		Registry({ existing }),
		{ { Id("npc:guard"), chain.proposal } });

	Expect(chain.play.hasPlay(), "folded chain should still produce a play before fold");
	Expect(chain.fold.folded(), "folded chain should fold below minimum score");
	Expect(!chain.proposal.hasProposal(), "folded chain should not project control proposal");
	Expect(chain.proposal.status == iggy::NpcPlayControlProposalStatus::NoKeptPlay, "folded chain proposal should report NoKeptPlay");
	Expect(report.appliedCount == 0, "folded report should count no applied proposals");
	Expect(report.failedCount == 1, "folded report should count one failed proposal");
	Expect(report.noProposalCount == 1, "folded report should count NoProposal failure");
	Expect(!report.changed, "folded report should not change controls");
	Expect(SameControls(report.registry.entries, { existing }), "folded report should preserve registry");
	Expect(SameEvents(report.events, {
		iggy::NpcPlayControlFrameEvent2D::ProposalFailed,
		iggy::NpcPlayControlFrameEvent2D::NoControlChanged,
	}), "folded report should emit failure then NoControlChanged");
}

void TestSeekingAndFleeingRequireAndPreserveTargetPosition()
{
	const CardChainResult missingSeek = RunCardChain(
		StrengthEnt("strength:seek-missing", "action:seek-missing", iggy::NpcBehaviorStateType::Seeking));
	Expect(missingSeek.proposal.status == iggy::NpcPlayControlProposalStatus::MissingTargetPosition, "seeking without target position should fail proposal");

	iggy::NpcPlayControlProposalContext seekContext;
	seekContext.hasTargetPosition = true;
	seekContext.targetPosition = { 8.0F, 9.0F };
	const CardChainResult seek = RunCardChain(
		StrengthEnt("strength:seek", "action:seek", iggy::NpcBehaviorStateType::Seeking),
		seekContext);
	Expect(seek.proposal.hasProposal(), "seeking with target position should project proposal");
	Expect(seek.proposal.objective.type == iggy::NpcObjectiveType::MoveTo, "seeking should map to MoveTo objective");
	Expect(NearVec(seek.proposal.objective.targetPosition, seekContext.targetPosition), "seeking objective should preserve target position");
	Expect(seek.proposal.behavior.type == iggy::NpcBehaviorStateType::Seeking, "seeking should map to Seeking behavior");
	Expect(NearVec(seek.proposal.behavior.targetPosition, seekContext.targetPosition), "seeking behavior should preserve target position");

	iggy::NpcPlayControlProposalContext fleeContext;
	fleeContext.hasTargetPosition = true;
	fleeContext.targetPosition = { -4.0F, 2.0F };
	iggy::NpcPlayControlProposalConfig config;
	config.fleeingMoveMode = iggy::NpcMoveMode::Sprint;
	const CardChainResult flee = RunCardChain(
		StrengthEnt("strength:flee", "action:flee", iggy::NpcBehaviorStateType::Fleeing),
		fleeContext,
		config);
	Expect(flee.proposal.hasProposal(), "fleeing with target position should project proposal");
	Expect(flee.proposal.objective.type == iggy::NpcObjectiveType::Flee, "fleeing should map to Flee objective");
	Expect(NearVec(flee.proposal.objective.targetPosition, fleeContext.targetPosition), "fleeing objective should preserve target position");
	Expect(flee.proposal.behavior.type == iggy::NpcBehaviorStateType::Fleeing, "fleeing should map to Fleeing behavior");
	Expect(NearVec(flee.proposal.behavior.targetPosition, fleeContext.targetPosition), "fleeing behavior should preserve target position");
	Expect(flee.proposal.moveMode == iggy::NpcMoveMode::Sprint, "fleeing should preserve configured move mode");
}

void TestAttackingAndInteractingRequireAndPreserveTargetId()
{
	const CardChainResult missingAttack = RunCardChain(
		StrengthEnt("strength:attack-missing", "action:attack-missing", iggy::NpcBehaviorStateType::Attacking));
	Expect(missingAttack.proposal.status == iggy::NpcPlayControlProposalStatus::MissingTargetId, "attacking without target id should fail proposal");

	iggy::NpcPlayControlProposalContext attackContext;
	attackContext.targetId = Id("target:enemy");
	const CardChainResult attack = RunCardChain(
		StrengthEnt("strength:attack", "action:attack", iggy::NpcBehaviorStateType::Attacking),
		attackContext);
	Expect(attack.proposal.hasProposal(), "attacking with target id should project proposal");
	Expect(attack.proposal.objective.type == iggy::NpcObjectiveType::Attack, "attacking should map to Attack objective");
	Expect(attack.proposal.objective.targetId == Id("target:enemy"), "attacking objective should preserve target id");
	Expect(attack.proposal.behavior.type == iggy::NpcBehaviorStateType::Attacking, "attacking should map to Attacking behavior");
	Expect(attack.proposal.behavior.targetId == Id("target:enemy"), "attacking behavior should preserve target id");

	iggy::NpcPlayControlProposalContext interactContext;
	interactContext.targetId = Id("target:door");
	const CardChainResult interact = RunCardChain(
		StrengthEnt("strength:interact", "action:interact", iggy::NpcBehaviorStateType::Interacting),
		interactContext);
	Expect(interact.proposal.hasProposal(), "interacting with target id should project proposal");
	Expect(interact.proposal.objective.type == iggy::NpcObjectiveType::Interact, "interacting should map to Interact objective");
	Expect(interact.proposal.objective.targetId == Id("target:door"), "interacting objective should preserve target id");
	Expect(interact.proposal.behavior.type == iggy::NpcBehaviorStateType::Interacting, "interacting should map to Interacting behavior");
	Expect(interact.proposal.behavior.targetId == Id("target:door"), "interacting behavior should preserve target id");
}

void TestDuplicateNpcProposalsApplySequentiallyAndLaterSuccessWins()
{
	const CardChainResult first = RunCardChain(
		StrengthEnt("strength:first", "action:first", iggy::NpcBehaviorStateType::Idle));
	const CardChainResult second = RunCardChain(
		StrengthEnt("strength:second", "action:second", iggy::NpcBehaviorStateType::Waiting));

	const iggy::NpcPlayControlFrameReport2D report = ApplyAndReport(
		Registry({}),
		{
			{ Id("npc:guard"), first.proposal },
			{ Id("npc:guard"), second.proposal },
		});

	Expect(report.appliedCount == 2, "duplicate success report should count both applications");
	Expect(report.duplicateNpcProposalCount == 1, "duplicate success report should count later duplicate");
	Expect(report.registry.entries.size() == 1, "duplicate success should keep one final control");
	if (report.registry.entries.size() == 1) {
		Expect(report.registry.entries[0].npcId == Id("npc:guard"), "duplicate success should preserve npc id");
		Expect(report.registry.entries[0].behavior.type == iggy::NpcBehaviorStateType::Waiting, "duplicate later successful behavior should win");
		Expect(report.registry.entries[0].objective.type == iggy::NpcObjectiveType::Wait, "duplicate later successful objective should win");
	}
	Expect(SameEvents(report.events, {
		iggy::NpcPlayControlFrameEvent2D::ProposalApplied,
		iggy::NpcPlayControlFrameEvent2D::ProposalApplied,
		iggy::NpcPlayControlFrameEvent2D::ProposalAppended,
		iggy::NpcPlayControlFrameEvent2D::ProposalUpdated,
		iggy::NpcPlayControlFrameEvent2D::DuplicateNpcProposalObserved,
		iggy::NpcPlayControlFrameEvent2D::ControlChanged,
	}), "duplicate success report should preserve event order");
}

void TestFailedLaterDuplicateDoesNotEraseEarlierSuccessfulControl()
{
	const CardChainResult first = RunCardChain(
		StrengthEnt("strength:first", "action:first", iggy::NpcBehaviorStateType::Idle));
	const CardChainResult folded = RunCardChain(
		StrengthEnt("strength:folded", "action:folded", iggy::NpcBehaviorStateType::Waiting, 1.0F),
		{},
		{},
		{ 2.0F });

	const iggy::NpcPlayControlFrameReport2D report = ApplyAndReport(
		Registry({}),
		{
			{ Id("npc:guard"), first.proposal },
			{ Id("npc:guard"), folded.proposal },
		});

	Expect(first.proposal.hasProposal(), "failed duplicate setup should have first proposal");
	Expect(!folded.proposal.hasProposal(), "failed duplicate setup should have folded proposal");
	Expect(report.appliedCount == 1, "failed duplicate report should count earlier success");
	Expect(report.failedCount == 1, "failed duplicate report should count later failure");
	Expect(report.noProposalCount == 1, "failed duplicate report should count folded proposal as NoProposal");
	Expect(report.duplicateNpcProposalCount == 1, "failed duplicate report should count duplicate attempt");
	Expect(report.registry.entries.size() == 1, "failed duplicate should keep earlier control");
	if (report.registry.entries.size() == 1)
		Expect(report.registry.entries[0].behavior.type == iggy::NpcBehaviorStateType::Idle, "failed duplicate should not erase earlier behavior");
}

void TestNamespacedAndUnqualifiedIdsAndActionTagsRemainDistinct()
{
	const CardChainResult unqualified = RunCardChain(
		StrengthEnt("strength:plain", "action:plain", iggy::NpcBehaviorStateType::Idle));
	const CardChainResult namespaced = RunCardChain(
		StrengthEnt("strength:namespaced", "action:namespaced", iggy::NpcBehaviorStateType::Waiting));

	const iggy::NpcPlayControlFrameReport2D report = ApplyAndReport(
		Registry({}),
		{
			{ Id("guard"), unqualified.proposal },
			{ Id("npc:guard"), namespaced.proposal },
		});

	Expect(unqualified.proposal.actionTag == Id("action:plain"), "unqualified proposal should preserve exact action tag");
	Expect(namespaced.proposal.actionTag == Id("action:namespaced"), "namespaced proposal should preserve exact action tag");
	Expect(report.duplicateNpcProposalCount == 0, "namespaced/unqualified npc ids should not be duplicate observations");
	Expect(report.registry.entries.size() == 2, "namespaced/unqualified npc ids should create distinct controls");
	if (report.registry.entries.size() == 2) {
		Expect(report.registry.entries[0].npcId == Id("guard"), "first distinct control should preserve unqualified id");
		Expect(report.registry.entries[1].npcId == Id("npc:guard"), "second distinct control should preserve namespaced id");
	}
}

void TestPipelineInputsAreNotMutated()
{
	const iggy::NpcStrengthEnt ent = StrengthEnt("strength:immutable", "action:immutable", iggy::NpcBehaviorStateType::Idle);
	CardChainResult chain = RunCardChain(ent);
	iggy::NpcActorControlState2DRegistry registry = Registry({ Control("npc:guard") });
	std::vector<iggy::NpcPlayControlFrameProposal2D> requests {
		{ Id("npc:guard"), chain.proposal },
	};
	const iggy::NpcStrengthPool poolBefore = chain.poolBuild.pool;
	const iggy::NpcStrengthDrawResult drawBefore = chain.strengthDraw;
	const iggy::NpcHand handBefore = chain.hand;
	const iggy::NpcRead readBefore = chain.read;
	const iggy::NpcPlay playBefore = chain.play;
	const iggy::NpcTell tellBefore = chain.tell;
	const iggy::NpcFold foldBefore = chain.fold;
	const iggy::NpcPlayControlProposal proposalBefore = chain.proposal;
	const iggy::NpcActorControlState2DRegistry registryBefore = registry;
	const std::vector<iggy::NpcPlayControlFrameProposal2D> requestsBefore = requests;

	const iggy::NpcPlayControlFrameApply2DResult apply =
		iggy::NpcPlayControlFrameApplier2D {}.apply(registry, requests);
	const iggy::NpcPlayControlFrameReport2D report =
		iggy::NpcPlayControlFrameReporter2D {}.report(apply);

	Expect(report.changed, "immutability setup should apply a control update");
	Expect(SameStrengthEnts(chain.poolBuild.pool.entries, poolBefore.entries), "pipeline should not mutate strength pool");
	Expect(chain.strengthDraw.entries.size() == drawBefore.entries.size(), "pipeline should not mutate draw entry count");
	Expect(chain.hand.ents.size() == handBefore.ents.size(), "pipeline should not mutate hand ent count");
	Expect(chain.read.rankedEnts.size() == readBefore.rankedEnts.size(), "pipeline should not mutate read ranked count");
	Expect(chain.play.status == playBefore.status, "pipeline should not mutate play status");
	Expect(chain.tell.playedCount == tellBefore.playedCount, "pipeline should not mutate tell counts");
	Expect(chain.fold.status == foldBefore.status && chain.fold.reason == foldBefore.reason, "pipeline should not mutate fold status");
	Expect(SameProposalCore(chain.proposal, proposalBefore), "pipeline should not mutate proposal");
	Expect(SameControls(registry.entries, registryBefore.entries), "pipeline should not mutate input control registry");
	Expect(requests.size() == requestsBefore.size() && SameProposalCore(requests[0].proposal, requestsBefore[0].proposal), "pipeline should not mutate proposal requests");
}

} // namespace

int main()
{
	TestValidTraitCardUpdatesExistingControlAndReportsUpdated();
	TestValidTraitCardAppendsMissingControlAndReportsAppended();
	TestFoldedPlayDoesNotMutateControlsAndReportsNoProposal();
	TestSeekingAndFleeingRequireAndPreserveTargetPosition();
	TestAttackingAndInteractingRequireAndPreserveTargetId();
	TestDuplicateNpcProposalsApplySequentiallyAndLaterSuccessWins();
	TestFailedLaterDuplicateDoesNotEraseEarlierSuccessfulControl();
	TestNamespacedAndUnqualifiedIdsAndActionTagsRemainDistinct();
	TestPipelineInputsAreNotMutated();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
