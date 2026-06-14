#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "scene/ai/NpcStrengthStore2D.hpp"
#include "scene/ai/NpcTraitSet2D.hpp"
#include "scene/npc/NpcBehaviorState2D.hpp"

namespace iggy {

enum class NpcStrengthStoreQuery2DStatus {
	Queried,
	InvalidStrength,
};

struct NpcStrengthStoreQuery2DEntry {
	NpcStrengthBehaviorEntry2D entry;
	std::size_t entryIndex = 0;
};

struct NpcStrengthStoreQuery2DResult {
	NpcStrengthStoreQuery2DStatus status = NpcStrengthStoreQuery2DStatus::Queried;
	std::uint32_t strength = 0;
	NpcBehaviorState2DType behaviorState = NpcBehaviorState2DType::None;
	std::vector<NpcStrengthStoreQuery2DEntry> entries;

	[[nodiscard]] bool hasEntries() const;
};

class NpcStrengthStoreQuery2D {
public:
	[[nodiscard]] NpcStrengthStoreQuery2DResult query(
		const NpcStrengthStore2D &store,
		std::uint32_t strength,
		NpcBehaviorState2DType behaviorState) const;

	[[nodiscard]] NpcStrengthStoreQuery2DResult query(
		const NpcStrengthStore2D &store,
		const NpcTraitSet2D &traits,
		NpcBehaviorState2DType behaviorState) const;
};

} // namespace iggy
