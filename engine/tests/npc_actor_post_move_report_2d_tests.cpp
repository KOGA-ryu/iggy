#include <cstdlib>

#include "scene/npc/NpcActorPostMoveReport2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

bool AllRefreshFlags(const iggy::NpcActorPostMoveReport2D &report)
{
	return report.needsOccupancyRebuild
		&& report.needsAiMapQueryRefresh
		&& report.needsInteractionRefresh
		&& report.needsRenderRefresh
		&& report.needsVisibilityRefresh;
}

bool NoRefreshFlags(const iggy::NpcActorPostMoveReport2D &report)
{
	return !report.needsOccupancyRebuild
		&& !report.needsAiMapQueryRefresh
		&& !report.needsInteractionRefresh
		&& !report.needsRenderRefresh
		&& !report.needsVisibilityRefresh;
}

void TestMovedAcrossTilesReportsDirtyTilesAndRefreshes()
{
	const iggy::NpcActorPostMoveReport2D report = iggy::NpcActorPostMoveReporter2D {}.report(
		Id("npc:runner"),
		{ 1.25F, 2.75F },
		{ 3.25F, 4.25F },
		iggy::NpcActorPostMoveReport2DStatus::Moved);

	Expect(report.status == iggy::NpcActorPostMoveReport2DStatus::Moved, "moved report should preserve Moved status");
	Expect(report.moved(), "moved helper should be true for Moved");
	Expect(!report.blocked(), "blocked helper should be false for Moved");
	Expect(report.blockingKind == iggy::NpcActorPostMoveBlockingKind2D::None, "moved report should have no blocking kind");
	Expect(report.oldTile == iggy::TileCoord { 1, 2 }, "moved report should derive old tile");
	Expect(report.newTile == iggy::TileCoord { 3, 4 }, "moved report should derive new tile");
	Expect(report.dirtyTiles.size() == 2, "moved across tiles should report two dirty tiles");
	Expect(report.dirtyTiles[0] == iggy::TileCoord { 1, 2 }, "dirty tiles should preserve old tile first");
	Expect(report.dirtyTiles[1] == iggy::TileCoord { 3, 4 }, "dirty tiles should preserve new tile second");
	Expect(report.hasDirtyTiles(), "moved report should have dirty tiles");
	Expect(AllRefreshFlags(report), "moved report should mark all refresh flags");
}

void TestMovedWithinSameTileReportsOneDirtyTileAndRefreshes()
{
	const iggy::NpcActorPostMoveReport2D report = iggy::NpcActorPostMoveReporter2D {}.report(
		Id("npc:runner"),
		{ 2.10F, 2.20F },
		{ 2.80F, 2.90F },
		iggy::NpcActorPostMoveReport2DStatus::Moved);

	Expect(report.oldTile == iggy::TileCoord { 2, 2 }, "same-tile move should derive old tile");
	Expect(report.newTile == iggy::TileCoord { 2, 2 }, "same-tile move should derive new tile");
	Expect(report.dirtyTiles.size() == 1, "same-tile move should dedupe dirty tiles");
	Expect(report.dirtyTiles[0] == iggy::TileCoord { 2, 2 }, "same-tile move should keep one dirty tile");
	Expect(AllRefreshFlags(report), "same-tile moved report should mark all refresh flags");
}

void TestNotMovedReportsNoDirtyTilesOrRefreshes()
{
	const iggy::NpcActorPostMoveReport2D report = iggy::NpcActorPostMoveReporter2D {}.report({
		Id("npc:guard"),
		{ 1.0F, 1.0F },
		{ 1.0F, 1.0F },
		iggy::NpcActorPostMoveReport2DStatus::NotMoved,
	});

	Expect(report.status == iggy::NpcActorPostMoveReport2DStatus::NotMoved, "not-moved report should preserve status");
	Expect(!report.moved(), "not-moved report should not be moved");
	Expect(!report.blocked(), "not-moved report should not be blocked");
	Expect(report.blockingKind == iggy::NpcActorPostMoveBlockingKind2D::None, "not-moved report should default to no blocker");
	Expect(report.dirtyTiles.empty(), "not-moved report should have no dirty tiles");
	Expect(!report.hasDirtyTiles(), "not-moved report should report no dirty tiles");
	Expect(NoRefreshFlags(report), "not-moved report should not mark refresh flags");
}

