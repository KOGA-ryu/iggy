#include <cstdlib>
#include <vector>

#include "scene/npc/NpcActorMovementFrameApply2D.hpp"
#include "scene/npc/NpcActorMovementFrameReport2D.hpp"
#include "scene/npc/NpcActorMovementRefreshWork2D.hpp"
#include "scene/npc/NpcActorVisualRefresh2D.hpp"
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
		Id("profile:visual-refresh"),
		Id("faction:visual-refresh"),
		position,
		Id("goal:visual-refresh"),
		present,
	};
}

iggy::NpcActorState2DRegistry Actors(std::vector<iggy::NpcActorState2D> actors)
{
	const iggy::NpcActorState2DRegistryBuildResult result =
		iggy::NpcActorState2DRegistryBuilder {}.build(actors);
	Expect(result.built, "visual refresh actor registry fixture should build");
	return result.registry;
}

iggy::NpcActorMovementRefreshWork2DItem Item(
	iggy::NpcActorMovementRefreshWork2DType type,
	std::vector<iggy::TileCoord> dirtyTiles)
{
	iggy::NpcActorMovementRefreshWork2DItem item;
	item.type = type;
	item.dirtyTiles = dirtyTiles;
	return item;
}

iggy::NpcActorMovementRefreshWork2D Work(
	std::vector<iggy::NpcActorMovementRefreshWork2DItem> items)
{
	iggy::NpcActorMovementRefreshWork2D work;
	work.items = items;
	for (const iggy::NpcActorMovementRefreshWork2DItem &item : items) {
		if (item.type == iggy::NpcActorMovementRefreshWork2DType::RenderRefresh) {
			work.needsRenderRefresh = true;
		}
		if (item.type == iggy::NpcActorMovementRefreshWork2DType::VisibilityRefresh) {
			work.needsVisibilityRefresh = true;
		}
		for (iggy::TileCoord tile : item.dirtyTiles) {
			bool found = false;
			for (iggy::TileCoord existing : work.dirtyTiles) {
				if (existing == tile) {
					found = true;
					break;
				}
			}
			if (!found) {
				work.dirtyTiles.push_back(tile);
			}
		}
	}
	return work;
}

iggy::NpcActorPathStepOccupancyFilter2D Filter(
	const char *npcId,
	iggy::Vec2 oldPosition,
	iggy::Vec2 proposedPosition,
	iggy::NpcActorPathStepOccupancyFilter2DStatus status,
	bool requestsMovement,
	const char *blockingNpcId = "")
{
	iggy::NpcActorPathStepOccupancyFilter2D filter;
	filter.step.npcId = Id(npcId);
	filter.step.oldPosition = oldPosition;
	filter.step.proposedPosition = proposedPosition;
	filter.step.oldTile = iggy::tileForPoint(oldPosition);
	filter.step.proposedTile = iggy::tileForPoint(proposedPosition);
	filter.step.moveMode = iggy::NpcMoveMode::Walk;
	filter.step.status = iggy::NpcActorPathStep2DStatus::Proposed;
	filter.step.requestsMovement = true;
	filter.status = status;
	filter.requestsMovement = requestsMovement;
	if (blockingNpcId[0] != '\0') {
		filter.blockingNpcId = Id(blockingNpcId);
	}
	return filter;
}

iggy::NpcActorMovementFrameApply2DRequest Request(
	const iggy::NpcActorPathStepOccupancyFilter2D &filter)
{
	iggy::NpcActorMovementFrameApply2DRequest request;
	request.filter = filter;
	return request;
}

bool SameActors(
	const iggy::NpcActorState2DRegistry &actual,
	const iggy::NpcActorState2DRegistry &expected)
{
	if (actual.actors.size() != expected.actors.size()) {
		return false;
	}
	for (std::size_t index = 0; index < actual.actors.size(); ++index) {
		const iggy::NpcActorState2D &left = actual.actors[index];
		const iggy::NpcActorState2D &right = expected.actors[index];
		if (left.npcId != right.npcId
			|| !NearVec(left.position, right.position)
			|| left.present != right.present) {
			return false;
		}
	}
	return true;
}

