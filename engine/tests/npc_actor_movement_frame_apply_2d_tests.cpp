#include <cstdlib>
#include <vector>

#include "scene/npc/NpcActorMovementFrameApply2D.hpp"
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

bool SameFilter(
	const iggy::NpcActorPathStepOccupancyFilter2D &actual,
	const iggy::NpcActorPathStepOccupancyFilter2D &expected)
{
	return actual.step.npcId == expected.step.npcId
		&& NearVec(actual.step.oldPosition, expected.step.oldPosition)
		&& NearVec(actual.step.proposedPosition, expected.step.proposedPosition)
		&& actual.step.oldTile == expected.step.oldTile
		&& actual.step.proposedTile == expected.step.proposedTile
		&& actual.step.status == expected.step.status
		&& actual.step.requestsMovement == expected.step.requestsMovement
		&& actual.status == expected.status
		&& actual.requestsMovement == expected.requestsMovement
		&& actual.blockingNpcId == expected.blockingNpcId;
}

bool SameRequests(
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> &actual,
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> &expected)
{
	if (actual.size() != expected.size()) {
		return false;
	}
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (!SameFilter(actual[index].filter, expected[index].filter)) {
			return false;
		}
	}
	return true;
}

void TestEmptyRequestListReturnsNoChanges()
{
	const iggy::NpcActorState2DRegistry registry = Registry({
		Actor("npc:guard", { 0.5F, 0.5F }),
	});

	const iggy::NpcActorMovementFrameApply2DResult result =
		iggy::NpcActorMovementFrameApplier2D {}.apply(registry, {});

	Expect(result.status == iggy::NpcActorMovementFrameApply2DStatus::NoMovementsApplied, "empty batch should not apply movement");
	Expect(!result.applied(), "empty batch applied helper should be false");
	Expect(!result.hasChanges(), "empty batch should not have changes");
	Expect(result.requestCount == 0, "empty batch should preserve zero request count");
	Expect(result.entries.empty(), "empty batch should have no entries");
	Expect(SameRegistry(result.inputRegistry, registry), "empty batch should preserve copied input registry");
	Expect(SameRegistry(result.registry, registry), "empty batch should return unchanged registry");
}

void TestSingleAllowedMovementUpdatesMatchingActor()
{
	const iggy::NpcActorState2DRegistry registry = Registry({
		Actor("npc:mover", { 0.5F, 0.5F }),
		Actor("npc:other", { 4.5F, 0.5F }),
	});
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> requests {
		Request(Filter("npc:mover", { 0.5F, 0.5F }, { 1.5F, 0.5F })),
	};

	const iggy::NpcActorMovementFrameApply2DResult result =
		iggy::NpcActorMovementFrameApplier2D {}.apply(registry, requests);

	Expect(result.status == iggy::NpcActorMovementFrameApply2DStatus::Applied, "allowed movement should apply");
	Expect(result.applied(), "allowed movement applied helper should be true");
	Expect(result.changed, "allowed movement should mark changed");
	Expect(result.requestCount == 1, "allowed movement should preserve request count");
	Expect(result.appliedCount == 1 && result.movedCount == 1 && result.changedCount == 1, "allowed movement should update applied/moved/changed counts");
	Expect(result.blockedCount == 0 && result.rejectedCount == 0 && result.missingActorCount == 0, "allowed movement should not increment failure counts");
	Expect(result.entries.size() == 1, "allowed movement should preserve one entry");
	Expect(result.entries[0].requestIndex == 0, "entry should preserve request index");
	Expect(result.entries[0].actorIndex.has_value() && *result.entries[0].actorIndex == 0, "entry should preserve actor index");
	Expect(!result.entries[0].hasIssue, "allowed movement should not preserve issue");
	Expect(result.entries[0].executor.status == iggy::NpcActorMovementExecutor2DStatus::Moved, "entry should preserve moved executor result");
	Expect(NearVec(result.registry.actors[0].position, { 1.5F, 0.5F }), "returned registry should update moved actor");
	Expect(NearVec(result.registry.actors[1].position, { 4.5F, 0.5F }), "returned registry should preserve other actor");
}

