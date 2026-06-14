#include "scene/npc/NpcMoveMode2D.hpp"

namespace iggy {

bool npcMoveModeMoves(NpcMoveMode2D mode)
{
	return mode == NpcMoveMode2D::Walk
		|| mode == NpcMoveMode2D::Jog
		|| mode == NpcMoveMode2D::Run
		|| mode == NpcMoveMode2D::Sprint;
}

float npcMoveModeSpeedMultiplier(NpcMoveMode2D mode)
{
	switch (mode) {
	case NpcMoveMode2D::Walk:
		return 1.0F;
	case NpcMoveMode2D::Jog:
		return 1.5F;
	case NpcMoveMode2D::Run:
		return 2.0F;
	case NpcMoveMode2D::Sprint:
		return 3.0F;
	case NpcMoveMode2D::None:
	case NpcMoveMode2D::Still:
		return 0.0F;
	}

	return 0.0F;
}

} // namespace iggy