void TestNoMatchingWorkReturnsNoRefresh()
{
	const iggy::NpcActorMovementRefreshWork2D work = Work({
		Item(iggy::NpcActorMovementRefreshWork2DType::InteractionRefresh, { { 0, 0 } }),
	});
	const iggy::NpcActorState2DRegistry actors = Actors({ Actor("npc:one", { 0.5F, 0.5F }) });

	const iggy::NpcActorVisualRefresh2DResult result =
		iggy::NpcActorVisualRefresher2D {}.refresh(work, actors);

	Expect(!result.hasWork(), "non-visual work should report no visual work");
	Expect(result.render.status == iggy::NpcActorVisualRefresh2DStatus::NoRefreshNeeded, "non-render work should not refresh render");
	Expect(result.visibility.status == iggy::NpcActorVisualRefresh2DStatus::NoRefreshNeeded, "non-visibility work should not refresh visibility");
	Expect(result.render.dirtyTiles.empty() && result.visibility.dirtyTiles.empty(), "non-visual work should not copy dirty tiles into visual packets");
}

void TestVisibilityRefreshProducesOnlyVisibilityPacket()
{
	const iggy::NpcActorMovementRefreshWork2D work = Work({
		Item(iggy::NpcActorMovementRefreshWork2DType::VisibilityRefresh, { { 0, 0 }, { 1, 0 } }),
	});
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:old", { 0.5F, 0.5F }),
		Actor("npc:new", { 1.5F, 0.5F }),
	});

	const iggy::NpcActorVisualRefresh2DResult result =
		iggy::NpcActorVisualRefresher2D {}.refresh(work, actors);

	Expect(result.visibility.status == iggy::NpcActorVisualRefresh2DStatus::Refreshed, "visibility work should refresh visibility packet");
	Expect(result.visibility.dirtyTileCount == 2, "visibility packet should preserve dirty tile count");
	Expect(result.visibility.affectedActorCount == 2, "visibility packet should report actors on dirty tiles");
	Expect(result.render.status == iggy::NpcActorVisualRefresh2DStatus::NoRefreshNeeded, "visibility work should not create render packet work");
	Expect(result.render.dirtyTiles.empty(), "visibility work should leave render dirty tiles empty");
}

void TestRenderRefreshProducesOnlyRenderPacket()
{
	const iggy::NpcActorMovementRefreshWork2D work = Work({
		Item(iggy::NpcActorMovementRefreshWork2DType::RenderRefresh, { { 2, 0 } }),
	});
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:render", { 2.5F, 0.5F }),
	});

	const iggy::NpcActorVisualRefresh2DResult result =
		iggy::NpcActorVisualRefresher2D {}.refresh(work, actors);

	Expect(result.render.status == iggy::NpcActorVisualRefresh2DStatus::Refreshed, "render work should refresh render packet");
	Expect(result.render.dirtyTiles.size() == 1 && result.render.dirtyTiles[0] == iggy::TileCoord { 2, 0 }, "render packet should preserve render dirty tile");
	Expect(result.render.affectedActorCount == 1 && result.render.affectedActors[0].npcId == Id("npc:render"), "render packet should report actor on dirty tile");
	Expect(result.visibility.status == iggy::NpcActorVisualRefresh2DStatus::NoRefreshNeeded, "render work should not create visibility packet work");
}

void TestCombinedWorkProducesIndependentPackets()
{
	const iggy::NpcActorMovementRefreshWork2D work = Work({
		Item(iggy::NpcActorMovementRefreshWork2DType::RenderRefresh, { { 0, 0 } }),
		Item(iggy::NpcActorMovementRefreshWork2DType::VisibilityRefresh, { { 1, 0 } }),
	});
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:render", { 0.5F, 0.5F }),
		Actor("npc:visibility", { 1.5F, 0.5F }),
	});

	const iggy::NpcActorVisualRefresh2DResult result =
		iggy::NpcActorVisualRefresher2D {}.refresh(work, actors);

	Expect(result.hasWork(), "combined visual work should report work");
	Expect(result.render.dirtyTiles.size() == 1 && result.render.dirtyTiles[0] == iggy::TileCoord { 0, 0 }, "render packet should keep render dirty tiles separate");
	Expect(result.visibility.dirtyTiles.size() == 1 && result.visibility.dirtyTiles[0] == iggy::TileCoord { 1, 0 }, "visibility packet should keep visibility dirty tiles separate");
	Expect(result.render.affectedActors.size() == 1 && result.render.affectedActors[0].npcId == Id("npc:render"), "render packet should only report render dirty actor");
	Expect(result.visibility.affectedActors.size() == 1 && result.visibility.affectedActors[0].npcId == Id("npc:visibility"), "visibility packet should only report visibility dirty actor");
}

