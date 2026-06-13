#include "scene/draft/DraftDoorCompile2D.hpp"

namespace iggy {
namespace {

[[nodiscard]] bool HasPositiveSize(Vec2 size)
{
	return size.x > 0.0F && size.y > 0.0F;
}

} // namespace

bool DraftDoorCompile2DResult::hasIssues() const
{
	return !issues.empty();
}

DraftDoorCompile2DResult DraftDoorCompiler2D::compile(const DraftCompilePlan2DResult &plan) const
{
	DraftDoorCompile2DResult result;
	for (const DraftDoorPlan2D &door : plan.doors) {
		if (!HasPositiveSize(door.symbol.size)) {
			result.issues.push_back({
				DraftDoorCompile2DIssueCode::NonPositiveSize,
				door.symbolIndex,
				door.symbol,
			});
			continue;
		}

		result.doors.push_back({
			door.symbol.id,
			door.symbolIndex,
			door.symbol.position,
			door.symbol.size,
			door.symbol.rotationRadians,
			door.symbol.assetId,
			door.symbol.definitionId,
			DraftDoorSwing2D::Unknown,
		});
	}
	return result;
}

} // namespace iggy
