#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "scene/ai/NpcWisdomPool.hpp"
#include "scene/ai/NpcTraitSet.hpp"
#include "scene/npc/NpcBehaviorState.hpp"

namespace iggy {

enum class NpcWisdomDrawStatus {
	Drawn,
	InvalidWisdom,
};

struct NpcWisdomDrawEntry {
	NpcWisdomEnt entry;
	std::size_t entryIndex = 0;
};

struct NpcWisdomDrawResult {
	NpcWisdomDrawStatus status = NpcWisdomDrawStatus::Drawn;
	std::uint32_t wisdom = 0;
	NpcBehaviorStateType behaviorState = NpcBehaviorStateType::None;
	std::vector<NpcWisdomDrawEntry> entries;

	[[nodiscard]] bool hasEntries() const;
};

class NpcWisdomDraw {
public:
	[[nodiscard]] NpcWisdomDrawResult draw(
		const NpcWisdomPool &pool,
		std::uint32_t wisdom,
		NpcBehaviorStateType behaviorState) const;

	[[nodiscard]] NpcWisdomDrawResult draw(
		const NpcWisdomPool &pool,
		const NpcTraitSet &traits,
		NpcBehaviorStateType behaviorState) const;
};

} // namespace iggy
