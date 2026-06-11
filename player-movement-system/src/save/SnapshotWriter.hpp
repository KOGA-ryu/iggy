#pragma once

#include "save/SimulationSnapshot.hpp"
#include "simulation/SimulationWorld.hpp"

namespace dev {

class SnapshotWriter {
public:
	[[nodiscard]] SimulationSnapshot write(const SimulationWorld &world) const;
};

} // namespace dev
