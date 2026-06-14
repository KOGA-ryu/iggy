#include <cstdlib>
#include <vector>

#include "scene/ai/NpcAiDecision2D.hpp"
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
		{ Id("tag:ai") },
		{},
		enabled,
	};
}

iggy::AiMap2D Map(std::vector<iggy::AiMapNode2D> nodes)
{
	return { nodes };
}

iggy::NpcAiProfile2D Profile(
	const char *profileId = "ai-profile:guard",
	float aggression = 0.5F,
	float bravery = 0.5F,
	float alertness = 1.0F)
{
	return {
		Id(profileId),
		Id("faction:town"),
		aggression,
		bravery,
		alertness,
		4.0F,
		{ Id("tag:ai") },
	};
}

iggy::NpcAiCurrentState2D State(
	const char *npcId = "npc:guard",
	iggy::Vec2 position = { 0.0F, 0.0F },
	bool enabled = true)
{
	return {
		Id(npcId),
		position,
		Id("goal:watch"),
		enabled,
	};
}

bool SameNodes(const std::vector<iggy::AiMapNode2D> &actual, const std::vector<iggy::AiMapNode2D> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (actual[index].id != expected[index].id
			|| !NearVec(actual[index].position, expected[index].position)
			|| actual[index].radius != expected[index].radius
			|| actual[index].patrolWeight != expected[index].patrolWeight
			|| actual[index].coverWeight != expected[index].coverWeight
			|| actual[index].dangerWeight != expected[index].dangerWeight
			|| actual[index].interestWeight != expected[index].interestWeight
			|| actual[index].tags != expected[index].tags
			|| actual[index].links != expected[index].links
			|| actual[index].enabled != expected[index].enabled) {
			return false;
		}
	}
	return true;
}

bool SameProfile(const iggy::NpcAiProfile2D &actual, const iggy::NpcAiProfile2D &expected)
{
	return actual.profileId == expected.profileId
		&& actual.factionId == expected.factionId
		&& actual.aggression == expected.aggression
		&& actual.bravery == expected.bravery
		&& actual.alertness == expected.alertness
		&& actual.preferredRange == expected.preferredRange
		&& actual.behaviorTags == expected.behaviorTags;
}

bool SameState(const iggy::NpcAiCurrentState2D &actual, const iggy::NpcAiCurrentState2D &expected)
{
	return actual.npcId == expected.npcId
		&& NearVec(actual.position, expected.position)
		&& actual.currentGoalId == expected.currentGoalId
		&& actual.enabled == expected.enabled;
}

void ExpectDecidedNode(
	const iggy::NpcAiDecision2DResult &result,
	iggy::NpcAiBehaviorIntent2DType intentType,
	const char *nodeId,
	const char *message)
{
	Expect(result.status == iggy::NpcAiDecision2DStatus::Decided, message);
	Expect(result.hasDecision(), message);
	Expect(result.query.status == iggy::AiMapQuery2DStatus::Matched, message);
	Expect(result.score.status == iggy::NpcAiContextScore2DStatus::Scored, message);
	Expect(result.intent.status == iggy::NpcAiBehaviorIntent2DStatus::Classified, message);
	Expect(result.intent.type == intentType, message);
	Expect(result.target.status == iggy::NpcAiIntentTarget2DStatus::Selected, message);
	Expect(result.target.selectedFromNode, message);
	Expect(result.target.selectedNodeId == Id(nodeId), message);
}

void TestPatrolDecision()
{
	const iggy::AiMap2D map = Map({
		Node("ai:patrol-low", { 0.0F, 0.0F }, 3.0F, 2.0F, 0.0F, 0.0F, 0.0F),
		Node("ai:patrol-high", { 1.0F, 0.0F }, 3.0F, 5.0F, 0.0F, 0.0F, 0.0F),
	});

	const iggy::NpcAiDecision2DResult result =
		iggy::NpcAiDecisionMaker2D {}.decide(map, Profile("ai-profile:patrol", 0.0F, 0.0F, 0.0F), State());

	ExpectDecidedNode(result, iggy::NpcAiBehaviorIntent2DType::Patrol, "ai:patrol-high", "patrol decision should choose patrol intent and target");
}

void TestCoverDecision()
{
	const iggy::AiMap2D map = Map({
		Node("ai:cover-low", { 0.0F, 0.0F }, 3.0F, 0.0F, 1.0F, 0.0F, 0.0F),
		Node("ai:cover-high", { 1.0F, 0.0F }, 3.0F, 0.0F, 5.0F, 0.0F, 0.0F),
	});

	const iggy::NpcAiDecision2DResult result =
		iggy::NpcAiDecisionMaker2D {}.decide(map, Profile("ai-profile:cover", 0.0F, 0.0F, 0.0F), State());

	ExpectDecidedNode(result, iggy::NpcAiBehaviorIntent2DType::TakeCover, "ai:cover-high", "cover decision should choose cover intent and target");
}

