#include <cstdlib>
#include <vector>

#include "scene/npc/NpcActorPathStepOccupancyFilter2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::NpcActorState2D Actor(
	const char *npcId,
	iggy::Vec2 position,
	bool present = true,
	const char *profileId = "ai-profile:guard")
{
	return {
		Id(npcId),
		Id(profileId),
		Id("faction:town"),
		position,
		{},
		present,
	};
}

iggy::NpcActorState2DRegistry Registry(std::vector<iggy::NpcActorState2D> actors)
{
	iggy::NpcActorState2DRegistry registry;
	registry.actors = actors;
	return registry;
}

iggy::NpcActorOccupancy2D Occupancy(std::vector<iggy::NpcActorState2D> actors)
{
	return iggy::NpcActorOccupancyProjector2D {}.project(Registry(actors));
}

iggy::NpcActorPathStep2D Step(
	const char *npcId,
	iggy::TileCoord proposedTile = { 1, 0 },
	bool proposed = true)
{
	iggy::NpcActorPathStep2D step;
	step.status = proposed ? iggy::NpcActorPathStep2DStatus::Proposed : iggy::NpcActorPathStep2DStatus::NoPath;
	step.npcId = Id(npcId);
	step.oldPosition = { 0.5F, 0.5F };
	step.proposedPosition = iggy::tileCenter(proposedTile);
	step.oldTile = { 0, 0 };
	step.proposedTile = proposedTile;
	step.moveMode = iggy::NpcMoveMode::Walk;
	step.requestsMovement = proposed;
	return step;
}

bool SameActor(const iggy::NpcActorState2D &actual, const iggy::NpcActorState2D &expected)
{
	return actual.npcId == expected.npcId
		&& actual.aiProfileId == expected.aiProfileId
		&& actual.factionId == expected.factionId
		&& NearVec(actual.position, expected.position)
		&& actual.currentGoalId == expected.currentGoalId
		&& actual.present == expected.present;
}

bool SameEntry(const iggy::NpcActorOccupancyEntry2D &actual, const iggy::NpcActorOccupancyEntry2D &expected)
{
	return actual.npcId == expected.npcId
		&& actual.tile == expected.tile
		&& actual.actorIndex == expected.actorIndex
		&& SameActor(actual.actor, expected.actor);
}

bool SameOccupancy(const iggy::NpcActorOccupancy2D &actual, const iggy::NpcActorOccupancy2D &expected)
{
	if (actual.status != expected.status
		|| actual.entries.size() != expected.entries.size()
		|| actual.occupiedTiles.size() != expected.occupiedTiles.size()
		|| actual.issues.size() != expected.issues.size()) {
		return false;
	}

	for (std::size_t index = 0; index < actual.entries.size(); ++index) {
		if (!SameEntry(actual.entries[index], expected.entries[index])) {
			return false;
		}
	}

	for (std::size_t index = 0; index < actual.occupiedTiles.size(); ++index) {
		const iggy::NpcActorOccupiedTile2D &left = actual.occupiedTiles[index];
		const iggy::NpcActorOccupiedTile2D &right = expected.occupiedTiles[index];
		if (left.tile != right.tile
			|| left.npcIds != right.npcIds
			|| left.actorIndexes != right.actorIndexes) {
			return false;
		}
	}

	for (std::size_t index = 0; index < actual.issues.size(); ++index) {
		const iggy::NpcActorOccupancyIssue2D &left = actual.issues[index];
		const iggy::NpcActorOccupancyIssue2D &right = expected.issues[index];
		if (left.code != right.code
			|| left.tile != right.tile
			|| left.firstActorIndex != right.firstActorIndex
			|| left.laterActorIndex != right.laterActorIndex
			|| left.occupiedTile.tile != right.occupiedTile.tile
			|| left.occupiedTile.npcIds != right.occupiedTile.npcIds
			|| left.occupiedTile.actorIndexes != right.occupiedTile.actorIndexes) {
			return false;
		}
	}

	return true;
}

bool SameStep(const iggy::NpcActorPathStep2D &actual, const iggy::NpcActorPathStep2D &expected)
{
	return actual.status == expected.status
		&& actual.npcId == expected.npcId
		&& NearVec(actual.oldPosition, expected.oldPosition)
		&& NearVec(actual.proposedPosition, expected.proposedPosition)
		&& actual.oldTile == expected.oldTile
		&& actual.proposedTile == expected.proposedTile
		&& actual.moveMode == expected.moveMode
		&& actual.requestsMovement == expected.requestsMovement
		&& actual.completedPath == expected.completedPath;
}

