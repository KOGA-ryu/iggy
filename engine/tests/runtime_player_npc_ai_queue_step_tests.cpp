#include <cstdlib>
#include <string_view>
#include <vector>

#include "runtime/RuntimePlayerNpcAiQueueStep.hpp"
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
	float patrolWeight)
{
	return {
		Id(id),
		position,
		radius,
		patrolWeight,
		0.0F,
		0.0F,
		0.0F,
		{},
		{},
		true,
	};
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

iggy::NpcAiProfile2D Profile(const char *npcId)
{
	return {
		Id(npcId),
		Id("faction:queue-order"),
		0.0F,
		0.0F,
		0.0F,
		4.0F,
		{},
	};
}

iggy::NpcAiCurrentState2D State(const char *npcId, iggy::Vec2 position = { 0.5F, 0.5F })
{
	return {
		Id(npcId),
		position,
		Id("goal:queue-order"),
		true,
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

iggy::runtime::GameplayCommandFrame2D ExistingFrame()
{
	return { { iggy::runtime::GameplayCommand2DFactory {}.wait(Id("actor:existing")) } };
}

iggy::runtime::RuntimePlayerNpcAiQueueInput BaseInput(
	iggy::runtime::RuntimePlayerNpcAiQueueOrder order = iggy::runtime::RuntimePlayerNpcAiQueueOrder::PlayerThenNpcAi)
{
	iggy::runtime::RuntimePlayerNpcAiQueueInput input;
	input.playerActorId = Id("player:one");
	input.playerInputContext = {};
	input.playerIntents = { iggy::playerMoveToPointIntent({ 9.0F, 9.0F }) };
	input.aiMap = { { Node("ai:patrol", { 3.5F, 0.5F }, 4.0F, 5.0F) } };
	input.levelMap = Grid({ "...." });
	input.npcs = { Npc("npc:one") };
	input.order = order;
	return input;
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

bool SameIntent(const iggy::PlayerInputIntent2D &actual, const iggy::PlayerInputIntent2D &expected)
{
	return actual.type == expected.type
		&& NearVec(actual.worldPoint, expected.worldPoint)
		&& actual.tile == expected.tile
		&& actual.targetId == expected.targetId;
}

bool SameNode(const iggy::AiMapNode2D &actual, const iggy::AiMapNode2D &expected)
{
	return actual.id == expected.id
		&& NearVec(actual.position, expected.position)
		&& actual.radius == expected.radius
		&& actual.patrolWeight == expected.patrolWeight
		&& actual.coverWeight == expected.coverWeight
		&& actual.dangerWeight == expected.dangerWeight
		&& actual.interestWeight == expected.interestWeight
		&& actual.tags == expected.tags
		&& actual.links == expected.links
		&& actual.enabled == expected.enabled;
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

bool SameInput(
	const iggy::runtime::RuntimePlayerNpcAiQueueInput &actual,
	const iggy::runtime::RuntimePlayerNpcAiQueueInput &expected)
{
	if (!(SameQueue(actual.queue, expected.queue)
		&& actual.queueConfig.maxFrames == expected.queueConfig.maxFrames
		&& actual.playerActorId == expected.playerActorId
		&& actual.playerInputContext.playerControlEnabled == expected.playerInputContext.playerControlEnabled
		&& actual.playerInputContext.worldInputEnabled == expected.playerInputContext.worldInputEnabled
		&& actual.playerInputContext.interactionEnabled == expected.playerInputContext.interactionEnabled
		&& actual.playerInputContext.cancelEnabled == expected.playerInputContext.cancelEnabled
		&& actual.playerIntents.size() == expected.playerIntents.size()
		&& actual.aiMap.nodes.size() == expected.aiMap.nodes.size()
		&& SameGrid(actual.levelMap, expected.levelMap)
		&& actual.npcs.size() == expected.npcs.size()
		&& actual.npcAi.decision.intent.minimumScore == expected.npcAi.decision.intent.minimumScore
		&& actual.npcAi.route.arrivalTolerance == expected.npcAi.route.arrivalTolerance
		&& actual.npcAi.proposal.waypointLookahead == expected.npcAi.proposal.waypointLookahead
		&& actual.npcAi.proposal.arrivalTolerance == expected.npcAi.proposal.arrivalTolerance
		&& actual.order == expected.order)) {
		return false;
	}
	for (std::size_t index = 0; index < actual.playerIntents.size(); ++index) {
		if (!SameIntent(actual.playerIntents[index], expected.playerIntents[index]))
			return false;
	}
	for (std::size_t index = 0; index < actual.aiMap.nodes.size(); ++index) {
		if (!SameNode(actual.aiMap.nodes[index], expected.aiMap.nodes[index]))
			return false;
	}
	for (std::size_t index = 0; index < actual.npcs.size(); ++index) {
		if (!SameNpcInput(actual.npcs[index], expected.npcs[index]))
			return false;
	}
	return true;
}

void ExpectPlayerFrame(const iggy::runtime::GameplayCommandFrame2D &frame, const char *message)
{
	Expect(frame.commands.size() == 1, message);
	if (frame.commands.empty())
		return;
	Expect(frame.commands[0].actorId == Id("player:one"), message);
	Expect(frame.commands[0].type == iggy::runtime::GameplayCommand2DType::MoveToPoint, message);
	Expect(NearVec(frame.commands[0].targetPoint, { 9.0F, 9.0F }), message);
}

void ExpectNpcFrame(const iggy::runtime::GameplayCommandFrame2D &frame, const char *message)
{
	Expect(frame.commands.size() == 1, message);
	if (frame.commands.empty())
		return;
	Expect(frame.commands[0].actorId == Id("npc:one"), message);
	Expect(frame.commands[0].type == iggy::runtime::GameplayCommand2DType::MoveToPoint, message);
	Expect(NearVec(frame.commands[0].targetPoint, { 1.5F, 0.5F }), message);
}

void TestPlayerThenNpcAiQueuesPlayerFrameBeforeNpcFrame()
{
	const iggy::runtime::RuntimePlayerNpcAiQueueInput input =
		BaseInput(iggy::runtime::RuntimePlayerNpcAiQueueOrder::PlayerThenNpcAi);

	const iggy::runtime::RuntimePlayerNpcAiQueueResult result =
		iggy::runtime::RuntimePlayerNpcAiQueueStep {}.push(input);

	Expect(result.status == iggy::runtime::RuntimePlayerNpcAiQueueStatus::Queued, "player-then-NPC should queue");
	Expect(result.order == iggy::runtime::RuntimePlayerNpcAiQueueOrder::PlayerThenNpcAi, "result should preserve player-first order");
	Expect(result.player.status == iggy::runtime::RuntimePlayerInputQueueStatus::Queued, "player intake should queue");
	Expect(result.npcAi.status == iggy::runtime::RuntimeNpcAiDecisionQueueStatus::Queued, "NPC AI intake should queue");
	Expect(result.queue.frames.size() == 2, "player-then-NPC should append two frames");
	ExpectPlayerFrame(result.queue.frames[0], "player frame should be first");
	ExpectNpcFrame(result.queue.frames[1], "NPC frame should be second");
}

void TestNpcAiThenPlayerQueuesNpcFrameBeforePlayerFrame()
{
	const iggy::runtime::RuntimePlayerNpcAiQueueInput input =
		BaseInput(iggy::runtime::RuntimePlayerNpcAiQueueOrder::NpcAiThenPlayer);

	const iggy::runtime::RuntimePlayerNpcAiQueueResult result =
		iggy::runtime::RuntimePlayerNpcAiQueueStep {}.push(input);

	Expect(result.status == iggy::runtime::RuntimePlayerNpcAiQueueStatus::Queued, "NPC-then-player should queue");
	Expect(result.order == iggy::runtime::RuntimePlayerNpcAiQueueOrder::NpcAiThenPlayer, "result should preserve NPC-first order");
	Expect(result.queue.frames.size() == 2, "NPC-then-player should append two frames");
	ExpectNpcFrame(result.queue.frames[0], "NPC frame should be first");
	ExpectPlayerFrame(result.queue.frames[1], "player frame should be second");
}

void TestFirstPlayerRejectionStopsNpcAiStep()
{
	iggy::runtime::RuntimePlayerNpcAiQueueInput input =
		BaseInput(iggy::runtime::RuntimePlayerNpcAiQueueOrder::PlayerThenNpcAi);
	input.queue.frames.push_back(ExistingFrame());
	input.queueConfig.maxFrames = 1;
	const iggy::runtime::RuntimeCommandQueueState queueBefore = input.queue;

	const iggy::runtime::RuntimePlayerNpcAiQueueResult result =
		iggy::runtime::RuntimePlayerNpcAiQueueStep {}.push(input);

	Expect(result.status == iggy::runtime::RuntimePlayerNpcAiQueueStatus::PlayerRejected, "first player rejection should be reported");
	Expect(result.player.status == iggy::runtime::RuntimePlayerInputQueueStatus::RejectedFull, "player nested result should preserve rejection");
	Expect(result.npcAi.entries.empty(), "NPC AI step should not run after first player rejection");
	Expect(result.npcAi.mapping.frame.commands.empty(), "NPC AI mapping should remain empty when not run");
	Expect(SameQueue(result.queue, queueBefore), "first rejection should preserve original queue");
}

void TestFirstNpcAiRejectionStopsPlayerStep()
{
	iggy::runtime::RuntimePlayerNpcAiQueueInput input =
		BaseInput(iggy::runtime::RuntimePlayerNpcAiQueueOrder::NpcAiThenPlayer);
	input.queue.frames.push_back(ExistingFrame());
	input.queueConfig.maxFrames = 1;
	const iggy::runtime::RuntimeCommandQueueState queueBefore = input.queue;

	const iggy::runtime::RuntimePlayerNpcAiQueueResult result =
		iggy::runtime::RuntimePlayerNpcAiQueueStep {}.push(input);

	Expect(result.status == iggy::runtime::RuntimePlayerNpcAiQueueStatus::NpcAiRejected, "first NPC AI rejection should be reported");
	Expect(result.npcAi.status == iggy::runtime::RuntimeNpcAiDecisionQueueStatus::RejectedFull, "NPC AI nested result should preserve rejection");
	Expect(result.npcAi.entries.size() == 1, "NPC AI decision generation should run before its queue rejection");
	Expect(result.player.mapping.frame.commands.empty(), "player step should not run after first NPC AI rejection");
	Expect(SameQueue(result.queue, queueBefore), "first NPC rejection should preserve original queue");
}

void TestSecondNpcAiRejectionPreservesFirstPlayerFrame()
{
	iggy::runtime::RuntimePlayerNpcAiQueueInput input =
		BaseInput(iggy::runtime::RuntimePlayerNpcAiQueueOrder::PlayerThenNpcAi);
	input.queueConfig.maxFrames = 1;

	const iggy::runtime::RuntimePlayerNpcAiQueueResult result =
		iggy::runtime::RuntimePlayerNpcAiQueueStep {}.push(input);

	Expect(result.status == iggy::runtime::RuntimePlayerNpcAiQueueStatus::NpcAiRejected, "second NPC AI rejection should be reported");
	Expect(result.player.status == iggy::runtime::RuntimePlayerInputQueueStatus::Queued, "first player step should queue");
	Expect(result.npcAi.status == iggy::runtime::RuntimeNpcAiDecisionQueueStatus::RejectedFull, "second NPC AI step should reject full queue");
	Expect(result.queue.frames.size() == 1, "second rejection should preserve first queued frame");
	ExpectPlayerFrame(result.queue.frames[0], "second NPC rejection should leave player frame queued");
}

void TestSecondPlayerRejectionPreservesFirstNpcAiFrame()
{
	iggy::runtime::RuntimePlayerNpcAiQueueInput input =
		BaseInput(iggy::runtime::RuntimePlayerNpcAiQueueOrder::NpcAiThenPlayer);
	input.queueConfig.maxFrames = 1;

	const iggy::runtime::RuntimePlayerNpcAiQueueResult result =
		iggy::runtime::RuntimePlayerNpcAiQueueStep {}.push(input);

	Expect(result.status == iggy::runtime::RuntimePlayerNpcAiQueueStatus::PlayerRejected, "second player rejection should be reported");
	Expect(result.npcAi.status == iggy::runtime::RuntimeNpcAiDecisionQueueStatus::Queued, "first NPC AI step should queue");
	Expect(result.player.status == iggy::runtime::RuntimePlayerInputQueueStatus::RejectedFull, "second player step should reject full queue");
	Expect(result.queue.frames.size() == 1, "second rejection should preserve first queued NPC frame");
	ExpectNpcFrame(result.queue.frames[0], "second player rejection should leave NPC frame queued");
}

void TestEmptyPlayerAndNpcInputsStillQueueEmptyFrames()
{
	iggy::runtime::RuntimePlayerNpcAiQueueInput input = BaseInput();
	input.playerIntents.clear();
	input.npcs.clear();

	const iggy::runtime::RuntimePlayerNpcAiQueueResult result =
		iggy::runtime::RuntimePlayerNpcAiQueueStep {}.push(input);

	Expect(result.status == iggy::runtime::RuntimePlayerNpcAiQueueStatus::Queued, "empty player and NPC inputs should queue");
	Expect(result.queue.frames.size() == 2, "empty player and NPC inputs should append two empty frames");
	Expect(result.queue.frames[0].commands.empty(), "empty player frame should have no commands");
	Expect(result.queue.frames[1].commands.empty(), "empty NPC AI frame should have no commands");
}

void TestInputsAreNotMutated()
{
	iggy::runtime::RuntimePlayerNpcAiQueueInput input =
		BaseInput(iggy::runtime::RuntimePlayerNpcAiQueueOrder::NpcAiThenPlayer);
	input.queue.frames.push_back(ExistingFrame());
	input.queueConfig.maxFrames = 4;
	input.playerInputContext.cancelEnabled = false;
	const iggy::runtime::RuntimePlayerNpcAiQueueInput inputBefore = input;

	const iggy::runtime::RuntimePlayerNpcAiQueueResult result =
		iggy::runtime::RuntimePlayerNpcAiQueueStep {}.push(input);

	Expect(result.status == iggy::runtime::RuntimePlayerNpcAiQueueStatus::Queued, "immutability setup should queue");
	Expect(SameInput(input, inputBefore), "player/NPC AI queue step should not mutate input packet");
}

} // namespace

int main()
{
	TestPlayerThenNpcAiQueuesPlayerFrameBeforeNpcFrame();
	TestNpcAiThenPlayerQueuesNpcFrameBeforePlayerFrame();
	TestFirstPlayerRejectionStopsNpcAiStep();
	TestFirstNpcAiRejectionStopsPlayerStep();
	TestSecondNpcAiRejectionPreservesFirstPlayerFrame();
	TestSecondPlayerRejectionPreservesFirstNpcAiFrame();
	TestEmptyPlayerAndNpcInputsStillQueueEmptyFrames();
	TestInputsAreNotMutated();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
