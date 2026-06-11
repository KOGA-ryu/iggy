#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "save/SimulationSnapshot.hpp"

namespace dev {

using SnapshotBytes = std::vector<uint8_t>;

class SnapshotCodec {
public:
	[[nodiscard]] SnapshotBytes encode(const SimulationSnapshot &snapshot) const;
	[[nodiscard]] std::optional<SimulationSnapshot> decode(const SnapshotBytes &bytes) const;
};

} // namespace dev
