#pragma once

#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeSourceDrainer.hpp"

namespace dev {

class RuntimeSourceDrainerSettingsBuilder {
public:
	[[nodiscard]] RuntimeSourceDrainerSettings build(
	    const RuntimeSourceSettings &sources,
	    const RuntimeInputSettings &input) const;
};

} // namespace dev
