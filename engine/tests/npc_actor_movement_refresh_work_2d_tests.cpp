#include <cstdlib>
#include <vector>

#include "scene/npc/NpcActorMovementFrameApply2D.hpp"
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
	iggy::Vec2 proposedPosition)
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
	filter.status = iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed;
	filter.requestsMovement = true;
	return filter;
}

iggy::NpcActorMovementFrameApply2DRequest Request(const iggy::NpcActorPathStepOccupancyFilter2D &filter)
{
	iggy::NpcActorMovementFrameApply2DRequest request;
	request.filter = filter;
	return request;
}

iggy::NpcActorMovementFrameReport2D MovedReport()
{
	const iggy::NpcActorState2DRegistry registry = Registry({
		Actor("npc:a", { 0.5F, 0.5F }),
		Actor("npc:b", { 1.5F, 0.5F }),
	});
	const iggy::NpcActorMovementFrameApply2DResult apply =
		iggy::NpcActorMovementFrameApplier2D {}.apply(registry, {
			Request(Filter("npc:a", { 0.5F, 0.5F }, { 1.5F, 0.5F })),
			Request(Filter("npc:b", { 1.5F, 0.5F }, { 2.5F, 0.5F })),
		});
	return iggy::NpcActorMovementFrameReporter2D {}.report(apply);
}

bool SameTiles(
	const std::vector<iggy::TileCoord> &actual,
	const std::vector<iggy::TileCoord> &expected)
{
	if (actual.size() != expected.size()) {
		return false;
	}
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (!(actual[index] == expected[index])) {
			return false;
		}
	}
	return true;
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

bool SameReportSummary(
	const iggy::NpcActorMovementFrameReport2D &actual,
	const iggy::NpcActorMovementFrameReport2D &expected)
{
	return actual.requestCount == expected.requestCount
		&& actual.entryCount == expected.entryCount
		&& actual.movedCount == expected.movedCount
		&& actual.blockedCount == expected.blockedCount
		&& actual.rejectedCount == expected.rejectedCount
		&& actual.noMovementCount == expected.noMovementCount
		&& actual.missingActorCount == expected.missingActorCount
		&& actual.changedCount == expected.changedCount
		&& SameTiles(actual.dirtyTiles, expected.dirtyTiles)
		&& actual.needsOccupancyRebuild == expected.needsOccupancyRebuild
		&& actual.needsAiMapQueryRefresh == expected.needsAiMapQueryRefresh
		&& actual.needsInteractionRefresh == expected.needsInteractionRefresh
		&& actual.needsRenderRefresh == expected.needsRenderRefresh
		&& actual.needsVisibilityRefresh == expected.needsVisibilityRefresh
		&& actual.events == expected.events
		&& SameRegistry(actual.registry, expected.registry);
}

void ExpectWorkTypes(
	const std::vector<iggy::NpcActorMovementRefreshWork2DItem> &items,
	const std::vector<iggy::NpcActorMovementRefreshWork2DType> &types,
	const char *message)
{
	Expect(items.size() == types.size(), message);
	if (items.size() != types.size()) {
		return;
	}
	for (std::size_t index = 0; index < items.size(); ++index) {
		Expect(items[index].type == types[index], message);
	}
}

void TestEmptyNoOpMovementFrameReportProducesNoWork()
{
	const iggy::NpcActorMovementFrameReport2D report;

	const iggy::NpcActorMovementRefreshWork2D work =
		iggy::NpcActorMovementRefreshWorkProjector2D {}.project(report);

	Expect(!work.hasWork(), "empty report should not produce work");
	Expect(work.items.empty(), "empty report should have no work items");
	Expect(work.dirtyTiles.empty(), "empty report should have no dirty tiles");
	Expect(SameReportSummary(work.report, report), "empty report should be copied by value");
}

