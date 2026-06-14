#include <cstdlib>
#include <vector>

#include "scene/ai/NpcMapPlayReport.hpp"
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
	iggy::NpcBehaviorStateType behaviorState,
	float weight,
	std::vector<iggy::ResourceId> mapTags = {},
	iggy::NpcHandTraitSource source = iggy::NpcHandTraitSource::Strength,
	std::size_t drawEntryIndex = 0)
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

void TestEmptyHandProducesFoldedNoPlayReport()
{
	const iggy::NpcHand hand;
	const iggy::AiMapQuery2DResult map;

	const iggy::NpcMapPlayReport report = iggy::NpcMapPlayReporter {}.report(hand, map);

	Expect(SameHand(report.hand, hand), "empty report should copy input hand");
	Expect(report.map.status == map.status && report.map.tags.empty(), "empty report should copy input map");
	Expect(report.rawRankedCount == 0 && report.mapRankedCount == 0, "empty report should have no ranked entries");
	Expect(!report.hasRawSelection, "empty report should have no raw selection");
	Expect(!report.hasMapSelection, "empty report should have no map selection");
	Expect(!report.mapChangedSelection, "empty report should not report changed selection");
	Expect(report.play.status == iggy::NpcPlayStatus::NoPlayableEnt, "empty report should preserve no-play status");
	Expect(report.folded(), "empty report should fold final result");
	Expect(report.fold.reason == iggy::NpcFoldReason::NoPlayableEnt, "empty report should fold because there is no playable ent");
}

void TestRawAndMapReadsAgreeWithoutMapEffect()
{
	const iggy::NpcHand hand = Hand({
		Ent("strength:hold", "action:hold", iggy::NpcBehaviorStateType::Waiting, 4.0F, { Id("zone:cover") }),
		Ent("wisdom:push", "action:push", iggy::NpcBehaviorStateType::Seeking, 2.0F, { Id("zone:cover") }, iggy::NpcHandTraitSource::Wisdom),
	});

	const iggy::NpcMapPlayReport report =
		iggy::NpcMapPlayReporter {}.report(hand, Map({ Id("zone:cover") }), {}, { 0.0F, 0.0F, 0.0F, false });

	Expect(report.rawRankedCount == 2 && report.mapRankedCount == 2, "agreeing report should rank both paths");
	Expect(report.hasRawSelection && report.hasMapSelection, "agreeing report should preserve both selections");
	Expect(report.rawSelectedActionTag == Id("action:hold"), "raw selection should preserve winner action");
	Expect(report.mapSelectedActionTag == Id("action:hold"), "map selection should preserve same winner action");
	Expect(report.rawSelectedBehaviorState == iggy::NpcBehaviorStateType::Waiting, "raw selection should preserve behavior");
	Expect(report.mapSelectedBehaviorState == iggy::NpcBehaviorStateType::Waiting, "map selection should preserve behavior");
	Expect(!report.mapChangedSelection, "zero map effect should not change selection");
	Expect(report.kept(), "agreeing report should keep playable ent");
}

void TestMapBonusChangesSelection()
{
	const iggy::NpcHand hand = Hand({
		Ent("strength:map", "action:map", iggy::NpcBehaviorStateType::Seeking, 2.0F, { Id("zone:cover") }),
		Ent("dexterity:raw", "action:raw", iggy::NpcBehaviorStateType::Idle, 5.0F, { Id("zone:loud") }, iggy::NpcHandTraitSource::Dexterity),
	});

	const iggy::NpcMapPlayReport report =
		iggy::NpcMapPlayReporter {}.report(hand, Map({ Id("zone:cover") }), {}, { 0.0F, 4.0F, 0.0F, false });

	Expect(report.rawRead.rankedEnts.size() == 2, "changed-selection setup should rank raw entries");
	Expect(report.rawRead.rankedEnts[0].ent.entryId == Id("dexterity:raw"), "raw read should pick higher base weight");
	Expect(report.mapRead.rankedEnts.size() == 2, "changed-selection setup should rank map entries");
	Expect(report.mapRead.rankedEnts[0].ent.entryId == Id("strength:map"), "map read should pick matching lower base weight");
	Expect(report.mapRead.rankedEnts[0].baseWeight == 2.0F, "map read should preserve base weight");
	Expect(report.mapRead.rankedEnts[0].score == 6.0F, "map read should preserve boosted score");
	Expect(report.mapRead.rankedEnts[0].matchedMapTags == std::vector<iggy::ResourceId> { Id("zone:cover") }, "map read should preserve matched tags");
	Expect(report.rawSelectedActionTag == Id("action:raw"), "summary should preserve raw selected action");
	Expect(report.mapSelectedActionTag == Id("action:map"), "summary should preserve map selected action");
	Expect(report.rawSelectedBehaviorState == iggy::NpcBehaviorStateType::Idle, "summary should preserve raw selected behavior");
	Expect(report.mapSelectedBehaviorState == iggy::NpcBehaviorStateType::Seeking, "summary should preserve map selected behavior");
	Expect(report.mapChangedSelection, "map bonus should report changed selection");
}

