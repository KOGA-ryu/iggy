#include "scene/ai/NpcHand.hpp"

namespace {

iggy::NpcHandIssue InvalidDrawIssue(iggy::NpcHandTraitSource source)
{
	return {
		iggy::NpcHandIssueCode::InvalidDraw,
		source,
	};
}

void AppendStrength(iggy::NpcHand &hand, const iggy::NpcStrengthDrawResult &draw)
{
	if (draw.status != iggy::NpcStrengthDrawStatus::Drawn) {
		hand.issues.push_back(InvalidDrawIssue(iggy::NpcHandTraitSource::Strength));
		return;
	}
	for (const iggy::NpcStrengthDrawEntry &drawEntry : draw.entries) {
		hand.ents.push_back({
			iggy::NpcHandTraitSource::Strength,
			drawEntry.entry.entryId,
			drawEntry.entry.actionTag,
			drawEntry.entry.behaviorState,
			drawEntry.entry.weight,
			drawEntry.entry.mapTags,
			drawEntry.entryIndex,
		});
	}
}

void AppendDexterity(iggy::NpcHand &hand, const iggy::NpcDexterityDrawResult &draw)
{
	if (draw.status != iggy::NpcDexterityDrawStatus::Drawn) {
		hand.issues.push_back(InvalidDrawIssue(iggy::NpcHandTraitSource::Dexterity));
		return;
	}
	for (const iggy::NpcDexterityDrawEntry &drawEntry : draw.entries) {
		hand.ents.push_back({
			iggy::NpcHandTraitSource::Dexterity,
			drawEntry.entry.entryId,
			drawEntry.entry.actionTag,
			drawEntry.entry.behaviorState,
			drawEntry.entry.weight,
			drawEntry.entry.mapTags,
			drawEntry.entryIndex,
		});
	}
}

void AppendConstitution(iggy::NpcHand &hand, const iggy::NpcConstitutionDrawResult &draw)
{
	if (draw.status != iggy::NpcConstitutionDrawStatus::Drawn) {
		hand.issues.push_back(InvalidDrawIssue(iggy::NpcHandTraitSource::Constitution));
		return;
	}
	for (const iggy::NpcConstitutionDrawEntry &drawEntry : draw.entries) {
		hand.ents.push_back({
			iggy::NpcHandTraitSource::Constitution,
			drawEntry.entry.entryId,
			drawEntry.entry.actionTag,
			drawEntry.entry.behaviorState,
			drawEntry.entry.weight,
			drawEntry.entry.mapTags,
			drawEntry.entryIndex,
		});
	}
}

void AppendIntelligence(iggy::NpcHand &hand, const iggy::NpcIntelligenceDrawResult &draw)
{
	if (draw.status != iggy::NpcIntelligenceDrawStatus::Drawn) {
		hand.issues.push_back(InvalidDrawIssue(iggy::NpcHandTraitSource::Intelligence));
		return;
	}
	for (const iggy::NpcIntelligenceDrawEntry &drawEntry : draw.entries) {
		hand.ents.push_back({
			iggy::NpcHandTraitSource::Intelligence,
			drawEntry.entry.entryId,
			drawEntry.entry.actionTag,
			drawEntry.entry.behaviorState,
			drawEntry.entry.weight,
			drawEntry.entry.mapTags,
			drawEntry.entryIndex,
		});
	}
}

void AppendWisdom(iggy::NpcHand &hand, const iggy::NpcWisdomDrawResult &draw)
{
	if (draw.status != iggy::NpcWisdomDrawStatus::Drawn) {
		hand.issues.push_back(InvalidDrawIssue(iggy::NpcHandTraitSource::Wisdom));
		return;
	}
	for (const iggy::NpcWisdomDrawEntry &drawEntry : draw.entries) {
		hand.ents.push_back({
			iggy::NpcHandTraitSource::Wisdom,
			drawEntry.entry.entryId,
			drawEntry.entry.actionTag,
			drawEntry.entry.behaviorState,
			drawEntry.entry.weight,
			drawEntry.entry.mapTags,
			drawEntry.entryIndex,
		});
	}
}

void AppendCharisma(iggy::NpcHand &hand, const iggy::NpcCharismaDrawResult &draw)
{
	if (draw.status != iggy::NpcCharismaDrawStatus::Drawn) {
		hand.issues.push_back(InvalidDrawIssue(iggy::NpcHandTraitSource::Charisma));
		return;
	}
	for (const iggy::NpcCharismaDrawEntry &drawEntry : draw.entries) {
		hand.ents.push_back({
			iggy::NpcHandTraitSource::Charisma,
			drawEntry.entry.entryId,
			drawEntry.entry.actionTag,
			drawEntry.entry.behaviorState,
			drawEntry.entry.weight,
			drawEntry.entry.mapTags,
			drawEntry.entryIndex,
		});
	}
}

} // namespace

namespace iggy {

bool NpcHand::hasEnts() const
{
	return !ents.empty();
}

bool NpcHand::hasIssues() const
{
	return !issues.empty();
}

NpcHand NpcHandAssembler::assemble(
	const NpcStrengthDrawResult &strength,
	const NpcDexterityDrawResult &dexterity,
	const NpcConstitutionDrawResult &constitution,
	const NpcIntelligenceDrawResult &intelligence,
	const NpcWisdomDrawResult &wisdom,
	const NpcCharismaDrawResult &charisma) const
{
	NpcHand hand;

	AppendStrength(hand, strength);
	AppendDexterity(hand, dexterity);
	AppendConstitution(hand, constitution);
	AppendIntelligence(hand, intelligence);
	AppendWisdom(hand, wisdom);
	AppendCharisma(hand, charisma);

	return hand;
}

} // namespace iggy
