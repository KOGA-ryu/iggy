#include "scene/draft/DraftLevelGeometryPlan2D.hpp"

#include <algorithm>
#include <cmath>

namespace iggy {
namespace {

[[nodiscard]] float ClampNonNegative(float value)
{
	return std::max(0.0F, value);
}

[[nodiscard]] std::size_t CountBuildingIssues(const DraftBuildingCompile2DResult &building)
{
	return building.plan.issues.size()
		+ building.walls.issues.size()
		+ building.doors.issues.size()
		+ building.attachments.issues.size()
		+ building.morphPlan.issues.size()
		+ building.wallCuts.issues.size();
}

[[nodiscard]] bool HasPositiveSize(const DraftCompiledWallSegment2D &segment)
{
	return segment.size.x > 0.0F && segment.size.y > 0.0F;
}

[[nodiscard]] bool HasPositiveSize(const DraftSymbol2D &symbol)
{
	return symbol.size.x > 0.0F && symbol.size.y > 0.0F;
}

[[nodiscard]] bool IsRotated(const DraftCompiledWallSegment2D &segment, float tolerance)
{
	return std::fabs(segment.rotationRadians) > tolerance;
}

[[nodiscard]] DraftLevelGeometryPlan2DIssue Issue(
	DraftLevelGeometryPlan2DIssueCode code,
	std::size_t segmentIndex,
	const DraftCompiledWallSegment2D &segment,
	std::size_t buildingIssueCount = 0)
{
	return {
		code,
		segmentIndex,
		segment,
		buildingIssueCount,
		0,
		{},
	};
}

[[nodiscard]] DraftLevelGeometryPlan2DIssue CollisionBlockerIssue(
	DraftLevelGeometryPlan2DIssueCode code,
	std::size_t symbolIndex,
	const DraftSymbol2D &symbol)
{
	return {
		code,
		0,
		{},
		0,
		symbolIndex,
		symbol,
	};
}

[[nodiscard]] DraftLevelCollisionBox2D CollisionBox(const DraftCompiledWallSegment2D &segment)
{
	return {
		segment.sourceWallId,
		segment.sourceWallIndex,
		segment.center,
		segment.size,
		segment.rotationRadians,
		segment.assetId,
		segment.definitionId,
	};
}

[[nodiscard]] DraftLevelCollisionBox2D CollisionBox(const DraftCollisionBlockerPlan2D &blocker)
{
	return {
		blocker.symbol.id,
		blocker.symbolIndex,
		blocker.symbol.position,
		blocker.symbol.size,
		blocker.symbol.rotationRadians,
		blocker.symbol.assetId,
		blocker.symbol.definitionId,
	};
}

} // namespace

bool DraftLevelGeometryPlan2DResult::hasIssues() const
{
	return !issues.empty();
}

DraftLevelGeometryPlan2DResult DraftLevelGeometryPlanner2D::plan(
	const DraftBuildingCompile2DResult &building,
	const DraftLevelGeometryPlan2DConfig &config) const
{
	DraftLevelGeometryPlan2DResult result;
	result.sourceWallSegmentCount = building.wallCuts.segments.size();
	result.buildingCompileHadIssues = building.hasIssues();

	const std::size_t buildingIssueCount = CountBuildingIssues(building);
	if (result.buildingCompileHadIssues) {
		result.issues.push_back(Issue(DraftLevelGeometryPlan2DIssueCode::BuildingCompileHasIssues, 0, {}, buildingIssueCount));
		if (!config.allowIssueBearingCompile)
			return result;
	}

	const float rotationTolerance = ClampNonNegative(config.rotationToleranceRadians);
	for (std::size_t segmentIndex = 0; segmentIndex < building.wallCuts.segments.size(); ++segmentIndex) {
		const DraftCompiledWallSegment2D &segment = building.wallCuts.segments[segmentIndex];
		if (!HasPositiveSize(segment)) {
			result.issues.push_back(Issue(DraftLevelGeometryPlan2DIssueCode::NonPositiveSegmentSize, segmentIndex, segment));
			continue;
		}

		if (IsRotated(segment, rotationTolerance)) {
			result.issues.push_back(Issue(DraftLevelGeometryPlan2DIssueCode::UnsupportedRotatedSegment, segmentIndex, segment));
			continue;
		}

		result.collisionBoxes.push_back(CollisionBox(segment));
	}

	for (const DraftCollisionBlockerPlan2D &blocker : building.plan.collisionBlockers) {
		if (!HasPositiveSize(blocker.symbol)) {
			result.issues.push_back(CollisionBlockerIssue(
				DraftLevelGeometryPlan2DIssueCode::NonPositiveCollisionBlockerSize,
				blocker.symbolIndex,
				blocker.symbol));
			continue;
		}

		result.collisionBoxes.push_back(CollisionBox(blocker));
	}

	return result;
}

} // namespace iggy
