#include <cstdlib>
#include <vector>

#include "scene/ai/NpcPlayControlProposal.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::NpcHandEnt HandEnt(iggy::NpcBehaviorStateType behaviorState, const char *actionTag = "action:selected")
{
	return {
		iggy::NpcHandTraitSource::Strength,
		Id("strength:selected"),
		Id(actionTag),
		behaviorState,
		4.0F,
		{ Id("tag:selected") },
		7,
	};
}

iggy::NpcReadEnt ReadEnt(iggy::NpcBehaviorStateType behaviorState, const char *actionTag = "action:selected")
{
	return {
		HandEnt(behaviorState, actionTag),
		4.0F,
		2,
	};
}

iggy::NpcFold KeptFold(iggy::NpcBehaviorStateType behaviorState, const char *actionTag = "action:selected")
{
	const iggy::NpcReadEnt selected = ReadEnt(behaviorState, actionTag);
	const iggy::NpcRead read {
		{ { selected.ent }, {} },
		{ selected },
		{},
	};
	const iggy::NpcTell tell = iggy::NpcTeller {}.tell(iggy::NpcPlaySelector {}.play(read));
	return iggy::NpcFolder {}.fold(tell);
}

iggy::NpcFold FoldedFold()
{
	const iggy::NpcTell tell = iggy::NpcTeller {}.tell(iggy::NpcPlaySelector {}.play({}));
	return iggy::NpcFolder {}.fold(tell);
}

bool SameHandEnt(const iggy::NpcHandEnt &actual, const iggy::NpcHandEnt &expected)
{
	return actual.source == expected.source
		&& actual.entryId == expected.entryId
		&& actual.actionTag == expected.actionTag
		&& actual.behaviorState == expected.behaviorState
		&& actual.weight == expected.weight
		&& actual.mapTags == expected.mapTags
		&& actual.drawEntryIndex == expected.drawEntryIndex;
}

bool SameReadEnt(const iggy::NpcReadEnt &actual, const iggy::NpcReadEnt &expected)
{
	return SameHandEnt(actual.ent, expected.ent)
		&& actual.score == expected.score
		&& actual.handIndex == expected.handIndex;
}

bool SameFoldSelectedPlay(const iggy::NpcFold &actual, const iggy::NpcFold &expected)
{
	return actual.status == expected.status
		&& actual.reason == expected.reason
		&& actual.tell.play.status == expected.tell.play.status
		&& SameReadEnt(actual.tell.play.selected, expected.tell.play.selected)
		&& actual.tell.handEntCount == expected.tell.handEntCount
		&& actual.tell.rankedEntCount == expected.tell.rankedEntCount
		&& actual.tell.issueCount == expected.tell.issueCount
		&& actual.tell.playedCount == expected.tell.playedCount;
}

void ExpectNoProposal(const iggy::NpcPlayControlProposal &proposal, iggy::NpcPlayControlProposalStatus status, const char *message)
{
	Expect(proposal.status == status, message);
	Expect(!proposal.hasProposal(), "non-proposed result should not report proposal");
}

void TestFoldedPlayReturnsNoKeptPlay()
{
	const iggy::NpcFold fold = FoldedFold();

	const iggy::NpcPlayControlProposal proposal = iggy::NpcPlayControlProjector {}.project(fold);

	ExpectNoProposal(proposal, iggy::NpcPlayControlProposalStatus::NoKeptPlay, "folded play should return NoKeptPlay");
	Expect(SameFoldSelectedPlay(proposal.fold, fold), "NoKeptPlay result should preserve fold");
	Expect(proposal.actionTag.empty(), "NoKeptPlay should not select an action tag");
}

