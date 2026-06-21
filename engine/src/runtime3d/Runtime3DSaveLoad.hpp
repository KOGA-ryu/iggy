#pragma once

#include <string>

#include "runtime3d/Runtime3DSaveEnvelope.hpp"
#include "runtime3d/Runtime3DSessionState.hpp"

namespace iggy::runtime3d {

enum class Runtime3DSaveLoadStatus {
	Unsupported,
	Saved,
	Loaded,
};

struct Runtime3DSaveLoadResult {
	Runtime3DSaveLoadStatus status = Runtime3DSaveLoadStatus::Unsupported;
	Runtime3DSessionState state;
	std::string reason = "runtime3d save encoding not implemented";
};

[[nodiscard]] Runtime3DSaveLoadResult SaveRuntime3DSessionPlaceholder(const Runtime3DSessionState &state);
[[nodiscard]] Runtime3DSaveLoadResult LoadRuntime3DSessionPlaceholder(const Runtime3DSaveEnvelope &envelope);

} // namespace iggy::runtime3d
