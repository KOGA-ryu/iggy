#pragma once

#include <string>
#include <vector>

#include "runtime3d/Runtime3DEntityId.hpp"
#include "runtime3d/Runtime3DEntityState.hpp"
#include "runtime3d/Runtime3DTransform.hpp"
#include "runtime3d/Runtime3DWorldState.hpp"

namespace iggy::runtime3d {

struct Runtime3DSceneProjectionItem {
	Runtime3DEntityId entityId;
	Runtime3DEntityKind kind = Runtime3DEntityKind::Prop;
	Runtime3DTransform transform;
	std::string assetRef;
};

struct Runtime3DSceneProjection {
	std::vector<Runtime3DSceneProjectionItem> items;
};

[[nodiscard]] Runtime3DSceneProjection ProjectRuntime3DScene(const Runtime3DWorldState &world);

} // namespace iggy::runtime3d
