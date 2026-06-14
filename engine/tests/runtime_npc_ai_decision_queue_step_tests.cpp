#include <cstdlib>
#include <string_view>
#include <vector>

#include "runtime/RuntimeNpcAiDecisionQueueStep.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::AiMapNode2D Node(
	const char *id,
	iggy::Vec2 position,
	float radius,
	float patrolWeight,
	float coverWeight,
	float dangerWeight,
	float interestWeight,
	bool enabled = true)
{
	return {
		Id(id),
		position,
		radius,
		patrolWeight,
		coverWeight,
		dangerWeight,
		interestWeight,
		{},
		{},
		enabled,
	};
}

iggy::AiMap2D AiMap(std::vector<iggy::AiMapNode2D> nodes)
{
	return { nodes };
}

iggy::LevelTileMap Grid(std::vector<std::string_view> rows)
{
	iggy::LevelTileMap map;
	map.height = static_cast<int>(rows.size());
	map.width = rows.empty() ? 0 : static_cast<int>(rows.front().size());
	for (std::string_view row : rows) {
		for (char cell : row)
			map.tiles.push_back({ cell != '#' });
	}
	return map;
}

iggy::NpcAiProfile2D Profile(
	const char *profileId = "ai-profile:queue",
	float aggression = 0.0F,
	float bravery = 0.0F,
	float alertness = 0.0F)
{
	return {
		Id(profileId),
		Id("faction:queue"),
		aggression,
		bravery,
		alertness,
		4.0F,
		{},
	};
}

iggy::NpcAiCurrentState2D State(
	const char *npcId,
	iggy::Vec2 position = { 0.5F, 0.5F },
	bool enabled = true)
{
	return {
		Id(npcId),
		position,
		Id("goal:queue"),
		enabled,
	};
}

iggy::runtime::RuntimeNpcAiDecisionQueueNpcInput Npc(
	const char *npcId,
	iggy::Vec2 position = { 0.5F, 0.5F })
{
	return {
		Profile(npcId),
		State(npcId, position),
	};
}

iggy::runtime::GameplayCommandFrame2D MoveFrame(const char *actorId, iggy::Vec2 point)
{
	return { { iggy::runtime::GameplayCommand2DFactory {}.moveToPoint(Id(actorId), point) } };
}

bool SameCommand(const iggy::runtime::GameplayCommand2D &actual, const iggy::runtime::GameplayCommand2D &expected)
{
	return actual.type == expected.type
		&& actual.actorId == expected.actorId
		&& NearVec(actual.targetPoint, expected.targetPoint)
		&& actual.targetTile == expected.targetTile
		&& actual.targetId == expected.targetId;
}

bool SameFrame(const iggy::runtime::GameplayCommandFrame2D &actual, const iggy::runtime::GameplayCommandFrame2D &expected)
{
	if (actual.commands.size() != expected.commands.size())
		return false;
	for (std::size_t index = 0; index < actual.commands.size(); ++index) {
		if (!SameCommand(actual.commands[index], expected.commands[index]))
			return false;
	}
	return true;
}

bool SameQueue(const iggy::runtime::RuntimeCommandQueueState &actual, const iggy::runtime::RuntimeCommandQueueState &expected)
{
	if (actual.frames.size() != expected.frames.size())
		return false;
	for (std::size_t index = 0; index < actual.frames.size(); ++index) {
		if (!SameFrame(actual.frames[index], expected.frames[index]))
			return false;
	}
	return true;
}

bool SameAiMap(const iggy::AiMap2D &actual, const iggy::AiMap2D &expected)
{
	if (actual.nodes.size() != expected.nodes.size())
		return false;
	for (std::size_t index = 0; index < actual.nodes.size(); ++index) {
		if (actual.nodes[index].id != expected.nodes[index].id
			|| !NearVec(actual.nodes[index].position, expected.nodes[index].position)
			|| actual.nodes[index].radius != expected.nodes[index].radius
			|| actual.nodes[index].patrolWeight != expected.nodes[index].patrolWeight
			|| actual.nodes[index].coverWeight != expected.nodes[index].coverWeight
			|| actual.nodes[index].dangerWeight != expected.nodes[index].dangerWeight
			|| actual.nodes[index].interestWeight != expected.nodes[index].interestWeight
			|| actual.nodes[index].tags != expected.nodes[index].tags
			|| actual.nodes[index].links != expected.nodes[index].links
			|| actual.nodes[index].enabled != expected.nodes[index].enabled) {
			return false;
		}
	}
	return true;
}

