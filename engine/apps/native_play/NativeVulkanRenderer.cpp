#include "NativeVulkanRenderer.hpp"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>

namespace iggy::native_play {
namespace {

constexpr int MaxFramesInFlight = 2;
constexpr VkFormat DepthFormat = VK_FORMAT_D32_SFLOAT;

#ifndef IGGY_NATIVE_PLAY_SHADER_DIR
#define IGGY_NATIVE_PLAY_SHADER_DIR "."
#endif

struct Vertex3D {
	std::array<float, 3> position {};
	std::array<float, 3> color {};
};

struct PushConstants {
	Mat4 mvp;
	std::array<float, 4> tint { 1.0F, 1.0F, 1.0F, 1.0F };
};

const std::vector<Vertex3D> CubeVertices {
	{ { -0.5F, -0.5F, -0.5F }, { 0.10F, 0.55F, 0.95F } },
	{ { 0.5F, -0.5F, -0.5F }, { 0.25F, 0.80F, 0.95F } },
	{ { 0.5F, 0.5F, -0.5F }, { 0.95F, 0.75F, 0.25F } },
	{ { -0.5F, 0.5F, -0.5F }, { 0.90F, 0.35F, 0.50F } },
	{ { -0.5F, -0.5F, 0.5F }, { 0.25F, 0.70F, 0.45F } },
	{ { 0.5F, -0.5F, 0.5F }, { 0.70F, 0.45F, 0.95F } },
	{ { 0.5F, 0.5F, 0.5F }, { 0.95F, 0.55F, 0.20F } },
	{ { -0.5F, 0.5F, 0.5F }, { 0.85F, 0.85F, 0.45F } },
};

const std::vector<std::uint16_t> CubeIndices {
	0, 1, 2, 2, 3, 0,
	4, 6, 5, 6, 4, 7,
	0, 4, 5, 5, 1, 0,
	3, 2, 6, 6, 7, 3,
	1, 5, 6, 6, 2, 1,
	0, 3, 7, 7, 4, 0,
};

struct QueueFamilyIndices {
	std::optional<std::uint32_t> graphicsFamily;
	std::optional<std::uint32_t> presentFamily;

	[[nodiscard]] bool complete() const
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapchainSupport {
	VkSurfaceCapabilitiesKHR capabilities {};
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct NativeVulkanBufferResource {
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
};

struct NativeVulkanMeshResource {
	NativeVulkanBufferResource vertex;
	NativeVulkanBufferResource index;
	std::uint32_t indexCount = 0;
};

bool HasBuffer(const NativeVulkanBufferResource &buffer)
{
	return buffer.buffer != VK_NULL_HANDLE && buffer.memory != VK_NULL_HANDLE;
}

bool HasMesh(const NativeVulkanMeshResource &mesh)
{
	return HasBuffer(mesh.vertex) &&
		HasBuffer(mesh.index) &&
		mesh.indexCount > 0;
}

void ThrowIfFailed(VkResult result, const char *message)
{
	if (result != VK_SUCCESS)
		throw std::runtime_error(message);
}

std::vector<char> ReadBinaryFile(const std::filesystem::path &path)
{
	std::ifstream file(path, std::ios::ate | std::ios::binary);
	if (!file.is_open())
		throw std::runtime_error("failed to open shader file: " + path.string());

	const auto size = file.tellg();
	std::vector<char> buffer(static_cast<std::size_t>(size));
	file.seekg(0);
	file.read(buffer.data(), static_cast<std::streamsize>(size));
	return buffer;
}

std::filesystem::path ShaderPath(const char *filename)
{
	return std::filesystem::path(IGGY_NATIVE_PLAY_SHADER_DIR) / filename;
}

bool HasInstanceExtension(const std::vector<VkExtensionProperties> &available, const char *name)
{
	return std::any_of(available.begin(), available.end(), [name](const auto &extension) {
		return std::strcmp(extension.extensionName, name) == 0;
	});
}

std::vector<const char *> RequiredInstanceExtensions(SDL_Window *window)
{
	unsigned int sdlExtensionCount = 0;
	if (!SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, nullptr))
		throw std::runtime_error(SDL_GetError());

	std::vector<const char *> extensions(sdlExtensionCount);
	if (!SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, extensions.data()))
		throw std::runtime_error(SDL_GetError());

	std::uint32_t availableCount = 0;
	ThrowIfFailed(
		vkEnumerateInstanceExtensionProperties(nullptr, &availableCount, nullptr),
		"failed to enumerate Vulkan instance extensions");
	std::vector<VkExtensionProperties> available(availableCount);
	ThrowIfFailed(
		vkEnumerateInstanceExtensionProperties(nullptr, &availableCount, available.data()),
		"failed to read Vulkan instance extensions");

