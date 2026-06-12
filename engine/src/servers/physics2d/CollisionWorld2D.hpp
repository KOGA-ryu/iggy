#pragma once

#include <cstddef>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "servers/physics2d/CollisionShape2D.hpp"

namespace iggy::physics2d {

struct CollisionObject2D {
	ResourceId id;
	CollisionShape2D shape;
	bool solid = true;
};

class CollisionWorld2D {
public:
	CollisionWorld2D() = default;
	explicit CollisionWorld2D(std::vector<CollisionObject2D> objects);

	[[nodiscard]] const std::vector<CollisionObject2D> &objects() const;

private:
	std::vector<CollisionObject2D> objects_;
};

enum class CollisionWorldBuildIssueCode {
	InvalidShape,
};

struct CollisionWorldBuildIssue {
	CollisionWorldBuildIssueCode code = CollisionWorldBuildIssueCode::InvalidShape;
	std::size_t index = 0;
	CollisionObject2D object;
};

struct CollisionWorldBuildResult {
	bool built = false;
	CollisionWorld2D world;
	std::vector<CollisionWorldBuildIssue> issues;
};

class CollisionWorld2DBuilder {
public:
	[[nodiscard]] CollisionWorldBuildResult build(std::vector<CollisionObject2D> objects) const;
};

} // namespace iggy::physics2d
