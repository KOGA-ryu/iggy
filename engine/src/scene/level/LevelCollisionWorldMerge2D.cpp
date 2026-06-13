#include "scene/level/LevelCollisionWorldMerge2D.hpp"

namespace iggy {
namespace {

[[nodiscard]] LevelCollisionWorldMerge2DIssue Issue(
	LevelCollisionWorldMerge2DIssueCode code,
	std::size_t objectIndex,
	const physics2d::CollisionObject2D &object)
{
	return {
		code,
		objectIndex,
		object,
	};
}

[[nodiscard]] bool HasDuplicateNonEmptyId(
	const std::vector<physics2d::CollisionObject2D> &objects,
	std::size_t objectIndex)
{
	if (objects[objectIndex].id.empty())
		return false;

	for (std::size_t previous = 0; previous < objectIndex; ++previous) {
		if (objects[previous].id == objects[objectIndex].id)
			return true;
	}

	return false;
}

} // namespace

bool LevelCollisionWorldMerge2DResult::hasIssues() const
{
	return !issues.empty();
}

LevelCollisionWorldMerge2DResult LevelCollisionWorldMerge2D::merge(
	const physics2d::CollisionWorld2D &primary,
	const physics2d::CollisionWorld2D &secondary) const
{
	LevelCollisionWorldMerge2DResult result;
	result.primaryObjectCount = primary.objects().size();
	result.secondaryObjectCount = secondary.objects().size();

	std::vector<physics2d::CollisionObject2D> objects;
	objects.reserve(primary.objects().size() + secondary.objects().size());
	objects.insert(objects.end(), primary.objects().begin(), primary.objects().end());
	objects.insert(objects.end(), secondary.objects().begin(), secondary.objects().end());
	result.mergedObjectCount = objects.size();

	for (std::size_t objectIndex = 0; objectIndex < objects.size(); ++objectIndex) {
		if (!HasDuplicateNonEmptyId(objects, objectIndex))
			continue;

		result.issues.push_back(Issue(
			LevelCollisionWorldMerge2DIssueCode::DuplicateObjectId,
			objectIndex,
			objects[objectIndex]));
	}

	if (!result.issues.empty())
		return result;

	const physics2d::CollisionWorldBuildResult worldBuild = physics2d::CollisionWorld2DBuilder {}.build(objects);
	if (worldBuild.built) {
		result.built = true;
		result.world = worldBuild.world;
		return result;
	}

	for (const physics2d::CollisionWorldBuildIssue &buildIssue : worldBuild.issues) {
		result.issues.push_back(Issue(
			LevelCollisionWorldMerge2DIssueCode::WorldBuildFailed,
			buildIssue.index,
			buildIssue.object));
	}

	return result;
}

} // namespace iggy
