#include <cstdlib>
#include <vector>

#include "scene/ai/NpcTell.hpp"
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

iggy::NpcPlay PlayWithRead(const iggy::NpcRead &read)
{
	return iggy::NpcPlaySelector {}.play(read);
}

void TestEmptyNoPlayProducesCountsAndNoLines()
{
	const iggy::NpcPlay play;

	const iggy::NpcTell tell = iggy::NpcTeller {}.tell(play);

	Expect(tell.play.status == iggy::NpcPlayStatus::NoPlayableEnt, "empty tell should preserve no-play status");
	Expect(tell.handEntCount == 0, "empty tell should count no hand ents");
	Expect(tell.rankedEntCount == 0, "empty tell should count no ranked ents");
	Expect(tell.issueCount == 0, "empty tell should count no issues");
	Expect(tell.playedCount == 0, "empty tell should count no played ent");
	Expect(tell.lines.empty(), "empty tell should have no lines");
	Expect(!tell.hasLines(), "empty tell should report no lines");
}

void TestPlayedResultEmitsPlayedLineFirst()
{
	const iggy::NpcReadEnt selected = ReadEnt(
		HandEnt(iggy::NpcHandTraitSource::Strength, "strength:push", "action:push", iggy::NpcBehaviorStateType::Seeking, 3.0F, 9, { Id("tag:push") }),
		3.0F,
		2);
	const iggy::NpcPlay play = PlayWithRead({
		{ { selected.ent }, {} },
		{ selected },
		{},
	});

	const iggy::NpcTell tell = iggy::NpcTeller {}.tell(play);

	Expect(tell.playedCount == 1, "played tell should count selected play");
	Expect(tell.lines.size() == 1, "single played tell should have one line");
	Expect(tell.hasLines(), "single played tell should report lines");
	if (tell.lines.size() == 1) {
		const iggy::NpcTellLine &line = tell.lines[0];
		Expect(line.outcome == iggy::NpcTellLineOutcome::Played, "first tell line should be played");
		Expect(line.source == iggy::NpcHandTraitSource::Strength, "played tell line should preserve source trait");
		Expect(line.actionTag == Id("action:push"), "played tell line should preserve action tag");
		Expect(line.behaviorState == iggy::NpcBehaviorStateType::Seeking, "played tell line should preserve behavior state");
		Expect(line.score == 3.0F, "played tell line should preserve score");
		Expect(line.handIndex == 2, "played tell line should preserve hand index");
		Expect(line.drawEntryIndex == 9, "played tell line should preserve draw entry index");
	}
}

void TestRemainingRankedEntsEmitRankedLinesInReadOrder()
{
	const iggy::NpcReadEnt first = ReadEnt(
		HandEnt(iggy::NpcHandTraitSource::Dexterity, "dexterity:first", "action:first", iggy::NpcBehaviorStateType::Seeking, 5.0F),
		5.0F,
		0);
	const iggy::NpcReadEnt second = ReadEnt(
		HandEnt(iggy::NpcHandTraitSource::Wisdom, "wisdom:second", "action:second", iggy::NpcBehaviorStateType::Waiting, 3.0F),
		3.0F,
		1);
	const iggy::NpcReadEnt third = ReadEnt(
		HandEnt(iggy::NpcHandTraitSource::Charisma, "charisma:third", "action:third", iggy::NpcBehaviorStateType::Interacting, 2.0F),
		2.0F,
		2);
	const iggy::NpcPlay play = PlayWithRead({
		{ { first.ent, second.ent, third.ent }, {} },
		{ first, second, third },
		{},
	});

	const iggy::NpcTell tell = iggy::NpcTeller {}.tell(play);

	Expect(tell.lines.size() == 3, "played plus remaining ranked tell should have three lines");
	if (tell.lines.size() == 3) {
		Expect(tell.lines[0].outcome == iggy::NpcTellLineOutcome::Played && tell.lines[0].actionTag == Id("action:first"), "first tell line should be selected play");
		Expect(tell.lines[1].outcome == iggy::NpcTellLineOutcome::Ranked && tell.lines[1].actionTag == Id("action:second"), "second tell line should be next ranked ent");
		Expect(tell.lines[2].outcome == iggy::NpcTellLineOutcome::Ranked && tell.lines[2].actionTag == Id("action:third"), "third tell line should preserve ranked order");
	}
}

