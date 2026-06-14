#include <cstdlib>
#include <vector>

#include "scene/ai/NpcAiIntentTarget2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::Near;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::AiMapNode2D Node(
	const char *id,
	iggy::Vec2 position,
	float patrolWeight,
	float coverWeight,
	float dangerWeight,
	float interestWeight)
{
	return {
		Id(id),
		position,
		1.0F,
		patrolWeight,
		coverWeight,
		dangerWeight,
		interestWeight,
		{ Id("tag:ai") },
		{},
		true,
	};
}

iggy::NpcAiProfile2D Profile()
{
	return {
		Id("ai-profile:guard"),
		Id("faction:town"),
		0.5F,
		0.25F,
		0.75F,
		4.0F,
		{ Id("tag:ai") },
	};
}

iggy::NpcAiCurrentState2D State()
{
	return {
		Id("npc:guard"),
		{ 10.0F, 20.0F },
		Id("goal:watch"),
		true,
	};
}

iggy::AiMapQuery2DResult Query(std::vector<iggy::AiMapQuery2DEntry> entries)
{
	iggy::AiMapQuery2DResult query;
	query.status = entries.empty() ? iggy::AiMapQuery2DStatus::NoMatch : iggy::AiMapQuery2DStatus::Matched;
	query.position = { 10.0F, 20.0F };
	query.entries = entries;
	query.tags = { Id("tag:ai") };
	return query;
}

iggy::NpcAiContextScore2DResult Score(std::vector<iggy::AiMapQuery2DEntry> entries = {})
{
	iggy::NpcAiContextScore2DResult score;
	score.status = iggy::NpcAiContextScore2DStatus::Scored;
	score.profileValidation = iggy::validate(Profile());
	score.currentStateValidation = iggy::validate(State());
	score.query = Query(entries);
	score.patrolScore = 1.0F;
	score.coverScore = 2.0F;
	score.dangerScore = 3.0F;
	score.interestScore = 4.0F;
	score.matchedBehaviorTags = { Id("tag:ai") };
	return score;
}

iggy::NpcAiBehaviorIntent2DResult Intent(
	iggy::NpcAiBehaviorIntent2DType type,
	std::vector<iggy::AiMapQuery2DEntry> entries = {},
	iggy::NpcAiBehaviorIntent2DStatus status = iggy::NpcAiBehaviorIntent2DStatus::Classified)
{
	iggy::NpcAiBehaviorIntent2DResult intent;
	intent.status = status;
	intent.score = Score(entries);
	intent.type = type;
	intent.selectedScore = 7.0F;
	intent.reason = iggy::NpcAiBehaviorIntent2DReason::PatrolScore;
	return intent;
}

bool SameIntent(const iggy::NpcAiBehaviorIntent2DResult &actual, const iggy::NpcAiBehaviorIntent2DResult &expected)
{
	return actual.status == expected.status
		&& actual.type == expected.type
		&& Near(actual.selectedScore, expected.selectedScore)
		&& actual.reason == expected.reason
		&& actual.score.status == expected.score.status
		&& actual.score.query.entries.size() == expected.score.query.entries.size()
		&& actual.score.matchedBehaviorTags == expected.score.matchedBehaviorTags;
}

void ExpectSelectedNode(
	const iggy::NpcAiIntentTarget2DResult &result,
	const char *nodeId,
	iggy::Vec2 position,
	std::size_t candidateIndex,
	float selectedScore,
	const char *message)
{
	Expect(result.status == iggy::NpcAiIntentTarget2DStatus::Selected, message);
	Expect(result.hasTarget(), message);
	Expect(result.selectedFromNode, message);
	Expect(result.selectedNodeId == Id(nodeId), message);
	Expect(result.selectedNode.id == Id(nodeId), message);
	Expect(NearVec(result.selectedPosition, position), message);
	Expect(result.selectedCandidateIndex == candidateIndex, message);
	Expect(Near(result.selectedScore, selectedScore), message);
}

void TestNoIntentReturnsNoIntent()
{
	const iggy::NpcAiBehaviorIntent2DResult none =
		Intent(iggy::NpcAiBehaviorIntent2DType::None, {}, iggy::NpcAiBehaviorIntent2DStatus::NotScorable);

	const iggy::NpcAiIntentTarget2DResult result =
		iggy::NpcAiIntentTargetSelector2D {}.select(none);

	Expect(result.status == iggy::NpcAiIntentTarget2DStatus::NoIntent, "not scorable intent should return NoIntent");
	Expect(!result.hasTarget(), "not scorable intent should not have a target");
	Expect(!result.selectedFromNode, "not scorable intent should not select from node");
	Expect(SameIntent(result.intent, none), "not scorable target result should preserve intent");
}

