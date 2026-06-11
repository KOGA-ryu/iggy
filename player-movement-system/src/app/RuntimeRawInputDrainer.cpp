#include "RuntimeRawInputDrainer.hpp"

namespace dev {

RuntimeRawInputDrainer::RuntimeRawInputDrainer(RuntimeInputRouter &router)
    : router_(router)
{
}

int RuntimeRawInputDrainer::drain(
    const std::vector<RawInputSource *> &sources,
    const RuntimeInputContext &context) const
{
	int routed = 0;
	for (RawInputSource *source : sources) {
		if (source == nullptr)
			continue;

		for (const RawInputEvent &event : source->drain()) {
			RuntimeInputRouteResult result = router_.route(event, context);
			if (result.handled)
				++routed;
		}
	}
	return routed;
}

} // namespace dev
