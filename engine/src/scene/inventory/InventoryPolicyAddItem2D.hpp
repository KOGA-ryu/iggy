#pragma once

#include <cstdint>

#include "core/resource/ResourceId.hpp"
#include "scene/inventory/InventoryAddItem2D.hpp"
#include "scene/inventory/InventoryEvent2D.hpp"
#include "scene/inventory/InventoryStackPolicy2D.hpp"
#include "scene/inventory/InventoryState2D.hpp"
#include "scene/inventory/ItemDefinition2D.hpp"

namespace iggy {

enum class InventoryPolicyAddItem2DStatus {
	Added,
	PlanFailed,
	AddFailed,
};

struct InventoryPolicyAddItem2DResult {
	InventoryPolicyAddItem2DStatus status = InventoryPolicyAddItem2DStatus::PlanFailed;
	InventoryStackPolicy2DResult plan;
	InventoryAddItem2DResult add;
	InventoryState2D inventory;
	InventoryEventRecorder2D events;
	bool changed = false;

	[[nodiscard]] bool added() const;
};

class InventoryPolicyAddItem2D {
public:
	[[nodiscard]] InventoryPolicyAddItem2DResult add(
		const InventoryState2D &inventory,
		const ItemDefinition2DCatalog &catalog,
		ResourceId itemId,
		std::uint32_t count) const;
};

} // namespace iggy
