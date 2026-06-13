#include <cstdlib>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/ui/UiToolBeltState.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

std::vector<iggy::ResourceId> Items()
{
	return {
		Id("tool:select"), Id("tool:move"), {},
		{}, Id("tool:inspect"), {},
		Id("tool:paint"), {}, Id("tool:erase"),
	};
}

void TestInvalidStateIsRejectedByQueries()
{
	const iggy::ui::UiToolBeltState invalid { 0, 3, 0, 0 };

	Expect(!iggy::ui::validUiToolBeltState(invalid), "invalid tool belt state should be rejected");
	Expect(iggy::ui::uiToolBeltActiveIndex(invalid) == -1, "invalid tool belt active index should be -1");
	Expect(!iggy::ui::uiToolBeltCellOccupied(invalid, Items(), 0, 0), "invalid tool belt should have no occupied cells");
}

void TestSlotIndexAndOccupancyUseRowMajorItems()
{
	const iggy::ui::UiToolBeltState state { 3, 3, 0, 0 };
	const std::vector<iggy::ResourceId> items = Items();

	Expect(iggy::ui::uiToolBeltSlotIndex(state, 2, 2) == 8, "tool belt slot index should be row major");
	Expect(iggy::ui::uiToolBeltCellOccupied(state, items, 0, 1), "tool belt occupied query should see non-empty id");
	Expect(!iggy::ui::uiToolBeltCellOccupied(state, items, 1, 0), "tool belt occupied query should treat empty id as empty");
	Expect(iggy::ui::uiToolBeltRowLeadColumn(state, items, 1) == 1, "tool belt row lead should find first occupied cell");
}

void TestNormalizeMovesEmptyActiveCellToFirstOccupiedCell()
{
	const iggy::ui::UiToolBeltState state { 3, 3, 1, 0 };

	const iggy::ui::UiToolBeltState normalized = iggy::ui::normalizeUiToolBeltToOccupied(state, Items());

	Expect(normalized.activeRow == 0 && normalized.activeColumn == 0, "tool belt normalize should move to first occupied cell");
}

void TestOccupiedRowSteppingSkipsEmptyRowsAndWraps()
{
	const iggy::ui::UiToolBeltState state { 4, 3, 0, 0 };
	std::vector<iggy::ResourceId> items = Items();
	items.resize(12);

	const iggy::ui::UiToolBeltState next = iggy::ui::stepUiToolBeltRowOccupied(state, 1, items);
	const iggy::ui::UiToolBeltState previous = iggy::ui::stepUiToolBeltRowOccupied(state, -1, items);

	Expect(next.activeRow == 1 && next.activeColumn == 1, "occupied row step should land on next non-empty row lead");
	Expect(previous.activeRow == 2 && previous.activeColumn == 0, "occupied row step should wrap to previous non-empty row");
}

void TestOccupiedColumnSteppingSkipsEmptyCells()
{
	const iggy::ui::UiToolBeltState state { 3, 3, 2, 0 };

	const iggy::ui::UiToolBeltState next = iggy::ui::stepUiToolBeltColumnOccupied(state, 1, Items());
	const iggy::ui::UiToolBeltState previous = iggy::ui::stepUiToolBeltColumnOccupied(state, -1, Items());

	Expect(next.activeRow == 2 && next.activeColumn == 2, "occupied column step should skip empty cells");
	Expect(previous.activeRow == 2 && previous.activeColumn == 2, "occupied column negative step should wrap to occupied cell");
}

void TestViewCompactsOccupiedRowsAndActiveRowItems()
{
	const iggy::ui::UiToolBeltState state { 3, 3, 2, 2 };

	const iggy::ui::UiToolBeltView view = iggy::ui::uiToolBeltView(state, Items());

	Expect(view.rows.size() == 3, "tool belt view should include occupied rows");
	Expect(view.activeRowPosition == 2, "tool belt view should report compacted active row position");
	Expect(view.activeRowItems.size() == 2, "tool belt view should compact active row items");
	if (view.activeRowItems.size() == 2) {
		Expect(view.activeRowItems[0].column == 0 && !view.activeRowItems[0].active, "tool belt view should preserve inactive item column");
		Expect(view.activeRowItems[1].column == 2 && view.activeRowItems[1].active, "tool belt view should preserve active item column");
	}
}

void TestPinsDeduplicateRemoveAndPruneEmptyRows()
{
	const iggy::ui::UiToolBeltState state { 3, 3, 0, 0 };
	std::vector<int> pins;

	pins = iggy::ui::pinUiToolBeltRow(pins, 2);
	pins = iggy::ui::pinUiToolBeltRow(pins, 2);
	pins = iggy::ui::pinUiToolBeltRow(pins, 1);
	pins = iggy::ui::unpinUiToolBeltRow(pins, 2);
	pins = iggy::ui::pinUiToolBeltRow(pins, 2);
	const std::vector<int> pruned = iggy::ui::pruneUiToolBeltPins(pins, state, Items());

	Expect(pins.size() == 2, "tool belt pins should deduplicate and remove requested row");
	Expect(pruned.size() == 2 && pruned[0] == 1 && pruned[1] == 2, "tool belt pin prune should preserve occupied pinned rows");
}

void TestOperationsDoNotMutateInputs()
{
	const iggy::ui::UiToolBeltState state { 3, 3, 1, 0 };
	const std::vector<iggy::ResourceId> items = Items();
	const std::vector<int> pins { 0, 1, 2 };

	(void)iggy::ui::normalizeUiToolBeltToOccupied(state, items);
	(void)iggy::ui::stepUiToolBeltRowOccupied(state, 1, items);
	(void)iggy::ui::stepUiToolBeltColumnOccupied(state, 1, items);
	(void)iggy::ui::pruneUiToolBeltPins(pins, state, items);

	Expect(state.activeRow == 1 && state.activeColumn == 0, "tool belt operations should not mutate state input");
	Expect(items.size() == 9 && items[0] == Id("tool:select"), "tool belt operations should not mutate item input");
	Expect(pins.size() == 3, "tool belt operations should not mutate pin input");
}

} // namespace

int main()
{
	TestInvalidStateIsRejectedByQueries();
	TestSlotIndexAndOccupancyUseRowMajorItems();
	TestNormalizeMovesEmptyActiveCellToFirstOccupiedCell();
	TestOccupiedRowSteppingSkipsEmptyRowsAndWraps();
	TestOccupiedColumnSteppingSkipsEmptyCells();
	TestViewCompactsOccupiedRowsAndActiveRowItems();
	TestPinsDeduplicateRemoveAndPruneEmptyRows();
	TestOperationsDoNotMutateInputs();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
