#include "scene/interaction/InteractionEffectCatalog2D.hpp"

#include <utility>

namespace iggy {

InteractionEffectCatalog2D::InteractionEffectCatalog2D(std::vector<InteractionEffectEntry2D> entries)
    : entries_(std::move(entries))
{
}

const std::vector<InteractionEffectEntry2D> &InteractionEffectCatalog2D::entries() const
{
	return entries_;
}

const std::vector<InteractionEffect2D> *InteractionEffectCatalog2D::find(const ResourceId &targetId) const
{
	for (const InteractionEffectEntry2D &entry : entries_) {
		if (entry.targetId == targetId)
			return &entry.effects;
	}
	return nullptr;
}

bool InteractionEffectCatalog2D::contains(const ResourceId &targetId) const
{
	return find(targetId) != nullptr;
}

InteractionEffectCatalog2DBuildResult InteractionEffectCatalog2DBuilder::build(const std::vector<InteractionEffectEntry2D> &entries) const
{
	InteractionEffectCatalog2DBuildResult result;

	for (std::size_t entryIndex = 0; entryIndex < entries.size(); ++entryIndex) {
		const InteractionEffectEntry2D &entry = entries[entryIndex];
		if (entry.targetId.empty()) {
			result.issues.push_back({
				InteractionEffectCatalog2DIssueCode::MissingTargetId,
				entryIndex,
				0,
				entry,
				InteractionEffect2DStatus::Valid,
			});
		}
		if (entry.effects.empty()) {
			result.issues.push_back({
				InteractionEffectCatalog2DIssueCode::EmptyEffects,
				entryIndex,
				0,
				entry,
				InteractionEffect2DStatus::Valid,
			});
			continue;
		}
		for (std::size_t effectIndex = 0; effectIndex < entry.effects.size(); ++effectIndex) {
			const InteractionEffect2DStatus effectStatus = validate(entry.effects[effectIndex]);
			if (effectStatus == InteractionEffect2DStatus::Valid)
				continue;
			result.issues.push_back({
				InteractionEffectCatalog2DIssueCode::InvalidEffect,
				entryIndex,
				effectIndex,
				entry,
				effectStatus,
			});
		}
	}

	if (!result.issues.empty())
		return result;

	result.built = true;
	result.catalog = InteractionEffectCatalog2D { entries };
	return result;
}

} // namespace iggy
