#include "scene/draft/DraftWallCompile2D.hpp"

namespace iggy {
namespace {

[[nodiscard]] bool HasPositiveSize(Vec2 size)
{
	return size.x > 0.0F && size.y > 0.0F;
}

} // namespace

bool DraftWallCompile2DResult::hasIssues() const
{
	return !issues.empty();
}

DraftWallCompile2DResult DraftWallCompiler2D::compile(const DraftCompilePlan2DResult &plan) const
{
	DraftWallCompile2DResult result;
	for (const DraftWallPlan2D &wall : plan.walls) {
		if (!HasPositiveSize(wall.symbol.size)) {
			result.issues.push_back({
				DraftWallCompile2DIssueCode::NonPositiveSize,
				wall.symbolIndex,
				wall.symbol,
			});
			continue;
		}

		result.walls.push_back({
			wall.symbol.id,
			wall.symbolIndex,
			wall.symbol.position,
			wall.symbol.size,
			wall.symbol.rotationRadians,
			wall.symbol.assetId,
			wall.symbol.definitionId,
		});
	}
	return result;
}

} // namespace iggy
