#pragma once

#include "servers/render/RenderCommand2D.hpp"

namespace iggy::render {

class RenderCommandList2DComposer {
public:
	void append(RenderCommandList2D &target, const RenderCommandList2D &source) const;
	[[nodiscard]] RenderCommandList2D merged(const RenderCommandList2D &first, const RenderCommandList2D &second) const;
};

} // namespace iggy::render