	if (HasInstanceExtension(available, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
		extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
	if (HasInstanceExtension(available, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
		extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

	return extensions;
}

std::vector<const char *> DeviceExtensionsFor(VkPhysicalDevice device)
{
	std::uint32_t extensionCount = 0;
	ThrowIfFailed(
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr),
		"failed to enumerate Vulkan device extensions");
	std::vector<VkExtensionProperties> available(extensionCount);
	ThrowIfFailed(
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, available.data()),
		"failed to read Vulkan device extensions");

	std::vector<const char *> extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	const bool hasPortabilitySubset =
		std::any_of(available.begin(), available.end(), [](const auto &extension) {
			return std::strcmp(extension.extensionName, "VK_KHR_portability_subset") == 0;
		});
	if (hasPortabilitySubset)
		extensions.push_back("VK_KHR_portability_subset");
	return extensions;
}

bool SupportsDeviceExtensions(VkPhysicalDevice device)
{
	std::uint32_t extensionCount = 0;
	ThrowIfFailed(
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr),
		"failed to enumerate Vulkan device extensions");
	std::vector<VkExtensionProperties> available(extensionCount);
	ThrowIfFailed(
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, available.data()),
		"failed to read Vulkan device extensions");

	std::set<std::string> required { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	for (const auto &extension : available)
		required.erase(extension.extensionName);
	return required.empty();
}

QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	QueueFamilyIndices indices;
	std::uint32_t familyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);
	std::vector<VkQueueFamilyProperties> families(familyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, families.data());

	for (std::uint32_t i = 0; i < familyCount; ++i) {
		if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
			indices.graphicsFamily = i;

		VkBool32 presentSupport = VK_FALSE;
		ThrowIfFailed(
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport),
			"failed to query Vulkan present support");
		if (presentSupport == VK_TRUE)
			indices.presentFamily = i;

		if (indices.complete())
			break;
	}
	return indices;
}

SwapchainSupport QuerySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	SwapchainSupport support;
	ThrowIfFailed(
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &support.capabilities),
		"failed to query swapchain capabilities");

	std::uint32_t formatCount = 0;
	ThrowIfFailed(
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr),
		"failed to query swapchain surface formats");
	support.formats.resize(formatCount);
	if (formatCount > 0) {
		ThrowIfFailed(
			vkGetPhysicalDeviceSurfaceFormatsKHR(
				device,
				surface,
				&formatCount,
				support.formats.data()),
			"failed to read swapchain surface formats");
	}

	std::uint32_t presentModeCount = 0;
	ThrowIfFailed(
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr),
		"failed to query swapchain present modes");
	support.presentModes.resize(presentModeCount);
	if (presentModeCount > 0) {
		ThrowIfFailed(
			vkGetPhysicalDeviceSurfacePresentModesKHR(
				device,
				surface,
				&presentModeCount,
				support.presentModes.data()),
			"failed to read swapchain present modes");
	}
	return support;
}

bool DeviceUsable(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	const QueueFamilyIndices indices = FindQueueFamilies(device, surface);
	if (!indices.complete() || !SupportsDeviceExtensions(device))
		return false;

	const SwapchainSupport support = QuerySwapchainSupport(device, surface);
	return !support.formats.empty() && !support.presentModes.empty();
}

int DeviceScore(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties properties {};
	VkPhysicalDeviceMemoryProperties memory {};
	vkGetPhysicalDeviceProperties(device, &properties);
	vkGetPhysicalDeviceMemoryProperties(device, &memory);

	int score = 0;
	if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		score += 10000;
	else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
		score += 5000;

	for (std::uint32_t i = 0; i < memory.memoryHeapCount; ++i) {
		if ((memory.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0)
			score += static_cast<int>(std::min<VkDeviceSize>(
				memory.memoryHeaps[i].size / (1024 * 1024),
				4096));
	}
	return score;
}

VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats)
{
	for (const auto &format : formats) {
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
				format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return format;
	}
	return formats.front();
}

VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR> &presentModes)
{
	for (const auto &mode : presentModes) {
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return mode;
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D ChooseExtent(SDL_Window *window, const VkSurfaceCapabilitiesKHR &capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max())
		return capabilities.currentExtent;

	int drawableWidth = 0;
	int drawableHeight = 0;
	SDL_Vulkan_GetDrawableSize(window, &drawableWidth, &drawableHeight);

	VkExtent2D extent {
		static_cast<std::uint32_t>(std::max(1, drawableWidth)),
		static_cast<std::uint32_t>(std::max(1, drawableHeight)),
	};
	extent.width = std::clamp(
		extent.width,
		capabilities.minImageExtent.width,
		capabilities.maxImageExtent.width);
	extent.height = std::clamp(
		extent.height,
		capabilities.minImageExtent.height,
		capabilities.maxImageExtent.height);
	return extent;
}

std::uint32_t FindMemoryType(
	VkPhysicalDevice physicalDevice,
	std::uint32_t typeFilter,
	VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memoryProperties {};
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	for (std::uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
		if ((typeFilter & (1U << i)) != 0 &&
				(memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	}
	throw std::runtime_error("failed to find suitable Vulkan memory type");
}

VkVertexInputBindingDescription CubeVertexBindingDescription()
{
	VkVertexInputBindingDescription description {};
	description.binding = 0;
	description.stride = sizeof(Vertex3D);
	description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return description;
}

std::array<VkVertexInputAttributeDescription, 2> CubeVertexAttributeDescriptions()
{
	std::array<VkVertexInputAttributeDescription, 2> descriptions {};
	descriptions[0].binding = 0;
	descriptions[0].location = 0;
	descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	descriptions[0].offset = offsetof(Vertex3D, position);
	descriptions[1].binding = 0;
	descriptions[1].location = 1;
	descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	descriptions[1].offset = offsetof(Vertex3D, color);
	return descriptions;
}

} // namespace

class NativeVulkanRenderer::Impl final {
public:
	void initialize(SDL_Window *window)
	{
		window_ = window;
		createInstance();
		if (!SDL_Vulkan_CreateSurface(window_, instance_, &surface_))
			throw std::runtime_error(SDL_GetError());
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapchain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createDepthResources();
		createFramebuffers();
		createCommandPool();
		createSceneMeshes();
		createCommandBuffers();
		createSyncObjects();
	}

	void drawFrame(const NativeVulkanFrameInput &input)
	{
		ThrowIfFailed(
			vkWaitForFences(device_, 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX),
			"failed to wait for frame fence");

		std::uint32_t imageIndex = 0;
		VkResult result = vkAcquireNextImageKHR(
			device_,
			swapchain_,
			UINT64_MAX,
			imageAvailableSemaphores_[currentFrame_],
			VK_NULL_HANDLE,
			&imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapchain();
			return;
		}
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
			throw std::runtime_error("failed to acquire swapchain image");

		ThrowIfFailed(
			vkResetFences(device_, 1, &inFlightFences_[currentFrame_]),
			"failed to reset frame fence");
		ThrowIfFailed(
			vkResetCommandBuffer(commandBuffers_[currentFrame_], 0),
			"failed to reset command buffer");
		recordCommandBuffer(commandBuffers_[currentFrame_], imageIndex, input);

		const VkSemaphore waitSemaphores[] = { imageAvailableSemaphores_[currentFrame_] };
		const VkSemaphore signalSemaphores[] = { renderFinishedSemaphores_[currentFrame_] };
		const VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		VkSubmitInfo submitInfo {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers_[currentFrame_];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		ThrowIfFailed(
			vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFences_[currentFrame_]),
			"failed to submit Vulkan frame");

		VkPresentInfoKHR presentInfo {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapchain_;
		presentInfo.pImageIndices = &imageIndex;

		result = vkQueuePresentKHR(presentQueue_, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR ||
				result == VK_SUBOPTIMAL_KHR ||
				framebufferResized_) {
			framebufferResized_ = false;
			recreateSwapchain();
		} else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present Vulkan frame");
		}

		currentFrame_ = (currentFrame_ + 1) % MaxFramesInFlight;
	}

	void markFramebufferResized()
	{
		framebufferResized_ = true;
	}

	float aspectRatio() const
	{
		if (swapchainExtent_.height == 0)
			return 1.0F;
		return static_cast<float>(swapchainExtent_.width) /
			static_cast<float>(swapchainExtent_.height);
	}

	void waitIdle()
	{
		if (device_ != VK_NULL_HANDLE)
			ThrowIfFailed(vkDeviceWaitIdle(device_), "failed to idle Vulkan device");
	}

	void cleanup()
	{
		if (device_ != VK_NULL_HANDLE)
			vkDeviceWaitIdle(device_);

		cleanupSwapchain();

		destroyMeshResource(cubeMesh_);

		for (std::size_t i = 0; i < imageAvailableSemaphores_.size(); ++i) {
			if (imageAvailableSemaphores_[i] != VK_NULL_HANDLE)
				vkDestroySemaphore(device_, imageAvailableSemaphores_[i], nullptr);
			if (renderFinishedSemaphores_[i] != VK_NULL_HANDLE)
				vkDestroySemaphore(device_, renderFinishedSemaphores_[i], nullptr);
			if (inFlightFences_[i] != VK_NULL_HANDLE)
				vkDestroyFence(device_, inFlightFences_[i], nullptr);
		}
		imageAvailableSemaphores_.clear();
		renderFinishedSemaphores_.clear();
		inFlightFences_.clear();

		if (commandPool_ != VK_NULL_HANDLE) {
			vkDestroyCommandPool(device_, commandPool_, nullptr);
			commandPool_ = VK_NULL_HANDLE;
		}
		if (device_ != VK_NULL_HANDLE) {
			vkDestroyDevice(device_, nullptr);
			device_ = VK_NULL_HANDLE;
		}
		if (surface_ != VK_NULL_HANDLE) {
			vkDestroySurfaceKHR(instance_, surface_, nullptr);
			surface_ = VK_NULL_HANDLE;
		}
		if (instance_ != VK_NULL_HANDLE) {
			vkDestroyInstance(instance_, nullptr);
			instance_ = VK_NULL_HANDLE;
		}
		window_ = nullptr;
	}

private:
	void createInstance()
	{
		const std::vector<const char *> extensions = RequiredInstanceExtensions(window_);

		VkApplicationInfo appInfo {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Iggy Native Play";
		appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
		appInfo.pEngineName = "Iggy";
		appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
		appInfo.apiVersion = VK_API_VERSION_1_2;

		VkInstanceCreateInfo createInfo {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();
		for (const char *extension : extensions) {
			if (std::strcmp(extension, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0)
				createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
		}

		ThrowIfFailed(vkCreateInstance(&createInfo, nullptr, &instance_), "failed to create Vulkan instance");
	}

	void pickPhysicalDevice()
	{
		std::uint32_t deviceCount = 0;
		ThrowIfFailed(
			vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr),
			"failed to enumerate Vulkan physical devices");
		if (deviceCount == 0)
			throw std::runtime_error("no Vulkan physical devices found");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		ThrowIfFailed(
			vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data()),
			"failed to read Vulkan physical devices");

		int bestScore = std::numeric_limits<int>::min();
		for (VkPhysicalDevice device : devices) {
			if (!DeviceUsable(device, surface_))
				continue;
			const int score = DeviceScore(device);
			if (score > bestScore) {
				bestScore = score;
				physicalDevice_ = device;
			}
		}

		if (physicalDevice_ == VK_NULL_HANDLE)
			throw std::runtime_error("no Vulkan device supports graphics, presentation, and swapchain");

		VkPhysicalDeviceProperties properties {};
		vkGetPhysicalDeviceProperties(physicalDevice_, &properties);
		std::cout << "Selected Vulkan device: " << properties.deviceName;
		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			std::cout << " (discrete)";
		else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
			std::cout << " (integrated)";
		std::cout << std::endl;
	}

	void createLogicalDevice()
	{
		queueFamilies_ = FindQueueFamilies(physicalDevice_, surface_);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		const std::set<std::uint32_t> uniqueFamilies = {
			*queueFamilies_.graphicsFamily,
			*queueFamilies_.presentFamily,
		};
		const float queuePriority = 1.0F;
		for (std::uint32_t family : uniqueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = family;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		const std::vector<const char *> deviceExtensions = DeviceExtensionsFor(physicalDevice_);
		VkPhysicalDeviceFeatures features {};
		VkDeviceCreateInfo createInfo {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<std::uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &features;
		createInfo.enabledExtensionCount = static_cast<std::uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		ThrowIfFailed(vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_), "failed to create Vulkan device");
		vkGetDeviceQueue(device_, *queueFamilies_.graphicsFamily, 0, &graphicsQueue_);
		vkGetDeviceQueue(device_, *queueFamilies_.presentFamily, 0, &presentQueue_);
	}

	void createSwapchain()
	{
		const SwapchainSupport support = QuerySwapchainSupport(physicalDevice_, surface_);
		const VkSurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(support.formats);
		const VkPresentModeKHR presentMode = ChoosePresentMode(support.presentModes);
		const VkExtent2D extent = ChooseExtent(window_, support.capabilities);

		std::uint32_t imageCount = support.capabilities.minImageCount + 1;
		if (support.capabilities.maxImageCount > 0)
			imageCount = std::min(imageCount, support.capabilities.maxImageCount);

		VkSwapchainCreateInfoKHR createInfo {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface_;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		const std::uint32_t queueFamilyIndices[] = {
			*queueFamilies_.graphicsFamily,
			*queueFamilies_.presentFamily,
		};
		if (queueFamilies_.graphicsFamily != queueFamilies_.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		} else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		createInfo.preTransform = support.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		ThrowIfFailed(
			vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapchain_),
			"failed to create Vulkan swapchain");

		ThrowIfFailed(
			vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, nullptr),
			"failed to query swapchain images");
		swapchainImages_.resize(imageCount);
		ThrowIfFailed(
			vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, swapchainImages_.data()),
			"failed to read swapchain images");

		swapchainImageFormat_ = surfaceFormat.format;
		swapchainExtent_ = extent;
	}

	void createImageViews()
	{
		swapchainImageViews_.resize(swapchainImages_.size());
		for (std::size_t i = 0; i < swapchainImages_.size(); ++i) {
			VkImageViewCreateInfo createInfo {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapchainImages_[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapchainImageFormat_;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			ThrowIfFailed(
				vkCreateImageView(device_, &createInfo, nullptr, &swapchainImageViews_[i]),
				"failed to create swapchain image view");
		}
	}

	void createRenderPass()
	{
		VkAttachmentDescription colorAttachment {};
		colorAttachment.format = swapchainImageFormat_;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentDescription depthAttachment {};
		depthAttachment.format = DepthFormat;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentRef {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef {};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDependency dependency {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo {};
		const std::array<VkAttachmentDescription, 2> attachments {
			colorAttachment,
			depthAttachment,
		};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<std::uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		ThrowIfFailed(
			vkCreateRenderPass(device_, &renderPassInfo, nullptr, &renderPass_),
			"failed to create Vulkan render pass");
	}

	VkShaderModule createShaderModule(const std::vector<char> &code)
	{
		VkShaderModuleCreateInfo createInfo {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const std::uint32_t *>(code.data());

		VkShaderModule shaderModule = VK_NULL_HANDLE;
		ThrowIfFailed(
			vkCreateShaderModule(device_, &createInfo, nullptr, &shaderModule),
			"failed to create Vulkan shader module");
		return shaderModule;
	}

	void createGraphicsPipeline()
	{
		const std::vector<char> vertexShaderCode =
			ReadBinaryFile(ShaderPath("cube.vert.spv"));
		const std::vector<char> fragmentShaderCode =
			ReadBinaryFile(ShaderPath("cube.frag.spv"));
		VkShaderModule vertexShader = createShaderModule(vertexShaderCode);
		VkShaderModule fragmentShader = createShaderModule(fragmentShaderCode);

		VkPipelineShaderStageCreateInfo vertexStage {};
		vertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertexStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexStage.module = vertexShader;
		vertexStage.pName = "main";

		VkPipelineShaderStageCreateInfo fragmentStage {};
		fragmentStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragmentStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentStage.module = fragmentShader;
		fragmentStage.pName = "main";

		const VkPipelineShaderStageCreateInfo shaderStages[] = {
			vertexStage,
			fragmentStage,
		};

		const VkVertexInputBindingDescription bindingDescription =
			CubeVertexBindingDescription();
		const auto attributeDescriptions = CubeVertexAttributeDescriptions();
		VkPipelineVertexInputStateCreateInfo vertexInput {};
		vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInput.vertexBindingDescriptionCount = 1;
		vertexInput.pVertexBindingDescriptions = &bindingDescription;
		vertexInput.vertexAttributeDescriptionCount =
			static_cast<std::uint32_t>(attributeDescriptions.size());
		vertexInput.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport {};
		viewport.x = 0.0F;
		viewport.y = 0.0F;
		viewport.width = static_cast<float>(swapchainExtent_.width);
		viewport.height = static_cast<float>(swapchainExtent_.height);
		viewport.minDepth = 0.0F;
		viewport.maxDepth = 1.0F;

		VkRect2D scissor {};
		scissor.offset = { 0, 0 };
		scissor.extent = swapchainExtent_;

		VkPipelineViewportStateCreateInfo viewportState {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0F;
		rasterizer.cullMode = VK_CULL_MODE_NONE;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling {};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo depthStencil {};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState colorBlendAttachment {};
		colorBlendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlending {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;

		VkPushConstantRange pushConstant {};
		pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstant.offset = 0;
		pushConstant.size = sizeof(PushConstants);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstant;
		ThrowIfFailed(
			vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipelineLayout_),
			"failed to create Vulkan pipeline layout");

		VkGraphicsPipelineCreateInfo pipelineInfo {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInput;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.layout = pipelineLayout_;
		pipelineInfo.renderPass = renderPass_;
		pipelineInfo.subpass = 0;

		ThrowIfFailed(
			vkCreateGraphicsPipelines(
				device_,
				VK_NULL_HANDLE,
				1,
				&pipelineInfo,
				nullptr,
				&graphicsPipeline_),
			"failed to create Vulkan graphics pipeline");

		vkDestroyShaderModule(device_, fragmentShader, nullptr);
		vkDestroyShaderModule(device_, vertexShader, nullptr);
	}

	void createImage(
		std::uint32_t width,
		std::uint32_t height,
		VkFormat format,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkImage &image,
		VkDeviceMemory &imageMemory)
	{
		VkImageCreateInfo imageInfo {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		ThrowIfFailed(
			vkCreateImage(device_, &imageInfo, nullptr, &image),
			"failed to create Vulkan image");

		VkMemoryRequirements memoryRequirements {};
		vkGetImageMemoryRequirements(device_, image, &memoryRequirements);

		VkMemoryAllocateInfo allocInfo {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memoryRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(
			physicalDevice_,
			memoryRequirements.memoryTypeBits,
			properties);

		ThrowIfFailed(
			vkAllocateMemory(device_, &allocInfo, nullptr, &imageMemory),
			"failed to allocate Vulkan image memory");
		ThrowIfFailed(
			vkBindImageMemory(device_, image, imageMemory, 0),
			"failed to bind Vulkan image memory");
	}

	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect)
	{
		VkImageViewCreateInfo viewInfo {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = aspect;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView = VK_NULL_HANDLE;
		ThrowIfFailed(
			vkCreateImageView(device_, &viewInfo, nullptr, &imageView),
			"failed to create Vulkan image view");
		return imageView;
	}

	void createDepthResources()
	{
		createImage(
			swapchainExtent_.width,
			swapchainExtent_.height,
			DepthFormat,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			depthImage_,
			depthImageMemory_);
		depthImageView_ = createImageView(
			depthImage_,
			DepthFormat,
			VK_IMAGE_ASPECT_DEPTH_BIT);
	}

	NativeVulkanBufferResource createBuffer(
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties)
	{
		NativeVulkanBufferResource buffer;
		VkBufferCreateInfo bufferInfo {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		ThrowIfFailed(
			vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer.buffer),
			"failed to create Vulkan buffer");

		VkMemoryRequirements memoryRequirements {};
		vkGetBufferMemoryRequirements(device_, buffer.buffer, &memoryRequirements);

		VkMemoryAllocateInfo allocInfo {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memoryRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(
			physicalDevice_,
			memoryRequirements.memoryTypeBits,
			properties);

		ThrowIfFailed(
			vkAllocateMemory(device_, &allocInfo, nullptr, &buffer.memory),
			"failed to allocate Vulkan buffer memory");
		ThrowIfFailed(
			vkBindBufferMemory(device_, buffer.buffer, buffer.memory, 0),
			"failed to bind Vulkan buffer memory");
		return buffer;
	}

	void copyToBuffer(
		const NativeVulkanBufferResource &buffer,
		const void *data,
		VkDeviceSize size)
	{
		void *mapped = nullptr;
		ThrowIfFailed(
			vkMapMemory(device_, buffer.memory, 0, size, 0, &mapped),
			"failed to map Vulkan buffer memory");
		std::memcpy(mapped, data, static_cast<std::size_t>(size));
		vkUnmapMemory(device_, buffer.memory);
	}

	NativeVulkanMeshResource createMeshResource(
		const std::vector<Vertex3D> &vertices,
		const std::vector<std::uint16_t> &indices)
	{
		NativeVulkanMeshResource mesh;
		if (vertices.empty() || indices.empty())
			return mesh;

		const VkDeviceSize vertexSize = sizeof(vertices[0]) * vertices.size();
		mesh.vertex = createBuffer(
			vertexSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		copyToBuffer(mesh.vertex, vertices.data(), vertexSize);

		const VkDeviceSize indexSize = sizeof(indices[0]) * indices.size();
		mesh.index = createBuffer(
			indexSize,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		copyToBuffer(mesh.index, indices.data(), indexSize);
		mesh.indexCount = static_cast<std::uint32_t>(indices.size());
		return mesh;
	}

	void createSceneMeshes()
	{
		cubeMesh_ = createMeshResource(CubeVertices, CubeIndices);
	}

	void createFramebuffers()
	{
		swapchainFramebuffers_.resize(swapchainImageViews_.size());
		for (std::size_t i = 0; i < swapchainImageViews_.size(); ++i) {
			const VkImageView attachments[] = {
				swapchainImageViews_[i],
				depthImageView_,
			};
			VkFramebufferCreateInfo framebufferInfo {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass_;
			framebufferInfo.attachmentCount = 2;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = swapchainExtent_.width;
			framebufferInfo.height = swapchainExtent_.height;
			framebufferInfo.layers = 1;

			ThrowIfFailed(
				vkCreateFramebuffer(device_, &framebufferInfo, nullptr, &swapchainFramebuffers_[i]),
				"failed to create Vulkan framebuffer");
		}
	}

	void createCommandPool()
	{
		VkCommandPoolCreateInfo poolInfo {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = *queueFamilies_.graphicsFamily;

		ThrowIfFailed(
			vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_),
			"failed to create Vulkan command pool");
	}

	void createCommandBuffers()
	{
		commandBuffers_.resize(MaxFramesInFlight);
		VkCommandBufferAllocateInfo allocInfo {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool_;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = static_cast<std::uint32_t>(commandBuffers_.size());

		ThrowIfFailed(
			vkAllocateCommandBuffers(device_, &allocInfo, commandBuffers_.data()),
			"failed to allocate Vulkan command buffers");
	}

	void createSyncObjects()
	{
		imageAvailableSemaphores_.resize(MaxFramesInFlight);
		renderFinishedSemaphores_.resize(MaxFramesInFlight);
		inFlightFences_.resize(MaxFramesInFlight);

		VkSemaphoreCreateInfo semaphoreInfo {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (int i = 0; i < MaxFramesInFlight; ++i) {
			ThrowIfFailed(
				vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &imageAvailableSemaphores_[i]),
				"failed to create image-available semaphore");
			ThrowIfFailed(
				vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &renderFinishedSemaphores_[i]),
				"failed to create render-finished semaphore");
			ThrowIfFailed(
				vkCreateFence(device_, &fenceInfo, nullptr, &inFlightFences_[i]),
				"failed to create in-flight fence");
		}
	}

	PushConstants pushConstantsForModel(
		const Mat4 &viewProjection,
		const Mat4 &model,
		std::array<float, 4> tint) const
	{
		PushConstants constants;
		constants.mvp = Multiply(viewProjection, model);
		constants.tint = tint;
		return constants;
	}

	void drawMesh(
		VkCommandBuffer commandBuffer,
		const NativeVulkanMeshResource &mesh,
		const Mat4 &viewProjection,
		const Mat4 &model,
		std::array<float, 4> tint) const
	{
		if (!HasMesh(mesh))
			return;

		const VkBuffer vertexBuffers[] = { mesh.vertex.buffer };
		const VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(
			commandBuffer,
			mesh.index.buffer,
			0,
			VK_INDEX_TYPE_UINT16);

		const PushConstants constants =
			pushConstantsForModel(viewProjection, model, tint);
		vkCmdPushConstants(
			commandBuffer,
			pipelineLayout_,
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(PushConstants),
			&constants);
		vkCmdDrawIndexed(
			commandBuffer,
			mesh.indexCount,
			1,
			0,
			0,
			0);
	}

	const NativeVulkanMeshResource *meshForSceneModel(NativeSceneModelId modelId) const
	{
		switch (modelId) {
		case NativeSceneModelId::Floor:
		case NativeSceneModelId::Wall:
		case NativeSceneModelId::NpcActor:
		case NativeSceneModelId::Player:
			return &cubeMesh_;
		}
		return nullptr;
	}

	void drawSceneDrawItems(
		VkCommandBuffer commandBuffer,
		const Mat4 &viewProjection,
		const std::vector<NativeSceneDrawItem> &drawItems) const
	{
		for (const NativeSceneDrawItem &item : drawItems) {
			const NativeVulkanMeshResource *mesh = meshForSceneModel(item.modelId);
			if (mesh == nullptr)
				continue;
			drawMesh(commandBuffer, *mesh, viewProjection, item.model, item.tint);
		}
	}

	void recordCommandBuffer(
		VkCommandBuffer commandBuffer,
		std::uint32_t imageIndex,
		const NativeVulkanFrameInput &input)
	{
		VkCommandBufferBeginInfo beginInfo {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		ThrowIfFailed(
			vkBeginCommandBuffer(commandBuffer, &beginInfo),
			"failed to begin Vulkan command buffer");

		std::array<VkClearValue, 2> clearValues {};
		clearValues[0].color = { { 0.035F, 0.045F, 0.070F, 1.0F } };
		clearValues[1].depthStencil = { 1.0F, 0 };

		VkRenderPassBeginInfo renderPassInfo {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass_;
		renderPassInfo.framebuffer = swapchainFramebuffers_[imageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapchainExtent_;
		renderPassInfo.clearValueCount = static_cast<std::uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);

		if (input.drawItems != nullptr)
			drawSceneDrawItems(commandBuffer, input.viewProjection, *input.drawItems);
		vkCmdEndRenderPass(commandBuffer);

		ThrowIfFailed(
			vkEndCommandBuffer(commandBuffer),
			"failed to record Vulkan command buffer");
	}

	void recreateSwapchain()
	{
		int width = 0;
		int height = 0;
		SDL_Vulkan_GetDrawableSize(window_, &width, &height);
		if (width <= 0 || height <= 0)
			return;

		ThrowIfFailed(vkDeviceWaitIdle(device_), "failed to idle Vulkan device");
		cleanupSwapchain();
		createSwapchain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createDepthResources();
		createFramebuffers();
	}

	void cleanupSwapchain()
	{
		for (VkFramebuffer framebuffer : swapchainFramebuffers_)
			vkDestroyFramebuffer(device_, framebuffer, nullptr);
		swapchainFramebuffers_.clear();

		if (depthImageView_ != VK_NULL_HANDLE) {
			vkDestroyImageView(device_, depthImageView_, nullptr);
			depthImageView_ = VK_NULL_HANDLE;
		}
		if (depthImage_ != VK_NULL_HANDLE) {
			vkDestroyImage(device_, depthImage_, nullptr);
			depthImage_ = VK_NULL_HANDLE;
		}
		if (depthImageMemory_ != VK_NULL_HANDLE) {
			vkFreeMemory(device_, depthImageMemory_, nullptr);
			depthImageMemory_ = VK_NULL_HANDLE;
		}

		if (graphicsPipeline_ != VK_NULL_HANDLE) {
			vkDestroyPipeline(device_, graphicsPipeline_, nullptr);
			graphicsPipeline_ = VK_NULL_HANDLE;
		}
		if (pipelineLayout_ != VK_NULL_HANDLE) {
			vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
			pipelineLayout_ = VK_NULL_HANDLE;
		}

		if (renderPass_ != VK_NULL_HANDLE) {
			vkDestroyRenderPass(device_, renderPass_, nullptr);
			renderPass_ = VK_NULL_HANDLE;
		}

		for (VkImageView imageView : swapchainImageViews_)
			vkDestroyImageView(device_, imageView, nullptr);
		swapchainImageViews_.clear();

		if (swapchain_ != VK_NULL_HANDLE) {
			vkDestroySwapchainKHR(device_, swapchain_, nullptr);
			swapchain_ = VK_NULL_HANDLE;
		}
	}

	void destroyBuffer(NativeVulkanBufferResource &buffer)
	{
		if (buffer.buffer != VK_NULL_HANDLE)
			vkDestroyBuffer(device_, buffer.buffer, nullptr);
		if (buffer.memory != VK_NULL_HANDLE)
			vkFreeMemory(device_, buffer.memory, nullptr);
		buffer = {};
	}

	void destroyMeshResource(NativeVulkanMeshResource &mesh)
	{
		destroyBuffer(mesh.index);
		destroyBuffer(mesh.vertex);
		mesh = {};
	}

	SDL_Window *window_ = nullptr;
	VkInstance instance_ = VK_NULL_HANDLE;
	VkSurfaceKHR surface_ = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
	VkDevice device_ = VK_NULL_HANDLE;
	VkQueue graphicsQueue_ = VK_NULL_HANDLE;
	VkQueue presentQueue_ = VK_NULL_HANDLE;
	QueueFamilyIndices queueFamilies_;
	VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
	std::vector<VkImage> swapchainImages_;
	VkFormat swapchainImageFormat_ = VK_FORMAT_UNDEFINED;
	VkExtent2D swapchainExtent_ {};
	std::vector<VkImageView> swapchainImageViews_;
	VkRenderPass renderPass_ = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
	VkPipeline graphicsPipeline_ = VK_NULL_HANDLE;
	VkImage depthImage_ = VK_NULL_HANDLE;
	VkDeviceMemory depthImageMemory_ = VK_NULL_HANDLE;
	VkImageView depthImageView_ = VK_NULL_HANDLE;
	std::vector<VkFramebuffer> swapchainFramebuffers_;
	VkCommandPool commandPool_ = VK_NULL_HANDLE;
	NativeVulkanMeshResource cubeMesh_;
	std::vector<VkCommandBuffer> commandBuffers_;
	std::vector<VkSemaphore> imageAvailableSemaphores_;
	std::vector<VkSemaphore> renderFinishedSemaphores_;
	std::vector<VkFence> inFlightFences_;
	std::size_t currentFrame_ = 0;
	bool framebufferResized_ = false;
};

NativeVulkanRenderer::NativeVulkanRenderer()
	: impl_(std::make_unique<Impl>())
{
}

NativeVulkanRenderer::~NativeVulkanRenderer() = default;

NativeVulkanRenderer::NativeVulkanRenderer(NativeVulkanRenderer &&) noexcept = default;

NativeVulkanRenderer &
NativeVulkanRenderer::operator=(NativeVulkanRenderer &&) noexcept = default;

void NativeVulkanRenderer::initialize(SDL_Window *window)
{
	impl_->initialize(window);
}

void NativeVulkanRenderer::drawFrame(const NativeVulkanFrameInput &input)
{
	impl_->drawFrame(input);
}

void NativeVulkanRenderer::markFramebufferResized()
{
	impl_->markFramebufferResized();
}

float NativeVulkanRenderer::aspectRatio() const
{
	return impl_->aspectRatio();
}

void NativeVulkanRenderer::waitIdle()
{
	impl_->waitIdle();
}

void NativeVulkanRenderer::cleanup()
{
	impl_->cleanup();
}

} // namespace iggy::native_play
