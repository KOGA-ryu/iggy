#pragma once

#include "save/SimulationSnapshot.hpp"
#include "save/SnapshotByteStream.hpp"
#include "save/SnapshotEnemyCodec.hpp"
#include "save/SnapshotEntityCodec.hpp"
#include "save/SnapshotPlayerCodec.hpp"
#include "save/SnapshotVectorCodec.hpp"

namespace dev {

class SnapshotSchemaCodec {
public:
	void writeSnapshot(SnapshotByteWriter &writer, const SimulationSnapshot &snapshot) const;
	[[nodiscard]] bool readSnapshot(SnapshotByteReader &reader, SimulationSnapshot &snapshot) const;

private:
	SnapshotEntityCodec entityCodec_;
	SnapshotPlayerCodec playerCodec_;
	SnapshotEnemyCodec enemyCodec_;
	SnapshotVectorCodec vectorCodec_;
};

} // namespace dev
