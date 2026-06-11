#pragma once

#include <string>

#include "app/RuntimeLoopTypes.hpp"

namespace dev {

class RuntimeFrameTraceHeaderText {
public:
	[[nodiscard]] std::string format(const RuntimeFrameReport &report) const;
};

} // namespace dev
