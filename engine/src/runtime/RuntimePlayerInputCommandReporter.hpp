#pragma once

#include "runtime/RuntimePlayerInputCommandReport.hpp"

namespace iggy::runtime {

class RuntimePlayerInputCommandReporter {
public:
	[[nodiscard]] RuntimePlayerInputCommandReport report(const RuntimePlayerInputCommandRunnerResult &result) const;

	[[nodiscard]] RuntimePlayerInputGatedCommandReport reportGated(const RuntimePlayerInputGatedCommandRunnerResult &result) const;
};

} // namespace iggy::runtime
