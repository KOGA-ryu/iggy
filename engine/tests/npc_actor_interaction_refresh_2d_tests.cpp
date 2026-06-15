#include <cstdlib>
#include <vector>

#include "scene/npc/NpcActorInteractionRefresh2D.hpp"
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
		Id("profile:interaction-refresh"),
		Id("faction:interaction-refresh"),
		position,
		Id("goal:interaction-refresh"),
		present,
	};
}

iggy::NpcActorState2DRegistry Actors(std::vector<iggy::NpcActorState2D> actors)
{
	const iggy::NpcActorState2DRegistryBuildResult result =
		iggy::NpcActorState2DRegistryBuilder {}.build(actors);
	Expect(result.built, "interaction refresh actor registry fixture should build");
	return result.registry;
}

iggy::InteractionTarget2D Target(
	const char *targetId,
	iggy::Vec2 position,
	bool enabled = true,
	iggy::InteractionTarget2DKind kind = iggy::InteractionTarget2DKind::Usable)
{
	return {
		Id(targetId),
		kind,
		position,
		0.5F,
		enabled,
	};
}

iggy::InteractionTarget2DRegistry Targets(std::vector<iggy::InteractionTarget2D> targets)
{
	const iggy::InteractionTarget2DRegistryBuildResult result =
		iggy::InteractionTarget2DRegistryBuilder {}.build(targets);
	Expect(result.built, "interaction refresh target registry fixture should build");
	return result.registry;
}

