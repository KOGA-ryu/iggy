#include <cstdlib>
#include <vector>

#include "scene/npc/NpcActorPathStepOccupancyPolicyFilter2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId(value);
}

iggy::NpcActorState2D Actor(const char *npcId, iggy::Vec2 position)
{
	return { Id(npcId), Id("ai-profile:guard"), Id("faction:town"), position, {}, true };
}

iggy::NpcActorOccupancy2D Occupancy(std::vector<iggy::NpcActorState2D> actors)
{
	iggy::NpcActorState2DRegistry registry;
	registry.actors = std::move(actors);
	return iggy::NpcActorOccupancyProjector2D {}.project(registry);
}

iggy::NpcActorPathStep2D Step(const char *npcId, iggy::TileCoord tile = { 1, 0 }, bool proposed = true)
{
	iggy::NpcActorPathStep2D step;
	step.status = proposed ? iggy::NpcActorPathStep2DStatus::Proposed : iggy::NpcActorPathStep2DStatus::NoPath;
	step.npcId = Id(npcId);
	step.oldPosition = { 0.5F, 0.5F };
	step.proposedPosition = iggy::tileCenter(tile);
	step.oldTile = { 0, 0 };
	step.proposedTile = tile;
	step.moveMode = iggy::NpcMoveMode::Walk;
	step.requestsMovement = proposed;
	return step;
}

void TestNoStepProposalReturnsNoMovement()
{
	const iggy::NpcActorPathStepOccupancyPolicyFilter2D result =
		iggy::NpcActorPathStepOccupancyPolicyFilterProjector2D {}.filter(
			Step("npc:mover", { 1, 0 }, false),
			Occupancy({ Actor("npc:blocker", { 1.5F, 0.5F }) }));

	Expect(result.status == iggy::NpcActorPathStepOccupancyPolicyFilter2DStatus::NoStepProposal, "policy filter should preserve no-step status");
	Expect(!result.requestsMovement, "policy filter no-step should not request movement");
	Expect(result.filter.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::NoStepProposal, "policy filter should expose no-step executor filter");
}

void TestDefaultHardBlockMatchesExistingFilter()
{
	const iggy::NpcActorPathStep2D step = Step("npc:mover");
	const iggy::NpcActorOccupancy2D occupancy = Occupancy({ Actor("npc:blocker", { 1.5F, 0.5F }) });

	const iggy::NpcActorPathStepOccupancyFilter2D oldFilter =
		iggy::NpcActorPathStepOccupancyFilterProjector2D {}.filter(step, occupancy);
	const iggy::NpcActorPathStepOccupancyPolicyFilter2D policy =
		iggy::NpcActorPathStepOccupancyPolicyFilterProjector2D {}.filter(step, occupancy);

	Expect(oldFilter.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc, "existing filter setup should block");
	Expect(policy.status == iggy::NpcActorPathStepOccupancyPolicyFilter2DStatus::BlockedByNpc, "default policy filter should hard-block different npc");
	Expect(policy.filter.status == oldFilter.status, "default policy filter should expose compatible old filter status");
	Expect(policy.blockingNpcId == Id("npc:blocker"), "policy filter should preserve blocker id");
}

void TestCapacityTwoAllowsDifferentOccupant()
{
	iggy::NpcActorPathStepOccupancyPolicyFilter2DConfig config;
	config.policy.maxOccupantsPerTile = 2;

	const iggy::NpcActorPathStepOccupancyPolicyFilter2D result =
		iggy::NpcActorPathStepOccupancyPolicyFilterProjector2D {}.filter(
			Step("npc:mover"),
			Occupancy({ Actor("npc:other", { 1.5F, 0.5F }) }),
			config);

	Expect(result.status == iggy::NpcActorPathStepOccupancyPolicyFilter2DStatus::Allowed, "capacity two should allow entering one-occupant tile");
	Expect(result.allowed(), "capacity two allowed result should request movement");
	Expect(result.filter.allowed(), "capacity two should expose allowed executor filter");
	Expect(result.policy.occupancyCount == 1, "capacity two should preserve existing occupancy count");
	Expect(result.policy.capacity == 2, "capacity two should preserve policy capacity");
}

