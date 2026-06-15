#include "scene/ai/NpcAiProfileTraitResolver.hpp"

namespace {

const iggy::NpcAiProfileTraitEntry *FindProfileEntry(
	const iggy::NpcAiProfileTraitCatalog &catalog,
	const iggy::ResourceId &profileId,
	std::size_t &entryIndex)
{
	for (std::size_t index = 0; index < catalog.entries.size(); ++index) {
		if (catalog.entries[index].profileId == profileId) {
			entryIndex = index;
			return &catalog.entries[index];
		}
	}
	return nullptr;
}

iggy::NpcAiProfileTraitResolveIssue Issue(
	iggy::NpcAiProfileTraitResolveIssueCode code,
	std::size_t actorIndex,
	const iggy::NpcActorState2D &actor)
{
	return {
		code,
		actorIndex,
		actor,
		actor.aiProfileId,
	};
}

} // namespace

namespace iggy {

bool NpcAiProfileTraitResolveResult::hasSubjects() const
{
	return !subjects.empty();
}

bool NpcAiProfileTraitResolveResult::hasIssues() const
{
	return !issues.empty();
}

NpcAiProfileTraitResolveResult NpcAiProfileTraitResolver::resolve(
	const NpcActorState2DRegistry &actors,
	const NpcAiProfileTraitCatalog &catalog,
	const NpcAiProfileTraitResolverConfig &config) const
{
	NpcAiProfileTraitResolveResult result;
	result.actors = actors;
	result.catalog = catalog;
	result.config = config;
	result.actorCount = actors.actors.size();

	for (std::size_t actorIndex = 0; actorIndex < actors.actors.size(); ++actorIndex) {
		const NpcActorState2D &actor = actors.actors[actorIndex];
		NpcAiProfileTraitResolveEntry entry;
		entry.actorIndex = actorIndex;
		entry.actor = actor;

		if (!actor.present && !config.includeAbsentActors) {
			entry.status = NpcAiProfileTraitResolveStatus::ActorNotPresent;
			++result.absentSkippedCount;
			result.entries.push_back(entry);
			continue;
		}

		if (actor.aiProfileId.empty()) {
			entry.status = NpcAiProfileTraitResolveStatus::EmptyActorProfileId;
			result.issues.push_back(Issue(
				NpcAiProfileTraitResolveIssueCode::EmptyActorProfileId,
				actorIndex,
				actor));
			++result.emptyActorProfileIdCount;
			result.entries.push_back(entry);
			continue;
		}

		std::size_t profileEntryIndex = 0;
		const NpcAiProfileTraitEntry *profile =
			FindProfileEntry(catalog, actor.aiProfileId, profileEntryIndex);
		if (profile == nullptr) {
			entry.status = NpcAiProfileTraitResolveStatus::MissingProfile;
			result.issues.push_back(Issue(
				NpcAiProfileTraitResolveIssueCode::MissingProfile,
				actorIndex,
				actor));
			++result.missingProfileCount;
			result.entries.push_back(entry);
			continue;
		}

		entry.profileEntryIndex = profileEntryIndex;
		entry.profile = *profile;
		entry.subjectIndex = result.subjects.size();
		entry.subject.npcId = actor.npcId;
		entry.subject.traits = profile->traits;
		entry.status = NpcAiProfileTraitResolveStatus::Resolved;
		entry.resolved = true;
		result.subjects.push_back(entry.subject);
		++result.resolvedCount;
		result.entries.push_back(entry);
	}

	return result;
}

} // namespace iggy
