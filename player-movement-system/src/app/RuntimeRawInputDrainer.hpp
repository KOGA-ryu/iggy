#pragma once

#include <vector>

#include "app/RuntimeInputRouter.hpp"
#include "input/RawInputSource.hpp"

namespace dev {

class RuntimeRawInputDrainer {
public:
	explicit RuntimeRawInputDrainer(RuntimeInputRouter &router);

	[[nodiscard]] int drain(
	    const std::vector<RawInputSource *> &sources,
	    const RuntimeInputContext &context) const;

private:
	RuntimeInputRouter &router_;
};

} // namespace dev
