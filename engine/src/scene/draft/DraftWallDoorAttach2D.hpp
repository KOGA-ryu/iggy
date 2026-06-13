#pragma once

#include <cstddef>
#include <vector>

#include "scene/draft/DraftDoorCompile2D.hpp"
#include "scene/draft/DraftWallCompile2D.hpp"

namespace iggy {

struct DraftWallDoorAttach2DConfig {
	float positionTolerance = 0.001F;
};

struct DraftWallDoorAttachment2D {
	std::size_t doorIndex = 0;
	std::size_t wallIndex = 0;
	DraftCompiledDoor2D door;
	DraftCompiledWall2D wall;
};

enum class DraftWallDoorAttach2DIssueCode {
	NoContainingWall,
	AmbiguousContainingWall,
};

struct DraftWallDoorAttach2DIssue {
	DraftWallDoorAttach2DIssueCode code = DraftWallDoorAttach2DIssueCode::NoContainingWall;
	std::size_t doorIndex = 0;
	DraftCompiledDoor2D door;
	std::vector<std::size_t> wallIndexes;
	std::vector<DraftCompiledWall2D> walls;
};

struct DraftWallDoorAttach2DResult {
	std::vector<DraftWallDoorAttachment2D> attachments;
	std::vector<DraftWallDoorAttach2DIssue> issues;

	[[nodiscard]] bool hasIssues() const;
};

class DraftWallDoorAttach2D {
public:
	[[nodiscard]] DraftWallDoorAttach2DResult attach(
		const std::vector<DraftCompiledWall2D> &walls,
		const std::vector<DraftCompiledDoor2D> &doors,
		const DraftWallDoorAttach2DConfig &config = {}) const;
};

} // namespace iggy
