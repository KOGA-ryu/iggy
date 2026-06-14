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
	std::vector<iggy::ResourceId> tags = {},
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
		tags,
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
	float aggression = 0.0F,
	float bravery = 0.0F,
	float alertness = 1.0F,
	std::vector<iggy::ResourceId> behaviorTags = {})
{
	return {
		Id(profileId),
		Id("faction:town"),
		aggression,
		bravery,
		alertness,
		4.0F,
		behaviorTags,
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

void ExpectSelectedNode(
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

void TestPatrolContextSelectsPatrolNode()
{
	const iggy::AiMap2D map = Map({
		Node("ai:patrol-low", { 0.0F, 0.0F }, 4.0F, 1.0F, 0.0F, 0.0F, 0.0F),
		Node("ai:patrol-high", { 2.0F, 0.0F }, 4.0F, 5.0F, 0.0F, 0.0F, 0.0F),
	});

	const iggy::NpcAiDecision2DResult result =
		iggy::NpcAiDecisionMaker2D {}.decide(map, Profile("ai-profile:patrol"), State());

	ExpectSelectedNode(result, iggy::NpcAiBehaviorIntent2DType::Patrol, "ai:patrol-high", "map-owned patrol context should drive patrol decision");
	Expect(Near(result.query.patrolWeight, 6.0F), "patrol decision should preserve aggregated query patrol weight");
	Expect(Near(result.score.patrolScore, 6.0F), "patrol decision should preserve scored patrol weight");
}

void TestOverlappingContextsAggregateTieBreakAndTarget()
{
	const iggy::AiMap2D map = Map({
		Node("ai:tactical-dangerous", { 0.0F, 0.0F }, 4.0F, 0.0F, 2.0F, 1.5F, 1.0F),
		Node("ai:tactical-safer", { 1.0F, 0.0F }, 4.0F, 0.0F, 2.0F, 0.5F, 3.0F),
	});

	const iggy::NpcAiDecision2DResult result =
		iggy::NpcAiDecisionMaker2D {}.decide(map, Profile("ai-profile:tactical", 0.0F, 0.0F, 1.0F), State());

	ExpectSelectedNode(result, iggy::NpcAiBehaviorIntent2DType::AvoidDanger, "ai:tactical-safer", "danger should win deterministic score tie and target safest local node");
	Expect(result.query.entries.size() == 2, "overlapping contexts should both appear in query entries");
	Expect(Near(result.query.coverWeight, 4.0F), "overlapping contexts should aggregate cover weight");
	Expect(Near(result.query.dangerWeight, 2.0F), "overlapping contexts should aggregate danger weight");
	Expect(Near(result.query.interestWeight, 4.0F), "overlapping contexts should aggregate interest weight");
	Expect(Near(result.score.coverScore, 4.0F), "cover score should participate in tie");
	Expect(Near(result.score.dangerScore, 4.0F), "danger score should participate in tie");
	Expect(Near(result.score.interestScore, 4.0F), "interest score should participate in tie");
	Expect(result.intent.reason == iggy::NpcAiBehaviorIntent2DReason::DangerScore, "danger should win score tie before cover, interest, and patrol");
	Expect(Near(result.target.selectedScore, 0.5F), "AvoidDanger target should choose lowest node danger weight");
}

void TestDisabledNodeIgnoredByQuery()
{
	const iggy::AiMap2D map = Map({
		Node("ai:disabled-danger", { 0.0F, 0.0F }, 4.0F, 0.0F, 0.0F, 100.0F, 0.0F, {}, false),
		Node("ai:enabled-patrol", { 0.0F, 0.0F }, 4.0F, 2.0F, 0.0F, 0.0F, 0.0F),
	});

	const iggy::NpcAiDecision2DResult result =
		iggy::NpcAiDecisionMaker2D {}.decide(map, Profile("ai-profile:ignore-disabled"), State());

	ExpectSelectedNode(result, iggy::NpcAiBehaviorIntent2DType::Patrol, "ai:enabled-patrol", "disabled AI map nodes should not influence decisions");
	Expect(result.query.entries.size() == 1, "disabled AI map node should be excluded from query entries");
	Expect(Near(result.query.dangerWeight, 0.0F), "disabled AI map node danger should not aggregate");
}

void TestDisabledNpcAndInvalidInputsPreserveDiagnostics()
{
	const iggy::AiMap2D map = Map({
		Node("ai:local", { 0.0F, 0.0F }, 2.0F, 1.0F, 0.0F, 0.0F, 0.0F),
	});

	const iggy::NpcAiDecision2DResult disabled =
		iggy::NpcAiDecisionMaker2D {}.decide(map, Profile("ai-profile:disabled"), State("npc:disabled", { 0.0F, 0.0F }, false));
	const iggy::NpcAiDecision2DResult invalidProfile =
		iggy::NpcAiDecisionMaker2D {}.decide(map, Profile("", -1.0F), State());
	const iggy::NpcAiDecision2DResult invalidState =
		iggy::NpcAiDecisionMaker2D {}.decide(map, Profile("ai-profile:valid"), State(""));

	Expect(disabled.status == iggy::NpcAiDecision2DStatus::DisabledNpc, "disabled NPC should map to DisabledNpc");
	Expect(!disabled.hasDecision(), "disabled NPC should not produce target decision");
	Expect(disabled.target.status == iggy::NpcAiIntentTarget2DStatus::NoIntent, "disabled NPC should not produce target");

	Expect(invalidProfile.status == iggy::NpcAiDecision2DStatus::InvalidProfile, "invalid profile should map to InvalidProfile");
	Expect(!invalidProfile.score.profileValidation.valid, "invalid profile diagnostics should be preserved");
	Expect(!invalidProfile.score.profileValidation.issues.empty(), "invalid profile issues should be preserved");
	Expect(invalidProfile.target.status == iggy::NpcAiIntentTarget2DStatus::NoIntent, "invalid profile should not produce target");

	Expect(invalidState.status == iggy::NpcAiDecision2DStatus::InvalidCurrentState, "invalid current state should map to InvalidCurrentState");
	Expect(!invalidState.score.currentStateValidation.valid, "invalid current-state diagnostics should be preserved");
	Expect(!invalidState.score.currentStateValidation.issues.empty(), "invalid current-state issues should be preserved");
	Expect(invalidState.target.status == iggy::NpcAiIntentTarget2DStatus::NoIntent, "invalid current state should not produce target");
}

void TestNoMapContext()
{
	const iggy::AiMap2D map = Map({
		Node("ai:far-away", { 100.0F, 100.0F }, 1.0F, 10.0F, 0.0F, 0.0F, 0.0F),
	});

	const iggy::NpcAiDecision2DResult result =
		iggy::NpcAiDecisionMaker2D {}.decide(map, Profile("ai-profile:no-context"), State());

	Expect(result.status == iggy::NpcAiDecision2DStatus::NoMapContext, "position outside AI map context should return NoMapContext");
	Expect(result.query.status == iggy::AiMapQuery2DStatus::NoMatch, "NoMapContext should preserve query NoMatch");
	Expect(result.score.status == iggy::NpcAiContextScore2DStatus::NoMapContext, "NoMapContext should preserve score status");
	Expect(result.intent.status == iggy::NpcAiBehaviorIntent2DStatus::NotScorable, "NoMapContext should not classify intent");
	Expect(result.target.status == iggy::NpcAiIntentTarget2DStatus::NoIntent, "NoMapContext should not select target");
}

void TestBelowThresholdHoldsCurrentPosition()
{
	const iggy::AiMap2D map = Map({
		Node("ai:low-signal", { 2.0F, 3.0F }, 2.0F, 1.0F, 1.0F, 0.0F, 1.0F),
	});
	const iggy::NpcAiCurrentState2D state = State("npc:hold", { 2.0F, 3.0F });
	const iggy::NpcAiDecision2DConfig config { { 20.0F } };

	const iggy::NpcAiDecision2DResult result =
		iggy::NpcAiDecisionMaker2D {}.decide(map, Profile("ai-profile:hold", 0.0F, 0.0F, 1.0F), state, config);

	Expect(result.status == iggy::NpcAiDecision2DStatus::Decided, "scorable context below threshold should still decide HoldPosition");
	Expect(result.intent.type == iggy::NpcAiBehaviorIntent2DType::HoldPosition, "below-threshold scores should classify HoldPosition");
	Expect(result.intent.reason == iggy::NpcAiBehaviorIntent2DReason::BelowThreshold, "below-threshold decision should preserve reason");
	Expect(result.target.status == iggy::NpcAiIntentTarget2DStatus::Selected, "HoldPosition should select a target");
	Expect(!result.target.selectedFromNode, "HoldPosition target should not come from a map node");
	Expect(NearVec(result.target.selectedPosition, state.position), "HoldPosition should target current NPC position");
}

void TestTagsAndNamespacedIdsRemainDistinct()
{
	const iggy::AiMap2D map = Map({
		Node("ai:plain", { 0.0F, 0.0F }, 4.0F, 2.0F, 0.0F, 0.0F, 0.0F, { Id("cover"), Id("ai:cover") }),
		Node("ai:namespaced", { 0.0F, 0.0F }, 4.0F, 3.0F, 0.0F, 0.0F, 0.0F, { Id("ai:cover"), Id("cover") }),
	});
	const iggy::NpcAiProfile2D profile =
		Profile("ai-profile:tags", 0.0F, 0.0F, 1.0F, { Id("ai:cover"), Id("cover"), Id("missing") });

	const iggy::NpcAiDecision2DResult result =
		iggy::NpcAiDecisionMaker2D {}.decide(map, profile, State());

	ExpectSelectedNode(result, iggy::NpcAiBehaviorIntent2DType::Patrol, "ai:namespaced", "namespaced id setup should still decide from patrol");
	Expect(result.query.tags.size() == 2, "query tags should de-dupe exact values");
	if (result.query.tags.size() == 2) {
		Expect(result.query.tags[0] == Id("cover"), "query tags should preserve first-seen unqualified tag");
		Expect(result.query.tags[1] == Id("ai:cover"), "query tags should preserve distinct namespaced tag");
	}
	Expect(result.score.matchedBehaviorTags.size() == 2, "matched behavior tags should intersect profile and map tags");
	if (result.score.matchedBehaviorTags.size() == 2) {
		Expect(result.score.matchedBehaviorTags[0] == Id("ai:cover"), "matched tags should preserve profile tag order first");
		Expect(result.score.matchedBehaviorTags[1] == Id("cover"), "matched tags should keep namespaced and unqualified ids distinct");
	}
}

void TestMapWeightsChangeDecisionWithoutProfileChange()
{
	const iggy::NpcAiProfile2D profile = Profile("ai-profile:stable", 0.0F, 0.0F, 1.0F);
	const iggy::NpcAiCurrentState2D state = State();
	const iggy::AiMap2D patrolMap = Map({
		Node("ai:patrol", { 0.0F, 0.0F }, 3.0F, 4.0F, 0.0F, 0.0F, 0.0F),
	});
	const iggy::AiMap2D coverMap = Map({
		Node("ai:cover", { 0.0F, 0.0F }, 3.0F, 0.0F, 5.0F, 0.0F, 0.0F),
	});

	const iggy::NpcAiDecision2DResult patrol =
		iggy::NpcAiDecisionMaker2D {}.decide(patrolMap, profile, state);
	const iggy::NpcAiDecision2DResult cover =
		iggy::NpcAiDecisionMaker2D {}.decide(coverMap, profile, state);

	Expect(patrol.intent.type == iggy::NpcAiBehaviorIntent2DType::Patrol, "map patrol weights should drive patrol intent with unchanged profile");
	Expect(cover.intent.type == iggy::NpcAiBehaviorIntent2DType::TakeCover, "map cover weights should drive cover intent with unchanged profile");
}

void TestProfileBiasChangesDecisionWithoutMapChange()
{
	const iggy::AiMap2D map = Map({
		Node("ai:cover-danger", { 0.0F, 0.0F }, 3.0F, 0.0F, 4.0F, 3.0F, 0.0F),
	});
	const iggy::NpcAiCurrentState2D state = State();

	const iggy::NpcAiDecision2DResult cautious =
		iggy::NpcAiDecisionMaker2D {}.decide(map, Profile("ai-profile:cautious", 0.0F, 0.0F, 1.0F), state);
	const iggy::NpcAiDecision2DResult brave =
		iggy::NpcAiDecisionMaker2D {}.decide(map, Profile("ai-profile:brave", 0.0F, 1.0F, 0.0F), state);

	Expect(cautious.intent.type == iggy::NpcAiBehaviorIntent2DType::AvoidDanger, "cautious profile bias should prefer avoiding danger on same map");
	Expect(brave.intent.type == iggy::NpcAiBehaviorIntent2DType::HoldPosition, "brave low-alert profile should suppress same map scores below default threshold");
	Expect(Near(cautious.score.dangerScore, 6.0F), "cautious profile should preserve high danger score diagnostics");
	Expect(Near(brave.score.coverScore, 0.0F), "brave profile should suppress cover score on same map");
	Expect(Near(brave.score.dangerScore, 0.0F), "brave profile should suppress danger score on same map");
}

void TestInputsAreNotMutated()
{
	iggy::AiMap2D map = Map({
		Node("ai:immutable", { 0.0F, 0.0F }, 3.0F, 4.0F, 1.0F, 0.0F, 2.0F, { Id("tag:immutable") }),
	});
	iggy::NpcAiProfile2D profile = Profile("ai-profile:immutable", 0.25F, 0.5F, 1.0F, { Id("tag:immutable") });
	iggy::NpcAiCurrentState2D state = State("npc:immutable");
	const std::vector<iggy::AiMapNode2D> nodesBefore = map.nodes;
	const iggy::NpcAiProfile2D profileBefore = profile;
	const iggy::NpcAiCurrentState2D stateBefore = state;

	const iggy::NpcAiDecision2DResult result =
		iggy::NpcAiDecisionMaker2D {}.decide(map, profile, state);

	Expect(result.hasDecision(), "immutability setup should produce a decision");
	Expect(SameNodes(map.nodes, nodesBefore), "decision pipeline should not mutate AI map");
	Expect(SameProfile(profile, profileBefore), "decision pipeline should not mutate NPC profile");
	Expect(SameState(state, stateBefore), "decision pipeline should not mutate current state");
}

} // namespace

int main()
{
	TestPatrolContextSelectsPatrolNode();
	TestOverlappingContextsAggregateTieBreakAndTarget();
	TestDisabledNodeIgnoredByQuery();
	TestDisabledNpcAndInvalidInputsPreserveDiagnostics();
	TestNoMapContext();
	TestBelowThresholdHoldsCurrentPosition();
	TestTagsAndNamespacedIdsRemainDistinct();
	TestMapWeightsChangeDecisionWithoutProfileChange();
	TestProfileBiasChangesDecisionWithoutMapChange();
	TestInputsAreNotMutated();

	return Failures;
}
