#include "scene/debug/SceneAsciiCanvas2D.hpp"

namespace {

std::size_t IndexOf(const iggy::SceneAsciiCanvas2D &canvas, std::size_t x, std::size_t y)
{
	return y * canvas.width + x;
}

bool InBounds(const iggy::SceneAsciiCanvas2D &canvas, std::size_t x, std::size_t y)
{
	return x < canvas.width && y < canvas.height;
}

} // namespace

namespace iggy {

bool SceneAsciiCanvas2D::valid() const
{
	return width > 0 && height > 0 && cells.size() == width * height;
}

bool SceneAsciiCanvas2D::empty() const
{
	return cells.empty();
}

std::size_t SceneAsciiCanvas2D::cellCount() const
{
	return cells.size();
}

SceneAsciiCanvas2D makeSceneAsciiCanvas2D(std::size_t width, std::size_t height, char fillGlyph)
{
	if (width == 0 || height == 0)
		return {};

	SceneAsciiCanvas2D canvas;
	canvas.width = width;
	canvas.height = height;
	canvas.cells.assign(width * height, SceneAsciiCell2D { fillGlyph });
	return canvas;
}

SceneAsciiCanvas2DSetResult setSceneAsciiCanvas2DPoint(
	const SceneAsciiCanvas2D &canvas,
	std::size_t x,
	std::size_t y,
	char glyph)
{
	SceneAsciiCanvas2DSetResult result;
	result.canvas = canvas;
	result.x = x;
	result.y = y;
	result.glyph = glyph;

	if (!canvas.valid()) {
		result.status = SceneAsciiCanvas2DStatus::InvalidCanvas;
		return result;
	}

	if (!InBounds(canvas, x, y)) {
		result.status = SceneAsciiCanvas2DStatus::OutOfBounds;
		return result;
	}

	const std::size_t index = IndexOf(canvas, x, y);
	result.changed = result.canvas.cells[index].glyph != glyph;
	result.canvas.cells[index].glyph = glyph;
	result.status = SceneAsciiCanvas2DStatus::Ok;
	return result;
}

SceneAsciiCanvas2DGetResult getSceneAsciiCanvas2DPoint(
	const SceneAsciiCanvas2D &canvas,
	std::size_t x,
	std::size_t y)
{
	SceneAsciiCanvas2DGetResult result;
	result.x = x;
	result.y = y;

	if (!canvas.valid()) {
		result.status = SceneAsciiCanvas2DStatus::InvalidCanvas;
		return result;
	}

	if (!InBounds(canvas, x, y)) {
		result.status = SceneAsciiCanvas2DStatus::OutOfBounds;
		return result;
	}

	result.cell = canvas.cells[IndexOf(canvas, x, y)];
	result.status = SceneAsciiCanvas2DStatus::Ok;
	return result;
}

SceneAsciiCanvas2DLabelResult drawSceneAsciiCanvas2DLabel(
	const SceneAsciiCanvas2D &canvas,
	std::size_t x,
	std::size_t y,
	const std::string &label)
{
	SceneAsciiCanvas2DLabelResult result;
	result.canvas = canvas;
	result.x = x;
	result.y = y;
	result.label = label;

	if (!canvas.valid()) {
		result.status = SceneAsciiCanvas2DStatus::InvalidCanvas;
		return result;
	}

	if (!InBounds(canvas, x, y)) {
		result.status = SceneAsciiCanvas2DStatus::OutOfBounds;
		return result;
	}

	result.status = SceneAsciiCanvas2DStatus::Ok;
	const std::size_t available = canvas.width - x;
	const std::size_t visibleCount = label.size() < available ? label.size() : available;
	result.clipped = label.size() > visibleCount;
	result.drawnCount = visibleCount;

	for (std::size_t offset = 0; offset < visibleCount; ++offset) {
		SceneAsciiCell2D &cell = result.canvas.cells[IndexOf(canvas, x + offset, y)];
		if (cell.glyph != label[offset]) {
			cell.glyph = label[offset];
			result.changed = true;
		}
	}

	return result;
}

std::vector<std::string> renderSceneAsciiCanvas2DRows(const SceneAsciiCanvas2D &canvas)
{
	if (!canvas.valid())
		return {};

	std::vector<std::string> rows;
	rows.reserve(canvas.height);
	for (std::size_t y = 0; y < canvas.height; ++y) {
		std::string row;
		row.reserve(canvas.width);
		for (std::size_t x = 0; x < canvas.width; ++x)
			row.push_back(canvas.cells[IndexOf(canvas, x, y)].glyph);
		rows.push_back(row);
	}
	return rows;
}

std::string renderSceneAsciiCanvas2DString(const SceneAsciiCanvas2D &canvas)
{
	const std::vector<std::string> rows = renderSceneAsciiCanvas2DRows(canvas);
	std::string rendered;
	for (std::size_t index = 0; index < rows.size(); ++index) {
		if (index > 0)
			rendered.push_back('\n');
		rendered += rows[index];
	}
	return rendered;
}

} // namespace iggy
