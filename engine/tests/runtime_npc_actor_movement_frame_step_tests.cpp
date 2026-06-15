#include <cstdlib>
#include <type_traits>
#include <vector>

#include "runtime/RuntimeNpcActorMovementFrameStep.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

template <typename T, typename = void>
struct HasNpcActorsField : std::false_type {
};

template <typename T>
struct HasNpcActorsField<T, std::void_t<decltype(&T::npcActors)>> : std::true_type {
};

template <typename T, typename = void>
struct HasNpcControlsField : std::false_type {
};

template <typename T>
struct HasNpcControlsField<T, std::void_t<decltype(&T::npcControls)>> : std::true_type {
};

static_assert(!HasNpcActorsField<iggy::runtime::RuntimeSessionState>::value);
static_assert(!HasNpcControlsField<iggy::runtime::RuntimeSessionState>::value);

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
		Id("ai-profile:runtime-movement"),
		Id("faction:runtime-movement"),
		position,
		Id("goal:runtime-movement"),
		present,
	};
}

iggy::NpcActorState2DRegistry ActorRegistry(std::vector<iggy::NpcActorState2D> actors)
{
	iggy::NpcActorState2DRegistry registry;
	registry.actors = actors;
	return registry;
}

iggy::NpcActorControlState2D Control(const char *npcId)
{
	return {
		Id(npcId),
		iggy::waitNpcObjective(),
		iggy::idleNpcBehaviorState(),
		iggy::NpcMoveMode::Still,
	};
}

iggy::NpcActorControlState2DRegistry ControlRegistry(
	std::vector<iggy::NpcActorControlState2D> controls)
{
	iggy::NpcActorControlState2DRegistry registry;
	registry.entries = controls;
	return registry;
}

iggy::runtime::GameplayCommandFrame2D WaitFrame(const char *actorId)
{
	return {
		{ iggy::runtime::GameplayCommand2DFactory {}.wait(Id(actorId)) },
	};
}

iggy::InteractionTarget2DRegistry InteractionTargets()
{
	return iggy::InteractionTarget2DRegistry {
		{ {
			Id("target:runtime-movement"),
			iggy::InteractionTarget2DKind::Usable,
			{ 1.0F, 2.0F },
			0.5F,
			true,
		} },
	};
}

iggy::InventoryState2D Inventory()
{
	return {
		{ { Id("item:runtime-movement"), 2 } },
	};
}

iggy::LevelItemDrop2DRegistry Drops()
{
	return {
		{ { Id("drop:runtime-movement"), Id("item:runtime-movement"), 1, { 3.0F, 4.0F }, 0.25F, true } },
	};
}