iggy::NpcActorMovementRefreshWork2D Work(
	std::vector<iggy::NpcActorMovementRefreshWork2DItem> items)
{
	iggy::NpcActorMovementRefreshWork2D work;
	work.items = items;
	for (const iggy::NpcActorMovementRefreshWork2DItem &item : items) {
		if (item.type == iggy::NpcActorMovementRefreshWork2DType::InteractionRefresh) {
			work.needsInteractionRefresh = true;
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

iggy::NpcActorMovementRefreshWork2DItem Item(
	iggy::NpcActorMovementRefreshWork2DType type,
	std::vector<iggy::TileCoord> dirtyTiles)
{
	iggy::NpcActorMovementRefreshWork2DItem item;
	item.type = type;
	item.dirtyTiles = dirtyTiles;
	return item;
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

bool SameWork(
	const iggy::NpcActorMovementRefreshWork2D &actual,
	const iggy::NpcActorMovementRefreshWork2D &expected)
{
	if (actual.items.size() != expected.items.size()
		|| actual.dirtyTiles.size() != expected.dirtyTiles.size()
		|| actual.needsInteractionRefresh != expected.needsInteractionRefresh) {
		return false;
	}
	for (std::size_t index = 0; index < actual.dirtyTiles.size(); ++index) {
		if (!(actual.dirtyTiles[index] == expected.dirtyTiles[index])) {
			return false;
		}
	}
	return true;
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

bool SameTargets(
	const iggy::InteractionTarget2DRegistry &actual,
	const iggy::InteractionTarget2DRegistry &expected)
{
	if (actual.targets().size() != expected.targets().size()) {
		return false;
	}
	for (std::size_t index = 0; index < actual.targets().size(); ++index) {
		const iggy::InteractionTarget2D &left = actual.targets()[index];
		const iggy::InteractionTarget2D &right = expected.targets()[index];
		if (left.id != right.id
			|| left.kind != right.kind
			|| !NearVec(left.position, right.position)
			|| left.radius != right.radius
			|| left.enabled != right.enabled) {
			return false;
		}
	}
	return true;
}

void TestNoInteractionRefreshWorkReturnsNoRefresh()
{
	const iggy::NpcActorMovementRefreshWork2D work = Work({
		Item(iggy::NpcActorMovementRefreshWork2DType::OccupancyRebuild, { { 0, 0 } }),
	});
	const iggy::NpcActorState2DRegistry actors = Actors({ Actor("npc:one", { 0.5F, 0.5F }) });
	const iggy::InteractionTarget2DRegistry targets = Targets({ Target("target:one", { 0.5F, 0.5F }) });

	const iggy::NpcActorInteractionRefresh2DResult result =
		iggy::NpcActorInteractionRefresher2D {}.refresh(work, actors, targets);

	Expect(result.status == iggy::NpcActorInteractionRefresh2DStatus::NoRefreshNeeded, "non-interaction work should not refresh interactions");
	Expect(!result.hasWork(), "non-interaction work should report no work");
	Expect(result.dirtyTiles.empty(), "non-interaction work should not copy dirty tiles into interaction result");
	Expect(result.affectedActors.empty() && result.affectedTargets.empty(), "non-interaction work should not report affected facts");
}

void TestInteractionRefreshPreservesDirtyTilesAndAffectedFacts()
{
	const iggy::NpcActorMovementRefreshWork2D work = Work({
		Item(iggy::NpcActorMovementRefreshWork2DType::InteractionRefresh, { { 0, 0 }, { 1, 0 } }),
	});
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:old-absent", { 0.5F, 0.5F }, false),
		Actor("npc:new", { 1.5F, 0.5F }),
		Actor("npc:other", { 3.5F, 0.5F }),
	});
	const iggy::InteractionTarget2DRegistry targets = Targets({
		Target("target:disabled-old", { 0.5F, 0.5F }, false),
		Target("target:new", { 1.5F, 0.5F }, true),
		Target("target:other", { 3.5F, 0.5F }, true),
	});

	const iggy::NpcActorInteractionRefresh2DResult result =
		iggy::NpcActorInteractionRefresher2D {}.refresh(work, actors, targets);

	Expect(result.status == iggy::NpcActorInteractionRefresh2DStatus::Refreshed, "interaction work should refresh");
	Expect(result.hasWork(), "interaction work should report work");
	Expect(result.dirtyTiles.size() == 2 && result.dirtyTiles[0] == iggy::TileCoord { 0, 0 } && result.dirtyTiles[1] == iggy::TileCoord { 1, 0 }, "interaction refresh should preserve dirty tile order");
	Expect(result.affectedActorCount == 1 && result.affectedActors[0].npcId == Id("npc:new"), "interaction refresh should report present actors on dirty tiles");
	Expect(result.affectedTargetCount == 1 && result.affectedTargets[0].targetId == Id("target:new"), "interaction refresh should report enabled targets on dirty tiles");
}

void TestConfigCanIncludeAbsentActorsAndDisabledTargets()
{
	const iggy::NpcActorMovementRefreshWork2D work = Work({
		Item(iggy::NpcActorMovementRefreshWork2DType::InteractionRefresh, { { 0, 0 } }),
	});
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:absent", { 0.5F, 0.5F }, false),
	});
	const iggy::InteractionTarget2DRegistry targets = Targets({
		Target("target:disabled", { 0.5F, 0.5F }, false),
	});
	iggy::NpcActorInteractionRefresh2DConfig config;
	config.includeAbsentActors = true;
	config.includeDisabledTargets = true;

	const iggy::NpcActorInteractionRefresh2DResult result =
		iggy::NpcActorInteractionRefresher2D {}.refresh(work, actors, targets, config);

	Expect(result.affectedActorCount == 1 && result.affectedActors[0].npcId == Id("npc:absent"), "config should include absent actors");
	Expect(result.affectedTargetCount == 1 && result.affectedTargets[0].targetId == Id("target:disabled"), "config should include disabled targets");
}

void TestRepeatedDirtyTilesAndMultipleItemsAreDeterministic()
{
	const iggy::NpcActorMovementRefreshWork2D work = Work({
		Item(iggy::NpcActorMovementRefreshWork2DType::InteractionRefresh, { { 1, 0 }, { 1, 0 } }),
		Item(iggy::NpcActorMovementRefreshWork2DType::RenderRefresh, { { 9, 0 } }),
		Item(iggy::NpcActorMovementRefreshWork2DType::InteractionRefresh, { { 2, 0 }, { 1, 0 } }),
	});
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:tile-two", { 2.5F, 0.5F }),
		Actor("npc:tile-one", { 1.5F, 0.5F }),
	});
	const iggy::InteractionTarget2DRegistry targets = Targets({
		Target("target:tile-two", { 2.5F, 0.5F }),
		Target("target:tile-one", { 1.5F, 0.5F }),
	});

	const iggy::NpcActorInteractionRefresh2DResult result =
		iggy::NpcActorInteractionRefresher2D {}.refresh(work, actors, targets);

	Expect(result.dirtyTiles.size() == 2 && result.dirtyTiles[0] == iggy::TileCoord { 1, 0 } && result.dirtyTiles[1] == iggy::TileCoord { 2, 0 }, "interaction refresh should dedupe dirty tiles in item order");
	Expect(result.affectedActors.size() == 2
		&& result.affectedActors[0].npcId == Id("npc:tile-two")
		&& result.affectedActors[1].npcId == Id("npc:tile-one"), "interaction refresh should preserve actor registry order");
	Expect(result.affectedTargets.size() == 2
		&& result.affectedTargets[0].targetId == Id("target:tile-two")
		&& result.affectedTargets[1].targetId == Id("target:tile-one"), "interaction refresh should preserve target registry order");
}

