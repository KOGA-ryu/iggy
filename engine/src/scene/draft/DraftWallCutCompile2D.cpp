#include "scene/draft/DraftWallCutCompile2D.hpp"

#include <algorithm>
#include <cmath>

namespace iggy {
namespace {

struct CutInterval {
	float start = 0.0F;
	float end = 0.0F;
	DraftWallDoorMorphPlanDoor2D door;
};

[[nodiscard]] float ClampNonNegative(float value)
{
	return std::max(0.0F, value);
}

[[nodiscard]] DraftCompiledWallSegment2D OriginalSegment(const DraftCompiledWall2D &wall, bool hasDoorCut = false, const std::vector<ResourceId> &sourceDoorIds = {})
{
	return {
		wall.sourceSymbolId,
		wall.sourceSymbolIndex,
		wall.center,
		wall.size,
		wall.rotationRadians,
		wall.assetId,
		wall.definitionId,
		hasDoorCut,
		sourceDoorIds,
	};
}

[[nodiscard]] DraftWallCutCompile2DIssue Issue(
	DraftWallCutCompile2DIssueCode code,
	std::size_t wallPlanIndex,
	const DraftWallDoorMorphPlanWall2D &wallPlan,
	std::vector<DraftWallDoorMorphPlanDoor2D> doors)
{
	return {
		code,
		wallPlanIndex,
		wallPlan.wallIndex,
		wallPlan.wall,
		doors,
	};
}

[[nodiscard]] bool IsRotated(const DraftCompiledWall2D &wall, float tolerance)
{
	return std::fabs(wall.rotationRadians) > tolerance;
}

[[nodiscard]] bool IsHorizontal(const DraftCompiledWall2D &wall)
{
	return wall.size.x >= wall.size.y;
}

[[nodiscard]] float AxisCenter(const DraftCompiledWall2D &wall, bool horizontal)
{
	return horizontal ? wall.center.x : wall.center.y;
}

[[nodiscard]] float AxisSize(const DraftCompiledWall2D &wall, bool horizontal)
{
	return horizontal ? wall.size.x : wall.size.y;
}

[[nodiscard]] float DoorAxisCenter(const DraftCompiledDoor2D &door, bool horizontal)
{
	return horizontal ? door.center.x : door.center.y;
}

[[nodiscard]] float DoorAxisSize(const DraftCompiledDoor2D &door, bool horizontal)
{
	return horizontal ? door.size.x : door.size.y;
}

[[nodiscard]] Vec2 SegmentCenter(const DraftCompiledWall2D &wall, bool horizontal, float centerOnAxis)
{
	if (horizontal)
		return { centerOnAxis, wall.center.y };
	return { wall.center.x, centerOnAxis };
}

[[nodiscard]] Vec2 SegmentSize(const DraftCompiledWall2D &wall, bool horizontal, float sizeOnAxis)
{
	if (horizontal)
		return { sizeOnAxis, wall.size.y };
	return { wall.size.x, sizeOnAxis };
}

[[nodiscard]] std::vector<ResourceId> DoorIds(const std::vector<DraftWallDoorMorphPlanDoor2D> &doors)
{
	std::vector<ResourceId> ids;
	for (const DraftWallDoorMorphPlanDoor2D &door : doors)
		ids.push_back(door.door.sourceSymbolId);
	return ids;
}

[[nodiscard]] bool HasDoorLargerThanWall(const DraftCompiledWall2D &wall, const std::vector<DraftWallDoorMorphPlanDoor2D> &doors)
{
	for (const DraftWallDoorMorphPlanDoor2D &door : doors) {
		if (door.door.size.x > wall.size.x || door.door.size.y > wall.size.y)
			return true;
	}
	return false;
}

[[nodiscard]] bool BuildIntervals(
	const DraftCompiledWall2D &wall,
	const std::vector<DraftWallDoorMorphPlanDoor2D> &doors,
	bool horizontal,
	std::vector<CutInterval> &intervals)
{
	const float wallHalfSize = AxisSize(wall, horizontal) * 0.5F;
	const float wallStart = AxisCenter(wall, horizontal) - wallHalfSize;
	const float wallEnd = AxisCenter(wall, horizontal) + wallHalfSize;

	for (const DraftWallDoorMorphPlanDoor2D &door : doors) {
		const float doorHalfSize = DoorAxisSize(door.door, horizontal) * 0.5F;
		const float doorStart = DoorAxisCenter(door.door, horizontal) - doorHalfSize;
		const float doorEnd = DoorAxisCenter(door.door, horizontal) + doorHalfSize;
		if (doorStart < wallStart || doorEnd > wallEnd)
			return false;
		intervals.push_back({ doorStart, doorEnd, door });
	}

	std::sort(intervals.begin(), intervals.end(), [](const CutInterval &left, const CutInterval &right) {
		if (left.start == right.start)
			return left.end < right.end;
		return left.start < right.start;
	});
	return true;
}

[[nodiscard]] bool HasOverlappingIntervals(const std::vector<CutInterval> &intervals)
{
	for (std::size_t index = 1; index < intervals.size(); ++index) {
		if (intervals[index].start < intervals[index - 1].end)
			return true;
	}
	return false;
}

void AppendSegment(
	std::vector<DraftCompiledWallSegment2D> &segments,
	const DraftCompiledWall2D &wall,
	bool horizontal,
	float start,
	float end,
	float minimumSegmentSize,
	const std::vector<ResourceId> &sourceDoorIds)
{
	const float size = end - start;
	if (size <= minimumSegmentSize)
		return;

	segments.push_back({
		wall.sourceSymbolId,
		wall.sourceSymbolIndex,
		SegmentCenter(wall, horizontal, start + size * 0.5F),
		SegmentSize(wall, horizontal, size),
		wall.rotationRadians,
		wall.assetId,
		wall.definitionId,
		true,
		sourceDoorIds,
	});
}

} // namespace

bool DraftWallCutCompile2DResult::hasIssues() const
{
	return !issues.empty();
}

DraftWallCutCompile2DResult DraftWallCutCompiler2D::compile(
	const DraftWallDoorMorphPlan2DResult &plan,
	const DraftWallCutCompile2DConfig &config) const
{
	DraftWallCutCompile2DResult result;
	const float rotationTolerance = ClampNonNegative(config.rotationToleranceRadians);
	const float minimumSegmentSize = ClampNonNegative(config.minimumSegmentSize);

	for (std::size_t wallPlanIndex = 0; wallPlanIndex < plan.wallPlans.size(); ++wallPlanIndex) {
		const DraftWallDoorMorphPlanWall2D &wallPlan = plan.wallPlans[wallPlanIndex];
		if (!wallPlan.hasDoorCuts()) {
			result.segments.push_back(OriginalSegment(wallPlan.wall));
			continue;
		}

		if (IsRotated(wallPlan.wall, rotationTolerance)) {
			result.issues.push_back(Issue(DraftWallCutCompile2DIssueCode::UnsupportedRotatedWall, wallPlanIndex, wallPlan, wallPlan.doors));
			result.segments.push_back(OriginalSegment(wallPlan.wall));
			continue;
		}

		if (HasDoorLargerThanWall(wallPlan.wall, wallPlan.doors)) {
			result.issues.push_back(Issue(DraftWallCutCompile2DIssueCode::DoorLargerThanWall, wallPlanIndex, wallPlan, wallPlan.doors));
			result.segments.push_back(OriginalSegment(wallPlan.wall));
			continue;
		}

		const bool horizontal = IsHorizontal(wallPlan.wall);
		std::vector<CutInterval> intervals;
		if (!BuildIntervals(wallPlan.wall, wallPlan.doors, horizontal, intervals)) {
			result.issues.push_back(Issue(DraftWallCutCompile2DIssueCode::DoorOutsideWallBounds, wallPlanIndex, wallPlan, wallPlan.doors));
			result.segments.push_back(OriginalSegment(wallPlan.wall));
			continue;
		}

		if (HasOverlappingIntervals(intervals)) {
			result.issues.push_back(Issue(DraftWallCutCompile2DIssueCode::OverlappingDoorCuts, wallPlanIndex, wallPlan, wallPlan.doors));
			result.segments.push_back(OriginalSegment(wallPlan.wall));
			continue;
		}

		const float wallHalfSize = AxisSize(wallPlan.wall, horizontal) * 0.5F;
		const float wallStart = AxisCenter(wallPlan.wall, horizontal) - wallHalfSize;
		const float wallEnd = AxisCenter(wallPlan.wall, horizontal) + wallHalfSize;
		const std::vector<ResourceId> sourceDoorIds = DoorIds(wallPlan.doors);
		const std::size_t previousSegmentCount = result.segments.size();
		float cursor = wallStart;
		for (const CutInterval &interval : intervals) {
			AppendSegment(result.segments, wallPlan.wall, horizontal, cursor, interval.start, minimumSegmentSize, sourceDoorIds);
			cursor = interval.end;
		}
		AppendSegment(result.segments, wallPlan.wall, horizontal, cursor, wallEnd, minimumSegmentSize, sourceDoorIds);

		if (result.segments.size() == previousSegmentCount) {
			result.issues.push_back(Issue(DraftWallCutCompile2DIssueCode::NoPositiveSegmentProduced, wallPlanIndex, wallPlan, wallPlan.doors));
			result.segments.push_back(OriginalSegment(wallPlan.wall));
		}
	}

	return result;
}

} // namespace iggy
