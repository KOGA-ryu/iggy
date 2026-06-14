#pragma once

#include <vector>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "scene/ai/AiMap2D.hpp"

namespace iggy {

enum class AiMapQuery2DStatus {
	NoMatch,
	Matched,
};

struct AiMapQuery2DEntry {
	AiMapNode2D node;
	float distance = 0.0F;
};

struct AiMapQuery2DResult {
	AiMapQuery2DStatus status = AiMapQuery2DStatus::NoMatch;
	Vec2 position;
	std::vector<AiMapQuery2DEntry> entries;
	float patrolWeight = 0.0F;
	float coverWeight = 0.0F;
	float dangerWeight = 0.0F;
	float interestWeight = 0.0F;
	std::vector<ResourceId> tags;

	[[nodiscard]] bool hasMatches() const;
};

class AiMapQuery2D {
public:
	[[nodiscard]] AiMapQuery2DResult query(const AiMap2D &map, Vec2 position) const;
};

} // namespace iggy
