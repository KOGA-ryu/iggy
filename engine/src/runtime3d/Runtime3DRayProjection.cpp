#include "runtime3d/Runtime3DRayProjection.hpp"

namespace iggy::runtime3d {

Runtime3DRayProjectionResult ProjectRuntime3DRayToGroundPlane(Runtime3DRayProjectionRequest request)
{
	if (request.ray.direction.y == 0.0F)
		return { false, {}, 0.0F, "ray parallel to ground" };

	const float distance = (request.groundY - request.ray.origin.y) / request.ray.direction.y;
	if (distance < 0.0F || distance > request.maxDistance)
		return { false, {}, distance, "ground outside ray range" };

	return { true, request.ray.pointAtDistance(distance), distance, "ground hit" };
}

} // namespace iggy::runtime3d
