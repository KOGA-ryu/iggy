#include <cstdlib>
#include <vector>

#include "scene/npc/NpcActorMovementReservedFrameApply2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId(value);
}

iggy::NpcActorState2D Actor(const char *npcId, iggy::Vec2 position)
{
	return { Id(npcId), Id("ai-profile:guard"), Id("faction:town"), position, {}, true };
}

iggy::NpcActorState2DRegistry Registry(std::vector<iggy::NpcActorState2D> actors)
{
	iggy::NpcActorState2DRegistry registry;
	registry.actors = std::move(actors);
	return registry;
}

iggy::NpcActorPathStepOccupancyFilter2D AllowedFilter(const char *npcId, iggy::TileCoord tile)
{
	iggy::NpcActorPathStepOccupancyFilter2D filter;
	filter.status = iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed;
	filter.requestsMovement = true;
	filter.step.status = iggy::NpcActorPathStep2DStatus::Proposed;
	filter.step.npcId = Id(npcId);
	filter.step.oldPosition = { 0.5F, 0.5F };
	filter.step.proposedPosition = iggy::tileCenter(tile);
	filter.step.oldTile = { 0, 0 };
	filter.step.proposedTile = tile;
	filter.step.moveMode = iggy::NpcMoveMode::Walk;
	filter.step.requestsMovement = true;
	return filter;
}

iggy::NpcActorPathStepOccupancyFilter2D BlockedFilter(const char *npcId, iggy::TileCoord tile, const char *blocker)
{
	iggy::NpcActorPathStepOccupancyFilter2D filter = AllowedFilter(npcId, tile);
	filter.status = iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc;
	filter.requestsMovement = false;
	filter.blockingNpcId = Id(blocker);
	return filter;
}

iggy::NpcActorMovementFrameApply2DRequest Request(iggy::NpcActorPathStepOccupancyFilter2D filter)
{
	return { filter };
}

void TestDefaultReservationAppliesOnlyFirstSameTileMovement()
{
	const iggy::NpcActorState2DRegistry registry = Registry({
		Actor("npc:first", { 0.5F, 0.5F }),
		Actor("npc:second", { 2.5F, 0.5F }),
	});
	const iggy::NpcActorOccupancy2D occupancy = iggy::NpcActorOccupancyProjector2D {}.project(registry);
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> requests {
		Request(AllowedFilter("npc:first", { 1, 0 })),
		Request(AllowedFilter("npc:second", { 1, 0 })),
	};

	const iggy::NpcActorMovementReservedFrameApply2DResult result =
		iggy::NpcActorMovementReservedFrameApplier2D {}.apply(registry, requests, occupancy);

	Expect(result.reservation.acceptedRequests.size() == 1, "reserved apply should apply only accepted reservation requests");
	Expect(result.reservation.reservationBlockedCount == 1, "reserved apply should preserve reservation rejection diagnostics");
	Expect(result.apply.movedCount == 1, "reserved apply should move first accepted actor");
	Expect(NearVec(result.registry.actors[0].position, { 1.5F, 0.5F }), "first accepted actor should move");
	Expect(NearVec(result.registry.actors[1].position, { 2.5F, 0.5F }), "reservation-blocked actor should not move");
	Expect(result.report.needsOccupancyRebuild, "accepted movement should still produce refresh facts");
}

void TestCapacityTwoAppliesBothSameTileMovements()
{
	const iggy::NpcActorState2DRegistry registry = Registry({
		Actor("npc:first", { 0.5F, 0.5F }),
		Actor("npc:second", { 2.5F, 0.5F }),
	});
	const iggy::NpcActorOccupancy2D occupancy = iggy::NpcActorOccupancyProjector2D {}.project(registry);
	iggy::NpcActorMovementReservedFrameApply2DConfig config;
	config.reservation.policy.maxOccupantsPerTile = 2;

	const iggy::NpcActorMovementReservedFrameApply2DResult result =
		iggy::NpcActorMovementReservedFrameApplier2D {}.apply(
			registry,
			{
				Request(AllowedFilter("npc:first", { 1, 0 })),
				Request(AllowedFilter("npc:second", { 1, 0 })),
			},
			occupancy,
			config);

	Expect(result.reservation.acceptedRequests.size() == 2, "capacity two reserved apply should accept both requests");
	Expect(result.apply.movedCount == 2, "capacity two reserved apply should move both actors");
	Expect(NearVec(result.registry.actors[0].position, { 1.5F, 0.5F }), "first actor should move to shared tile");
	Expect(NearVec(result.registry.actors[1].position, { 1.5F, 0.5F }), "second actor should move to shared tile");
}

void TestBlockedRequestPassesThroughForDiagnostics()
{
	const iggy::NpcActorState2DRegistry registry = Registry({
		Actor("npc:mover", { 0.5F, 0.5F }),
	});
	const iggy::NpcActorOccupancy2D occupancy = iggy::NpcActorOccupancyProjector2D {}.project(registry);

	const iggy::NpcActorMovementReservedFrameApply2DResult result =
		iggy::NpcActorMovementReservedFrameApplier2D {}.apply(
			registry,
			{ Request(BlockedFilter("npc:mover", { 1, 0 }, "npc:blocker")) },
			occupancy);

	Expect(result.reservation.acceptedRequests.size() == 1, "blocked request should pass through reservation for apply diagnostics");
	Expect(result.apply.blockedCount == 1, "blocked request should preserve blocked apply count");
	Expect(!result.changed, "blocked request should not change registry");
	Expect(!result.report.needsOccupancyRebuild, "blocked request should not emit refresh facts");
}

void TestInputsAreNotMutated()
{
	iggy::NpcActorState2DRegistry registry = Registry({
		Actor("npc:first", { 0.5F, 0.5F }),
		Actor("npc:second", { 2.5F, 0.5F }),
	});
	const iggy::NpcActorState2DRegistry before = registry;
	iggy::NpcActorOccupancy2D occupancy = iggy::NpcActorOccupancyProjector2D {}.project(registry);
	const iggy::NpcActorOccupancy2D occupancyBefore = occupancy;
	std::vector<iggy::NpcActorMovementFrameApply2DRequest> requests {
		Request(AllowedFilter("npc:first", { 1, 0 })),
		Request(AllowedFilter("npc:second", { 1, 0 })),
	};
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> requestsBefore = requests;

	const iggy::NpcActorMovementReservedFrameApply2DResult result =
		iggy::NpcActorMovementReservedFrameApplier2D {}.apply(registry, requests, occupancy);

	Expect(result.changed, "immutability setup should move one actor");
	Expect(NearVec(registry.actors[0].position, before.actors[0].position), "reserved apply should not mutate input registry");
	Expect(occupancy.entries.size() == occupancyBefore.entries.size(), "reserved apply should not mutate occupancy");
	Expect(requests[1].filter.step.proposedTile == requestsBefore[1].filter.step.proposedTile, "reserved apply should not mutate requests");
}

} // namespace

int main()
{
	TestDefaultReservationAppliesOnlyFirstSameTileMovement();
	TestCapacityTwoAppliesBothSameTileMovements();
	TestBlockedRequestPassesThroughForDiagnostics();
	TestInputsAreNotMutated();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
