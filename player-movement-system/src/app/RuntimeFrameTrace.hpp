#pragma once

#include <string>
#include <vector>

#include "app/RuntimeLoopTypes.hpp"

namespace dev {

class RuntimeFrameTrace {
public:
	[[nodiscard]] std::vector<std::string> format(const RuntimeFrameReport &report) const;
};

} // namespace dev
