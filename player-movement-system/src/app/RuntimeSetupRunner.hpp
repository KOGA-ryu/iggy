#pragma once

#include <vector>

#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeSourceDrainer.hpp"
#include "session/SessionCommandDispatcher.hpp"

namespace dev {

struct RuntimeSetupRunResult {
	RuntimeSetupResult setup;
	std::vector<InventoryCommandResult> inventoryCommandResults;
	bool framesAllowed = true;
};

class RuntimeSetupRunner {
public:
	RuntimeSetupRunner(
	    SessionCommandDispatcher &sessionDispatcher,
	    RuntimeSourceDrainer &sourceDrainer);

	[[nodiscard]] RuntimeSetupRunResult run(const RuntimeSetupSettings &settings) const;

private:
	SessionCommandDispatcher &sessionDispatcher_;
	RuntimeSourceDrainer &sourceDrainer_;
};

} // namespace dev
