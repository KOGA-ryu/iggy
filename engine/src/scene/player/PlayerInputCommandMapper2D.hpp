#pragma once

#include "core/resource/ResourceId.hpp"
#include "runtime/GameplayCommand2D.hpp"
#include "scene/player/PlayerInputIntent2D.hpp"

namespace iggy {

enum class PlayerInputCommandMapper2DStatus {
	Mapped,
	InvalidIntent,
	UnsupportedIntent,
};

struct PlayerInputCommandMapper2DResult {
	PlayerInputCommandMapper2DStatus status = PlayerInputCommandMapper2DStatus::UnsupportedIntent;
	runtime::GameplayCommand2D command;
	PlayerInputIntent2DStatus intentStatus = PlayerInputIntent2DStatus::Valid;
};

class PlayerInputCommandMapper2D {
public:
	[[nodiscard]] PlayerInputCommandMapper2DResult map(ResourceId actorId, PlayerInputIntent2D intent) const;
};

} // namespace iggy
