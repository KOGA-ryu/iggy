#include "runtime3d/Runtime3DEntityState.hpp"

namespace iggy::runtime3d {

bool IsRuntime3DActorKind(Runtime3DEntityKind kind)
{
	return kind == Runtime3DEntityKind::Player ||
		kind == Runtime3DEntityKind::Ally ||
		kind == Runtime3DEntityKind::Enemy;
}

bool IsRuntime3DTargetableKind(Runtime3DEntityKind kind)
{
	return kind == Runtime3DEntityKind::Player ||
		kind == Runtime3DEntityKind::Ally ||
		kind == Runtime3DEntityKind::Enemy ||
		kind == Runtime3DEntityKind::Pickup ||
		kind == Runtime3DEntityKind::Door ||
		kind == Runtime3DEntityKind::Objective ||
		kind == Runtime3DEntityKind::TacticalMarker ||
		kind == Runtime3DEntityKind::CameraAnchor;
}

} // namespace iggy::runtime3d
