#include "RuntimeInputDrainResultBuilder.hpp"

namespace dev {

void RuntimeInputDrainResultBuilder::record(const RuntimeInputRouteResult &routeResult)
{
	if (routeResult.handled)
		++result_.handled;
	if (routeResult.movementBlockReason.has_value())
		result_.movementBlockReasons.push_back(*routeResult.movementBlockReason);
}

RuntimeInputDrainResult RuntimeInputDrainResultBuilder::build() const
{
	return result_;
}

} // namespace dev
