#include "scene/draft/DraftWallDoorAttach2D.hpp"

#include <algorithm>

namespace iggy {
namespace {

[[nodiscard]] bool ContainsDoorCenter(const DraftCompiledWall2D &wall, const DraftCompiledDoor2D &door, float tolerance)
{
	const float halfWidth = wall.size.x * 0.5F;
	const float halfHeight = wall.size.y * 0.5F;
	return door.center.x >= wall.center.x - halfWidth - tolerance
		&& door.center.x <= wall.center.x + halfWidth + tolerance
		&& door.center.y >= wall.center.y - halfHeight - tolerance
		&& door.center.y <= wall.center.y + halfHeight + tolerance;
}

} // namespace

bool DraftWallDoorAttach2DResult::hasIssues() const
{
	return !issues.empty();
}

DraftWallDoorAttach2DResult DraftWallDoorAttach2D::attach(
	const std::vector<DraftCompiledWall2D> &walls,
	const std::vector<DraftCompiledDoor2D> &doors,
	const DraftWallDoorAttach2DConfig &config) const
{
	DraftWallDoorAttach2DResult result;
	const float tolerance = std::max(0.0F, config.positionTolerance);

	for (std::size_t doorIndex = 0; doorIndex < doors.size(); ++doorIndex) {
		std::vector<std::size_t> containingWallIndexes;
		std::vector<DraftCompiledWall2D> containingWalls;
		for (std::size_t wallIndex = 0; wallIndex < walls.size(); ++wallIndex) {
			if (ContainsDoorCenter(walls[wallIndex], doors[doorIndex], tolerance)) {
				containingWallIndexes.push_back(wallIndex);
				containingWalls.push_back(walls[wallIndex]);
			}
		}

		if (containingWallIndexes.size() == 1) {
			result.attachments.push_back({
				doorIndex,
				containingWallIndexes[0],
				doors[doorIndex],
				containingWalls[0],
			});
			continue;
		}

		result.issues.push_back({
			containingWallIndexes.empty()
				? DraftWallDoorAttach2DIssueCode::NoContainingWall
				: DraftWallDoorAttach2DIssueCode::AmbiguousContainingWall,
			doorIndex,
			doors[doorIndex],
			containingWallIndexes,
			containingWalls,
		});
	}

	return result;
}

} // namespace iggy
