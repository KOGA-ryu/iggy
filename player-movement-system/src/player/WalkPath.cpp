#include "WalkPath.hpp"

namespace dev {

void WalkPath::clear()
{
	length_ = 0;
}

bool WalkPath::pushStep(Point tile)
{
	if (length_ == steps_.size())
		return false;
	steps_[length_++] = tile;
	return true;
}

std::optional<Point> WalkPath::popNext()
{
	if (length_ == 0)
		return std::nullopt;

	const Point next = steps_[0];
	for (std::size_t i = 1; i < length_; ++i) {
		steps_[i - 1] = steps_[i];
	}
	--length_;
	return next;
}

bool WalkPath::empty() const
{
	return length_ == 0;
}

std::size_t WalkPath::size() const
{
	return length_;
}

std::optional<Point> WalkPath::peekNext() const
{
	if (length_ == 0)
		return std::nullopt;
	return steps_[0];
}

} // namespace dev