void TestNoCandidatesReturnsNoCandidatesForActionableIntent()
{
	const iggy::NpcAiBehaviorIntent2DResult intent = Intent(iggy::NpcAiBehaviorIntent2DType::Patrol);

	const iggy::NpcAiIntentTarget2DResult result =
		iggy::NpcAiIntentTargetSelector2D {}.select(intent);

	Expect(result.status == iggy::NpcAiIntentTarget2DStatus::NoCandidates, "actionable intent with no query entries should return NoCandidates");
	Expect(!result.hasTarget(), "NoCandidates target result should not have a target");
	Expect(SameIntent(result.intent, intent), "NoCandidates target result should preserve intent");
}

void TestPatrolSelectsHighestPatrolNode()
{
	const std::vector<iggy::AiMapQuery2DEntry> entries {
		{ Node("ai:low", { 0.0F, 0.0F }, 1.0F, 0.0F, 0.0F, 0.0F), 0.1F },
		{ Node("ai:high", { 2.0F, 0.0F }, 4.0F, 0.0F, 0.0F, 0.0F), 1.0F },
		{ Node("ai:middle", { 1.0F, 0.0F }, 3.0F, 0.0F, 0.0F, 0.0F), 0.0F },
	};

	const iggy::NpcAiIntentTarget2DResult result =
		iggy::NpcAiIntentTargetSelector2D {}.select(Intent(iggy::NpcAiBehaviorIntent2DType::Patrol, entries));

	ExpectSelectedNode(result, "ai:high", { 2.0F, 0.0F }, 1, 4.0F, "Patrol should select highest patrol node");
}

void TestTakeCoverSelectsHighestCoverNode()
{
	const std::vector<iggy::AiMapQuery2DEntry> entries {
		{ Node("ai:a", { 0.0F, 0.0F }, 0.0F, 2.0F, 0.0F, 0.0F), 0.0F },
		{ Node("ai:b", { 1.0F, 0.0F }, 0.0F, 5.0F, 0.0F, 0.0F), 2.0F },
	};

	const iggy::NpcAiIntentTarget2DResult result =
		iggy::NpcAiIntentTargetSelector2D {}.select(Intent(iggy::NpcAiBehaviorIntent2DType::TakeCover, entries));

	ExpectSelectedNode(result, "ai:b", { 1.0F, 0.0F }, 1, 5.0F, "TakeCover should select highest cover node");
}

void TestAvoidDangerSelectsLowestDangerNode()
{
	const std::vector<iggy::AiMapQuery2DEntry> entries {
		{ Node("ai:danger", { 0.0F, 0.0F }, 0.0F, 0.0F, 8.0F, 0.0F), 0.0F },
		{ Node("ai:safe", { 1.0F, 0.0F }, 0.0F, 0.0F, 1.0F, 0.0F), 5.0F },
		{ Node("ai:medium", { 2.0F, 0.0F }, 0.0F, 0.0F, 3.0F, 0.0F), 1.0F },
	};

	const iggy::NpcAiIntentTarget2DResult result =
		iggy::NpcAiIntentTargetSelector2D {}.select(Intent(iggy::NpcAiBehaviorIntent2DType::AvoidDanger, entries));

	ExpectSelectedNode(result, "ai:safe", { 1.0F, 0.0F }, 1, 1.0F, "AvoidDanger should select lowest danger node");
}

void TestInvestigateSelectsHighestInterestNode()
{
	const std::vector<iggy::AiMapQuery2DEntry> entries {
		{ Node("ai:a", { 0.0F, 0.0F }, 0.0F, 0.0F, 0.0F, 2.0F), 0.0F },
		{ Node("ai:b", { 1.0F, 0.0F }, 0.0F, 0.0F, 0.0F, 9.0F), 2.0F },
	};

	const iggy::NpcAiIntentTarget2DResult result =
		iggy::NpcAiIntentTargetSelector2D {}.select(Intent(iggy::NpcAiBehaviorIntent2DType::Investigate, entries));

	ExpectSelectedNode(result, "ai:b", { 1.0F, 0.0F }, 1, 9.0F, "Investigate should select highest interest node");
}

void TestTiesChooseLowerDistanceThenEarlierEntry()
{
	const std::vector<iggy::AiMapQuery2DEntry> lowerDistance {
		{ Node("ai:far", { 0.0F, 0.0F }, 5.0F, 0.0F, 0.0F, 0.0F), 4.0F },
		{ Node("ai:near", { 1.0F, 0.0F }, 5.0F, 0.0F, 0.0F, 0.0F), 1.0F },
		{ Node("ai:near-later", { 2.0F, 0.0F }, 5.0F, 0.0F, 0.0F, 0.0F), 1.0F },
	};
	const std::vector<iggy::AiMapQuery2DEntry> earlierOrder {
		{ Node("ai:first", { 3.0F, 0.0F }, 5.0F, 0.0F, 0.0F, 0.0F), 1.0F },
		{ Node("ai:second", { 4.0F, 0.0F }, 5.0F, 0.0F, 0.0F, 0.0F), 1.0F },
	};

	const iggy::NpcAiIntentTarget2DResult distanceResult =
		iggy::NpcAiIntentTargetSelector2D {}.select(Intent(iggy::NpcAiBehaviorIntent2DType::Patrol, lowerDistance));
	const iggy::NpcAiIntentTarget2DResult orderResult =
		iggy::NpcAiIntentTargetSelector2D {}.select(Intent(iggy::NpcAiBehaviorIntent2DType::Patrol, earlierOrder));

	ExpectSelectedNode(distanceResult, "ai:near", { 1.0F, 0.0F }, 1, 5.0F, "score ties should choose lower distance first");
	ExpectSelectedNode(orderResult, "ai:first", { 3.0F, 0.0F }, 0, 5.0F, "score and distance ties should preserve earlier query entry");
}