void TestBlockedNoStepAndAbsentRequestsDoNotMoveActors()
{
	const iggy::NpcActorState2DRegistry registry = Registry({
		Actor("npc:blocked", { 0.5F, 0.5F }),
		Actor("npc:idle", { 2.5F, 0.5F }),
		Actor("npc:absent", { 4.5F, 0.5F }, false),
	});
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> requests {
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
	};

	const iggy::NpcActorMovementFrameApply2DResult result =
		iggy::NpcActorMovementFrameApplier2D {}.apply(registry, requests);

	Expect(result.status == iggy::NpcActorMovementFrameApply2DStatus::NoMovementsApplied, "blocked/no-step/rejected batch should not apply movements");
	Expect(!result.changed, "blocked/no-step/rejected batch should not change registry");
	Expect(SameRegistry(result.registry, registry), "blocked/no-step/rejected batch should preserve registry");
	Expect(result.blockedCount == 1, "blocked request should increment blocked count");
	Expect(result.noMovementCount == 1, "no-step request should increment no movement count");
	Expect(result.rejectedCount == 1, "absent actor request should increment rejected count");
	Expect(result.appliedCount == 0 && result.changedCount == 0, "no moved actors should keep applied/changed counts zero");
	Expect(result.entries[0].executor.postMove.status == iggy::NpcActorPostMoveReport2DStatus::Blocked, "blocked entry should preserve blocked post-move report");
	Expect(result.entries[0].executor.postMove.blockingNpcId == Id("npc:blocker"), "blocked entry should preserve blocking NPC id");
	Expect(result.entries[1].executor.postMove.status == iggy::NpcActorPostMoveReport2DStatus::NotMoved, "no-step entry should preserve not-moved post report");
	Expect(result.entries[2].executor.status == iggy::NpcActorMovementExecutor2DStatus::ActorNotPresent, "absent actor entry should preserve executor status");
}

void TestMissingActorRecordsIssueAndContinues()
{
	const iggy::NpcActorState2DRegistry registry = Registry({
		Actor("npc:mover", { 0.5F, 0.5F }),
	});
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> requests {
		Request(Filter("npc:missing", { 9.5F, 0.5F }, { 10.5F, 0.5F })),
		Request(Filter("npc:mover", { 0.5F, 0.5F }, { 1.5F, 0.5F })),
	};

	const iggy::NpcActorMovementFrameApply2DResult result =
		iggy::NpcActorMovementFrameApplier2D {}.apply(registry, requests);

	Expect(result.status == iggy::NpcActorMovementFrameApply2DStatus::Applied, "later valid request should still apply after missing actor");
	Expect(result.entries.size() == 2, "missing actor batch should preserve both entries");
	Expect(result.entries[0].hasIssue, "missing actor entry should preserve issue");
	Expect(result.entries[0].issue == iggy::NpcActorMovementFrameApply2DIssueCode::ActorNotFound, "missing actor entry should use ActorNotFound issue");
	Expect(!result.entries[0].actorIndex.has_value(), "missing actor entry should not preserve actor index");
	Expect(result.missingActorCount == 1, "missing actor count should increment");
	Expect(result.movedCount == 1 && result.changedCount == 1, "later valid request should move actor");
	Expect(NearVec(result.registry.actors[0].position, { 1.5F, 0.5F }), "later valid request should update registry");
}

void TestDuplicateSameNpcRequestsAreSequentialAndLaterSuccessWins()
{
	const iggy::NpcActorState2DRegistry registry = Registry({
		Actor("npc:mover", { 0.5F, 0.5F }),
	});
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> requests {
		Request(Filter("npc:mover", { 0.5F, 0.5F }, { 1.5F, 0.5F })),
		Request(Filter("npc:mover", { 1.5F, 0.5F }, { 2.5F, 0.5F })),
	};

	const iggy::NpcActorMovementFrameApply2DResult result =
		iggy::NpcActorMovementFrameApplier2D {}.apply(registry, requests);

	Expect(result.movedCount == 2 && result.changedCount == 2, "duplicate successful requests should both move sequentially");
	Expect(NearVec(result.registry.actors[0].position, { 2.5F, 0.5F }), "later successful duplicate should win final registry position");
	Expect(NearVec(result.entries[0].executor.actor.position, { 0.5F, 0.5F }), "first executor should see original actor position");
	Expect(NearVec(result.entries[1].executor.actor.position, { 1.5F, 0.5F }), "second executor should see carried actor position from first move");
	Expect(result.entries[0].actorIndex.has_value() && result.entries[1].actorIndex.has_value(), "duplicate entries should preserve actor index");
	Expect(*result.entries[0].actorIndex == 0 && *result.entries[1].actorIndex == 0, "duplicate entries should target same carried actor index");
}

