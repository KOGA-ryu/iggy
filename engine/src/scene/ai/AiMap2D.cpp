#include "scene/ai/AiMap2D.hpp"

namespace {

bool HasEarlierMatchingId(const std::vector<iggy::AiMapNode2D> &nodes, std::size_t currentIndex)
{
	for (std::size_t index = 0; index < currentIndex; ++index) {
		if (nodes[index].id == nodes[currentIndex].id)
			return true;
	}
	return false;
}

bool HasNodeId(const std::vector<iggy::AiMapNode2D> &nodes, const iggy::ResourceId &id)
{
	for (const iggy::AiMapNode2D &node : nodes) {
		if (node.id == id)
			return true;
	}
	return false;
}

bool HasEarlierMatchingLink(const std::vector<iggy::ResourceId> &links, std::size_t currentIndex)
{
	for (std::size_t index = 0; index < currentIndex; ++index) {
		if (links[index] == links[currentIndex])
			return true;
	}
	return false;
}

bool ContainsPosition(const iggy::AiMapNode2D &node, iggy::Vec2 position)
{
	const float dx = position.x - node.position.x;
	const float dy = position.y - node.position.y;
	return dx * dx + dy * dy <= node.radius * node.radius;
}

iggy::AiMap2DIssue Issue(
	iggy::AiMap2DIssueCode code,
	std::size_t nodeIndex,
	const iggy::AiMapNode2D &node,
	std::size_t linkIndex = 0,
	iggy::ResourceId linkId = {})
{
	return {
		code,
		nodeIndex,
		node,
		linkIndex,
		linkId,
	};
}

} // namespace

namespace iggy {

const AiMapNode2D *AiMap2D::find(const ResourceId &id) const
{
	for (const AiMapNode2D &node : nodes) {
		if (node.id == id)
			return &node;
	}
	return nullptr;
}

bool AiMap2D::contains(const ResourceId &id) const
{
	return find(id) != nullptr;
}

std::vector<const AiMapNode2D *> AiMap2D::nodesContaining(Vec2 position) const
{
	std::vector<const AiMapNode2D *> containing;
	for (const AiMapNode2D &node : nodes) {
		if (node.enabled && ContainsPosition(node, position))
			containing.push_back(&node);
	}
	return containing;
}

AiMap2DBuildResult AiMap2DBuilder::build(const std::vector<AiMapNode2D> &nodes) const
{
	AiMap2DBuildResult result;

	for (std::size_t nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex) {
		const AiMapNode2D &node = nodes[nodeIndex];
		if (node.id.empty())
			result.issues.push_back(Issue(AiMap2DIssueCode::EmptyNodeId, nodeIndex, node));
		if (HasEarlierMatchingId(nodes, nodeIndex))
			result.issues.push_back(Issue(AiMap2DIssueCode::DuplicateNodeId, nodeIndex, node));
		if (node.radius < 0.0F)
			result.issues.push_back(Issue(AiMap2DIssueCode::NegativeRadius, nodeIndex, node));

		for (std::size_t linkIndex = 0; linkIndex < node.links.size(); ++linkIndex) {
			const ResourceId &linkId = node.links[linkIndex];
			if (!HasNodeId(nodes, linkId)) {
				result.issues.push_back(Issue(
					AiMap2DIssueCode::MissingLinkTarget,
					nodeIndex,
					node,
					linkIndex,
					linkId));
			}
			if (HasEarlierMatchingLink(node.links, linkIndex)) {
				result.issues.push_back(Issue(
					AiMap2DIssueCode::DuplicateLink,
					nodeIndex,
					node,
					linkIndex,
					linkId));
			}
		}
	}

	if (!result.issues.empty())
		return result;

	result.built = true;
	result.map.nodes = nodes;
	return result;
}

} // namespace iggy
