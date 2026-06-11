#pragma once

#include "app/RuntimeDebugArtifactLayout.hpp"
#include "app/RuntimeDebugManifest.hpp"
#include "app/RuntimeLoopTypes.hpp"
#include "files/TextFileStore.hpp"

namespace dev {

class RuntimeDebugManifestWriteStep {
public:
	explicit RuntimeDebugManifestWriteStep(
	    RuntimeDebugManifest manifest = RuntimeDebugManifest {},
	    TextFileStore textFileStore = TextFileStore {});

	[[nodiscard]] bool write(
	    const RuntimeDebugArtifactPaths &paths,
	    const GameLoopResult &result,
	    bool traceSaved) const;

private:
	RuntimeDebugManifest manifest_;
	TextFileStore textFileStore_;
};

} // namespace dev
