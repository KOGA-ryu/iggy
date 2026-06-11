#pragma once

#include <filesystem>

#include "save/SnapshotFileStore.hpp"
#include "save/SnapshotReader.hpp"
#include "save/SnapshotWriter.hpp"
#include "simulation/SimulationWorld.hpp"

namespace dev {

class SaveGameService {
public:
	SaveGameService(SnapshotWriter writer = SnapshotWriter {}, SnapshotReader reader = SnapshotReader {}, SnapshotFileStore store = SnapshotFileStore {});

	[[nodiscard]] bool saveWorld(const std::filesystem::path &path, const SimulationWorld &world) const;
	[[nodiscard]] bool loadWorld(const std::filesystem::path &path, SimulationWorld &world) const;

private:
	SnapshotWriter writer_;
	SnapshotReader reader_;
	SnapshotFileStore store_;
};

} // namespace dev
