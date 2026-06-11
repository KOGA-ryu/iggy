#pragma once

#include <optional>

#include "save/SimulationSnapshot.hpp"
#include "save/SnapshotBytes.hpp"

namespace dev {

class SnapshotCodec {
public:
	[[nodiscard]] SnapshotBytes encode(const SimulationSnapshot &snapshot) const;
	[[nodiscard]] std::optional<SimulationSnapshot> decode(const SnapshotBytes &bytes) const;
};

} // namespace dev
