#pragma once

#include <cstddef>
#include <string>

namespace dev {

class RuntimeRunTraceFrameHeaderText {
public:
	[[nodiscard]] std::string format(std::size_t frameIndex) const;
};

} // namespace dev
