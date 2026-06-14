#include <cstdlib>
#include <vector>

#include "scene/ai/NpcRead.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::NpcHandEnt Ent(
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

bool SameEnt(const iggy::NpcHandEnt &actual, const iggy::NpcHandEnt &expected)
{
	return actual.source == expected.source
		&& actual.entryId == expected.entryId
		&& actual.actionTag == expected.actionTag
		&& actual.behaviorState == expected.behaviorState
		&& actual.weight == expected.weight
		&& actual.mapTags == expected.mapTags
		&& actual.drawEntryIndex == expected.drawEntryIndex;
}

bool SameHand(const iggy::NpcHand &actual, const iggy::NpcHand &expected)
{
	if (actual.ents.size() != expected.ents.size())
		return false;
	if (actual.issues.size() != expected.issues.size())
		return false;
	for (std::size_t index = 0; index < actual.ents.size(); ++index) {
		if (!SameEnt(actual.ents[index], expected.ents[index]))
			return false;
	}
	for (std::size_t index = 0; index < actual.issues.size(); ++index) {
		if (actual.issues[index].code != expected.issues[index].code
			|| actual.issues[index].source != expected.issues[index].source)
			return false;
	}
	return true;
}

void TestEmptyHandReadsEmptyRankedList()
{
	const iggy::NpcRead read = iggy::NpcReader {}.read({});

	Expect(read.hand.ents.empty(), "empty read should copy empty hand ents");
	Expect(read.hand.issues.empty(), "empty read should copy empty hand issues");
	Expect(read.rankedEnts.empty(), "empty hand should read no ranked ents");
	Expect(!read.hasRankedEnts(), "empty read should report no ranked ents");
	Expect(read.issues.empty(), "empty read should have no issues");
	Expect(!read.hasIssues(), "empty read should report no issues");
}

void TestScoresEqualEntWeights()
{
	const iggy::NpcHand hand {
		{
			Ent(iggy::NpcHandTraitSource::Strength, "strength:push", "action:push", iggy::NpcBehaviorStateType::Seeking, 1.5F, 2),
		},
		{},
	};

	const iggy::NpcRead read = iggy::NpcReader {}.read(hand);

	Expect(read.rankedEnts.size() == 1, "single ent hand should read one ranked ent");
	if (read.rankedEnts.size() == 1) {
		Expect(read.rankedEnts[0].score == 1.5F, "read score should equal ent weight");
		Expect(read.rankedEnts[0].handIndex == 0, "read should preserve original hand index");
		Expect(SameEnt(read.rankedEnts[0].ent, hand.ents[0]), "read should copy ranked ent payload");
	}
}

void TestRankingDescendingByWeight()
{
	const iggy::NpcHand hand {
		{
			Ent(iggy::NpcHandTraitSource::Strength, "strength:low", "action:low", iggy::NpcBehaviorStateType::Seeking, 1.0F),
			Ent(iggy::NpcHandTraitSource::Dexterity, "dexterity:high", "action:high", iggy::NpcBehaviorStateType::Seeking, 4.0F),
			Ent(iggy::NpcHandTraitSource::Wisdom, "wisdom:mid", "action:mid", iggy::NpcBehaviorStateType::Seeking, 2.0F),
		},
		{},
	};

	const iggy::NpcRead read = iggy::NpcReader {}.read(hand);

	Expect(read.rankedEnts.size() == 3, "rank setup should read three ents");
	if (read.rankedEnts.size() == 3) {
		Expect(read.rankedEnts[0].ent.entryId == Id("dexterity:high") && read.rankedEnts[0].handIndex == 1, "highest weight should rank first");
		Expect(read.rankedEnts[1].ent.entryId == Id("wisdom:mid") && read.rankedEnts[1].handIndex == 2, "middle weight should rank second");
		Expect(read.rankedEnts[2].ent.entryId == Id("strength:low") && read.rankedEnts[2].handIndex == 0, "lowest weight should rank last");
	}
}

void TestTiesPreserveOriginalHandOrder()
{
	const iggy::NpcHand hand {
		{
			Ent(iggy::NpcHandTraitSource::Strength, "strength:first", "action:first", iggy::NpcBehaviorStateType::Waiting, 2.0F),
			Ent(iggy::NpcHandTraitSource::Charisma, "charisma:second", "action:second", iggy::NpcBehaviorStateType::Waiting, 2.0F),
			Ent(iggy::NpcHandTraitSource::Intelligence, "intelligence:third", "action:third", iggy::NpcBehaviorStateType::Waiting, 2.0F),
		},
		{},
	};

	const iggy::NpcRead read = iggy::NpcReader {}.read(hand);

	Expect(read.rankedEnts.size() == 3, "tie setup should read three ents");
	if (read.rankedEnts.size() == 3) {
		Expect(read.rankedEnts[0].handIndex == 0, "first tied ent should remain first");
		Expect(read.rankedEnts[1].handIndex == 1, "second tied ent should remain second");
		Expect(read.rankedEnts[2].handIndex == 2, "third tied ent should remain third");
	}
}

