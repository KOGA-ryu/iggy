#include "scene/level/LevelCollisionSourceWorldBuilder2D.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include "core/math/Aabb2.hpp"
#include "servers/physics2d/CollisionShape2D.hpp"

namespace iggy {
namespace {

struct GeneratedSourceBox {
	std::size_t boxIndex = 0;
	LevelCollisionSourceBox2D box;
};

[[nodiscard]] float ClampNonNegative(float value)
{
	return std::max(0.0F, value);
}

[[nodiscard]] bool HasPositiveSize(const LevelCollisionSourceBox2D &box)
{
	return box.size.x > 0.0F && box.size.y > 0.0F;
}

[[nodiscard]] bool IsRotated(const LevelCollisionSourceBox2D &box, float tolerance)
{
	return std::fabs(box.rotationRadians) > tolerance;
}

[[nodiscard]] Aabb2 BoundsOf(const LevelCollisionSourceBox2D &box)
{
	const Vec2 halfSize { box.size.x * 0.5F, box.size.y * 0.5F };
	return {
		{ box.center.x - halfSize.x, box.center.y - halfSize.y },
		{ box.center.x + halfSize.x, box.center.y + halfSize.y },
	};
}

[[nodiscard]] bool HasRepeatedSameSourceWall(
	const LevelCollisionSource2D &source,
	std::size_t boxIndex)
{
	const LevelCollisionSourceBox2D &box = source.boxes[boxIndex];
	if (box.sourceWallId.empty())
		return false;

	std::size_t matchCount = 0;
	for (const LevelCollisionSourceBox2D &candidate : source.boxes) {
		if (candidate.sourceWallId == box.sourceWallId && candidate.sourceWallIndex == box.sourceWallIndex)
			++matchCount;
	}
	return matchCount > 1;
}

[[nodiscard]] ResourceId CollisionObjectId(
	const LevelCollisionSource2D &source,
	std::size_t boxIndex)
{
	const LevelCollisionSourceBox2D &box = source.boxes[boxIndex];
	if (!HasRepeatedSameSourceWall(source, boxIndex))
		return box.sourceWallId;

	std::string generatedId(box.sourceWallId.value());
	generatedId += "#segment:";
	generatedId += std::to_string(boxIndex);
	return ResourceId(std::move(generatedId));
}

[[nodiscard]] physics2d::CollisionObject2D CollisionObject(
	const LevelCollisionSource2D &source,
	std::size_t boxIndex)
{
	const LevelCollisionSourceBox2D &box = source.boxes[boxIndex];
	return {
		CollisionObjectId(source, boxIndex),
		physics2d::makeAabbShape(BoundsOf(box)),
		true,
	};
}

[[nodiscard]] LevelCollisionSourceWorldBuilder2DIssue Issue(
	LevelCollisionSourceWorldBuilder2DIssueCode code,
	std::size_t boxIndex,
	const LevelCollisionSourceBox2D &box)
{
	return {
		code,
		boxIndex,
		box,
	};
}

} // namespace

bool LevelCollisionSourceWorldBuilder2DResult::hasIssues() const
{
	return !issues.empty();
}

LevelCollisionSourceWorldBuilder2DResult LevelCollisionSourceWorldBuilder2D::build(
	const LevelCollisionSource2D &source,
	const LevelCollisionSourceWorldBuilder2DConfig &config) const
{
	LevelCollisionSourceWorldBuilder2DResult result;
	result.sourceBoxCount = source.boxes.size();

	std::vector<physics2d::CollisionObject2D> objects;
	std::vector<GeneratedSourceBox> generatedBoxes;
	const float rotationTolerance = ClampNonNegative(config.rotationToleranceRadians);

	for (std::size_t boxIndex = 0; boxIndex < source.boxes.size(); ++boxIndex) {
		const LevelCollisionSourceBox2D &box = source.boxes[boxIndex];
		if (!HasPositiveSize(box)) {
			result.issues.push_back(Issue(LevelCollisionSourceWorldBuilder2DIssueCode::NonPositiveBoxSize, boxIndex, box));
			continue;
		}

		if (IsRotated(box, rotationTolerance)) {
			result.issues.push_back(Issue(LevelCollisionSourceWorldBuilder2DIssueCode::UnsupportedRotation, boxIndex, box));
			continue;
		}

		objects.push_back(CollisionObject(source, boxIndex));
		generatedBoxes.push_back({ boxIndex, box });
	}

	result.generatedObjectCount = objects.size();

	const physics2d::CollisionWorldBuildResult worldBuild = physics2d::CollisionWorld2DBuilder {}.build(objects);
	if (worldBuild.built) {
		result.built = true;
		result.world = worldBuild.world;
		return result;
	}

	for (const physics2d::CollisionWorldBuildIssue &worldIssue : worldBuild.issues) {
		if (worldIssue.index < generatedBoxes.size()) {
			const GeneratedSourceBox &generated = generatedBoxes[worldIssue.index];
			result.issues.push_back(Issue(
				LevelCollisionSourceWorldBuilder2DIssueCode::CollisionWorldBuildFailed,
				generated.boxIndex,
				generated.box));
			continue;
		}

		result.issues.push_back(Issue(LevelCollisionSourceWorldBuilder2DIssueCode::CollisionWorldBuildFailed, 0, {}));
	}

	return result;
}

} // namespace iggy
