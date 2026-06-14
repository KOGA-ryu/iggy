#include <cstdlib>
#include <vector>

#include "scene/ai/NpcHand.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::NpcStrengthEnt StrengthEnt(
	const char *entryId,
	iggy::NpcBehaviorStateType behaviorState,
	const char *actionTag,
	float weight = 1.0F,
	std::vector<iggy::ResourceId> mapTags = {})
{
	return {
		Id(entryId),
		0,
		behaviorState,
		Id(actionTag),
		weight,
		mapTags,
	};
}

iggy::NpcDexterityEnt DexterityEnt(
	const char *entryId,
	iggy::NpcBehaviorStateType behaviorState,
	const char *actionTag,
	float weight = 1.0F,
	std::vector<iggy::ResourceId> mapTags = {})
{
	return {
		Id(entryId),
		0,
		behaviorState,
		Id(actionTag),
		weight,
		mapTags,
	};
}

iggy::NpcConstitutionEnt ConstitutionEnt(
	const char *entryId,
	iggy::NpcBehaviorStateType behaviorState,
	const char *actionTag,
	float weight = 1.0F,
	std::vector<iggy::ResourceId> mapTags = {})
{
	return {
		Id(entryId),
		0,
		behaviorState,
		Id(actionTag),
		weight,
		mapTags,
	};
}

iggy::NpcIntelligenceEnt IntelligenceEnt(
	const char *entryId,
	iggy::NpcBehaviorStateType behaviorState,
	const char *actionTag,
	float weight = 1.0F,
	std::vector<iggy::ResourceId> mapTags = {})
{
	return {
		Id(entryId),
		0,
		behaviorState,
		Id(actionTag),
		weight,
		mapTags,
	};
}

iggy::NpcWisdomEnt WisdomEnt(
	const char *entryId,
	iggy::NpcBehaviorStateType behaviorState,
	const char *actionTag,
	float weight = 1.0F,
	std::vector<iggy::ResourceId> mapTags = {})
{
	return {
		Id(entryId),
		0,
		behaviorState,
		Id(actionTag),
		weight,
		mapTags,
	};
}

iggy::NpcCharismaEnt CharismaEnt(
	const char *entryId,
	iggy::NpcBehaviorStateType behaviorState,
	const char *actionTag,
	float weight = 1.0F,
	std::vector<iggy::ResourceId> mapTags = {})
{
	return {
		Id(entryId),
		0,
		behaviorState,
		Id(actionTag),
		weight,
		mapTags,
	};
}

iggy::NpcStrengthDrawResult StrengthDraw(std::vector<iggy::NpcStrengthDrawEntry> entries = {})
{
	iggy::NpcStrengthDrawResult result;
	result.strength = 10;
	result.behaviorState = iggy::NpcBehaviorStateType::Seeking;
	result.entries = entries;
	return result;
}

iggy::NpcDexterityDrawResult DexterityDraw(std::vector<iggy::NpcDexterityDrawEntry> entries = {})
{
	iggy::NpcDexterityDrawResult result;
	result.dexterity = 10;
	result.behaviorState = iggy::NpcBehaviorStateType::Seeking;
	result.entries = entries;
	return result;
}

iggy::NpcConstitutionDrawResult ConstitutionDraw(std::vector<iggy::NpcConstitutionDrawEntry> entries = {})
{
	iggy::NpcConstitutionDrawResult result;
	result.constitution = 10;
	result.behaviorState = iggy::NpcBehaviorStateType::Seeking;
	result.entries = entries;
	return result;
}

iggy::NpcIntelligenceDrawResult IntelligenceDraw(std::vector<iggy::NpcIntelligenceDrawEntry> entries = {})
{
	iggy::NpcIntelligenceDrawResult result;
	result.intelligence = 10;
	result.behaviorState = iggy::NpcBehaviorStateType::Seeking;
	result.entries = entries;
	return result;
}

iggy::NpcWisdomDrawResult WisdomDraw(std::vector<iggy::NpcWisdomDrawEntry> entries = {})
{
	iggy::NpcWisdomDrawResult result;
	result.wisdom = 10;
	result.behaviorState = iggy::NpcBehaviorStateType::Seeking;
	result.entries = entries;
	return result;
}