void TestDirtyTileOrderAndActorOrderAreDeterministic()
{
	const iggy::NpcActorMovementRefreshWork2D work = Work({
		Item(iggy::NpcActorMovementRefreshWork2DType::RenderRefresh, { { 1, 0 }, { 1, 0 } }),
		Item(iggy::NpcActorMovementRefreshWork2DType::VisibilityRefresh, { { 4, 0 } }),
		Item(iggy::NpcActorMovementRefreshWork2DType::RenderRefresh, { { 2, 0 }, { 1, 0 } }),
	});
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:tile-two", { 2.5F, 0.5F }),
		Actor("npc:tile-one", { 1.5F, 0.5F }),
		Actor("npc:tile-four", { 4.5F, 0.5F }),
	});

	const iggy::NpcActorVisualRefresh2DResult result =
		iggy::NpcActorVisualRefresher2D {}.refresh(work, actors);

	Expect(result.render.dirtyTiles.size() == 2 && result.render.dirtyTiles[0] == iggy::TileCoord { 1, 0 } && result.render.dirtyTiles[1] == iggy::TileCoord { 2, 0 }, "render packet should dedupe dirty tiles in item order");
	Expect(result.render.affectedActors.size() == 2
		&& result.render.affectedActors[0].npcId == Id("npc:tile-two")
		&& result.render.affectedActors[1].npcId == Id("npc:tile-one"), "render packet should preserve actor registry order");
	Expect(result.visibility.dirtyTiles.size() == 1 && result.visibility.dirtyTiles[0] == iggy::TileCoord { 4, 0 }, "visibility packet should ignore render dirty tiles");
}

void TestAbsentActorsAreConfigurableAndIdsAreExact()
{
	const iggy::NpcActorMovementRefreshWork2D work = Work({
		Item(iggy::NpcActorMovementRefreshWork2DType::RenderRefresh, { { 0, 0 } }),
		Item(iggy::NpcActorMovementRefreshWork2DType::VisibilityRefresh, { { 0, 0 } }),
	});
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc", { 0.5F, 0.5F }, false),
		Actor("npc:qualified", { 0.5F, 0.5F }),
		Actor("npc:other", { 2.5F, 0.5F }),
	});

	const iggy::NpcActorVisualRefresh2DResult defaultResult =
		iggy::NpcActorVisualRefresher2D {}.refresh(work, actors);
	iggy::NpcActorVisualRefresh2DConfig config;
	config.includeAbsentActors = true;
	const iggy::NpcActorVisualRefresh2DResult included =
		iggy::NpcActorVisualRefresher2D {}.refresh(work, actors, config);

	Expect(defaultResult.render.affectedActors.size() == 1 && defaultResult.render.affectedActors[0].npcId == Id("npc:qualified"), "visual refresh should ignore absent actors by default and preserve exact ids");
	Expect(included.render.affectedActors.size() == 2
		&& included.render.affectedActors[0].npcId == Id("npc")
		&& included.render.affectedActors[1].npcId == Id("npc:qualified"), "visual refresh config should include absent actors in registry order");
	Expect(included.visibility.affectedActors.size() == 2, "visibility packet should use the same absent actor policy");
}

void TestInputsAreNotMutated()
{
	const iggy::NpcActorMovementRefreshWork2D work = Work({
		Item(iggy::NpcActorMovementRefreshWork2DType::RenderRefresh, { { 0, 0 } }),
		Item(iggy::NpcActorMovementRefreshWork2DType::VisibilityRefresh, { { 1, 0 } }),
	});
	const iggy::NpcActorMovementRefreshWork2D workBefore = work;
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:immutable", { 0.5F, 0.5F }),
	});
	const iggy::NpcActorState2DRegistry actorsBefore = actors;

	(void)iggy::NpcActorVisualRefresher2D {}.refresh(work, actors);

	Expect(work.items.size() == workBefore.items.size() && work.dirtyTiles == workBefore.dirtyTiles, "visual refresh should not mutate work");
	Expect(SameActors(actors, actorsBefore), "visual refresh should not mutate actors");
}

