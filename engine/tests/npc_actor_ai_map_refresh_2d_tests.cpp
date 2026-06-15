#include <cstdlib>
#include <vector>

#include "scene/npc/NpcActorAiMapRefresh2D.hpp"
#include "scene/npc/NpcActorMovementFrameApply2D.hpp"
#include "scene/npc/NpcActorMovementFrameReport2D.hpp"
#include "scene/npc/NpcActorMovementRefreshWork2D.hpp"
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
		Id("profile:ai-map-refresh"),
		Id("faction:ai-map-refresh"),
		position,
		Id("goal:ai-map-refresh"),
		present,
	};
}

iggy::NpcActorState2DRegistry Actors(std::vector<iggy::NpcActorState2D> actors)
{
	const iggy::NpcActorState2DRegistryBuildResult result =
		iggy::NpcActorState2DRegistryBuilder {}.build(actors);
	Expect(result.built, "ai map refresh actor registry fixture should build");
	return result.registry;
}

iggy::AiMapNode2D Node(
	const char *nodeId,
	iggy::Vec2 position,
	float radius,
	std::vector<iggy::ResourceId> tags = {},
	bool enabled = true,
	float patrolWeight = 0.0F,
	float coverWeight = 0.0F,
	float dangerWeight = 0.0F,
	float interestWeight = 0.0F)
{
	return {
		Id(nodeId),
		position,
		radius,
		patrolWeight,
		coverWeight,
		dangerWeight,
		interestWeight,
		tags,
		{},
		enabled,
	};
}

iggy::AiMap2D Map(std::vector<iggy::AiMapNode2D> nodes)
{
	return { nodes };
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
		if (item.type == iggy::NpcActorMovementRefreshWork2DType::AiMapQueryRefresh) {
			work.needsAiMapQueryRefresh = true;
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

bool SameNodes(const std::vector<iggy::AiMapNode2D> &actual, const std::vector<iggy::AiMapNode2D> &expected)
{
	if (actual.size() != expected.size()) {
		return false;
	}
	for (std::size_t index = 0; index < actual.size(); ++index) {
		const iggy::AiMapNode2D &left = actual[index];
		const iggy::AiMapNode2D &right = expected[index];
		if (left.id != right.id
			|| !NearVec(left.position, right.position)
			|| left.radius != right.radius
			|| left.tags != right.tags
			|| left.enabled != right.enabled
			|| left.patrolWeight != right.patrolWeight
			|| left.coverWeight != right.coverWeight
			|| left.dangerWeight != right.dangerWeight
			|| left.interestWeight != right.interestWeight) {
			return false;
		}
	}
	return true;
}

void TestNoAiMapRefreshWorkReturnsNoRefresh()
{
	const iggy::NpcActorMovementRefreshWork2D work = Work({
		Item(iggy::NpcActorMovementRefreshWork2DType::InteractionRefresh, { { 0, 0 } }),
	});
	const iggy::NpcActorState2DRegistry actors = Actors({ Actor("npc:one", { 0.5F, 0.5F }) });
	const iggy::AiMap2D map = Map({ Node("ai:one", { 0.5F, 0.5F }, 1.0F, { Id("tag:one") }) });

	const iggy::NpcActorAiMapRefresh2DResult result =
		iggy::NpcActorAiMapRefresher2D {}.refresh(work, actors, map);

	Expect(result.status == iggy::NpcActorAiMapRefresh2DStatus::NoRefreshNeeded, "non-ai-map work should not refresh ai map queries");
	Expect(!result.hasWork(), "non-ai-map work should report no work");
	Expect(result.dirtyTiles.empty(), "non-ai-map work should not copy dirty tiles into ai map result");
	Expect(result.affectedActors.empty(), "non-ai-map work should report no actors");
}

void TestAiMapRefreshReportsPresentActorsOnDirtyTiles()
{
	const iggy::NpcActorMovementRefreshWork2D work = Work({
		Item(iggy::NpcActorMovementRefreshWork2DType::AiMapQueryRefresh, { { 0, 0 }, { 1, 0 } }),
	});
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:old", { 0.5F, 0.5F }),
		Actor("npc:new", { 1.5F, 0.5F }),
		Actor("npc:other", { 3.5F, 0.5F }),
	});
	const iggy::AiMap2D map = Map({
		Node("ai:old", { 0.5F, 0.5F }, 0.25F, { Id("tag:old") }),
		Node("ai:new", { 1.5F, 0.5F }, 0.25F, { Id("tag:new") }),
	});

	const iggy::NpcActorAiMapRefresh2DResult result =
		iggy::NpcActorAiMapRefresher2D {}.refresh(work, actors, map);

	Expect(result.status == iggy::NpcActorAiMapRefresh2DStatus::Refreshed, "ai map work should refresh");
	Expect(result.hasWork(), "ai map work should report work");
	Expect(result.dirtyTiles.size() == 2 && result.dirtyTiles[0] == iggy::TileCoord { 0, 0 } && result.dirtyTiles[1] == iggy::TileCoord { 1, 0 }, "ai map refresh should preserve dirty tile order");
	Expect(result.affectedActorCount == 2, "ai map refresh should report actors on dirty tiles");
	Expect(result.affectedActors[0].npcId == Id("npc:old") && result.affectedActors[1].npcId == Id("npc:new"), "ai map refresh should preserve actor registry order");
	Expect(result.affectedActors[1].query.tags == std::vector<iggy::ResourceId>({ Id("tag:new") }), "ai map refresh should preserve nested query tags");
}

