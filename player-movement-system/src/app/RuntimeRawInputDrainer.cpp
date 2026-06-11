#include "RuntimeRawInputDrainer.hpp"

#include "app/RuntimeInputDrainResultBuilder.hpp"

namespace dev {

RuntimeRawInputDrainer::RuntimeRawInputDrainer(RuntimeInputRouter &router)
    : router_(router)
{
}

RuntimeInputDrainResult RuntimeRawInputDrainer::drain(
    const std::vector<RawInputSource *> &sources,
    const RuntimeInputContext &context) const
{
	RuntimeInputDrainResultBuilder drainResult;
	for (RawInputSource *source : sources) {
		if (source == nullptr)
			continue;

		for (const RawInputEvent &event : source->drain()) {
			drainResult.record(router_.route(event, context));
		}
	}
	return drainResult.build();
}

} // namespace dev
