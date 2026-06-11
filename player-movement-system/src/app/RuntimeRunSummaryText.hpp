#pragma once

#include <string>

#include "app/RuntimeLoopTypes.hpp"

namespace dev {

enum class RuntimeRunSummaryDetail {
	CountsOnly,
	WithFinalMode,
};

class RuntimeRunSummaryText {
public:
	[[nodiscard]] std::string format(const GameLoopResult &result, RuntimeRunSummaryDetail detail) const;
};

} // namespace dev
