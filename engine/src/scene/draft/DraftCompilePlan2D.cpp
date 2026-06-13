#include "scene/draft/DraftCompilePlan2D.hpp"

namespace iggy {

bool DraftCompilePlan2DResult::hasIssues() const
{
	return !issues.empty();
}

DraftCompilePlan2DResult DraftCompilePlanner2D::plan(const DraftDocument2D &document) const
{
	DraftCompilePlan2DResult result;
	for (std::size_t index = 0; index < document.symbols.size(); ++index) {
		const DraftSymbol2D &symbol = document.symbols[index];
		if (!symbol.enabled) {
			result.ignoredDisabledSymbols.push_back({ index, symbol });
			continue;
		}

		if (symbol.kind == DraftSymbol2DKind::Wall) {
			result.walls.push_back({ index, symbol });
		} else if (symbol.kind == DraftSymbol2DKind::Door) {
			result.doors.push_back({ index, symbol });
		} else if (symbol.kind == DraftSymbol2DKind::Object || symbol.kind == DraftSymbol2DKind::Furniture) {
			result.objects.push_back({ index, symbol });
		} else if (symbol.kind == DraftSymbol2DKind::ItemDrop) {
			result.itemDrops.push_back({ index, symbol });
		} else if (symbol.kind == DraftSymbol2DKind::Npc) {
			result.npcs.push_back({ index, symbol });
		} else if (symbol.kind == DraftSymbol2DKind::Region || symbol.kind == DraftSymbol2DKind::StoryMarker) {
			result.markers.push_back({ index, symbol });
		} else if (symbol.kind == DraftSymbol2DKind::Unknown) {
			result.issues.push_back({
				DraftCompilePlan2DIssueCode::UnknownSymbolKind,
				index,
				symbol,
			});
		}
	}

	return result;
}

} // namespace iggy
