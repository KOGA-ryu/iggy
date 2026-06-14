#include "scene/ai/NpcConstitutionDraw.hpp"

namespace {

bool ConstitutionInRange(std::uint32_t value)
{
	return value <= static_cast<std::uint32_t>(iggy::NpcTraitSetMaxScore);
}

} // namespace

namespace iggy {

bool NpcConstitutionDrawResult::hasEntries() const
{
	return !entries.empty();
}

NpcConstitutionDrawResult NpcConstitutionDraw::draw(
	const NpcConstitutionPool &pool,
	std::uint32_t constitution,
	NpcBehaviorStateType behaviorState) const
{
	NpcConstitutionDrawResult result;
	result.constitution = constitution;
	result.behaviorState = behaviorState;

	if (!ConstitutionInRange(constitution)) {
		result.status = NpcConstitutionDrawStatus::InvalidConstitution;
		return result;
	}

	for (std::size_t index = 0; index < pool.entries.size(); ++index) {
		const NpcConstitutionEnt &entry = pool.entries[index];
		if (entry.minimumConstitution <= constitution && entry.behaviorState == behaviorState)
			result.entries.push_back({ entry, index });
	}

	return result;
}

NpcConstitutionDrawResult NpcConstitutionDraw::draw(
	const NpcConstitutionPool &pool,
	const NpcTraitSet &traits,
	NpcBehaviorStateType behaviorState) const
{
	return draw(pool, static_cast<std::uint32_t>(traits.constitution), behaviorState);
}

} // namespace iggy
