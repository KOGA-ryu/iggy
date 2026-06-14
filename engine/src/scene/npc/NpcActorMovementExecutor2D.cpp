#include "scene/npc/NpcActorMovementExecutor2D.hpp"

namespace {

iggy::NpcActorPostMoveReport2D RejectedReport(const iggy::NpcActorState2D &actor)
{
	return iggy::NpcActorPostMoveReporter2D {}.report(
		actor.npcId,
		actor.position,
		actor.position,
		iggy::NpcActorPostMoveReport2DStatus::Rejected,
		iggy::NpcActorPostMoveBlockingKind2D::InvalidStep);
}

} // namespace

namespace iggy {

bool NpcActorMovementExecutor2DResult::moved() const
{
	return status == NpcActorMovementExecutor2DStatus::Moved && changed;
}

NpcActorMovementExecutor2DResult NpcActorMovementExecutor2D::execute(
	const NpcActorState2D &actor,
	const NpcActorPathStepOccupancyFilter2D &filter) const
{
	NpcActorMovementExecutor2DResult result;
	result.actor = actor;
	result.filter = filter;
	result.resultActor = actor;

	if (!actor.present) {
		result.status = NpcActorMovementExecutor2DStatus::ActorNotPresent;
		result.postMove = RejectedReport(actor);
		return result;
	}

	if (filter.step.npcId != actor.npcId) {
		result.status = NpcActorMovementExecutor2DStatus::ActorMismatch;
		result.postMove = RejectedReport(actor);
		return result;
	}

	if (filter.status == NpcActorPathStepOccupancyFilter2DStatus::Allowed && filter.requestsMovement) {
		result.status = NpcActorMovementExecutor2DStatus::Moved;
		result.resultActor.position = filter.step.proposedPosition;
		result.changed = true;
		result.postMove = NpcActorPostMoveReporter2D {}.report(
			actor.npcId,
			actor.position,
			result.resultActor.position,
			NpcActorPostMoveReport2DStatus::Moved);
		return result;
	}

	if (filter.status == NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc) {
		result.status = NpcActorMovementExecutor2DStatus::BlockedByNpc;
		result.postMove = NpcActorPostMoveReporter2D {}.report(
			actor.npcId,
			actor.position,
			actor.position,
			NpcActorPostMoveReport2DStatus::Blocked,
			NpcActorPostMoveBlockingKind2D::Npc,
			filter.blockingNpcId);
		return result;
	}

	if (filter.status == NpcActorPathStepOccupancyFilter2DStatus::NoStepProposal) {
		result.status = NpcActorMovementExecutor2DStatus::NoMovement;
		result.postMove = NpcActorPostMoveReporter2D {}.report(
			actor.npcId,
			actor.position,
			actor.position,
			NpcActorPostMoveReport2DStatus::NotMoved);
		return result;
	}

	result.status = NpcActorMovementExecutor2DStatus::Rejected;
	result.postMove = RejectedReport(actor);
	return result;
}

} // namespace iggy
