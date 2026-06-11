#include "RuntimeRawInputDrainer.hpp"

namespace dev {

RuntimeRawInputDrainer::RuntimeRawInputDrainer(RuntimeInputRouter &router)
    : router_(router)
{
}

RuntimeInputDrainResult RuntimeRawInputDrainer::drain(
    const std::vector<RawInputSource *> &sources,
    const RuntimeInputContext &context) const
{
	RuntimeInputDrainResult drainResult;
	for (RawInputSource *source : sources) {
		if (source == nullptr)
			continue;

		for (const RawInputEvent &event : source->drain()) {
			RuntimeInputRouteResult result = router_.route(event, context);
			if (result.handled)
				++drainResult.handled;
			if (result.movementBlockReason.has_value())
				drainResult.movementBlockReasons.push_back(*result.movementBlockReason);
		}
	}
	return drainResult;
}

} // namespace dev
