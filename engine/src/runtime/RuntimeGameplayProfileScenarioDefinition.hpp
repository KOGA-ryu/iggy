#pragma once

#include <cstddef>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "runtime/RuntimeGameplayScenarioDefinition.hpp"
#include "scene/ai/NpcAiProfileTraitResolver.hpp"

namespace iggy::runtime {

struct RuntimeGameplayProfileScenarioFrameDefinition {
	bool hasFrameId = false;
	ResourceId frameId;
	RuntimeGameplayFrameInput playerFrame;
	NpcMapPlayControlFramePlanPools pools;
	AiMap2D aiMap;
	NpcAiProfileTraitResolverConfig profileConfig;
	NpcMapPlayControlFramePlanConfig controlConfig;
	LevelTileMap movementMap;
	NpcActorMovementFramePlan2DConfig movementConfig;
	NpcActorOccupancy2D previousOccupancy;
	InteractionTarget2DRegistry interactionTargets;
	AiMap2D refreshAiMap;
	NpcActorMovementRefreshFrame2DConfig refreshConfig;
};

struct RuntimeGameplayProfileScenarioDefinition {
	bool hasScenarioId = false;
	ResourceId scenarioId;
	RuntimeGameplayState initialState;
	NpcAiProfileTraitCatalog profileTraits;
	std::vector<RuntimeGameplayProfileScenarioFrameDefinition> frames;
};

struct RuntimeGameplayProfileScenarioDefinitionBuildResult {
	RuntimeGameplayProfileScenarioDefinition definition;
	RuntimeGameplayScenarioDefinition scenarioDefinition;
	RuntimeGameplayScenario scenario;
	std::vector<NpcAiProfileTraitResolveResult> profileFrames;
	bool built = false;
	std::size_t frameCount = 0;
	std::size_t resolvedSubjectCount = 0;
	std::size_t missingProfileCount = 0;
	std::size_t emptyActorProfileIdCount = 0;
	std::size_t absentSkippedCount = 0;
};

class RuntimeGameplayProfileScenarioDefinitionBuilder {
public:
	[[nodiscard]] RuntimeGameplayProfileScenarioDefinitionBuildResult build(
		const RuntimeGameplayProfileScenarioDefinition &definition) const;
};

} // namespace iggy::runtime
