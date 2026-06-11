#pragma once

#include "app/RuntimeArtifactOutputService.hpp"
#include "app/RuntimeLoopTypes.hpp"

namespace dev {

class RuntimeOutputFinalizer {
public:
	explicit RuntimeOutputFinalizer(RuntimeArtifactOutputService outputService = RuntimeArtifactOutputService {});

	void finalize(const RuntimeOutputSettings &settings, GameLoopResult &result) const;
	[[nodiscard]] static bool failed(const RuntimeOutputResult &result);

private:
	RuntimeArtifactOutputService outputService_;
};

} // namespace dev
