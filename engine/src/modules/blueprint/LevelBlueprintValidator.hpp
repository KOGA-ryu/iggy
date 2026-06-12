#pragma once

#include <string>
#include <vector>

#include "scene/level/LevelBlueprint.hpp"

namespace iggy {

enum class LevelBlueprintIssueCode {
	InvalidBounds,
	MissingPlayerStart,
	DuplicatePlayerStart,
	PlayerStartOutOfBounds,
	EntitySpawnOutOfBounds,
	EmptyEntityType,
	DuplicateEntityId,
	InvalidTileCount,
};

struct LevelBlueprintIssue {
	LevelBlueprintIssueCode code;
	std::string message;
};

struct LevelBlueprintValidationReport {
	bool valid = true;
	std::vector<LevelBlueprintIssue> issues;
};

class LevelBlueprintValidator {
public:
	[[nodiscard]] LevelBlueprintValidationReport validate(const LevelBlueprint &blueprint) const;
};

} // namespace iggy