void TestAbsentActorsAreConfigurable()
{
	const iggy::NpcActorMovementRefreshWork2D work = Work({
		Item(iggy::NpcActorMovementRefreshWork2DType::AiMapQueryRefresh, { { 0, 0 } }),
	});
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:absent", { 0.5F, 0.5F }, false),
	});
	const iggy::AiMap2D map = Map({ Node("ai:absent", { 0.5F, 0.5F }, 1.0F, { Id("tag:absent") }) });

	const iggy::NpcActorAiMapRefresh2DResult defaultResult =
		iggy::NpcActorAiMapRefresher2D {}.refresh(work, actors, map);
	iggy::NpcActorAiMapRefresh2DConfig config;
	config.includeAbsentActors = true;
	const iggy::NpcActorAiMapRefresh2DResult included =
		iggy::NpcActorAiMapRefresher2D {}.refresh(work, actors, map, config);

	Expect(defaultResult.affectedActors.empty(), "ai map refresh should ignore absent actors by default");
	Expect(included.affectedActorCount == 1 && included.affectedActors[0].npcId == Id("npc:absent"), "ai map refresh config should include absent actors");
}

void TestNestedQueryExcludesDisabledMapNodes()
{
	const iggy::NpcActorMovementRefreshWork2D work = Work({
		Item(iggy::NpcActorMovementRefreshWork2DType::AiMapQueryRefresh, { { 0, 0 } }),
	});
	const iggy::NpcActorState2DRegistry actors = Actors({ Actor("npc:query", { 0.5F, 0.5F }) });
	const iggy::AiMap2D map = Map({
		Node("ai:disabled", { 0.5F, 0.5F }, 1.0F, { Id("tag:disabled") }, false, 9.0F),
		Node("ai:enabled", { 0.5F, 0.5F }, 1.0F, { Id("tag:enabled") }, true, 1.0F, 2.0F, 3.0F, 4.0F),
	});

	const iggy::NpcActorAiMapRefresh2DResult result =
		iggy::NpcActorAiMapRefresher2D {}.refresh(work, actors, map);

	Expect(result.affectedActorCount == 1, "ai map refresh query setup should report one actor");
	if (result.affectedActorCount == 1) {
		const iggy::AiMapQuery2DResult &query = result.affectedActors[0].query;
		Expect(query.status == iggy::AiMapQuery2DStatus::Matched, "ai map refresh should preserve query match");
		Expect(query.entries.size() == 1 && query.entries[0].node.id == Id("ai:enabled"), "ai map refresh query should exclude disabled node");
		Expect(query.tags == std::vector<iggy::ResourceId>({ Id("tag:enabled") }), "ai map refresh query should preserve enabled node tags");
		Expect(query.patrolWeight == 1.0F && query.coverWeight == 2.0F && query.dangerWeight == 3.0F && query.interestWeight == 4.0F, "ai map refresh query should preserve weights");
	}
}

void TestRepeatedDirtyTilesAndMultipleItemsAreDeterministic()
{
	const iggy::NpcActorMovementRefreshWork2D work = Work({
		Item(iggy::NpcActorMovementRefreshWork2DType::AiMapQueryRefresh, { { 1, 0 }, { 1, 0 } }),
		Item(iggy::NpcActorMovementRefreshWork2DType::RenderRefresh, { { 9, 0 } }),
		Item(iggy::NpcActorMovementRefreshWork2DType::AiMapQueryRefresh, { { 2, 0 }, { 1, 0 } }),
	});
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:tile-two", { 2.5F, 0.5F }),
		Actor("npc:tile-one", { 1.5F, 0.5F }),
	});

	const iggy::NpcActorAiMapRefresh2DResult result =
		iggy::NpcActorAiMapRefresher2D {}.refresh(work, actors, {});

	Expect(result.dirtyTiles.size() == 2 && result.dirtyTiles[0] == iggy::TileCoord { 1, 0 } && result.dirtyTiles[1] == iggy::TileCoord { 2, 0 }, "ai map refresh should dedupe dirty tiles in item order");
	Expect(result.affectedActors.size() == 2
		&& result.affectedActors[0].npcId == Id("npc:tile-two")
		&& result.affectedActors[1].npcId == Id("npc:tile-one"), "ai map refresh should preserve actor registry order");
}