iggy::runtime::RuntimeGameplayState GameplayState(
	iggy::NpcActorState2DRegistry actors = {},
	iggy::NpcActorControlState2DRegistry controls = {})
{
	iggy::runtime::RuntimeGameplayState state;
	state.session.tickIndex = 77;
	state.session.hasPlayer = true;
	state.session.player.id = Id("player:runtime-movement");
	state.session.player.position = { 9.0F, 10.0F };
	state.commandQueue.frames = { WaitFrame("player:queued") };
	state.interaction.targets = InteractionTargets();
	state.inventory.inventory = Inventory();
	state.inventory.drops = Drops();
	state.npcActors = actors;
	state.npcControls = controls;
	return state;
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

iggy::NpcActorMovementFrameApply2DRequest Request(
	const iggy::NpcActorPathStepOccupancyFilter2D &filter)
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

bool SameActors(
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

bool SameControls(
	const iggy::NpcActorControlState2DRegistry &actual,
	const iggy::NpcActorControlState2DRegistry &expected)
{
	if (actual.entries.size() != expected.entries.size()) {
		return false;
	}
	for (std::size_t index = 0; index < actual.entries.size(); ++index) {
		if (actual.entries[index].npcId != expected.entries[index].npcId
			|| actual.entries[index].moveMode != expected.entries[index].moveMode
			|| actual.entries[index].objective.type != expected.entries[index].objective.type
			|| actual.entries[index].behavior.type != expected.entries[index].behavior.type) {
			return false;
		}
	}
	return true;
}

bool SameGameplayNonNpcActors(
	const iggy::runtime::RuntimeGameplayState &actual,
	const iggy::runtime::RuntimeGameplayState &expected)
{
	return actual.session.tickIndex == expected.session.tickIndex
		&& actual.session.hasPlayer == expected.session.hasPlayer
		&& actual.session.player.id == expected.session.player.id
		&& NearVec(actual.session.player.position, expected.session.player.position)
		&& actual.commandQueue.frames.size() == expected.commandQueue.frames.size()
		&& actual.interaction.targets.targets().size() == expected.interaction.targets.targets().size()
		&& actual.inventory.inventory.stacks.size() == expected.inventory.inventory.stacks.size()
		&& actual.inventory.drops.drops.size() == expected.inventory.drops.drops.size()
		&& SameControls(actual.npcControls, expected.npcControls);
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

void TestEmptyRequestsPreserveFullGameplayState()
{
	const iggy::runtime::RuntimeGameplayState state = GameplayState(
		ActorRegistry({ Actor("npc:guard", { 0.5F, 0.5F }) }),
		ControlRegistry({ Control("npc:guard") }));
	const iggy::runtime::RuntimeNpcActorMovementFrameInput input { state, {} };

	const iggy::runtime::RuntimeNpcActorMovementFrameResult result =
		iggy::runtime::RuntimeNpcActorMovementFrameStep {}.run(input);

	Expect(!result.changed, "empty runtime NPC movement frame should not change");
	Expect(result.apply.status == iggy::NpcActorMovementFrameApply2DStatus::NoMovementsApplied, "empty runtime NPC movement frame should preserve apply status");
	Expect(!result.report.changed(), "empty runtime NPC movement frame should preserve unchanged report");
	Expect(SameActors(result.state.npcActors, state.npcActors), "empty runtime NPC movement frame should preserve actor registry");
	Expect(SameGameplayNonNpcActors(result.state, state), "empty runtime NPC movement frame should preserve non-actor gameplay state");
}

void TestAllowedMovementUpdatesReturnedNpcActorsOnly()
{
	const iggy::runtime::RuntimeGameplayState state = GameplayState(
		ActorRegistry({
			Actor("npc:mover", { 0.5F, 0.5F }),
			Actor("npc:other", { 4.5F, 0.5F }),
		}),
		ControlRegistry({
			Control("npc:mover"),
			Control("npc:other"),
		}));
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> requests {
		Request(Filter("npc:mover", { 0.5F, 0.5F }, { 1.5F, 0.5F })),
	};
	const iggy::runtime::RuntimeNpcActorMovementFrameInput input { state, requests };

	const iggy::runtime::RuntimeNpcActorMovementFrameResult result =
		iggy::runtime::RuntimeNpcActorMovementFrameStep {}.run(input);

	Expect(result.changed, "allowed runtime NPC movement frame should change");
	Expect(result.apply.movedCount == 1 && result.report.movedCount == 1, "allowed runtime NPC movement frame should preserve moved counts");
	Expect(NearVec(result.state.npcActors.actors[0].position, { 1.5F, 0.5F }), "allowed runtime NPC movement frame should update returned actor position");
	Expect(NearVec(result.state.npcActors.actors[1].position, { 4.5F, 0.5F }), "allowed runtime NPC movement frame should preserve other actors");
	Expect(NearVec(state.npcActors.actors[0].position, { 0.5F, 0.5F }), "allowed runtime NPC movement frame should not mutate input actor registry");
	Expect(SameGameplayNonNpcActors(result.state, state), "allowed runtime NPC movement frame should preserve session/queue/interaction/inventory/controls");
}

void TestBlockedRequestPreservesActorAndReportsBlockedFacts()
{
	const iggy::runtime::RuntimeGameplayState state = GameplayState(
		ActorRegistry({ Actor("npc:blocked", { 0.5F, 0.5F }) }),
		ControlRegistry({ Control("npc:blocked") }));
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> requests {
		Request(Filter(
			"npc:blocked",
			{ 0.5F, 0.5F },
			{ 1.5F, 0.5F },
			iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc,
			false,
			"npc:blocker")),
	};

	const iggy::runtime::RuntimeNpcActorMovementFrameResult result =
		iggy::runtime::RuntimeNpcActorMovementFrameStep {}.run({ state, requests });

	Expect(!result.changed, "blocked runtime NPC movement frame should not change");
	Expect(SameActors(result.state.npcActors, state.npcActors), "blocked runtime NPC movement frame should preserve actor registry");
	Expect(result.apply.blockedCount == 1 && result.report.blockedCount == 1, "blocked runtime NPC movement frame should preserve blocked counts");
	Expect(result.apply.entries.size() == 1, "blocked runtime NPC movement frame should preserve apply entry");
	Expect(result.apply.entries[0].executor.postMove.blockingNpcId == Id("npc:blocker"), "blocked runtime NPC movement frame should preserve blocking NPC id");
	Expect(SameGameplayNonNpcActors(result.state, state), "blocked runtime NPC movement frame should preserve non-actor gameplay state");
}

void TestMixedMovedBlockedAndMissingRequestsPreserveDiagnostics()
{
	const iggy::runtime::RuntimeGameplayState state = GameplayState(
		ActorRegistry({
			Actor("npc:mover", { 0.5F, 0.5F }),
			Actor("npc:blocked", { 2.5F, 0.5F }),
		}),
		ControlRegistry({
			Control("npc:mover"),
			Control("npc:blocked"),
		}));
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> requests {
		Request(Filter("npc:mover", { 0.5F, 0.5F }, { 1.5F, 0.5F })),
		Request(Filter(
			"npc:blocked",
			{ 2.5F, 0.5F },
			{ 3.5F, 0.5F },
			iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc,
			false,
			"npc:blocker")),
		Request(Filter("npc:missing", { 4.5F, 0.5F }, { 5.5F, 0.5F })),
	};
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> originalRequests = requests;

	const iggy::runtime::RuntimeNpcActorMovementFrameResult result =
		iggy::runtime::RuntimeNpcActorMovementFrameStep {}.run({ state, requests });

	Expect(result.changed, "mixed runtime NPC movement frame should change when one actor moves");
	Expect(result.apply.requestCount == 3 && result.report.requestCount == 3, "mixed runtime NPC movement frame should preserve request count");
	Expect(result.apply.movedCount == 1 && result.apply.blockedCount == 1 && result.apply.missingActorCount == 1, "mixed runtime NPC movement frame should preserve apply counts");
	Expect(result.report.movedCount == 1 && result.report.blockedCount == 1 && result.report.missingActorCount == 1, "mixed runtime NPC movement frame should preserve report counts");
	Expect(NearVec(result.state.npcActors.actors[0].position, { 1.5F, 0.5F }), "mixed runtime NPC movement frame should update moved actor");
	Expect(NearVec(result.state.npcActors.actors[1].position, { 2.5F, 0.5F }), "mixed runtime NPC movement frame should preserve blocked actor");
	Expect(result.report.dirtyTiles.size() == 2, "mixed runtime NPC movement frame should aggregate moved dirty tiles only");
	Expect(result.report.needsOccupancyRebuild && result.report.needsAiMapQueryRefresh, "mixed runtime NPC movement frame should expose refresh facts from moved entry");
	Expect(result.apply.entries.size() == 3 && result.apply.entries[2].hasIssue, "mixed runtime NPC movement frame should preserve missing actor issue");
	Expect(SameRequests(requests, originalRequests), "runtime NPC movement frame should not mutate request vector");
}

} // namespace

int main()
{
	TestEmptyRequestsPreserveFullGameplayState();
	TestAllowedMovementUpdatesReturnedNpcActorsOnly();
	TestBlockedRequestPreservesActorAndReportsBlockedFacts();
	TestMixedMovedBlockedAndMissingRequestsPreserveDiagnostics();

	return Failures;
}
