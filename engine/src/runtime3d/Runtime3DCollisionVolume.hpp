#pragma once

#include "core/math/Aabb3.hpp"

namespace iggy::runtime3d {

enum class Runtime3DCollisionVolumeKind {
	None,
	Aabb,
	CapsulePlaceholder,
};

struct Runtime3DCollisionVolume {
	Runtime3DCollisionVolumeKind kind = Runtime3DCollisionVolumeKind::None;
	iggy::Aabb3 bounds;
};

} // namespace iggy::runtime3d
