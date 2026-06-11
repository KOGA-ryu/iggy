#include "RuntimeRunTraceFrameHeaderText.hpp"

#include <sstream>

namespace dev {

std::string RuntimeRunTraceFrameHeaderText::format(std::size_t frameIndex) const
{
	std::ostringstream line;
	line << "frame[" << frameIndex << "]";
	return line.str();
}

} // namespace dev