void TestNoStepProposalReturnsNoStepAndDoesNotQuery()
{
	const iggy::NpcActorPathStep2D step = Step("npc:mover", { 1, 0 }, false);
	const iggy::NpcActorOccupancy2D occupancy = Occupancy({
		Actor("npc:blocker", { 1.5F, 0.5F }),
	});

	const iggy::NpcActorPathStepOccupancyFilter2D result =
		iggy::NpcActorPathStepOccupancyFilterProjector2D {}.filter(step, occupancy);

	Expect(result.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::NoStepProposal, "no-step input should return NoStepProposal");
	Expect(!result.requestsMovement, "no-step input should not request movement");
	Expect(!result.allowed(), "no-step input should not be allowed");
	Expect(result.occupancy.status == iggy::NpcActorOccupancyBlock2DStatus::Empty, "no-step input should not need an occupancy block query");
	Expect(SameStep(result.step, step), "no-step filter should preserve copied step");
}

void TestProposedStepToEmptyTileIsAllowed()
{
	const iggy::NpcActorPathStep2D step = Step("npc:mover", { 2, 0 });
	const iggy::NpcActorOccupancy2D occupancy = Occupancy({
		Actor("npc:other", { 0.5F, 0.5F }),
	});

	const iggy::NpcActorPathStepOccupancyFilter2D result =
		iggy::NpcActorPathStepOccupancyFilterProjector2D {}.filter(step, occupancy);

	Expect(result.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed, "empty proposed tile should be allowed");
	Expect(result.allowed(), "empty proposed tile should report allowed");
	Expect(result.requestsMovement, "allowed result should request movement");
	Expect(result.occupancy.status == iggy::NpcActorOccupancyBlock2DStatus::Empty, "empty proposed tile should preserve empty block status");
	Expect(result.step.proposedTile == iggy::TileCoord { 2, 0 }, "allowed result should preserve proposed tile");
	Expect(NearVec(result.step.proposedPosition, { 2.5F, 0.5F }), "allowed result should preserve proposed position");
}

void TestProposedStepToSelfOnlyTileIsAllowed()
{
	const iggy::NpcActorPathStep2D step = Step("npc:mover", { 1, 0 });
	const iggy::NpcActorOccupancy2D occupancy = Occupancy({
		Actor("npc:mover", { 1.5F, 0.5F }),
	});

	const iggy::NpcActorPathStepOccupancyFilter2D result =
		iggy::NpcActorPathStepOccupancyFilterProjector2D {}.filter(step, occupancy);

	Expect(result.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed, "self-only proposed tile should be allowed");
	Expect(result.allowed(), "self-only proposed tile should report allowed");
	Expect(result.occupancy.status == iggy::NpcActorOccupancyBlock2DStatus::OnlySelf, "self-only proposed tile should preserve OnlySelf query result");
	Expect(result.blockingNpcId.empty(), "self-only proposed tile should not preserve a blocking id");
}

void TestProposedStepBlockedByOtherNpc()
{
	const iggy::NpcActorPathStep2D step = Step("npc:mover", { 1, 0 });
	const iggy::NpcActorOccupancy2D occupancy = Occupancy({
		Actor("npc:blocker", { 1.5F, 0.5F }),
	});

	const iggy::NpcActorPathStepOccupancyFilter2D result =
		iggy::NpcActorPathStepOccupancyFilterProjector2D {}.filter(step, occupancy);

	Expect(result.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc, "other occupant should block proposed step");
	Expect(!result.allowed(), "blocked result should not be allowed");
	Expect(!result.requestsMovement, "blocked result should not request movement");
	Expect(result.occupancy.status == iggy::NpcActorOccupancyBlock2DStatus::Blocked, "blocked result should preserve block query status");
	Expect(result.blockingNpcId == Id("npc:blocker"), "blocked result should preserve first blocking npc id");
}

void TestDuplicateSameTileSelfOnlyAllowedButMixedBlocks()
{
	const iggy::NpcActorPathStep2D step = Step("npc:mover", { 1, 0 });
	const iggy::NpcActorOccupancy2D repeatedSelf = Occupancy({
		Actor("npc:mover", { 1.25F, 0.25F }),
		Actor("npc:mover", { 1.75F, 0.75F }),
	});
	const iggy::NpcActorOccupancy2D mixed = Occupancy({
		Actor("npc:mover", { 1.25F, 0.25F }),
		Actor("npc:blocker", { 1.75F, 0.75F }),
	});

	const iggy::NpcActorPathStepOccupancyFilter2D selfResult =
		iggy::NpcActorPathStepOccupancyFilterProjector2D {}.filter(step, repeatedSelf);
	const iggy::NpcActorPathStepOccupancyFilter2D mixedResult =
		iggy::NpcActorPathStepOccupancyFilterProjector2D {}.filter(step, mixed);

	Expect(repeatedSelf.hasIssues(), "repeated self occupancy should preserve projection issue");
	Expect(selfResult.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed, "duplicate same-id occupancy should be allowed");
	Expect(selfResult.occupancy.occupancy.npcIds.size() == 2, "duplicate same-id result should preserve all occupants");
	Expect(mixed.hasIssues(), "mixed duplicate occupancy should preserve projection issue");
	Expect(mixedResult.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc, "mixed duplicate occupancy should block");
	Expect(mixedResult.blockingNpcId == Id("npc:blocker"), "mixed duplicate occupancy should report first different occupant");
}

