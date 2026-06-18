#include "runtime/RuntimeGameplayProductInputAccumulator.hpp"

#include <algorithm>

namespace iggy::runtime {
namespace {

bool IsHeldMovementControl(RuntimeGameplayProductInputControl2D control)
{
	switch (control) {
	case RuntimeGameplayProductInputControl2D::MoveNorth:
	case RuntimeGameplayProductInputControl2D::MoveSouth:
	case RuntimeGameplayProductInputControl2D::MoveWest:
	case RuntimeGameplayProductInputControl2D::MoveEast:
		return true;
	case RuntimeGameplayProductInputControl2D::None:
	case RuntimeGameplayProductInputControl2D::Interact:
	case RuntimeGameplayProductInputControl2D::Inspect:
	case RuntimeGameplayProductInputControl2D::Wait:
	case RuntimeGameplayProductInputControl2D::Cancel:
	case RuntimeGameplayProductInputControl2D::PrimaryPoint:
	case RuntimeGameplayProductInputControl2D::PrimaryTile:
		break;
	}
	return false;
}

bool IsOneShotControl(RuntimeGameplayProductInputControl2D control)
{
	switch (control) {
	case RuntimeGameplayProductInputControl2D::Interact:
	case RuntimeGameplayProductInputControl2D::Inspect:
	case RuntimeGameplayProductInputControl2D::Wait:
	case RuntimeGameplayProductInputControl2D::Cancel:
		return true;
	case RuntimeGameplayProductInputControl2D::None:
	case RuntimeGameplayProductInputControl2D::MoveNorth:
	case RuntimeGameplayProductInputControl2D::MoveSouth:
	case RuntimeGameplayProductInputControl2D::MoveWest:
	case RuntimeGameplayProductInputControl2D::MoveEast:
	case RuntimeGameplayProductInputControl2D::PrimaryPoint:
	case RuntimeGameplayProductInputControl2D::PrimaryTile:
		break;
	}
	return false;
}

RuntimeGameplayProductInputEvent2D PressedEvent(
	RuntimeGameplayProductInputControl2D control)
{
	RuntimeGameplayProductInputEvent2D event;
	event.control = control;
	event.kind = RuntimeGameplayProductInputEventKind::Pressed;
	return event;
}

} // namespace

RuntimeGameplayProductInputAccumulatorRecordResult
RuntimeGameplayProductInputAccumulator::record(
	const RuntimeGameplayProductInputAccumulatorState &state,
	const RuntimeGameplayProductInputEvent2D &event) const
{
	RuntimeGameplayProductInputAccumulatorRecordResult result;
	result.state = state;

	if (IsHeldMovementControl(event.control)) {
		auto found = std::find(
			result.state.heldControls.begin(),
			result.state.heldControls.end(),
			event.control);
		if (event.kind == RuntimeGameplayProductInputEventKind::Pressed) {
			if (found == result.state.heldControls.end()) {
				result.state.heldControls.push_back(event.control);
				result.changed = true;
			}
		} else if (found != result.state.heldControls.end()) {
			result.state.heldControls.erase(found);
			result.changed = true;
		}
	} else if (IsOneShotControl(event.control) &&
		event.kind == RuntimeGameplayProductInputEventKind::Pressed) {
		result.state.pendingOneShotEvents.push_back(event);
		result.changed = true;
	}

	result.heldControlCount = result.state.heldControls.size();
	result.pendingOneShotCount = result.state.pendingOneShotEvents.size();
	return result;
}

RuntimeGameplayProductInputAccumulatorFrameResult
RuntimeGameplayProductInputAccumulator::buildFrame(
	const RuntimeGameplayProductInputAccumulatorFrameInput &input) const
{
	RuntimeGameplayProductInputAccumulatorFrameResult result;
	result.state = input.state;
	result.frame.bindingContext = input.bindingContext;

	for (RuntimeGameplayProductInputControl2D control :
		input.state.heldControls) {
		result.frame.events.push_back(PressedEvent(control));
	}
	result.heldEventCount = result.frame.events.size();

	result.frame.events.insert(
		result.frame.events.end(),
		input.state.pendingOneShotEvents.begin(),
		input.state.pendingOneShotEvents.end());
	result.oneShotEventCount = input.state.pendingOneShotEvents.size();
	result.eventCount = result.frame.events.size();
	result.state.pendingOneShotEvents.clear();
	return result;
}

RuntimeGameplayProductInputAccumulatorState
RuntimeGameplayProductInputAccumulator::clear(
	const RuntimeGameplayProductInputAccumulatorState &) const
{
	return {};
}

} // namespace iggy::runtime