bool SameGrid(const iggy::LevelTileMap &actual, const iggy::LevelTileMap &expected)
{
	if (actual.id != expected.id
		|| actual.width != expected.width
		|| actual.height != expected.height
		|| actual.tiles.size() != expected.tiles.size()) {
		return false;
	}
	for (std::size_t index = 0; index < actual.tiles.size(); ++index) {
		if (actual.tiles[index].walkable != expected.tiles[index].walkable)
			return false;
	}
	return true;
}

bool SameNpcInput(
	const iggy::runtime::RuntimeNpcAiDecisionQueueNpcInput &actual,
	const iggy::runtime::RuntimeNpcAiDecisionQueueNpcInput &expected)
{
	return actual.profile.profileId == expected.profile.profileId
		&& actual.profile.factionId == expected.profile.factionId
		&& actual.profile.aggression == expected.profile.aggression
		&& actual.profile.bravery == expected.profile.bravery
		&& actual.profile.alertness == expected.profile.alertness
		&& actual.profile.preferredRange == expected.profile.preferredRange
		&& actual.profile.behaviorTags == expected.profile.behaviorTags
		&& actual.state.npcId == expected.state.npcId
		&& NearVec(actual.state.position, expected.state.position)
		&& actual.state.currentGoalId == expected.state.currentGoalId
		&& actual.state.enabled == expected.state.enabled;
}

void TestOneNpcWithValidAiPathQueuesOneMoveCommand()
{
	const iggy::AiMap2D aiMap = AiMap({
		Node("ai:patrol", { 3.5F, 0.5F }, 4.0F, 5.0F, 0.0F, 0.0F, 0.0F),
	});
	const iggy::LevelTileMap grid = Grid({ "...." });

	const iggy::runtime::RuntimeNpcAiDecisionQueueResult result =
		iggy::runtime::RuntimeNpcAiDecisionQueueStep {}.push({}, {}, aiMap, grid, { Npc("npc:one") });

	Expect(result.status == iggy::runtime::RuntimeNpcAiDecisionQueueStatus::Queued, "valid NPC AI decision should queue");
	Expect(result.entries.size() == 1, "one NPC should produce one audit entry");
	Expect(result.entries[0].npcIndex == 0, "audit entry should preserve NPC index");
	Expect(result.entries[0].decision.status == iggy::NpcAiDecision2DStatus::Decided, "valid NPC should decide");
	Expect(result.entries[0].route.status == iggy::NpcAiRouteRequest2DStatus::Requested, "valid NPC should request route");
	Expect(result.entries[0].navigation.status == iggy::NpcAiNavigationRequest2DStatus::Built, "valid NPC should build navigation request");
	Expect(result.entries[0].path.status == iggy::NpcAiPathReport2DStatus::PathFound, "valid NPC should find path");
	Expect(result.entries[0].proposal.status == iggy::NpcAiMovementProposal2DStatus::Proposed, "valid NPC should produce movement proposal");
	Expect(result.mapping.frame.commands.size() == 1, "valid NPC should map one command");
	Expect(result.queue.frames.size() == 1, "valid NPC should queue one command frame");
	Expect(result.queue.frames[0].commands.size() == 1, "queued frame should contain one command");
	Expect(result.queue.frames[0].commands[0].actorId == Id("npc:one"), "queued command should preserve NPC id");
	Expect(NearVec(result.queue.frames[0].commands[0].targetPoint, { 1.5F, 0.5F }), "queued command should target first next waypoint");
}

