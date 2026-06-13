#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "core/resource/ResourceId.hpp"

namespace iggy {

enum class ItemDefinition2DKind {
	Unknown,
	Consumable,
	KeyItem,
	Material,
	Equipment,
};

struct ItemDefinition2D {
	ResourceId itemId;
	std::string displayName;
	std::uint32_t maxStackCount = 0;
	ItemDefinition2DKind kind = ItemDefinition2DKind::Unknown;
};

struct ItemDefinition2DCatalog {
	std::vector<ItemDefinition2D> entries;

	[[nodiscard]] const ItemDefinition2D *find(const ResourceId &itemId) const;
	[[nodiscard]] bool contains(const ResourceId &itemId) const;
};

enum class ItemDefinition2DIssueCode {
	EmptyItemId,
	DuplicateItemId,
	EmptyDisplayName,
	ZeroMaxStackCount,
};

struct ItemDefinition2DIssue {
	ItemDefinition2DIssueCode code = ItemDefinition2DIssueCode::EmptyItemId;
	std::size_t entryIndex = 0;
	ItemDefinition2D entry;
};

struct ItemDefinition2DCatalogBuildResult {
	bool built = false;
	ItemDefinition2DCatalog catalog;
	std::vector<ItemDefinition2DIssue> issues;
};

class ItemDefinition2DCatalogBuilder {
public:
	[[nodiscard]] ItemDefinition2DCatalogBuildResult build(const std::vector<ItemDefinition2D> &entries) const;
};

} // namespace iggy
