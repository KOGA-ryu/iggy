#pragma once

#include "core/math/Aabb2.hpp"
#include "scene/camera/CameraState.hpp"

namespace iggy {

struct CameraClampResult {
	CameraState state;
	bool clamped = false;
};

class CameraClamp {
public:
	[[nodiscard]] CameraClampResult clamp(CameraState state, Aabb2 bounds) const;
};

} // namespace iggy
