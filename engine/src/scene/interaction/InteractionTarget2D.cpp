#include "scene/interaction/InteractionTarget2D.hpp"

#include <utility>

namespace {

bool HasEarlierMatchingId(const std::vector<iggy::InteractionTarget2D> &targets, std::size_t currentIndex)
{
	for (std::size_t index = 0; index < currentIndex; ++index) {
		if (targets[index].id == targets[currentIndex].id)
			return true;
	}
	return false;
}

} // namespace

namespace iggy {

InteractionTarget2DRegistry::InteractionTarget2DRegistry(std::vector<InteractionTarget2D> targets)
    : targets_(std::move(targets))
{
}

const std::vector<InteractionTarget2D> &InteractionTarget2DRegistry::targets() const
{
	return targets_;
}

const InteractionTarget2D *InteractionTarget2DRegistry::find(const ResourceId &id) const
{
	for (const InteractionTarget2D &target : targets_) {
		if (target.id == id)
			return &target;
	}
	return nullptr;
}

bool InteractionTarget2DRegistry::contains(const ResourceId &id) const
{
	return find(id) != nullptr;
}

InteractionTarget2DRegistryBuildResult InteractionTarget2DRegistryBuilder::build(const std::vector<InteractionTarget2D> &targets) const
{
	InteractionTarget2DRegistryBuildResult result;
	for (std::size_t index = 0; index < targets.size(); ++index) {
		const InteractionTarget2D &target = targets[index];
		if (target.id.empty()) {
			result.issues.push_back({
				InteractionTarget2DRegistryIssueCode::EmptyId,
				index,
				target,
			});
		}
		if (HasEarlierMatchingId(targets, index)) {
			result.issues.push_back({
				InteractionTarget2DRegistryIssueCode::DuplicateId,
				index,
				target,
			});
		}
		if (target.radius < 0.0F) {
			result.issues.push_back({
				InteractionTarget2DRegistryIssueCode::InvalidRadius,
				index,
				target,
			});
		}
	}

	if (!result.issues.empty())
		return result;

	result.built = true;
	result.registry = InteractionTarget2DRegistry { targets };
	return result;
}

} // namespace iggy
