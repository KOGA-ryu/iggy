#include "scene/npc/NpcMoveMode.hpp"

namespace iggy {

bool npcMoveModeMoves(NpcMoveMode mode)
{
	return mode == NpcMoveMode::Walk
		|| mode == NpcMoveMode::Jog
		|| mode == NpcMoveMode::Run
		|| mode == NpcMoveMode::Sprint;
}

float npcMoveModeSpeedMultiplier(NpcMoveMode mode)
{
	switch (mode) {
	case NpcMoveMode::Walk:
		return 1.0F;
	case NpcMoveMode::Jog:
		return 1.5F;
	case NpcMoveMode::Run:
		return 2.0F;
	case NpcMoveMode::Sprint:
		return 3.0F;
	case NpcMoveMode::None:
	case NpcMoveMode::Still:
		return 0.0F;
	}

	return 0.0F;
}

} // namespace iggy
