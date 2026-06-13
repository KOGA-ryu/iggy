#pragma once

#include <cstddef>

#include "scene/draft/DraftCompilePlan2D.hpp"
#include "scene/draft/DraftDoorCompile2D.hpp"
#include "scene/draft/DraftWallCompile2D.hpp"
#include "scene/draft/DraftWallCutCompile2D.hpp"
#include "scene/draft/DraftWallDoorAttach2D.hpp"
#include "scene/draft/DraftWallDoorMorphPlan2D.hpp"

namespace iggy {

struct DraftBuildingCompile2DConfig {
	DraftWallDoorAttach2DConfig attach;
	DraftWallCutCompile2DConfig cut;
};

struct DraftBuildingCompile2DResult {
	DraftCompilePlan2DResult plan;
	DraftWallCompile2DResult walls;
	DraftDoorCompile2DResult doors;
	DraftWallDoorAttach2DResult attachments;
	DraftWallDoorMorphPlan2DResult morphPlan;
	DraftWallCutCompile2DResult wallCuts;
	std::size_t compiledWallSegmentCount = 0;
	std::size_t compiledDoorCount = 0;

	[[nodiscard]] bool hasIssues() const;
};

class DraftBuildingCompiler2D {
public:
	[[nodiscard]] DraftBuildingCompile2DResult compile(
		const DraftDocument2D &document,
		const DraftBuildingCompile2DConfig &config = {}) const;
};

} // namespace iggy
