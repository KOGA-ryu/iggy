#include <cstdlib>
#include <vector>

#include "scene/npc/NpcActorOccupancyQuery2D.hpp"
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
		const iggy::NpcActorOccupiedTile2D &actualTile = actual.occupiedTiles[index];
		const iggy::NpcActorOccupiedTile2D &expectedTile = expected.occupiedTiles[index];
		if (actualTile.tile != expectedTile.tile
			|| actualTile.npcIds != expectedTile.npcIds
			|| actualTile.actorIndexes != expectedTile.actorIndexes) {
			return false;
		}
	}

	for (std::size_t index = 0; index < actual.issues.size(); ++index) {
		const iggy::NpcActorOccupancyIssue2D &actualIssue = actual.issues[index];
		const iggy::NpcActorOccupancyIssue2D &expectedIssue = expected.issues[index];
		if (actualIssue.code != expectedIssue.code
			|| actualIssue.tile != expectedIssue.tile
			|| actualIssue.firstActorIndex != expectedIssue.firstActorIndex
			|| actualIssue.laterActorIndex != expectedIssue.laterActorIndex
			|| actualIssue.occupiedTile.tile != expectedIssue.occupiedTile.tile
			|| actualIssue.occupiedTile.npcIds != expectedIssue.occupiedTile.npcIds
			|| actualIssue.occupiedTile.actorIndexes != expectedIssue.occupiedTile.actorIndexes) {
			return false;
		}
	}

	return true;
}

void TestEmptyOccupancyQueriesEmptyAndOpen()
{
	const iggy::NpcActorOccupancy2D occupancy;
	const iggy::TileCoord tile { 2, 3 };

	const iggy::NpcActorOccupancyQuery2DResult occupants = iggy::npcActorOccupantsAt(occupancy, tile);
	const iggy::NpcActorOccupancyBlock2DResult block =
		iggy::npcActorTileBlockedFor(occupancy, Id("npc:guard"), tile);

	Expect(occupants.status == iggy::NpcActorOccupancyQuery2DStatus::Empty, "empty occupancy query should report Empty");
	Expect(!occupants.occupied(), "empty occupancy query should not be occupied");
	Expect(occupants.entries.empty(), "empty occupancy query should have no entries");
	Expect(!iggy::npcActorTileOccupied(occupancy, tile), "empty occupancy should not be occupied");
	Expect(!iggy::npcActorOccupancyContains(occupancy, tile), "empty occupancy should not contain tile");
	Expect(iggy::npcActorFirstOccupantAt(occupancy, tile) == nullptr, "empty occupancy should have no first occupant");
	Expect(block.status == iggy::NpcActorOccupancyBlock2DStatus::Empty, "empty tile should report Empty block status");
	Expect(!block.blocked(), "empty tile should not be blocked");
}

void TestSingleOccupantPreservesOccupantFacts()
{
	const iggy::NpcActorState2D actor = Actor("npc:guard", { 1.25F, 2.75F });
	const iggy::NpcActorOccupancy2D occupancy = Occupancy({ actor });
	const iggy::TileCoord tile { 1, 2 };

	const iggy::NpcActorOccupancyQuery2DResult occupants = iggy::npcActorOccupantsAt(occupancy, tile);
	const iggy::NpcActorOccupancyEntry2D *first = iggy::npcActorFirstOccupantAt(occupancy, tile);

	Expect(occupants.status == iggy::NpcActorOccupancyQuery2DStatus::Occupied, "single occupant query should report Occupied");
	Expect(occupants.occupied(), "single occupant query should be occupied");
	Expect(occupants.npcIds.size() == 1 && occupants.npcIds[0] == Id("npc:guard"), "query should preserve occupant npc id");
	Expect(occupants.actorIndexes.size() == 1 && occupants.actorIndexes[0] == 0, "query should preserve occupant actor index");
	Expect(occupants.entries.size() == 1 && SameActor(occupants.entries[0].actor, actor), "query should copy occupant entry actor");
	Expect(first != nullptr && first->npcId == Id("npc:guard"), "first occupant should point at matching occupancy entry");
	Expect(iggy::npcActorTileOccupied(occupancy, tile), "single occupant tile should be occupied");
}

