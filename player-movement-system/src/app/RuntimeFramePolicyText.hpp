#pragma once

#include <string>
#include <string_view>

#include "simulation/SimulationFramePolicyDescriber.hpp"

namespace dev {

enum class RuntimeFramePolicyBoolStyle {
	Numeric,
	Words,
};

class RuntimeFramePolicyText {
public:
	[[nodiscard]] std::string format(
	    std::string_view label,
	    const SimulationFramePolicyDescription &description,
	    RuntimeFramePolicyBoolStyle boolStyle) const;
	[[nodiscard]] std::string formatNone(std::string_view label) const;
};

} // namespace dev
