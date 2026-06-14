#include "scene/ai/NpcCharismaDraw.hpp"

namespace {

bool CharismaInRange(std::uint32_t value)
{
	return value <= static_cast<std::uint32_t>(iggy::NpcTraitSetMaxScore);
}

} // namespace

namespace iggy {

bool NpcCharismaDrawResult::hasEntries() const
{
	return !entries.empty();
}

NpcCharismaDrawResult NpcCharismaDraw::draw(
	const NpcCharismaPool &pool,
	std::uint32_t charisma,
	NpcBehaviorStateType behaviorState) const
{
	NpcCharismaDrawResult result;
	result.charisma = charisma;
	result.behaviorState = behaviorState;

	if (!CharismaInRange(charisma)) {
		result.status = NpcCharismaDrawStatus::InvalidCharisma;
		return result;
	}

	for (std::size_t index = 0; index < pool.entries.size(); ++index) {
		const NpcCharismaEnt &entry = pool.entries[index];
		if (entry.minimumCharisma <= charisma && entry.behaviorState == behaviorState)
			result.entries.push_back({ entry, index });
	}

	return result;
}

NpcCharismaDrawResult NpcCharismaDraw::draw(
	const NpcCharismaPool &pool,
	const NpcTraitSet &traits,
	NpcBehaviorStateType behaviorState) const
{
	return draw(pool, static_cast<std::uint32_t>(traits.charisma), behaviorState);
}

} // namespace iggy
