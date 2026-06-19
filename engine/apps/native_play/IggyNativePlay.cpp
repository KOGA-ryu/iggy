#include <SDL.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "scene/level/TileCoord.hpp"

#include "NativePlayMath.hpp"
#include "NativeProductSession.hpp"
#include "NativeSceneDrawList.hpp"
#include "NativeStaticModelLoadReport.hpp"
#include "NativeStaticModelPolicy.hpp"
#include "NativeVulkanRenderer.hpp"

namespace {

namespace runtime = iggy::runtime;
using iggy::native_play::BuildNativeSceneDrawItems;
using iggy::native_play::BuildNativeStaticModelLoadReport;
using iggy::native_play::DefaultNativeStaticModelPolicy;
using iggy::native_play::LookAt;
using iggy::native_play::Mat4;
using iggy::native_play::Multiply;
using iggy::native_play::NativeStaticModelFallbackKind;
using iggy::native_play::NativeStaticModelLoadEntry;
using iggy::native_play::NativeStaticModelLoadReport;
using iggy::native_play::NativeStaticModelLoadStatus;
using iggy::native_play::NativeStaticModelSlot;
using iggy::native_play::NativeProductSession;
using iggy::native_play::NativeProductSessionConfig;
using iggy::native_play::NativeSceneDrawItem;
using iggy::native_play::NativeVulkanFrameInput;
using iggy::native_play::NativeVulkanRenderer;
using iggy::native_play::ParseNativeScriptedProductControls;
using iggy::native_play::Perspective;

constexpr int InitialWindowWidth = 1280;
constexpr int InitialWindowHeight = 720;
constexpr float Pi = 3.14159265358979323846F;
constexpr auto ProductTickInterval = std::chrono::milliseconds(250);

#ifndef IGGY_NATIVE_PLAY_ASSET_DIR
#define IGGY_NATIVE_PLAY_ASSET_DIR "."
#endif

struct LaunchOptions {
	bool showHelp = false;
	bool dumpStaticModelLoadReport = false;
	bool hasPlayPath = false;
	std::filesystem::path playPath;
	std::vector<std::string> scriptedControls;
	std::chrono::milliseconds scriptedControlInterval = ProductTickInterval;
	bool quitAfterScriptedControls = false;
	bool debugScriptedControls = false;
	bool dumpFinalState = false;
	std::vector<iggy::TileCoord> expectedPlayerTiles;
};

std::optional<iggy::runtime::RuntimeGameplayProductInputControl2D>
MapSdlKeyToProductControl(SDL_Keycode key)
{
	using iggy::runtime::RuntimeGameplayProductInputControl2D;
	switch (key) {
	case SDLK_UP:
	case SDLK_w:
		return RuntimeGameplayProductInputControl2D::MoveNorth;
	case SDLK_DOWN:
	case SDLK_s:
		return RuntimeGameplayProductInputControl2D::MoveSouth;
	case SDLK_LEFT:
	case SDLK_a:
		return RuntimeGameplayProductInputControl2D::MoveWest;
	case SDLK_RIGHT:
	case SDLK_d:
		return RuntimeGameplayProductInputControl2D::MoveEast;
	case SDLK_e:
	case SDLK_RETURN:
	case SDLK_KP_ENTER:
		return RuntimeGameplayProductInputControl2D::Interact;
	case SDLK_i:
		return RuntimeGameplayProductInputControl2D::Inspect;
	case SDLK_SPACE:
		return RuntimeGameplayProductInputControl2D::Wait;
	case SDLK_ESCAPE:
		return RuntimeGameplayProductInputControl2D::Cancel;
	default:
		break;
	}
	return std::nullopt;
}

std::string Trim(std::string value)
{
	const auto first = std::find_if_not(
		value.begin(),
		value.end(),
		[](unsigned char character) { return std::isspace(character) != 0; });
	const auto last = std::find_if_not(
		value.rbegin(),
		value.rend(),
		[](unsigned char character) { return std::isspace(character) != 0; })
		.base();
	if (first >= last)
		return {};
	return std::string(first, last);
}

std::size_t ParsePositiveCount(const std::string &value, const char *name)
{
	if (value.empty())
		throw std::runtime_error(std::string(name) + " requires a positive integer");
	std::size_t consumed = 0;
	const unsigned long parsed = std::stoul(value, &consumed);
	if (consumed != value.size() || parsed == 0)
		throw std::runtime_error(std::string(name) + " requires a positive integer");
	return static_cast<std::size_t>(parsed);
}

int ParseIntStrict(const std::string &value, const char *name)
{
	if (value.empty())
		throw std::runtime_error(std::string(name) + " requires an integer");
	std::size_t consumed = 0;
	const int parsed = std::stoi(value, &consumed);
	if (consumed != value.size())
		throw std::runtime_error(std::string(name) + " requires an integer");
	return parsed;
}

iggy::TileCoord ParseExpectedTile(const std::string &value)
{
	const std::size_t comma = value.find(',');
	if (comma == std::string::npos)
		throw std::runtime_error("expected player tile must use x,y format");
	return {
		ParseIntStrict(Trim(value.substr(0, comma)), "expected player tile x"),
		ParseIntStrict(Trim(value.substr(comma + 1)), "expected player tile y"),
	};
}

std::vector<iggy::TileCoord> ParseExpectedTiles(const std::string &spec)
{
	std::vector<iggy::TileCoord> tiles;
	std::size_t start = 0;
	while (start <= spec.size()) {
		const std::size_t separator = spec.find(';', start);
		const std::string token = Trim(spec.substr(
			start,
			separator == std::string::npos ? std::string::npos : separator - start));
		if (!token.empty())
			tiles.push_back(ParseExpectedTile(token));
		if (separator == std::string::npos)
			break;
		start = separator + 1;
	}
	return tiles;
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
		if (arg == "--dump-static-model-load-report") {
			options.dumpStaticModelLoadReport = true;
			continue;
		}
		if (arg == "--play") {
			if (i + 1 >= argc)
				throw std::runtime_error("--play requires a scenario path");
			options.hasPlayPath = true;
			options.playPath = argv[++i];
			continue;
		}
		if (arg == "--scripted-controls") {
			if (i + 1 >= argc)
				throw std::runtime_error("--scripted-controls requires a comma-separated list");
			options.scriptedControls.push_back(argv[++i]);
			continue;
		}
		if (arg == "--scripted-control-interval-ms") {
			if (i + 1 >= argc)
				throw std::runtime_error("--scripted-control-interval-ms requires a value");
			options.scriptedControlInterval = std::chrono::milliseconds(
				static_cast<int>(
					ParsePositiveCount(argv[++i], "--scripted-control-interval-ms")));
			continue;
		}
		if (arg == "--quit-after-script") {
			options.quitAfterScriptedControls = true;
			continue;
		}
		if (arg == "--debug-scripted-controls") {
			options.debugScriptedControls = true;
			continue;
		}
		if (arg == "--dump-final-state") {
			options.dumpFinalState = true;
			continue;
		}
		if (arg == "--expect-player-tiles") {
			if (i + 1 >= argc)
				throw std::runtime_error("--expect-player-tiles requires a semicolon-separated list");
			std::vector<iggy::TileCoord> parsed = ParseExpectedTiles(argv[++i]);
			options.expectedPlayerTiles.insert(
				options.expectedPlayerTiles.end(),
				parsed.begin(),
				parsed.end());
			continue;
		}
		throw std::runtime_error("unknown argument: " + arg);
	}
	if (!options.scriptedControls.empty() && !options.hasPlayPath)
		throw std::runtime_error("--scripted-controls requires --play");
	if (options.dumpFinalState && options.scriptedControls.empty())
		throw std::runtime_error("--dump-final-state requires --scripted-controls");
	if (!options.expectedPlayerTiles.empty() && options.scriptedControls.empty())
		throw std::runtime_error("--expect-player-tiles requires --scripted-controls");
	if (!options.expectedPlayerTiles.empty()) {
		const auto controls =
			ParseNativeScriptedProductControls(options.scriptedControls);
		if (options.expectedPlayerTiles.size() != controls.size())
			throw std::runtime_error("--expect-player-tiles count must match expanded scripted controls");
	}
	return options;
}

