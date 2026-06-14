#include <cstdlib>
#include <vector>

#include "scene/npc/NpcActorMovementFrameReport2D.hpp"
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

iggy::NpcActorMovementFrameApply2DResult Apply(
	const iggy::NpcActorState2DRegistry &registry,
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> &requests)
{
	return iggy::NpcActorMovementFrameApplier2D {}.apply(registry, requests);
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

bool SameApplySummary(
	const iggy::NpcActorMovementFrameApply2DResult &actual,
	const iggy::NpcActorMovementFrameApply2DResult &expected)
{
	return actual.status == expected.status
		&& SameRegistry(actual.inputRegistry, expected.inputRegistry)
		&& SameRegistry(actual.registry, expected.registry)
		&& actual.entries.size() == expected.entries.size()
		&& actual.requestCount == expected.requestCount
		&& actual.appliedCount == expected.appliedCount
		&& actual.movedCount == expected.movedCount
		&& actual.blockedCount == expected.blockedCount
		&& actual.noMovementCount == expected.noMovementCount
		&& actual.rejectedCount == expected.rejectedCount
		&& actual.missingActorCount == expected.missingActorCount
		&& actual.changedCount == expected.changedCount
		&& actual.changed == expected.changed;
}

bool AllRefreshFlags(const iggy::NpcActorMovementFrameReport2D &report)
{
	return report.needsOccupancyRebuild
		&& report.needsAiMapQueryRefresh
		&& report.needsInteractionRefresh
		&& report.needsRenderRefresh
		&& report.needsVisibilityRefresh;
}

bool NoRefreshFlags(const iggy::NpcActorMovementFrameReport2D &report)
{
	return !report.needsOccupancyRebuild
		&& !report.needsAiMapQueryRefresh
		&& !report.needsInteractionRefresh
		&& !report.needsRenderRefresh
		&& !report.needsVisibilityRefresh;
}

void ExpectEvents(
	const std::vector<iggy::NpcActorMovementFrameEvent2D> &actual,
	const std::vector<iggy::NpcActorMovementFrameEvent2D> &expected,
	const char *message)
{
	Expect(actual.size() == expected.size(), message);
	if (actual.size() != expected.size()) {
		return;
	}
	for (std::size_t index = 0; index < actual.size(); ++index) {
		Expect(actual[index] == expected[index], message);
	}
}

void TestEmptyApplyReportsUnchangedNoFacts()
{
	const iggy::NpcActorState2DRegistry registry = Registry({
		Actor("npc:guard", { 0.5F, 0.5F }),
	});
	const iggy::NpcActorMovementFrameApply2DResult apply = Apply(registry, {});

	const iggy::NpcActorMovementFrameReport2D report =
		iggy::NpcActorMovementFrameReporter2D {}.report(apply);

	Expect(report.requestCount == 0 && report.entryCount == 0, "empty report should have zero request and entry counts");
	Expect(report.movedCount == 0 && report.blockedCount == 0 && report.rejectedCount == 0, "empty report should have zero movement counts");
	Expect(report.dirtyTiles.empty(), "empty report should have no dirty tiles");
	Expect(!report.hasDirtyTiles(), "empty report dirty helper should be false");
	Expect(NoRefreshFlags(report), "empty report should not request refreshes");
	Expect(!report.changed(), "empty report changed helper should be false");
	ExpectEvents(
		report.events,
		{ iggy::NpcActorMovementFrameEvent2D::MovementFrameUnchanged },
		"empty report should only emit final unchanged event");
	Expect(SameApplySummary(report.apply, apply), "empty report should preserve copied apply result");
	Expect(SameRegistry(report.registry, apply.registry), "empty report should preserve returned registry");
}

void TestMovedEntriesAggregateCountsDirtyTilesRefreshesAndChangedEvent()
{
	const iggy::NpcActorState2DRegistry registry = Registry({
		Actor("npc:a", { 0.5F, 0.5F }),
		Actor("npc:b", { 1.5F, 0.5F }),
	});
	const iggy::NpcActorMovementFrameApply2DResult apply = Apply(registry, {
		Request(Filter("npc:a", { 0.5F, 0.5F }, { 1.5F, 0.5F })),
		Request(Filter("npc:b", { 1.5F, 0.5F }, { 2.5F, 0.5F })),
	});

	const iggy::NpcActorMovementFrameReport2D report =
		iggy::NpcActorMovementFrameReporter2D {}.report(apply);

	Expect(report.requestCount == 2 && report.entryCount == 2, "moved report should preserve request and entry counts");
	Expect(report.movedCount == 2 && report.changedCount == 2, "moved report should preserve moved and changed counts");
	Expect(report.blockedCount == 0 && report.rejectedCount == 0 && report.missingActorCount == 0, "moved report should not count failures");
	Expect(report.changed(), "moved report should report changed");
	Expect(report.dirtyTiles.size() == 3, "moved report should dedupe overlapping dirty tiles");
	Expect(report.dirtyTiles[0] == iggy::TileCoord { 0, 0 }, "dirty tiles should preserve first old tile");
	Expect(report.dirtyTiles[1] == iggy::TileCoord { 1, 0 }, "dirty tiles should preserve first-seen shared tile");
	Expect(report.dirtyTiles[2] == iggy::TileCoord { 2, 0 }, "dirty tiles should preserve second new tile");
	Expect(AllRefreshFlags(report), "moved report should OR all refresh flags");
	ExpectEvents(
		report.events,
		{
			iggy::NpcActorMovementFrameEvent2D::MovementApplied,
			iggy::NpcActorMovementFrameEvent2D::MovementApplied,
			iggy::NpcActorMovementFrameEvent2D::DirtyTileObserved,
			iggy::NpcActorMovementFrameEvent2D::DirtyTileObserved,
			iggy::NpcActorMovementFrameEvent2D::DirtyTileObserved,
			iggy::NpcActorMovementFrameEvent2D::RefreshNeeded,
			iggy::NpcActorMovementFrameEvent2D::RefreshNeeded,
			iggy::NpcActorMovementFrameEvent2D::RefreshNeeded,
			iggy::NpcActorMovementFrameEvent2D::RefreshNeeded,
			iggy::NpcActorMovementFrameEvent2D::RefreshNeeded,
			iggy::NpcActorMovementFrameEvent2D::MovementFrameChanged,
		},
		"moved report should preserve deterministic event order");
}

void TestBlockedRejectedNoStepAndMissingEventsAreDeterministic()
{
	const iggy::NpcActorState2DRegistry registry = Registry({
		Actor("npc:blocked", { 0.5F, 0.5F }),
		Actor("npc:idle", { 2.5F, 0.5F }),
		Actor("npc:absent", { 4.5F, 0.5F }, false),
	});
	const iggy::NpcActorMovementFrameApply2DResult apply = Apply(registry, {
		Request(Filter(
			"npc:blocked",
			{ 0.5F, 0.5F },
			{ 1.5F, 0.5F },
			iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc,
			false,
			"npc:blocker")),
		Request(Filter(
			"npc:idle",
			{ 2.5F, 0.5F },
			{ 2.5F, 0.5F },
			iggy::NpcActorPathStepOccupancyFilter2DStatus::NoStepProposal,
			false)),
		Request(Filter("npc:absent", { 4.5F, 0.5F }, { 5.5F, 0.5F })),
		Request(Filter("npc:missing", { 8.5F, 0.5F }, { 9.5F, 0.5F })),
	});

	const iggy::NpcActorMovementFrameReport2D report =
		iggy::NpcActorMovementFrameReporter2D {}.report(apply);

	Expect(report.movedCount == 0, "mixed non-moving report should not count moves");
	Expect(report.blockedCount == 1, "mixed non-moving report should count blocked entry");
	Expect(report.noMovementCount == 1, "mixed non-moving report should count no-op entry");
	Expect(report.rejectedCount == 1, "mixed non-moving report should count rejected entry");
	Expect(report.missingActorCount == 1, "mixed non-moving report should count missing actor");
	Expect(!report.changed(), "mixed non-moving report should stay unchanged");
	Expect(report.dirtyTiles.empty(), "mixed non-moving report should not dirty tiles");
	Expect(NoRefreshFlags(report), "mixed non-moving report should not request refreshes");
	ExpectEvents(
		report.events,
		{
			iggy::NpcActorMovementFrameEvent2D::MovementBlocked,
			iggy::NpcActorMovementFrameEvent2D::MovementNoOp,
			iggy::NpcActorMovementFrameEvent2D::MovementRejected,
			iggy::NpcActorMovementFrameEvent2D::ActorNotFound,
			iggy::NpcActorMovementFrameEvent2D::MovementFrameUnchanged,
		},
		"mixed non-moving report should preserve deterministic event order");
}

void TestMixedFrameKeepsChangedFinalEventAfterFailures()
{
	const iggy::NpcActorState2DRegistry registry = Registry({
		Actor("npc:mover", { 0.5F, 0.5F }),
		Actor("npc:blocked", { 3.5F, 0.5F }),
	});
	const iggy::NpcActorMovementFrameApply2DResult apply = Apply(registry, {
		Request(Filter("npc:mover", { 0.5F, 0.5F }, { 1.5F, 0.5F })),
		Request(Filter(
			"npc:blocked",
			{ 3.5F, 0.5F },
			{ 4.5F, 0.5F },
			iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc,
			false,
			"npc:blocker")),
	});

	const iggy::NpcActorMovementFrameReport2D report =
		iggy::NpcActorMovementFrameReporter2D {}.report(apply);

	Expect(report.movedCount == 1 && report.blockedCount == 1, "mixed changed report should preserve moved and blocked counts");
	Expect(report.changed(), "mixed changed report should report changed");
	Expect(report.dirtyTiles.size() == 2, "mixed changed report should aggregate moved dirty tiles only");
	ExpectEvents(
		report.events,
		{
			iggy::NpcActorMovementFrameEvent2D::MovementApplied,
			iggy::NpcActorMovementFrameEvent2D::MovementBlocked,
			iggy::NpcActorMovementFrameEvent2D::DirtyTileObserved,
			iggy::NpcActorMovementFrameEvent2D::DirtyTileObserved,
			iggy::NpcActorMovementFrameEvent2D::RefreshNeeded,
			iggy::NpcActorMovementFrameEvent2D::RefreshNeeded,
			iggy::NpcActorMovementFrameEvent2D::RefreshNeeded,
			iggy::NpcActorMovementFrameEvent2D::RefreshNeeded,
			iggy::NpcActorMovementFrameEvent2D::RefreshNeeded,
			iggy::NpcActorMovementFrameEvent2D::MovementFrameChanged,
		},
		"mixed changed report should keep phase-ordered events");
}

void TestReportCopiesApplyAndDoesNotMutateInput()
{
	iggy::NpcActorMovementFrameApply2DResult apply = Apply(
		Registry({ Actor("npc:mover", { 0.5F, 0.5F }) }),
		{ Request(Filter("npc:mover", { 0.5F, 0.5F }, { 1.5F, 0.5F })) });
	const iggy::NpcActorMovementFrameApply2DResult before = apply;

	const iggy::NpcActorMovementFrameReport2D report =
		iggy::NpcActorMovementFrameReporter2D {}.report(apply);

	Expect(SameApplySummary(report.apply, before), "report should copy full apply summary by value");
	Expect(SameApplySummary(apply, before), "reporter should not mutate input apply result");
	Expect(SameRegistry(report.registry, before.registry), "report should preserve copied returned registry");

	apply.registry.actors[0].position = { 9.5F, 9.5F };
	Expect(NearVec(report.registry.actors[0].position, { 1.5F, 0.5F }), "report registry copy should not alias input apply result");
	Expect(NearVec(report.apply.registry.actors[0].position, { 1.5F, 0.5F }), "report apply copy should not alias input apply result");
}

} // namespace

int main()
{
	TestEmptyApplyReportsUnchangedNoFacts();
	TestMovedEntriesAggregateCountsDirtyTilesRefreshesAndChangedEvent();
	TestBlockedRejectedNoStepAndMissingEventsAreDeterministic();
	TestMixedFrameKeepsChangedFinalEventAfterFailures();
	TestReportCopiesApplyAndDoesNotMutateInput();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
