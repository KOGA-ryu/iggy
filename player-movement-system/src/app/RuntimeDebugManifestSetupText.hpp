#pragma once

#include <string>

#include "app/RuntimeLoopTypes.hpp"

namespace dev {

class RuntimeDebugManifestSetupText {
public:
	[[nodiscard]] std::string format(const RuntimeSetupResult &setup) const;
};

} // namespace dev
