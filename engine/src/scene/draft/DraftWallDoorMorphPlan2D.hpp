#pragma once

#include <cstddef>
#include <vector>

#include "scene/draft/DraftWallDoorAttach2D.hpp"

namespace iggy {

struct DraftWallDoorMorphPlanDoor2D {
	std::size_t doorIndex = 0;
	DraftCompiledDoor2D door;
};

struct DraftWallDoorMorphPlanWall2D {
	std::size_t wallIndex = 0;
	DraftCompiledWall2D wall;
	std::vector<DraftWallDoorMorphPlanDoor2D> doors;

	[[nodiscard]] bool hasDoorCuts() const;
};

enum class DraftWallDoorMorphPlan2DIssueCode {
	AttachmentWallIndexOutOfRange,
	DoorLargerThanWall,
};

struct DraftWallDoorMorphPlan2DIssue {
	DraftWallDoorMorphPlan2DIssueCode code = DraftWallDoorMorphPlan2DIssueCode::AttachmentWallIndexOutOfRange;
	std::size_t attachmentIndex = 0;
	std::size_t wallIndex = 0;
	std::size_t doorIndex = 0;
	DraftCompiledDoor2D door;
	DraftCompiledWall2D wall;
};

struct DraftWallDoorMorphPlan2DResult {
	std::vector<DraftWallDoorMorphPlanWall2D> wallPlans;
	std::vector<DraftWallDoorMorphPlan2DIssue> issues;

	[[nodiscard]] bool hasIssues() const;
};

class DraftWallDoorMorphPlanner2D {
public:
	[[nodiscard]] DraftWallDoorMorphPlan2DResult plan(
		const std::vector<DraftCompiledWall2D> &walls,
		const DraftWallDoorAttach2DResult &attachments) const;
};

} // namespace iggy
