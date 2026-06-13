#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"

namespace iggy {

struct LevelItemDrop2D {
	ResourceId id;
	ResourceId itemId;
	std::uint32_t count = 0;
	Vec2 position;
	float pickupRadius = 0.0F;
	bool enabled = true;
};

struct LevelItemDrop2DRegistry {
	std::vector<LevelItemDrop2D> drops;

	[[nodiscard]] const LevelItemDrop2D *find(const ResourceId &id) const;
	[[nodiscard]] bool contains(const ResourceId &id) const;
};

enum class LevelItemDrop2DIssueCode {
	EmptyId,
	DuplicateId,
	EmptyItemId,
	ZeroCount,
	NegativePickupRadius,
};

struct LevelItemDrop2DIssue {
	LevelItemDrop2DIssueCode code = LevelItemDrop2DIssueCode::EmptyId;
	std::size_t dropIndex = 0;
	LevelItemDrop2D drop;
};

struct LevelItemDrop2DRegistryBuildResult {
	bool built = false;
	LevelItemDrop2DRegistry registry;
	std::vector<LevelItemDrop2DIssue> issues;
};

class LevelItemDrop2DRegistryBuilder {
public:
	[[nodiscard]] LevelItemDrop2DRegistryBuildResult build(const std::vector<LevelItemDrop2D> &drops) const;
};

} // namespace iggy
