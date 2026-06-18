#pragma once

#include <cstddef>
#include <filesystem>
#include <vector>

#include "runtime/RuntimeGameplayOrchestratedFrameStep.hpp"
#include "runtime/RuntimeGameplayProductScenarioLoader.hpp"
#include "runtime/RuntimeGameplayScenarioRunner.hpp"
#include "scene/player/PlayerInputIntentGate2D.hpp"
#include "servers/physics2d/CollisionWorld2D.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayProductLoopStatus {
	Ready,
	LoadFailed,
};

struct RuntimeGameplayProductLoopState {
	bool loaded = false;
	std::filesystem::path inputPath;
	std::filesystem::path sourcePath;
	bool hasPackage = false;
	std::filesystem::path packageRoot;
	std::filesystem::path manifestPath;
	std::filesystem::path mainScenarioPath;
	RuntimeGameplayTomlScenarioPackageManifest packageManifest;
	RuntimeGameplayProfileScenarioDefinition definition;
	RuntimeGameplayScenario scenario;
	RuntimeGameplayState initialState;
	RuntimeGameplayState currentState;
	std::size_t nextFrameIndex = 0;
};

struct RuntimeGameplayProductLoopBuildResult {
	RuntimeGameplayProductLoopStatus status =
		RuntimeGameplayProductLoopStatus::LoadFailed;
	RuntimeGameplayProductLoopState state;
	RuntimeGameplayProductScenarioLoadResult load;
};

enum class RuntimeGameplayProductLoopStepStatus {
	Stepped,
	NotLoaded,
	NoFrameAvailable,
};

struct RuntimeGameplayProductLoopStepInput {
	RuntimeGameplayProductLoopState state;
	std::vector<PlayerInputIntent2D> playerIntents;
	bool hasPlayerInputContextOverride = false;
	PlayerInputContext2D playerInputContextOverride;
};

struct RuntimeGameplayProductLoopStepResult {
	RuntimeGameplayProductLoopStepStatus status =
		RuntimeGameplayProductLoopStepStatus::NotLoaded;
	RuntimeGameplayProductLoopState state;
	RuntimeGameplayOrchestratedFrameResult frame;
	std::size_t frameIndex = 0;
};

class RuntimeGameplayProductLoop {
public:
	[[nodiscard]] RuntimeGameplayProductLoopBuildResult build(
		const RuntimeGameplayProductScenarioLoadResult &load) const;

	[[nodiscard]] RuntimeGameplayProductLoopStepResult step(
		const RuntimeGameplayProductLoopStepInput &input) const;

	[[nodiscard]] RuntimeGameplayProductLoopStepResult step(
		const RuntimeGameplayProductLoopStepInput &input,
		const physics2d::CollisionWorld2D &explicitWorld) const;
};

} // namespace iggy::runtime
