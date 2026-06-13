#pragma once

#include <cstdint>

#include "core/resource/ResourceId.hpp"
#include "scene/inventory/InventoryState2D.hpp"

namespace iggy {

enum class InventoryAddItem2DStatus {
	Added,
	InvalidItemId,
	InvalidCount,
};

struct InventoryAddItem2DResult {
	InventoryAddItem2DStatus status = InventoryAddItem2DStatus::InvalidItemId;
	InventoryState2D inventory;
	ResourceId itemId;
	std::uint32_t count = 0;
	bool changed = false;
};

class InventoryAddItem2D {
public:
	[[nodiscard]] InventoryAddItem2DResult add(
		const InventoryState2D &inventory,
		ResourceId itemId,
		std::uint32_t count) const;
};

} // namespace iggy
