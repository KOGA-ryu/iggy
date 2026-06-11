#include "RuntimeSetupRunResultApplier.hpp"

namespace dev {

bool RuntimeSetupRunResultApplier::apply(
    const RuntimeSetupRunResult &setupResult,
    GameLoopResult &result,
    RuntimeRunRecorder &recorder) const
{
	result.setup = setupResult.setup;
	recorder.recordSetupInventoryCommandResults(setupResult.inventoryCommandResults);

	return setupResult.framesAllowed;
}

} // namespace dev