void TestDangerDecision()
{
	const iggy::AiMap2D map = Map({
		Node("ai:danger-high", { 0.0F, 0.0F }, 3.0F, 0.0F, 0.0F, 9.0F, 0.0F),
		Node("ai:danger-low", { 1.0F, 0.0F }, 3.0F, 0.0F, 0.0F, 1.0F, 0.0F),
	});

	const iggy::NpcAiDecision2DResult result =
		iggy::NpcAiDecisionMaker2D {}.decide(map, Profile("ai-profile:danger", 0.0F, 0.0F, 1.0F), State());

	ExpectDecidedNode(result, iggy::NpcAiBehaviorIntent2DType::AvoidDanger, "ai:danger-low", "danger decision should choose avoid danger intent and safest local target");
}

void TestInterestDecision()
{
	const iggy::AiMap2D map = Map({
		Node("ai:interest-low", { 0.0F, 0.0F }, 3.0F, 0.0F, 0.0F, 0.0F, 2.0F),
		Node("ai:interest-high", { 1.0F, 0.0F }, 3.0F, 0.0F, 0.0F, 0.0F, 7.0F),
	});

	const iggy::NpcAiDecision2DResult result =
		iggy::NpcAiDecisionMaker2D {}.decide(map, Profile("ai-profile:interest", 0.0F, 0.0F, 1.0F), State());

	ExpectDecidedNode(result, iggy::NpcAiBehaviorIntent2DType::Investigate, "ai:interest-high", "interest decision should choose investigate intent and target");
}

void TestHoldPositionDecisionFromThreshold()
{
	const iggy::AiMap2D map = Map({
		Node("ai:quiet", { 2.0F, 3.0F }, 2.0F, 1.0F, 0.0F, 0.0F, 0.0F),
	});
	const iggy::NpcAiCurrentState2D state = State("npc:hold", { 2.0F, 3.0F });
	const iggy::NpcAiDecision2DConfig config { { 10.0F } };

	const iggy::NpcAiDecision2DResult result =
		iggy::NpcAiDecisionMaker2D {}.decide(map, Profile("ai-profile:hold", 0.0F, 0.0F, 0.0F), state, config);

	Expect(result.status == iggy::NpcAiDecision2DStatus::Decided, "below-threshold scorable context should still decide HoldPosition");
	Expect(result.hasDecision(), "HoldPosition decision should report decision");
	Expect(result.intent.type == iggy::NpcAiBehaviorIntent2DType::HoldPosition, "below-threshold decision should classify HoldPosition");
	Expect(result.intent.reason == iggy::NpcAiBehaviorIntent2DReason::BelowThreshold, "HoldPosition decision should preserve below-threshold reason");
	Expect(result.target.status == iggy::NpcAiIntentTarget2DStatus::Selected, "HoldPosition decision should select target");
	Expect(!result.target.selectedFromNode, "HoldPosition decision should not select a node target");
	Expect(NearVec(result.target.selectedPosition, state.position), "HoldPosition decision should target current NPC position");
}

void TestInvalidProfileMapsStatusAndPreservesDiagnostics()
{
	const iggy::AiMap2D map = Map({
		Node("ai:node", { 0.0F, 0.0F }, 2.0F, 1.0F, 0.0F, 0.0F, 0.0F),
	});

	const iggy::NpcAiDecision2DResult result =
		iggy::NpcAiDecisionMaker2D {}.decide(map, Profile("", -1.0F), State());

	Expect(result.status == iggy::NpcAiDecision2DStatus::InvalidProfile, "invalid profile should map to InvalidProfile decision status");
	Expect(!result.hasDecision(), "invalid profile should not have decision");
	Expect(!result.score.profileValidation.valid, "invalid profile decision should preserve profile diagnostics");
	Expect(result.intent.status == iggy::NpcAiBehaviorIntent2DStatus::NotScorable, "invalid profile decision should preserve non-scorable intent");
	Expect(result.target.status == iggy::NpcAiIntentTarget2DStatus::NoIntent, "invalid profile decision should preserve no target");
}

void TestInvalidCurrentStateMapsStatusAndPreservesDiagnostics()
{
	const iggy::AiMap2D map = Map({
		Node("ai:node", { 0.0F, 0.0F }, 2.0F, 1.0F, 0.0F, 0.0F, 0.0F),
	});

	const iggy::NpcAiDecision2DResult result =
		iggy::NpcAiDecisionMaker2D {}.decide(map, Profile(), State(""));

	Expect(result.status == iggy::NpcAiDecision2DStatus::InvalidCurrentState, "invalid state should map to InvalidCurrentState decision status");
	Expect(!result.hasDecision(), "invalid state should not have decision");
	Expect(!result.score.currentStateValidation.valid, "invalid state decision should preserve state diagnostics");
	Expect(result.target.status == iggy::NpcAiIntentTarget2DStatus::NoIntent, "invalid state decision should preserve no target");
}

