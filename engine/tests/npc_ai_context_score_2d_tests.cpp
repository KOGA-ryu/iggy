#include <cstdlib>
#include <vector>

#include "scene/ai/NpcAiContextScore2D.hpp"
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

iggy::NpcAiProfile2D Profile(
	const char *profileId = "ai-profile:guard",
	float aggression = 0.5F,
	float bravery = 0.25F,
	float alertness = 0.75F,
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
	iggy::Vec2 position = { 1.0F, 2.0F },
	bool enabled = true)
{
	return {
		Id(npcId),
		position,
		Id("goal:patrol"),
		enabled,
	};
}

iggy::AiMapNode2D Node(const char *id, std::vector<iggy::ResourceId> tags = {})
{
	return {
		Id(id),
		{ 1.0F, 2.0F },
		2.0F,
		1.0F,
		2.0F,
		3.0F,
		4.0F,
		tags,
		{},
		true,
	};
}

iggy::AiMapQuery2DResult Query(
	float patrolWeight = 8.0F,
	float coverWeight = 6.0F,
	float dangerWeight = 5.0F,
	float interestWeight = 10.0F,
	std::vector<iggy::ResourceId> tags = { Id("tag:cover"), Id("tag:patrol") })
{
	iggy::AiMapQuery2DResult query;
	query.status = iggy::AiMapQuery2DStatus::Matched;
	query.position = { 1.0F, 2.0F };
	query.entries.push_back({ Node("ai:node", tags), 0.0F });
	query.patrolWeight = patrolWeight;
	query.coverWeight = coverWeight;
	query.dangerWeight = dangerWeight;
	query.interestWeight = interestWeight;
	query.tags = tags;
	return query;
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

bool SameQuery(const iggy::AiMapQuery2DResult &actual, const iggy::AiMapQuery2DResult &expected)
{
	if (actual.status != expected.status
		|| !NearVec(actual.position, expected.position)
		|| actual.entries.size() != expected.entries.size()
		|| actual.patrolWeight != expected.patrolWeight
		|| actual.coverWeight != expected.coverWeight
		|| actual.dangerWeight != expected.dangerWeight
		|| actual.interestWeight != expected.interestWeight
		|| actual.tags != expected.tags) {
		return false;
	}

	for (std::size_t index = 0; index < actual.entries.size(); ++index) {
		if (actual.entries[index].node.id != expected.entries[index].node.id
			|| actual.entries[index].distance != expected.entries[index].distance)
			return false;
	}
	return true;
}

void ExpectZeroScores(const iggy::NpcAiContextScore2DResult &result, const char *message)
{
	Expect(
		result.patrolScore == 0.0F
			&& result.coverScore == 0.0F
			&& result.dangerScore == 0.0F
			&& result.interestScore == 0.0F,
		message);
}

void TestInvalidProfilePreservesDiagnostics()
{
	const iggy::NpcAiProfile2D profile = Profile("", -0.1F);
	const iggy::NpcAiCurrentState2D state = State();
	const iggy::AiMapQuery2DResult query = Query();

	const iggy::NpcAiContextScore2DResult result =
		iggy::NpcAiContextScorer2D {}.score(profile, state, query);

	Expect(result.status == iggy::NpcAiContextScore2DStatus::InvalidProfile, "invalid profile should stop scoring first");
	Expect(!result.canPlan(), "invalid profile score should not be plannable");
	Expect(result.hasContext(), "invalid profile score should still preserve matched query context");
	Expect(!result.profileValidation.valid, "invalid profile score should preserve profile diagnostics");
	Expect(result.profileValidation.issues.size() == 2, "invalid profile score should preserve all profile issues");
	Expect(result.currentStateValidation.valid, "invalid profile score should still preserve state validation");
	Expect(SameQuery(result.query, query), "invalid profile score should copy query result");
	ExpectZeroScores(result, "invalid profile score should have no meaningful scores");
}

void TestInvalidCurrentStatePreservesDiagnostics()
{
	const iggy::NpcAiProfile2D profile = Profile();
	const iggy::NpcAiCurrentState2D state = State("");
	const iggy::AiMapQuery2DResult query = Query();

	const iggy::NpcAiContextScore2DResult result =
		iggy::NpcAiContextScorer2D {}.score(profile, state, query);

	Expect(result.status == iggy::NpcAiContextScore2DStatus::InvalidCurrentState, "invalid state should stop scoring after valid profile");
	Expect(result.profileValidation.valid, "invalid state score should preserve valid profile validation");
	Expect(!result.currentStateValidation.valid, "invalid state score should preserve state diagnostics");
	Expect(result.currentStateValidation.issues.size() == 1, "invalid state score should report state issue");
	ExpectZeroScores(result, "invalid state score should have no meaningful scores");
}

void TestDisabledNpcReturnsDisabledStatus()
{
	const iggy::NpcAiProfile2D profile = Profile();
	const iggy::NpcAiCurrentState2D state = State("npc:disabled", { 0.0F, 0.0F }, false);
	const iggy::AiMapQuery2DResult query = Query();

	const iggy::NpcAiContextScore2DResult result =
		iggy::NpcAiContextScorer2D {}.score(profile, state, query);

	Expect(result.status == iggy::NpcAiContextScore2DStatus::DisabledNpc, "disabled npc should not score context");
	Expect(result.profileValidation.valid && result.currentStateValidation.valid, "disabled npc should still preserve valid validation results");
	Expect(!result.canPlan(), "disabled npc score should not be plannable");
	ExpectZeroScores(result, "disabled npc score should have no meaningful scores");
}

void TestNoMapContextReturnsNoMapContext()
{
	const iggy::NpcAiProfile2D profile = Profile();
	const iggy::NpcAiCurrentState2D state = State();
	iggy::AiMapQuery2DResult query;
	query.position = { 5.0F, 6.0F };

	const iggy::NpcAiContextScore2DResult result =
		iggy::NpcAiContextScorer2D {}.score(profile, state, query);

	Expect(result.status == iggy::NpcAiContextScore2DStatus::NoMapContext, "empty query should return NoMapContext");
	Expect(!result.hasContext(), "NoMapContext result should have no context");
	Expect(!result.canPlan(), "NoMapContext result should not be plannable");
	Expect(result.matchedBehaviorTags.empty(), "NoMapContext result should have no matched tags");
	ExpectZeroScores(result, "NoMapContext result should have zero scores");
}

void TestValidProfileStateAndQueryScoresDeterministically()
{
	const iggy::NpcAiProfile2D profile =
		Profile("ai-profile:scoring", 0.8F, 0.25F, 0.5F, { Id("tag:patrol"), Id("tag:missing"), Id("tag:cover") });
	const iggy::NpcAiCurrentState2D state = State();
	const iggy::AiMapQuery2DResult query = Query(8.0F, 6.0F, 5.0F, 10.0F, { Id("tag:cover"), Id("tag:patrol") });

	const iggy::NpcAiContextScore2DResult result =
		iggy::NpcAiContextScorer2D {}.score(profile, state, query);

	Expect(result.status == iggy::NpcAiContextScore2DStatus::Scored, "valid context should be scored");
	Expect(result.hasContext(), "scored context should report context");
	Expect(result.canPlan(), "scored context should be plannable by future layers");
	Expect(Near(result.patrolScore, 6.4F), "patrol score should reduce patrol weight by aggression factor");
	Expect(Near(result.coverScore, 4.5F), "cover score should scale inversely with bravery");
	Expect(Near(result.dangerScore, 6.25F), "danger score should scale by danger, bravery, and alertness");
	Expect(Near(result.interestScore, 5.0F), "interest score should scale by alertness");
	Expect(result.matchedBehaviorTags == std::vector<iggy::ResourceId>({ Id("tag:patrol"), Id("tag:cover") }), "matched tags should preserve profile order");
	Expect(SameProfile(result.profileValidation.profile, profile), "scored context should copy profile validation result");
	Expect(SameState(result.currentStateValidation.state, state), "scored context should copy state validation result");
	Expect(SameQuery(result.query, query), "scored context should copy query result");
}

void TestTraitChangesAffectScores()
{
	const iggy::AiMapQuery2DResult query = Query(8.0F, 6.0F, 5.0F, 10.0F);
	const iggy::NpcAiCurrentState2D state = State();
	const iggy::NpcAiContextScore2DResult cautious =
		iggy::NpcAiContextScorer2D {}.score(Profile("ai-profile:cautious", 0.0F, 0.0F, 1.0F), state, query);
	const iggy::NpcAiContextScore2DResult brave =
		iggy::NpcAiContextScorer2D {}.score(Profile("ai-profile:brave", 1.0F, 1.0F, 0.0F), state, query);

	Expect(Near(cautious.patrolScore, 8.0F), "zero aggression should keep full patrol score");
	Expect(Near(brave.patrolScore, 6.0F), "full aggression should reduce patrol score by 25 percent");
	Expect(Near(cautious.coverScore, 6.0F), "zero bravery should keep full cover score");
	Expect(Near(brave.coverScore, 0.0F), "full bravery should remove cover score");
	Expect(Near(cautious.dangerScore, 10.0F), "high alertness and low bravery should increase danger score");
	Expect(Near(brave.dangerScore, 0.0F), "high bravery and no alertness should remove danger score");
	Expect(Near(cautious.interestScore, 10.0F), "full alertness should keep interest score");
	Expect(Near(brave.interestScore, 0.0F), "zero alertness should remove interest score");
}

void TestTagIntersectionDedupesInProfileOrderAndKeepsExactIds()
{
	const iggy::NpcAiProfile2D profile =
		Profile("ai-profile:tags", 0.5F, 0.5F, 0.5F, { Id("cover"), Id("tag:cover"), Id("cover"), Id("tag:patrol") });
	const iggy::AiMapQuery2DResult query =
		Query(0.0F, 0.0F, 0.0F, 0.0F, { Id("tag:cover"), Id("cover"), Id("tag:patrol") });

	const iggy::NpcAiContextScore2DResult result =
		iggy::NpcAiContextScorer2D {}.score(profile, State(), query);

	Expect(result.matchedBehaviorTags == std::vector<iggy::ResourceId>({ Id("cover"), Id("tag:cover"), Id("tag:patrol") }), "matched tags should de-dupe in profile order and keep exact ResourceIds");
}

void TestInputsAreNotMutated()
{
	iggy::NpcAiProfile2D profile =
		Profile("ai-profile:immutable", 0.2F, 0.3F, 0.4F, { Id("tag:a"), Id("tag:b") });
	iggy::NpcAiCurrentState2D state = State("npc:immutable", { 8.0F, 9.0F });
	iggy::AiMapQuery2DResult query = Query(1.0F, 2.0F, 3.0F, 4.0F, { Id("tag:a") });
	const iggy::NpcAiProfile2D profileBefore = profile;
	const iggy::NpcAiCurrentState2D stateBefore = state;
	const iggy::AiMapQuery2DResult queryBefore = query;

	const iggy::NpcAiContextScore2DResult result =
		iggy::NpcAiContextScorer2D {}.score(profile, state, query);

	Expect(result.status == iggy::NpcAiContextScore2DStatus::Scored, "context scoring immutability setup should score");
	Expect(SameProfile(profile, profileBefore), "context scoring should not mutate profile input");
	Expect(SameState(state, stateBefore), "context scoring should not mutate state input");
	Expect(SameQuery(query, queryBefore), "context scoring should not mutate query input");
}

} // namespace

int main()
{
	TestInvalidProfilePreservesDiagnostics();
	TestInvalidCurrentStatePreservesDiagnostics();
	TestDisabledNpcReturnsDisabledStatus();
	TestNoMapContextReturnsNoMapContext();
	TestValidProfileStateAndQueryScoresDeterministically();
	TestTraitChangesAffectScores();
	TestTagIntersectionDedupesInProfileOrderAndKeepsExactIds();
	TestInputsAreNotMutated();

	return Failures;
}
