#include <cstdlib>
#include <string>
#include <vector>

#include "scene/debug/SceneAsciiCanvas2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

void TestCanvasBuildDimensionsFillAndCellCount()
{
	const iggy::SceneAsciiCanvas2D canvas = iggy::makeSceneAsciiCanvas2D(4, 3, '.');

	Expect(canvas.valid(), "non-zero ascii canvas should be valid");
	Expect(!canvas.empty(), "non-zero ascii canvas should not be empty");
	Expect(canvas.width == 4 && canvas.height == 3, "ascii canvas should preserve dimensions");
	Expect(canvas.cellCount() == 12, "ascii canvas should preserve cell count");
	Expect(iggy::renderSceneAsciiCanvas2DRows(canvas) == std::vector<std::string>({ "....", "....", "...." }), "ascii canvas should fill all cells");
}

void TestZeroDimensionsProduceInvalidEmptyCanvas()
{
	const iggy::SceneAsciiCanvas2D zeroWidth = iggy::makeSceneAsciiCanvas2D(0, 2, '#');
	const iggy::SceneAsciiCanvas2D zeroHeight = iggy::makeSceneAsciiCanvas2D(2, 0, '#');

	Expect(!zeroWidth.valid() && zeroWidth.empty(), "zero-width ascii canvas should be invalid empty");
	Expect(!zeroHeight.valid() && zeroHeight.empty(), "zero-height ascii canvas should be invalid empty");
	Expect(iggy::renderSceneAsciiCanvas2DRows(zeroWidth).empty(), "invalid ascii canvas rows should render empty");
	Expect(iggy::renderSceneAsciiCanvas2DString(zeroHeight).empty(), "invalid ascii canvas string should render empty");
}

void TestSetAndGetInBounds()
{
	const iggy::SceneAsciiCanvas2D canvas = iggy::makeSceneAsciiCanvas2D(3, 2, '.');

	const iggy::SceneAsciiCanvas2DSetResult set =
		iggy::setSceneAsciiCanvas2DPoint(canvas, 1, 0, '@');
	const iggy::SceneAsciiCanvas2DGetResult get =
		iggy::getSceneAsciiCanvas2DPoint(set.canvas, 1, 0);

	Expect(set.status == iggy::SceneAsciiCanvas2DStatus::Ok, "in-bounds ascii set should succeed");
	Expect(set.x == 1 && set.y == 0 && set.glyph == '@', "ascii set should preserve request diagnostics");
	Expect(set.changed, "ascii set should report changed cell");
	Expect(get.status == iggy::SceneAsciiCanvas2DStatus::Ok, "in-bounds ascii get should succeed");
	Expect(get.cell.glyph == '@', "ascii get should return written glyph");
	Expect(iggy::renderSceneAsciiCanvas2DRows(set.canvas) == std::vector<std::string>({ ".@.", "..." }), "ascii set should update expected row");
}

void TestOutOfBoundsSetAndGetDoNotMutate()
{
	const iggy::SceneAsciiCanvas2D canvas = iggy::makeSceneAsciiCanvas2D(2, 2, '.');

	const iggy::SceneAsciiCanvas2DSetResult set =
		iggy::setSceneAsciiCanvas2DPoint(canvas, 2, 0, '@');
	const iggy::SceneAsciiCanvas2DGetResult get =
		iggy::getSceneAsciiCanvas2DPoint(canvas, 0, 2);

	Expect(set.status == iggy::SceneAsciiCanvas2DStatus::OutOfBounds, "out-of-bounds ascii set should fail");
	Expect(!set.changed, "out-of-bounds ascii set should not report change");
	Expect(iggy::renderSceneAsciiCanvas2DRows(set.canvas) == std::vector<std::string>({ "..", ".." }), "out-of-bounds ascii set should preserve canvas");
	Expect(get.status == iggy::SceneAsciiCanvas2DStatus::OutOfBounds, "out-of-bounds ascii get should fail");
	Expect(get.cell.glyph == ' ', "out-of-bounds ascii get should return default cell");
}

