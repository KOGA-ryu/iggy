#include "scene/level/LevelCollisionSource2D.hpp"

#include <algorithm>
#include <cmath>

namespace iggy {
namespace {

[[nodiscard]] float ClampNonNegative(float value)
{
	return std::max(0.0F, value);
}

[[nodiscard]] bool HasPositiveSize(const DraftLevelCollisionBox2D &box)
{
	return box.size.x > 0.0F && box.size.y > 0.0F;
}

[[nodiscard]] bool IsRotated(const DraftLevelCollisionBox2D &box, float tolerance)
{
	return std::fabs(box.rotationRadians) > tolerance;
}

[[nodiscard]] LevelCollisionSource2DIssue Issue(
	LevelCollisionSource2DIssueCode code,
	std::size_t boxIndex,
	const DraftLevelCollisionBox2D &box,
	std::size_t geometryIssueCount = 0)
{
	return {
		code,
		boxIndex,
		box,
		geometryIssueCount,
	};
}

[[nodiscard]] LevelCollisionSourceBox2D SourceBox(const DraftLevelCollisionBox2D &box)
{
	return {
		box.sourceWallId,
		box.sourceWallIndex,
		box.center,
		box.size,
		box.rotationRadians,
		box.assetId,
		box.definitionId,
	};
}

} // namespace

bool LevelCollisionSource2DBuildResult::hasIssues() const
{
	return !issues.empty();
}

LevelCollisionSource2DBuildResult LevelCollisionSource2DBuilder::build(
	const DraftLevelGeometryPlan2DResult &geometry,
	const LevelCollisionSource2DConfig &config) const
{
	LevelCollisionSource2DBuildResult result;
	result.sourceGeometryBoxCount = geometry.collisionBoxes.size();
	result.geometryPlanHadIssues = geometry.hasIssues();

	if (result.geometryPlanHadIssues) {
		result.issues.push_back(Issue(
			LevelCollisionSource2DIssueCode::GeometryPlanHasIssues,
			0,
			{},
			geometry.issues.size()));
		if (!config.allowIssueBearingPlan)
			return result;
	}

	const float rotationTolerance = ClampNonNegative(config.rotationToleranceRadians);
	for (std::size_t boxIndex = 0; boxIndex < geometry.collisionBoxes.size(); ++boxIndex) {
		const DraftLevelCollisionBox2D &box = geometry.collisionBoxes[boxIndex];
		if (!HasPositiveSize(box)) {
			result.issues.push_back(Issue(LevelCollisionSource2DIssueCode::NonPositiveBoxSize, boxIndex, box));
			continue;
		}

		if (IsRotated(box, rotationTolerance)) {
			result.issues.push_back(Issue(LevelCollisionSource2DIssueCode::UnsupportedRotation, boxIndex, box));
			continue;
		}

		result.source.boxes.push_back(SourceBox(box));
	}

	result.built = true;
	return result;
}

} // namespace iggy
