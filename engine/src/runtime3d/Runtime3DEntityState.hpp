#pragma once

#include <string>

#include "runtime3d/Runtime3DCollisionVolume.hpp"
#include "runtime3d/Runtime3DEntityId.hpp"
#include "runtime3d/Runtime3DInteractionVolume.hpp"
#include "runtime3d/Runtime3DTransform.hpp"

namespace iggy::runtime3d {

enum class Runtime3DEntityKind {
	Player,
	Ally,
	Enemy,
	Pickup,
	Door,
	Wall,
	Floor,
	Prop,
	Objective,
	TacticalMarker,
	CameraAnchor,
};

struct Runtime3DEntityState {
	Runtime3DEntityId id;
	Runtime3DEntityKind kind = Runtime3DEntityKind::Prop;
	Runtime3DTransform transform;
	Runtime3DCollisionVolume collisionVolume;
	Runtime3DInteractionVolume interactionVolume;
	std::string assetRef;
	bool persistent = true;
};

[[nodiscard]] bool IsRuntime3DActorKind(Runtime3DEntityKind kind);
[[nodiscard]] bool IsRuntime3DTargetableKind(Runtime3DEntityKind kind);

} // namespace iggy::runtime3d
