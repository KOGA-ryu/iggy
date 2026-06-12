#include "servers/physics2d/CollisionWorld2D.hpp"

#include <utility>

namespace iggy::physics2d {

CollisionWorld2D::CollisionWorld2D(std::vector<CollisionObject2D> objects)
	: objects_(std::move(objects))
{
}

const std::vector<CollisionObject2D> &CollisionWorld2D::objects() const
{
	return objects_;
}

CollisionWorldBuildResult CollisionWorld2DBuilder::build(std::vector<CollisionObject2D> objects) const
{
	CollisionWorldBuildResult result;

	for (std::size_t index = 0; index < objects.size(); ++index) {
		if (isValid(objects[index].shape))
			continue;

		result.issues.push_back({ CollisionWorldBuildIssueCode::InvalidShape, index, objects[index] });
	}

	if (!result.issues.empty())
		return result;

	result.built = true;
	result.world = CollisionWorld2D(std::move(objects));
	return result;
}

} // namespace iggy::physics2d
