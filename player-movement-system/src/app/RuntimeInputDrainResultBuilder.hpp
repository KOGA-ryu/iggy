#pragma once

#include "app/RuntimeInputTypes.hpp"

namespace dev {

class RuntimeInputDrainResultBuilder {
public:
	void record(const RuntimeInputRouteResult &routeResult);
	[[nodiscard]] RuntimeInputDrainResult build() const;

private:
	RuntimeInputDrainResult result_;
};

} // namespace dev
