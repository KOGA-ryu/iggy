#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeGameplayAsciiSourcePlanProfileScenarioConverter.hpp"
#include "runtime/RuntimeGameplayProfileScenarioDefinition.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayScenarioAuthoringSource {
	Unsupported,
	ProfileScenarioDefinition,
	AsciiSourcePlan,
};

enum class RuntimeGameplayScenarioAuthoringAdapterStatus {
	UnsupportedSource,
	InvalidPacket,
	Converted,
	ConversionFailed,
};

enum class RuntimeGameplayScenarioAuthoringAdapterIssueCode {
	UnsupportedSource,
	MissingProfileScenarioPayload,
	MissingAsciiSourcePlanPayload,
	MissingAsciiSourcePlanConversionConfig,
	AsciiSourcePlanConversionFailed,
};

struct RuntimeGameplayScenarioAuthoringAdapterIssue {
	RuntimeGameplayScenarioAuthoringAdapterIssueCode code =
		RuntimeGameplayScenarioAuthoringAdapterIssueCode::UnsupportedSource;
	RuntimeGameplayScenarioAuthoringSource source =
		RuntimeGameplayScenarioAuthoringSource::Unsupported;
	bool hasProfileScenario = false;
	bool hasAsciiSourcePlan = false;
	bool hasAsciiSourcePlanConversionConfig = false;
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus asciiSourcePlanConversionStatus =
		RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::SourcePlanInvalid;
};

struct RuntimeGameplayScenarioAuthoringPacket {
	RuntimeGameplayScenarioAuthoringSource source =
		RuntimeGameplayScenarioAuthoringSource::Unsupported;
	bool hasProfileScenario = false;
	RuntimeGameplayProfileScenarioDefinition profileScenario;
	bool hasAsciiSourcePlan = false;
	RuntimeGameplayAsciiSourcePlan asciiSourcePlan;
};

struct RuntimeGameplayScenarioAuthoringAdapterConfig {
	bool hasAsciiSourcePlanProfileScenarioConfig = false;
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig asciiSourcePlanProfileScenario;
};

struct RuntimeGameplayScenarioAuthoringAdapterResult {
	RuntimeGameplayScenarioAuthoringPacket packet;
	RuntimeGameplayScenarioAuthoringAdapterConfig config;
	RuntimeGameplayProfileScenarioDefinition profileScenario;
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult asciiSourcePlanConversion;
	RuntimeGameplayScenarioAuthoringAdapterStatus status =
		RuntimeGameplayScenarioAuthoringAdapterStatus::UnsupportedSource;
	std::vector<RuntimeGameplayScenarioAuthoringAdapterIssue> issues;
	bool converted = false;
	std::size_t issueCount = 0;

	[[nodiscard]] bool ok() const;
	[[nodiscard]] bool hasIssues() const;
};

class RuntimeGameplayScenarioAuthoringAdapter {
public:
	[[nodiscard]] RuntimeGameplayScenarioAuthoringAdapterResult convert(
		const RuntimeGameplayScenarioAuthoringPacket &packet) const;
	[[nodiscard]] RuntimeGameplayScenarioAuthoringAdapterResult convert(
		const RuntimeGameplayScenarioAuthoringPacket &packet,
		const RuntimeGameplayScenarioAuthoringAdapterConfig &config) const;
};

} // namespace iggy::runtime
