#pragma once

namespace iggy {

enum class NpcMoveMode2D {
	None,
	Still,
	Walk,
	Jog,
	Run,
	Sprint,
};

[[nodiscard]] bool npcMoveModeMoves(NpcMoveMode2D mode);
[[nodiscard]] float npcMoveModeSpeedMultiplier(NpcMoveMode2D mode);

} // namespace iggy
