#include "scene/npc/NpcActorMovementReservedFrameApply2D.hpp"

namespace iggy {

bool NpcActorMovementReservedFrameApply2DResult::applied() const
{
	return status == NpcActorMovementReservedFrameApply2DStatus::Applied;
}

NpcActorMovementReservedFrameApply2DResult NpcActorMovementReservedFrameApplier2D::apply(
	const NpcActorState2DRegistry &registry,
	const std::vector<NpcActorMovementFrameApply2DRequest> &requests,
	const NpcActorOccupancy2D &occupancy,
	const NpcActorMovementReservedFrameApply2DConfig &config) const
{
	NpcActorMovementReservedFrameApply2DResult result;
	result.inputRegistry = registry;
	result.occupancy = occupancy;
	result.requestCount = requests.size();

	result.reservation = NpcActorMovementReservationProjector2D {}.reserve(requests, config.reservation);
	result.acceptedRequestCount = result.reservation.acceptedCount;
	result.reservationRejectedCount = result.reservation.rejectedCount;

	result.apply = NpcActorMovementFrameApplier2D {}.apply(registry, result.reservation.acceptedRequests);
	result.report = NpcActorMovementFrameReporter2D {}.report(result.apply);
	result.registry = result.apply.registry;
	result.movedCount = result.apply.movedCount;
	result.blockedCount = result.apply.blockedCount;
	result.changed = result.apply.changed;
	if (result.apply.applied()) {
		result.status = NpcActorMovementReservedFrameApply2DStatus::Applied;
	}
	return result;
}

} // namespace iggy