void TestIdleKeptPlayProjectsToWaitIdleStill()
{
	const iggy::NpcFold fold = KeptFold(iggy::NpcBehaviorStateType::Idle, "action:idle");

	const iggy::NpcPlayControlProposal proposal = iggy::NpcPlayControlProjector {}.project(fold);

	Expect(proposal.status == iggy::NpcPlayControlProposalStatus::Proposed, "idle play should propose control");
	Expect(proposal.hasProposal(), "idle play should report proposal");
	Expect(proposal.actionTag == Id("action:idle"), "idle proposal should preserve selected action tag");
	Expect(proposal.requestedBehaviorState == iggy::NpcBehaviorStateType::Idle, "idle proposal should preserve requested behavior state");
	Expect(proposal.objective.type == iggy::NpcObjectiveType::Wait, "idle proposal should use Wait objective");
	Expect(proposal.behavior.type == iggy::NpcBehaviorStateType::Idle, "idle proposal should use Idle behavior");
	Expect(proposal.moveMode == iggy::NpcMoveMode::Still, "idle proposal should use configured default Still mode");
}

void TestWaitingKeptPlayProjectsToWaitWaiting()
{
	const iggy::NpcFold fold = KeptFold(iggy::NpcBehaviorStateType::Waiting);

	const iggy::NpcPlayControlProposal proposal = iggy::NpcPlayControlProjector {}.project(fold);

	Expect(proposal.status == iggy::NpcPlayControlProposalStatus::Proposed, "waiting play should propose control");
	Expect(proposal.objective.type == iggy::NpcObjectiveType::Wait, "waiting proposal should use Wait objective");
	Expect(proposal.behavior.type == iggy::NpcBehaviorStateType::Waiting, "waiting proposal should use Waiting behavior");
	Expect(proposal.moveMode == iggy::NpcMoveMode::Still, "waiting proposal should use Still mode");
}

void TestNoneKeptPlayProjectsToNoneAndNoMove()
{
	const iggy::NpcFold fold = KeptFold(iggy::NpcBehaviorStateType::None);

	const iggy::NpcPlayControlProposal proposal = iggy::NpcPlayControlProjector {}.project(fold);

	Expect(proposal.status == iggy::NpcPlayControlProposalStatus::Proposed, "none play should propose explicit no-control facts");
	Expect(proposal.objective.type == iggy::NpcObjectiveType::None, "none proposal should use None objective");
	Expect(proposal.behavior.type == iggy::NpcBehaviorStateType::None, "none proposal should use None behavior");
	Expect(proposal.moveMode == iggy::NpcMoveMode::None, "none proposal should use None move mode");
}

void TestSeekingWithoutTargetPositionReportsMissingTargetPosition()
{
	const iggy::NpcFold fold = KeptFold(iggy::NpcBehaviorStateType::Seeking, "action:seek");

	const iggy::NpcPlayControlProposal proposal = iggy::NpcPlayControlProjector {}.project(fold);

	ExpectNoProposal(proposal, iggy::NpcPlayControlProposalStatus::MissingTargetPosition, "seeking without target position should fail");
	Expect(proposal.actionTag == Id("action:seek"), "missing target position should still preserve selected action tag");
	Expect(proposal.requestedBehaviorState == iggy::NpcBehaviorStateType::Seeking, "missing target position should preserve requested state");
}

void TestSeekingWithTargetPositionProjectsMoveToSeeking()
{
	const iggy::NpcFold fold = KeptFold(iggy::NpcBehaviorStateType::Seeking);
	iggy::NpcPlayControlProposalContext context;
	context.hasTargetPosition = true;
	context.targetPosition = { 4.0F, 6.0F };
	iggy::NpcPlayControlProposalConfig config;
	config.seekingMoveMode = iggy::NpcMoveMode::Jog;

	const iggy::NpcPlayControlProposal proposal = iggy::NpcPlayControlProjector {}.project(fold, context, config);

	Expect(proposal.status == iggy::NpcPlayControlProposalStatus::Proposed, "seeking with target position should propose control");
	Expect(proposal.objective.type == iggy::NpcObjectiveType::MoveTo, "seeking proposal should use MoveTo objective");
	Expect(proposal.objective.targetPosition == context.targetPosition, "seeking objective should preserve target position");
	Expect(proposal.behavior.type == iggy::NpcBehaviorStateType::Seeking, "seeking proposal should use Seeking behavior");
	Expect(proposal.behavior.targetPosition == context.targetPosition, "seeking behavior should preserve target position");
	Expect(proposal.moveMode == iggy::NpcMoveMode::Jog, "seeking proposal should use configured move mode");
}