void TestInvalidCanvasOperationsDoNotMutate()
{
	iggy::SceneAsciiCanvas2D invalid;
	invalid.width = 2;
	invalid.height = 2;
	invalid.cells = { { '.' } };

	const iggy::SceneAsciiCanvas2DSetResult set =
		iggy::setSceneAsciiCanvas2DPoint(invalid, 0, 0, '@');
	const iggy::SceneAsciiCanvas2DGetResult get =
		iggy::getSceneAsciiCanvas2DPoint(invalid, 0, 0);
	const iggy::SceneAsciiCanvas2DLabelResult label =
		iggy::drawSceneAsciiCanvas2DLabel(invalid, 0, 0, "bad");

	Expect(!invalid.valid(), "invalid ascii fixture should be invalid");
	Expect(set.status == iggy::SceneAsciiCanvas2DStatus::InvalidCanvas, "invalid ascii set should report invalid canvas");
	Expect(get.status == iggy::SceneAsciiCanvas2DStatus::InvalidCanvas, "invalid ascii get should report invalid canvas");
	Expect(label.status == iggy::SceneAsciiCanvas2DStatus::InvalidCanvas, "invalid ascii label should report invalid canvas");
	Expect(set.canvas.cells.size() == invalid.cells.size() && label.canvas.cells.size() == invalid.cells.size(), "invalid ascii operations should preserve copied cells");
}

void TestDrawLabelInBounds()
{
	const iggy::SceneAsciiCanvas2D canvas = iggy::makeSceneAsciiCanvas2D(6, 2, '.');

	const iggy::SceneAsciiCanvas2DLabelResult label =
		iggy::drawSceneAsciiCanvas2DLabel(canvas, 1, 1, "NPC");

	Expect(label.status == iggy::SceneAsciiCanvas2DStatus::Ok, "in-bounds ascii label should succeed");
	Expect(label.x == 1 && label.y == 1 && label.label == "NPC", "ascii label should preserve request diagnostics");
	Expect(label.drawnCount == 3 && !label.clipped, "in-bounds ascii label should draw all characters without clipping");
	Expect(label.changed, "in-bounds ascii label should report change");
	Expect(iggy::renderSceneAsciiCanvas2DRows(label.canvas) == std::vector<std::string>({ "......", ".NPC.." }), "ascii label should write visible characters");
}

void TestDrawLabelClipsAtRightEdge()
{
	const iggy::SceneAsciiCanvas2D canvas = iggy::makeSceneAsciiCanvas2D(5, 1, '.');

	const iggy::SceneAsciiCanvas2DLabelResult label =
		iggy::drawSceneAsciiCanvas2DLabel(canvas, 3, 0, "GOBLIN");

	Expect(label.status == iggy::SceneAsciiCanvas2DStatus::Ok, "clipped ascii label should still succeed from valid start");
	Expect(label.drawnCount == 2 && label.clipped, "clipped ascii label should report visible draw count and clipping");
	Expect(iggy::renderSceneAsciiCanvas2DRows(label.canvas) == std::vector<std::string>({ "...GO" }), "clipped ascii label should draw only visible characters");
}

