#pragma once

#include <optional>

#include "save/SnapshotCodec.hpp"

namespace dev {

class SnapshotFrameCodec {
public:
	[[nodiscard]] SnapshotBytes encode(const SnapshotBytes &payload) const;
	[[nodiscard]] std::optional<SnapshotBytes> decode(const SnapshotBytes &bytes) const;
};

} // namespace dev
