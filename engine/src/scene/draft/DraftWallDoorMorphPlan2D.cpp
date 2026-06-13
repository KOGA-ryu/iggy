#include "scene/draft/DraftWallDoorMorphPlan2D.hpp"

namespace iggy {
namespace {

[[nodiscard]] bool DoorIsLargerThanWall(const DraftCompiledDoor2D &door, const DraftCompiledWall2D &wall)
{
	return door.size.x > wall.size.x || door.size.y > wall.size.y;
}

} // namespace

bool DraftWallDoorMorphPlanWall2D::hasDoorCuts() const
{
	return !doors.empty();
}

bool DraftWallDoorMorphPlan2DResult::hasIssues() const
{
	return !issues.empty();
}

DraftWallDoorMorphPlan2DResult DraftWallDoorMorphPlanner2D::plan(
	const std::vector<DraftCompiledWall2D> &walls,
	const DraftWallDoorAttach2DResult &attachments) const
{
	DraftWallDoorMorphPlan2DResult result;
	for (std::size_t wallIndex = 0; wallIndex < walls.size(); ++wallIndex) {
		result.wallPlans.push_back({
			wallIndex,
			walls[wallIndex],
			{},
		});
	}

	for (std::size_t attachmentIndex = 0; attachmentIndex < attachments.attachments.size(); ++attachmentIndex) {
		const DraftWallDoorAttachment2D &attachment = attachments.attachments[attachmentIndex];
		if (attachment.wallIndex >= walls.size()) {
			result.issues.push_back({
				DraftWallDoorMorphPlan2DIssueCode::AttachmentWallIndexOutOfRange,
				attachmentIndex,
				attachment.wallIndex,
				attachment.doorIndex,
				attachment.door,
				{},
			});
			continue;
		}

		DraftWallDoorMorphPlanWall2D &wallPlan = result.wallPlans[attachment.wallIndex];
		wallPlan.doors.push_back({
			attachment.doorIndex,
			attachment.door,
		});

		if (DoorIsLargerThanWall(attachment.door, wallPlan.wall)) {
			result.issues.push_back({
				DraftWallDoorMorphPlan2DIssueCode::DoorLargerThanWall,
				attachmentIndex,
				attachment.wallIndex,
				attachment.doorIndex,
				attachment.door,
				wallPlan.wall,
			});
		}
	}

	return result;
}

} // namespace iggy
