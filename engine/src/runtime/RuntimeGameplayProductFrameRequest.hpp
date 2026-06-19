#pragma once

#include <cstddef>

#include "runtime/RuntimeGameplayProductPlayMode.hpp"
#include "runtime/RuntimeGameplayProductPresentationCamera.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayProductFrameRequestStatus {
	Stepped,
	NotLoaded,
	NoFrameAvailable,
};

struct RuntimeGameplayProductFrameRequestInput {
	RuntimeGameplayProductPlayModeState state;
	RuntimeGameplayProductInputFrame2D inputFrame;
	bool allowFreePlayFrameWhenNoFrameAvailable = false;
	RuntimeGameplayProductPresentationCameraConfig presentationCamera;
};

struct RuntimeGameplayProductFrameRequestResult {
	RuntimeGameplayProductFrameRequestStatus status =
		RuntimeGameplayProductFrameRequestStatus::NotLoaded;
	RuntimeGameplayProductPresentationCameraResult presentationCamera;
	RuntimeGameplayProductPlayModeFrameResult frame;
	RuntimeGameplayProductPlayModeState state;
	std::size_t inputEventCount = 0;
	std::size_t ignoredInputEventCount = 0;
};

class RuntimeGameplayProductFrameRequest {
public:
	[[nodiscard]] RuntimeGameplayProductFrameRequestResult run(
		const RuntimeGameplayProductFrameRequestInput &input) const;
};

} // namespace iggy::runtime
