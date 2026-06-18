#include "scene/player/PlayerInputBinding2D.hpp"

namespace iggy {
namespace {

void AddIssue(
	PlayerInputBinding2DResult &result,
	std::size_t actionIndex,
	PlayerInputBindingIssueCode code,
	PlayerInputBindingAction2D action)
{
	PlayerInputBindingIssue2D issue;
	issue.actionIndex = actionIndex;
	issue.code = code;
	issue.action = action;
	result.issues.push_back(issue);
}

ResourceId ResolveTarget(
	const PlayerInputBindingContext2D &context,
	const PlayerInputBindingAction2D &action)
{
	if (!action.targetId.empty())
		return action.targetId;
	if (context.hasSelectedTargetId && !context.selectedTargetId.empty())
		return context.selectedTargetId;
	if (context.hasHoveredTargetId && !context.hoveredTargetId.empty())
		return context.hoveredTargetId;
	return {};
}

TileCoord TilePlus(TileCoord tile, TileCoord delta)
{
	return { tile.x + delta.x, tile.y + delta.y };
}

} // namespace

bool PlayerInputBinding2DResult::hasIssues() const
{
	return issueCount > 0;
}

PlayerInputBinding2DResult PlayerInputBinding2D::bind(
	const PlayerInputBindingContext2D &context,
	const std::vector<PlayerInputBindingAction2D> &actions) const
{
	PlayerInputBinding2DResult result;
	result.inputContext = context.input;
	result.actionCount = actions.size();

	for (std::size_t index = 0; index < actions.size(); ++index) {
		const PlayerInputBindingAction2D &action = actions[index];
		switch (action.type) {
		case PlayerInputBindingAction2DType::None:
			break;
		case PlayerInputBindingAction2DType::MoveToPoint:
			result.intents.push_back(playerMoveToPointIntent(action.worldPoint));
			break;
		case PlayerInputBindingAction2DType::MoveToTile:
			result.intents.push_back(playerMoveToTileIntent(action.tile));
			break;
		case PlayerInputBindingAction2DType::MoveByTileDelta:
			if (!context.hasCurrentPlayerTile) {
				AddIssue(
					result,
					index,
					PlayerInputBindingIssueCode::MissingCurrentPlayerTile,
					action);
				break;
			}
			result.intents.push_back(playerMoveToTileIntent(
				TilePlus(context.currentPlayerTile, action.tileDelta)));
			break;
		case PlayerInputBindingAction2DType::Interact: {
			const ResourceId target = ResolveTarget(context, action);
			if (target.empty()) {
				AddIssue(
					result,
					index,
					PlayerInputBindingIssueCode::MissingTarget,
					action);
				break;
			}
			result.intents.push_back(playerInteractIntent(target));
			break;
		}
		case PlayerInputBindingAction2DType::Inspect: {
			const ResourceId target = ResolveTarget(context, action);
			if (target.empty()) {
				AddIssue(
					result,
					index,
					PlayerInputBindingIssueCode::MissingTarget,
					action);
				break;
			}
			result.intents.push_back(playerInspectIntent(target));
			break;
		}
		case PlayerInputBindingAction2DType::Wait:
			result.intents.push_back(playerWaitIntent());
			break;
		case PlayerInputBindingAction2DType::Cancel:
			result.intents.push_back(playerCancelIntent());
			break;
		default:
			AddIssue(
				result,
				index,
				PlayerInputBindingIssueCode::UnsupportedAction,
				action);
			break;
		}
	}

	result.emittedIntentCount = result.intents.size();
	result.issueCount = result.issues.size();
	return result;
}

} // namespace iggy
