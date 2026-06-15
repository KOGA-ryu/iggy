#include <cstdlib>
#include <vector>

#include "scene/npc/NpcActorMovementFrameApply2D.hpp"
#include "scene/npc/NpcActorMovementFrameReport2D.hpp"
#include "scene/npc/NpcActorMovementRefreshFrame2D.hpp"
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
		Id("profile:refresh-frame"),
		Id("faction:refresh-frame"),
		position,
		Id("goal:refresh-frame"),
		present,
	};
}

iggy::NpcActorState2DRegistry Actors(std::vector<iggy::NpcActorState2D> actors)
{
	const iggy::NpcActorState2DRegistryBuildResult result =
		iggy::NpcActorState2DRegistryBuilder {}.build(actors);
	Expect(result.built, "refresh frame actor registry fixture should build");
	return result.registry;
}

iggy::InteractionTarget2D Target(
	const char *targetId,
	iggy::Vec2 position,
	bool enabled = true)
{
	return {
		Id(targetId),
		iggy::InteractionTarget2DKind::Usable,
		position,
		0.5F,
		enabled,
	};
}

iggy::InteractionTarget2DRegistry Targets(std::vector<iggy::InteractionTarget2D> targets)
{
	const iggy::InteractionTarget2DRegistryBuildResult result =
		iggy::InteractionTarget2DRegistryBuilder {}.build(targets);
	Expect(result.built, "refresh frame target registry fixture should build");
	return result.registry;
}

