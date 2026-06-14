#include "scene/ai/NpcStrengthDraw.hpp"

namespace {

bool StrengthInRange(std::uint32_t value)
{
	return value <= static_cast<std::uint32_t>(iggy::NpcTraitSetMaxScore);
}

} // namespace

namespace iggy {

bool NpcStrengthDrawResult::hasEntries() const
{
	return !entries.empty();
}

NpcStrengthDrawResult NpcStrengthDraw::draw(
	const NpcStrengthPool &pool,
	std::uint32_t strength,
	NpcBehaviorStateType behaviorState) const
{
	NpcStrengthDrawResult result;
	result.strength = strength;
	result.behaviorState = behaviorState;

	if (!StrengthInRange(strength)) {
		result.status = NpcStrengthDrawStatus::InvalidStrength;
		return result;
	}

	for (std::size_t index = 0; index < pool.entries.size(); ++index) {
		const NpcStrengthEnt &entry = pool.entries[index];
		if (entry.minimumStrength <= strength && entry.behaviorState == behaviorState)
			result.entries.push_back({ entry, index });
	}

	return result;
}

NpcStrengthDrawResult NpcStrengthDraw::draw(
	const NpcStrengthPool &pool,
	const NpcTraitSet &traits,
	NpcBehaviorStateType behaviorState) const
{
	return draw(pool, static_cast<std::uint32_t>(traits.strength), behaviorState);
}

} // namespace iggy
