#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeGameplayProfileScenarioDefinition.hpp"
#include "runtime/RuntimeGameplayScenarioValidator.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayProfileScenarioValidationStatus {
	Valid,
	Invalid,
};

enum class RuntimeGameplayProfileScenarioIssueCode {
	EmptyScenarioId,
	EmptyFrameId,
	MissingProfileTrait,
	EmptyActorProfileId,
	NestedScenarioInvalid,
};

struct RuntimeGameplayProfileScenarioIssue {
	RuntimeGameplayProfileScenarioIssueCode code =
		RuntimeGameplayProfileScenarioIssueCode::NestedScenarioInvalid;
	std::size_t frameIndex = 0;
	std::size_t actorIndex = 0;
	ResourceId npcId;
	ResourceId profileId;
	NpcAiProfileTraitResolveIssue profileIssue;
	RuntimeGameplayScenarioIssue nestedIssue;
};

struct RuntimeGameplayProfileScenarioValidationResult {
	RuntimeGameplayProfileScenarioDefinition definition;
	RuntimeGameplayProfileScenarioDefinitionBuildResult build;
	RuntimeGameplayScenarioValidationResult scenario;
	std::vector<RuntimeGameplayProfileScenarioIssue> issues;
	RuntimeGameplayProfileScenarioValidationStatus status =
		RuntimeGameplayProfileScenarioValidationStatus::Valid;
	std::size_t frameCount = 0;
	std::size_t issueCount = 0;
	std::size_t emptyScenarioIdCount = 0;
	std::size_t emptyFrameIdCount = 0;
	std::size_t missingProfileTraitCount = 0;
	std::size_t emptyActorProfileIdCount = 0;
	std::size_t nestedScenarioIssueCount = 0;

	[[nodiscard]] bool ok() const;
};

class RuntimeGameplayProfileScenarioValidator {
public:
	[[nodiscard]] RuntimeGameplayProfileScenarioValidationResult validate(
		const RuntimeGameplayProfileScenarioDefinition &definition) const;
};

} // namespace iggy::runtime
