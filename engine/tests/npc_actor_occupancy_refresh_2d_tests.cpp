#include <cstdlib>
#include <vector>

#include "scene/npc/NpcActorOccupancyRefresh2D.hpp"
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
	bool present = true)
{
	return {
		Id(npcId),
		Id("ai-profile:guard"),
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

iggy::NpcActorOccupancy2D Occupancy(
	const iggy::NpcActorState2DRegistry &registry,
	const iggy::NpcActorOccupancy2DConfig &config = {})
{
	return iggy::NpcActorOccupancyProjector2D {}.project(registry, config);
}

iggy::NpcActorMovementRefreshWork2D Work(std::vector<iggy::NpcActorMovementRefreshWork2DType> types)
{
	iggy::NpcActorMovementRefreshWork2D work;
	for (iggy::NpcActorMovementRefreshWork2DType type : types) {
		iggy::NpcActorMovementRefreshWork2DItem item;
		item.type = type;
		work.items.push_back(item);
	}
	return work;
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

bool SameRegistry(
	const iggy::NpcActorState2DRegistry &actual,
	const iggy::NpcActorState2DRegistry &expected)
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

bool SameOccupiedTile(
	const iggy::NpcActorOccupiedTile2D &actual,
	const iggy::NpcActorOccupiedTile2D &expected)
{
	return actual.tile == expected.tile
		&& actual.npcIds == expected.npcIds
		&& actual.actorIndexes == expected.actorIndexes;
}

bool SameOccupancyEntry(
	const iggy::NpcActorOccupancyEntry2D &actual,
	const iggy::NpcActorOccupancyEntry2D &expected)
{
	return actual.npcId == expected.npcId
		&& actual.tile == expected.tile
		&& actual.actorIndex == expected.actorIndex
		&& SameActor(actual.actor, expected.actor);
}

bool SameIssue(
	const iggy::NpcActorOccupancyIssue2D &actual,
	const iggy::NpcActorOccupancyIssue2D &expected)
{
	return actual.code == expected.code
		&& actual.tile == expected.tile
		&& actual.firstActorIndex == expected.firstActorIndex
		&& actual.laterActorIndex == expected.laterActorIndex
		&& SameOccupiedTile(actual.occupiedTile, expected.occupiedTile);
}

bool SameOccupancy(
	const iggy::NpcActorOccupancy2D &actual,
	const iggy::NpcActorOccupancy2D &expected)
{
	if (actual.status != expected.status
		|| actual.entries.size() != expected.entries.size()
		|| actual.occupiedTiles.size() != expected.occupiedTiles.size()
		|| actual.issues.size() != expected.issues.size()) {
		return false;
	}
	for (std::size_t index = 0; index < actual.entries.size(); ++index) {
		if (!SameOccupancyEntry(actual.entries[index], expected.entries[index])) {
			return false;
		}
	}
	for (std::size_t index = 0; index < actual.occupiedTiles.size(); ++index) {
		if (!SameOccupiedTile(actual.occupiedTiles[index], expected.occupiedTiles[index])) {
			return false;
		}
	}
	for (std::size_t index = 0; index < actual.issues.size(); ++index) {
		if (!SameIssue(actual.issues[index], expected.issues[index])) {
			return false;
		}
	}
	return true;
}

bool SameWork(
	const iggy::NpcActorMovementRefreshWork2D &actual,
	const iggy::NpcActorMovementRefreshWork2D &expected)
{
	if (actual.items.size() != expected.items.size()
		|| actual.dirtyTiles != expected.dirtyTiles
		|| actual.needsOccupancyRebuild != expected.needsOccupancyRebuild
		|| actual.needsAiMapQueryRefresh != expected.needsAiMapQueryRefresh
		|| actual.needsInteractionRefresh != expected.needsInteractionRefresh
		|| actual.needsRenderRefresh != expected.needsRenderRefresh
		|| actual.needsVisibilityRefresh != expected.needsVisibilityRefresh) {
		return false;
	}
	for (std::size_t index = 0; index < actual.items.size(); ++index) {
		if (actual.items[index].type != expected.items[index].type
			|| actual.items[index].dirtyTiles != expected.items[index].dirtyTiles) {
			return false;
		}
	}
	return true;
}

void TestNoRefreshWorkReturnsPreviousOccupancyUnchanged()
{
	const iggy::NpcActorState2DRegistry registry = Registry({
		Actor("npc:moved", { 4.5F, 4.5F }),
	});
	const iggy::NpcActorOccupancy2D previous = Occupancy(Registry({
		Actor("npc:moved", { 0.5F, 0.5F }),
	}));
	const iggy::NpcActorMovementRefreshWork2D work;

	const iggy::NpcActorOccupancyRefresh2DResult result =
		iggy::NpcActorOccupancyRefresher2D {}.refresh(registry, previous, work);

	Expect(result.status == iggy::NpcActorOccupancyRefresh2DStatus::NoRefreshNeeded, "no occupancy work should not refresh");
	Expect(!result.refreshed && !result.changed() && !result.refreshedOccupancy(), "no occupancy work should report unchanged");
	Expect(SameOccupancy(result.occupancy, previous), "no occupancy work should return previous occupancy");
	Expect(result.occupancy.entries[0].tile == iggy::TileCoord { 0, 0 }, "no occupancy work should not rebuild from changed registry");
}

void TestOccupancyRebuildWorkRebuildsFromSuppliedRegistry()
{
	const iggy::NpcActorState2DRegistry registry = Registry({
		Actor("npc:moved", { 4.5F, 4.5F }),
	});
	const iggy::NpcActorOccupancy2D previous = Occupancy(Registry({
		Actor("npc:moved", { 0.5F, 0.5F }),
	}));
	const iggy::NpcActorMovementRefreshWork2D work =
		Work({ iggy::NpcActorMovementRefreshWork2DType::OccupancyRebuild });

	const iggy::NpcActorOccupancyRefresh2DResult result =
		iggy::NpcActorOccupancyRefresher2D {}.refresh(registry, previous, work);

	Expect(result.status == iggy::NpcActorOccupancyRefresh2DStatus::Refreshed, "occupancy work should refresh");
	Expect(result.refreshed && result.changed() && result.refreshedOccupancy(), "occupancy work should report refreshed");
	Expect(result.occupancy.entries.size() == 1, "rebuilt occupancy should include moved actor");
	Expect(result.occupancy.entries[0].tile == iggy::TileCoord { 4, 4 }, "rebuilt occupancy should reflect current registry tile");
	Expect(result.occupancy.entries[0].npcId == Id("npc:moved"), "rebuilt occupancy should preserve actor id");
}

void TestIncludeAbsentPolicyIsPreservedThroughConfig()
{
	const iggy::NpcActorState2DRegistry registry = Registry({
		Actor("npc:present", { 0.5F, 0.5F }, true),
		Actor("npc:absent", { 1.5F, 1.5F }, false),
	});
	const iggy::NpcActorMovementRefreshWork2D work =
		Work({ iggy::NpcActorMovementRefreshWork2DType::OccupancyRebuild });

	const iggy::NpcActorOccupancyRefresh2DResult defaultResult =
		iggy::NpcActorOccupancyRefresher2D {}.refresh(registry, {}, work);

	iggy::NpcActorOccupancyRefresh2DConfig includeAbsentConfig;
	includeAbsentConfig.occupancy.includeAbsent = true;
	const iggy::NpcActorOccupancyRefresh2DResult includeAbsentResult =
		iggy::NpcActorOccupancyRefresher2D {}.refresh(registry, {}, work, includeAbsentConfig);

	Expect(defaultResult.occupancy.entries.size() == 1, "default rebuild should ignore absent actors");
	Expect(defaultResult.occupancy.entries[0].npcId == Id("npc:present"), "default rebuild should keep present actor");
	Expect(includeAbsentResult.occupancy.entries.size() == 2, "includeAbsent rebuild should include absent actors");
	Expect(includeAbsentResult.occupancy.entries[1].npcId == Id("npc:absent"), "includeAbsent rebuild should preserve absent actor id");
	Expect(!includeAbsentResult.occupancy.entries[1].actor.present, "includeAbsent rebuild should preserve absent actor flag");
}

void TestDuplicateOccupiedTilesRemainInspectableAfterRebuild()
{
	const iggy::NpcActorState2DRegistry registry = Registry({
		Actor("npc:first", { 2.25F, 2.25F }),
		Actor("npc:second", { 2.75F, 2.75F }),
	});
	const iggy::NpcActorMovementRefreshWork2D work =
		Work({ iggy::NpcActorMovementRefreshWork2DType::OccupancyRebuild });

	const iggy::NpcActorOccupancyRefresh2DResult result =
		iggy::NpcActorOccupancyRefresher2D {}.refresh(registry, {}, work);

	Expect(result.occupancy.entries.size() == 2, "duplicate rebuild should preserve both entries");
	Expect(result.occupancy.occupiedTiles.size() == 1, "duplicate rebuild should keep shared occupied tile group");
	Expect(result.occupancy.hasIssues(), "duplicate rebuild should preserve issue visibility");
	Expect(result.occupancy.issues.size() == 1, "duplicate rebuild should preserve duplicate issue");
	Expect(result.occupancy.issues[0].code == iggy::NpcActorOccupancyIssue2DCode::DuplicateOccupiedTile, "duplicate rebuild should report duplicate tile");
	Expect(result.occupancy.issues[0].laterActorIndex == 1, "duplicate rebuild should preserve later actor index");
}

void TestNonOccupancyRefreshWorkDoesNotRebuild()
{
	const iggy::NpcActorState2DRegistry registry = Registry({
		Actor("npc:moved", { 4.5F, 4.5F }),
	});
	const iggy::NpcActorOccupancy2D previous = Occupancy(Registry({
		Actor("npc:moved", { 0.5F, 0.5F }),
	}));
	const iggy::NpcActorMovementRefreshWork2D work = Work({
		iggy::NpcActorMovementRefreshWork2DType::AiMapQueryRefresh,
		iggy::NpcActorMovementRefreshWork2DType::RenderRefresh,
	});

	const iggy::NpcActorOccupancyRefresh2DResult result =
		iggy::NpcActorOccupancyRefresher2D {}.refresh(registry, previous, work);

	Expect(result.status == iggy::NpcActorOccupancyRefresh2DStatus::NoRefreshNeeded, "non-occupancy work should not refresh");
	Expect(SameOccupancy(result.occupancy, previous), "non-occupancy work should return previous occupancy");
}

void TestInputsAndCopiedFactsArePreserved()
{
	iggy::NpcActorState2DRegistry registry = Registry({
		Actor("npc:moved", { 4.5F, 4.5F }),
	});
	iggy::NpcActorOccupancy2D previous = Occupancy(Registry({
		Actor("npc:moved", { 0.5F, 0.5F }),
	}));
	iggy::NpcActorMovementRefreshWork2D work =
		Work({ iggy::NpcActorMovementRefreshWork2DType::OccupancyRebuild });
	work.dirtyTiles = { iggy::TileCoord { 0, 0 }, iggy::TileCoord { 4, 4 } };
	work.items[0].dirtyTiles = work.dirtyTiles;
	work.needsOccupancyRebuild = true;

	const iggy::NpcActorState2DRegistry registryBefore = registry;
	const iggy::NpcActorOccupancy2D previousBefore = previous;
	const iggy::NpcActorMovementRefreshWork2D workBefore = work;

	const iggy::NpcActorOccupancyRefresh2DResult result =
		iggy::NpcActorOccupancyRefresher2D {}.refresh(registry, previous, work);

	Expect(SameRegistry(result.registry, registryBefore), "result should copy source registry");
	Expect(SameOccupancy(result.previousOccupancy, previousBefore), "result should copy previous occupancy");
	Expect(SameWork(result.work, workBefore), "result should copy refresh work");
	Expect(SameRegistry(registry, registryBefore), "refresh should not mutate source registry");
	Expect(SameOccupancy(previous, previousBefore), "refresh should not mutate previous occupancy");
	Expect(SameWork(work, workBefore), "refresh should not mutate refresh work");

	registry.actors[0].position = { 9.5F, 9.5F };
	previous.entries.clear();
	work.items.clear();

	Expect(SameRegistry(result.registry, registryBefore), "result registry should not alias source registry");
	Expect(SameOccupancy(result.previousOccupancy, previousBefore), "result previous occupancy should not alias source occupancy");
	Expect(SameWork(result.work, workBefore), "result work should not alias source work");
}

} // namespace

int main()
{
	TestNoRefreshWorkReturnsPreviousOccupancyUnchanged();
	TestOccupancyRebuildWorkRebuildsFromSuppliedRegistry();
	TestIncludeAbsentPolicyIsPreservedThroughConfig();
	TestDuplicateOccupiedTilesRemainInspectableAfterRebuild();
	TestNonOccupancyRefreshWorkDoesNotRebuild();
	TestInputsAndCopiedFactsArePreserved();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