void TestCapacityFullBlocks()
{
	iggy::NpcActorPathStepOccupancyPolicyFilter2DConfig config;
	config.policy.maxOccupantsPerTile = 2;

	const iggy::NpcActorPathStepOccupancyPolicyFilter2D result =
		iggy::NpcActorPathStepOccupancyPolicyFilterProjector2D {}.filter(
			Step("npc:third"),
			Occupancy({
				Actor("npc:first", { 1.5F, 0.5F }),
				Actor("npc:second", { 1.25F, 0.25F }),
			}),
			config);

	Expect(result.status == iggy::NpcActorPathStepOccupancyPolicyFilter2DStatus::BlockedByNpc, "capacity full should block entering third npc");
	Expect(!result.requestsMovement, "capacity full should not request movement");
	Expect(result.filter.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc, "capacity full should expose blocked executor filter");
	Expect(result.blockingNpcId == Id("npc:first"), "capacity full should preserve first blocker");
}

void TestSelfOnlyAndEmptyMovingId()
{
	const iggy::NpcActorPathStepOccupancyPolicyFilter2D self =
		iggy::NpcActorPathStepOccupancyPolicyFilterProjector2D {}.filter(
			Step("npc:mover"),
			Occupancy({ Actor("npc:mover", { 1.5F, 0.5F }) }));
	iggy::NpcActorPathStep2D emptyStep = Step("");
	emptyStep.npcId = {};
	const iggy::NpcActorPathStepOccupancyPolicyFilter2D empty =
		iggy::NpcActorPathStepOccupancyPolicyFilterProjector2D {}.filter(
			emptyStep,
			Occupancy({ Actor("npc:blocker", { 1.5F, 0.5F }) }));

	Expect(self.allowed(), "self-only policy filter should allow movement");
	Expect(self.filter.allowed(), "self-only policy filter should expose allowed executor filter");
	Expect(empty.status == iggy::NpcActorPathStepOccupancyPolicyFilter2DStatus::BlockedByNpc, "empty moving id should block on occupied tile");
	Expect(empty.blockingNpcId == Id("npc:blocker"), "empty moving id should preserve blocker");
}

void TestInputsAreNotMutated()
{
	iggy::NpcActorPathStep2D step = Step("npc:mover");
	iggy::NpcActorOccupancy2D occupancy = Occupancy({ Actor("npc:blocker", { 1.5F, 0.5F }) });
	const iggy::NpcActorPathStep2D stepBefore = step;
	const iggy::NpcActorOccupancy2D occupancyBefore = occupancy;

	const iggy::NpcActorPathStepOccupancyPolicyFilter2D result =
		iggy::NpcActorPathStepOccupancyPolicyFilterProjector2D {}.filter(step, occupancy);

	Expect(result.status == iggy::NpcActorPathStepOccupancyPolicyFilter2DStatus::BlockedByNpc, "immutability setup should block");
	Expect(step.npcId == stepBefore.npcId && step.proposedTile == stepBefore.proposedTile, "policy filter should not mutate step");
	Expect(occupancy.entries.size() == occupancyBefore.entries.size(), "policy filter should not mutate occupancy entries");
	Expect(occupancy.occupiedTiles.size() == occupancyBefore.occupiedTiles.size(), "policy filter should not mutate occupied tiles");
}

} // namespace

int main()
{
	TestNoStepProposalReturnsNoMovement();
	TestDefaultHardBlockMatchesExistingFilter();
	TestCapacityTwoAllowsDifferentOccupant();
	TestCapacityFullBlocks();
	TestSelfOnlyAndEmptyMovingId();
	TestInputsAreNotMutated();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
