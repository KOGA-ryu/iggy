#pragma once

#include "runtime/RuntimeSessionState.hpp"
#include "servers/physics2d/CollisionWorld2D.hpp"

namespace iggy::runtime {

enum class RuntimeCollisionWorldSource {
	Explicit,
	SessionCache,
	Empty,
};

struct RuntimeCollisionWorldRequest {
	const physics2d::CollisionWorld2D *explicitWorld = nullptr;
};

struct RuntimeCollisionWorldResult {
	RuntimeCollisionWorldSource source = RuntimeCollisionWorldSource::Empty;
	physics2d::CollisionWorld2D world;
};

class RuntimeCollisionWorldProvider {
public:
	[[nodiscard]] RuntimeCollisionWorldResult resolve(
		const RuntimeSessionState &session,
		RuntimeCollisionWorldRequest request) const;
};

} // namespace iggy::runtime
