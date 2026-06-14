#include "scene/ai/NpcWisdomDraw.hpp"

namespace {

bool WisdomInRange(std::uint32_t value)
{
	return value <= static_cast<std::uint32_t>(iggy::NpcTraitSetMaxScore);
}

} // namespace

namespace iggy {

bool NpcWisdomDrawResult::hasEntries() const
{
	return !entries.empty();
}

NpcWisdomDrawResult NpcWisdomDraw::draw(
	const NpcWisdomPool &pool,
	std::uint32_t wisdom,
	NpcBehaviorStateType behaviorState) const
{
	NpcWisdomDrawResult result;
	result.wisdom = wisdom;
	result.behaviorState = behaviorState;

	if (!WisdomInRange(wisdom)) {
		result.status = NpcWisdomDrawStatus::InvalidWisdom;
		return result;
	}

	for (std::size_t index = 0; index < pool.entries.size(); ++index) {
		const NpcWisdomEnt &entry = pool.entries[index];
		if (entry.minimumWisdom <= wisdom && entry.behaviorState == behaviorState)
			result.entries.push_back({ entry, index });
	}

	return result;
}

NpcWisdomDrawResult NpcWisdomDraw::draw(
	const NpcWisdomPool &pool,
	const NpcTraitSet &traits,
	NpcBehaviorStateType behaviorState) const
{
	return draw(pool, static_cast<std::uint32_t>(traits.wisdom), behaviorState);
}

} // namespace iggy
