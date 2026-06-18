#include "runtime/RuntimeGameplayProductInputAdapter.hpp"

namespace iggy::runtime {
namespace {

PlayerInputBindingAction2D Action(PlayerInputBindingAction2DType type)
{
	PlayerInputBindingAction2D action;
	action.type = type;
	return action;
}

PlayerInputBindingAction2D MoveDelta(TileCoord delta)
{
	PlayerInputBindingAction2D action =
		Action(PlayerInputBindingAction2DType::MoveByTileDelta);
	action.tileDelta = delta;
	return action;
}

void AddIssue(
	RuntimeGameplayProductInputAdapterResult &result,
	std::size_t eventIndex,
	RuntimeGameplayProductInputAdapterIssueCode code,
	RuntimeGameplayProductInputEvent2D event)
{
	RuntimeGameplayProductInputAdapterIssue issue;
	issue.eventIndex = eventIndex;
	issue.code = code;
	issue.event = event;
	result.issues.push_back(issue);
}

} // namespace

bool RuntimeGameplayProductInputAdapterResult::hasIssues() const
{
	return issueCount > 0;
}

RuntimeGameplayProductInputAdapterResult RuntimeGameplayProductInputAdapter::map(
	const RuntimeGameplayProductInputFrame2D &frame) const
{
	RuntimeGameplayProductInputAdapterResult result;
	result.bindingContext = frame.bindingContext;
	result.eventCount = frame.events.size();

	for (std::size_t index = 0; index < frame.events.size(); ++index) {
		const RuntimeGameplayProductInputEvent2D &event = frame.events[index];
		if (event.kind == RuntimeGameplayProductInputEventKind::Released) {
			++result.ignoredReleaseCount;
			continue;
		}

		switch (event.control) {
		case RuntimeGameplayProductInputControl2D::None:
			break;
		case RuntimeGameplayProductInputControl2D::MoveNorth:
			result.actions.push_back(MoveDelta({ 0, -1 }));
			break;
		case RuntimeGameplayProductInputControl2D::MoveSouth:
			result.actions.push_back(MoveDelta({ 0, 1 }));
			break;
		case RuntimeGameplayProductInputControl2D::MoveWest:
			result.actions.push_back(MoveDelta({ -1, 0 }));
			break;
		case RuntimeGameplayProductInputControl2D::MoveEast:
			result.actions.push_back(MoveDelta({ 1, 0 }));
			break;
		case RuntimeGameplayProductInputControl2D::Interact: {
			PlayerInputBindingAction2D action =
				Action(PlayerInputBindingAction2DType::Interact);
			if (event.hasTargetId)
				action.targetId = event.targetId;
			result.actions.push_back(action);
			break;
		}
		case RuntimeGameplayProductInputControl2D::Inspect: {
			PlayerInputBindingAction2D action =
				Action(PlayerInputBindingAction2DType::Inspect);
			if (event.hasTargetId)
				action.targetId = event.targetId;
			result.actions.push_back(action);
			break;
		}
		case RuntimeGameplayProductInputControl2D::Wait:
			result.actions.push_back(Action(PlayerInputBindingAction2DType::Wait));
			break;
		case RuntimeGameplayProductInputControl2D::Cancel:
			result.actions.push_back(Action(PlayerInputBindingAction2DType::Cancel));
			break;
		case RuntimeGameplayProductInputControl2D::PrimaryPoint: {
			if (!event.hasWorldPoint) {
				AddIssue(
					result,
					index,
					RuntimeGameplayProductInputAdapterIssueCode::MissingWorldPoint,
					event);
				break;
			}
			PlayerInputBindingAction2D action =
				Action(PlayerInputBindingAction2DType::MoveToPoint);
			action.worldPoint = event.worldPoint;
			result.actions.push_back(action);
			break;
		}
		case RuntimeGameplayProductInputControl2D::PrimaryTile: {
			if (!event.hasTile) {
				AddIssue(
					result,
					index,
					RuntimeGameplayProductInputAdapterIssueCode::MissingTile,
					event);
				break;
			}
			PlayerInputBindingAction2D action =
				Action(PlayerInputBindingAction2DType::MoveToTile);
			action.tile = event.tile;
			result.actions.push_back(action);
			break;
		}
		default:
			AddIssue(
				result,
				index,
				RuntimeGameplayProductInputAdapterIssueCode::UnsupportedControl,
				event);
			break;
		}
	}

	result.emittedActionCount = result.actions.size();
	result.issueCount = result.issues.size();
	return result;
}

} // namespace iggy::runtime
