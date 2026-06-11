#pragma once

#include "player/Player.hpp"
#include "save/SnapshotByteStream.hpp"
#include "save/SnapshotEntityCodec.hpp"

namespace dev {

class SnapshotPlayerCodec {
public:
	void writePlayer(SnapshotByteWriter &writer, const Player &player) const;
	[[nodiscard]] bool readPlayer(SnapshotByteReader &reader, Player &player) const;

private:
	SnapshotEntityCodec entityCodec_;
};

} // namespace dev
