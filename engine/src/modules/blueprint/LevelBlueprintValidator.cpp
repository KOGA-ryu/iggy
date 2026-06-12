#include "modules/blueprint/LevelBlueprintValidator.hpp"

#include <cstddef>
#include <string_view>
#include <string>
#include <unordered_set>
#include <utility>

namespace iggy {

namespace {

void AddIssue(LevelBlueprintValidationReport &report, LevelBlueprintIssueCode code, std::string message)
{
	report.valid = false;
	report.issues.push_back({ code, std::move(message) });
}

} // namespace

LevelBlueprintValidationReport LevelBlueprintValidator::validate(const LevelBlueprint &blueprint) const
{
	LevelBlueprintValidationReport report;
	const bool boundsValid = blueprint.bounds.width > 0 && blueprint.bounds.height > 0;

	if (!boundsValid)
		AddIssue(report, LevelBlueprintIssueCode::InvalidBounds, "level bounds must have positive width and height");

	if (blueprint.playerStarts.empty()) {
		AddIssue(report, LevelBlueprintIssueCode::MissingPlayerStart, "level blueprint must have one player start");
	} else if (blueprint.playerStarts.size() > 1) {
		AddIssue(report, LevelBlueprintIssueCode::DuplicatePlayerStart, "level blueprint must not have more than one player start");
	}

	if (boundsValid) {
		for (const PlayerStart &start : blueprint.playerStarts) {
			if (!blueprint.bounds.contains(start.x, start.y))
				AddIssue(report, LevelBlueprintIssueCode::PlayerStartOutOfBounds, "player start must be inside level bounds");
		}

		for (const BlueprintEntitySpawn &spawn : blueprint.entitySpawns) {
			if (!blueprint.bounds.contains(spawn.x, spawn.y))
				AddIssue(report, LevelBlueprintIssueCode::EntitySpawnOutOfBounds, "entity spawn must be inside level bounds");
		}
	}

	for (const BlueprintEntitySpawn &spawn : blueprint.entitySpawns) {
		if (spawn.type.empty())
			AddIssue(report, LevelBlueprintIssueCode::EmptyEntityType, "entity spawn type ResourceId must not be empty");
	}

	std::unordered_set<std::string_view> entityIds;
	for (const BlueprintEntitySpawn &spawn : blueprint.entitySpawns) {
		if (spawn.id.empty())
			continue;
		if (!entityIds.insert(spawn.id.value()).second)
			AddIssue(report, LevelBlueprintIssueCode::DuplicateEntityId, "entity spawn ids must be unique when provided");
	}

	if (!blueprint.tiles.empty()) {
		const std::size_t expectedTileCount = static_cast<std::size_t>(blueprint.bounds.width) * static_cast<std::size_t>(blueprint.bounds.height);
		if (!boundsValid || blueprint.tiles.size() != expectedTileCount)
			AddIssue(report, LevelBlueprintIssueCode::InvalidTileCount, "explicit tile grid size must match level bounds");
	}

	return report;
}

} // namespace iggy
