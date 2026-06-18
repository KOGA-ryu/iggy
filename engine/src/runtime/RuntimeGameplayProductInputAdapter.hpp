#pragma once

#include <cstddef>
#include <vector>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "scene/level/TileCoord.hpp"
#include "scene/player/PlayerInputBinding2D.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayProductInputControl2D {
	None,
	MoveNorth,
	MoveSouth,
	MoveWest,
	MoveEast,
	Interact,
	Inspect,
	Wait,
	Cancel,
	PrimaryPoint,
	PrimaryTile,
};

enum class RuntimeGameplayProductInputEventKind {
	Pressed,
	Released,
};

struct RuntimeGameplayProductInputEvent2D {
	RuntimeGameplayProductInputControl2D control =
		RuntimeGameplayProductInputControl2D::None;
	RuntimeGameplayProductInputEventKind kind =
		RuntimeGameplayProductInputEventKind::Pressed;
	bool hasTile = false;
	TileCoord tile;
	bool hasWorldPoint = false;
	Vec2 worldPoint;
	bool hasTargetId = false;
	ResourceId targetId;
};

struct RuntimeGameplayProductInputFrame2D {
	PlayerInputBindingContext2D bindingContext;
	std::vector<RuntimeGameplayProductInputEvent2D> events;
};

enum class RuntimeGameplayProductInputAdapterIssueCode {
	UnsupportedControl,
	MissingTile,
	MissingWorldPoint,
};

struct RuntimeGameplayProductInputAdapterIssue {
	std::size_t eventIndex = 0;
	RuntimeGameplayProductInputAdapterIssueCode code =
		RuntimeGameplayProductInputAdapterIssueCode::UnsupportedControl;
	RuntimeGameplayProductInputEvent2D event;
};

struct RuntimeGameplayProductInputAdapterResult {
	PlayerInputBindingContext2D bindingContext;
	std::vector<PlayerInputBindingAction2D> actions;
	std::vector<RuntimeGameplayProductInputAdapterIssue> issues;
	std::size_t eventCount = 0;
	std::size_t emittedActionCount = 0;
	std::size_t ignoredReleaseCount = 0;
	std::size_t issueCount = 0;

	[[nodiscard]] bool hasIssues() const;
};

class RuntimeGameplayProductInputAdapter {
public:
	[[nodiscard]] RuntimeGameplayProductInputAdapterResult map(
		const RuntimeGameplayProductInputFrame2D &frame) const;
};

} // namespace iggy::runtime
