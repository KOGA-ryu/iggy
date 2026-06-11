#pragma once

#include <string>
#include <string_view>

#include "combat/CombatEvent.hpp"

namespace dev {

class RuntimeCombatText {
public:
	[[nodiscard]] std::string formatEvent(std::string_view label, const CombatEvent &event) const;
};

} // namespace dev
