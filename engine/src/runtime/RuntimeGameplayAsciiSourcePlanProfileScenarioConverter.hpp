#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeGameplayAsciiSourcePlanValidator.hpp"
#include "runtime/RuntimeGameplayProfileScenarioValidator.hpp"
#include "scene/npc/NpcActorControlState2D.hpp"
#include "scene/npc/NpcActorState2D.hpp"

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

struct RuntimeGameplayAsciiSourcePlanTerrainPromotion {
	char glyph = '\0';
	bool walkable = true;
};

struct RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig {
	NpcAiProfileTraitCatalog profileTraits;
	bool hasDefaultFrame = false;
	RuntimeGameplayProfileScenarioFrameDefinition defaultFrame;
	std::vector<RuntimeGameplayAsciiSourcePlanTerrainPromotion> terrain;
	ResourceId defaultFactionId;
	ResourceId defaultGoalId;
	bool hasDefaultControl = false;
	NpcActorControlState2D defaultControl;
};

struct RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue {
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode code =
		RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::SourcePlanInvalid;
	std::size_t row = 0;
	std::size_t column = 0;
	char glyph = '\0';
	RuntimeGameplayAsciiSourcePlanIssue sourceIssue;
	NpcActorState2DIssue actorIssue;
	NpcActorControlState2DIssue controlIssue;
	RuntimeGameplayProfileScenarioIssue profileIssue;
};

struct RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult {
	RuntimeGameplayAsciiSourcePlan sourcePlan;
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config;
	RuntimeGameplayAsciiSourcePlanValidationResult sourceValidation;
	LevelTileMap promotedMap;
	NpcActorState2DRegistryBuildResult actorRegistry;
	NpcActorControlState2DRegistryBuildResult controlRegistry;
	RuntimeGameplayProfileScenarioDefinition definition;
	RuntimeGameplayProfileScenarioValidationResult profileValidation;
	std::vector<RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue> issues;
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus status =
		RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::SourcePlanInvalid;
	bool converted = false;
	std::size_t issueCount = 0;
	std::size_t sourcePlanIssueCount = 0;
	std::size_t unsupportedTerrainPromotionCount = 0;
	std::size_t missingFrameDefaultsCount = 0;
	std::size_t actorRegistryIssueCount = 0;
	std::size_t controlRegistryIssueCount = 0;
	std::size_t profileScenarioIssueCount = 0;
	std::size_t promotedActorCount = 0;
	std::size_t defaultControlCount = 0;

	[[nodiscard]] bool ok() const;
};

class RuntimeGameplayAsciiSourcePlanProfileScenarioConverter {
public:
	[[nodiscard]] RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult convert(
		const RuntimeGameplayAsciiSourcePlan &sourcePlan,
		const RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig &config = {}) const;
};

} // namespace iggy::runtime
