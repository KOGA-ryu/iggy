#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <vector>

#include "world/Point.hpp"

namespace dev {

constexpr std::size_t MaxWalkPathLength = 100;

class WalkPath {
public:
	void clear();
	bool pushStep(Point tile);
	std::optional<Point> popNext();

	[[nodiscard]] bool empty() const;
	[[nodiscard]] std::size_t size() const;
	[[nodiscard]] std::optional<Point> peekNext() const;
	[[nodiscard]] std::vector<Point> steps() const;
	void replace(std::vector<Point> steps);

private:
	std::array<Point, MaxWalkPathLength> steps_ {};
	std::size_t length_ = 0;
};

} // namespace dev
