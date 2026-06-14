#pragma once

#include <cstddef>
#include <vector>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"

namespace iggy {

enum class DraftSymbol2DKind {
	Unknown,
	Wall,
	Door,
	Object,
	Furniture,
	ItemDrop,
	Npc,
	Region,
	StoryMarker,
	CollisionBlocker,
};

struct DraftSymbol2D {
	ResourceId id;
	DraftSymbol2DKind kind = DraftSymbol2DKind::Unknown;
	Vec2 position;
	Vec2 size;
	float rotationRadians = 0.0F;
	ResourceId assetId;
	ResourceId definitionId;
	bool enabled = true;
};

struct DraftDocument2D {
	std::vector<DraftSymbol2D> symbols;

	[[nodiscard]] const DraftSymbol2D *find(const ResourceId &id) const;
	[[nodiscard]] bool contains(const ResourceId &id) const;
};

enum class DraftDocument2DIssueCode {
	EmptySymbolId,
	DuplicateSymbolId,
};

struct DraftDocument2DIssue {
	DraftDocument2DIssueCode code = DraftDocument2DIssueCode::EmptySymbolId;
	std::size_t symbolIndex = 0;
	DraftSymbol2D symbol;
};

struct DraftDocument2DBuildResult {
	bool built = false;
	DraftDocument2D document;
	std::vector<DraftDocument2DIssue> issues;
};

class DraftDocument2DBuilder {
public:
	[[nodiscard]] DraftDocument2DBuildResult build(const std::vector<DraftSymbol2D> &symbols) const;
};

} // namespace iggy
