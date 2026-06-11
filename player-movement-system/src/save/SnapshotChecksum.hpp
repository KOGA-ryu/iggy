#pragma once

#include <cstddef>
#include <cstdint>

#include "save/SnapshotCodec.hpp"

namespace dev {

class SnapshotChecksum {
public:
	void appendTo(SnapshotBytes &bytes) const;
	[[nodiscard]] bool hasValidTrailingChecksum(const SnapshotBytes &bytes, std::size_t payloadSize) const;
	[[nodiscard]] uint32_t compute(const SnapshotBytes &bytes, std::size_t length) const;

private:
	[[nodiscard]] uint32_t readTrailingU32(const SnapshotBytes &bytes) const;
};

} // namespace dev
