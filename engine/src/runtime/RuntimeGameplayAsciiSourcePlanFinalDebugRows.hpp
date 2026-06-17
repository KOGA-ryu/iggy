#pragma once

#include <string>
#include <vector>

#include "runtime/RuntimeGameplayAsciiSourcePlan.hpp"
#include "runtime/RuntimeGameplayState.hpp"

namespace iggy::runtime {

[[nodiscard]] std::vector<std::string> finalDebugRowsForAsciiSourcePlan(
	const RuntimeGameplayAsciiSourcePlan &plan,
	const RuntimeGameplayState &state);

} // namespace iggy::runtime
