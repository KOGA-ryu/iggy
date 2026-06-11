#pragma once

#include <string>
#include <vector>

#include "app/RuntimeLoopTypes.hpp"

namespace dev {

class RuntimeDebugManifestSections {
public:
	[[nodiscard]] std::vector<std::string> formatRunStatus(const GameLoopResult &result) const;
	[[nodiscard]] std::vector<std::string> formatSetup(const GameLoopResult &result) const;
	[[nodiscard]] std::vector<std::string> formatRuntimeScripts(const GameLoopResult &result) const;
};

} // namespace dev
