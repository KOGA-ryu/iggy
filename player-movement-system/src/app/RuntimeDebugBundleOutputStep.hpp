#pragma once

#include <filesystem>

#include "app/RuntimeDebugArtifactBundle.hpp"
#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeOutputResultBuilder.hpp"

namespace dev {

class RuntimeDebugBundleOutputStep {
public:
	explicit RuntimeDebugBundleOutputStep(RuntimeDebugArtifactBundle debugBundle = RuntimeDebugArtifactBundle {});

	void save(
	    const std::filesystem::path &path,
	    const GameLoopResult &result,
	    RuntimeOutputResultBuilder &output) const;

private:
	RuntimeDebugArtifactBundle debugBundle_;
};

} // namespace dev
