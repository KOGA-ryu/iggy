#include "scene/ai/NpcDexterityDraw.hpp"

namespace {

bool DexterityInRange(std::uint32_t value)
{
	return value <= static_cast<std::uint32_t>(iggy::NpcTraitSetMaxScore);
}

} // namespace

namespace iggy {

bool NpcDexterityDrawResult::hasEntries() const
{
	return !entries.empty();
}

NpcDexterityDrawResult NpcDexterityDraw::draw(
	const NpcDexterityPool &pool,
	std::uint32_t dexterity,
	NpcBehaviorStateType behaviorState) const
{
	NpcDexterityDrawResult result;
	result.dexterity = dexterity;
	result.behaviorState = behaviorState;

	if (!DexterityInRange(dexterity)) {
		result.status = NpcDexterityDrawStatus::InvalidDexterity;
		return result;
	}

	for (std::size_t index = 0; index < pool.entries.size(); ++index) {
		const NpcDexterityEnt &entry = pool.entries[index];
		if (entry.minimumDexterity <= dexterity && entry.behaviorState == behaviorState)
			result.entries.push_back({ entry, index });
	}

	return result;
}

NpcDexterityDrawResult NpcDexterityDraw::draw(
	const NpcDexterityPool &pool,
	const NpcTraitSet &traits,
	NpcBehaviorStateType behaviorState) const
{
	return draw(pool, static_cast<std::uint32_t>(traits.dexterity), behaviorState);
}

} // namespace iggy
