#include "RuntimeOutputFailurePolicy.hpp"

namespace dev {

bool RuntimeOutputFailurePolicy::failed(const RuntimeOutputResult &result) const
{
	return (result.runTraceSaveAttempted && !result.runTraceSaved)
	    || (result.debugBundleSaveAttempted && !result.debugBundleSaved);
}

} // namespace dev