void TestDisabledNpcMapsStatus()
{
	const iggy::AiMap2D map = Map({
		Node("ai:node", { 0.0F, 0.0F }, 2.0F, 1.0F, 0.0F, 0.0F, 0.0F),
	});

	const iggy::NpcAiDecision2DResult result =
		iggy::NpcAiDecisionMaker2D {}.decide(map, Profile(), State("npc:disabled", { 0.0F, 0.0F }, false));

	Expect(result.status == iggy::NpcAiDecision2DStatus::DisabledNpc, "disabled NPC should map to DisabledNpc decision status");
	Expect(!result.hasDecision(), "disabled NPC should not have decision");
	Expect(result.score.status == iggy::NpcAiContextScore2DStatus::DisabledNpc, "disabled NPC decision should preserve score status");
}

void TestNoMapContextMapsStatus()
{
	const iggy::AiMap2D map = Map({
		Node("ai:far", { 100.0F, 100.0F }, 1.0F, 1.0F, 0.0F, 0.0F, 0.0F),
	});

	const iggy::NpcAiDecision2DResult result =
		iggy::NpcAiDecisionMaker2D {}.decide(map, Profile(), State());

	Expect(result.status == iggy::NpcAiDecision2DStatus::NoMapContext, "no local map context should map to NoMapContext decision status");
	Expect(!result.hasDecision(), "no local map context should not have decision");
	Expect(result.query.status == iggy::AiMapQuery2DStatus::NoMatch, "no local map context should preserve query NoMatch");
	Expect(result.score.status == iggy::NpcAiContextScore2DStatus::NoMapContext, "no local map context should preserve score status");
	Expect(result.target.status == iggy::NpcAiIntentTarget2DStatus::NoIntent, "no local map context should preserve no target");
}

void TestConfigThresholdAffectsDecisionDeterministically()
{
	const iggy::AiMap2D map = Map({
		Node("ai:patrol", { 0.0F, 0.0F }, 2.0F, 3.0F, 0.0F, 0.0F, 0.0F),
	});

	const iggy::NpcAiDecision2DResult defaultResult =
		iggy::NpcAiDecisionMaker2D {}.decide(map, Profile("ai-profile:default", 0.0F, 0.0F, 0.0F), State());
	const iggy::NpcAiDecision2DResult thresholdResult =
		iggy::NpcAiDecisionMaker2D {}.decide(map, Profile("ai-profile:threshold", 0.0F, 0.0F, 0.0F), State(), { { 100.0F } });

	Expect(defaultResult.status == iggy::NpcAiDecision2DStatus::Decided, "default threshold should decide from patrol score");
	Expect(defaultResult.intent.type == iggy::NpcAiBehaviorIntent2DType::Patrol, "default threshold should preserve patrol intent");
	Expect(thresholdResult.status == iggy::NpcAiDecision2DStatus::Decided, "high threshold should still decide HoldPosition");
	Expect(thresholdResult.intent.type == iggy::NpcAiBehaviorIntent2DType::HoldPosition, "high threshold should classify HoldPosition");
	Expect(!thresholdResult.target.selectedFromNode, "high-threshold HoldPosition should not select map node");
}

void TestInputsAreNotMutated()
{
	iggy::AiMap2D map = Map({
		Node("ai:node", { 0.0F, 0.0F }, 2.0F, 1.0F, 0.0F, 0.0F, 0.0F),
	});
	iggy::NpcAiProfile2D profile = Profile("ai-profile:immutable");
	iggy::NpcAiCurrentState2D state = State("npc:immutable");
	const std::vector<iggy::AiMapNode2D> nodesBefore = map.nodes;
	const iggy::NpcAiProfile2D profileBefore = profile;
	const iggy::NpcAiCurrentState2D stateBefore = state;

	const iggy::NpcAiDecision2DResult result =
		iggy::NpcAiDecisionMaker2D {}.decide(map, profile, state);

	Expect(result.hasDecision(), "decision immutability setup should produce a decision");
	Expect(SameNodes(map.nodes, nodesBefore), "decision maker should not mutate AI map");
	Expect(SameProfile(profile, profileBefore), "decision maker should not mutate profile");
	Expect(SameState(state, stateBefore), "decision maker should not mutate current state");
}

} // namespace

int main()
{
	TestPatrolDecision();
	TestCoverDecision();
	TestDangerDecision();
	TestInterestDecision();
	TestHoldPositionDecisionFromThreshold();
	TestInvalidProfileMapsStatusAndPreservesDiagnostics();
	TestInvalidCurrentStateMapsStatusAndPreservesDiagnostics();
	TestDisabledNpcMapsStatus();
	TestNoMapContextMapsStatus();
	TestConfigThresholdAffectsDecisionDeterministically();
	TestInputsAreNotMutated();

	return Failures;
}
