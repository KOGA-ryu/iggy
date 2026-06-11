#pragma once

#include <filesystem>
#include <vector>

#include "app/RuntimeLoopTypes.hpp"

namespace dev {

enum class RuntimeArtifactOutputKind {
	RunTrace,
	DebugBundle,
};

struct RuntimeArtifactOutputRequest {
	RuntimeArtifactOutputKind kind = RuntimeArtifactOutputKind::RunTrace;
	std::filesystem::path path;
};

class RuntimeArtifactOutputPlan {
public:
	[[nodiscard]] std::vector<RuntimeArtifactOutputRequest> build(const RuntimeOutputSettings &settings) const;
};

} // namespace dev