void TestMovementApplyReportWorkAcceptanceRefreshesVisualPackets()
{
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:mover", { 0.5F, 0.5F }),
	});
	const iggy::NpcActorMovementFrameApply2DResult apply =
		iggy::NpcActorMovementFrameApplier2D {}.apply(
			actors,
			{ Request(Filter(
				"npc:mover",
				{ 0.5F, 0.5F },
				{ 1.5F, 0.5F },
				iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed,
				true)) });
	const iggy::NpcActorMovementFrameReport2D report =
		iggy::NpcActorMovementFrameReporter2D {}.report(apply);
	const iggy::NpcActorMovementRefreshWork2D work =
		iggy::NpcActorMovementRefreshWorkProjector2D {}.project(report);

	const iggy::NpcActorVisualRefresh2DResult result =
		iggy::NpcActorVisualRefresher2D {}.refresh(work, apply.registry);

	Expect(work.needsRenderRefresh && work.needsVisibilityRefresh, "moved actor should request render and visibility refresh work");
	Expect(result.render.status == iggy::NpcActorVisualRefresh2DStatus::Refreshed, "movement refresh chain should refresh render packet");
	Expect(result.visibility.status == iggy::NpcActorVisualRefresh2DStatus::Refreshed, "movement refresh chain should refresh visibility packet");
	Expect(result.render.dirtyTiles.size() == 2 && result.render.dirtyTiles[0] == iggy::TileCoord { 0, 0 } && result.render.dirtyTiles[1] == iggy::TileCoord { 1, 0 }, "render packet should preserve old/new dirty tiles");
	Expect(result.visibility.dirtyTiles.size() == 2 && result.visibility.dirtyTiles[0] == iggy::TileCoord { 0, 0 } && result.visibility.dirtyTiles[1] == iggy::TileCoord { 1, 0 }, "visibility packet should preserve old/new dirty tiles");
	Expect(result.render.affectedActorCount == 1 && result.render.affectedActors[0].npcId == Id("npc:mover"), "render packet should report moved actor on dirty tile");
	Expect(result.visibility.affectedActorCount == 1 && result.visibility.affectedActors[0].npcId == Id("npc:mover"), "visibility packet should report moved actor on dirty tile");
}

void TestBlockedMovementProducesNoVisualRefresh()
{
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:mover", { 0.5F, 0.5F }),
		Actor("npc:blocker", { 1.5F, 0.5F }),
	});
	const iggy::NpcActorMovementFrameApply2DResult apply =
		iggy::NpcActorMovementFrameApplier2D {}.apply(
			actors,
			{ Request(Filter(
				"npc:mover",
				{ 0.5F, 0.5F },
				{ 1.5F, 0.5F },
				iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc,
				false,
				"npc:blocker")) });
	const iggy::NpcActorMovementFrameReport2D report =
		iggy::NpcActorMovementFrameReporter2D {}.report(apply);
	const iggy::NpcActorMovementRefreshWork2D work =
		iggy::NpcActorMovementRefreshWorkProjector2D {}.project(report);

	const iggy::NpcActorVisualRefresh2DResult result =
		iggy::NpcActorVisualRefresher2D {}.refresh(work, apply.registry);

	Expect(!work.needsRenderRefresh && !work.needsVisibilityRefresh, "blocked movement should not request visual refresh work");
	Expect(!result.hasWork(), "blocked movement refresh chain should not produce visual packets");
	Expect(result.render.affectedActors.empty() && result.visibility.affectedActors.empty(), "blocked movement refresh chain should not report affected visual actors");
}

} // namespace

int main()
{
	TestNoMatchingWorkReturnsNoRefresh();
	TestVisibilityRefreshProducesOnlyVisibilityPacket();
	TestRenderRefreshProducesOnlyRenderPacket();
	TestCombinedWorkProducesIndependentPackets();
	TestDirtyTileOrderAndActorOrderAreDeterministic();
	TestAbsentActorsAreConfigurableAndIdsAreExact();
	TestInputsAreNotMutated();
	TestMovementApplyReportWorkAcceptanceRefreshesVisualPackets();
	TestBlockedMovementProducesNoVisualRefresh();

	return Failures;
}
