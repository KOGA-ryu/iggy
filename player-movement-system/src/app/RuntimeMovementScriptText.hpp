#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "replay/MovementScriptRunner.hpp"

namespace dev {

class RuntimeMovementScriptText {
public:
	[[nodiscard]] std::string formatResult(std::string_view label, const MovementScriptRunResult &result) const;
	[[nodiscard]] std::string formatAggregate(std::string_view label, const std::vector<MovementScriptRunResult> &results) const;
};

} // namespace dev