void TestFleeingWithTargetPositionProjectsFleeing()
{
	const iggy::NpcFold fold = KeptFold(iggy::NpcBehaviorStateType::Fleeing);
	iggy::NpcPlayControlProposalContext context;
	context.hasTargetPosition = true;
	context.targetPosition = { -2.0F, 3.5F };
	iggy::NpcPlayControlProposalConfig config;
	config.fleeingMoveMode = iggy::NpcMoveMode::Sprint;

	const iggy::NpcPlayControlProposal proposal = iggy::NpcPlayControlProjector {}.project(fold, context, config);

	Expect(proposal.status == iggy::NpcPlayControlProposalStatus::Proposed, "fleeing with target position should propose control");
	Expect(proposal.objective.type == iggy::NpcObjectiveType::Flee, "fleeing proposal should use Flee objective");
	Expect(proposal.objective.targetPosition == context.targetPosition, "flee objective should preserve target position");
	Expect(proposal.behavior.type == iggy::NpcBehaviorStateType::Fleeing, "fleeing proposal should use Fleeing behavior");
	Expect(proposal.behavior.targetPosition == context.targetPosition, "flee behavior should preserve target position");
	Expect(proposal.moveMode == iggy::NpcMoveMode::Sprint, "fleeing proposal should use configured move mode");
}

void TestAttackingWithoutTargetIdReportsMissingTargetId()
{
	const iggy::NpcFold fold = KeptFold(iggy::NpcBehaviorStateType::Attacking, "action:attack");

	const iggy::NpcPlayControlProposal proposal = iggy::NpcPlayControlProjector {}.project(fold);

	ExpectNoProposal(proposal, iggy::NpcPlayControlProposalStatus::MissingTargetId, "attacking without target id should fail");
	Expect(proposal.actionTag == Id("action:attack"), "missing target id should preserve selected action tag");
	Expect(proposal.requestedBehaviorState == iggy::NpcBehaviorStateType::Attacking, "missing target id should preserve requested state");
}

void TestAttackingWithTargetIdProjectsAttack()
{
	const iggy::NpcFold fold = KeptFold(iggy::NpcBehaviorStateType::Attacking);
	iggy::NpcPlayControlProposalContext context;
	context.targetId = Id("target:enemy");
	iggy::NpcPlayControlProposalConfig config;
	config.attackingMoveMode = iggy::NpcMoveMode::Run;

	const iggy::NpcPlayControlProposal proposal = iggy::NpcPlayControlProjector {}.project(fold, context, config);

	Expect(proposal.status == iggy::NpcPlayControlProposalStatus::Proposed, "attacking with target id should propose control");
	Expect(proposal.objective.type == iggy::NpcObjectiveType::Attack, "attacking proposal should use Attack objective");
	Expect(proposal.objective.targetId == Id("target:enemy"), "attack objective should preserve target id");
	Expect(proposal.behavior.type == iggy::NpcBehaviorStateType::Attacking, "attacking proposal should use Attacking behavior");
	Expect(proposal.behavior.targetId == Id("target:enemy"), "attack behavior should preserve target id");
	Expect(proposal.moveMode == iggy::NpcMoveMode::Run, "attacking proposal should use configured move mode");
}

void TestInteractingWithTargetIdProjectsInteract()
{
	const iggy::NpcFold fold = KeptFold(iggy::NpcBehaviorStateType::Interacting);
	iggy::NpcPlayControlProposalContext context;
	context.targetId = Id("target:door");
	iggy::NpcPlayControlProposalConfig config;
	config.interactingMoveMode = iggy::NpcMoveMode::Walk;

	const iggy::NpcPlayControlProposal proposal = iggy::NpcPlayControlProjector {}.project(fold, context, config);

	Expect(proposal.status == iggy::NpcPlayControlProposalStatus::Proposed, "interacting with target id should propose control");
	Expect(proposal.objective.type == iggy::NpcObjectiveType::Interact, "interacting proposal should use Interact objective");
	Expect(proposal.objective.targetId == Id("target:door"), "interact objective should preserve target id");
	Expect(proposal.behavior.type == iggy::NpcBehaviorStateType::Interacting, "interacting proposal should use Interacting behavior");
	Expect(proposal.behavior.targetId == Id("target:door"), "interact behavior should preserve target id");
	Expect(proposal.moveMode == iggy::NpcMoveMode::Walk, "interacting proposal should use configured move mode");
}

