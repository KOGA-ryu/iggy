#pragma once

#include <filesystem>

#include "app/RuntimeDebugArtifactLayout.hpp"
#include "app/RuntimeDebugManifest.hpp"
#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeTraceService.hpp"

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
	    RuntimeTraceService traceService = RuntimeTraceService {},
	    RuntimeDebugManifest manifest = RuntimeDebugManifest {},
	    RuntimeDebugArtifactLayout layout = RuntimeDebugArtifactLayout {});

	[[nodiscard]] RuntimeDebugArtifactBundleResult save(const std::filesystem::path &rootPath, const GameLoopResult &result) const;

private:
	RuntimeTraceService traceService_;
	RuntimeDebugManifest manifest_;
	RuntimeDebugArtifactLayout layout_;
};

} // namespace dev
