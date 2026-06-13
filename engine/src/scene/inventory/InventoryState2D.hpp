#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "core/resource/ResourceId.hpp"

namespace iggy {

struct InventoryItemStack2D {
	ResourceId itemId;
	std::uint32_t count = 0;
};

struct InventoryState2D {
	std::vector<InventoryItemStack2D> stacks;

	[[nodiscard]] const InventoryItemStack2D *find(const ResourceId &itemId) const;
	[[nodiscard]] bool contains(const ResourceId &itemId) const;
};

enum class InventoryState2DIssueCode {
	EmptyItemId,
	ZeroCount,
	DuplicateItemId,
};

struct InventoryState2DIssue {
	InventoryState2DIssueCode code = InventoryState2DIssueCode::EmptyItemId;
	std::size_t stackIndex = 0;
	InventoryItemStack2D stack;
};

struct InventoryState2DBuildResult {
	bool built = false;
	InventoryState2D inventory;
	std::vector<InventoryState2DIssue> issues;
};

class InventoryState2DBuilder {
public:
	[[nodiscard]] InventoryState2DBuildResult build(const std::vector<InventoryItemStack2D> &stacks) const;
};

} // namespace iggy
