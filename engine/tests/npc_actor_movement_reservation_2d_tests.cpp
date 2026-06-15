#include <cstdlib>
#include <vector>

#include "scene/npc/NpcActorMovementReservation2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId(value);
}

iggy::NpcActorPathStepOccupancyFilter2D Filter(
	const char *npcId,
	iggy::TileCoord tile,
	iggy::NpcActorPathStepOccupancyFilter2DStatus status = iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed)
{
	iggy::NpcActorPathStepOccupancyFilter2D filter;
	filter.status = status;
	filter.requestsMovement = status == iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed;
	filter.step.status = filter.requestsMovement ? iggy::NpcActorPathStep2DStatus::Proposed : iggy::NpcActorPathStep2DStatus::NoPath;
	filter.step.npcId = Id(npcId);
	filter.step.oldPosition = { 0.5F, 0.5F };
	filter.step.proposedPosition = iggy::tileCenter(tile);
	filter.step.oldTile = { 0, 0 };
	filter.step.proposedTile = tile;
	filter.step.moveMode = iggy::NpcMoveMode::Walk;
	filter.step.requestsMovement = filter.requestsMovement;
	return filter;
}

iggy::NpcActorMovementFrameApply2DRequest Request(
	const char *npcId,
	iggy::TileCoord tile,
	iggy::NpcActorPathStepOccupancyFilter2DStatus status = iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed)
{
	return { Filter(npcId, tile, status) };
}

void TestEmptyReservations()
{
	const iggy::NpcActorMovementReservation2DResult result =
		iggy::NpcActorMovementReservationProjector2D {}.reserve({});

	Expect(result.status == iggy::NpcActorMovementReservation2DStatus::NoAcceptedRequests, "empty reservation should have no accepted requests");
	Expect(result.entries.empty(), "empty reservation should have no entries");
	Expect(result.requestCount == 0 && result.acceptedCount == 0 && result.rejectedCount == 0, "empty reservation counts should be zero");
}

void TestDefaultNoSharingAllowsFirstRejectsSecond()
{
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> requests {
		Request("npc:first", { 1, 0 }),
		Request("npc:second", { 1, 0 }),
	};

	const iggy::NpcActorMovementReservation2DResult result =
		iggy::NpcActorMovementReservationProjector2D {}.reserve(requests);

	Expect(result.status == iggy::NpcActorMovementReservation2DStatus::Reserved, "reservation with accepted request should be reserved");
	Expect(result.acceptedRequests.size() == 1, "default reservation should accept first same-tile movement only");
	Expect(result.entries.size() == 2, "reservation should preserve one entry per request");
	Expect(result.entries[0].status == iggy::NpcActorMovementReservationEntry2DStatus::Accepted, "first request should be accepted");
	Expect(result.entries[1].status == iggy::NpcActorMovementReservationEntry2DStatus::ReservationBlocked, "second different npc should be reservation-blocked");
	Expect(result.entries[1].blockingNpcId == Id("npc:first"), "reservation block should preserve first claiming npc");
	Expect(result.entries[1].blockingRequestIndex.has_value() && *result.entries[1].blockingRequestIndex == 0, "reservation block should preserve blocking request index");
}

void TestCapacityTwoAllowsTwoDifferentNpcs()
{
	iggy::NpcActorMovementReservation2DConfig config;
	config.policy.maxOccupantsPerTile = 2;
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> requests {
		Request("npc:first", { 1, 0 }),
		Request("npc:second", { 1, 0 }),
		Request("npc:third", { 1, 0 }),
	};

	const iggy::NpcActorMovementReservation2DResult result =
		iggy::NpcActorMovementReservationProjector2D {}.reserve(requests, config);

	Expect(result.acceptedRequests.size() == 2, "capacity two should accept two same-tile requests");
	Expect(result.reservationBlockedCount == 1, "capacity two should block third same-tile request");
	Expect(result.entries[2].blockingNpcId == Id("npc:first"), "capacity full should preserve first claimant as blocker");
}

void TestSameNpcDuplicateIsAllowedDeterministically()
{
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> requests {
		Request("npc:mover", { 1, 0 }),
		Request("npc:mover", { 1, 0 }),
	};

	const iggy::NpcActorMovementReservation2DResult result =
		iggy::NpcActorMovementReservationProjector2D {}.reserve(requests);

	Expect(result.acceptedRequests.size() == 2, "same npc duplicate requests should stay accepted for sequential apply semantics");
	Expect(result.reservationBlockedCount == 0, "same npc duplicate requests should not reservation block each other");
	Expect(result.entries[1].acceptedRequestIndex.has_value() && *result.entries[1].acceptedRequestIndex == 1, "same npc duplicate should preserve accepted request order");
}

void TestNonMovementRequestsPassThroughWithoutClaim()
{
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> requests {
		Request("npc:blocked", { 1, 0 }, iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc),
		Request("npc:first", { 1, 0 }),
	};

	const iggy::NpcActorMovementReservation2DResult result =
		iggy::NpcActorMovementReservationProjector2D {}.reserve(requests);

	Expect(result.acceptedRequests.size() == 2, "non-movement diagnostics should pass through reservation");
	Expect(result.movementRequestCount == 1, "non-movement diagnostics should not count as movement claim");
	Expect(result.reservedMovementCount == 1, "allowed request should claim destination after non-movement pass-through");
	Expect(result.reservationBlockedCount == 0, "non-movement request should not block later movement reservation");
}

void TestInputRequestsAreNotMutated()
{
	std::vector<iggy::NpcActorMovementFrameApply2DRequest> requests {
		Request("npc:first", { 1, 0 }),
		Request("npc:second", { 1, 0 }),
	};
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> before = requests;

	const iggy::NpcActorMovementReservation2DResult result =
		iggy::NpcActorMovementReservationProjector2D {}.reserve(requests);

	Expect(result.reservationBlockedCount == 1, "immutability setup should block one request");
	Expect(requests[0].filter.step.npcId == before[0].filter.step.npcId, "reservation should not mutate first request");
	Expect(requests[1].filter.step.proposedTile == before[1].filter.step.proposedTile, "reservation should not mutate second request");
}

} // namespace

int main()
{
	TestEmptyReservations();
	TestDefaultNoSharingAllowsFirstRejectsSecond();
	TestCapacityTwoAllowsTwoDifferentNpcs();
	TestSameNpcDuplicateIsAllowedDeterministically();
	TestNonMovementRequestsPassThroughWithoutClaim();
	TestInputRequestsAreNotMutated();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
