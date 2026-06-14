#pragma once

#include <cstddef>
#include <vector>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"

namespace iggy {

struct AiMapNode2D {
	ResourceId id;
	Vec2 position;
	float radius = 0.0F;
	float patrolWeight = 0.0F;
	float coverWeight = 0.0F;
	float dangerWeight = 0.0F;
	float interestWeight = 0.0F;
	std::vector<ResourceId> tags;
	std::vector<ResourceId> links;
	bool enabled = true;
};

struct AiMap2D {
	std::vector<AiMapNode2D> nodes;

	[[nodiscard]] const AiMapNode2D *find(const ResourceId &id) const;
	[[nodiscard]] bool contains(const ResourceId &id) const;
	[[nodiscard]] std::vector<const AiMapNode2D *> nodesContaining(Vec2 position) const;
};

enum class AiMap2DIssueCode {
	EmptyNodeId,
	DuplicateNodeId,
	NegativeRadius,
	MissingLinkTarget,
	DuplicateLink,
};

struct AiMap2DIssue {
	AiMap2DIssueCode code = AiMap2DIssueCode::EmptyNodeId;
	std::size_t nodeIndex = 0;
	AiMapNode2D node;
	std::size_t linkIndex = 0;
	ResourceId linkId;
};

struct AiMap2DBuildResult {
	bool built = false;
	AiMap2D map;
	std::vector<AiMap2DIssue> issues;
};

class AiMap2DBuilder {
public:
	[[nodiscard]] AiMap2DBuildResult build(const std::vector<AiMapNode2D> &nodes) const;
};

} // namespace iggy