void PrintUsage()
{
	std::cout
		<< "Usage: iggy_native_play [--play PATH]\n"
		<< "       iggy_native_play --play PATH --scripted-controls LIST [OPTIONS]\n"
		<< "\n"
		<< "Opens the native SDL/Vulkan play shell.\n"
		<< "\n"
		<< "Asset diagnostics options:\n"
		<< "  --dump-static-model-load-report      Print static model asset load status\n"
		<< "                                       without launching SDL/Vulkan.\n"
		<< "\n"
		<< "Scripted control options:\n"
		<< "  --scripted-controls LIST             Comma-separated controls such as\n"
		<< "                                       east,east,south or right*3,wait.\n"
		<< "  --scripted-control-interval-ms N     Delay between scripted controls.\n"
		<< "  --debug-scripted-controls            Print per-step frame diagnostics.\n"
		<< "  --dump-final-state                   Print final player tile, next frame\n"
		<< "                                       index, render command count, and\n"
		<< "                                       active/held input counts after the\n"
		<< "                                       scripted sequence completes.\n"
		<< "  --expect-player-tiles 'x,y;x,y'      Fail unless scripted steps land on\n"
		<< "                                       the expected player tiles.\n"
		<< "  --quit-after-script                  Exit after the scripted sequence.\n";
}

