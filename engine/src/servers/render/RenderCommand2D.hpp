#pragma once

#include <cstddef>
#include <vector>

#include "core/math/Aabb2.hpp"
#include "core/math/Rect2.hpp"
#include "core/resource/ResourceId.hpp"

namespace iggy::render {

enum class RenderCommand2DType {
	Quad,
};

struct RenderCommandTexture2D {
	ResourceId textureId;
	Rect2 sourceRect;
	bool hasSourceRect = false;
};

struct RenderCommand2D {
	RenderCommand2DType type = RenderCommand2DType::Quad;
	Aabb2 worldBounds;
	ResourceId materialId;
	int layer = 0;
	std::size_t order = 0;
	RenderCommandTexture2D texture;
};

struct RenderCommandList2D {
	std::vector<RenderCommand2D> commands;
};

class RenderCommandListBuilder2D {
public:
	void addQuad(RenderCommandList2D &list, Aabb2 worldBounds, ResourceId materialId, int layer) const;
	void addTexturedQuad(RenderCommandList2D &list, Aabb2 worldBounds, ResourceId materialId, ResourceId textureId, Rect2 sourceRect, int layer) const;
};

} // namespace iggy::render