void TestMultipleNpcsQueueCommandsInInputOrder()
{
	const iggy::AiMap2D aiMap = AiMap({
		Node("ai:patrol", { 3.5F, 0.5F }, 5.0F, 5.0F, 0.0F, 0.0F, 0.0F),
	});
	const iggy::LevelTileMap grid = Grid({ "...." });

	const iggy::runtime::RuntimeNpcAiDecisionQueueResult result =
		iggy::runtime::RuntimeNpcAiDecisionQueueStep {}.push(
			{},
			{},
			aiMap,
			grid,
			{
				Npc("npc:first", { 0.5F, 0.5F }),
				Npc("npc:second", { 1.5F, 0.5F }),
			});

	Expect(result.status == iggy::runtime::RuntimeNpcAiDecisionQueueStatus::Queued, "multiple valid NPCs should queue");
	Expect(result.entries.size() == 2, "multiple NPCs should preserve entry count");
	Expect(result.mapping.frame.commands.size() == 2, "multiple valid NPCs should map two commands");
	Expect(result.queue.frames.size() == 1, "multiple valid NPCs should queue one frame");
	Expect(result.queue.frames[0].commands[0].actorId == Id("npc:first"), "first command should preserve first NPC id");
	Expect(NearVec(result.queue.frames[0].commands[0].targetPoint, { 1.5F, 0.5F }), "first command should preserve first NPC next waypoint");
	Expect(result.queue.frames[0].commands[1].actorId == Id("npc:second"), "second command should preserve second NPC id");
	Expect(NearVec(result.queue.frames[0].commands[1].targetPoint, { 2.5F, 0.5F }), "second command should preserve second NPC next waypoint");
}

void TestInvalidNoPathAndHoldNpcsPreserveDiagnosticsAndQueueValidCommands()
{
	const iggy::AiMap2D aiMap = AiMap({
		Node("ai:valid", { 3.5F, 2.5F }, 2.1F, 20.0F, 0.0F, 0.0F, 0.0F),
		Node("ai:blocked", { 3.5F, 0.5F }, 3.1F, 20.0F, 0.0F, 0.0F, 0.0F),
		Node("ai:quiet", { 0.5F, 1.5F }, 1.0F, 1.0F, 0.0F, 0.0F, 0.0F),
	});
	const iggy::LevelTileMap grid = Grid({
		".#..",
		"####",
		"....",
	});
	iggy::runtime::RuntimeNpcAiDecisionQueueNpcInput invalidProfile = Npc("npc:invalid", { 0.5F, 0.5F });
	invalidProfile.profile.profileId = {};
	const iggy::runtime::RuntimeNpcAiDecisionQueueConfig config { { { 10.0F } }, {}, {} };

	const iggy::runtime::RuntimeNpcAiDecisionQueueResult result =
		iggy::runtime::RuntimeNpcAiDecisionQueueStep {}.push(
			{},
			{},
			aiMap,
			grid,
			{
				Npc("npc:valid", { 1.5F, 2.5F }),
				invalidProfile,
				Npc("npc:no-path", { 0.5F, 0.5F }),
				Npc("npc:hold", { 0.5F, 1.5F }),
			},
			config);

	Expect(result.status == iggy::runtime::RuntimeNpcAiDecisionQueueStatus::Queued, "mixed NPC decisions should still queue valid commands");
	Expect(result.entries.size() == 4, "mixed NPC decisions should preserve all entries");
	Expect(result.entries[0].proposal.status == iggy::NpcAiMovementProposal2DStatus::Proposed, "valid NPC should produce movement proposal");
	Expect(result.entries[1].decision.status == iggy::NpcAiDecision2DStatus::InvalidProfile, "invalid profile should preserve decision diagnostic");
	Expect(result.entries[1].proposal.status == iggy::NpcAiMovementProposal2DStatus::NoPath, "invalid profile should produce no movement proposal");
	Expect(result.entries[2].path.status == iggy::NpcAiPathReport2DStatus::PathNotFound, "blocked NPC should preserve path failure diagnostic");
	Expect(result.entries[3].proposal.status == iggy::NpcAiMovementProposal2DStatus::HoldPosition, "hold NPC should preserve hold proposal");
	Expect(result.mapping.frame.commands.size() == 1, "mixed NPC decisions should map only valid movement commands");
	Expect(result.mapping.issues.size() == 3, "mixed NPC decisions should report non-command proposals as mapping issues");
	Expect(result.mapping.issues[0].proposalIndex == 1, "first issue should preserve invalid profile proposal index");
	Expect(result.mapping.issues[1].proposalIndex == 2, "second issue should preserve no-route proposal index");
	Expect(result.mapping.issues[2].proposalIndex == 3, "third issue should preserve hold proposal index");
	Expect(result.queue.frames.size() == 1, "mixed NPC decisions should queue one frame");
	Expect(result.queue.frames[0].commands.size() == 1, "mixed NPC decisions queued frame should contain only valid command");
	Expect(result.queue.frames[0].commands[0].actorId == Id("npc:valid"), "mixed NPC decisions should preserve valid command actor");
}

