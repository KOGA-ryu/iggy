#pragma once

#include "app/RuntimeArtifactOutputService.hpp"
#include "app/RuntimeLoopTypes.hpp"

namespace dev {

class RuntimeOutputFinalizer {
public:
	explicit RuntimeOutputFinalizer(RuntimeArtifactOutputService outputService = RuntimeArtifactOutputService {});

	void finalize(const RuntimeOutputSettings &settings, GameLoopResult &result) const;

private:
	RuntimeArtifactOutputService outputService_;
};

} // namespace dev
