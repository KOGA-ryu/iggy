#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeGameplayScenarioDefinition.hpp"
#include "scene/npc/NpcActorFrameState2D.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayScenarioValidationStatus {
	Valid,
	Invalid,
};

enum class RuntimeGameplayScenarioIssueCode {
	EmptyScenarioId,
	EmptyFrameId,
	MissingControl,
	OrphanControl,
	InvalidMovementMap,
	DuplicateTraitSubject,
	OrphanTraitSubject,
	MissingTraitSubject,
};

struct RuntimeGameplayScenarioIssue {
	RuntimeGameplayScenarioIssueCode code = RuntimeGameplayScenarioIssueCode::MissingTraitSubject;
	std::size_t frameIndex = 0;
	std::size_t actorIndex = 0;
	std::size_t controlIndex = 0;
	std::size_t subjectIndex = 0;
	std::size_t firstSubjectIndex = 0;
	ResourceId npcId;
	NpcActorFrameState2DIssue frameIssue;
	NpcMapPlayControlFramePlanSubject subject;
	LevelTileMap movementMap;
};

struct RuntimeGameplayScenarioValidationResult {
	RuntimeGameplayScenarioDefinition definition;
	RuntimeGameplayScenario scenario;
	NpcActorFrameState2DProjectionResult frameState;
	std::vector<RuntimeGameplayScenarioIssue> issues;
	RuntimeGameplayScenarioValidationStatus status = RuntimeGameplayScenarioValidationStatus::Valid;
	std::size_t frameCount = 0;
	std::size_t issueCount = 0;
	std::size_t missingControlCount = 0;
	std::size_t orphanControlCount = 0;
	std::size_t invalidMovementMapCount = 0;
	std::size_t duplicateTraitSubjectCount = 0;
	std::size_t orphanTraitSubjectCount = 0;
	std::size_t missingTraitSubjectCount = 0;

	[[nodiscard]] bool ok() const;
};

class RuntimeGameplayScenarioValidator {
public:
	[[nodiscard]] RuntimeGameplayScenarioValidationResult validate(
		const RuntimeGameplayScenarioDefinition &definition) const;
};

} // namespace iggy::runtime
