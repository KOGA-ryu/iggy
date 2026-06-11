#pragma once

#include <filesystem>

namespace dev {

struct RuntimeDebugArtifactPaths {
	std::filesystem::path rootPath;
	std::filesystem::path manifestPath;
	std::filesystem::path tracePath;
};

class RuntimeDebugArtifactLayout {
public:
	[[nodiscard]] RuntimeDebugArtifactPaths pathsForRoot(const std::filesystem::path &rootPath) const;
};

} // namespace dev
