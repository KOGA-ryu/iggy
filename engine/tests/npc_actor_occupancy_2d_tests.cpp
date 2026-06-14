#include <cstdlib>
#include <vector>

#include "scene/npc/NpcActorOccupancy2D.hpp"
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

bool SameActor(const iggy::NpcActorState2D &actual, const iggy::NpcActorState2D &expected)
{
	return actual.npcId == expected.npcId
		&& actual.aiProfileId == expected.aiProfileId
		&& actual.factionId == expected.factionId
		&& NearVec(actual.position, expected.position)
		&& actual.currentGoalId == expected.currentGoalId
		&& actual.present == expected.present;
}

bool SameRegistry(const iggy::NpcActorState2DRegistry &actual, const iggy::NpcActorState2DRegistry &expected)
{
	if (actual.actors.size() != expected.actors.size()) {
		return false;
	}

	for (std::size_t index = 0; index < actual.actors.size(); ++index) {
		if (!SameActor(actual.actors[index], expected.actors[index])) {
			return false;
		}
	}
	return true;
}

void TestEmptyRegistryBuildsEmptyOccupancy()
{
	const iggy::NpcActorState2DRegistry registry;

	const iggy::NpcActorOccupancy2D occupancy =
		iggy::NpcActorOccupancyProjector2D {}.project(registry);

	Expect(occupancy.status == iggy::NpcActorOccupancy2DStatus::Built, "empty occupancy projection should build");
	Expect(occupancy.entries.empty(), "empty occupancy projection should have no entries");
	Expect(occupancy.occupiedTiles.empty(), "empty occupancy projection should have no occupied tiles");
	Expect(occupancy.issues.empty(), "empty occupancy projection should have no issues");
	Expect(!occupancy.hasIssues(), "empty occupancy projection should not report issues");
}

void TestPresentActorMapsToExpectedTileAndPreservesPayload()
{
	const iggy::NpcActorState2D actor = Actor("npc:guard", { 2.75F, -1.25F });
	const iggy::NpcActorState2DRegistry registry = Registry({ actor });

	const iggy::NpcActorOccupancy2D occupancy =
		iggy::NpcActorOccupancyProjector2D {}.project(registry);

	Expect(occupancy.entries.size() == 1, "present actor should create one occupancy entry");
	Expect(occupancy.occupiedTiles.size() == 1, "present actor should create one occupied tile");
	if (occupancy.entries.size() == 1) {
		Expect(occupancy.entries[0].npcId == Id("npc:guard"), "occupancy entry should preserve npc id");
		Expect(occupancy.entries[0].tile == iggy::TileCoord { 2, -2 }, "occupancy entry should use tileForPoint convention");
		Expect(occupancy.entries[0].actorIndex == 0, "occupancy entry should preserve actor index");
		Expect(SameActor(occupancy.entries[0].actor, actor), "occupancy entry should copy actor payload");
	}
	if (occupancy.occupiedTiles.size() == 1) {
		Expect(occupancy.occupiedTiles[0].tile == iggy::TileCoord { 2, -2 }, "occupied tile should preserve tile");
		Expect(occupancy.occupiedTiles[0].npcIds.size() == 1 && occupancy.occupiedTiles[0].npcIds[0] == Id("npc:guard"), "occupied tile should preserve npc id");
		Expect(occupancy.occupiedTiles[0].actorIndexes.size() == 1 && occupancy.occupiedTiles[0].actorIndexes[0] == 0, "occupied tile should preserve actor index");
	}
}

void TestAbsentActorsAreIgnoredByDefault()
{
	const iggy::NpcActorState2DRegistry registry = Registry({
		Actor("npc:present", { 0.5F, 0.5F }, true),
		Actor("npc:absent", { 1.5F, 1.5F }, false),
	});

	const iggy::NpcActorOccupancy2D occupancy =
		iggy::NpcActorOccupancyProjector2D {}.project(registry);

	Expect(occupancy.entries.size() == 1, "absent actor should be ignored by default");
	Expect(occupancy.entries[0].npcId == Id("npc:present"), "default occupancy should keep present actor");
	Expect(occupancy.occupiedTiles.size() == 1, "default occupancy should only include present occupied tile");
}

void TestIncludeAbsentIncludesAbsentActors()
{
	const iggy::NpcActorState2DRegistry registry = Registry({
		Actor("npc:present", { 0.5F, 0.5F }, true),
		Actor("npc:absent", { 1.5F, 1.5F }, false),
	});
	iggy::NpcActorOccupancy2DConfig config;
	config.includeAbsent = true;

	const iggy::NpcActorOccupancy2D occupancy =
		iggy::NpcActorOccupancyProjector2D {}.project(registry, config);

	Expect(occupancy.entries.size() == 2, "includeAbsent should include absent actor entries");
	Expect(occupancy.entries[1].npcId == Id("npc:absent"), "includeAbsent should preserve absent actor id");
	Expect(!occupancy.entries[1].actor.present, "includeAbsent should preserve absent actor flag");
	Expect(occupancy.occupiedTiles.size() == 2, "includeAbsent should include absent actor occupied tile");
}

