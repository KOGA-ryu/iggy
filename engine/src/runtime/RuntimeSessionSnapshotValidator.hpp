#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeSessionSnapshot.hpp"

namespace iggy::runtime {

enum class RuntimeSessionSnapshotIssueCode {
	InvalidMapDimensions,
	TileCountMismatch,
	PlayerOutOfBounds,
	NpcOutOfBounds,
};

struct RuntimeSessionSnapshotIssue {
	RuntimeSessionSnapshotIssueCode code = RuntimeSessionSnapshotIssueCode::InvalidMapDimensions;
	std::size_t index = 0;
};

struct RuntimeSessionSnapshotValidationResult {
	bool valid = false;
	std::vector<RuntimeSessionSnapshotIssue> issues;
};

class RuntimeSessionSnapshotValidator {
public:
	[[nodiscard]] RuntimeSessionSnapshotValidationResult validate(const RuntimeSessionSnapshot &snapshot) const;
};

} // namespace iggy::runtime
