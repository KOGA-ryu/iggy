#pragma once

#include <vector>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "scene/level/TileCoord.hpp"

namespace iggy::runtime {

enum class GameplayCommand2DType {
	None,
	MoveToPoint,
	MoveToTile,
	Interact,
	Wait,
};

enum class GameplayCommand2DStatus {
	Valid,
	MissingTarget,
};

struct GameplayCommand2D {
	GameplayCommand2DType type = GameplayCommand2DType::None;
	ResourceId actorId;
	Vec2 targetPoint;
	TileCoord targetTile;
	ResourceId targetId;
};

struct GameplayCommandFrame2D {
	std::vector<GameplayCommand2D> commands;
};

class GameplayCommand2DFactory {
public:
	[[nodiscard]] GameplayCommand2D none(ResourceId actorId = {}) const;
	[[nodiscard]] GameplayCommand2D moveToPoint(ResourceId actorId, Vec2 targetPoint) const;
	[[nodiscard]] GameplayCommand2D moveToTile(ResourceId actorId, TileCoord targetTile) const;
	[[nodiscard]] GameplayCommand2D interact(ResourceId actorId, ResourceId targetId) const;
	[[nodiscard]] GameplayCommand2D wait(ResourceId actorId = {}) const;
};

[[nodiscard]] GameplayCommand2DStatus validate(GameplayCommand2D command);
[[nodiscard]] bool valid(GameplayCommand2D command);

} // namespace iggy::runtime
