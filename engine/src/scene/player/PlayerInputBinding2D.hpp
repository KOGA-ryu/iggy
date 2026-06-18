#pragma once

#include <cstddef>
#include <vector>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "scene/level/TileCoord.hpp"
#include "scene/player/PlayerInputIntentGate2D.hpp"

namespace iggy {

enum class PlayerInputBindingAction2DType {
	None,
	MoveToPoint,
	MoveToTile,
	MoveByTileDelta,
	Interact,
	Inspect,
	Wait,
	Cancel,
};

struct PlayerInputBindingAction2D {
	PlayerInputBindingAction2DType type =
		PlayerInputBindingAction2DType::None;
	Vec2 worldPoint;
	TileCoord tile;
	TileCoord tileDelta;
	ResourceId targetId;
};

enum class PlayerInputBindingIssueCode {
	MissingCurrentPlayerTile,
	MissingTarget,
	UnsupportedAction,
};

struct PlayerInputBindingIssue2D {
	std::size_t actionIndex = 0;
	PlayerInputBindingIssueCode code =
		PlayerInputBindingIssueCode::UnsupportedAction;
	PlayerInputBindingAction2D action;
};

struct PlayerInputBindingContext2D {
	PlayerInputContext2D input;
	bool hasCurrentPlayerTile = false;
	TileCoord currentPlayerTile;
	bool hasSelectedTargetId = false;
	ResourceId selectedTargetId;
	bool hasHoveredTargetId = false;
	ResourceId hoveredTargetId;
};

struct PlayerInputBinding2DResult {
	PlayerInputContext2D inputContext;
	std::vector<PlayerInputIntent2D> intents;
	std::vector<PlayerInputBindingIssue2D> issues;
	std::size_t actionCount = 0;
	std::size_t emittedIntentCount = 0;
	std::size_t issueCount = 0;

	[[nodiscard]] bool hasIssues() const;
};

class PlayerInputBinding2D {
public:
	[[nodiscard]] PlayerInputBinding2DResult bind(
		const PlayerInputBindingContext2D &context,
		const std::vector<PlayerInputBindingAction2D> &actions) const;
};

} // namespace iggy