void TestMovedFrameWithAllFlagsProducesStableWorkItems()
{
	const iggy::NpcActorMovementFrameReport2D report = MovedReport();

	const iggy::NpcActorMovementRefreshWork2D work =
		iggy::NpcActorMovementRefreshWorkProjector2D {}.project(report);

	Expect(work.hasWork(), "moved report should produce refresh work");
	ExpectWorkTypes(
		work.items,
		{
			iggy::NpcActorMovementRefreshWork2DType::OccupancyRebuild,
			iggy::NpcActorMovementRefreshWork2DType::AiMapQueryRefresh,
			iggy::NpcActorMovementRefreshWork2DType::InteractionRefresh,
			iggy::NpcActorMovementRefreshWork2DType::RenderRefresh,
			iggy::NpcActorMovementRefreshWork2DType::VisibilityRefresh,
		},
		"all refresh work items should use stable order");
	Expect(SameTiles(work.dirtyTiles, report.dirtyTiles), "work should preserve report dirty tile order");
	for (const iggy::NpcActorMovementRefreshWork2DItem &item : work.items) {
		Expect(SameTiles(item.dirtyTiles, report.dirtyTiles), "each item should carry report dirty tiles");
	}
	Expect(work.needsOccupancyRebuild && work.needsAiMapQueryRefresh, "work should mirror first refresh flags");
	Expect(work.needsInteractionRefresh && work.needsRenderRefresh && work.needsVisibilityRefresh, "work should mirror remaining refresh flags");
}

void TestDirtyTilesArePreservedExactlyFromReport()
{
	iggy::NpcActorMovementFrameReport2D report;
	report.dirtyTiles = {
		iggy::TileCoord { 2, 0 },
		iggy::TileCoord { 1, 0 },
		iggy::TileCoord { 2, 0 },
	};
	report.needsRenderRefresh = true;

	const iggy::NpcActorMovementRefreshWork2D work =
		iggy::NpcActorMovementRefreshWorkProjector2D {}.project(report);

	Expect(SameTiles(work.dirtyTiles, report.dirtyTiles), "projector should not recompute or dedupe dirty tiles");
	Expect(work.items.size() == 1, "single refresh flag should produce one item");
	Expect(SameTiles(work.items[0].dirtyTiles, report.dirtyTiles), "item should preserve exact dirty tile list");
}

void TestIndividualRefreshFlagsProduceMatchingItems()
{
	iggy::NpcActorMovementFrameReport2D report;
	report.dirtyTiles = { iggy::TileCoord { 4, 3 } };
	report.needsAiMapQueryRefresh = true;
	report.needsVisibilityRefresh = true;

	const iggy::NpcActorMovementRefreshWork2D work =
		iggy::NpcActorMovementRefreshWorkProjector2D {}.project(report);

	ExpectWorkTypes(
		work.items,
		{
			iggy::NpcActorMovementRefreshWork2DType::AiMapQueryRefresh,
			iggy::NpcActorMovementRefreshWork2DType::VisibilityRefresh,
		},
		"individual refresh flags should produce only matching items in global order");
	Expect(!work.needsOccupancyRebuild && !work.needsInteractionRefresh && !work.needsRenderRefresh, "unset flags should stay unset");
}

void TestRefreshFlagWithoutDirtyTilesStillProducesWork()
{
	iggy::NpcActorMovementFrameReport2D report;
	report.needsOccupancyRebuild = true;

	const iggy::NpcActorMovementRefreshWork2D work =
		iggy::NpcActorMovementRefreshWorkProjector2D {}.project(report);

	Expect(work.hasWork(), "true refresh flag should produce work even without dirty tiles");
	Expect(work.items.size() == 1, "one true refresh flag should produce one work item");
	Expect(work.items[0].type == iggy::NpcActorMovementRefreshWork2DType::OccupancyRebuild, "work item should preserve refresh type");
	Expect(work.items[0].dirtyTiles.empty(), "work item should carry empty dirty tiles when report has none");
}

void TestSourceReportIsCopiedAndInputIsNotMutated()
{
	iggy::NpcActorMovementFrameReport2D report = MovedReport();
	const iggy::NpcActorMovementFrameReport2D before = report;

	const iggy::NpcActorMovementRefreshWork2D work =
		iggy::NpcActorMovementRefreshWorkProjector2D {}.project(report);

	Expect(SameReportSummary(work.report, before), "work should preserve copied source report");
	Expect(SameReportSummary(report, before), "projector should not mutate input report");

	report.dirtyTiles.push_back(iggy::TileCoord { 9, 9 });
	report.registry.actors[0].position = { 9.5F, 9.5F };

	Expect(SameTiles(work.dirtyTiles, before.dirtyTiles), "work dirty tiles should not alias input report");
	Expect(SameReportSummary(work.report, before), "copied report should not alias input report");
}

} // namespace

int main()
{
	TestEmptyNoOpMovementFrameReportProducesNoWork();
	TestMovedFrameWithAllFlagsProducesStableWorkItems();
	TestDirtyTilesArePreservedExactlyFromReport();
	TestIndividualRefreshFlagsProduceMatchingItems();
	TestRefreshFlagWithoutDirtyTilesStillProducesWork();
	TestSourceReportIsCopiedAndInputIsNotMutated();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
