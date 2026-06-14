#include <cstdlib>
#include <vector>

#include "scene/ai/NpcAiBehaviorIntent2D.hpp"
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

iggy::AiMapNode2D Node(const char *id)
{
	return {
		Id(id),
		{ 2.0F, 3.0F },
		1.0F,
		0.0F,
		0.0F,
		0.0F,
		0.0F,
		{ Id("tag:patrol") },
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
		{ Id("tag:patrol"), Id("tag:cover") },
	};
}

iggy::NpcAiCurrentState2D State(bool enabled = true)
{
	return {
		Id("npc:guard"),
		{ 2.0F, 3.0F },
		Id("goal:watch"),
		enabled,
	};
}

iggy::AiMapQuery2DResult Query()
{
	iggy::AiMapQuery2DResult query;
	query.status = iggy::AiMapQuery2DStatus::Matched;
	query.position = { 2.0F, 3.0F };
	query.entries.push_back({ Node("ai:node"), 0.0F });
	query.tags = { Id("tag:patrol"), Id("tag:cover") };
	return query;
}

iggy::NpcAiContextScore2DResult Score(
	float patrolScore,
	float coverScore,
	float dangerScore,
	float interestScore,
	iggy::NpcAiContextScore2DStatus status = iggy::NpcAiContextScore2DStatus::Scored)
{
	iggy::NpcAiContextScore2DResult score;
	score.status = status;
	score.profileValidation = iggy::validate(Profile());
	score.currentStateValidation = iggy::validate(State());
	score.query = Query();
	score.patrolScore = patrolScore;
	score.coverScore = coverScore;
	score.dangerScore = dangerScore;
	score.interestScore = interestScore;
	score.matchedBehaviorTags = { Id("tag:patrol") };
	return score;
}

bool SameScore(const iggy::NpcAiContextScore2DResult &actual, const iggy::NpcAiContextScore2DResult &expected)
{
	return actual.status == expected.status
		&& actual.profileValidation.valid == expected.profileValidation.valid
		&& actual.currentStateValidation.valid == expected.currentStateValidation.valid
		&& actual.query.status == expected.query.status
		&& NearVec(actual.query.position, expected.query.position)
		&& actual.query.entries.size() == expected.query.entries.size()
		&& Near(actual.patrolScore, expected.patrolScore)
		&& Near(actual.coverScore, expected.coverScore)
		&& Near(actual.dangerScore, expected.dangerScore)
		&& Near(actual.interestScore, expected.interestScore)
		&& actual.matchedBehaviorTags == expected.matchedBehaviorTags;
}

void ExpectIntent(
	const iggy::NpcAiBehaviorIntent2DResult &result,
	iggy::NpcAiBehaviorIntent2DType type,
	float selectedScore,
	iggy::NpcAiBehaviorIntent2DReason reason,
	const char *message)
{
	Expect(result.status == iggy::NpcAiBehaviorIntent2DStatus::Classified, message);
	Expect(result.type == type, message);
	Expect(Near(result.selectedScore, selectedScore), message);
	Expect(result.reason == reason, message);
	Expect(result.hasIntent(), message);
}

void TestNonScorableScoreProducesNoIntent()
{
	const iggy::NpcAiContextScore2DResult score =
		Score(10.0F, 10.0F, 10.0F, 10.0F, iggy::NpcAiContextScore2DStatus::NoMapContext);

	const iggy::NpcAiBehaviorIntent2DResult result =
		iggy::NpcAiBehaviorIntentClassifier2D {}.classify(score);

	Expect(result.status == iggy::NpcAiBehaviorIntent2DStatus::NotScorable, "non-scorable score should return NotScorable");
	Expect(result.type == iggy::NpcAiBehaviorIntent2DType::None, "non-scorable score should have no intent type");
	Expect(result.selectedScore == 0.0F, "non-scorable score should have zero selected score");
	Expect(result.reason == iggy::NpcAiBehaviorIntent2DReason::ScoreNotPlannable, "non-scorable score should preserve reason");
	Expect(!result.hasIntent(), "non-scorable score should have no actionable intent");
	Expect(SameScore(result.score, score), "non-scorable score should be copied into result");
}

void TestHighestPatrolClassifiesPatrol()
{
	const iggy::NpcAiBehaviorIntent2DResult result =
		iggy::NpcAiBehaviorIntentClassifier2D {}.classify(Score(5.0F, 1.0F, 2.0F, 3.0F));

	ExpectIntent(result, iggy::NpcAiBehaviorIntent2DType::Patrol, 5.0F, iggy::NpcAiBehaviorIntent2DReason::PatrolScore, "highest patrol score should classify Patrol");
}

void TestHighestCoverClassifiesTakeCover()
{
	const iggy::NpcAiBehaviorIntent2DResult result =
		iggy::NpcAiBehaviorIntentClassifier2D {}.classify(Score(1.0F, 6.0F, 2.0F, 3.0F));

	ExpectIntent(result, iggy::NpcAiBehaviorIntent2DType::TakeCover, 6.0F, iggy::NpcAiBehaviorIntent2DReason::CoverScore, "highest cover score should classify TakeCover");
}

