#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeGameplayProfileScenarioDefinition.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayScenarioAuthoringSource {
	Unsupported,
	ProfileScenarioDefinition,
};

enum class RuntimeGameplayScenarioAuthoringAdapterStatus {
	UnsupportedSource,
	InvalidPacket,
	Converted,
};

enum class RuntimeGameplayScenarioAuthoringAdapterIssueCode {
	UnsupportedSource,
	MissingProfileScenarioPayload,
};

struct RuntimeGameplayScenarioAuthoringAdapterIssue {
	RuntimeGameplayScenarioAuthoringAdapterIssueCode code =
		RuntimeGameplayScenarioAuthoringAdapterIssueCode::UnsupportedSource;
	RuntimeGameplayScenarioAuthoringSource source =
		RuntimeGameplayScenarioAuthoringSource::Unsupported;
	bool hasProfileScenario = false;
};

struct RuntimeGameplayScenarioAuthoringPacket {
	RuntimeGameplayScenarioAuthoringSource source =
		RuntimeGameplayScenarioAuthoringSource::Unsupported;
	bool hasProfileScenario = false;
	RuntimeGameplayProfileScenarioDefinition profileScenario;
};

struct RuntimeGameplayScenarioAuthoringAdapterResult {
	RuntimeGameplayScenarioAuthoringPacket packet;
	RuntimeGameplayProfileScenarioDefinition profileScenario;
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
};

} // namespace iggy::runtime
