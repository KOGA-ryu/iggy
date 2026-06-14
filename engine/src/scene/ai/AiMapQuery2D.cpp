#include "scene/ai/AiMapQuery2D.hpp"

#include <cmath>

namespace {

float Distance(iggy::Vec2 a, iggy::Vec2 b)
{
	const float dx = a.x - b.x;
	const float dy = a.y - b.y;
	return std::sqrt(dx * dx + dy * dy);
}

bool HasTag(const std::vector<iggy::ResourceId> &tags, const iggy::ResourceId &tag)
{
	for (const iggy::ResourceId &existing : tags) {
		if (existing == tag)
			return true;
	}
	return false;
}

} // namespace

namespace iggy {

bool AiMapQuery2DResult::hasMatches() const
{
	return !entries.empty();
}

AiMapQuery2DResult AiMapQuery2D::query(const AiMap2D &map, Vec2 position) const
{
	AiMapQuery2DResult result;
	result.position = position;

	for (const AiMapNode2D *node : map.nodesContaining(position)) {
		result.entries.push_back({ *node, Distance(position, node->position) });
		result.patrolWeight += node->patrolWeight;
		result.coverWeight += node->coverWeight;
		result.dangerWeight += node->dangerWeight;
		result.interestWeight += node->interestWeight;

		for (const ResourceId &tag : node->tags) {
			if (!HasTag(result.tags, tag))
				result.tags.push_back(tag);
		}
	}

	if (!result.entries.empty())
		result.status = AiMapQuery2DStatus::Matched;

	return result;
}

} // namespace iggy