void TestHighestDangerClassifiesAvoidDanger()
{
	const iggy::NpcAiBehaviorIntent2DResult result =
		iggy::NpcAiBehaviorIntentClassifier2D {}.classify(Score(1.0F, 2.0F, 7.0F, 3.0F));

	ExpectIntent(result, iggy::NpcAiBehaviorIntent2DType::AvoidDanger, 7.0F, iggy::NpcAiBehaviorIntent2DReason::DangerScore, "highest danger score should classify AvoidDanger");
}

void TestHighestInterestClassifiesInvestigate()
{
	const iggy::NpcAiBehaviorIntent2DResult result =
		iggy::NpcAiBehaviorIntentClassifier2D {}.classify(Score(1.0F, 2.0F, 3.0F, 8.0F));

	ExpectIntent(result, iggy::NpcAiBehaviorIntent2DType::Investigate, 8.0F, iggy::NpcAiBehaviorIntent2DReason::InterestScore, "highest interest score should classify Investigate");
}

void TestThresholdFallbackClassifiesHoldPosition()
{
	const iggy::NpcAiBehaviorIntent2DConfig config { 2.5F };

	const iggy::NpcAiBehaviorIntent2DResult below =
		iggy::NpcAiBehaviorIntentClassifier2D {}.classify(Score(2.49F, 2.0F, 1.0F, 0.5F), config);
	const iggy::NpcAiBehaviorIntent2DResult atThreshold =
		iggy::NpcAiBehaviorIntentClassifier2D {}.classify(Score(2.5F, 0.0F, 0.0F, 0.0F), config);

	ExpectIntent(below, iggy::NpcAiBehaviorIntent2DType::HoldPosition, 0.0F, iggy::NpcAiBehaviorIntent2DReason::BelowThreshold, "all scores below threshold should classify HoldPosition");
	ExpectIntent(atThreshold, iggy::NpcAiBehaviorIntent2DType::Patrol, 2.5F, iggy::NpcAiBehaviorIntent2DReason::PatrolScore, "score equal to threshold should be eligible");
}

void TestTieBreakOrderIsDeterministic()
{
	const iggy::NpcAiBehaviorIntentClassifier2D classifier;

	const iggy::NpcAiBehaviorIntent2DResult dangerBeatsAll =
		classifier.classify(Score(4.0F, 4.0F, 4.0F, 4.0F));
	const iggy::NpcAiBehaviorIntent2DResult coverBeatsInterestAndPatrol =
		classifier.classify(Score(3.0F, 3.0F, 0.0F, 3.0F));
	const iggy::NpcAiBehaviorIntent2DResult interestBeatsPatrol =
		classifier.classify(Score(2.0F, 0.0F, 0.0F, 2.0F));

	ExpectIntent(dangerBeatsAll, iggy::NpcAiBehaviorIntent2DType::AvoidDanger, 4.0F, iggy::NpcAiBehaviorIntent2DReason::DangerScore, "danger should win full tie");
	ExpectIntent(coverBeatsInterestAndPatrol, iggy::NpcAiBehaviorIntent2DType::TakeCover, 3.0F, iggy::NpcAiBehaviorIntent2DReason::CoverScore, "cover should beat interest and patrol ties");
	ExpectIntent(interestBeatsPatrol, iggy::NpcAiBehaviorIntent2DType::Investigate, 2.0F, iggy::NpcAiBehaviorIntent2DReason::InterestScore, "interest should beat patrol tie");
}

void TestCopiedScoreIsPreservedForClassifiedResult()
{
	const iggy::NpcAiContextScore2DResult score = Score(1.0F, 2.0F, 3.0F, 4.0F);

	const iggy::NpcAiBehaviorIntent2DResult result =
		iggy::NpcAiBehaviorIntentClassifier2D {}.classify(score);

	Expect(SameScore(result.score, score), "classified result should preserve copied score result");
}

void TestInputScoreIsNotMutated()
{
	iggy::NpcAiContextScore2DResult score = Score(1.0F, 2.0F, 3.0F, 4.0F);
	const iggy::NpcAiContextScore2DResult before = score;

	const iggy::NpcAiBehaviorIntent2DResult result =
		iggy::NpcAiBehaviorIntentClassifier2D {}.classify(score);

	Expect(result.status == iggy::NpcAiBehaviorIntent2DStatus::Classified, "immutability setup should classify");
	Expect(SameScore(score, before), "intent classifier should not mutate input score");
}

} // namespace

int main()
{
	TestNonScorableScoreProducesNoIntent();
	TestHighestPatrolClassifiesPatrol();
	TestHighestCoverClassifiesTakeCover();
	TestHighestDangerClassifiesAvoidDanger();
	TestHighestInterestClassifiesInvestigate();
	TestThresholdFallbackClassifiesHoldPosition();
	TestTieBreakOrderIsDeterministic();
	TestCopiedScoreIsPreservedForClassifiedResult();
	TestInputScoreIsNotMutated();

	return Failures;
}
