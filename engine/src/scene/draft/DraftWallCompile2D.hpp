#pragma once

#include <cstddef>
#include <vector>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "scene/draft/DraftCompilePlan2D.hpp"

namespace iggy {

struct DraftCompiledWall2D {
	ResourceId sourceSymbolId;
	std::size_t sourceSymbolIndex = 0;
	Vec2 center;
	Vec2 size;
	float rotationRadians = 0.0F;
	ResourceId assetId;
	ResourceId definitionId;
};

enum class DraftWallCompile2DIssueCode {
	NonPositiveSize,
};

struct DraftWallCompile2DIssue {
	DraftWallCompile2DIssueCode code = DraftWallCompile2DIssueCode::NonPositiveSize;
	std::size_t symbolIndex = 0;
	DraftSymbol2D symbol;
};

struct DraftWallCompile2DResult {
	std::vector<DraftCompiledWall2D> walls;
	std::vector<DraftWallCompile2DIssue> issues;

	[[nodiscard]] bool hasIssues() const;
};

class DraftWallCompiler2D {
public:
	[[nodiscard]] DraftWallCompile2DResult compile(const DraftCompilePlan2DResult &plan) const;
};

} // namespace iggy