void TestLaterFailedDuplicateDoesNotEraseEarlierSuccess()
{
	const iggy::NpcActorState2DRegistry registry = Registry({
		Actor("npc:mover", { 0.5F, 0.5F }),
	});
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> requests {
		Request(Filter("npc:mover", { 0.5F, 0.5F }, { 1.5F, 0.5F })),
		Request(Filter(
			"npc:mover",
			{ 1.5F, 0.5F },
			{ 2.5F, 0.5F },
			iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc,
			false,
			"npc:blocker")),
	};

	const iggy::NpcActorMovementFrameApply2DResult result =
		iggy::NpcActorMovementFrameApplier2D {}.apply(registry, requests);

	Expect(result.movedCount == 1 && result.blockedCount == 1, "later failed duplicate should preserve moved and blocked counts");
	Expect(NearVec(result.registry.actors[0].position, { 1.5F, 0.5F }), "later failed duplicate should not erase earlier success");
	Expect(result.entries[1].executor.status == iggy::NpcActorMovementExecutor2DStatus::BlockedByNpc, "later duplicate should preserve blocked executor status");
	Expect(NearVec(result.entries[1].executor.actor.position, { 1.5F, 0.5F }), "later duplicate should still see carried position before failing");
}

void TestNamespacedAndUnqualifiedIdsRemainDistinctInLookup()
{
	const iggy::NpcActorState2DRegistry registry = Registry({
		Actor("guard", { 0.5F, 0.5F }),
	});
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> requests {
		Request(Filter("npc:guard", { 0.5F, 0.5F }, { 1.5F, 0.5F })),
		Request(Filter("guard", { 0.5F, 0.5F }, { 2.5F, 0.5F })),
	};

	const iggy::NpcActorMovementFrameApply2DResult result =
		iggy::NpcActorMovementFrameApplier2D {}.apply(registry, requests);

	Expect(result.missingActorCount == 1, "namespaced request should not match unqualified actor");
	Expect(result.movedCount == 1, "exact unqualified request should move actor");
	Expect(result.entries[0].hasIssue, "namespaced mismatch should record missing actor issue");
	Expect(result.entries[1].actorIndex.has_value() && *result.entries[1].actorIndex == 0, "exact unqualified request should find actor");
	Expect(result.registry.actors[0].npcId == Id("guard"), "result should preserve unqualified actor id");
	Expect(NearVec(result.registry.actors[0].position, { 2.5F, 0.5F }), "exact unqualified request should update position");
}

void TestInputsAreNotMutated()
{
	iggy::NpcActorState2DRegistry registry = Registry({
		Actor("npc:mover", { 0.5F, 0.5F }),
	});
	std::vector<iggy::NpcActorMovementFrameApply2DRequest> requests {
		Request(Filter("npc:mover", { 0.5F, 0.5F }, { 1.5F, 0.5F })),
	};
	const iggy::NpcActorState2DRegistry registryBefore = registry;
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> requestsBefore = requests;

	const iggy::NpcActorMovementFrameApply2DResult result =
		iggy::NpcActorMovementFrameApplier2D {}.apply(registry, requests);

	Expect(result.changed, "immutability setup should change returned registry");
	Expect(SameRegistry(registry, registryBefore), "frame applier should not mutate input registry");
	Expect(SameRequests(requests, requestsBefore), "frame applier should not mutate input requests");
	Expect(SameRegistry(result.inputRegistry, registryBefore), "result should preserve copied input registry");
}

} // namespace

int main()
{
	TestEmptyRequestListReturnsNoChanges();
	TestSingleAllowedMovementUpdatesMatchingActor();
	TestBlockedNoStepAndAbsentRequestsDoNotMoveActors();
	TestMissingActorRecordsIssueAndContinues();
	TestDuplicateSameNpcRequestsAreSequentialAndLaterSuccessWins();
	TestLaterFailedDuplicateDoesNotEraseEarlierSuccess();
	TestNamespacedAndUnqualifiedIdsRemainDistinctInLookup();
	TestInputsAreNotMutated();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
