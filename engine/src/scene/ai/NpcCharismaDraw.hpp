#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "scene/ai/NpcCharismaPool.hpp"
#include "scene/ai/NpcTraitSet.hpp"
#include "scene/npc/NpcBehaviorState.hpp"

namespace iggy {

enum class NpcCharismaDrawStatus {
	Drawn,
	InvalidCharisma,
};

struct NpcCharismaDrawEntry {
	NpcCharismaEnt entry;
	std::size_t entryIndex = 0;
};

struct NpcCharismaDrawResult {
	NpcCharismaDrawStatus status = NpcCharismaDrawStatus::Drawn;
	std::uint32_t charisma = 0;
	NpcBehaviorStateType behaviorState = NpcBehaviorStateType::None;
	std::vector<NpcCharismaDrawEntry> entries;

	[[nodiscard]] bool hasEntries() const;
};

class NpcCharismaDraw {
public:
	[[nodiscard]] NpcCharismaDrawResult draw(
		const NpcCharismaPool &pool,
		std::uint32_t charisma,
		NpcBehaviorStateType behaviorState) const;

	[[nodiscard]] NpcCharismaDrawResult draw(
		const NpcCharismaPool &pool,
		const NpcTraitSet &traits,
		NpcBehaviorStateType behaviorState) const;
};

} // namespace iggy
