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
		{},
	});
}

void RenderCommandListBuilder2D::addTexturedQuad(RenderCommandList2D &list, Aabb2 worldBounds, ResourceId materialId, ResourceId textureId, Rect2 sourceRect, int layer) const
{
	list.commands.push_back({
		RenderCommand2DType::Quad,
		worldBounds,
		materialId,
		layer,
		list.commands.size(),
		{ textureId, sourceRect, true },
	});
}

} // namespace iggy::render