void TestDrawLabelOutOfBoundsAndEmptyLabel()
{
	const iggy::SceneAsciiCanvas2D canvas = iggy::makeSceneAsciiCanvas2D(3, 1, '.');

	const iggy::SceneAsciiCanvas2DLabelResult outOfBounds =
		iggy::drawSceneAsciiCanvas2DLabel(canvas, 3, 0, "X");
	const iggy::SceneAsciiCanvas2DLabelResult empty =
		iggy::drawSceneAsciiCanvas2DLabel(canvas, 2, 0, "");

	Expect(outOfBounds.status == iggy::SceneAsciiCanvas2DStatus::OutOfBounds, "out-of-bounds ascii label should fail");
	Expect(!outOfBounds.changed && outOfBounds.drawnCount == 0, "out-of-bounds ascii label should not draw");
	Expect(iggy::renderSceneAsciiCanvas2DRows(outOfBounds.canvas) == std::vector<std::string>({ "..." }), "out-of-bounds ascii label should preserve canvas");
	Expect(empty.status == iggy::SceneAsciiCanvas2DStatus::Ok, "empty ascii label from valid start should succeed");
	Expect(empty.drawnCount == 0 && !empty.clipped && !empty.changed, "empty ascii label should be a no-op");
	Expect(iggy::renderSceneAsciiCanvas2DRows(empty.canvas) == std::vector<std::string>({ "..." }), "empty ascii label should preserve canvas");
}

void TestRenderRowsAndStringOutputExactly()
{
	iggy::SceneAsciiCanvas2D canvas = iggy::makeSceneAsciiCanvas2D(3, 2, '.');
	canvas = iggy::setSceneAsciiCanvas2DPoint(canvas, 0, 0, '@').canvas;
	canvas = iggy::setSceneAsciiCanvas2DPoint(canvas, 2, 1, '#').canvas;

	Expect(iggy::renderSceneAsciiCanvas2DRows(canvas) == std::vector<std::string>({ "@..", "..#" }), "ascii rows should preserve top-to-bottom order");
	Expect(iggy::renderSceneAsciiCanvas2DString(canvas) == std::string("@..\n..#"), "ascii string should join rows with newlines");
}

void TestMultipleWritesOverwriteOnlyRequestedCellsAndPreserveInputCopy()
{
	const iggy::SceneAsciiCanvas2D original = iggy::makeSceneAsciiCanvas2D(4, 1, '.');
	const iggy::SceneAsciiCanvas2DSetResult first =
		iggy::setSceneAsciiCanvas2DPoint(original, 0, 0, 'A');
	const iggy::SceneAsciiCanvas2DSetResult second =
		iggy::setSceneAsciiCanvas2DPoint(first.canvas, 3, 0, 'Z');
	const iggy::SceneAsciiCanvas2DSetResult overwrite =
		iggy::setSceneAsciiCanvas2DPoint(second.canvas, 0, 0, 'B');
	const iggy::SceneAsciiCanvas2DSetResult same =
		iggy::setSceneAsciiCanvas2DPoint(overwrite.canvas, 0, 0, 'B');

	Expect(iggy::renderSceneAsciiCanvas2DRows(original) == std::vector<std::string>({ "...." }), "ascii value operations should not mutate input canvas");
	Expect(iggy::renderSceneAsciiCanvas2DRows(first.canvas) == std::vector<std::string>({ "A..." }), "first ascii write should update requested cell");
	Expect(iggy::renderSceneAsciiCanvas2DRows(second.canvas) == std::vector<std::string>({ "A..Z" }), "second ascii write should preserve previous cell");
	Expect(iggy::renderSceneAsciiCanvas2DRows(overwrite.canvas) == std::vector<std::string>({ "B..Z" }), "ascii overwrite should only update requested cell");
	Expect(!same.changed, "writing the same glyph should not report a changed cell");
}

} // namespace

int main()
{
	TestCanvasBuildDimensionsFillAndCellCount();
	TestZeroDimensionsProduceInvalidEmptyCanvas();
	TestSetAndGetInBounds();
	TestOutOfBoundsSetAndGetDoNotMutate();
	TestInvalidCanvasOperationsDoNotMutate();
	TestDrawLabelInBounds();
	TestDrawLabelClipsAtRightEdge();
	TestDrawLabelOutOfBoundsAndEmptyLabel();
	TestRenderRowsAndStringOutputExactly();
	TestMultipleWritesOverwriteOnlyRequestedCellsAndPreserveInputCopy();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
