#include "servers/render/RenderCommand2D.hpp"

namespace iggy::render {

void RenderCommandListBuilder2D::addQuad(RenderCommandList2D &list, Aabb2 worldBounds, ResourceId materialId, int layer) const
{
	list.commands.push_back({
		RenderCommand2DType::Quad,
		worldBounds,
		materialId,
		layer,
		list.commands.size(),
	});
}

} // namespace iggy::render
