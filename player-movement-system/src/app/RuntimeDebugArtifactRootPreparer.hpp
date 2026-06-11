#pragma once

#include <filesystem>

namespace dev {

class RuntimeDebugArtifactRootPreparer {
public:
	[[nodiscard]] bool prepare(const std::filesystem::path &rootPath) const;
};

} // namespace dev
