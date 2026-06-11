#pragma once

#include <cstddef>
#include <vector>

namespace dev {

class RuntimeEventStreamDelta {
public:
	template <typename Event>
	[[nodiscard]] std::vector<Event> eventsSince(const std::vector<Event> &events, std::size_t offset) const
	{
		if (offset >= events.size())
			return {};

		return {
			events.begin() + static_cast<typename std::vector<Event>::difference_type>(offset),
			events.end(),
		};
	}
};

} // namespace dev
