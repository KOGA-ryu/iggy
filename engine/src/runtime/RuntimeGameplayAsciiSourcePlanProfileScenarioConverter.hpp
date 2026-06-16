#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeGameplayAsciiSourcePlanValidator.hpp"
#include "runtime/RuntimeGameplayProfileScenarioValidator.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus {
	Converted,
	SourcePlanInvalid,
	UnsupportedTerrainPromotion,
	MissingFrameDefaults,
	ActorRegistryInvalid,
	ControlRegistryInvalid,
	ProfileScenarioInvalid,
};

enum class RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode {
	SourcePlanInvalid,
	UnsupportedTerrainPromotion,
	MissingFrameDefaults,
	ActorRegistryInvalid,
	ControlRegistryInvalid,
	ProfileScenarioInvalid,
};

struct RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig {
	NpcAiProfileTraitCatalog profileTraits;
	bool hasDefaultFrame = false;
	RuntimeGameplayProfileScenarioFrameDefinition defaultFrame;
};

struct RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue {
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode code =
		RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::SourcePlanInvalid;
	RuntimeGameplayAsciiSourcePlanIssue sourceIssue;
	RuntimeGameplayProfileScenarioIssue profileIssue;
};

struct RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult {
	RuntimeGameplayAsciiSourcePlan sourcePlan;
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config;
	RuntimeGameplayAsciiSourcePlanValidationResult sourceValidation;
	RuntimeGameplayProfileScenarioDefinition definition;
	RuntimeGameplayProfileScenarioValidationResult profileValidation;
	std::vector<RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue> issues;
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus status =
		RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::SourcePlanInvalid;
	bool converted = false;
	std::size_t issueCount = 0;
	std::size_t sourcePlanIssueCount = 0;
	std::size_t missingFrameDefaultsCount = 0;
	std::size_t profileScenarioIssueCount = 0;

	[[nodiscard]] bool ok() const;
};

class RuntimeGameplayAsciiSourcePlanProfileScenarioConverter {
public:
	[[nodiscard]] RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult convert(
		const RuntimeGameplayAsciiSourcePlan &sourcePlan,
		const RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig &config = {}) const;
};

} // namespace iggy::runtime
