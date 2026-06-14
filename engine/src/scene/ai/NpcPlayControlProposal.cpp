#include "scene/ai/NpcPlayControlProposal.hpp"

namespace {

iggy::NpcPlayControlProposal MissingTargetPosition(iggy::NpcPlayControlProposal proposal)
{
	proposal.status = iggy::NpcPlayControlProposalStatus::MissingTargetPosition;
	return proposal;
}

iggy::NpcPlayControlProposal MissingTargetId(iggy::NpcPlayControlProposal proposal)
{
	proposal.status = iggy::NpcPlayControlProposalStatus::MissingTargetId;
	return proposal;
}

iggy::NpcPlayControlProposal Validate(iggy::NpcPlayControlProposal proposal)
{
	if (!iggy::valid(proposal.objective)) {
		proposal.status = iggy::NpcPlayControlProposalStatus::InvalidObjective;
		return proposal;
	}

	if (!iggy::valid(proposal.behavior)) {
		proposal.status = iggy::NpcPlayControlProposalStatus::InvalidBehavior;
		return proposal;
	}

	proposal.status = iggy::NpcPlayControlProposalStatus::Proposed;
	return proposal;
}

} // namespace

namespace iggy {

bool NpcPlayControlProposal::hasProposal() const
{
	return status == NpcPlayControlProposalStatus::Proposed;
}

NpcPlayControlProposal NpcPlayControlProjector::project(
	const NpcFold &fold,
	const NpcPlayControlProposalContext &context,
	const NpcPlayControlProposalConfig &config) const
{
	NpcPlayControlProposal proposal;
	proposal.fold = fold;

	if (!fold.kept() || !fold.tell.play.hasPlay()) {
		proposal.status = NpcPlayControlProposalStatus::NoKeptPlay;
		return proposal;
	}

	const NpcHandEnt &selected = fold.tell.play.selected.ent;
	proposal.actionTag = selected.actionTag;
	proposal.requestedBehaviorState = selected.behaviorState;

	switch (selected.behaviorState) {
	case NpcBehaviorStateType::None:
		proposal.objective = noneNpcObjective();
		proposal.behavior = noneNpcBehaviorState();
		proposal.moveMode = NpcMoveMode::None;
		break;
	case NpcBehaviorStateType::Idle:
		proposal.objective = waitNpcObjective();
		proposal.behavior = idleNpcBehaviorState();
		proposal.moveMode = config.idleMoveMode;
		break;
	case NpcBehaviorStateType::Waiting:
		proposal.objective = waitNpcObjective();
		proposal.behavior = waitingNpcBehaviorState();
		proposal.moveMode = config.waitingMoveMode;
		break;
	case NpcBehaviorStateType::Seeking:
		if (!context.hasTargetPosition)
			return MissingTargetPosition(proposal);
		proposal.objective = moveToNpcObjective(context.targetPosition);
		proposal.behavior = seekingNpcBehaviorState(context.targetPosition);
		proposal.moveMode = config.seekingMoveMode;
		break;
	case NpcBehaviorStateType::Fleeing:
		if (!context.hasTargetPosition)
			return MissingTargetPosition(proposal);
		proposal.objective = fleeNpcObjective(context.targetPosition);
		proposal.behavior = fleeingNpcBehaviorState(context.targetPosition);
		proposal.moveMode = config.fleeingMoveMode;
		break;
	case NpcBehaviorStateType::Attacking:
		if (context.targetId.empty())
			return MissingTargetId(proposal);
		proposal.objective = attackNpcObjective(context.targetId);
		proposal.behavior = attackingNpcBehaviorState(context.targetId);
		proposal.moveMode = config.attackingMoveMode;
		break;
	case NpcBehaviorStateType::Interacting:
		if (context.targetId.empty())
			return MissingTargetId(proposal);
		proposal.objective = interactNpcObjective(context.targetId);
		proposal.behavior = interactingNpcBehaviorState(context.targetId);
		proposal.moveMode = config.interactingMoveMode;
		break;
	case NpcBehaviorStateType::Stunned:
		proposal.objective = waitNpcObjective();
		proposal.behavior = stunnedNpcBehaviorState();
		proposal.moveMode = config.stunnedMoveMode;
		break;
	case NpcBehaviorStateType::Disabled:
		proposal.objective = noneNpcObjective();
		proposal.behavior = disabledNpcBehaviorState();
		proposal.moveMode = config.disabledMoveMode;
		break;
	}

	return Validate(proposal);
}

} // namespace iggy
