#pragma once

#include <cstddef>
#include <cstdint>

#include "replay/CommandLogBytes.hpp"

namespace dev {

class CommandLogChecksum {
public:
	void appendTo(CommandLogBytes &bytes) const;
	[[nodiscard]] bool hasValidTrailingChecksum(const CommandLogBytes &bytes, std::size_t payloadSize) const;
	[[nodiscard]] uint32_t compute(const CommandLogBytes &bytes, std::size_t length) const;

private:
	[[nodiscard]] uint32_t readTrailingU32(const CommandLogBytes &bytes) const;
};

} // namespace dev