void TestQueryCanBeDisabledForRequestFactsOnly()
{
	const iggy::NpcActorMovementRefreshWork2D work = Work({
		Item(iggy::NpcActorMovementRefreshWork2DType::AiMapQueryRefresh, { { 0, 0 } }),
	});
	const iggy::NpcActorState2DRegistry actors = Actors({ Actor("npc:request", { 0.5F, 0.5F }) });
	const iggy::AiMap2D map = Map({ Node("ai:request", { 0.5F, 0.5F }, 1.0F, { Id("tag:request") }) });
	iggy::NpcActorAiMapRefresh2DConfig config;
	config.queryAffectedActors = false;

	const iggy::NpcActorAiMapRefresh2DResult result =
		iggy::NpcActorAiMapRefresher2D {}.refresh(work, actors, map, config);

	Expect(result.affectedActorCount == 1, "query-disabled ai map refresh should still report affected actor");
	Expect(result.affectedActors[0].query.status == iggy::AiMapQuery2DStatus::NoMatch, "query-disabled ai map refresh should leave default query status");
	Expect(NearVec(result.affectedActors[0].query.position, { 0.5F, 0.5F }), "query-disabled ai map refresh should preserve query request position");
	Expect(result.affectedActors[0].query.entries.empty(), "query-disabled ai map refresh should not run nested query");
}

void TestInputsAreNotMutated()
{
	const iggy::NpcActorMovementRefreshWork2D work = Work({
		Item(iggy::NpcActorMovementRefreshWork2DType::AiMapQueryRefresh, { { 0, 0 } }),
	});
	const iggy::NpcActorMovementRefreshWork2D workBefore = work;
	const iggy::NpcActorState2DRegistry actors = Actors({ Actor("npc:immutable", { 0.5F, 0.5F }) });
	const iggy::NpcActorState2DRegistry actorsBefore = actors;
	const iggy::AiMap2D map = Map({ Node("ai:immutable", { 0.5F, 0.5F }, 1.0F, { Id("tag:immutable") }) });
	const std::vector<iggy::AiMapNode2D> mapBefore = map.nodes;

	(void)iggy::NpcActorAiMapRefresher2D {}.refresh(work, actors, map);

	Expect(work.items.size() == workBefore.items.size() && work.dirtyTiles == workBefore.dirtyTiles, "ai map refresh should not mutate work");
	Expect(SameActors(actors, actorsBefore), "ai map refresh should not mutate actors");
	Expect(SameNodes(map.nodes, mapBefore), "ai map refresh should not mutate map");
}

void TestMovementApplyReportWorkAcceptanceRefreshesAiMap()
{
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:mover", { 0.5F, 0.5F }),
	});
	const iggy::AiMap2D map = Map({
		Node("ai:new-tile", { 1.5F, 0.5F }, 1.0F, { Id("tag:new-tile") }, true, 2.0F),
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

	const iggy::NpcActorAiMapRefresh2DResult result =
		iggy::NpcActorAiMapRefresher2D {}.refresh(work, apply.registry, map);

	Expect(work.needsAiMapQueryRefresh, "moved actor should request ai map query refresh work");
	Expect(result.status == iggy::NpcActorAiMapRefresh2DStatus::Refreshed, "movement refresh chain should refresh ai map queries");
	Expect(result.dirtyTiles.size() == 2 && result.dirtyTiles[0] == iggy::TileCoord { 0, 0 } && result.dirtyTiles[1] == iggy::TileCoord { 1, 0 }, "movement refresh chain should preserve old/new dirty tiles");
	Expect(result.affectedActorCount == 1 && result.affectedActors[0].npcId == Id("npc:mover"), "movement refresh chain should report moved actor on new dirty tile");
	Expect(result.affectedActors[0].query.tags == std::vector<iggy::ResourceId>({ Id("tag:new-tile") }), "movement refresh chain should preserve nested ai map query facts");
}

void TestBlockedMovementProducesNoAiMapRefresh()
{
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:mover", { 0.5F, 0.5F }),
		Actor("npc:blocker", { 1.5F, 0.5F }),
	});
	const iggy::AiMap2D map = Map({ Node("ai:block", { 1.5F, 0.5F }, 1.0F, { Id("tag:block") }) });
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

	const iggy::NpcActorAiMapRefresh2DResult result =
		iggy::NpcActorAiMapRefresher2D {}.refresh(work, apply.registry, map);

	Expect(!work.needsAiMapQueryRefresh, "blocked movement should not request ai map refresh work");
	Expect(result.status == iggy::NpcActorAiMapRefresh2DStatus::NoRefreshNeeded, "blocked movement refresh chain should not refresh ai map queries");
	Expect(result.affectedActors.empty(), "blocked movement refresh chain should not report affected actors");
}

} // namespace

int main()
{
	TestNoAiMapRefreshWorkReturnsNoRefresh();
	TestAiMapRefreshReportsPresentActorsOnDirtyTiles();
	TestAbsentActorsAreConfigurable();
	TestNestedQueryExcludesDisabledMapNodes();
	TestRepeatedDirtyTilesAndMultipleItemsAreDeterministic();
	TestQueryCanBeDisabledForRequestFactsOnly();
	TestInputsAreNotMutated();
	TestMovementApplyReportWorkAcceptanceRefreshesAiMap();
	TestBlockedMovementProducesNoAiMapRefresh();

	return Failures;
}
