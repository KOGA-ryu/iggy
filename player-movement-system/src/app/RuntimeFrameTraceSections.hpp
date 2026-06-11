#pragma once

#include <string>
#include <vector>

#include "app/RuntimeLoopTypes.hpp"

namespace dev {

class RuntimeFrameTraceSections {
public:
	[[nodiscard]] std::vector<std::string> formatRuntimeSources(const RuntimeFrameReport &report) const;
	[[nodiscard]] std::vector<std::string> formatLifecycleEvents(const RuntimeFrameReport &report) const;
	[[nodiscard]] std::vector<std::string> formatSimulationEvents(const RuntimeFrameReport &report) const;
};

} // namespace dev
