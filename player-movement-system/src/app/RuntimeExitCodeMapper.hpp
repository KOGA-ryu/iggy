#pragma once

namespace dev {

class RuntimeExitCodeMapper {
public:
	[[nodiscard]] int exitCodeFor(bool failed) const;
};

} // namespace dev
