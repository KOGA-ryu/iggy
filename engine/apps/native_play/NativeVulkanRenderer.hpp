#pragma once

#include <memory>
#include <vector>

#include "NativePlayMath.hpp"
#include "NativeSceneDrawList.hpp"

struct SDL_Window;

namespace iggy::native_play {

struct NativeVulkanFrameInput {
	Mat4 viewProjection;
	const std::vector<NativeSceneDrawItem> *drawItems = nullptr;
};

class NativeVulkanRenderer {
public:
	NativeVulkanRenderer();
	~NativeVulkanRenderer();

	NativeVulkanRenderer(const NativeVulkanRenderer &) = delete;
	NativeVulkanRenderer &operator=(const NativeVulkanRenderer &) = delete;
	NativeVulkanRenderer(NativeVulkanRenderer &&) noexcept;
	NativeVulkanRenderer &operator=(NativeVulkanRenderer &&) noexcept;

	void initialize(SDL_Window *window);
	void drawFrame(const NativeVulkanFrameInput &input);
	void markFramebufferResized();
	[[nodiscard]] float aspectRatio() const;
	void waitIdle();
	void cleanup();

private:
	class Impl;
	std::unique_ptr<Impl> impl_;
};

} // namespace iggy::native_play
