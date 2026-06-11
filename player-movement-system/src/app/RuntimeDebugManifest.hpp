#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "app/RuntimeLoopTypes.hpp"

namespace dev {

struct RuntimeDebugManifestContext {
	std::filesystem::path rootPath;
	std::filesystem::path manifestPath;
	std::filesystem::path tracePath;
	bool traceSaved = false;
};

class RuntimeDebugManifest {
public:
	[[nodiscard]] std::vector<std::string> format(
	    const GameLoopResult &result,
	    const RuntimeDebugManifestContext &context) const;
};

} // namespace dev
