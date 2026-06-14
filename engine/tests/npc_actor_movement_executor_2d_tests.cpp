#include <cstdlib>

#include "scene/npc/NpcActorMovementExecutor2D.hpp"
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
	iggy::Vec2 position = { 0.5F, 0.5F },
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

iggy::NpcActorPathStep2D Step(
	const char *npcId,
	iggy::Vec2 oldPosition,
	iggy::Vec2 proposedPosition,
	iggy::NpcActorPathStep2DStatus status = iggy::NpcActorPathStep2DStatus::Proposed,
	bool requestsMovement = true)
{
	iggy::NpcActorPathStep2D step;
	step.status = status;
	step.npcId = Id(npcId);
	step.oldPosition = oldPosition;
	step.proposedPosition = proposedPosition;
	step.oldTile = iggy::tileForPoint(oldPosition);
	step.proposedTile = iggy::tileForPoint(proposedPosition);
	step.moveMode = iggy::NpcMoveMode::Walk;
	step.requestsMovement = requestsMovement;
	return step;
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
	filter.step = Step(
		npcId,
		oldPosition,
		proposedPosition,
		status == iggy::NpcActorPathStepOccupancyFilter2DStatus::NoStepProposal
			? iggy::NpcActorPathStep2DStatus::NoPath
			: iggy::NpcActorPathStep2DStatus::Proposed,
		status != iggy::NpcActorPathStepOccupancyFilter2DStatus::NoStepProposal);
	filter.status = status;
	filter.requestsMovement = requestsMovement;
	if (blockingNpcId[0] != '\0') {
		filter.blockingNpcId = Id(blockingNpcId);
	}
	return filter;
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

bool SameStep(const iggy::NpcActorPathStep2D &actual, const iggy::NpcActorPathStep2D &expected)
{
	return actual.status == expected.status
		&& actual.npcId == expected.npcId
		&& NearVec(actual.oldPosition, expected.oldPosition)
		&& NearVec(actual.proposedPosition, expected.proposedPosition)
		&& actual.oldTile == expected.oldTile
		&& actual.proposedTile == expected.proposedTile
		&& actual.moveMode == expected.moveMode
		&& actual.requestsMovement == expected.requestsMovement
		&& actual.completedPath == expected.completedPath;
}

bool SameFilter(
	const iggy::NpcActorPathStepOccupancyFilter2D &actual,
	const iggy::NpcActorPathStepOccupancyFilter2D &expected)
{
	return SameStep(actual.step, expected.step)
		&& actual.status == expected.status
		&& actual.requestsMovement == expected.requestsMovement
		&& actual.blockingNpcId == expected.blockingNpcId;
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

void TestAllowedFilterMovesActorAndReportsPostMove()
{
	const iggy::NpcActorState2D actor = Actor("npc:mover", { 0.5F, 0.5F });
	const iggy::NpcActorPathStepOccupancyFilter2D filter = Filter(
		"npc:mover",
		actor.position,
		{ 2.5F, 0.5F },
		iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed,
		true);

	const iggy::NpcActorMovementExecutor2DResult result =
		iggy::NpcActorMovementExecutor2D {}.execute(actor, filter);

	Expect(result.status == iggy::NpcActorMovementExecutor2DStatus::Moved, "allowed movement should move actor");
	Expect(result.moved(), "moved helper should be true for moved result");
	Expect(result.changed, "moved result should be changed");
	Expect(SameActor(result.actor, actor), "result should preserve copied input actor");
	Expect(SameFilter(result.filter, filter), "result should preserve copied filter");
	Expect(result.resultActor.npcId == actor.npcId, "moved actor should preserve npc id");
	Expect(NearVec(result.resultActor.position, filter.step.proposedPosition), "moved actor should use proposed position");
	Expect(result.postMove.status == iggy::NpcActorPostMoveReport2DStatus::Moved, "moved result should preserve Moved post report");
	Expect(result.postMove.dirtyTiles.size() == 2, "cross-tile move should dirty old and new tiles");
	Expect(result.postMove.dirtyTiles[0] == iggy::TileCoord { 0, 0 }, "post report should dirty old tile first");
	Expect(result.postMove.dirtyTiles[1] == iggy::TileCoord { 2, 0 }, "post report should dirty new tile second");
	Expect(AllRefreshFlags(result.postMove), "moved result should mark refresh flags");
}

void TestAllowedSameTileMoveUpdatesPositionAndOneDirtyTile()
{
	const iggy::NpcActorState2D actor = Actor("npc:mover", { 1.10F, 1.10F });
	const iggy::NpcActorPathStepOccupancyFilter2D filter = Filter(
		"npc:mover",
		actor.position,
		{ 1.80F, 1.80F },
		iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed,
		true);

	const iggy::NpcActorMovementExecutor2DResult result =
		iggy::NpcActorMovementExecutor2D {}.execute(actor, filter);

	Expect(result.status == iggy::NpcActorMovementExecutor2DStatus::Moved, "same-tile allowed movement should move actor");
	Expect(NearVec(result.resultActor.position, { 1.80F, 1.80F }), "same-tile move should update position");
	Expect(result.postMove.dirtyTiles.size() == 1, "same-tile move should produce one dirty tile");
	Expect(result.postMove.dirtyTiles[0] == iggy::TileCoord { 1, 1 }, "same-tile move should dirty the shared tile");
	Expect(AllRefreshFlags(result.postMove), "same-tile move should still mark refresh flags");
}

void TestBlockedByNpcKeepsActorAndReportsBlocker()
{
	const iggy::NpcActorState2D actor = Actor("npc:mover", { 0.5F, 0.5F });
	const iggy::NpcActorPathStepOccupancyFilter2D filter = Filter(
		"npc:mover",
		actor.position,
		{ 1.5F, 0.5F },
		iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc,
		false,
		"npc:blocker");

	const iggy::NpcActorMovementExecutor2DResult result =
		iggy::NpcActorMovementExecutor2D {}.execute(actor, filter);

	Expect(result.status == iggy::NpcActorMovementExecutor2DStatus::BlockedByNpc, "blocked filter should map to BlockedByNpc");
	Expect(!result.changed, "blocked result should not change actor");
	Expect(!result.moved(), "blocked result should not report moved");
	Expect(SameActor(result.resultActor, actor), "blocked result should preserve actor state");
	Expect(result.postMove.status == iggy::NpcActorPostMoveReport2DStatus::Blocked, "blocked result should produce blocked post report");
	Expect(result.postMove.blockingKind == iggy::NpcActorPostMoveBlockingKind2D::Npc, "blocked result should report NPC blocker kind");
	Expect(result.postMove.blockingNpcId == Id("npc:blocker"), "blocked result should preserve blocking NPC id");
	Expect(result.postMove.dirtyTiles.empty(), "blocked result should not dirty tiles");
	Expect(NoRefreshFlags(result.postMove), "blocked result should not mark refresh flags");
}

void TestNoStepProposalKeepsActorAndReportsNoMovement()
{
	const iggy::NpcActorState2D actor = Actor("npc:mover", { 0.5F, 0.5F });
	const iggy::NpcActorPathStepOccupancyFilter2D filter = Filter(
		"npc:mover",
		actor.position,
		actor.position,
		iggy::NpcActorPathStepOccupancyFilter2DStatus::NoStepProposal,
		false);

	const iggy::NpcActorMovementExecutor2DResult result =
		iggy::NpcActorMovementExecutor2D {}.execute(actor, filter);

	Expect(result.status == iggy::NpcActorMovementExecutor2DStatus::NoMovement, "no-step filter should map to NoMovement");
	Expect(!result.changed, "no-step result should not change actor");
	Expect(SameActor(result.resultActor, actor), "no-step result should preserve actor state");
	Expect(result.postMove.status == iggy::NpcActorPostMoveReport2DStatus::NotMoved, "no-step result should produce NotMoved report");
	Expect(result.postMove.dirtyTiles.empty(), "no-step result should not dirty tiles");
	Expect(NoRefreshFlags(result.postMove), "no-step result should not mark refresh flags");
}

void TestActorIdMismatchRejectsAndKeepsActor()
{
	const iggy::NpcActorState2D actor = Actor("npc:mover", { 0.5F, 0.5F });
	const iggy::NpcActorPathStepOccupancyFilter2D filter = Filter(
		"npc:other",
		actor.position,
		{ 1.5F, 0.5F },
		iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed,
		true);

	const iggy::NpcActorMovementExecutor2DResult result =
		iggy::NpcActorMovementExecutor2D {}.execute(actor, filter);

	Expect(result.status == iggy::NpcActorMovementExecutor2DStatus::ActorMismatch, "mismatched actor id should reject");
	Expect(!result.changed, "mismatched actor id should not change actor");
	Expect(SameActor(result.resultActor, actor), "mismatched actor id should preserve actor");
	Expect(result.postMove.status == iggy::NpcActorPostMoveReport2DStatus::Rejected, "mismatched actor id should produce rejected report");
	Expect(result.postMove.blockingKind == iggy::NpcActorPostMoveBlockingKind2D::InvalidStep, "mismatched actor id should use invalid-step blocker kind");
	Expect(NoRefreshFlags(result.postMove), "mismatched actor id should not mark refresh flags");
}

void TestActorNotPresentRejectsAndKeepsActor()
{
	const iggy::NpcActorState2D actor = Actor("npc:mover", { 0.5F, 0.5F }, false);
	const iggy::NpcActorPathStepOccupancyFilter2D filter = Filter(
		"npc:mover",
		actor.position,
		{ 1.5F, 0.5F },
		iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed,
		true);

	const iggy::NpcActorMovementExecutor2DResult result =
		iggy::NpcActorMovementExecutor2D {}.execute(actor, filter);

	Expect(result.status == iggy::NpcActorMovementExecutor2DStatus::ActorNotPresent, "absent actor should reject");
	Expect(!result.changed, "absent actor should not change");
	Expect(SameActor(result.resultActor, actor), "absent actor should be preserved");
	Expect(result.postMove.status == iggy::NpcActorPostMoveReport2DStatus::Rejected, "absent actor should produce rejected post report");
	Expect(result.postMove.blockingKind == iggy::NpcActorPostMoveBlockingKind2D::InvalidStep, "absent actor should use invalid-step blocker kind");
}

void TestNamespacedAndUnqualifiedActorIdsRemainExact()
{
	const iggy::NpcActorState2D unqualified = Actor("guard", { 0.5F, 0.5F });
	const iggy::NpcActorPathStepOccupancyFilter2D exactFilter = Filter(
		"guard",
		unqualified.position,
		{ 1.5F, 0.5F },
		iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed,
		true);
	const iggy::NpcActorPathStepOccupancyFilter2D namespacedFilter = Filter(
		"npc:guard",
		unqualified.position,
		{ 1.5F, 0.5F },
		iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed,
		true);

	const iggy::NpcActorMovementExecutor2DResult exact =
		iggy::NpcActorMovementExecutor2D {}.execute(unqualified, exactFilter);
	const iggy::NpcActorMovementExecutor2DResult mismatch =
		iggy::NpcActorMovementExecutor2D {}.execute(unqualified, namespacedFilter);

	Expect(exact.status == iggy::NpcActorMovementExecutor2DStatus::Moved, "exact unqualified id should move");
	Expect(exact.resultActor.npcId == Id("guard"), "executor should preserve unqualified actor id");
	Expect(mismatch.status == iggy::NpcActorMovementExecutor2DStatus::ActorMismatch, "namespaced id should mismatch unqualified actor id");
	Expect(mismatch.resultActor.npcId == Id("guard"), "mismatch should preserve original unqualified actor id");
	Expect(namespacedFilter.step.npcId == Id("npc:guard"), "mismatch filter should preserve namespaced id");
}

void TestInputsAreNotMutated()
{
	const iggy::NpcActorState2D actor = Actor("npc:mover", { 0.5F, 0.5F });
	const iggy::NpcActorPathStepOccupancyFilter2D filter = Filter(
		"npc:mover",
		actor.position,
		{ 1.5F, 0.5F },
		iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed,
		true);
	const iggy::NpcActorState2D actorBefore = actor;
	const iggy::NpcActorPathStepOccupancyFilter2D filterBefore = filter;

	const iggy::NpcActorMovementExecutor2DResult result =
		iggy::NpcActorMovementExecutor2D {}.execute(actor, filter);

	Expect(result.moved(), "immutability setup should move actor");
	Expect(SameActor(actor, actorBefore), "executor should not mutate input actor");
	Expect(SameFilter(filter, filterBefore), "executor should not mutate input filter");
}

} // namespace

int main()
{
	TestAllowedFilterMovesActorAndReportsPostMove();
	TestAllowedSameTileMoveUpdatesPositionAndOneDirtyTile();
	TestBlockedByNpcKeepsActorAndReportsBlocker();
	TestNoStepProposalKeepsActorAndReportsNoMovement();
	TestActorIdMismatchRejectsAndKeepsActor();
	TestActorNotPresentRejectsAndKeepsActor();
	TestNamespacedAndUnqualifiedActorIdsRemainExact();
	TestInputsAreNotMutated();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
