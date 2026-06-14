#include <cstdlib>
#include <vector>

#include "scene/ai/NpcPlay.hpp"
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

void TestEmptyReadReturnsNoPlayableEnt()
{
	const iggy::NpcRead read;

	const iggy::NpcPlay play = iggy::NpcPlaySelector {}.play(read);

	Expect(play.status == iggy::NpcPlayStatus::NoPlayableEnt, "empty read should return NoPlayableEnt");
	Expect(!play.hasPlay(), "empty read should not report a play");
	Expect(SameRead(play.read, read), "empty play should preserve read");
}

void TestSingleRankedEntIsSelected()
{
	const iggy::NpcReadEnt ent = ReadEnt(
		HandEnt(iggy::NpcHandTraitSource::Strength, "strength:push", "action:push", iggy::NpcBehaviorStateType::Seeking, 3.0F, 8, { Id("tag:push") }),
		3.0F,
		2);
	const iggy::NpcRead read {
		{},
		{ ent },
		{},
	};

	const iggy::NpcPlay play = iggy::NpcPlaySelector {}.play(read);

	Expect(play.status == iggy::NpcPlayStatus::Played, "single ranked ent should be played");
	Expect(play.hasPlay(), "single ranked ent should report a play");
	Expect(SameReadEnt(play.selected, ent), "single ranked ent should be selected by value");
	Expect(SameRead(play.read, read), "single ranked ent play should preserve full read");
}

void TestMultipleRankedEntsSelectFirst()
{
	const iggy::NpcReadEnt first = ReadEnt(
		HandEnt(iggy::NpcHandTraitSource::Dexterity, "dexterity:high", "action:high", iggy::NpcBehaviorStateType::Seeking, 4.0F),
		4.0F,
		1);
	const iggy::NpcReadEnt second = ReadEnt(
		HandEnt(iggy::NpcHandTraitSource::Wisdom, "wisdom:mid", "action:mid", iggy::NpcBehaviorStateType::Seeking, 2.0F),
		2.0F,
		2);
	const iggy::NpcRead read {
		{},
		{ first, second },
		{},
	};

	const iggy::NpcPlay play = iggy::NpcPlaySelector {}.play(read);

	Expect(play.status == iggy::NpcPlayStatus::Played, "multi-ent read should be played");
	Expect(SameReadEnt(play.selected, first), "play should select the first ranked ent");
}

void TestTieOrderingComesFromRead()
{
	const iggy::NpcReadEnt firstTied = ReadEnt(
		HandEnt(iggy::NpcHandTraitSource::Charisma, "charisma:first", "action:first", iggy::NpcBehaviorStateType::Waiting, 2.0F),
		2.0F,
		4);
	const iggy::NpcReadEnt secondTied = ReadEnt(
		HandEnt(iggy::NpcHandTraitSource::Intelligence, "intelligence:second", "action:second", iggy::NpcBehaviorStateType::Waiting, 2.0F),
		2.0F,
		1);
	const iggy::NpcRead read {
		{},
		{ firstTied, secondTied },
		{},
	};

	const iggy::NpcPlay play = iggy::NpcPlaySelector {}.play(read);

	Expect(play.status == iggy::NpcPlayStatus::Played, "tied ranked ents should still produce a play");
	Expect(SameReadEnt(play.selected, firstTied), "play should preserve tie ordering supplied by read");
}

void TestReadIssuesAndSelectedDetailsArePreserved()
{
	const iggy::NpcReadEnt selected = ReadEnt(
		HandEnt(iggy::NpcHandTraitSource::Wisdom, "wisdom:warn", "action:warn", iggy::NpcBehaviorStateType::Interacting, 5.0F, 12, { Id("tag"), Id("tag:map") }),
		5.0F,
		3);
	const iggy::NpcRead read {
		{
			{ selected.ent },
			{
				{ iggy::NpcHandIssueCode::InvalidDraw, iggy::NpcHandTraitSource::Dexterity },
			},
		},
		{ selected },
		{
			{ iggy::NpcHandIssueCode::InvalidDraw, iggy::NpcHandTraitSource::Dexterity },
		},
	};

	const iggy::NpcPlay play = iggy::NpcPlaySelector {}.play(read);

	Expect(play.status == iggy::NpcPlayStatus::Played, "read with issues can still play ranked ents");
	Expect(SameRead(play.read, read), "play should preserve full read with issues");
	Expect(SameReadEnt(play.selected, selected), "play should preserve selected ent details");
	Expect(play.selected.ent.source == iggy::NpcHandTraitSource::Wisdom, "selected ent should preserve source trait");
	Expect(play.selected.ent.actionTag == Id("action:warn"), "selected ent should preserve action tag");
	Expect(play.selected.ent.behaviorState == iggy::NpcBehaviorStateType::Interacting, "selected ent should preserve behavior state");
	Expect(play.selected.score == 5.0F, "selected ent should preserve score");
	Expect(play.selected.handIndex == 3, "selected ent should preserve hand index");
}

void TestPlayDoesNotMutateInput()
{
	iggy::NpcRead read {
		{
			{
				HandEnt(iggy::NpcHandTraitSource::Strength, "strength:one", "action:one", iggy::NpcBehaviorStateType::Seeking, 1.0F, 2, { Id("tag:one") }),
			},
			{
				{ iggy::NpcHandIssueCode::InvalidDraw, iggy::NpcHandTraitSource::Wisdom },
			},
		},
		{
			ReadEnt(HandEnt(iggy::NpcHandTraitSource::Strength, "strength:one", "action:one", iggy::NpcBehaviorStateType::Seeking, 1.0F, 2, { Id("tag:one") }), 1.0F, 0),
		},
		{
			{ iggy::NpcHandIssueCode::InvalidDraw, iggy::NpcHandTraitSource::Wisdom },
		},
	};
	const iggy::NpcRead before = read;

	const iggy::NpcPlay play = iggy::NpcPlaySelector {}.play(read);

	Expect(play.status == iggy::NpcPlayStatus::Played, "immutability setup should play");
	Expect(SameRead(read, before), "play should not mutate input read");
	Expect(SameRead(play.read, before), "play should copy read by value");
}

} // namespace

int main()
{
	TestEmptyReadReturnsNoPlayableEnt();
	TestSingleRankedEntIsSelected();
	TestMultipleRankedEntsSelectFirst();
	TestTieOrderingComesFromRead();
	TestReadIssuesAndSelectedDetailsArePreserved();
	TestPlayDoesNotMutateInput();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
