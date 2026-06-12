#pragma once

#include <cstddef>
#include <vector>

#include "core/math/Aabb2.hpp"
#include "core/resource/ResourceId.hpp"

namespace iggy::render {

enum class RenderCommand2DType {
	Quad,
};

struct RenderCommand2D {
	RenderCommand2DType type = RenderCommand2DType::Quad;
	Aabb2 worldBounds;
	ResourceId materialId;
	int layer = 0;
	std::size_t order = 0;
};

struct RenderCommandList2D {
	std::vector<RenderCommand2D> commands;
};

class RenderCommandListBuilder2D {
public:
	void addQuad(RenderCommandList2D &list, Aabb2 worldBounds, ResourceId materialId, int layer) const;
};

} // namespace iggy::render
