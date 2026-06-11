#include "SaveGameService.hpp"

namespace dev {

SaveGameService::SaveGameService(SnapshotWriter writer, SnapshotReader reader, SnapshotFileStore store)
    : writer_(writer)
    , reader_(reader)
    , store_(store)
{
}

bool SaveGameService::saveWorld(const std::filesystem::path &path, const SimulationWorld &world) const
{
	return store_.save(path, writer_.write(world));
}

bool SaveGameService::loadWorld(const std::filesystem::path &path, SimulationWorld &world) const
{
	std::optional<SimulationSnapshot> snapshot = store_.load(path);
	if (!snapshot.has_value())
		return false;

	reader_.read(*snapshot, world);
	return true;
}

} // namespace dev
