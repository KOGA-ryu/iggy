#pragma once

#include <cstddef>
#include <cstdint>

#include "session/SessionCommandLogBytes.hpp"

namespace dev {

class SessionCommandLogChecksum {
public:
	void appendTo(SessionCommandLogBytes &bytes) const;
	[[nodiscard]] bool hasValidTrailingChecksum(const SessionCommandLogBytes &bytes, std::size_t payloadSize) const;
	[[nodiscard]] uint32_t compute(const SessionCommandLogBytes &bytes, std::size_t length) const;

private:
	[[nodiscard]] uint32_t readTrailingU32(const SessionCommandLogBytes &bytes) const;
};

} // namespace dev
