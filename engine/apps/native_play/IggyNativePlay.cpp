#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "runtime/RuntimeGameplayProductLoop.hpp"
#include "runtime/RuntimeGameplayProductPlayMode.hpp"
#include "runtime/RuntimeGameplayProductScenarioLoader.hpp"

namespace {

constexpr int InitialWindowWidth = 1280;
constexpr int InitialWindowHeight = 720;
constexpr int MaxFramesInFlight = 2;

struct LaunchOptions {
	bool showHelp = false;
	bool hasPlayPath = false;
	std::filesystem::path playPath;
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

struct ProductLoadState {
	iggy::runtime::RuntimeGameplayProductScenarioLoadResult load;
	iggy::runtime::RuntimeGameplayProductLoopBuildResult loop;
	iggy::runtime::RuntimeGameplayProductPlayModeBuildResult play;
};

void ThrowIfFailed(VkResult result, const char *message)
{
	if (result != VK_SUCCESS)
		throw std::runtime_error(message);
}

LaunchOptions ParseArgs(int argc, char **argv)
{
	LaunchOptions options;
	for (int i = 1; i < argc; ++i) {
		const std::string arg = argv[i];
		if (arg == "--help" || arg == "-h") {
			options.showHelp = true;
			continue;
		}
		if (arg == "--play") {
			if (i + 1 >= argc)
				throw std::runtime_error("--play requires a scenario path");
			options.hasPlayPath = true;
			options.playPath = argv[++i];
			continue;
		}
		throw std::runtime_error("unknown argument: " + arg);
	}
	return options;
}

void PrintUsage()
{
	std::cout
		<< "Usage: iggy_native_play [--play PATH]\n"
		<< "\n"
		<< "Opens the native SDL/Vulkan play shell. This first native shell\n"
		<< "validates --play scenarios and presents a Vulkan clear frame.\n";
}

void ConfigureMoltenVkIcdFallback()
{
#if defined(__APPLE__)
	if (std::getenv("VK_ICD_FILENAMES") != nullptr)
		return;

	const std::filesystem::path candidates[] = {
		"/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json",
		"/usr/local/etc/vulkan/icd.d/MoltenVK_icd.json",
		"/opt/homebrew/Cellar/molten-vk/1.4.1/etc/vulkan/icd.d/MoltenVK_icd.json",
	};
	for (const auto &candidate : candidates) {
		if (std::filesystem::exists(candidate)) {
			setenv("VK_ICD_FILENAMES", candidate.string().c_str(), 0);
			return;
		}
	}
#endif
}

ProductLoadState LoadProductScenario(const std::filesystem::path &path)
{
	ProductLoadState state;
	state.load = iggy::runtime::RuntimeGameplayProductScenarioLoader {}.load(path);
	if (!state.load.ok()) {
		std::cerr << "Failed to load product scenario: " << path << "\n";
		for (const auto &issue : state.load.issues) {
			std::cerr << "  " << issue.path;
			if (!issue.key.empty())
				std::cerr << " [" << issue.key << "]";
			if (!issue.detail.empty())
				std::cerr << ": " << issue.detail;
			std::cerr << "\n";
		}
		throw std::runtime_error("product scenario load failed");
	}

	state.loop = iggy::runtime::RuntimeGameplayProductLoop {}.build(state.load);
	state.play = iggy::runtime::RuntimeGameplayProductPlayMode {}.build(state.loop);
	if (state.play.status !=
			iggy::runtime::RuntimeGameplayProductPlayModeBuildStatus::Ready)
		throw std::runtime_error("product play mode build failed");

	std::cout << "Loaded product scenario: " << state.load.sourcePath << std::endl;
	return state;
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

class NativeVulkanApp final {
public:
	explicit NativeVulkanApp(const LaunchOptions &options)
		: options_(options)
	{
		if (options_.hasPlayPath)
			product_ = LoadProductScenario(options_.playPath);
	}

	~NativeVulkanApp()
	{
		cleanup();
	}

	void run()
	{
		initWindow();
		initVulkan();
		std::cout << "Native Vulkan shell ready. Press Escape or close the window to quit." << std::endl;
		mainLoop();
	}

private:
	void initWindow()
	{
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
			throw std::runtime_error(SDL_GetError());

		const std::string title = options_.hasPlayPath
			? "Iggy Native Play - " + options_.playPath.filename().string()
			: "Iggy Native Play";
		window_ = SDL_CreateWindow(
			title.c_str(),
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			InitialWindowWidth,
			InitialWindowHeight,
			SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
		if (window_ == nullptr)
			throw std::runtime_error(SDL_GetError());
	}

	void initVulkan()
	{
		createInstance();
		if (!SDL_Vulkan_CreateSurface(window_, instance_, &surface_))
			throw std::runtime_error(SDL_GetError());
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapchain();
		createImageViews();
		createRenderPass();
		createFramebuffers();
		createCommandPool();
		createCommandBuffers();
		createSyncObjects();
	}

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

		VkAttachmentReference colorAttachmentRef {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency dependency {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		ThrowIfFailed(
			vkCreateRenderPass(device_, &renderPassInfo, nullptr, &renderPass_),
			"failed to create Vulkan render pass");
	}

	void createFramebuffers()
	{
		swapchainFramebuffers_.resize(swapchainImageViews_.size());
		for (std::size_t i = 0; i < swapchainImageViews_.size(); ++i) {
			const VkImageView attachments[] = { swapchainImageViews_[i] };
			VkFramebufferCreateInfo framebufferInfo {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass_;
			framebufferInfo.attachmentCount = 1;
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

	void recordCommandBuffer(VkCommandBuffer commandBuffer, std::uint32_t imageIndex)
	{
		VkCommandBufferBeginInfo beginInfo {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		ThrowIfFailed(
			vkBeginCommandBuffer(commandBuffer, &beginInfo),
			"failed to begin Vulkan command buffer");

		VkClearValue clearColor {};
		clearColor.color = { { 0.035F, 0.045F, 0.070F, 1.0F } };

		VkRenderPassBeginInfo renderPassInfo {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass_;
		renderPassInfo.framebuffer = swapchainFramebuffers_[imageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapchainExtent_;
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdEndRenderPass(commandBuffer);

		ThrowIfFailed(
			vkEndCommandBuffer(commandBuffer),
			"failed to record Vulkan command buffer");
	}

	void drawFrame()
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
		recordCommandBuffer(commandBuffers_[currentFrame_], imageIndex);

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
		createFramebuffers();
	}

	void mainLoop()
	{
		bool running = true;
		while (running) {
			SDL_Event event;
			while (SDL_PollEvent(&event) != 0) {
				if (event.type == SDL_QUIT)
					running = false;
				if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
					running = false;
				if (event.type == SDL_WINDOWEVENT &&
						event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
					framebufferResized_ = true;
			}
			drawFrame();
		}

		if (device_ != VK_NULL_HANDLE)
			ThrowIfFailed(vkDeviceWaitIdle(device_), "failed to idle Vulkan device");
	}

	void cleanupSwapchain()
	{
		for (VkFramebuffer framebuffer : swapchainFramebuffers_)
			vkDestroyFramebuffer(device_, framebuffer, nullptr);
		swapchainFramebuffers_.clear();

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

	void cleanup()
	{
		if (device_ != VK_NULL_HANDLE)
			vkDeviceWaitIdle(device_);

		cleanupSwapchain();

		for (std::size_t i = 0; i < imageAvailableSemaphores_.size(); ++i) {
			if (imageAvailableSemaphores_[i] != VK_NULL_HANDLE)
				vkDestroySemaphore(device_, imageAvailableSemaphores_[i], nullptr);
			if (renderFinishedSemaphores_[i] != VK_NULL_HANDLE)
				vkDestroySemaphore(device_, renderFinishedSemaphores_[i], nullptr);
			if (inFlightFences_[i] != VK_NULL_HANDLE)
				vkDestroyFence(device_, inFlightFences_[i], nullptr);
		}

		if (commandPool_ != VK_NULL_HANDLE)
			vkDestroyCommandPool(device_, commandPool_, nullptr);
		if (device_ != VK_NULL_HANDLE)
			vkDestroyDevice(device_, nullptr);
		if (surface_ != VK_NULL_HANDLE)
			vkDestroySurfaceKHR(instance_, surface_, nullptr);
		if (instance_ != VK_NULL_HANDLE)
			vkDestroyInstance(instance_, nullptr);
		if (window_ != nullptr)
			SDL_DestroyWindow(window_);
		SDL_Quit();
	}

	LaunchOptions options_;
	std::optional<ProductLoadState> product_;
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
	std::vector<VkFramebuffer> swapchainFramebuffers_;
	VkCommandPool commandPool_ = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> commandBuffers_;
	std::vector<VkSemaphore> imageAvailableSemaphores_;
	std::vector<VkSemaphore> renderFinishedSemaphores_;
	std::vector<VkFence> inFlightFences_;
	std::size_t currentFrame_ = 0;
	bool framebufferResized_ = false;
};

} // namespace

int main(int argc, char **argv)
{
	try {
		ConfigureMoltenVkIcdFallback();
		LaunchOptions options = ParseArgs(argc, argv);
		if (options.showHelp) {
			PrintUsage();
			return 0;
		}

		NativeVulkanApp app(options);
		app.run();
		return 0;
	} catch (const std::exception &error) {
		std::cerr << "iggy_native_play: " << error.what() << "\n";
		return 1;
	}
}
