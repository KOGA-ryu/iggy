#pragma once

#include <vector>

#include "core/resource/ResourceId.hpp"

namespace iggy::runtime {

struct RuntimeInteractionRequiredItem {
	ResourceId targetId;
	ResourceId itemId;
};

using RuntimeInteractionRequiredItems = std::vector<RuntimeInteractionRequiredItem>;

} // namespace iggy::runtime
