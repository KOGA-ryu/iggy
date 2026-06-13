#include "scene/draft/DraftBuildingCompile2D.hpp"

namespace iggy {

bool DraftBuildingCompile2DResult::hasIssues() const
{
	return plan.hasIssues()
		|| walls.hasIssues()
		|| doors.hasIssues()
		|| attachments.hasIssues()
		|| morphPlan.hasIssues()
		|| wallCuts.hasIssues();
}

DraftBuildingCompile2DResult DraftBuildingCompiler2D::compile(
	const DraftDocument2D &document,
	const DraftBuildingCompile2DConfig &config) const
{
	DraftBuildingCompile2DResult result;
	result.plan = DraftCompilePlanner2D {}.plan(document);
	result.walls = DraftWallCompiler2D {}.compile(result.plan);
	result.doors = DraftDoorCompiler2D {}.compile(result.plan);
	result.attachments = DraftWallDoorAttach2D {}.attach(result.walls.walls, result.doors.doors, config.attach);
	result.morphPlan = DraftWallDoorMorphPlanner2D {}.plan(result.walls.walls, result.attachments);
	result.wallCuts = DraftWallCutCompiler2D {}.compile(result.morphPlan, config.cut);
	result.compiledWallSegmentCount = result.wallCuts.segments.size();
	result.compiledDoorCount = result.doors.doors.size();
	return result;
}

} // namespace iggy