iggy::AiMapNode2D Node(
	const char *nodeId,
	iggy::Vec2 position,
	std::vector<iggy::ResourceId> tags = {},
	bool enabled = true)
{
	return {
		Id(nodeId),
		position,
		0.75F,
		1.0F,
		2.0F,
		3.0F,
		4.0F,
		tags,
		{},
		enabled,
	};
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

iggy::NpcActorMovementFrameReport2D MovementReport(
	const iggy::NpcActorState2DRegistry &actors,
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> &requests)
{
	const iggy::NpcActorMovementFrameApply2DResult apply =
		iggy::NpcActorMovementFrameApplier2D {}.apply(actors, requests);
	return iggy::NpcActorMovementFrameReporter2D {}.report(apply);
}

iggy::NpcActorOccupancy2D Occupancy(const iggy::NpcActorState2DRegistry &actors)
{
	return iggy::NpcActorOccupancyProjector2D {}.project(actors);
}

iggy::NpcActorMovementRefreshFrame2DInput Input(
	const iggy::NpcActorMovementFrameReport2D &report,
	const iggy::NpcActorState2DRegistry &actors,
	const iggy::NpcActorOccupancy2D &previousOccupancy,
	const iggy::InteractionTarget2DRegistry &targets,
	const iggy::AiMap2D &aiMap,
	const iggy::NpcActorMovementRefreshFrame2DConfig &config = {})
{
	iggy::NpcActorMovementRefreshFrame2DInput input;
	input.movementReport = report;
	input.actors = actors;
	input.previousOccupancy = previousOccupancy;
	input.interactionTargets = targets;
	input.aiMap = aiMap;
	input.config = config;
	return input;
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
			|| left.aiProfileId != right.aiProfileId
			|| left.factionId != right.factionId
			|| !NearVec(left.position, right.position)
			|| left.currentGoalId != right.currentGoalId
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

bool SameAiMap(const iggy::AiMap2D &actual, const iggy::AiMap2D &expected)
{
	if (actual.nodes.size() != expected.nodes.size()) {
		return false;
	}
	for (std::size_t index = 0; index < actual.nodes.size(); ++index) {
		const iggy::AiMapNode2D &left = actual.nodes[index];
		const iggy::AiMapNode2D &right = expected.nodes[index];
		if (left.id != right.id
			|| !NearVec(left.position, right.position)
			|| left.radius != right.radius
			|| left.tags != right.tags
			|| left.enabled != right.enabled) {
			return false;
		}
	}
	return true;
}

void TestEmptyNoOpReportProducesNoRefreshes()
{
	const iggy::NpcActorState2DRegistry actors = Actors({});
	const iggy::NpcActorOccupancy2D previousOccupancy = Occupancy(actors);
	const iggy::NpcActorMovementFrameReport2D movementReport;

	const iggy::NpcActorMovementRefreshFrame2DResult result =
		iggy::NpcActorMovementRefreshFrameProjector2D {}.project(Input(
			movementReport,
			actors,
			previousOccupancy,
			Targets({}),
			{}));

	Expect(!result.hasRefreshWork(), "empty movement report should produce no refresh work");
	Expect(!result.refreshedAny(), "empty movement report should refresh no consumers");
	Expect(result.dirtyTileCount == 0, "empty movement report should have zero dirty tiles");
	Expect(!result.occupancyRefreshed && !result.interactionRefreshed && !result.aiMapRefreshed, "empty movement report should leave data refresh flags false");
	Expect(!result.renderRefreshed && !result.visibilityRefreshed, "empty movement report should leave visual refresh flags false");
}

void TestMovedActorRefreshesAllConsumers()
{
	const iggy::NpcActorState2DRegistry initialActors = Actors({
		Actor("npc:mover", { 0.5F, 0.5F }),
	});
	const iggy::NpcActorOccupancy2D previousOccupancy = Occupancy(initialActors);
	const iggy::NpcActorMovementFrameReport2D movementReport = MovementReport(
		initialActors,
		{ Request(Filter(
			"npc:mover",
			{ 0.5F, 0.5F },
			{ 1.5F, 0.5F },
			iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed,
			true)) });
	const iggy::NpcActorState2DRegistry returnedActors = movementReport.registry;
	const iggy::InteractionTarget2DRegistry targets = Targets({
		Target("target:new", { 1.5F, 0.5F }),
	});
	const iggy::AiMap2D aiMap = { {
		Node("ai:new", { 1.5F, 0.5F }, { Id("tag:new") }),
	} };

	const iggy::NpcActorMovementRefreshFrame2DResult result =
		iggy::NpcActorMovementRefreshFrameProjector2D {}.project(Input(
			movementReport,
			returnedActors,
			previousOccupancy,
			targets,
			aiMap));

	Expect(result.hasRefreshWork(), "moved actor should produce refresh work");
	Expect(result.refreshedAny(), "moved actor should refresh consumers");
	Expect(result.dirtyTileCount == 2, "moved actor should preserve old/new dirty tiles");
	Expect(result.occupancyRefreshed, "moved actor should refresh occupancy");
	Expect(result.interactionRefreshed, "moved actor should refresh interaction facts");
	Expect(result.aiMapRefreshed, "moved actor should refresh ai map query facts");
	Expect(result.renderRefreshed && result.visibilityRefreshed, "moved actor should refresh visual packets");

	Expect(result.occupancy.occupancy.entries.size() == 1
		&& result.occupancy.occupancy.entries[0].npcId == Id("npc:mover")
		&& result.occupancy.occupancy.entries[0].tile == iggy::TileCoord { 1, 0 }, "combined refresh should rebuild occupancy from returned actor registry");
	Expect(result.interaction.affectedActorCount == 1 && result.interaction.affectedActors[0].npcId == Id("npc:mover"), "combined refresh should report moved interaction actor");
	Expect(result.interaction.affectedTargetCount == 1 && result.interaction.affectedTargets[0].targetId == Id("target:new"), "combined refresh should report dirty interaction target");
	Expect(result.aiMap.affectedActorCount == 1 && result.aiMap.affectedActors[0].npcId == Id("npc:mover"), "combined refresh should report moved ai map actor");
	Expect(result.aiMap.affectedActors[0].query.tags == std::vector<iggy::ResourceId>({ Id("tag:new") }), "combined refresh should preserve nested ai map query tags");
	Expect(result.visual.render.affectedActorCount == 1 && result.visual.render.affectedActors[0].npcId == Id("npc:mover"), "combined refresh should report render affected actor");
	Expect(result.visual.visibility.affectedActorCount == 1 && result.visual.visibility.affectedActors[0].npcId == Id("npc:mover"), "combined refresh should report visibility affected actor");
}

void TestBlockedMovementProducesNoRefreshes()
{
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:mover", { 0.5F, 0.5F }),
		Actor("npc:blocker", { 1.5F, 0.5F }),
	});
	const iggy::NpcActorOccupancy2D previousOccupancy = Occupancy(actors);
	const iggy::NpcActorMovementFrameReport2D movementReport = MovementReport(
		actors,
		{ Request(Filter(
			"npc:mover",
			{ 0.5F, 0.5F },
			{ 1.5F, 0.5F },
			iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc,
			false,
			"npc:blocker")) });

	const iggy::NpcActorMovementRefreshFrame2DResult result =
		iggy::NpcActorMovementRefreshFrameProjector2D {}.project(Input(
			movementReport,
			movementReport.registry,
			previousOccupancy,
			Targets({ Target("target:block", { 1.5F, 0.5F }) }),
			{ { Node("ai:block", { 1.5F, 0.5F }, { Id("tag:block") }) } }));

	Expect(!result.hasRefreshWork(), "blocked movement should produce no refresh work");
	Expect(!result.refreshedAny(), "blocked movement should refresh no consumers");
	Expect(!result.occupancyRefreshed, "blocked movement should not rebuild occupancy");
	Expect(result.interaction.affectedActors.empty(), "blocked movement should not report interaction actors");
	Expect(result.aiMap.affectedActors.empty(), "blocked movement should not report ai map actors");
	Expect(result.visual.render.affectedActors.empty() && result.visual.visibility.affectedActors.empty(), "blocked movement should not report visual actors");
}

