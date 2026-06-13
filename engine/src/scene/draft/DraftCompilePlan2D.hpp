#pragma once

#include <cstddef>
#include <vector>

#include "scene/draft/DraftDocument2D.hpp"

namespace iggy {

struct DraftWallPlan2D {
	std::size_t symbolIndex = 0;
	DraftSymbol2D symbol;
};

struct DraftDoorPlan2D {
	std::size_t symbolIndex = 0;
	DraftSymbol2D symbol;
};

struct DraftObjectPlan2D {
	std::size_t symbolIndex = 0;
	DraftSymbol2D symbol;
};

struct DraftItemDropPlan2D {
	std::size_t symbolIndex = 0;
	DraftSymbol2D symbol;
};

struct DraftNpcPlan2D {
	std::size_t symbolIndex = 0;
	DraftSymbol2D symbol;
};

struct DraftMarkerPlan2D {
	std::size_t symbolIndex = 0;
	DraftSymbol2D symbol;
};

struct DraftIgnoredSymbol2D {
	std::size_t symbolIndex = 0;
	DraftSymbol2D symbol;
};

enum class DraftCompilePlan2DIssueCode {
	UnknownSymbolKind,
};

struct DraftCompilePlan2DIssue {
	DraftCompilePlan2DIssueCode code = DraftCompilePlan2DIssueCode::UnknownSymbolKind;
	std::size_t symbolIndex = 0;
	DraftSymbol2D symbol;
};

struct DraftCompilePlan2DResult {
	std::vector<DraftWallPlan2D> walls;
	std::vector<DraftDoorPlan2D> doors;
	std::vector<DraftObjectPlan2D> objects;
	std::vector<DraftItemDropPlan2D> itemDrops;
	std::vector<DraftNpcPlan2D> npcs;
	std::vector<DraftMarkerPlan2D> markers;
	std::vector<DraftIgnoredSymbol2D> ignoredDisabledSymbols;
	std::vector<DraftCompilePlan2DIssue> issues;

	[[nodiscard]] bool hasIssues() const;
};

class DraftCompilePlanner2D {
public:
	[[nodiscard]] DraftCompilePlan2DResult plan(const DraftDocument2D &document) const;
};

} // namespace iggy
