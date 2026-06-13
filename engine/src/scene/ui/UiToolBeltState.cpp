#include "scene/ui/UiToolBeltState.hpp"

#include <cstddef>

namespace iggy::ui {
namespace {

int WrapIndex(int value, int size)
{
	if (size <= 0)
		return value;
	return ((value % size) + size) % size;
}

} // namespace

bool validUiToolBeltState(const UiToolBeltState &state)
{
	return state.rows > 0
		&& state.columns > 0
		&& state.activeRow >= 0
		&& state.activeRow < state.rows
		&& state.activeColumn >= 0
		&& state.activeColumn < state.columns;
}

int uiToolBeltSlotIndex(const UiToolBeltState &state, int row, int column)
{
	if (!validUiToolBeltState(state) || row < 0 || row >= state.rows || column < 0 || column >= state.columns)
		return -1;
	return row * state.columns + column;
}

int uiToolBeltActiveIndex(const UiToolBeltState &state)
{
	return uiToolBeltSlotIndex(state, state.activeRow, state.activeColumn);
}

bool uiToolBeltCellOccupied(const UiToolBeltState &state, const std::vector<ResourceId> &itemIds, int row, int column)
{
	const int index = uiToolBeltSlotIndex(state, row, column);
	if (index < 0 || static_cast<std::size_t>(index) >= itemIds.size())
		return false;
	return !itemIds[static_cast<std::size_t>(index)].empty();
}

int uiToolBeltRowLeadColumn(const UiToolBeltState &state, const std::vector<ResourceId> &itemIds, int row)
{
	if (!validUiToolBeltState(state) || row < 0 || row >= state.rows)
		return -1;
	for (int column = 0; column < state.columns; ++column) {
		if (uiToolBeltCellOccupied(state, itemIds, row, column))
			return column;
	}
	return -1;
}

UiToolBeltState normalizeUiToolBeltToOccupied(UiToolBeltState state, const std::vector<ResourceId> &itemIds)
{
	if (!validUiToolBeltState(state) || uiToolBeltCellOccupied(state, itemIds, state.activeRow, state.activeColumn))
		return state;
	for (int row = 0; row < state.rows; ++row) {
		const int lead = uiToolBeltRowLeadColumn(state, itemIds, row);
		if (lead >= 0) {
			state.activeRow = row;
			state.activeColumn = lead;
			return state;
		}
	}
	return state;
}

UiToolBeltState stepUiToolBeltRow(UiToolBeltState state, int delta)
{
	if (!validUiToolBeltState(state))
		return state;
	state.activeRow = WrapIndex(state.activeRow + delta, state.rows);
	return state;
}

UiToolBeltState stepUiToolBeltColumn(UiToolBeltState state, int delta)
{
	if (!validUiToolBeltState(state))
		return state;
	state.activeColumn = WrapIndex(state.activeColumn + delta, state.columns);
	return state;
}

UiToolBeltState stepUiToolBeltRowOccupied(UiToolBeltState state, int delta, const std::vector<ResourceId> &itemIds)
{
	if (!validUiToolBeltState(state) || delta == 0)
		return state;

	const int direction = delta > 0 ? 1 : -1;
	int remaining = delta * direction;
	int row = state.activeRow;
	while (remaining > 0) {
		bool found = false;
		for (int hops = 0; hops < state.rows; ++hops) {
			row = WrapIndex(row + direction, state.rows);
			if (uiToolBeltRowLeadColumn(state, itemIds, row) >= 0) {
				found = true;
				break;
			}
		}
		if (!found)
			return state;
		--remaining;
	}

	state.activeRow = row;
	state.activeColumn = uiToolBeltRowLeadColumn(state, itemIds, row);
	return state;
}

UiToolBeltState stepUiToolBeltColumnOccupied(UiToolBeltState state, int delta, const std::vector<ResourceId> &itemIds)
{
	if (!validUiToolBeltState(state) || delta == 0)
		return state;

	const int direction = delta > 0 ? 1 : -1;
	int remaining = delta * direction;
	int column = state.activeColumn;
	while (remaining > 0) {
		bool found = false;
		for (int hops = 0; hops < state.columns; ++hops) {
			column = WrapIndex(column + direction, state.columns);
			if (uiToolBeltCellOccupied(state, itemIds, state.activeRow, column)) {
				found = true;
				break;
			}
		}
		if (!found)
			return state;
		--remaining;
	}

	state.activeColumn = column;
	return state;
}

UiToolBeltView uiToolBeltView(const UiToolBeltState &state, const std::vector<ResourceId> &itemIds)
{
	UiToolBeltView view;
	if (!validUiToolBeltState(state))
		return view;

	for (int row = 0; row < state.rows; ++row) {
		const int lead = uiToolBeltRowLeadColumn(state, itemIds, row);
		if (lead < 0)
			continue;
		if (row == state.activeRow)
			view.activeRowPosition = static_cast<int>(view.rows.size());
		view.rows.push_back({ row, lead });
	}

	if (view.activeRowPosition < 0)
		return view;

	for (int column = 0; column < state.columns; ++column) {
		if (uiToolBeltCellOccupied(state, itemIds, state.activeRow, column))
			view.activeRowItems.push_back({ column, column == state.activeColumn });
	}
	return view;
}

std::vector<int> pinUiToolBeltRow(std::vector<int> pins, int row)
{
	if (row < 0)
		return pins;
	for (const int pinned : pins) {
		if (pinned == row)
			return pins;
	}
	pins.push_back(row);
	return pins;
}

std::vector<int> unpinUiToolBeltRow(std::vector<int> pins, int row)
{
	std::vector<int> kept;
	kept.reserve(pins.size());
	for (const int pinned : pins) {
		if (pinned != row)
			kept.push_back(pinned);
	}
	return kept;
}

std::vector<int> pruneUiToolBeltPins(std::vector<int> pins, const UiToolBeltState &state, const std::vector<ResourceId> &itemIds)
{
	std::vector<int> kept;
	kept.reserve(pins.size());
	for (const int pinned : pins) {
		if (uiToolBeltRowLeadColumn(state, itemIds, pinned) >= 0)
			kept.push_back(pinned);
	}
	return kept;
}

} // namespace iggy::ui
