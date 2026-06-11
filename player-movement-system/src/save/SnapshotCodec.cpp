#include "SnapshotCodec.hpp"

#include "save/SnapshotByteStream.hpp"
#include "save/SnapshotFrameCodec.hpp"
#include "save/SnapshotSchemaCodec.hpp"

namespace dev {

SnapshotBytes SnapshotCodec::encode(const SimulationSnapshot &snapshot) const
{
	SnapshotBytes payload;
	SnapshotByteWriter writer { payload };
	SnapshotSchemaCodec {}.writeSnapshot(writer, snapshot);
	return SnapshotFrameCodec {}.encode(payload);
}

std::optional<SimulationSnapshot> SnapshotCodec::decode(const SnapshotBytes &bytes) const
{
	std::optional<SnapshotBytes> payload = SnapshotFrameCodec {}.decode(bytes);
	if (!payload.has_value())
		return std::nullopt;

	SnapshotByteReader reader { *payload };
	SimulationSnapshot snapshot;
	if (!SnapshotSchemaCodec {}.readSnapshot(reader, snapshot))
		return std::nullopt;

	return snapshot;
}

} // namespace dev
