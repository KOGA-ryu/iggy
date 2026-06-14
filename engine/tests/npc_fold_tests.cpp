#include <cstdlib>
#include <vector>

#include "scene/ai/NpcFold.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::NpcHandEnt HandEnt(
	iggy::NpcHandTraitSource source,
	const char *entryId,
	const char *actionTag,
	iggy::NpcBehaviorStateType behaviorState,
	float weight,
	std::size_t drawEntryIndex = 0,
	std::vector<iggy::ResourceId> mapTags = {})
{
	return {
		source,
		Id(entryId),
		Id(actionTag),
		behaviorState,
		weight,
		mapTags,
		drawEntryIndex,
	};
}

iggy::NpcReadEnt ReadEnt(const iggy::NpcHandEnt &ent, float score, std::size_t handIndex)
{
	return {
		ent,
		score,
		handIndex,
	};
}

bool SameHandEnt(const iggy::NpcHandEnt &actual, const iggy::NpcHandEnt &expected)
{
	return actual.source == expected.source
		&& actual.entryId == expected.entryId
		&& actual.actionTag == expected.actionTag
		&& actual.behaviorState == expected.behaviorState
		&& actual.weight == expected.weight
		&& actual.mapTags == expected.mapTags
		&& actual.drawEntryIndex == expected.drawEntryIndex;
}

bool SameReadEnt(const iggy::NpcReadEnt &actual, const iggy::NpcReadEnt &expected)
{
	return SameHandEnt(actual.ent, expected.ent)
		&& actual.score == expected.score
		&& actual.handIndex == expected.handIndex;
}

bool SameRead(const iggy::NpcRead &actual, const iggy::NpcRead &expected)
{
	if (actual.hand.ents.size() != expected.hand.ents.size()
		|| actual.hand.issues.size() != expected.hand.issues.size()
		|| actual.rankedEnts.size() != expected.rankedEnts.size()
		|| actual.issues.size() != expected.issues.size())
		return false;
	for (std::size_t index = 0; index < actual.hand.ents.size(); ++index) {
		if (!SameHandEnt(actual.hand.ents[index], expected.hand.ents[index]))
			return false;
	}
	for (std::size_t index = 0; index < actual.hand.issues.size(); ++index) {
		if (actual.hand.issues[index].code != expected.hand.issues[index].code
			|| actual.hand.issues[index].source != expected.hand.issues[index].source)
			return false;
	}
	for (std::size_t index = 0; index < actual.rankedEnts.size(); ++index) {
		if (!SameReadEnt(actual.rankedEnts[index], expected.rankedEnts[index]))
			return false;
	}
	for (std::size_t index = 0; index < actual.issues.size(); ++index) {
		if (actual.issues[index].code != expected.issues[index].code
			|| actual.issues[index].source != expected.issues[index].source)
			return false;
	}
	return true;
}

bool SamePlay(const iggy::NpcPlay &actual, const iggy::NpcPlay &expected)
{
	return actual.status == expected.status
		&& SameRead(actual.read, expected.read)
		&& SameReadEnt(actual.selected, expected.selected);
}

bool SameTellLine(const iggy::NpcTellLine &actual, const iggy::NpcTellLine &expected)
{
	return actual.outcome == expected.outcome
		&& actual.source == expected.source
		&& actual.actionTag == expected.actionTag
		&& actual.behaviorState == expected.behaviorState
		&& actual.score == expected.score
		&& actual.handIndex == expected.handIndex
		&& actual.drawEntryIndex == expected.drawEntryIndex
		&& actual.issueIndex == expected.issueIndex;
}

bool SameTell(const iggy::NpcTell &actual, const iggy::NpcTell &expected)
{
	if (!SamePlay(actual.play, expected.play)
		|| actual.handEntCount != expected.handEntCount
		|| actual.rankedEntCount != expected.rankedEntCount
		|| actual.issueCount != expected.issueCount
		|| actual.playedCount != expected.playedCount
		|| actual.lines.size() != expected.lines.size())
		return false;
	for (std::size_t index = 0; index < actual.lines.size(); ++index) {
		if (!SameTellLine(actual.lines[index], expected.lines[index]))
			return false;
	}
	return true;
}

iggy::NpcTell TellFromRead(const iggy::NpcRead &read)
{
	return iggy::NpcTeller {}.tell(iggy::NpcPlaySelector {}.play(read));
}

iggy::NpcTell PlayTell(float score, std::vector<iggy::NpcHandIssue> issues = {})
{
	const iggy::NpcReadEnt selected = ReadEnt(
		HandEnt(iggy::NpcHandTraitSource::Strength, "strength:play", "action:play", iggy::NpcBehaviorStateType::Seeking, score, 3, { Id("tag:play") }),
		score,
		0);
	return TellFromRead({
		{ { selected.ent }, issues },
		{ selected },
		issues,
	});
}

void TestNoPlayableEntFolds()
{
	const iggy::NpcTell tell = TellFromRead({});

	const iggy::NpcFold fold = iggy::NpcFolder {}.fold(tell);

	Expect(fold.status == iggy::NpcFoldStatus::Folded, "no playable ent should fold");
	Expect(fold.reason == iggy::NpcFoldReason::NoPlayableEnt, "no playable ent should use NoPlayableEnt reason");
	Expect(!fold.kept(), "no playable ent should not be kept");
	Expect(fold.folded(), "no playable ent should report folded");
	Expect(SameTell(fold.tell, tell), "fold should preserve tell");
}

