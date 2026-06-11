#pragma once

#include "enemies/Enemy.hpp"
#include "save/SnapshotByteStream.hpp"
#include "save/SnapshotEntityCodec.hpp"

namespace dev {

class SnapshotEnemyCodec {
public:
	void writeEnemy(SnapshotByteWriter &writer, const Enemy &enemy) const;
	[[nodiscard]] bool readEnemy(SnapshotByteReader &reader, Enemy &enemy) const;

private:
	SnapshotEntityCodec entityCodec_;
};

} // namespace dev