iggy::NpcCharismaDrawResult CharismaDraw(std::vector<iggy::NpcCharismaDrawEntry> entries = {})
{
	iggy::NpcCharismaDrawResult result;
	result.charisma = 10;
	result.behaviorState = iggy::NpcBehaviorStateType::Seeking;
	result.entries = entries;
	return result;
}

bool SameHandEnt(
	const iggy::NpcHandEnt &actual,
	iggy::NpcHandTraitSource source,
	const char *entryId,
	const char *actionTag,
	iggy::NpcBehaviorStateType behaviorState,
	float weight,
	const std::vector<iggy::ResourceId> &mapTags,
	std::size_t drawEntryIndex)
{
	return actual.source == source
		&& actual.entryId == Id(entryId)
		&& actual.actionTag == Id(actionTag)
		&& actual.behaviorState == behaviorState
		&& actual.weight == weight
		&& actual.mapTags == mapTags
		&& actual.drawEntryIndex == drawEntryIndex;
}

bool SameStrengthDrawEntry(
	const iggy::NpcStrengthDrawEntry &actual,
	const iggy::NpcStrengthDrawEntry &expected)
{
	return actual.entry.entryId == expected.entry.entryId
		&& actual.entry.minimumStrength == expected.entry.minimumStrength
		&& actual.entry.behaviorState == expected.entry.behaviorState
		&& actual.entry.actionTag == expected.entry.actionTag
		&& actual.entry.weight == expected.entry.weight
		&& actual.entry.mapTags == expected.entry.mapTags
		&& actual.entryIndex == expected.entryIndex;
}

void TestEmptyValidDrawsAssembleEmptyHand()
{
	const iggy::NpcHand hand = iggy::NpcHandAssembler {}.assemble(
		StrengthDraw(),
		DexterityDraw(),
		ConstitutionDraw(),
		IntelligenceDraw(),
		WisdomDraw(),
		CharismaDraw());

	Expect(hand.ents.empty(), "empty valid draws should assemble no hand ents");
	Expect(!hand.hasEnts(), "empty valid hand should report no ents");
	Expect(hand.issues.empty(), "empty valid draws should assemble no hand issues");
	Expect(!hand.hasIssues(), "empty valid hand should report no issues");
}

void TestOneEntFromEachTraitAssemblesInTraitOrder()
{
	const iggy::NpcHand hand = iggy::NpcHandAssembler {}.assemble(
		StrengthDraw({ { StrengthEnt("strength:one", iggy::NpcBehaviorStateType::Seeking, "action:strength"), 10 } }),
		DexterityDraw({ { DexterityEnt("dexterity:one", iggy::NpcBehaviorStateType::Waiting, "action:dexterity"), 11 } }),
		ConstitutionDraw({ { ConstitutionEnt("constitution:one", iggy::NpcBehaviorStateType::Fleeing, "action:constitution"), 12 } }),
		IntelligenceDraw({ { IntelligenceEnt("intelligence:one", iggy::NpcBehaviorStateType::Interacting, "action:intelligence"), 13 } }),
		WisdomDraw({ { WisdomEnt("wisdom:one", iggy::NpcBehaviorStateType::Attacking, "action:wisdom"), 14 } }),
		CharismaDraw({ { CharismaEnt("charisma:one", iggy::NpcBehaviorStateType::Idle, "action:charisma"), 15 } }));

	Expect(hand.ents.size() == 6, "one ent from each trait should assemble six hand ents");
	Expect(hand.issues.empty(), "valid trait draws should assemble without issues");
	if (hand.ents.size() == 6) {
		Expect(hand.ents[0].source == iggy::NpcHandTraitSource::Strength, "first hand ent should be Strength");
		Expect(hand.ents[1].source == iggy::NpcHandTraitSource::Dexterity, "second hand ent should be Dexterity");
		Expect(hand.ents[2].source == iggy::NpcHandTraitSource::Constitution, "third hand ent should be Constitution");
		Expect(hand.ents[3].source == iggy::NpcHandTraitSource::Intelligence, "fourth hand ent should be Intelligence");
		Expect(hand.ents[4].source == iggy::NpcHandTraitSource::Wisdom, "fifth hand ent should be Wisdom");
		Expect(hand.ents[5].source == iggy::NpcHandTraitSource::Charisma, "sixth hand ent should be Charisma");
	}
}

