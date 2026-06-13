#pragma once

#include "scene/interaction/InteractionEffectCatalog2D.hpp"
#include "scene/interaction/InteractionTarget2D.hpp"

namespace iggy::runtime {

struct RuntimeInteractionState {
	InteractionTarget2DRegistry targets;
	InteractionEffectCatalog2D effects;
};

} // namespace iggy::runtime