void TestSelfOccupantOnlyDoesNotBlock()
{
	const iggy::NpcActorOccupancy2D occupancy = Occupancy({
		Actor("npc:guard", { 0.5F, 0.5F }),
	});

	const iggy::NpcActorOccupancyBlock2DResult block =
		iggy::npcActorTileBlockedFor(occupancy, Id("npc:guard"), { 0, 0 });

	Expect(block.status == iggy::NpcActorOccupancyBlock2DStatus::OnlySelf, "self-only tile should report OnlySelf");
	Expect(!block.blocked(), "self-only tile should not be blocked");
	Expect(block.occupancy.npcIds.size() == 1 && block.occupancy.npcIds[0] == Id("npc:guard"), "self block query should preserve occupant");
}

void TestOtherOccupantBlocks()
{
	const iggy::NpcActorOccupancy2D occupancy = Occupancy({
		Actor("npc:guard", { 0.5F, 0.5F }),
	});

	const iggy::NpcActorOccupancyBlock2DResult block =
		iggy::npcActorTileBlockedFor(occupancy, Id("npc:intruder"), { 0, 0 });

	Expect(block.status == iggy::NpcActorOccupancyBlock2DStatus::Blocked, "other occupant should report Blocked");
	Expect(block.blocked(), "other occupant should block movement");
}

void TestEmptyMovingNpcIdOnOccupiedTileBlocks()
{
	const iggy::NpcActorOccupancy2D occupancy = Occupancy({
		Actor("npc:guard", { 0.5F, 0.5F }),
	});

	const iggy::NpcActorOccupancyBlock2DResult block =
		iggy::npcActorTileBlockedFor(occupancy, {}, { 0, 0 });

	Expect(block.status == iggy::NpcActorOccupancyBlock2DStatus::Blocked, "empty moving npc id should treat occupied tile as blocked");
	Expect(block.blocked(), "empty moving npc id should be blocked by any occupant");
}

void TestDuplicateTilePreservesAllOccupantsAndBlocksThirdNpc()
{
	const iggy::NpcActorOccupancy2D occupancy = Occupancy({
		Actor("npc:first", { 2.25F, 2.25F }),
		Actor("npc:second", { 2.75F, 2.75F }),
	});
	const iggy::TileCoord tile { 2, 2 };

	const iggy::NpcActorOccupancyQuery2DResult occupants = iggy::npcActorOccupantsAt(occupancy, tile);
	const iggy::NpcActorOccupancyBlock2DResult block =
		iggy::npcActorTileBlockedFor(occupancy, Id("npc:third"), tile);

	Expect(occupancy.hasIssues(), "duplicate occupancy setup should preserve projection issues");
	Expect(occupants.entries.size() == 2, "duplicate tile query should preserve all occupants");
	Expect(occupants.npcIds[0] == Id("npc:first"), "duplicate tile query should preserve first occupant order");
	Expect(occupants.npcIds[1] == Id("npc:second"), "duplicate tile query should preserve second occupant order");
	Expect(occupants.actorIndexes[0] == 0 && occupants.actorIndexes[1] == 1, "duplicate tile query should preserve actor indexes");
	Expect(block.status == iggy::NpcActorOccupancyBlock2DStatus::Blocked, "third npc should be blocked by duplicate occupied tile");
	Expect(block.blocked(), "third npc duplicate block query should be blocked");
}