void TestWithinTraitOrderAndIndexesArePreserved()
{
	const iggy::NpcHand hand = iggy::NpcHandAssembler {}.assemble(
		StrengthDraw({
			{ StrengthEnt("strength:first", iggy::NpcBehaviorStateType::Seeking, "action:first"), 2 },
			{ StrengthEnt("strength:second", iggy::NpcBehaviorStateType::Seeking, "action:second"), 4 },
		}),
		DexterityDraw(),
		ConstitutionDraw(),
		IntelligenceDraw(),
		WisdomDraw({
			{ WisdomEnt("wisdom:first", iggy::NpcBehaviorStateType::Waiting, "action:wise-first"), 1 },
			{ WisdomEnt("wisdom:second", iggy::NpcBehaviorStateType::Waiting, "action:wise-second"), 3 },
		}),
		CharismaDraw());

	Expect(hand.ents.size() == 4, "multiple ents per trait should be assembled");
	if (hand.ents.size() == 4) {
		Expect(SameHandEnt(hand.ents[0], iggy::NpcHandTraitSource::Strength, "strength:first", "action:first", iggy::NpcBehaviorStateType::Seeking, 1.0F, {}, 2), "first Strength ent should preserve order/index");
		Expect(SameHandEnt(hand.ents[1], iggy::NpcHandTraitSource::Strength, "strength:second", "action:second", iggy::NpcBehaviorStateType::Seeking, 1.0F, {}, 4), "second Strength ent should preserve order/index");
		Expect(SameHandEnt(hand.ents[2], iggy::NpcHandTraitSource::Wisdom, "wisdom:first", "action:wise-first", iggy::NpcBehaviorStateType::Waiting, 1.0F, {}, 1), "first Wisdom ent should preserve order/index");
		Expect(SameHandEnt(hand.ents[3], iggy::NpcHandTraitSource::Wisdom, "wisdom:second", "action:wise-second", iggy::NpcBehaviorStateType::Waiting, 1.0F, {}, 3), "second Wisdom ent should preserve order/index");
	}
}

void TestInvalidDrawRecordsIssueAndSkipsTrait()
{
	iggy::NpcDexterityDrawResult invalidDexterity = DexterityDraw({
		{ DexterityEnt("dexterity:skip", iggy::NpcBehaviorStateType::Seeking, "action:skip"), 0 },
	});
	invalidDexterity.status = iggy::NpcDexterityDrawStatus::InvalidDexterity;
	iggy::NpcWisdomDrawResult invalidWisdom = WisdomDraw({
		{ WisdomEnt("wisdom:skip", iggy::NpcBehaviorStateType::Seeking, "action:skip"), 0 },
	});
	invalidWisdom.status = iggy::NpcWisdomDrawStatus::InvalidWisdom;

	const iggy::NpcHand hand = iggy::NpcHandAssembler {}.assemble(
		StrengthDraw({ { StrengthEnt("strength:keep", iggy::NpcBehaviorStateType::Seeking, "action:keep"), 0 } }),
		invalidDexterity,
		ConstitutionDraw(),
		IntelligenceDraw(),
		invalidWisdom,
		CharismaDraw({ { CharismaEnt("charisma:keep", iggy::NpcBehaviorStateType::Seeking, "action:keep"), 0 } }));

	Expect(hand.ents.size() == 2, "invalid draws should be skipped while valid traits are preserved");
	if (hand.ents.size() == 2) {
		Expect(hand.ents[0].source == iggy::NpcHandTraitSource::Strength, "first preserved hand ent should be Strength");
		Expect(hand.ents[1].source == iggy::NpcHandTraitSource::Charisma, "second preserved hand ent should be Charisma");
	}
	Expect(hand.issues.size() == 2, "invalid draws should record issues");
	Expect(hand.hasIssues(), "invalid draw hand should report issues");
	if (hand.issues.size() == 2) {
		Expect(hand.issues[0].code == iggy::NpcHandIssueCode::InvalidDraw && hand.issues[0].source == iggy::NpcHandTraitSource::Dexterity, "first invalid draw issue should preserve Dexterity source");
		Expect(hand.issues[1].code == iggy::NpcHandIssueCode::InvalidDraw && hand.issues[1].source == iggy::NpcHandTraitSource::Wisdom, "second invalid draw issue should preserve Wisdom source");
	}
}