void TestMinimumScoreFiltersEntries()
{
	const iggy::NpcHand hand {
		{
			Ent(iggy::NpcHandTraitSource::Strength, "strength:below", "action:below", iggy::NpcBehaviorStateType::Seeking, 1.0F),
			Ent(iggy::NpcHandTraitSource::Dexterity, "dexterity:equal", "action:equal", iggy::NpcBehaviorStateType::Seeking, 2.0F),
			Ent(iggy::NpcHandTraitSource::Wisdom, "wisdom:above", "action:above", iggy::NpcBehaviorStateType::Seeking, 3.0F),
		},
		{},
	};

	const iggy::NpcRead read = iggy::NpcReader {}.read(hand, { 2.0F });

	Expect(read.rankedEnts.size() == 2, "minimum score should filter below-threshold ents");
	if (read.rankedEnts.size() == 2) {
		Expect(read.rankedEnts[0].ent.entryId == Id("wisdom:above"), "above-threshold ent should rank first");
		Expect(read.rankedEnts[1].ent.entryId == Id("dexterity:equal"), "equal-threshold ent should be included");
	}
}

void TestZeroWeightEntriesIncludedByDefault()
{
	const iggy::NpcHand hand {
		{
			Ent(iggy::NpcHandTraitSource::Constitution, "constitution:zero", "action:hold", iggy::NpcBehaviorStateType::Waiting, 0.0F),
		},
		{},
	};

	const iggy::NpcRead read = iggy::NpcReader {}.read(hand);

	Expect(read.rankedEnts.size() == 1, "zero-weight ent should be included by default");
	if (read.rankedEnts.size() == 1)
		Expect(read.rankedEnts[0].score == 0.0F, "zero-weight ent should score zero");
}

void TestNegativeWeightsScoreAsIsWhenAllowedByMinimum()
{
	const iggy::NpcHand hand {
		{
			Ent(iggy::NpcHandTraitSource::Strength, "strength:negative", "action:negative", iggy::NpcBehaviorStateType::Seeking, -1.0F),
			Ent(iggy::NpcHandTraitSource::Dexterity, "dexterity:zero", "action:zero", iggy::NpcBehaviorStateType::Seeking, 0.0F),
		},
		{},
	};

	const iggy::NpcRead read = iggy::NpcReader {}.read(hand, { -2.0F });

	Expect(read.rankedEnts.size() == 2, "negative minimum should include negative-weight ents");
	if (read.rankedEnts.size() == 2) {
		Expect(read.rankedEnts[0].ent.entryId == Id("dexterity:zero"), "zero score should rank above negative score");
		Expect(read.rankedEnts[1].ent.entryId == Id("strength:negative"), "negative score should rank below zero");
		Expect(read.rankedEnts[1].score == -1.0F, "negative-weight ent should score as-is");
	}
}

void TestHandIssuesArePreserved()
{
	const iggy::NpcHand hand {
		{
			Ent(iggy::NpcHandTraitSource::Strength, "strength:keep", "action:keep", iggy::NpcBehaviorStateType::Seeking, 1.0F),
		},
		{
			{ iggy::NpcHandIssueCode::InvalidDraw, iggy::NpcHandTraitSource::Wisdom },
		},
	};

	const iggy::NpcRead read = iggy::NpcReader {}.read(hand);

	Expect(read.hand.issues.size() == 1, "read should copy hand issues into copied hand");
	Expect(read.issues.size() == 1, "read should preserve hand issues directly");
	Expect(read.hasIssues(), "read should report preserved issues");
	if (read.issues.size() == 1)
		Expect(read.issues[0].code == iggy::NpcHandIssueCode::InvalidDraw && read.issues[0].source == iggy::NpcHandTraitSource::Wisdom, "read should preserve issue payload");
}

void TestReadDoesNotMutateInput()
{
	iggy::NpcHand hand {
		{
			Ent(iggy::NpcHandTraitSource::Strength, "strength:one", "action:one", iggy::NpcBehaviorStateType::Seeking, 1.0F, 3, { Id("tag:one") }),
			Ent(iggy::NpcHandTraitSource::Wisdom, "wisdom:two", "action:two", iggy::NpcBehaviorStateType::Waiting, 2.0F, 4, { Id("tag:two") }),
		},
		{
			{ iggy::NpcHandIssueCode::InvalidDraw, iggy::NpcHandTraitSource::Dexterity },
		},
	};
	const iggy::NpcHand before = hand;

	const iggy::NpcRead read = iggy::NpcReader {}.read(hand);

	Expect(read.rankedEnts.size() == 2, "immutability setup should read two ents");
	Expect(SameHand(hand, before), "read should not mutate input hand");
	Expect(SameHand(read.hand, before), "read should copy input hand by value");
}

} // namespace

int main()
{
	TestEmptyHandReadsEmptyRankedList();
	TestScoresEqualEntWeights();
	TestRankingDescendingByWeight();
	TestTiesPreserveOriginalHandOrder();
	TestMinimumScoreFiltersEntries();
	TestZeroWeightEntriesIncludedByDefault();
	TestNegativeWeightsScoreAsIsWhenAllowedByMinimum();
	TestHandIssuesArePreserved();
	TestReadDoesNotMutateInput();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
