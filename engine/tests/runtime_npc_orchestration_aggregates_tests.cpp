#include <cstdlib>

#include "runtime/RuntimeNpcOrchestrationAggregates.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

void TestEmptyAggregates()
{
	const iggy::runtime::RuntimeNpcMovementAggregate movement;
	const iggy::runtime::RuntimeNpcControlAggregate control;
	const iggy::runtime::RuntimeNpcRefreshAggregate refresh;

	Expect(!movement.hasMovementFacts(), "empty movement aggregate should have no movement facts");
	Expect(!movement.needsAnyRefresh(), "empty movement aggregate should have no refresh flags");
	Expect(!control.hasControlFacts(), "empty control aggregate should have no control facts");
	Expect(!refresh.refreshedAny(), "empty refresh aggregate should have no refresh flags");
}

void TestMovementResultFold()
{
	iggy::runtime::RuntimeNpcActorMovementPlannedFrameResult first;
	first.plannedRequestCount = 2;
	first.preReservationRequestCount = 3;
	first.reservationAcceptedCount = 1;
	first.reservationRejectedCount = 2;
	first.movedCount = 1;
	first.blockedCount = 1;
	first.rejectedCount = 1;
	first.missingActorCount = 1;
	first.changed = true;
	first.movement.report.dirtyTiles = { { 1, 2 }, { 2, 2 } };
	first.movement.report.needsOccupancyRebuild = true;
	first.movement.report.needsRenderRefresh = true;

	iggy::runtime::RuntimeNpcActorMovementPlannedFrameResult second;
	second.plannedRequestCount = 1;
	second.preReservationRequestCount = 1;
	second.reservationAcceptedCount = 1;
	second.movedCount = 2;
	second.movement.report.dirtyTiles = { { 3, 2 } };
	second.movement.report.needsAiMapQueryRefresh = true;
	second.movement.report.needsInteractionRefresh = true;
	second.movement.report.needsVisibilityRefresh = true;

	const iggy::runtime::RuntimeNpcActorMovementPlannedFrameResult before = first;
	iggy::runtime::RuntimeNpcMovementAggregate aggregate;
	foldRuntimeNpcMovementAggregate(aggregate, first);
	foldRuntimeNpcMovementAggregate(aggregate, second);

	Expect(aggregate.plannedRequestCount == 3, "movement aggregate should sum planned requests");
	Expect(aggregate.preReservationRequestCount == 4, "movement aggregate should sum pre-reservation requests");
	Expect(aggregate.reservationAcceptedCount == 2, "movement aggregate should sum accepted reservations");
	Expect(aggregate.reservationRejectedCount == 2, "movement aggregate should sum rejected reservations");
	Expect(aggregate.movedCount == 3, "movement aggregate should sum moved count");
	Expect(aggregate.blockedMovementCount == 1, "movement aggregate should sum blocked count");
	Expect(aggregate.rejectedMovementCount == 1, "movement aggregate should sum rejected count");
	Expect(aggregate.missingActorMovementCount == 1, "movement aggregate should sum missing actor count");
	Expect(aggregate.dirtyTileCount == 3, "movement aggregate should sum dirty tiles");
	Expect(aggregate.actorsChanged, "movement aggregate should OR actor changed flag");
	Expect(aggregate.needsAnyRefresh(), "movement aggregate should OR refresh flags");
	Expect(aggregate.needsOccupancyRefresh && aggregate.needsAiMapRefresh, "movement aggregate should preserve occupancy and AI map refresh flags");
	Expect(aggregate.needsInteractionRefresh && aggregate.needsRenderRefresh && aggregate.needsVisibilityRefresh, "movement aggregate should preserve remaining refresh flags");
	Expect(first.plannedRequestCount == before.plannedRequestCount && first.movement.report.dirtyTiles.size() == before.movement.report.dirtyTiles.size(), "movement fold should not mutate source result");
}

