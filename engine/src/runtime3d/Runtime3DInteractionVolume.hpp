#pragma once

#include "core/math/Aabb3.hpp"

namespace iggy::runtime3d {

enum class Runtime3DInteractionVolumeKind {
	None,
	Aabb,
};

struct Runtime3DInteractionVolume {
	Runtime3DInteractionVolumeKind kind = Runtime3DInteractionVolumeKind::None;
	iggy::Aabb3 bounds;
	float reachRadius = 0.0F;
};

} // namespace iggy::runtime3d
