#pragma once

#include <filesystem>

#include "app/RuntimeDebugArtifactLayout.hpp"
#include "app/RuntimeDebugArtifactRootPreparer.hpp"
#include "app/RuntimeDebugArtifactWriter.hpp"
#include "app/RuntimeLoopTypes.hpp"

namespace dev {

struct RuntimeDebugArtifactBundleResult {
	std::filesystem::path rootPath;
	std::filesystem::path manifestPath;
	std::filesystem::path tracePath;
	bool rootPrepared = false;
	bool traceSaved = false;
	bool manifestSaved = false;

	[[nodiscard]] bool saved() const;
};

class RuntimeDebugArtifactBundle {
public:
	explicit RuntimeDebugArtifactBundle(
	    RuntimeDebugArtifactLayout layout = RuntimeDebugArtifactLayout {},
	    RuntimeDebugArtifactRootPreparer rootPreparer = RuntimeDebugArtifactRootPreparer {},
	    RuntimeDebugArtifactWriter writer = RuntimeDebugArtifactWriter {});

	[[nodiscard]] RuntimeDebugArtifactBundleResult save(const std::filesystem::path &rootPath, const GameLoopResult &result) const;

private:
	RuntimeDebugArtifactLayout layout_;
	RuntimeDebugArtifactRootPreparer rootPreparer_;
	RuntimeDebugArtifactWriter writer_;
};

} // namespace dev
