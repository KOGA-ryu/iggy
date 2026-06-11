#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "app/GameLoop.hpp"
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
	explicit RuntimeDebugArtifactBundle(RuntimeTraceService traceService = RuntimeTraceService {});

	[[nodiscard]] RuntimeDebugArtifactBundleResult save(const std::filesystem::path &rootPath, const GameLoopResult &result) const;
	[[nodiscard]] std::vector<std::string> formatManifest(
	    const GameLoopResult &result,
	    const RuntimeDebugArtifactBundleResult &bundle) const;

private:
	[[nodiscard]] bool saveLines(const std::filesystem::path &path, const std::vector<std::string> &lines) const;

	RuntimeTraceService traceService_;
};

} // namespace dev