void TestConfigOptionsPassThrough()
{
	const iggy::NpcActorState2DRegistry initialActors = Actors({
		Actor("npc", { 0.5F, 0.5F }),
	});
	const iggy::NpcActorOccupancy2D previousOccupancy = Occupancy(initialActors);
	const iggy::NpcActorMovementFrameReport2D movementReport = MovementReport(
		initialActors,
		{ Request(Filter(
			"npc",
			{ 0.5F, 0.5F },
			{ 1.5F, 0.5F },
			iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed,
			true)) });
	iggy::NpcActorState2DRegistry returnedActors = movementReport.registry;
	if (!returnedActors.actors.empty()) {
		returnedActors.actors[0].present = false;
	}
	const iggy::InteractionTarget2DRegistry targets = Targets({
		Target("target:disabled", { 1.5F, 0.5F }, false),
	});
	const iggy::AiMap2D aiMap = { {
		Node("ai:config", { 1.5F, 0.5F }, { Id("tag:config") }),
	} };
	iggy::NpcActorMovementRefreshFrame2DConfig config;
	config.occupancy.occupancy.includeAbsent = true;
	config.interaction.includeAbsentActors = true;
	config.interaction.includeDisabledTargets = true;
	config.aiMap.includeAbsentActors = true;
	config.aiMap.queryAffectedActors = false;
	config.visual.includeAbsentActors = true;

	const iggy::NpcActorMovementRefreshFrame2DResult result =
		iggy::NpcActorMovementRefreshFrameProjector2D {}.project(Input(
			movementReport,
			returnedActors,
			previousOccupancy,
			targets,
			aiMap,
			config));

	Expect(result.occupancy.occupancy.entries.size() == 1 && result.occupancy.occupancy.entries[0].npcId == Id("npc"), "combined refresh should pass occupancy includeAbsent config");
	Expect(result.interaction.affectedActorCount == 1 && result.interaction.affectedActors[0].npcId == Id("npc"), "combined refresh should pass interaction absent actor config");
	Expect(result.interaction.affectedTargetCount == 1 && result.interaction.affectedTargets[0].targetId == Id("target:disabled"), "combined refresh should pass disabled target config");
	Expect(result.aiMap.affectedActorCount == 1 && result.aiMap.affectedActors[0].npcId == Id("npc"), "combined refresh should pass ai map absent actor config");
	Expect(result.aiMap.affectedActors[0].query.status == iggy::AiMapQuery2DStatus::NoMatch, "combined refresh should pass ai map query-disabled config");
	Expect(result.visual.render.affectedActorCount == 1 && result.visual.render.affectedActors[0].npcId == Id("npc"), "combined refresh should pass visual absent actor config");
	Expect(result.visual.visibility.affectedActorCount == 1 && result.visual.visibility.affectedActors[0].npcId == Id("npc"), "combined refresh should pass visual absent actor config to visibility");
}

