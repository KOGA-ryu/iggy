#pragma once

#include <cstdint>

#include "core/resource/ResourceId.hpp"
#include "scene/inventory/InventoryState2D.hpp"
#include "scene/inventory/ItemDefinition2D.hpp"

namespace iggy {

enum class InventoryStackPolicy2DStatus {
	CanAdd,
	MissingItemId,
	InvalidCount,
	ItemDefinitionNotFound,
	StackLimitExceeded,
};

struct InventoryStackPolicy2DResult {
	InventoryStackPolicy2DStatus status = InventoryStackPolicy2DStatus::MissingItemId;
	ResourceId itemId;
	std::uint32_t requestedCount = 0;
	std::uint32_t currentCount = 0;
	std::uint32_t maxStackCount = 0;
	std::uint32_t resultingCount = 0;
	ItemDefinition2D definition;

	[[nodiscard]] bool canAdd() const;
};

class InventoryStackPolicy2D {
public:
	[[nodiscard]] InventoryStackPolicy2DResult planAdd(
		const InventoryState2D &inventory,
		const ItemDefinition2DCatalog &catalog,
		ResourceId itemId,
		std::uint32_t count) const;
};

} // namespace iggy
