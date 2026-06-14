#include <cstdlib>
#include <vector>

#include "scene/ai/NpcMapReadProjection.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::NpcHandEnt HandEnt(
	const char *entryId,
	const char *actionTag,
	float weight,
	std::size_t drawEntryIndex = 0,
	std::vector<iggy::ResourceId> mapTags = {})
{
	return {
		iggy::NpcHandTraitSource::Strength,
		Id(entryId),
		Id(actionTag),
		iggy::NpcBehaviorStateType::Seeking,
		weight,
		mapTags,
		drawEntryIndex,
	};
}

iggy::NpcMapReadEnt MapReadEnt(
	const iggy::NpcHandEnt &ent,
	float baseWeight,
	float score,
	std::size_t handIndex,
	std::vector<iggy::ResourceId> matchedMapTags = {},
	std::vector<iggy::ResourceId> unmatchedMapTags = {})
{
	return {
		ent,
		baseWeight,
		score,
		handIndex,
		matchedMapTags,
		unmatchedMapTags,
		!matchedMapTags.empty(),
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

bool SameHand(const iggy::NpcHand &actual, const iggy::NpcHand &expected)
{
	if (actual.ents.size() != expected.ents.size())
		return false;
	if (actual.issues.size() != expected.issues.size())
		return false;
	for (std::size_t index = 0; index < actual.ents.size(); ++index) {
		if (!SameHandEnt(actual.ents[index], expected.ents[index]))
			return false;
	}
	for (std::size_t index = 0; index < actual.issues.size(); ++index) {
		if (actual.issues[index].code != expected.issues[index].code
			|| actual.issues[index].source != expected.issues[index].source)
			return false;
	}
	return true;
}

bool SameMapReadEnt(const iggy::NpcMapReadEnt &actual, const iggy::NpcMapReadEnt &expected)
{
	return SameHandEnt(actual.ent, expected.ent)
		&& actual.baseWeight == expected.baseWeight
		&& actual.score == expected.score
		&& actual.handIndex == expected.handIndex
		&& actual.matchedMapTags == expected.matchedMapTags
		&& actual.unmatchedMapTags == expected.unmatchedMapTags
		&& actual.mapMatched == expected.mapMatched;
}

bool SameMapRead(const iggy::NpcMapRead &actual, const iggy::NpcMapRead &expected)
{
	if (!SameHand(actual.hand, expected.hand))
		return false;
	if (actual.issues.size() != expected.issues.size())
		return false;
	if (actual.rankedEnts.size() != expected.rankedEnts.size())
		return false;
	if (actual.map.status != expected.map.status || actual.map.tags != expected.map.tags)
		return false;
	for (std::size_t index = 0; index < actual.issues.size(); ++index) {
		if (actual.issues[index].code != expected.issues[index].code
			|| actual.issues[index].source != expected.issues[index].source)
			return false;
	}
	for (std::size_t index = 0; index < actual.rankedEnts.size(); ++index) {
		if (!SameMapReadEnt(actual.rankedEnts[index], expected.rankedEnts[index]))
			return false;
	}
	return true;
}

void TestEmptyMapReadProjectsToEmptyReadPreservingHandAndIssues()
{
	const iggy::NpcHand hand {
		{},
		{
			{ iggy::NpcHandIssueCode::InvalidDraw, iggy::NpcHandTraitSource::Wisdom },
		},
	};
	iggy::NpcMapRead mapRead;
	mapRead.hand = hand;
	mapRead.issues = hand.issues;

	const iggy::NpcRead read = iggy::NpcMapReadProjection {}.toRead(mapRead);

	Expect(read.rankedEnts.empty(), "empty map read should project no read entries");
	Expect(SameHand(read.hand, hand), "empty map read projection should preserve hand");
	Expect(read.issues.size() == 1, "empty map read projection should preserve issues");
	if (read.issues.size() == 1)
		Expect(read.issues[0].source == iggy::NpcHandTraitSource::Wisdom, "empty map read projection should preserve issue payload");
}

void TestRankedMapEntriesProjectInExactOrder()
{
	const iggy::NpcHandEnt lowBaseWinner =
		HandEnt("strength:map", "action:map", 1.0F, 7, { Id("zone:cover") });
	const iggy::NpcHandEnt highBaseLoser =
		HandEnt("strength:raw", "action:raw", 5.0F, 8, { Id("zone:wrong") });
	iggy::NpcMapRead mapRead;
	mapRead.hand = {
		{
			lowBaseWinner,
			highBaseLoser,
		},
		{},
	};
	mapRead.rankedEnts = {
		MapReadEnt(lowBaseWinner, 1.0F, 6.0F, 0, { Id("zone:cover") }),
		MapReadEnt(highBaseLoser, 5.0F, 5.0F, 1, {}, { Id("zone:wrong") }),
	};

	const iggy::NpcRead read = iggy::NpcMapReadProjection {}.toRead(mapRead);

	Expect(read.rankedEnts.size() == 2, "ranked map read should project two read entries");
	if (read.rankedEnts.size() == 2) {
		Expect(read.rankedEnts[0].ent.entryId == Id("strength:map"), "projection should preserve map-ranked first entry");
		Expect(read.rankedEnts[0].score == 6.0F, "projection should preserve final map-aware score for first entry");
		Expect(read.rankedEnts[0].handIndex == 0, "projection should preserve first hand index");
		Expect(read.rankedEnts[1].ent.entryId == Id("strength:raw"), "projection should preserve map-ranked second entry");
		Expect(read.rankedEnts[1].score == 5.0F, "projection should preserve final map-aware score for second entry");
		Expect(read.rankedEnts[1].handIndex == 1, "projection should preserve second hand index");
	}
}

void TestMapAwareScoreNotBaseWeightIsProjected()
{
	const iggy::NpcHandEnt ent = HandEnt("strength:scored", "action:scored", 2.0F);
	iggy::NpcMapRead mapRead;
	mapRead.hand.ents = { ent };
	mapRead.rankedEnts = {
		MapReadEnt(ent, 2.0F, 9.5F, 0),
	};

	const iggy::NpcRead read = iggy::NpcMapReadProjection {}.toRead(mapRead);

	Expect(read.rankedEnts.size() == 1, "map score projection setup should produce one read ent");
	if (read.rankedEnts.size() == 1)
		Expect(read.rankedEnts[0].score == 9.5F, "projection should use map-aware final score instead of base weight");
}

void TestProjectionDoesNotMutateInput()
{
	iggy::NpcMapRead mapRead;
	mapRead.hand = {
		{
			HandEnt("strength:first", "action:first", 1.0F, 2, { Id("zone:a") }),
			HandEnt("strength:second", "action:second", 2.0F, 3, { Id("zone:b") }),
		},
		{
			{ iggy::NpcHandIssueCode::InvalidDraw, iggy::NpcHandTraitSource::Dexterity },
		},
	};
	mapRead.map.status = iggy::AiMapQuery2DStatus::Matched;
	mapRead.map.tags = { Id("zone:a") };
	mapRead.issues = mapRead.hand.issues;
	mapRead.rankedEnts = {
		MapReadEnt(mapRead.hand.ents[1], 2.0F, 4.0F, 1, {}, { Id("zone:b") }),
		MapReadEnt(mapRead.hand.ents[0], 1.0F, 3.0F, 0, { Id("zone:a") }),
	};
	const iggy::NpcMapRead before = mapRead;

	const iggy::NpcRead read = iggy::NpcMapReadProjection {}.toRead(mapRead);

	Expect(read.rankedEnts.size() == 2, "immutability setup should project two entries");
	Expect(SameMapRead(mapRead, before), "map read projection should not mutate input map read");
}

} // namespace

int main()
{
	TestEmptyMapReadProjectsToEmptyReadPreservingHandAndIssues();
	TestRankedMapEntriesProjectInExactOrder();
	TestMapAwareScoreNotBaseWeightIsProjected();
	TestProjectionDoesNotMutateInput();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