void TestScoreBelowMinimumFolds()
{
	const iggy::NpcTell tell = PlayTell(2.0F);

	const iggy::NpcFold fold = iggy::NpcFolder {}.fold(tell, { 3.0F });

	Expect(fold.status == iggy::NpcFoldStatus::Folded, "below-minimum score should fold");
	Expect(fold.reason == iggy::NpcFoldReason::BelowMinimumScore, "below-minimum score should use BelowMinimumScore reason");
}

void TestScoreEqualToMinimumIsKept()
{
	const iggy::NpcTell tell = PlayTell(3.0F);

	const iggy::NpcFold fold = iggy::NpcFolder {}.fold(tell, { 3.0F });

	Expect(fold.status == iggy::NpcFoldStatus::Kept, "score equal to minimum should be kept");
	Expect(fold.reason == iggy::NpcFoldReason::None, "kept fold should use no reason");
	Expect(fold.kept(), "equal-minimum score should report kept");
	Expect(!fold.folded(), "equal-minimum score should not report folded");
}

void TestTooManyIssuesFoldsWhenConfigured()
{
	const iggy::NpcHandIssue firstIssue { iggy::NpcHandIssueCode::InvalidDraw, iggy::NpcHandTraitSource::Dexterity };
	const iggy::NpcHandIssue secondIssue { iggy::NpcHandIssueCode::InvalidDraw, iggy::NpcHandTraitSource::Wisdom };
	const iggy::NpcTell tell = PlayTell(4.0F, { firstIssue, secondIssue });

	const iggy::NpcFold fold = iggy::NpcFolder {}.fold(tell, { 0.0F, 1 });

	Expect(fold.status == iggy::NpcFoldStatus::Folded, "too many issues should fold when configured");
	Expect(fold.reason == iggy::NpcFoldReason::TooManyIssues, "too many issues should use TooManyIssues reason");
}

void TestDefaultConfigKeepsValidPlayWithIssues()
{
	const iggy::NpcHandIssue issue { iggy::NpcHandIssueCode::InvalidDraw, iggy::NpcHandTraitSource::Charisma };
	const iggy::NpcTell tell = PlayTell(0.0F, { issue });

	const iggy::NpcFold fold = iggy::NpcFolder {}.fold(tell);

	Expect(fold.status == iggy::NpcFoldStatus::Kept, "default fold config should allow issues and zero score");
	Expect(fold.reason == iggy::NpcFoldReason::None, "default kept fold should use no reason");
	Expect(SameTell(fold.tell, tell), "default kept fold should preserve tell");
}

void TestFoldCheckOrderPrefersNoPlayableThenScoreThenIssues()
{
	iggy::NpcTell noPlayWithIssues = TellFromRead({
		{ {}, { { iggy::NpcHandIssueCode::InvalidDraw, iggy::NpcHandTraitSource::Strength } } },
		{},
		{ { iggy::NpcHandIssueCode::InvalidDraw, iggy::NpcHandTraitSource::Strength } },
	});

	const iggy::NpcFold noPlayFold = iggy::NpcFolder {}.fold(noPlayWithIssues, { 10.0F, 0 });
	Expect(noPlayFold.reason == iggy::NpcFoldReason::NoPlayableEnt, "no-play reason should win before score/issues");

	const iggy::NpcTell lowScoreWithIssues = PlayTell(1.0F, { { iggy::NpcHandIssueCode::InvalidDraw, iggy::NpcHandTraitSource::Wisdom } });
	const iggy::NpcFold scoreFold = iggy::NpcFolder {}.fold(lowScoreWithIssues, { 2.0F, 0 });
	Expect(scoreFold.reason == iggy::NpcFoldReason::BelowMinimumScore, "score reason should win before issue threshold when play exists");
}

void TestKeptResultPreservesSelectedPlayFacts()
{
	const iggy::NpcTell tell = PlayTell(5.0F);

	const iggy::NpcFold fold = iggy::NpcFolder {}.fold(tell);

	Expect(fold.kept(), "valid play should be kept");
	Expect(SameTell(fold.tell, tell), "kept fold should preserve tell");
	Expect(fold.tell.play.selected.ent.actionTag == Id("action:play"), "kept fold should preserve selected action tag");
	Expect(fold.tell.play.selected.score == 5.0F, "kept fold should preserve selected score");
	Expect(fold.tell.play.selected.handIndex == 0, "kept fold should preserve selected hand index");
}

void TestFoldDoesNotMutateInput()
{
	iggy::NpcTell tell = PlayTell(4.0F, { { iggy::NpcHandIssueCode::InvalidDraw, iggy::NpcHandTraitSource::Dexterity } });
	const iggy::NpcTell before = tell;

	const iggy::NpcFold fold = iggy::NpcFolder {}.fold(tell, { 0.0F, 3 });

	Expect(fold.kept(), "immutability setup should keep play");
	Expect(SameTell(tell, before), "fold should not mutate input tell");
	Expect(SameTell(fold.tell, before), "fold should copy tell by value");
}

} // namespace

int main()
{
	TestNoPlayableEntFolds();
	TestScoreBelowMinimumFolds();
	TestScoreEqualToMinimumIsKept();
	TestTooManyIssuesFoldsWhenConfigured();
	TestDefaultConfigKeepsValidPlayWithIssues();
	TestFoldCheckOrderPrefersNoPlayableThenScoreThenIssues();
	TestKeptResultPreservesSelectedPlayFacts();
	TestFoldDoesNotMutateInput();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
