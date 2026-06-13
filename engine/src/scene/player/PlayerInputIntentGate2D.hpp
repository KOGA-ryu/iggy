#pragma once

#include "scene/player/PlayerInputIntent2D.hpp"

namespace iggy {

enum class PlayerInputIntentBlockReason {
	None,
	InvalidIntent,
	PlayerControlDisabled,
	WorldInputDisabled,
	InteractionDisabled,
	CancelDisabled,
};

struct PlayerInputContext2D {
	bool playerControlEnabled = true;
	bool worldInputEnabled = true;
	bool interactionEnabled = true;
	bool cancelEnabled = true;
};

struct PlayerInputIntentGate2DResult {
	bool accepted = true;
	PlayerInputIntentBlockReason reason = PlayerInputIntentBlockReason::None;
	PlayerInputIntent2DStatus intentStatus = PlayerInputIntent2DStatus::Valid;
	PlayerInputIntent2D intent;
};

class PlayerInputIntentGate2D {
public:
	[[nodiscard]] PlayerInputIntentGate2DResult evaluate(const PlayerInputContext2D &context, PlayerInputIntent2D intent) const;
};

} // namespace iggy
