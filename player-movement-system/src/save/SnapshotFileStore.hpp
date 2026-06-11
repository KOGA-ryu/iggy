#pragma once

#include <filesystem>
#include <optional>

#include "save/SnapshotCodec.hpp"

namespace dev {

class SnapshotFileStore {
public:
	explicit SnapshotFileStore(SnapshotCodec codec = SnapshotCodec {});

	[[nodiscard]] bool save(const std::filesystem::path &path, const SimulationSnapshot &snapshot) const;
	[[nodiscard]] std::optional<SimulationSnapshot> load(const std::filesystem::path &path) const;

private:
	SnapshotCodec codec_;
};

} // namespace dev
