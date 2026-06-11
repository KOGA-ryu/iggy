#pragma once

#include <vector>

#include "targeting/TargetResolver.hpp"

namespace dev {

class TargetRegistry : public TargetResolver {
public:
	void add(Target target);
	void replaceAll(std::vector<Target> targets);
	void clear();
	[[nodiscard]] bool remove(TargetType type, TargetId id);
	[[nodiscard]] int removeAll(TargetType type);

	[[nodiscard]] Target resolveAtTile(Point tile) const override;
	[[nodiscard]] const std::vector<Target> &targets() const;
	[[nodiscard]] bool empty() const;

private:
	std::vector<Target> targets_;
};

} // namespace dev