void TestBlockedByMapPreservesBlockingKindWithoutRefreshes()
{
	const iggy::NpcActorPostMoveReport2D report = iggy::NpcActorPostMoveReporter2D {}.report(
		Id("npc:guard"),
		{ 4.25F, 5.75F },
		{ 4.25F, 5.75F },
		iggy::NpcActorPostMoveReport2DStatus::Blocked,
		iggy::NpcActorPostMoveBlockingKind2D::Map);

	Expect(report.status == iggy::NpcActorPostMoveReport2DStatus::Blocked, "blocked map report should preserve Blocked status");
	Expect(report.blocked(), "blocked helper should be true for Blocked");
	Expect(report.blockingKind == iggy::NpcActorPostMoveBlockingKind2D::Map, "blocked map report should preserve Map kind");
	Expect(report.blockingNpcId.empty(), "blocked map report should not invent blocking npc id");
	Expect(report.dirtyTiles.empty(), "blocked map report should have no dirty tiles");
	Expect(NoRefreshFlags(report), "blocked map report should not mark refresh flags");
}

void TestBlockedByNpcPreservesBlockingNpcIdWithoutRefreshes()
{
	const iggy::NpcActorPostMoveReport2D report = iggy::NpcActorPostMoveReporter2D {}.report(
		Id("npc:guard"),
		{ 4.25F, 5.75F },
		{ 4.25F, 5.75F },
		iggy::NpcActorPostMoveReport2DStatus::Blocked,
		iggy::NpcActorPostMoveBlockingKind2D::Npc,
		Id("npc:blocker"));

	Expect(report.status == iggy::NpcActorPostMoveReport2DStatus::Blocked, "blocked npc report should preserve Blocked status");
	Expect(report.blockingKind == iggy::NpcActorPostMoveBlockingKind2D::Npc, "blocked npc report should preserve Npc kind");
	Expect(report.blockingNpcId == Id("npc:blocker"), "blocked npc report should preserve blocking npc id");
	Expect(report.dirtyTiles.empty(), "blocked npc report should have no dirty tiles");
	Expect(NoRefreshFlags(report), "blocked npc report should not mark refresh flags");
}

void TestRejectedInvalidStepPreservesKindWithoutRefreshes()
{
	const iggy::NpcActorPostMoveReport2D report = iggy::NpcActorPostMoveReporter2D {}.report(
		Id("npc:guard"),
		{ 0.25F, 0.25F },
		{ 10.25F, 10.25F },
		iggy::NpcActorPostMoveReport2DStatus::Rejected,
		iggy::NpcActorPostMoveBlockingKind2D::InvalidStep);

	Expect(report.status == iggy::NpcActorPostMoveReport2DStatus::Rejected, "rejected report should preserve status");
	Expect(report.blockingKind == iggy::NpcActorPostMoveBlockingKind2D::InvalidStep, "rejected report should preserve invalid step kind");
	Expect(report.oldTile == iggy::TileCoord { 0, 0 }, "rejected report should still derive old tile");
	Expect(report.newTile == iggy::TileCoord { 10, 10 }, "rejected report should still derive supplied new tile");
	Expect(report.dirtyTiles.empty(), "rejected report should have no dirty tiles");
	Expect(NoRefreshFlags(report), "rejected report should not mark refresh flags");
}

