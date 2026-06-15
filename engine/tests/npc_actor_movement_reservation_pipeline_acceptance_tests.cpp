#include <cstdlib>
#include <vector>

#include "scene/npc/NpcActorMovementReservedFrameApply2D.hpp"
#include "scene/npc/NpcActorPathStepOccupancyPolicyFilter2D.hpp"
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

iggy::NpcActorMovementFrameApply2DRequest Request(iggy::NpcActorPathStepOccupancyFilter2D filter)
{
	return { filter };
}

iggy::NpcActorPathStepOccupancyFilter2D PolicyFilter(
	const char *npcId,
	iggy::TileCoord tile,
	const iggy::NpcActorOccupancy2D &occupancy,
	const iggy::NpcActorPathStepOccupancyPolicyFilter2DConfig &config = {})
{
	iggy::NpcActorPathStep2D step;
	step.status = iggy::NpcActorPathStep2DStatus::Proposed;
	step.npcId = Id(npcId);
	step.oldPosition = { 0.5F, 0.5F };
	step.proposedPosition = iggy::tileCenter(tile);
	step.oldTile = { 0, 0 };
	step.proposedTile = tile;
	step.moveMode = iggy::NpcMoveMode::Walk;
	step.requestsMovement = true;
	return iggy::NpcActorPathStepOccupancyPolicyFilterProjector2D {}.filter(step, occupancy, config).filter;
}

void TestDefaultReservationAllowsFirstRejectsSecond()
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

	Expect(result.reservation.reservationBlockedCount == 1, "default reservation should reject later same-tile request");
	Expect(result.apply.movedCount == 1, "default reservation should move only first claimant");
	Expect(NearVec(result.registry.actors[0].position, { 1.5F, 0.5F }), "first claimant should move");
	Expect(NearVec(result.registry.actors[1].position, { 2.5F, 0.5F }), "second claimant should remain in place");
	Expect(result.report.movedCount == 1 && result.report.changed(), "frame report should reflect accepted movement");
	Expect(result.report.needsOccupancyRebuild, "accepted movement should request occupancy refresh");
}

void TestCapacityTwoAllowsDuplicateFinalOccupancy()
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
	const iggy::NpcActorOccupancy2D refreshed = iggy::NpcActorOccupancyProjector2D {}.project(result.registry);
	const iggy::NpcActorOccupancyQuery2DResult occupants = npcActorOccupantsAt(refreshed, { 1, 0 });

	Expect(result.apply.movedCount == 2, "capacity two reservation should move both actors");
	Expect(occupants.npcIds.size() == 2, "capacity two final registry should project duplicate occupancy inspectably");
	Expect(refreshed.hasIssues(), "duplicate final occupancy should preserve duplicate tile issue");
}

void TestExistingHardBlockStillReportsBlocked()
{
	const iggy::NpcActorState2DRegistry registry = Registry({
		Actor("npc:mover", { 0.5F, 0.5F }),
		Actor("npc:blocker", { 1.5F, 0.5F }),
	});
	const iggy::NpcActorOccupancy2D occupancy = iggy::NpcActorOccupancyProjector2D {}.project(registry);
	const iggy::NpcActorPathStepOccupancyFilter2D filter = PolicyFilter("npc:mover", { 1, 0 }, occupancy);

	const iggy::NpcActorMovementReservedFrameApply2DResult result =
		iggy::NpcActorMovementReservedFrameApplier2D {}.apply(registry, { Request(filter) }, occupancy);

	Expect(filter.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc, "policy filter default should preserve hard-block behavior");
	Expect(result.apply.blockedCount == 1, "reserved apply should preserve existing blocked diagnostic");
	Expect(!result.changed, "blocked movement should not mutate returned registry");
	Expect(!result.report.needsOccupancyRebuild, "blocked-only movement should not emit refresh facts");
	Expect(filter.blockingNpcId == Id("npc:blocker"), "default hard block should preserve blocker id");
}

void TestSameNpcDuplicateDeterministicLaterWins()
{
	const iggy::NpcActorState2DRegistry registry = Registry({
		Actor("npc:mover", { 0.5F, 0.5F }),
	});
	const iggy::NpcActorOccupancy2D occupancy = iggy::NpcActorOccupancyProjector2D {}.project(registry);

	const iggy::NpcActorMovementReservedFrameApply2DResult result =
		iggy::NpcActorMovementReservedFrameApplier2D {}.apply(
			registry,
			{
				Request(AllowedFilter("npc:mover", { 1, 0 })),
				Request(AllowedFilter("npc:mover", { 2, 0 })),
			},
			occupancy);

	Expect(result.reservation.acceptedRequests.size() == 2, "same npc duplicate requests should be accepted sequentially");
	Expect(result.apply.movedCount == 2, "same npc duplicate accepted requests should both execute");
	Expect(NearVec(result.registry.actors[0].position, { 2.5F, 0.5F }), "later same-npc request should win final position");
}

void TestInputsAreNotMutated()
{
	iggy::NpcActorState2DRegistry registry = Registry({
		Actor("npc:first", { 0.5F, 0.5F }),
		Actor("npc:second", { 2.5F, 0.5F }),
	});
	const iggy::NpcActorState2DRegistry registryBefore = registry;
	iggy::NpcActorOccupancy2D occupancy = iggy::NpcActorOccupancyProjector2D {}.project(registry);
	const iggy::NpcActorOccupancy2D occupancyBefore = occupancy;
	std::vector<iggy::NpcActorMovementFrameApply2DRequest> requests {
		Request(AllowedFilter("npc:first", { 1, 0 })),
		Request(AllowedFilter("npc:second", { 1, 0 })),
	};
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> requestsBefore = requests;

	const iggy::NpcActorMovementReservedFrameApply2DResult result =
		iggy::NpcActorMovementReservedFrameApplier2D {}.apply(registry, requests, occupancy);

	Expect(result.changed, "immutability setup should move first actor");
	Expect(NearVec(registry.actors[0].position, registryBefore.actors[0].position), "reservation pipeline should not mutate registry input");
	Expect(occupancy.entries.size() == occupancyBefore.entries.size(), "reservation pipeline should not mutate occupancy");
	Expect(requests[1].filter.step.npcId == requestsBefore[1].filter.step.npcId, "reservation pipeline should not mutate requests");
}

} // namespace

int main()
{
	TestDefaultReservationAllowsFirstRejectsSecond();
	TestCapacityTwoAllowsDuplicateFinalOccupancy();
	TestExistingHardBlockStillReportsBlocked();
	TestSameNpcDuplicateDeterministicLaterWins();
	TestInputsAreNotMutated();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
