#pragma once

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
	bool converted = false;

	[[nodiscard]] bool ok() const;
};

class RuntimeGameplayScenarioAuthoringAdapter {
public:
	[[nodiscard]] RuntimeGameplayScenarioAuthoringAdapterResult convert(
		const RuntimeGameplayScenarioAuthoringPacket &packet) const;
};

} // namespace iggy::runtime
