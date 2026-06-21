#pragma once

#include <string>

#include "core/math/Ray3.hpp"
#include "core/math/Vec3.hpp"

namespace iggy::runtime3d {

struct Runtime3DRayProjectionRequest {
	iggy::Ray3 ray;
	float maxDistance = 1000.0F;
	float groundY = 0.0F;
};

struct Runtime3DRayProjectionResult {
	bool hit = false;
	iggy::Vec3 point;
	float distance = 0.0F;
	std::string status;
};

[[nodiscard]] Runtime3DRayProjectionResult ProjectRuntime3DRayToGroundPlane(Runtime3DRayProjectionRequest request);

} // namespace iggy::runtime3d
