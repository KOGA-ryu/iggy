#include <cstdlib>
#include <vector>

#include "scene/ai/NpcMapRead.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::NpcHandEnt Ent(
	const char *entryId,
	const char *actionTag,
	float weight,
	std::vector<iggy::ResourceId> mapTags = {},
	iggy::NpcHandTraitSource source = iggy::NpcHandTraitSource::Strength,
	std::size_t drawEntryIndex = 0)
{
	return {
		source,
		Id(entryId),
		Id(actionTag),
		iggy::NpcBehaviorStateType::Seeking,
		weight,
		mapTags,
		drawEntryIndex,
	};
}

iggy::NpcHand Hand(
	std::vector<iggy::NpcHandEnt> ents = {},
	std::vector<iggy::NpcHandIssue> issues = {})
{
	return {
		ents,
		issues,
	};
}

iggy::AiMapQuery2DResult Map(std::vector<iggy::ResourceId> tags = {})
{
	iggy::AiMapQuery2DResult map;
	map.status = tags.empty() ? iggy::AiMapQuery2DStatus::NoMatch : iggy::AiMapQuery2DStatus::Matched;
	map.tags = tags;
	map.patrolWeight = 1.0F;
	map.coverWeight = 2.0F;
	map.dangerWeight = 3.0F;
	map.interestWeight = 4.0F;
	return map;
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

void TestEmptyHandAndNoMatchMapProduceEmptyRead()
{
	const iggy::NpcHand hand;
	const iggy::AiMapQuery2DResult map;

	const iggy::NpcMapRead read = iggy::NpcMapReader {}.read(hand, map);

	Expect(read.rankedEnts.empty(), "empty hand should map-read no ranked ents");
	Expect(!read.hasRankedEnts(), "empty map read should report no ranked ents");
	Expect(!read.hasIssues(), "empty map read should report no issues");
	Expect(SameHand(read.hand, hand), "empty map read should copy hand");
	Expect(read.map.status == map.status && read.map.tags.empty(), "empty map read should copy map query");
}

void TestUntaggedEntIsMapNeutral()
{
	const iggy::NpcHand hand = Hand({
		Ent("strength:wait", "action:wait", 2.0F),
	});

	const iggy::NpcMapRead read = iggy::NpcMapReader {}.read(hand, Map({ Id("zone:cover") }), { 0.0F, 5.0F, 5.0F, true });

	Expect(read.rankedEnts.size() == 1, "untagged ent should remain eligible when map-tag match is required");
	if (read.rankedEnts.size() == 1) {
		Expect(read.rankedEnts[0].score == 2.0F, "untagged ent should score by base weight");
		Expect(read.rankedEnts[0].baseWeight == 2.0F, "untagged ent should preserve base weight");
		Expect(!read.rankedEnts[0].mapMatched, "untagged ent should not report map match");
		Expect(read.rankedEnts[0].matchedMapTags.empty(), "untagged ent should have no matched tags");
		Expect(read.rankedEnts[0].unmatchedMapTags.empty(), "untagged ent should have no unmatched tags");
	}
}

void TestMatchingMapTagBoostsScoreAndCanWin()
{
	const iggy::NpcHand hand = Hand({
		Ent("strength:heavy", "action:heavy", 4.0F, { Id("zone:wrong") }),
		Ent("wisdom:cover", "action:cover", 3.0F, { Id("zone:cover") }, iggy::NpcHandTraitSource::Wisdom),
	});

	const iggy::NpcMapRead read = iggy::NpcMapReader {}.read(hand, Map({ Id("zone:cover") }), { 0.0F, 2.0F, 0.0F, false });

	Expect(read.rankedEnts.size() == 2, "map tag boost setup should rank two ents");
	if (read.rankedEnts.size() == 2) {
		Expect(read.rankedEnts[0].ent.entryId == Id("wisdom:cover"), "matching lower base weight ent should win after bonus");
		Expect(read.rankedEnts[0].score == 5.0F, "matching ent should score base plus bonus");
		Expect(read.rankedEnts[0].matchedMapTags == std::vector<iggy::ResourceId> { Id("zone:cover") }, "matching ent should preserve matched tag");
		Expect(read.rankedEnts[0].mapMatched, "matching ent should report map match");
	}
}

void TestUnmatchedMapTagsApplyConfiguredPenalty()
{
	const iggy::NpcHand hand = Hand({
		Ent("strength:mixed", "action:mixed", 5.0F, { Id("zone:cover"), Id("zone:danger") }),
	});

	const iggy::NpcMapRead read = iggy::NpcMapReader {}.read(hand, Map({ Id("zone:cover") }), { 0.0F, 1.0F, 0.5F, false });

	Expect(read.rankedEnts.size() == 1, "penalty setup should keep one ent");
	if (read.rankedEnts.size() == 1) {
		Expect(read.rankedEnts[0].score == 5.5F, "score should include one bonus and one penalty");
		Expect(read.rankedEnts[0].matchedMapTags == std::vector<iggy::ResourceId> { Id("zone:cover") }, "matched tags should preserve order");
		Expect(read.rankedEnts[0].unmatchedMapTags == std::vector<iggy::ResourceId> { Id("zone:danger") }, "unmatched tags should preserve order");
	}
}

void TestRequireMapTagMatchFiltersTaggedNonMatchesOnly()
{
	const iggy::NpcHand hand = Hand({
		Ent("strength:neutral", "action:neutral", 1.0F),
		Ent("strength:miss", "action:miss", 3.0F, { Id("zone:missing") }),
		Ent("strength:hit", "action:hit", 2.0F, { Id("zone:cover") }),
	});

	const iggy::NpcMapRead read = iggy::NpcMapReader {}.read(hand, Map({ Id("zone:cover") }), { 0.0F, 1.0F, 0.0F, true });

	Expect(read.rankedEnts.size() == 2, "require map tag should keep neutral and matching ents");
	if (read.rankedEnts.size() == 2) {
		Expect(read.rankedEnts[0].ent.entryId == Id("strength:hit"), "matching tagged ent should rank first after bonus");
		Expect(read.rankedEnts[1].ent.entryId == Id("strength:neutral"), "neutral untagged ent should remain eligible");
	}
}

void TestMultipleMatchedTagsAccumulateAndDedupeInEntOrder()
{
	const iggy::NpcHand hand = Hand({
		Ent("strength:multi", "action:multi", 1.0F, { Id("zone:b"), Id("zone:a"), Id("zone:b"), Id("zone:c"), Id("zone:c") }),
	});

	const iggy::NpcMapRead read = iggy::NpcMapReader {}.read(hand, Map({ Id("zone:a"), Id("zone:b") }), { 0.0F, 1.5F, 0.25F, false });

	Expect(read.rankedEnts.size() == 1, "multi tag setup should rank one ent");
	if (read.rankedEnts.size() == 1) {
		Expect(read.rankedEnts[0].matchedMapTags == std::vector<iggy::ResourceId>({ Id("zone:b"), Id("zone:a") }), "matched tags should dedupe in ent order");
		Expect(read.rankedEnts[0].unmatchedMapTags == std::vector<iggy::ResourceId>({ Id("zone:c") }), "unmatched tags should dedupe in ent order");
		Expect(read.rankedEnts[0].score == 3.75F, "deduped matched and unmatched tags should determine score");
	}
}

void TestNamespacedAndUnqualifiedTagsRemainDistinct()
{
	const iggy::NpcHand hand = Hand({
		Ent("strength:plain", "action:plain", 1.0F, { Id("cover") }),
		Ent("strength:namespaced", "action:namespaced", 1.0F, { Id("zone:cover") }),
	});

	const iggy::NpcMapRead read = iggy::NpcMapReader {}.read(hand, Map({ Id("zone:cover") }), { 0.0F, 1.0F, 0.0F, false });

	Expect(read.rankedEnts.size() == 2, "distinct tag setup should rank two ents");
	if (read.rankedEnts.size() == 2) {
		Expect(read.rankedEnts[0].ent.entryId == Id("strength:namespaced"), "namespaced matching tag should receive bonus");
		Expect(read.rankedEnts[0].matchedMapTags == std::vector<iggy::ResourceId> { Id("zone:cover") }, "namespaced tag should match exactly");
		Expect(read.rankedEnts[1].ent.entryId == Id("strength:plain"), "unqualified tag should remain distinct");
		Expect(read.rankedEnts[1].matchedMapTags.empty(), "unqualified tag should not match namespaced map tag");
	}
}

void TestStableTieOrderingByOriginalHandOrder()
{
	const iggy::NpcHand hand = Hand({
		Ent("strength:first", "action:first", 2.0F, { Id("zone:cover") }),
		Ent("wisdom:second", "action:second", 2.0F, { Id("zone:cover") }, iggy::NpcHandTraitSource::Wisdom),
	});

	const iggy::NpcMapRead read = iggy::NpcMapReader {}.read(hand, Map({ Id("zone:cover") }));

	Expect(read.rankedEnts.size() == 2, "tie setup should rank two ents");
	if (read.rankedEnts.size() == 2) {
		Expect(read.rankedEnts[0].handIndex == 0, "first tied map-read ent should stay first");
		Expect(read.rankedEnts[1].handIndex == 1, "second tied map-read ent should stay second");
	}
}

void TestMinimumScoreAppliesAfterMapScore()
{
	const iggy::NpcHand hand = Hand({
		Ent("strength:filtered", "action:filtered", 1.0F, { Id("zone:missing") }),
		Ent("strength:kept", "action:kept", 1.0F, { Id("zone:cover") }),
	});

	const iggy::NpcMapRead read = iggy::NpcMapReader {}.read(hand, Map({ Id("zone:cover") }), { 2.0F, 1.0F, 0.0F, false });

	Expect(read.rankedEnts.size() == 1, "minimum score should apply after map score");
	if (read.rankedEnts.size() == 1)
		Expect(read.rankedEnts[0].ent.entryId == Id("strength:kept"), "boosted ent should meet minimum score");
}

void TestHandIssuesArePreserved()
{
	const iggy::NpcHand hand = Hand(
		{
			Ent("strength:one", "action:one", 1.0F),
		},
		{
			{ iggy::NpcHandIssueCode::InvalidDraw, iggy::NpcHandTraitSource::Charisma },
		});

	const iggy::NpcMapRead read = iggy::NpcMapReader {}.read(hand, Map({ Id("zone:cover") }));

	Expect(read.hand.issues.size() == 1, "map read should copy hand issues into copied hand");
	Expect(read.issues.size() == 1, "map read should preserve hand issues directly");
	Expect(read.hasIssues(), "map read should report preserved issues");
	if (read.issues.size() == 1)
		Expect(read.issues[0].source == iggy::NpcHandTraitSource::Charisma, "map read should preserve issue source");
}

void TestMapReadDoesNotMutateInputs()
{
	iggy::NpcHand hand = Hand({
		Ent("strength:one", "action:one", 1.0F, { Id("zone:cover") }, iggy::NpcHandTraitSource::Strength, 5),
		Ent("wisdom:two", "action:two", 2.0F, { Id("zone:interest") }, iggy::NpcHandTraitSource::Wisdom, 6),
	});
	iggy::AiMapQuery2DResult map = Map({ Id("zone:cover") });
	const iggy::NpcHand handBefore = hand;
	const iggy::AiMapQuery2DResult mapBefore = map;

	const iggy::NpcMapRead read = iggy::NpcMapReader {}.read(hand, map, { 0.0F, 2.0F, 1.0F, false });

	Expect(read.rankedEnts.size() == 2, "immutability setup should rank two ents");
	Expect(SameHand(hand, handBefore), "map read should not mutate input hand");
	Expect(map.tags == mapBefore.tags && map.status == mapBefore.status, "map read should not mutate input map");
	Expect(SameHand(read.hand, handBefore), "map read should copy hand by value");
	Expect(read.map.tags == mapBefore.tags && read.map.status == mapBefore.status, "map read should copy map by value");
}

} // namespace

int main()
{
	TestEmptyHandAndNoMatchMapProduceEmptyRead();
	TestUntaggedEntIsMapNeutral();
	TestMatchingMapTagBoostsScoreAndCanWin();
	TestUnmatchedMapTagsApplyConfiguredPenalty();
	TestRequireMapTagMatchFiltersTaggedNonMatchesOnly();
	TestMultipleMatchedTagsAccumulateAndDedupeInEntOrder();
	TestNamespacedAndUnqualifiedTagsRemainDistinct();
	TestStableTieOrderingByOriginalHandOrder();
	TestMinimumScoreAppliesAfterMapScore();
	TestHandIssuesArePreserved();
	TestMapReadDoesNotMutateInputs();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
