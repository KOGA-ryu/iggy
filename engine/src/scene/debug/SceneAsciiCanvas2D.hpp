#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace iggy {

struct SceneAsciiCell2D {
	char glyph = ' ';
};

struct SceneAsciiCanvas2D {
	std::size_t width = 0;
	std::size_t height = 0;
	std::vector<SceneAsciiCell2D> cells;

	[[nodiscard]] bool valid() const;
	[[nodiscard]] bool empty() const;
	[[nodiscard]] std::size_t cellCount() const;
};

enum class SceneAsciiCanvas2DStatus {
	Ok,
	InvalidCanvas,
	OutOfBounds,
};

struct SceneAsciiCanvas2DSetResult {
	SceneAsciiCanvas2D canvas;
	std::size_t x = 0;
	std::size_t y = 0;
	char glyph = ' ';
	SceneAsciiCanvas2DStatus status = SceneAsciiCanvas2DStatus::InvalidCanvas;
	bool changed = false;
};

struct SceneAsciiCanvas2DGetResult {
	std::size_t x = 0;
	std::size_t y = 0;
	SceneAsciiCanvas2DStatus status = SceneAsciiCanvas2DStatus::InvalidCanvas;
	SceneAsciiCell2D cell;
};

struct SceneAsciiCanvas2DLabelResult {
	SceneAsciiCanvas2D canvas;
	std::size_t x = 0;
	std::size_t y = 0;
	std::string label;
	SceneAsciiCanvas2DStatus status = SceneAsciiCanvas2DStatus::InvalidCanvas;
	std::size_t drawnCount = 0;
	bool clipped = false;
	bool changed = false;
};

[[nodiscard]] SceneAsciiCanvas2D makeSceneAsciiCanvas2D(
	std::size_t width,
	std::size_t height,
	char fillGlyph = ' ');

[[nodiscard]] SceneAsciiCanvas2DSetResult setSceneAsciiCanvas2DPoint(
	const SceneAsciiCanvas2D &canvas,
	std::size_t x,
	std::size_t y,
	char glyph);

[[nodiscard]] SceneAsciiCanvas2DGetResult getSceneAsciiCanvas2DPoint(
	const SceneAsciiCanvas2D &canvas,
	std::size_t x,
	std::size_t y);

[[nodiscard]] SceneAsciiCanvas2DLabelResult drawSceneAsciiCanvas2DLabel(
	const SceneAsciiCanvas2D &canvas,
	std::size_t x,
	std::size_t y,
	const std::string &label);

[[nodiscard]] std::vector<std::string> renderSceneAsciiCanvas2DRows(
	const SceneAsciiCanvas2D &canvas);

[[nodiscard]] std::string renderSceneAsciiCanvas2DString(
	const SceneAsciiCanvas2D &canvas);

} // namespace iggy
