#pragma once

#include <cstddef>
#include <vector>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "scene/draft/DraftCompilePlan2D.hpp"

namespace iggy {

enum class DraftDoorSwing2D {
	Unknown,
	Inward,
	Outward,
};

struct DraftCompiledDoor2D {
	ResourceId sourceSymbolId;
	std::size_t sourceSymbolIndex = 0;
	Vec2 center;
	Vec2 size;
	float rotationRadians = 0.0F;
	ResourceId assetId;
	ResourceId definitionId;
	DraftDoorSwing2D swing = DraftDoorSwing2D::Unknown;
};

enum class DraftDoorCompile2DIssueCode {
	NonPositiveSize,
};

struct DraftDoorCompile2DIssue {
	DraftDoorCompile2DIssueCode code = DraftDoorCompile2DIssueCode::NonPositiveSize;
	std::size_t symbolIndex = 0;
	DraftSymbol2D symbol;
};

struct DraftDoorCompile2DResult {
	std::vector<DraftCompiledDoor2D> doors;
	std::vector<DraftDoorCompile2DIssue> issues;

	[[nodiscard]] bool hasIssues() const;
};

class DraftDoorCompiler2D {
public:
	[[nodiscard]] DraftDoorCompile2DResult compile(const DraftCompilePlan2DResult &plan) const;
};

} // namespace iggy
