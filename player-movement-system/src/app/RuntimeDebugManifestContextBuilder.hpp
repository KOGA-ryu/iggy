#pragma once

#include "app/RuntimeDebugArtifactLayout.hpp"
#include "app/RuntimeDebugManifest.hpp"

namespace dev {

class RuntimeDebugManifestContextBuilder {
public:
	[[nodiscard]] RuntimeDebugManifestContext build(
	    const RuntimeDebugArtifactPaths &paths,
	    bool traceSaved) const;
};

} // namespace dev