void TestInputsAreNotMutatedAndIdsRemainExact()
{
	const iggy::NpcActorState2DRegistry initialActors = Actors({
		Actor("npc", { 0.5F, 0.5F }),
	});
	const iggy::NpcActorOccupancy2D previousOccupancy = Occupancy(initialActors);
	const iggy::NpcActorOccupancy2D previousOccupancyBefore = previousOccupancy;
	const iggy::NpcActorMovementFrameReport2D movementReport = MovementReport(
		initialActors,
		{ Request(Filter(
			"npc",
			{ 0.5F, 0.5F },
			{ 1.5F, 0.5F },
			iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed,
			true)) });
	const iggy::NpcActorMovementFrameReport2D movementReportBefore = movementReport;
	const iggy::NpcActorState2DRegistry actors = movementReport.registry;
	const iggy::NpcActorState2DRegistry actorsBefore = actors;
	const iggy::InteractionTarget2DRegistry targets = Targets({
		Target("target", { 1.5F, 0.5F }),
		Target("target:qualified", { 4.5F, 0.5F }),
	});
	const iggy::InteractionTarget2DRegistry targetsBefore = targets;
	const iggy::AiMap2D aiMap = { {
		Node("ai", { 1.5F, 0.5F }, { Id("tag") }),
		Node("ai:qualified", { 4.5F, 0.5F }, { Id("tag:qualified") }),
	} };
	const iggy::AiMap2D aiMapBefore = aiMap;

	const iggy::NpcActorMovementRefreshFrame2DResult result =
		iggy::NpcActorMovementRefreshFrameProjector2D {}.project(Input(
			movementReport,
			actors,
			previousOccupancy,
			targets,
			aiMap));

	Expect(result.interaction.affectedTargets.size() == 1 && result.interaction.affectedTargets[0].targetId == Id("target"), "combined refresh should preserve unqualified ids exactly");
	Expect(result.aiMap.affectedActors.size() == 1 && result.aiMap.affectedActors[0].npcId == Id("npc"), "combined refresh should preserve npc ids exactly");
	Expect(movementReport.dirtyTiles == movementReportBefore.dirtyTiles
		&& movementReport.needsRenderRefresh == movementReportBefore.needsRenderRefresh, "combined refresh should not mutate movement report");
	Expect(SameActors(actors, actorsBefore), "combined refresh should not mutate actors");
	Expect(previousOccupancy.entries.size() == previousOccupancyBefore.entries.size()
		&& previousOccupancy.occupiedTiles.size() == previousOccupancyBefore.occupiedTiles.size(), "combined refresh should not mutate previous occupancy");
	Expect(SameTargets(targets, targetsBefore), "combined refresh should not mutate interaction targets");
	Expect(SameAiMap(aiMap, aiMapBefore), "combined refresh should not mutate ai map");
}

} // namespace

int main()
{
	TestEmptyNoOpReportProducesNoRefreshes();
	TestMovedActorRefreshesAllConsumers();
	TestBlockedMovementProducesNoRefreshes();
	TestConfigOptionsPassThrough();
	TestInputsAreNotMutatedAndIdsRemainExact();

	return Failures;
}