void TestDuplicateOccupiedTilePreservesActorsAndReportsIssue()
{
	const iggy::NpcActorState2DRegistry registry = Registry({
		Actor("npc:first", { 1.25F, 1.25F }),
		Actor("npc:second", { 1.75F, 1.75F }),
	});

	const iggy::NpcActorOccupancy2D occupancy =
		iggy::NpcActorOccupancyProjector2D {}.project(registry);

	Expect(occupancy.entries.size() == 2, "duplicate tile actors should both remain entries");
	Expect(occupancy.occupiedTiles.size() == 1, "duplicate tile actors should share one occupied tile group");
	Expect(occupancy.hasIssues(), "duplicate tile actors should report issue");
	Expect(occupancy.issues.size() == 1, "two actors on one tile should produce one duplicate issue");
	if (occupancy.occupiedTiles.size() == 1) {
		Expect(occupancy.occupiedTiles[0].tile == iggy::TileCoord { 1, 1 }, "duplicate group should preserve occupied tile");
		Expect(occupancy.occupiedTiles[0].npcIds.size() == 2, "duplicate group should preserve both npc ids");
		Expect(occupancy.occupiedTiles[0].npcIds[0] == Id("npc:first"), "duplicate group should preserve first npc id");
		Expect(occupancy.occupiedTiles[0].npcIds[1] == Id("npc:second"), "duplicate group should preserve second npc id");
		Expect(occupancy.occupiedTiles[0].actorIndexes.size() == 2, "duplicate group should preserve both actor indexes");
		Expect(occupancy.occupiedTiles[0].actorIndexes[0] == 0, "duplicate group should preserve first actor index");
		Expect(occupancy.occupiedTiles[0].actorIndexes[1] == 1, "duplicate group should preserve second actor index");
	}
	if (occupancy.issues.size() == 1) {
		Expect(occupancy.issues[0].code == iggy::NpcActorOccupancyIssue2DCode::DuplicateOccupiedTile, "duplicate issue should use DuplicateOccupiedTile");
		Expect(occupancy.issues[0].tile == iggy::TileCoord { 1, 1 }, "duplicate issue should preserve tile");
		Expect(occupancy.issues[0].firstActorIndex == 0, "duplicate issue should preserve first actor index");
		Expect(occupancy.issues[0].laterActorIndex == 1, "duplicate issue should preserve later actor index");
		Expect(occupancy.issues[0].occupiedTile.npcIds.size() == 2, "duplicate issue should preserve tile group snapshot");
	}
}

void TestMultipleUniqueTilesPreserveActorAndFirstSeenTileOrder()
{
	const iggy::NpcActorState2DRegistry registry = Registry({
		Actor("npc:first", { 3.5F, 0.5F }),
		Actor("npc:second", { -1.25F, 4.5F }),
		Actor("npc:third", { 0.0F, 0.0F }),
	});

	const iggy::NpcActorOccupancy2D occupancy =
		iggy::NpcActorOccupancyProjector2D {}.project(registry);

	Expect(occupancy.entries.size() == 3, "unique tile actors should all remain entries");
	Expect(occupancy.occupiedTiles.size() == 3, "unique tile actors should create unique tile groups");
	Expect(!occupancy.hasIssues(), "unique tile actors should not report issues");
	Expect(occupancy.entries[0].npcId == Id("npc:first"), "entries should preserve first actor order");
	Expect(occupancy.entries[1].npcId == Id("npc:second"), "entries should preserve second actor order");
	Expect(occupancy.entries[2].npcId == Id("npc:third"), "entries should preserve third actor order");
	Expect(occupancy.occupiedTiles[0].tile == iggy::TileCoord { 3, 0 }, "occupied tiles should preserve first-seen tile order");
	Expect(occupancy.occupiedTiles[1].tile == iggy::TileCoord { -2, 4 }, "occupied tiles should preserve second first-seen tile order");
	Expect(occupancy.occupiedTiles[2].tile == iggy::TileCoord { 0, 0 }, "occupied tiles should preserve third first-seen tile order");
}

void TestNamespacedAndUnqualifiedIdsArePreservedExactly()
{
	const iggy::NpcActorState2DRegistry registry = Registry({
		Actor("guard", { 0.5F, 0.5F }),
		Actor("npc:guard", { 1.5F, 1.5F }),
	});

	const iggy::NpcActorOccupancy2D occupancy =
		iggy::NpcActorOccupancyProjector2D {}.project(registry);

	Expect(occupancy.entries.size() == 2, "namespaced setup should preserve both entries");
	Expect(occupancy.entries[0].npcId == Id("guard"), "occupancy should preserve unqualified id");
	Expect(occupancy.entries[1].npcId == Id("npc:guard"), "occupancy should preserve namespaced id");
	Expect(occupancy.entries[0].npcId != occupancy.entries[1].npcId, "namespaced and unqualified ids should remain distinct");
	Expect(occupancy.occupiedTiles[0].npcIds[0] == Id("guard"), "occupied tile should preserve unqualified id");
	Expect(occupancy.occupiedTiles[1].npcIds[0] == Id("npc:guard"), "occupied tile should preserve namespaced id");
}

void TestInputRegistryIsNotMutated()
{
	iggy::NpcActorState2DRegistry registry = Registry({
		Actor("npc:first", { 0.5F, 0.5F }),
		Actor("npc:absent", { 1.5F, 1.5F }, false),
		Actor("npc:second", { 2.5F, 2.5F }),
	});
	const iggy::NpcActorState2DRegistry before = registry;

	const iggy::NpcActorOccupancy2D occupancy =
		iggy::NpcActorOccupancyProjector2D {}.project(registry);

	Expect(occupancy.entries.size() == 2, "immutability setup should project present actors");
	Expect(SameRegistry(registry, before), "occupancy projection should not mutate input registry");
}

} // namespace

int main()
{
	TestEmptyRegistryBuildsEmptyOccupancy();
	TestPresentActorMapsToExpectedTileAndPreservesPayload();
	TestAbsentActorsAreIgnoredByDefault();
	TestIncludeAbsentIncludesAbsentActors();
	TestDuplicateOccupiedTilePreservesActorsAndReportsIssue();
	TestMultipleUniqueTilesPreserveActorAndFirstSeenTileOrder();
	TestNamespacedAndUnqualifiedIdsArePreservedExactly();
	TestInputRegistryIsNotMutated();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