void TestMovedNormalizesInconsistentBlockingFacts()
{
	const iggy::NpcActorPostMoveReport2D report = iggy::NpcActorPostMoveReporter2D {}.report(
		Id("npc:runner"),
		{ 1.25F, 1.25F },
		{ 2.25F, 2.25F },
		iggy::NpcActorPostMoveReport2DStatus::Moved,
		iggy::NpcActorPostMoveBlockingKind2D::Npc,
		Id("npc:blocker"));

	Expect(report.status == iggy::NpcActorPostMoveReport2DStatus::Moved, "inconsistent moved report should stay Moved");
	Expect(report.blockingKind == iggy::NpcActorPostMoveBlockingKind2D::None, "moved report should normalize blocking kind to None");
	Expect(report.blockingNpcId.empty(), "moved report should clear blocking npc id");
	Expect(AllRefreshFlags(report), "normalized moved report should still mark refresh flags");
}

void TestNamespacedAndUnqualifiedIdsArePreservedExactly()
{
	const iggy::NpcActorPostMoveReport2D namespaced = iggy::NpcActorPostMoveReporter2D {}.report(
		Id("npc:guard"),
		{ -0.25F, -1.25F },
		{ 0.25F, 0.25F },
		iggy::NpcActorPostMoveReport2DStatus::Blocked,
		iggy::NpcActorPostMoveBlockingKind2D::Npc,
		Id("npc:blocker"));
	const iggy::NpcActorPostMoveReport2D unqualified = iggy::NpcActorPostMoveReporter2D {}.report(
		Id("guard"),
		{ -0.25F, -1.25F },
		{ 0.25F, 0.25F },
		iggy::NpcActorPostMoveReport2DStatus::Blocked,
		iggy::NpcActorPostMoveBlockingKind2D::Npc,
		Id("blocker"));

	Expect(namespaced.npcId == Id("npc:guard"), "report should preserve namespaced npc id");
	Expect(unqualified.npcId == Id("guard"), "report should preserve unqualified npc id");
	Expect(namespaced.npcId != unqualified.npcId, "namespaced and unqualified npc ids should remain distinct");
	Expect(namespaced.blockingNpcId == Id("npc:blocker"), "report should preserve namespaced blocking npc id");
	Expect(unqualified.blockingNpcId == Id("blocker"), "report should preserve unqualified blocking npc id");
	Expect(namespaced.blockingNpcId != unqualified.blockingNpcId, "namespaced and unqualified blocking ids should remain distinct");
	Expect(namespaced.oldTile == iggy::TileCoord { -1, -2 }, "negative fractional old position should use tileForPoint convention");
}

void TestReportDoesNotMutateInputValues()
{
	iggy::NpcActorPostMoveReport2DInput input;
	input.npcId = Id("npc:runner");
	input.oldPosition = { 1.25F, 1.25F };
	input.newPosition = { 2.25F, 2.25F };
	input.status = iggy::NpcActorPostMoveReport2DStatus::Moved;
	const iggy::NpcActorPostMoveReport2DInput before = input;

	const iggy::NpcActorPostMoveReport2D report = iggy::NpcActorPostMoveReporter2D {}.report(input);

	Expect(report.moved(), "immutability setup should produce moved report");
	Expect(input.npcId == before.npcId, "report should not mutate input npc id");
	Expect(NearVec(input.oldPosition, before.oldPosition), "report should not mutate input old position");
	Expect(NearVec(input.newPosition, before.newPosition), "report should not mutate input new position");
	Expect(input.status == before.status, "report should not mutate input status");
	Expect(input.blockingKind == before.blockingKind, "report should not mutate input blocking kind");
	Expect(input.blockingNpcId == before.blockingNpcId, "report should not mutate input blocking npc id");
}

} // namespace

int main()
{
	TestMovedAcrossTilesReportsDirtyTilesAndRefreshes();
	TestMovedWithinSameTileReportsOneDirtyTileAndRefreshes();
	TestNotMovedReportsNoDirtyTilesOrRefreshes();
	TestBlockedByMapPreservesBlockingKindWithoutRefreshes();
	TestBlockedByNpcPreservesBlockingNpcIdWithoutRefreshes();
	TestRejectedInvalidStepPreservesKindWithoutRefreshes();
	TestMovedNormalizesInconsistentBlockingFacts();
	TestNamespacedAndUnqualifiedIdsArePreservedExactly();
	TestReportDoesNotMutateInputValues();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
