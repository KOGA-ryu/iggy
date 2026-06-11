#include "RuntimeDebugArtifactRootPreparer.hpp"

namespace dev {

bool RuntimeDebugArtifactRootPreparer::prepare(const std::filesystem::path &rootPath) const
{
	std::error_code error;
	std::filesystem::create_directories(rootPath, error);
	return !error;
}

} // namespace dev
