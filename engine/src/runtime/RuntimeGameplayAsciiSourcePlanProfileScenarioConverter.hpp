#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeGameplayAsciiSourcePlanRegionAiMapPromoter.hpp"
#include "runtime/RuntimeGameplayAsciiSourcePlanValidator.hpp"
#include "runtime/RuntimeGameplayProfileScenarioValidator.hpp"
#include "scene/ai/NpcAiProfileTraitCatalog.hpp"
#include "scene/interaction/InteractionEffectCatalog2D.hpp"
#include "scene/interaction/InteractionTarget2D.hpp"
#include "scene/inventory/LevelItemDrop2D.hpp"
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
	AuthoredControlInvalid,
	AuthoredPlayerCommandInvalid,
	PlayerStartInvalid,
	ProfileTraitCatalogInvalid,
	InteractionTargetRegistryInvalid,
	InteractionEffectCatalogInvalid,
	ItemDropRegistryInvalid,
	AiMapPromotionInvalid,
	ProfileScenarioInvalid,
};

enum class RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode {
	SourcePlanInvalid,
	UnsupportedTerrainPromotion,
	MissingFrameDefaults,
	ActorRegistryInvalid,
	ControlRegistryInvalid,
	UnknownAuthoredControlActor,
	DuplicateAuthoredControlActor,
	AmbiguousAuthoredPlayerCommandFrame,
	DuplicatePlayerStart,
	ProfileTraitCatalogInvalid,
	InteractionTargetRegistryInvalid,
	InteractionEffectCatalogInvalid,
	ItemDropRegistryInvalid,
	AiMapPromotionInvalid,
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
	bool promoteRegionAiMap = false;
	RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionConfig regionAiMap;
};

struct RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue {
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode code =
		RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::SourcePlanInvalid;
	std::size_t row = 0;
	std::size_t column = 0;
	char glyph = '\0';
	RuntimeGameplayAsciiSourcePlanIssue sourceIssue;
	RuntimeGameplayAsciiSourcePlanAuthoredControl authoredControl;
	RuntimeGameplayAsciiSourcePlanAuthoredPlayerCommand authoredPlayerCommand;
	NpcActorState2DIssue actorIssue;
	NpcActorControlState2DIssue controlIssue;
	NpcAiProfileTraitCatalogIssue profileTraitIssue;
	InteractionTarget2DRegistryIssue interactionTargetIssue;
	InteractionEffectCatalog2DIssue interactionEffectIssue;
	LevelItemDrop2DIssue itemDropIssue;
	RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionIssue aiMapPromotionIssue;
	RuntimeGameplayProfileScenarioIssue profileIssue;
};

struct RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult {
	RuntimeGameplayAsciiSourcePlan sourcePlan;
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config;
	RuntimeGameplayAsciiSourcePlanValidationResult sourceValidation;
	LevelTileMap promotedMap;
	NpcActorState2DRegistryBuildResult actorRegistry;
	NpcActorControlState2DRegistryBuildResult controlRegistry;
	NpcAiProfileTraitCatalogBuildResult profileTraitCatalog;
	InteractionTarget2DRegistryBuildResult interactionTargetRegistry;
	InteractionEffectCatalog2DBuildResult interactionEffectCatalog;
	LevelItemDrop2DRegistryBuildResult itemDropRegistry;
	RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionResult regionAiMapPromotion;
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
	std::size_t authoredControlIssueCount = 0;
	std::size_t authoredPlayerCommandIssueCount = 0;
	std::size_t playerStartIssueCount = 0;
	std::size_t profileTraitCatalogIssueCount = 0;
	std::size_t interactionTargetRegistryIssueCount = 0;
	std::size_t interactionEffectCatalogIssueCount = 0;
	std::size_t itemDropRegistryIssueCount = 0;
	std::size_t aiMapPromotionIssueCount = 0;
	std::size_t profileScenarioIssueCount = 0;
	std::size_t promotedActorCount = 0;
	std::size_t promotedPlayerCount = 0;
	std::size_t defaultControlCount = 0;
	std::size_t authoredControlCount = 0;
	std::size_t authoredPlayerCommandCount = 0;
	std::size_t promotedInteractionTargetCount = 0;
	std::size_t promotedInteractionEffectEntryCount = 0;
	std::size_t promotedItemDropCount = 0;
	std::size_t promotedAiMapRegionCount = 0;
	std::size_t unmappedAiMapRegionCount = 0;

	[[nodiscard]] bool ok() const;
};

class RuntimeGameplayAsciiSourcePlanProfileScenarioConverter {
public:
	[[nodiscard]] RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult convert(
		const RuntimeGameplayAsciiSourcePlan &sourcePlan,
		const RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig &config = {}) const;
};

} // namespace iggy::runtime
