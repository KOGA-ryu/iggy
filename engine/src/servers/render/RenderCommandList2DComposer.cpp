#include "servers/render/RenderCommandList2DComposer.hpp"

#include <vector>

namespace {

void AppendWithNormalizedOrder(iggy::render::RenderCommandList2D &target, iggy::render::RenderCommand2D command)
{
	command.order = target.commands.size();
	target.commands.push_back(command);
}

} // namespace

namespace iggy::render {

void RenderCommandList2DComposer::append(RenderCommandList2D &target, const RenderCommandList2D &source) const
{
	const std::vector<RenderCommand2D> snapshot = source.commands;
	for (RenderCommand2D command : snapshot)
		AppendWithNormalizedOrder(target, command);
}

RenderCommandList2D RenderCommandList2DComposer::merged(const RenderCommandList2D &first, const RenderCommandList2D &second) const
{
	RenderCommandList2D result;
	append(result, first);
	append(result, second);
	return result;
}

} // namespace iggy::render
