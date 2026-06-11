#pragma once

#include "save/SimulationSnapshot.hpp"
#include "simulation/SimulationWorld.hpp"

namespace dev {

class SnapshotReader {
public:
	void read(const SimulationSnapshot &snapshot, SimulationWorld &world) const;
};

} // namespace dev
