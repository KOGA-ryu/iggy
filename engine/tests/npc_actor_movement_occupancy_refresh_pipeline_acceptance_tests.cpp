#include <cstdlib>
#include <vector>

#include "scene/npc/NpcActorMovementFrameApply2D.hpp"
#include "scene/npc/NpcActorMovementFrameReport2D.hpp"
#include "scene/npc/NpcActorMovementRefreshWork2D.hpp"
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
		Id("goal:patrol"),
		present,
	};
}

iggy::NpcActorState2DRegistry Registry(std::vector<iggy::NpcActorState2D> actors)
{
	iggy::NpcActorState2DRegistry registry;
	registry.actors = actors;
	return registry;
}

iggy::NpcActorPathStepOccupancyFilter2D Filter(
	const char *npcId,
	iggy::Vec2 oldPosition,
	iggy::Vec2 proposedPosition,
	iggy::NpcActorPathStepOccupancyFilter2DStatus status = iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed,
	bool requestsMovement = true,
	const char *blockingNpcId = "")
{
	iggy::NpcActorPathStepOccupancyFilter2D filter;
	filter.step.npcId = Id(npcId);
	filter.step.oldPosition = oldPosition;
	filter.step.proposedPosition = proposedPosition;
	filter.step.oldTile = iggy::tileForPoint(oldPosition);
	filter.step.proposedTile = iggy::tileForPoint(proposedPosition);
	filter.step.moveMode = iggy::NpcMoveMode::Walk;
	filter.step.status = status == iggy::NpcActorPathStepOccupancyFilter2DStatus::NoStepProposal
		? iggy::NpcActorPathStep2DStatus::NoPath
		: iggy::NpcActorPathStep2DStatus::Proposed;
	filter.step.requestsMovement = status != iggy::NpcActorPathStepOccupancyFilter2DStatus::NoStepProposal;
	filter.status = status;
	filter.requestsMovement = requestsMovement;
	if (blockingNpcId[0] != '\0') {
		filter.blockingNpcId = Id(blockingNpcId);
	}
	return filter;
}

iggy::NpcActorMovementFrameApply2DRequest Request(const iggy::NpcActorPathStepOccupancyFilter2D &filter)
{
	iggy::NpcActorMovementFrameApply2DRequest request;
	request.filter = filter;
	return request;
}

iggy::NpcActorOccupancy2D Occupancy(const iggy::NpcActorState2DRegistry &registry)
{
	return iggy::NpcActorOccupancyProjector2D {}.project(registry);
}

iggy::NpcActorMovementFrameApply2DResult Apply(
	const iggy::NpcActorState2DRegistry &registry,
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> &requests)
{
	return iggy::NpcActorMovementFrameApplier2D {}.apply(registry, requests);
}

iggy::NpcActorMovementFrameReport2D Report(const iggy::NpcActorMovementFrameApply2DResult &apply)
{
	return iggy::NpcActorMovementFrameReporter2D {}.report(apply);
}

iggy::NpcActorMovementRefreshWork2D Work(const iggy::NpcActorMovementFrameReport2D &report)
{
	return iggy::NpcActorMovementRefreshWorkProjector2D {}.project(report);
}

