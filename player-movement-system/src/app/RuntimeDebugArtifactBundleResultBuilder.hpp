#pragma once

#include "app/RuntimeDebugArtifactBundle.hpp"
#include "app/RuntimeDebugArtifactLayout.hpp"
#include "app/RuntimeDebugArtifactWriter.hpp"

namespace dev {

class RuntimeDebugArtifactBundleResultBuilder {
public:
	explicit RuntimeDebugArtifactBundleResultBuilder(const RuntimeDebugArtifactPaths &paths);

	void markRootPrepared();
	void recordWrite(const RuntimeDebugArtifactWriteResult &write);

	[[nodiscard]] RuntimeDebugArtifactBundleResult result() const;

private:
	RuntimeDebugArtifactBundleResult bundle_;
};

} // namespace dev