const char *NativeStaticModelSlotName(NativeStaticModelSlot slot)
{
	switch (slot) {
	case NativeStaticModelSlot::Floor:
		return "Floor";
	case NativeStaticModelSlot::Wall:
		return "Wall";
	case NativeStaticModelSlot::NpcActor:
		return "NpcActor";
	case NativeStaticModelSlot::Player:
		return "Player";
	}
	return "Unknown";
}

const char *NativeStaticModelLoadStatusName(NativeStaticModelLoadStatus status)
{
	switch (status) {
	case NativeStaticModelLoadStatus::MissingPolicyRef:
		return "MissingPolicyRef";
	case NativeStaticModelLoadStatus::Loaded:
		return "Loaded";
	case NativeStaticModelLoadStatus::LoadFailed:
		return "LoadFailed";
	}
	return "Unknown";
}

const char *NativeStaticModelFallbackKindName(NativeStaticModelFallbackKind fallback)
{
	switch (fallback) {
	case NativeStaticModelFallbackKind::Cube:
		return "Cube";
	case NativeStaticModelFallbackKind::ProceduralBean:
		return "ProceduralBean";
	case NativeStaticModelFallbackKind::ProceduralNpcMarker:
		return "ProceduralNpcMarker";
	}
	return "Unknown";
}