void TestInputsAreNotMutated()
{
	const iggy::NpcActorMovementRefreshWork2D work = Work({
		Item(iggy::NpcActorMovementRefreshWork2DType::InteractionRefresh, { { 0, 0 } }),
	});
	const iggy::NpcActorMovementRefreshWork2D workBefore = work;
	const iggy::NpcActorState2DRegistry actors = Actors({ Actor("npc:immutable", { 0.5F, 0.5F }) });
	const iggy::NpcActorState2DRegistry actorsBefore = actors;
	const iggy::InteractionTarget2DRegistry targets = Targets({ Target("target:immutable", { 0.5F, 0.5F }) });
	const iggy::InteractionTarget2DRegistry targetsBefore = targets;

	(void)iggy::NpcActorInteractionRefresher2D {}.refresh(work, actors, targets);

	Expect(SameWork(work, workBefore), "interaction refresh should not mutate work");
	Expect(SameActors(actors, actorsBefore), "interaction refresh should not mutate actors");
	Expect(SameTargets(targets, targetsBefore), "interaction refresh should not mutate targets");
}

void TestMovementApplyReportWorkAcceptanceRefreshesInteractions()
{
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:mover", { 0.5F, 0.5F }),
	});
	const iggy::InteractionTarget2DRegistry targets = Targets({
		Target("target:new-tile", { 1.5F, 0.5F }),
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

	const iggy::NpcActorInteractionRefresh2DResult result =
		iggy::NpcActorInteractionRefresher2D {}.refresh(work, apply.registry, targets);

	Expect(work.needsInteractionRefresh, "moved actor should request interaction refresh work");
	Expect(result.status == iggy::NpcActorInteractionRefresh2DStatus::Refreshed, "movement refresh chain should refresh interactions");
	Expect(result.dirtyTiles.size() == 2 && result.dirtyTiles[0] == iggy::TileCoord { 0, 0 } && result.dirtyTiles[1] == iggy::TileCoord { 1, 0 }, "movement refresh chain should preserve old/new dirty tiles");
	Expect(result.affectedActorCount == 1 && result.affectedActors[0].npcId == Id("npc:mover"), "movement refresh chain should report moved actor on new dirty tile");
	Expect(result.affectedTargetCount == 1 && result.affectedTargets[0].targetId == Id("target:new-tile"), "movement refresh chain should report target on new dirty tile");
}

void TestBlockedMovementProducesNoInteractionRefresh()
{
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:mover", { 0.5F, 0.5F }),
		Actor("npc:blocker", { 1.5F, 0.5F }),
	});
	const iggy::InteractionTarget2DRegistry targets = Targets({
		Target("target:block", { 1.5F, 0.5F }),
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

	const iggy::NpcActorInteractionRefresh2DResult result =
		iggy::NpcActorInteractionRefresher2D {}.refresh(work, apply.registry, targets);

	Expect(!work.needsInteractionRefresh, "blocked movement should not request interaction refresh work");
	Expect(result.status == iggy::NpcActorInteractionRefresh2DStatus::NoRefreshNeeded, "blocked movement refresh chain should not refresh interactions");
	Expect(result.affectedActors.empty() && result.affectedTargets.empty(), "blocked movement refresh chain should not report affected interaction facts");
}

} // namespace

int main()
{
	TestNoInteractionRefreshWorkReturnsNoRefresh();
	TestInteractionRefreshPreservesDirtyTilesAndAffectedFacts();
	TestConfigCanIncludeAbsentActorsAndDisabledTargets();
	TestRepeatedDirtyTilesAndMultipleItemsAreDeterministic();
	TestInputsAreNotMutated();
	TestMovementApplyReportWorkAcceptanceRefreshesInteractions();
	TestBlockedMovementProducesNoInteractionRefresh();

	return Failures;
}