void TestEmptyMovingNpcIdOnOccupiedTileBlocks()
{
	iggy::NpcActorPathStep2D step = Step("", { 1, 0 });
	step.npcId = {};
	const iggy::NpcActorOccupancy2D occupancy = Occupancy({
		Actor("npc:blocker", { 1.5F, 0.5F }),
	});

	const iggy::NpcActorPathStepOccupancyFilter2D result =
		iggy::NpcActorPathStepOccupancyFilterProjector2D {}.filter(step, occupancy);

	Expect(result.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc, "empty moving npc id should block on occupied tile");
	Expect(result.blockingNpcId == Id("npc:blocker"), "empty moving npc id should preserve first occupant as blocker");
	Expect(result.occupancy.movingNpcId.empty(), "empty moving npc id should be preserved in occupancy query");
}

void TestNamespacedAndUnqualifiedIdsAreDistinct()
{
	const iggy::NpcActorPathStep2D unqualifiedStep = Step("guard", { 1, 0 });
	const iggy::NpcActorPathStep2D namespacedStep = Step("npc:guard", { 1, 0 });
	const iggy::NpcActorOccupancy2D occupancy = Occupancy({
		Actor("guard", { 1.5F, 0.5F }),
	});

	const iggy::NpcActorPathStepOccupancyFilter2D unqualified =
		iggy::NpcActorPathStepOccupancyFilterProjector2D {}.filter(unqualifiedStep, occupancy);
	const iggy::NpcActorPathStepOccupancyFilter2D namespaced =
		iggy::NpcActorPathStepOccupancyFilterProjector2D {}.filter(namespacedStep, occupancy);

	Expect(unqualified.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed, "exact unqualified self should be allowed");
	Expect(namespaced.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc, "namespaced id should differ from unqualified occupant");
	Expect(namespaced.blockingNpcId == Id("guard"), "namespaced moving id should report unqualified occupant as blocker");
}

void TestOccupancyIssuesDoNotPreventFiltering()
{
	const iggy::NpcActorPathStep2D step = Step("npc:third", { 2, 2 });
	const iggy::NpcActorOccupancy2D occupancy = Occupancy({
		Actor("npc:first", { 2.25F, 2.25F }),
		Actor("npc:second", { 2.75F, 2.75F }),
	});

	const iggy::NpcActorPathStepOccupancyFilter2D result =
		iggy::NpcActorPathStepOccupancyFilterProjector2D {}.filter(step, occupancy);

	Expect(occupancy.hasIssues(), "duplicate occupancy setup should have projection issue");
	Expect(result.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc, "occupancy issue should not prevent filtering");
	Expect(result.blockingNpcId == Id("npc:first"), "filter should preserve first blocking occupant in actor order");
	Expect(result.occupancy.occupancy.npcIds.size() == 2, "filter should preserve duplicate occupant query details");
}

void TestInputsAreNotMutated()
{
	iggy::NpcActorPathStep2D step = Step("npc:mover", { 1, 0 });
	iggy::NpcActorOccupancy2D occupancy = Occupancy({
		Actor("npc:blocker", { 1.5F, 0.5F }),
		Actor("npc:other", { 2.5F, 0.5F }),
	});
	const iggy::NpcActorPathStep2D stepBefore = step;
	const iggy::NpcActorOccupancy2D occupancyBefore = occupancy;

	const iggy::NpcActorPathStepOccupancyFilter2D result =
		iggy::NpcActorPathStepOccupancyFilterProjector2D {}.filter(step, occupancy);

	Expect(result.status == iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc, "immutability setup should block");
	Expect(SameStep(step, stepBefore), "occupancy filter should not mutate step input");
	Expect(SameOccupancy(occupancy, occupancyBefore), "occupancy filter should not mutate occupancy input");
}

} // namespace

int main()
{
	TestNoStepProposalReturnsNoStepAndDoesNotQuery();
	TestProposedStepToEmptyTileIsAllowed();
	TestProposedStepToSelfOnlyTileIsAllowed();
	TestProposedStepBlockedByOtherNpc();
	TestDuplicateSameTileSelfOnlyAllowedButMixedBlocks();
	TestEmptyMovingNpcIdOnOccupiedTileBlocks();
	TestNamespacedAndUnqualifiedIdsAreDistinct();
	TestOccupancyIssuesDoNotPreventFiltering();
	TestInputsAreNotMutated();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
