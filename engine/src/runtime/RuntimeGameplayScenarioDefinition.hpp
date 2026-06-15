#pragma once

#include <cstddef>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "runtime/RuntimeGameplayScenarioRunner.hpp"

namespace iggy::runtime {

struct RuntimeGameplayScenarioFrameDefinition {
	bool hasFrameId = false;
	ResourceId frameId;
	RuntimeGameplayOrchestratedFrameRunnerFrame frame;
};

struct RuntimeGameplayScenarioDefinition {
	bool hasScenarioId = false;
	ResourceId scenarioId;
	RuntimeGameplayState initialState;
	std::vector<RuntimeGameplayScenarioFrameDefinition> frames;
};

struct RuntimeGameplayScenarioDefinitionBuildResult {
	RuntimeGameplayScenarioDefinition definition;
	RuntimeGameplayScenario scenario;
	bool built = false;
	std::size_t frameCount = 0;
};

class RuntimeGameplayScenarioDefinitionBuilder {
public:
	[[nodiscard]] RuntimeGameplayScenarioDefinitionBuildResult build(
		const RuntimeGameplayScenarioDefinition &definition) const;
};

} // namespace iggy::runtime