void TestAvoidDangerTiesUseLowerDistanceThenEarlierEntry()
{
	const std::vector<iggy::AiMapQuery2DEntry> entries {
		{ Node("ai:far-safe", { 0.0F, 0.0F }, 0.0F, 0.0F, 1.0F, 0.0F), 4.0F },
		{ Node("ai:near-safe", { 1.0F, 0.0F }, 0.0F, 0.0F, 1.0F, 0.0F), 1.0F },
		{ Node("ai:near-safe-later", { 2.0F, 0.0F }, 0.0F, 0.0F, 1.0F, 0.0F), 1.0F },
	};

	const iggy::NpcAiIntentTarget2DResult result =
		iggy::NpcAiIntentTargetSelector2D {}.select(Intent(iggy::NpcAiBehaviorIntent2DType::AvoidDanger, entries));

	ExpectSelectedNode(result, "ai:near-safe", { 1.0F, 0.0F }, 1, 1.0F, "AvoidDanger danger ties should choose lower distance first");
}

void TestHoldPositionSelectsCurrentStatePosition()
{
	const iggy::NpcAiBehaviorIntent2DResult intent = Intent(iggy::NpcAiBehaviorIntent2DType::HoldPosition);

	const iggy::NpcAiIntentTarget2DResult result =
		iggy::NpcAiIntentTargetSelector2D {}.select(intent);

	Expect(result.status == iggy::NpcAiIntentTarget2DStatus::Selected, "HoldPosition should select current NPC position");
	Expect(result.hasTarget(), "HoldPosition should have a target");
	Expect(!result.selectedFromNode, "HoldPosition should not select from a map node");
	Expect(result.selectedNodeId.empty(), "HoldPosition should not set selected node id");
	Expect(NearVec(result.selectedPosition, { 10.0F, 20.0F }), "HoldPosition should preserve current state position");
	Expect(result.selectedCandidateIndex == 0, "HoldPosition should keep default candidate index");
	Expect(Near(result.selectedScore, intent.selectedScore), "HoldPosition should preserve intent selected score");
}

void TestCopiedIntentIsPreservedForSelectedTarget()
{
	const std::vector<iggy::AiMapQuery2DEntry> entries {
		{ Node("ai:target", { 1.0F, 0.0F }, 5.0F, 0.0F, 0.0F, 0.0F), 1.0F },
	};
	const iggy::NpcAiBehaviorIntent2DResult intent = Intent(iggy::NpcAiBehaviorIntent2DType::Patrol, entries);

	const iggy::NpcAiIntentTarget2DResult result =
		iggy::NpcAiIntentTargetSelector2D {}.select(intent);

	Expect(result.status == iggy::NpcAiIntentTarget2DStatus::Selected, "copied intent setup should select target");
	Expect(SameIntent(result.intent, intent), "target selection should preserve copied intent");
}

void TestInputIntentIsNotMutated()
{
	const std::vector<iggy::AiMapQuery2DEntry> entries {
		{ Node("ai:target", { 1.0F, 0.0F }, 5.0F, 0.0F, 0.0F, 0.0F), 1.0F },
	};
	iggy::NpcAiBehaviorIntent2DResult intent = Intent(iggy::NpcAiBehaviorIntent2DType::Patrol, entries);
	const iggy::NpcAiBehaviorIntent2DResult before = intent;

	const iggy::NpcAiIntentTarget2DResult result =
		iggy::NpcAiIntentTargetSelector2D {}.select(intent);

	Expect(result.status == iggy::NpcAiIntentTarget2DStatus::Selected, "intent target immutability setup should select target");
	Expect(SameIntent(intent, before), "intent target selection should not mutate input intent");
}

} // namespace

int main()
{
	TestNoIntentReturnsNoIntent();
	TestNoCandidatesReturnsNoCandidatesForActionableIntent();
	TestPatrolSelectsHighestPatrolNode();
	TestTakeCoverSelectsHighestCoverNode();
	TestAvoidDangerSelectsLowestDangerNode();
	TestInvestigateSelectsHighestInterestNode();
	TestTiesChooseLowerDistanceThenEarlierEntry();
	TestAvoidDangerTiesUseLowerDistanceThenEarlierEntry();
	TestHoldPositionSelectsCurrentStatePosition();
	TestCopiedIntentIsPreservedForSelectedTarget();
	TestInputIntentIsNotMutated();

	return Failures;
}