void TestStunnedAndDisabledProjectConfiguredModes()
{
	iggy::NpcPlayControlProposalConfig config;
	config.stunnedMoveMode = iggy::NpcMoveMode::Still;
	config.disabledMoveMode = iggy::NpcMoveMode::Still;

	const iggy::NpcPlayControlProposal stunned = iggy::NpcPlayControlProjector {}.project(
		KeptFold(iggy::NpcBehaviorStateType::Stunned),
		{},
		config);
	const iggy::NpcPlayControlProposal disabled = iggy::NpcPlayControlProjector {}.project(
		KeptFold(iggy::NpcBehaviorStateType::Disabled),
		{},
		config);

	Expect(stunned.status == iggy::NpcPlayControlProposalStatus::Proposed, "stunned play should propose control");
	Expect(stunned.objective.type == iggy::NpcObjectiveType::Wait, "stunned proposal should use Wait objective");
	Expect(stunned.behavior.type == iggy::NpcBehaviorStateType::Stunned, "stunned proposal should preserve Stunned behavior");
	Expect(stunned.moveMode == iggy::NpcMoveMode::Still, "stunned proposal should use configured move mode");
	Expect(disabled.status == iggy::NpcPlayControlProposalStatus::Proposed, "disabled play should propose control");
	Expect(disabled.objective.type == iggy::NpcObjectiveType::None, "disabled proposal should use None objective");
	Expect(disabled.behavior.type == iggy::NpcBehaviorStateType::Disabled, "disabled proposal should preserve Disabled behavior");
	Expect(disabled.moveMode == iggy::NpcMoveMode::Still, "disabled proposal should use configured move mode");
}

void TestCopiedFoldAndSelectedFactsArePreserved()
{
	const iggy::NpcFold fold = KeptFold(iggy::NpcBehaviorStateType::Idle, "action:preserve");

	const iggy::NpcPlayControlProposal proposal = iggy::NpcPlayControlProjector {}.project(fold);

	Expect(proposal.hasProposal(), "preservation setup should propose control");
	Expect(SameFoldSelectedPlay(proposal.fold, fold), "proposal should preserve fold by value");
	Expect(proposal.actionTag == Id("action:preserve"), "proposal should preserve selected action tag");
	Expect(proposal.requestedBehaviorState == fold.tell.play.selected.ent.behaviorState, "proposal should preserve requested behavior state");
}

void TestProjectDoesNotMutateInput()
{
	iggy::NpcFold fold = KeptFold(iggy::NpcBehaviorStateType::Seeking, "action:immutable");
	const iggy::NpcFold before = fold;
	iggy::NpcPlayControlProposalContext context;
	context.hasTargetPosition = true;
	context.targetPosition = { 1.0F, 2.0F };

	const iggy::NpcPlayControlProposal proposal = iggy::NpcPlayControlProjector {}.project(fold, context);

	Expect(proposal.hasProposal(), "immutability setup should propose control");
	Expect(SameFoldSelectedPlay(fold, before), "project should not mutate input fold");
	Expect(SameFoldSelectedPlay(proposal.fold, before), "project should copy fold by value");
}

} // namespace

int main()
{
	TestFoldedPlayReturnsNoKeptPlay();
	TestIdleKeptPlayProjectsToWaitIdleStill();
	TestWaitingKeptPlayProjectsToWaitWaiting();
	TestNoneKeptPlayProjectsToNoneAndNoMove();
	TestSeekingWithoutTargetPositionReportsMissingTargetPosition();
	TestSeekingWithTargetPositionProjectsMoveToSeeking();
	TestFleeingWithTargetPositionProjectsFleeing();
	TestAttackingWithoutTargetIdReportsMissingTargetId();
	TestAttackingWithTargetIdProjectsAttack();
	TestInteractingWithTargetIdProjectsInteract();
	TestStunnedAndDisabledProjectConfiguredModes();
	TestCopiedFoldAndSelectedFactsArePreserved();
	TestProjectDoesNotMutateInput();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
