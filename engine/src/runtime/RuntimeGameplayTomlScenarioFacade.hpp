#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include "runtime/RuntimeGameplayAsciiSourcePlanTomlFileReader.hpp"
#include "runtime/RuntimeGameplayProfileScenarioRunner.hpp"
#include "runtime/RuntimeGameplayProfileScenarioValidator.hpp"
#include "runtime/RuntimeGameplayScenarioAuthoringAdapter.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayTomlScenarioFacadeStatus {
	ReadFailed,
	ConversionFailed,
	LintFailed,
	LintOk,
	RunFailed,
	Ran,
};

struct RuntimeGameplayTomlScenarioTraceFrame {
	std::size_t index = 0;
	std::string frameId;
	std::size_t acceptedCommandCount = 0;
	std::size_t pickedUpCount = 0;
	bool interactionChanged = false;
	std::size_t npcMovedCount = 0;
	std::vector<std::string> rows;
};

struct RuntimeGameplayTomlScenarioExpectationComparison {
	bool present = false;
	bool matched = true;
	bool checkedFinalRows = false;
	bool finalRowsMatched = true;
	bool checkedFrameCount = false;
	bool frameCountMatched = true;
	bool checkedAcceptedCommandCount = false;
	bool acceptedCommandCountMatched = true;
	bool checkedPickedUpCount = false;
	bool pickedUpCountMatched = true;
	bool checkedInteractionChanged = false;
	bool interactionChangedMatched = true;
	bool checkedNpcMovedCount = false;
	bool npcMovedCountMatched = true;
};

struct RuntimeGameplayTomlScenarioFacadeConfig {
	bool lintOnly = false;
	bool captureTraceFrames = false;
};

struct RuntimeGameplayTomlScenarioFacadeResult {
	std::filesystem::path path;
	RuntimeGameplayTomlScenarioFacadeConfig config;
	RuntimeGameplayAsciiSourcePlanTomlFileReadResult read;
	RuntimeGameplayScenarioAuthoringAdapterResult adapter;
	RuntimeGameplayProfileScenarioValidationResult validation;
	RuntimeGameplayProfileScenarioRunResult run;
	std::vector<std::string> finalRows;
	std::vector<RuntimeGameplayTomlScenarioTraceFrame> traceFrames;
	RuntimeGameplayTomlScenarioExpectationComparison expectationComparison;
	RuntimeGameplayTomlScenarioFacadeStatus status =
		RuntimeGameplayTomlScenarioFacadeStatus::ReadFailed;

	[[nodiscard]] bool ok() const;
	[[nodiscard]] bool ran() const;
	[[nodiscard]] bool linted() const;
};

class RuntimeGameplayTomlScenarioFacade {
public:
	[[nodiscard]] RuntimeGameplayTomlScenarioFacadeResult execute(
		const std::filesystem::path &path,
		const RuntimeGameplayTomlScenarioFacadeConfig &config = {}) const;
};

} // namespace iggy::runtime
