#include "runtime/RuntimeGameplayScenarioDefinition.hpp"

namespace iggy::runtime {

RuntimeGameplayScenarioDefinitionBuildResult RuntimeGameplayScenarioDefinitionBuilder::build(
	const RuntimeGameplayScenarioDefinition &definition) const
{
	RuntimeGameplayScenarioDefinitionBuildResult result;
	result.definition = definition;
	result.scenario.initialState = definition.initialState;
	result.scenario.frames.reserve(definition.frames.size());
	for (const RuntimeGameplayScenarioFrameDefinition &frame : definition.frames) {
		result.scenario.frames.push_back({ frame.frame });
	}
	result.frameCount = result.scenario.frames.size();
	result.built = true;
	return result;
}

} // namespace iggy::runtime