void TestAiMovementFold()
{
	iggy::runtime::RuntimeNpcAiMovementPlannedFrameResult result;
	result.controlPlannedRequestCount = 2;
	result.controlAppliedCount = 1;
	result.controlFailedCount = 1;
	result.controlMapChangedSelectionCount = 1;
	result.changedControls = true;
	result.movementPlannedRequestCount = 3;
	result.movementPreReservationRequestCount = 4;
	result.movementReservationAcceptedCount = 2;
	result.movementReservationRejectedCount = 1;
	result.movedCount = 2;
	result.blockedCount = 1;
	result.rejectedCount = 1;
	result.missingActorCount = 1;
	result.changedActors = true;
	result.movement.movement.report.dirtyTiles = { { 4, 4 } };
	result.movement.movement.report.needsVisibilityRefresh = true;

	iggy::runtime::RuntimeNpcControlAggregate control;
	iggy::runtime::RuntimeNpcMovementAggregate movement;
	foldRuntimeNpcControlAggregate(control, result);
	foldRuntimeNpcMovementAggregate(movement, result);

	Expect(control.plannedRequestCount == 2 && control.appliedCount == 1, "AI movement fold should preserve control planned/applied counts");
	Expect(control.failedCount == 1 && control.mapChangedSelectionCount == 1, "AI movement fold should preserve control failed/map-changed counts");
	Expect(control.controlsChanged, "AI movement fold should preserve controls changed flag");
	Expect(movement.plannedRequestCount == 3 && movement.preReservationRequestCount == 4, "AI movement fold should preserve movement planning counts");
	Expect(movement.reservationAcceptedCount == 2 && movement.reservationRejectedCount == 1, "AI movement fold should preserve reservation counts");
	Expect(movement.movedCount == 2 && movement.blockedMovementCount == 1, "AI movement fold should preserve movement outcomes");
	Expect(movement.rejectedMovementCount == 1 && movement.missingActorMovementCount == 1, "AI movement fold should preserve rejected/missing outcomes");
	Expect(movement.dirtyTileCount == 1 && movement.needsVisibilityRefresh, "AI movement fold should preserve dirty/refresh facts");
}

void TestRefreshFold()
{
	iggy::runtime::RuntimeNpcAiMovementRefreshFrameResult result;
	result.controlPlannedRequestCount = 1;
	result.controlAppliedCount = 1;
	result.movementPlannedRequestCount = 1;
	result.movedCount = 1;
	result.dirtyTileCount = 2;
	result.changedControls = true;
	result.changedActors = true;
	result.occupancyRefreshed = true;
	result.interactionRefreshed = true;
	result.aiMapRefreshed = true;
	result.renderRefreshed = true;
	result.visibilityRefreshed = true;

	iggy::runtime::RuntimeNpcControlAggregate control;
	iggy::runtime::RuntimeNpcMovementAggregate movement;
	iggy::runtime::RuntimeNpcRefreshAggregate refresh;
	foldRuntimeNpcControlAggregate(control, result);
	foldRuntimeNpcMovementAggregate(movement, result);
	foldRuntimeNpcRefreshAggregate(refresh, result);

	Expect(control.plannedRequestCount == 1 && control.appliedCount == 1, "refresh fold should preserve control counts");
	Expect(movement.plannedRequestCount == 1 && movement.movedCount == 1, "refresh fold should preserve movement counts");
	Expect(movement.dirtyTileCount == 2 && movement.actorsChanged, "refresh fold should preserve movement dirty/changed facts");
	Expect(refresh.dirtyTileCount == 2, "refresh aggregate should preserve dirty tile count");
	Expect(refresh.occupancyRefreshCount == 1 && refresh.interactionRefreshCount == 1, "refresh aggregate should count occupancy/interaction refreshes");
	Expect(refresh.aiMapRefreshCount == 1 && refresh.renderRefreshCount == 1 && refresh.visibilityRefreshCount == 1, "refresh aggregate should count AI map/render/visibility refreshes");
	Expect(refresh.refreshedAny(), "refresh aggregate should report any refresh");
}

void TestOrchestratedReportFold()
{
	iggy::runtime::RuntimeGameplayOrchestratedFrameReport report;
	report.npcControlPlannedRequestCount = 1;
	report.npcControlAppliedCount = 1;
	report.npcMovementPlannedRequestCount = 2;
	report.npcMovedCount = 1;
	report.npcBlockedMovementCount = 1;
	report.npcRefreshDirtyTileCount = 2;
	report.npcControlsChanged = true;
	report.npcActorsChanged = true;
	report.npcOccupancyRefreshed = true;
	report.npcRenderRefreshed = true;

	iggy::runtime::RuntimeNpcControlAggregate control;
	iggy::runtime::RuntimeNpcMovementAggregate movement;
	iggy::runtime::RuntimeNpcRefreshAggregate refresh;
	foldRuntimeNpcControlAggregate(control, report);
	foldRuntimeNpcMovementAggregate(movement, report);
	foldRuntimeNpcRefreshAggregate(refresh, report);

	Expect(control.plannedRequestCount == 1 && control.appliedCount == 1, "orchestrated report fold should preserve control counts");
	Expect(control.controlsChanged, "orchestrated report fold should preserve control changed flag");
	Expect(movement.plannedRequestCount == 2 && movement.movedCount == 1 && movement.blockedMovementCount == 1, "orchestrated report fold should preserve movement counts");
	Expect(movement.dirtyTileCount == 2 && movement.actorsChanged, "orchestrated report fold should preserve dirty/actor facts");
	Expect(refresh.dirtyTileCount == 2 && refresh.occupancyRefreshed && refresh.renderRefreshed, "orchestrated report fold should preserve refresh facts");
}

} // namespace

int main()
{
	TestEmptyAggregates();
	TestMovementResultFold();
	TestAiMovementFold();
	TestRefreshFold();
	TestOrchestratedReportFold();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
