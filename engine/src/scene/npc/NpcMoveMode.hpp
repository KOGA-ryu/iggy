#pragma once

namespace iggy {

enum class NpcMoveMode {
	None,
	Still,
	Walk,
	Jog,
	Run,
	Sprint,
};

[[nodiscard]] bool npcMoveModeMoves(NpcMoveMode mode);
[[nodiscard]] float npcMoveModeSpeedMultiplier(NpcMoveMode mode);

} // namespace iggy
