#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "app/GameLoop.hpp"
#include "app/RuntimeFrameTrace.hpp"
#include "app/RuntimeFrameTraceFileStore.hpp"

namespace dev {

class RuntimeTraceService {
public:
	explicit RuntimeTraceService(
	    RuntimeFrameTrace formatter = RuntimeFrameTrace {},
	    RuntimeFrameTraceFileStore fileStore = RuntimeFrameTraceFileStore {});

	[[nodiscard]] std::vector<std::string> formatRun(const GameLoopResult &result) const;
	[[nodiscard]] bool saveRunTrace(const std::filesystem::path &path, const GameLoopResult &result) const;

private:
	RuntimeFrameTrace formatter_;
	RuntimeFrameTraceFileStore fileStore_;
};

} // namespace dev