void TestNormalizedFieldsArePreserved()
{
	const std::vector<iggy::ResourceId> tags { Id("tag"), Id("tag:map") };
	const iggy::NpcHand hand = iggy::NpcHandAssembler {}.assemble(
		StrengthDraw(),
		DexterityDraw(),
		ConstitutionDraw(),
		IntelligenceDraw({ { IntelligenceEnt("intelligence:read", iggy::NpcBehaviorStateType::Interacting, "action:read", 2.5F, tags), 7 } }),
		WisdomDraw(),
		CharismaDraw());

	Expect(hand.ents.size() == 1, "normalized field setup should assemble one hand ent");
	if (hand.ents.size() == 1) {
		Expect(SameHandEnt(hand.ents[0], iggy::NpcHandTraitSource::Intelligence, "intelligence:read", "action:read", iggy::NpcBehaviorStateType::Interacting, 2.5F, tags, 7), "hand ent should preserve normalized fields exactly");
	}
}

void TestZeroWeightEntsAreIncluded()
{
	const iggy::NpcHand hand = iggy::NpcHandAssembler {}.assemble(
		StrengthDraw(),
		DexterityDraw(),
		ConstitutionDraw({ { ConstitutionEnt("constitution:inert", iggy::NpcBehaviorStateType::Waiting, "action:hold", 0.0F), 5 } }),
		IntelligenceDraw(),
		WisdomDraw(),
		CharismaDraw());

	Expect(hand.ents.size() == 1, "zero-weight draw entry should still assemble into the hand");
	Expect(hand.hasEnts(), "zero-weight hand should report ents");
	if (hand.ents.size() == 1)
		Expect(hand.ents[0].weight == 0.0F, "zero-weight hand ent should preserve zero weight");
}

void TestAssemblerDoesNotMutateInputs()
{
	iggy::NpcStrengthDrawResult strength = StrengthDraw({
		{ StrengthEnt("strength:copy", iggy::NpcBehaviorStateType::Seeking, "action:copy", 1.25F, { Id("tag:copy") }), 9 },
	});
	const iggy::NpcStrengthDrawResult before = strength;

	const iggy::NpcHand hand = iggy::NpcHandAssembler {}.assemble(
		strength,
		DexterityDraw(),
		ConstitutionDraw(),
		IntelligenceDraw(),
		WisdomDraw(),
		CharismaDraw());

	Expect(hand.ents.size() == 1, "immutability setup should assemble one ent");
	Expect(strength.status == before.status, "hand assembler should not mutate draw status");
	Expect(strength.strength == before.strength, "hand assembler should not mutate requested trait score");
	Expect(strength.behaviorState == before.behaviorState, "hand assembler should not mutate requested behavior state");
	Expect(strength.entries.size() == before.entries.size(), "hand assembler should not mutate draw entry count");
	if (strength.entries.size() == before.entries.size() && !strength.entries.empty())
		Expect(SameStrengthDrawEntry(strength.entries[0], before.entries[0]), "hand assembler should not mutate draw entry payload");
}

} // namespace

int main()
{
	TestEmptyValidDrawsAssembleEmptyHand();
	TestOneEntFromEachTraitAssemblesInTraitOrder();
	TestWithinTraitOrderAndIndexesArePreserved();
	TestInvalidDrawRecordsIssueAndSkipsTrait();
	TestNormalizedFieldsArePreserved();
	TestZeroWeightEntsAreIncluded();
	TestAssemblerDoesNotMutateInputs();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