void TestDuplicateSameMovingNpcIdRepeatedDoesNotBlockUnlessMixed()
{
	const iggy::NpcActorOccupancy2D repeatedSelf = Occupancy({
		Actor("npc:guard", { 1.25F, 1.25F }),
		Actor("npc:guard", { 1.75F, 1.75F }),
	});
	const iggy::NpcActorOccupancy2D mixed = Occupancy({
		Actor("npc:guard", { 1.25F, 1.25F }),
		Actor("npc:other", { 1.75F, 1.75F }),
	});

	const iggy::NpcActorOccupancyBlock2DResult repeatedSelfBlock =
		iggy::npcActorTileBlockedFor(repeatedSelf, Id("npc:guard"), { 1, 1 });
	const iggy::NpcActorOccupancyBlock2DResult mixedBlock =
		iggy::npcActorTileBlockedFor(mixed, Id("npc:guard"), { 1, 1 });

	Expect(repeatedSelfBlock.status == iggy::NpcActorOccupancyBlock2DStatus::OnlySelf, "repeated same exact npc id should remain OnlySelf");
	Expect(!repeatedSelfBlock.blocked(), "repeated same exact npc id should not block itself");
	Expect(mixedBlock.status == iggy::NpcActorOccupancyBlock2DStatus::Blocked, "mixed duplicate tile occupants should block moving npc");
	Expect(mixedBlock.blocked(), "mixed duplicate tile should be blocked");
}

void TestNamespacedAndUnqualifiedIdsAreDistinctForBlocking()
{
	const iggy::NpcActorOccupancy2D occupancy = Occupancy({
		Actor("guard", { 3.25F, 3.25F }),
	});
	const iggy::TileCoord tile { 3, 3 };

	const iggy::NpcActorOccupancyBlock2DResult unqualified =
		iggy::npcActorTileBlockedFor(occupancy, Id("guard"), tile);
	const iggy::NpcActorOccupancyBlock2DResult namespaced =
		iggy::npcActorTileBlockedFor(occupancy, Id("npc:guard"), tile);

	Expect(unqualified.status == iggy::NpcActorOccupancyBlock2DStatus::OnlySelf, "exact unqualified id should be self");
	Expect(!unqualified.blocked(), "exact unqualified id should not be blocked");
	Expect(namespaced.status == iggy::NpcActorOccupancyBlock2DStatus::Blocked, "namespaced id should differ from unqualified occupant");
	Expect(namespaced.blocked(), "namespaced id should be blocked by unqualified occupant");
	Expect(unqualified.movingNpcId != namespaced.movingNpcId, "namespaced and unqualified moving ids should remain distinct");
}

void TestQueriesDoNotMutateOccupancy()
{
	iggy::NpcActorOccupancy2D occupancy = Occupancy({
		Actor("npc:first", { 0.5F, 0.5F }),
		Actor("npc:second", { 0.75F, 0.75F }),
		Actor("npc:third", { 2.5F, 2.5F }),
	});
	const iggy::NpcActorOccupancy2D before = occupancy;

	const iggy::NpcActorOccupancyQuery2DResult occupants = iggy::npcActorOccupantsAt(occupancy, { 0, 0 });
	const iggy::NpcActorOccupancyBlock2DResult block =
		iggy::npcActorTileBlockedFor(occupancy, Id("npc:first"), { 0, 0 });
	const bool occupied = iggy::npcActorTileOccupied(occupancy, { 2, 2 });

	Expect(occupants.entries.size() == 2, "immutability setup should query duplicate occupants");
	Expect(block.blocked(), "immutability setup should query blocking result");
	Expect(occupied, "immutability setup should query occupied tile");
	Expect(SameOccupancy(occupancy, before), "occupancy queries should not mutate input occupancy");
}

} // namespace

int main()
{
	TestEmptyOccupancyQueriesEmptyAndOpen();
	TestSingleOccupantPreservesOccupantFacts();
	TestSelfOccupantOnlyDoesNotBlock();
	TestOtherOccupantBlocks();
	TestEmptyMovingNpcIdOnOccupiedTileBlocks();
	TestDuplicateTilePreservesAllOccupantsAndBlocksThirdNpc();
	TestDuplicateSameMovingNpcIdRepeatedDoesNotBlockUnlessMixed();
	TestNamespacedAndUnqualifiedIdsAreDistinctForBlocking();
	TestQueriesDoNotMutateOccupancy();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