void TestMapProjectionScoreAndOrderDrivePlay()
{
	const iggy::NpcHand hand = Hand({
		Ent("strength:map", "action:map", iggy::NpcBehaviorStateType::Seeking, 1.0F, { Id("zone:cover") }),
		Ent("dexterity:raw", "action:raw", iggy::NpcBehaviorStateType::Idle, 4.0F, { Id("zone:loud") }, iggy::NpcHandTraitSource::Dexterity),
	});

	const iggy::NpcMapPlayReport report =
		iggy::NpcMapPlayReporter {}.report(hand, Map({ Id("zone:cover") }), {}, { 0.0F, 5.0F, 0.0F, false });

	Expect(report.projectedMapRead.rankedEnts.size() == report.mapRead.rankedEnts.size(), "projected map read should preserve ranked count");
	Expect(report.projectedMapRead.rankedEnts.size() == 2, "projection setup should produce two projected entries");
	if (report.projectedMapRead.rankedEnts.size() == 2) {
		Expect(report.projectedMapRead.rankedEnts[0].ent.entryId == report.mapRead.rankedEnts[0].ent.entryId, "projected read should preserve map rank order");
		Expect(report.projectedMapRead.rankedEnts[0].score == report.mapRead.rankedEnts[0].score, "projected read should preserve map final score");
		Expect(report.projectedMapRead.rankedEnts[0].score == 6.0F, "projected read should use map score");
	}
	Expect(report.play.hasPlay(), "projected map read should feed play");
	Expect(report.play.selected.ent.entryId == report.projectedMapRead.rankedEnts[0].ent.entryId, "play should consume projected map order");
	Expect(report.play.selected.score == report.projectedMapRead.rankedEnts[0].score, "play should consume projected map score");
}

void TestFoldConfigCanFoldMapSelectedPlay()
{
	const iggy::NpcHand hand = Hand({
		Ent("strength:low", "action:low", iggy::NpcBehaviorStateType::Seeking, 1.0F, { Id("zone:cover") }),
	});

	const iggy::NpcMapPlayReport report =
		iggy::NpcMapPlayReporter {}.report(hand, Map({ Id("zone:cover") }), {}, { 0.0F, 1.0F, 0.0F, false }, { 3.0F });

	Expect(report.hasMapSelection, "fold threshold setup should still select map play");
	Expect(report.mapSelectedActionTag == Id("action:low"), "fold threshold setup should preserve selected action");
	Expect(report.folded(), "fold threshold should fold map-selected play");
	Expect(report.fold.reason == iggy::NpcFoldReason::BelowMinimumScore, "fold threshold should preserve fold reason");
}

void TestHandIssuesPreservedThroughReport()
{
	const iggy::NpcHand hand = Hand(
		{
			Ent("strength:one", "action:one", iggy::NpcBehaviorStateType::Seeking, 1.0F),
		},
		{
			{ iggy::NpcHandIssueCode::InvalidDraw, iggy::NpcHandTraitSource::Charisma },
		});

	const iggy::NpcMapPlayReport report = iggy::NpcMapPlayReporter {}.report(hand, Map({ Id("zone:cover") }));

	Expect(report.rawRead.issues.size() == 1, "raw read should preserve hand issues");
	Expect(report.mapRead.issues.size() == 1, "map read should preserve hand issues");
	Expect(report.projectedMapRead.issues.size() == 1, "projected map read should preserve hand issues");
	Expect(report.tell.issueCount == 1, "tell should preserve issue count");
	if (report.projectedMapRead.issues.size() == 1)
		Expect(report.projectedMapRead.issues[0].source == iggy::NpcHandTraitSource::Charisma, "projected issue should preserve source");
}

void TestMapQueryPreservedByValue()
{
	const iggy::NpcHand hand = Hand({
		Ent("strength:one", "action:one", iggy::NpcBehaviorStateType::Seeking, 1.0F, { Id("zone:cover") }),
	});
	iggy::AiMapQuery2DResult map = Map({ Id("zone:cover"), Id("zone:quiet") });
	map.status = iggy::AiMapQuery2DStatus::Matched;
	map.position = { 3.0F, 4.0F };

	const iggy::NpcMapPlayReport report = iggy::NpcMapPlayReporter {}.report(hand, map);

	Expect(report.map.status == map.status, "report should copy map status");
	Expect(report.map.tags == map.tags, "report should copy map tags");
	Expect(report.map.position == map.position, "report should copy map position");
	Expect(report.map.patrolWeight == 1.0F && report.map.coverWeight == 2.0F, "report should copy map weights");
}

void TestReportDoesNotMutateInputs()
{
	iggy::NpcHand hand = Hand({
		Ent("strength:one", "action:one", iggy::NpcBehaviorStateType::Seeking, 1.0F, { Id("zone:cover") }, iggy::NpcHandTraitSource::Strength, 3),
		Ent("wisdom:two", "action:two", iggy::NpcBehaviorStateType::Waiting, 2.0F, { Id("zone:quiet") }, iggy::NpcHandTraitSource::Wisdom, 4),
	});
	iggy::AiMapQuery2DResult map = Map({ Id("zone:cover") });
	const iggy::NpcHand handBefore = hand;
	const iggy::AiMapQuery2DResult mapBefore = map;

	const iggy::NpcMapPlayReport report = iggy::NpcMapPlayReporter {}.report(hand, map, {}, { 0.0F, 1.0F, 0.5F, false });

	Expect(report.rawRankedCount == 2 && report.mapRankedCount == 2, "immutability setup should rank both entries");
	Expect(SameHand(hand, handBefore), "map play report should not mutate input hand");
	Expect(map.status == mapBefore.status && map.tags == mapBefore.tags && map.position == mapBefore.position, "map play report should not mutate input map query");
	Expect(SameHand(report.hand, handBefore), "map play report should copy hand by value");
	Expect(report.map.tags == mapBefore.tags, "map play report should copy map query by value");
}

} // namespace

int main()
{
	TestEmptyHandProducesFoldedNoPlayReport();
	TestRawAndMapReadsAgreeWithoutMapEffect();
	TestMapBonusChangesSelection();
	TestMapProjectionScoreAndOrderDrivePlay();
	TestFoldConfigCanFoldMapSelectedPlay();
	TestHandIssuesPreservedThroughReport();
	TestMapQueryPreservedByValue();
	TestReportDoesNotMutateInputs();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