void TestEmptyNpcListQueuesEmptyFrame()
{
	const iggy::AiMap2D aiMap = AiMap({});
	const iggy::LevelTileMap grid = Grid({ "..." });

	const iggy::runtime::RuntimeNpcAiDecisionQueueResult result =
		iggy::runtime::RuntimeNpcAiDecisionQueueStep {}.push({}, {}, aiMap, grid, {});

	Expect(result.status == iggy::runtime::RuntimeNpcAiDecisionQueueStatus::Queued, "empty NPC list should queue");
	Expect(result.entries.empty(), "empty NPC list should produce no entries");
	Expect(result.mapping.frame.commands.empty(), "empty NPC list should map empty command frame");
	Expect(!result.mapping.hasIssues(), "empty NPC list should produce no mapping issues");
	Expect(result.queue.frames.size() == 1, "empty NPC list should still queue one empty frame");
	Expect(result.queue.frames[0].commands.empty(), "queued empty NPC list frame should have no commands");
}

void TestFullBoundedQueueRejectsAfterDecisionGeneration()
{
	const iggy::AiMap2D aiMap = AiMap({
		Node("ai:patrol", { 2.5F, 0.5F }, 3.0F, 5.0F, 0.0F, 0.0F, 0.0F),
	});
	const iggy::LevelTileMap grid = Grid({ "..." });
	iggy::runtime::RuntimeCommandQueueState queue;
	queue.frames.push_back(MoveFrame("npc:existing", { 9.0F, 9.0F }));
	const iggy::runtime::RuntimeCommandQueueState queueBefore = queue;

	const iggy::runtime::RuntimeNpcAiDecisionQueueResult result =
		iggy::runtime::RuntimeNpcAiDecisionQueueStep {}.push(queue, { 1 }, aiMap, grid, { Npc("npc:new") });

	Expect(result.status == iggy::runtime::RuntimeNpcAiDecisionQueueStatus::RejectedFull, "full queue should reject after AI generation");
	Expect(result.entries.size() == 1, "full queue should still preserve generated NPC entry");
	Expect(result.entries[0].decision.status == iggy::NpcAiDecision2DStatus::Decided, "full queue should still run decision generation");
	Expect(result.mapping.frame.commands.size() == 1, "full queue should still map generated command frame");
	Expect(result.queuePush.status == iggy::runtime::RuntimeNpcAiMovementQueueStatus::RejectedFull, "nested movement queue should preserve rejection");
	Expect(SameQueue(result.queue, queueBefore), "full queue rejection should preserve original queue");
	Expect(SameQueue(result.queuePush.queue, queueBefore), "nested queue result should preserve original queue");
}

