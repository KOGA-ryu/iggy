#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "scene/ai/NpcAiProfileTraitCatalog.hpp"
#include "scene/ai/NpcMapPlayControlFramePlan.hpp"
#include "scene/npc/NpcActorState2D.hpp"

namespace iggy {

struct NpcAiProfileTraitResolverConfig {
	bool includeAbsentActors = false;
};

enum class NpcAiProfileTraitResolveStatus {
	Resolved,
	ActorNotPresent,
	EmptyActorProfileId,
	MissingProfile,
};

enum class NpcAiProfileTraitResolveIssueCode {
	EmptyActorProfileId,
	MissingProfile,
};

struct NpcAiProfileTraitResolveIssue {
	NpcAiProfileTraitResolveIssueCode code = NpcAiProfileTraitResolveIssueCode::MissingProfile;
	std::size_t actorIndex = 0;
	NpcActorState2D actor;
	ResourceId profileId;
};

struct NpcAiProfileTraitResolveEntry {
	std::size_t actorIndex = 0;
	NpcActorState2D actor;
	std::optional<std::size_t> profileEntryIndex;
	NpcAiProfileTraitEntry profile;
	std::optional<std::size_t> subjectIndex;
	NpcMapPlayControlFramePlanSubject subject;
	NpcAiProfileTraitResolveStatus status = NpcAiProfileTraitResolveStatus::MissingProfile;
	bool resolved = false;
};

struct NpcAiProfileTraitResolveResult {
	NpcActorState2DRegistry actors;
	NpcAiProfileTraitCatalog catalog;
	NpcAiProfileTraitResolverConfig config;
	std::vector<NpcAiProfileTraitResolveEntry> entries;
	std::vector<NpcAiProfileTraitResolveIssue> issues;
	std::vector<NpcMapPlayControlFramePlanSubject> subjects;
	std::size_t actorCount = 0;
	std::size_t resolvedCount = 0;
	std::size_t missingProfileCount = 0;
	std::size_t emptyActorProfileIdCount = 0;
	std::size_t absentSkippedCount = 0;

	[[nodiscard]] bool hasSubjects() const;
	[[nodiscard]] bool hasIssues() const;
};

class NpcAiProfileTraitResolver {
public:
	[[nodiscard]] NpcAiProfileTraitResolveResult resolve(
		const NpcActorState2DRegistry &actors,
		const NpcAiProfileTraitCatalog &catalog,
		const NpcAiProfileTraitResolverConfig &config = {}) const;
};

} // namespace iggy
