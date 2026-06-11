#include "RuntimeSetupFrameGate.hpp"

#include <utility>

namespace dev {

RuntimeSetupFrameGate::RuntimeSetupFrameGate(RuntimeSetupFailurePolicy failurePolicy)
    : failurePolicy_(std::move(failurePolicy))
{
}

bool RuntimeSetupFrameGate::allowsFrames(const RuntimeSetupResult &setup) const
{
	return !failurePolicy_.failed(setup);
}

} // namespace dev
