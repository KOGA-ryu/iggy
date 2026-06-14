#include "scene/ai/NpcStrengthStoreQuery2D.hpp"

namespace {

bool StrengthInRange(std::uint32_t value)
{
	return value <= static_cast<std::uint32_t>(iggy::NpcTraitSet2DMaxScore);
}

} // namespace

namespace iggy {

bool NpcStrengthStoreQuery2DResult::hasEntries() const
{
	return !entries.empty();
}

NpcStrengthStoreQuery2DResult NpcStrengthStoreQuery2D::query(
	const NpcStrengthStore2D &store,
	std::uint32_t strength,
	NpcBehaviorState2DType behaviorState) const
{
	NpcStrengthStoreQuery2DResult result;
	result.strength = strength;
	result.behaviorState = behaviorState;

	if (!StrengthInRange(strength)) {
		result.status = NpcStrengthStoreQuery2DStatus::InvalidStrength;
		return result;
	}

	for (std::size_t index = 0; index < store.entries.size(); ++index) {
		const NpcStrengthBehaviorEntry2D &entry = store.entries[index];
		if (entry.minimumStrength <= strength && entry.behaviorState == behaviorState)
			result.entries.push_back({ entry, index });
	}

	return result;
}

NpcStrengthStoreQuery2DResult NpcStrengthStoreQuery2D::query(
	const NpcStrengthStore2D &store,
	const NpcTraitSet2D &traits,
	NpcBehaviorState2DType behaviorState) const
{
	return query(store, static_cast<std::uint32_t>(traits.strength), behaviorState);
}

} // namespace iggy
