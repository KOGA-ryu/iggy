#include "TargetRegistry.hpp"

#include <algorithm>
#include <utility>

namespace dev {

void TargetRegistry::add(Target target)
{
	targets_.push_back(target);
}

void TargetRegistry::replaceAll(std::vector<Target> targets)
{
	targets_ = std::move(targets);
}

void TargetRegistry::clear()
{
	targets_.clear();
}

bool TargetRegistry::remove(TargetType type, TargetId id)
{
	const auto before = targets_.size();
	targets_.erase(
	    std::remove_if(targets_.begin(), targets_.end(), [type, id](const Target &target) {
		    return target.type == type && target.id == id;
	    }),
	    targets_.end());
	return targets_.size() != before;
}

int TargetRegistry::removeAll(TargetType type)
{
	const auto before = targets_.size();
	targets_.erase(
	    std::remove_if(targets_.begin(), targets_.end(), [type](const Target &target) {
		    return target.type == type;
	    }),
	    targets_.end());
	return static_cast<int>(before - targets_.size());
}

Target TargetRegistry::resolveAtTile(Point tile) const
{
	for (const Target &target : targets_) {
		if (target.tile == tile)
			return target;
	}

	return {
		.type = TargetType::EmptyTile,
		.id = 0,
		.tile = tile,
	};
}

const std::vector<Target> &TargetRegistry::targets() const
{
	return targets_;
}

bool TargetRegistry::empty() const
{
	return targets_.empty();
}

} // namespace dev