void PrintNativeStaticModelLoadReport(const NativeStaticModelLoadReport &report)
{
	std::cout
		<< "static-model-load-report"
		<< " loaded=" << report.loadedCount
		<< " failed=" << report.failedCount
		<< " missing=" << report.missingCount
		<< "\n";
	for (const NativeStaticModelLoadEntry &entry : report.entries) {
		std::cout
			<< "slot=" << NativeStaticModelSlotName(entry.slot)
			<< " filename=" << (entry.meshFilename.empty() ? "<missing>" : entry.meshFilename)
			<< " status=" << NativeStaticModelLoadStatusName(entry.status)
			<< " fallback=" << NativeStaticModelFallbackKindName(entry.fallback)
			<< " vertices=" << entry.vertexCount
			<< " indices=" << entry.indexCount
			<< " issues=" << entry.issueCount
			<< "\n";
	}
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

class NativeVulkanApp final {
public:
	explicit NativeVulkanApp(const LaunchOptions &options)
		: options_(options)
	{
		if (options_.hasPlayPath) {
			NativeProductSessionConfig config;
			config.playPath = options_.playPath;
			config.scriptedControlSpecs = options_.scriptedControls;
			config.productTickInterval = ProductTickInterval;
			config.scriptedControlInterval = options_.scriptedControlInterval;
			config.quitAfterScriptedControls = options_.quitAfterScriptedControls;
			config.debugScriptedControls = options_.debugScriptedControls;
			config.dumpFinalState = options_.dumpFinalState;
			config.expectedPlayerTiles = options_.expectedPlayerTiles;
			productSession_.emplace(std::move(config));
		}
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
		renderer_.initialize(window_);
	}

	Mat4 viewProjectionMatrix() const
	{
		const Mat4 view = cameraViewMatrix();
		const Mat4 projection = Perspective(
			55.0F * Pi / 180.0F,
			renderer_.aspectRatio(),
			0.1F,
			100.0F);
		return Multiply(projection, view);
	}

	Mat4 cameraViewMatrix() const
	{
		float extent = 6.0F;
		if (productSession_.has_value()) {
			const runtime::RuntimeGameplayState *state =
				productSession_->currentGameplayState();
			if (state != nullptr) {
				const auto &map = state->session.level.map;
				extent = std::max<float>(
					extent,
					static_cast<float>(std::max(map.width, map.height)));
			}
		}

		return LookAt(
			{ 0.0F, extent * 0.85F, extent * 1.15F },
			{ 0.0F, 0.0F, 0.0F },
			{ 0.0F, 1.0F, 0.0F });
	}

	const runtime::RuntimeGameplayState *nativeSceneDrawState() const
	{
		if (!productSession_.has_value())
			return nullptr;
		return productSession_->currentGameplayState();
	}

	void drawFrame()
	{
		using Clock = std::chrono::steady_clock;
		const float seconds =
			std::chrono::duration<float>(Clock::now() - startTime_).count();
		const Mat4 viewProjection = viewProjectionMatrix();
		const std::vector<NativeSceneDrawItem> drawItems =
			BuildNativeSceneDrawItems({ nativeSceneDrawState(), seconds });

		NativeVulkanFrameInput input;
		input.viewProjection = viewProjection;
		input.drawItems = &drawItems;
		renderer_.drawFrame(input);
	}

	void mainLoop()
	{
		bool running = true;
		while (running) {
			SDL_Event event;
			while (SDL_PollEvent(&event) != 0) {
				if (event.type == SDL_QUIT)
					running = false;
				if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
					if (event.type == SDL_KEYDOWN &&
							event.key.keysym.sym == SDLK_ESCAPE)
						running = false;
					if (event.key.repeat == 0) {
						const std::optional<runtime::RuntimeGameplayProductInputControl2D>
							control = MapSdlKeyToProductControl(event.key.keysym.sym);
						if (control.has_value() && productSession_.has_value()) {
							(void)productSession_->recordInput(
								*control,
								event.type == SDL_KEYDOWN
								? runtime::RuntimeGameplayProductInputEventKind::Pressed
								: runtime::RuntimeGameplayProductInputEventKind::Released);
						}
					}
				}
				if (event.type == SDL_WINDOWEVENT &&
						event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
					renderer_.markFramebufferResized();
			}
			const bool quitAfterScriptedStep =
				productSession_.has_value() &&
				productSession_->stepScriptedControlsIfDue();
			if (productSession_.has_value())
				productSession_->stepProductIfDue();
			drawFrame();
			if (quitAfterScriptedStep)
				running = false;
		}

		renderer_.waitIdle();
	}

	void cleanup()
	{
		renderer_.cleanup();
		if (window_ != nullptr) {
			SDL_DestroyWindow(window_);
			window_ = nullptr;
		}
		SDL_Quit();
	}

	LaunchOptions options_;
	std::optional<NativeProductSession> productSession_;
	NativeVulkanRenderer renderer_;
	SDL_Window *window_ = nullptr;
	std::chrono::steady_clock::time_point startTime_ =
		std::chrono::steady_clock::now();
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
		if (options.dumpStaticModelLoadReport) {
			const NativeStaticModelLoadReport report =
				BuildNativeStaticModelLoadReport(
					DefaultNativeStaticModelPolicy(),
					IGGY_NATIVE_PLAY_ASSET_DIR);
			PrintNativeStaticModelLoadReport(report);
			return report.failedCount == 0 && report.missingCount == 0 ? 0 : 1;
		}

		NativeVulkanApp app(options);
		app.run();
		return 0;
	} catch (const std::exception &error) {
		std::cerr << "iggy_native_play: " << error.what() << "\n";
		return 1;
	}
}
