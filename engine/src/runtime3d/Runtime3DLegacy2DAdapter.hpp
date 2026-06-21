#pragma once

#include <string>

#include "runtime3d/Runtime3DSessionState.hpp"

namespace iggy::runtime3d {

struct Runtime3DLegacy2DAdapterResult {
	bool converted = false;
	Runtime3DSessionState state;
	std::string status = "legacy 2d adapter placeholder";
};

[[nodiscard]] Runtime3DLegacy2DAdapterResult BuildRuntime3DLegacy2DAdapterPlaceholder();

} // namespace iggy::runtime3d