iggy::NpcActorOccupancyRefresh2DResult Refresh(
	const iggy::NpcActorState2DRegistry &registry,
	const iggy::NpcActorOccupancy2D &previousOccupancy,
	const iggy::NpcActorMovementRefreshWork2D &work)
{
	return iggy::NpcActorOccupancyRefresher2D {}.refresh(registry, previousOccupancy, work);
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

bool HasWorkType(
	const iggy::NpcActorMovementRefreshWork2D &work,
	iggy::NpcActorMovementRefreshWork2DType type)
{
	for (const iggy::NpcActorMovementRefreshWork2DItem &item : work.items) {
		if (item.type == type) {
			return true;
		}
	}
	return false;
}

const iggy::NpcActorOccupancyEntry2D *FindEntry(
	const iggy::NpcActorOccupancy2D &occupancy,
	const iggy::ResourceId &npcId)
{
	for (const iggy::NpcActorOccupancyEntry2D &entry : occupancy.entries) {
		if (entry.npcId == npcId) {
			return &entry;
		}
	}
	return nullptr;
}

void TestSuccessfulMovementRefreshesOccupancy()
{
	const iggy::NpcActorState2DRegistry initialRegistry = Registry({
		Actor("npc:mover", { 0.5F, 0.5F }),
	});
	const iggy::NpcActorOccupancy2D previousOccupancy = Occupancy(initialRegistry);

	const iggy::NpcActorMovementFrameApply2DResult apply = Apply(initialRegistry, {
		Request(Filter("npc:mover", { 0.5F, 0.5F }, { 2.5F, 0.5F })),
	});
	const iggy::NpcActorMovementFrameReport2D report = Report(apply);
	const iggy::NpcActorMovementRefreshWork2D work = Work(report);
	const iggy::NpcActorOccupancyRefresh2DResult refresh = Refresh(apply.registry, previousOccupancy, work);

	Expect(apply.changed, "successful movement should change actor registry by value");
	Expect(report.needsOccupancyRebuild, "successful movement report should request occupancy rebuild");
	Expect(HasWorkType(work, iggy::NpcActorMovementRefreshWork2DType::OccupancyRebuild), "successful movement work should contain occupancy rebuild item");
	Expect(work.dirtyTiles == report.dirtyTiles, "refresh work should preserve movement report dirty tiles");
	Expect(refresh.status == iggy::NpcActorOccupancyRefresh2DStatus::Refreshed, "occupancy refresh should rebuild after movement");
	Expect(refresh.refreshed, "occupancy refresh should mark refreshed");

	const iggy::NpcActorOccupancyEntry2D *entry = FindEntry(refresh.occupancy, Id("npc:mover"));
	Expect(entry != nullptr, "refreshed occupancy should contain moved actor");
	if (entry != nullptr) {
		Expect(entry->tile == iggy::TileCoord { 2, 0 }, "refreshed occupancy should use new actor tile");
	}
	Expect(previousOccupancy.entries[0].tile == iggy::TileCoord { 0, 0 }, "previous occupancy should still preserve old tile");
}

void TestBlockedMovementDoesNotRefreshOccupancy()
{
	const iggy::NpcActorState2DRegistry initialRegistry = Registry({
		Actor("npc:blocked", { 0.5F, 0.5F }),
	});
	const iggy::NpcActorOccupancy2D previousOccupancy = Occupancy(initialRegistry);

	const iggy::NpcActorMovementFrameApply2DResult apply = Apply(initialRegistry, {
		Request(Filter(
			"npc:blocked",
			{ 0.5F, 0.5F },
			{ 1.5F, 0.5F },
			iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc,
			false,
			"npc:blocker")),
	});
	const iggy::NpcActorMovementFrameReport2D report = Report(apply);
	const iggy::NpcActorMovementRefreshWork2D work = Work(report);
	const iggy::NpcActorOccupancyRefresh2DResult refresh = Refresh(apply.registry, previousOccupancy, work);

	Expect(!apply.changed, "blocked movement should not change actor registry");
	Expect(!report.needsOccupancyRebuild, "blocked movement should not request occupancy rebuild");
	Expect(!HasWorkType(work, iggy::NpcActorMovementRefreshWork2DType::OccupancyRebuild), "blocked movement work should not contain occupancy rebuild");
	Expect(refresh.status == iggy::NpcActorOccupancyRefresh2DStatus::NoRefreshNeeded, "blocked movement refresh should be no-op");
	Expect(!refresh.refreshed, "blocked movement refresh should not mark refreshed");
	Expect(SameOccupancy(refresh.occupancy, previousOccupancy), "blocked movement should return previous occupancy unchanged");
}

void TestMixedMovementAndBlockedFrameRefreshesSuccessfulMovementOnly()
{
	const iggy::NpcActorState2DRegistry initialRegistry = Registry({
		Actor("npc:mover", { 0.5F, 0.5F }),
		Actor("npc:blocked", { 4.5F, 0.5F }),
	});
	const iggy::NpcActorOccupancy2D previousOccupancy = Occupancy(initialRegistry);

	const iggy::NpcActorMovementFrameApply2DResult apply = Apply(initialRegistry, {
		Request(Filter("npc:mover", { 0.5F, 0.5F }, { 1.5F, 0.5F })),
		Request(Filter(
			"npc:blocked",
			{ 4.5F, 0.5F },
			{ 5.5F, 0.5F },
			iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc,
			false,
			"npc:blocker")),
	});
	const iggy::NpcActorMovementFrameReport2D report = Report(apply);
	const iggy::NpcActorMovementRefreshWork2D work = Work(report);
	const iggy::NpcActorOccupancyRefresh2DResult refresh = Refresh(apply.registry, previousOccupancy, work);

	Expect(apply.movedCount == 1 && apply.blockedCount == 1, "mixed frame should preserve moved and blocked counts");
	Expect(HasWorkType(work, iggy::NpcActorMovementRefreshWork2DType::OccupancyRebuild), "mixed frame should rebuild occupancy because one actor moved");
	Expect(refresh.refreshed, "mixed frame should refresh occupancy");

	const iggy::NpcActorOccupancyEntry2D *mover = FindEntry(refresh.occupancy, Id("npc:mover"));
	const iggy::NpcActorOccupancyEntry2D *blocked = FindEntry(refresh.occupancy, Id("npc:blocked"));
	Expect(mover != nullptr && blocked != nullptr, "mixed refreshed occupancy should preserve both actors");
	if (mover != nullptr) {
		Expect(mover->tile == iggy::TileCoord { 1, 0 }, "mixed refreshed occupancy should reflect successful movement");
	}
	if (blocked != nullptr) {
		Expect(blocked->tile == iggy::TileCoord { 4, 0 }, "mixed refreshed occupancy should preserve blocked actor position");
	}
}

void TestDuplicateFinalOccupancyRemainsInspectable()
{
	const iggy::NpcActorState2DRegistry registryWithDuplicateFinalPositions = Registry({
		Actor("npc:first", { 2.25F, 2.25F }),
		Actor("npc:second", { 2.75F, 2.75F }),
	});
	const iggy::NpcActorMovementFrameReport2D report = [] {
		iggy::NpcActorMovementFrameReport2D value;
		value.needsOccupancyRebuild = true;
		value.dirtyTiles = { iggy::TileCoord { 2, 2 } };
		return value;
	}();
	const iggy::NpcActorMovementRefreshWork2D work = Work(report);

	const iggy::NpcActorOccupancyRefresh2DResult refresh =
		Refresh(registryWithDuplicateFinalPositions, {}, work);

	Expect(refresh.refreshed, "duplicate final occupancy setup should refresh");
	Expect(refresh.occupancy.entries.size() == 2, "duplicate final occupancy should preserve both actors");
	Expect(refresh.occupancy.occupiedTiles.size() == 1, "duplicate final occupancy should keep one occupied tile group");
	Expect(refresh.occupancy.hasIssues(), "duplicate final occupancy should remain inspectable through issues");
	Expect(refresh.occupancy.issues.size() == 1, "duplicate final occupancy should preserve duplicate issue");
	Expect(refresh.occupancy.issues[0].code == iggy::NpcActorOccupancyIssue2DCode::DuplicateOccupiedTile, "duplicate final occupancy should report duplicate tile");
}

void TestPipelineInputsAreNotMutated()
{
	iggy::NpcActorState2DRegistry initialRegistry = Registry({
		Actor("npc:mover", { 0.5F, 0.5F }),
	});
	iggy::NpcActorOccupancy2D previousOccupancy = Occupancy(initialRegistry);

	const iggy::NpcActorState2DRegistry initialBefore = initialRegistry;
	const iggy::NpcActorOccupancy2D previousBefore = previousOccupancy;

	iggy::NpcActorMovementFrameApply2DResult apply = Apply(initialRegistry, {
		Request(Filter("npc:mover", { 0.5F, 0.5F }, { 1.5F, 0.5F })),
	});
	iggy::NpcActorMovementFrameReport2D report = Report(apply);
	iggy::NpcActorMovementRefreshWork2D work = Work(report);

	const iggy::NpcActorMovementFrameApply2DResult applyBefore = apply;
	const iggy::NpcActorMovementFrameReport2D reportBefore = report;
	const iggy::NpcActorMovementRefreshWork2D workBefore = work;

	const iggy::NpcActorOccupancyRefresh2DResult refresh =
		Refresh(apply.registry, previousOccupancy, work);

	Expect(refresh.refreshed, "immutability pipeline setup should refresh");
	Expect(SameRegistry(initialRegistry, initialBefore), "pipeline should not mutate initial registry");
	Expect(SameOccupancy(previousOccupancy, previousBefore), "pipeline should not mutate previous occupancy");
	Expect(SameRegistry(apply.registry, applyBefore.registry), "occupancy refresh should not mutate apply result registry");
	Expect(report.dirtyTiles == reportBefore.dirtyTiles, "occupancy refresh should not mutate report");
	Expect(work.items.size() == workBefore.items.size(), "occupancy refresh should not mutate work items");
	Expect(work.dirtyTiles == workBefore.dirtyTiles, "occupancy refresh should not mutate work dirty tiles");
}

} // namespace

int main()
{
	TestSuccessfulMovementRefreshesOccupancy();
	TestBlockedMovementDoesNotRefreshOccupancy();
	TestMixedMovementAndBlockedFrameRefreshesSuccessfulMovementOnly();
	TestDuplicateFinalOccupancyRemainsInspectable();
	TestPipelineInputsAreNotMutated();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
