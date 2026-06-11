#pragma once

#include <string>
#include <string_view>

#include "events/MovementEvent.hpp"

namespace dev {

class RuntimeMovementEventText {
public:
	[[nodiscard]] std::string formatEvent(std::string_view label, const MovementEvent &event) const;
};

} // namespace dev