void TestPathFailureDiagnosticsArePreserved()
{
	const iggy::AiMap2D aiMap = AiMap({
		Node("ai:patrol", { 4.5F, 1.5F }, 6.0F, 5.0F, 0.0F, 0.0F, 0.0F),
	});
	const iggy::LevelTileMap grid = Grid({
		"..#..",
		"..#..",
		"..#..",
	});

	const iggy::runtime::RuntimeNpcAiDecisionQueueResult result =
		iggy::runtime::RuntimeNpcAiDecisionQueueStep {}.push({}, {}, aiMap, grid, { Npc("npc:blocked", { 0.5F, 1.5F }) });

	Expect(result.status == iggy::runtime::RuntimeNpcAiDecisionQueueStatus::Queued, "path failure should still queue mapped empty frame");
	Expect(result.entries.size() == 1, "path failure should preserve NPC entry");
	Expect(result.entries[0].navigation.status == iggy::NpcAiNavigationRequest2DStatus::Built, "path failure setup should still have valid navigation request");
	Expect(result.entries[0].path.status == iggy::NpcAiPathReport2DStatus::PathNotFound, "path failure should preserve path report status");
	Expect(result.entries[0].path.path.status == iggy::navigation::NavigationPathStatus::NoPath, "path failure should preserve navigation path diagnostic");
	Expect(result.entries[0].proposal.status == iggy::NpcAiMovementProposal2DStatus::NoPath, "path failure should produce no movement proposal");
	Expect(result.mapping.frame.commands.empty(), "path failure should map no commands");
	Expect(result.mapping.issues.size() == 1, "path failure should preserve mapping issue");
	Expect(result.queue.frames.size() == 1, "path failure should still queue one frame");
	Expect(result.queue.frames[0].commands.empty(), "path failure queued frame should be empty");
}

void TestInputsAreNotMutated()
{
	iggy::runtime::RuntimeCommandQueueState queue;
	queue.frames.push_back(MoveFrame("npc:existing", { 6.0F, 6.0F }));
	const iggy::runtime::RuntimeCommandQueueState queueBefore = queue;
	iggy::runtime::RuntimeCommandQueueConfig queueConfig { 3 };
	const iggy::runtime::RuntimeCommandQueueConfig queueConfigBefore = queueConfig;
	iggy::AiMap2D aiMap = AiMap({
		Node("ai:patrol", { 2.5F, 0.5F }, 3.0F, 5.0F, 0.0F, 0.0F, 0.0F),
	});
	const iggy::AiMap2D aiMapBefore = aiMap;
	iggy::LevelTileMap grid = Grid({ "..." });
	const iggy::LevelTileMap gridBefore = grid;
	std::vector<iggy::runtime::RuntimeNpcAiDecisionQueueNpcInput> npcs {
		Npc("npc:immutable"),
	};
	const std::vector<iggy::runtime::RuntimeNpcAiDecisionQueueNpcInput> npcsBefore = npcs;

	const iggy::runtime::RuntimeNpcAiDecisionQueueResult result =
		iggy::runtime::RuntimeNpcAiDecisionQueueStep {}.push(queue, queueConfig, aiMap, grid, npcs);

	Expect(result.status == iggy::runtime::RuntimeNpcAiDecisionQueueStatus::Queued, "immutability setup should queue");
	Expect(SameQueue(queue, queueBefore), "decision queue step should not mutate input queue");
	Expect(queueConfig.maxFrames == queueConfigBefore.maxFrames, "decision queue step should not mutate queue config");
	Expect(SameAiMap(aiMap, aiMapBefore), "decision queue step should not mutate AI map");
	Expect(SameGrid(grid, gridBefore), "decision queue step should not mutate level tile map");
	Expect(npcs.size() == npcsBefore.size(), "decision queue step should not mutate NPC input vector size");
	for (std::size_t index = 0; index < npcs.size(); ++index)
		Expect(SameNpcInput(npcs[index], npcsBefore[index]), "decision queue step should not mutate NPC input");
}

} // namespace

int main()
{
	TestOneNpcWithValidAiPathQueuesOneMoveCommand();
	TestMultipleNpcsQueueCommandsInInputOrder();
	TestInvalidNoPathAndHoldNpcsPreserveDiagnosticsAndQueueValidCommands();
	TestEmptyNpcListQueuesEmptyFrame();
	TestFullBoundedQueueRejectsAfterDecisionGeneration();
	TestPathFailureDiagnosticsArePreserved();
	TestInputsAreNotMutated();

	return Failures;
}
