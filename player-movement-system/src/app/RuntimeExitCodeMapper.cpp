#include "RuntimeExitCodeMapper.hpp"

namespace dev {

int RuntimeExitCodeMapper::exitCodeFor(bool failed) const
{
	return failed ? 1 : 0;
}

} // namespace dev