void TestCountsMatchPlayReadAndIssues()
{
	const iggy::NpcReadEnt selected = ReadEnt(
		HandEnt(iggy::NpcHandTraitSource::Strength, "strength:first", "action:first", iggy::NpcBehaviorStateType::Seeking, 4.0F),
		4.0F,
		0);
	const iggy::NpcReadEnt ranked = ReadEnt(
		HandEnt(iggy::NpcHandTraitSource::Constitution, "constitution:second", "action:second", iggy::NpcBehaviorStateType::Seeking, 2.0F),
		2.0F,
		1);
	const iggy::NpcHandIssue issue { iggy::NpcHandIssueCode::InvalidDraw, iggy::NpcHandTraitSource::Intelligence };
	const iggy::NpcPlay play = PlayWithRead({
		{ { selected.ent, ranked.ent }, { issue } },
		{ selected, ranked },
		{ issue },
	});

	const iggy::NpcTell tell = iggy::NpcTeller {}.tell(play);

	Expect(tell.handEntCount == 2, "tell should count hand ents");
	Expect(tell.rankedEntCount == 2, "tell should count ranked ents");
	Expect(tell.issueCount == 1, "tell should count read issues");
	Expect(tell.playedCount == 1, "tell should count played ent");
}

void TestHandIssuesEmitInvalidDrawLines()
{
	const iggy::NpcHandIssue firstIssue { iggy::NpcHandIssueCode::InvalidDraw, iggy::NpcHandTraitSource::Dexterity };
	const iggy::NpcHandIssue secondIssue { iggy::NpcHandIssueCode::InvalidDraw, iggy::NpcHandTraitSource::Charisma };
	const iggy::NpcPlay play = PlayWithRead({
		{ {}, { firstIssue, secondIssue } },
		{},
		{ firstIssue, secondIssue },
	});

	const iggy::NpcTell tell = iggy::NpcTeller {}.tell(play);

	Expect(tell.issueCount == 2, "tell should count hand issues preserved by read");
	Expect(tell.lines.size() == 2, "tell should emit invalid draw lines for issues");
	if (tell.lines.size() == 2) {
		Expect(tell.lines[0].outcome == iggy::NpcTellLineOutcome::InvalidDraw && tell.lines[0].source == iggy::NpcHandTraitSource::Dexterity && tell.lines[0].issueIndex == 0, "first invalid draw line should preserve issue source/index");
		Expect(tell.lines[1].outcome == iggy::NpcTellLineOutcome::InvalidDraw && tell.lines[1].source == iggy::NpcHandTraitSource::Charisma && tell.lines[1].issueIndex == 1, "second invalid draw line should preserve issue source/index");
	}
}

void TestTellCopiesFullPlayByValue()
{
	const iggy::NpcReadEnt selected = ReadEnt(
		HandEnt(iggy::NpcHandTraitSource::Wisdom, "wisdom:copy", "action:copy", iggy::NpcBehaviorStateType::Waiting, 2.0F, 6, { Id("tag:copy") }),
		2.0F,
		1);
	const iggy::NpcPlay play = PlayWithRead({
		{ { selected.ent }, {} },
		{ selected },
		{},
	});

	const iggy::NpcTell tell = iggy::NpcTeller {}.tell(play);

	Expect(SamePlay(tell.play, play), "tell should preserve full play by value");
}

void TestTellDoesNotMutateInput()
{
	iggy::NpcReadEnt selected = ReadEnt(
		HandEnt(iggy::NpcHandTraitSource::Intelligence, "intelligence:plan", "action:plan", iggy::NpcBehaviorStateType::Seeking, 3.0F, 4, { Id("tag:plan") }),
		3.0F,
		0);
	iggy::NpcPlay play = PlayWithRead({
		{ { selected.ent }, { { iggy::NpcHandIssueCode::InvalidDraw, iggy::NpcHandTraitSource::Strength } } },
		{ selected },
		{ { iggy::NpcHandIssueCode::InvalidDraw, iggy::NpcHandTraitSource::Strength } },
	});
	const iggy::NpcPlay before = play;

	const iggy::NpcTell tell = iggy::NpcTeller {}.tell(play);

	Expect(tell.hasLines(), "immutability setup should produce tell lines");
	Expect(SamePlay(play, before), "tell should not mutate input play");
	Expect(SamePlay(tell.play, before), "tell should copy input play");
}

} // namespace

int main()
{
	TestEmptyNoPlayProducesCountsAndNoLines();
	TestPlayedResultEmitsPlayedLineFirst();
	TestRemainingRankedEntsEmitRankedLinesInReadOrder();
	TestCountsMatchPlayReadAndIssues();
	TestHandIssuesEmitInvalidDrawLines();
	TestTellCopiesFullPlayByValue();
	TestTellDoesNotMutateInput();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
