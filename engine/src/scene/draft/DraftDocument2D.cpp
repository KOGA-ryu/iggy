#include "scene/draft/DraftDocument2D.hpp"

namespace {

bool HasEarlierMatchingId(const std::vector<iggy::DraftSymbol2D> &symbols, std::size_t currentIndex)
{
	for (std::size_t index = 0; index < currentIndex; ++index) {
		if (symbols[index].id == symbols[currentIndex].id)
			return true;
	}
	return false;
}

} // namespace

namespace iggy {

const DraftSymbol2D *DraftDocument2D::find(const ResourceId &id) const
{
	for (const DraftSymbol2D &symbol : symbols) {
		if (symbol.id == id)
			return &symbol;
	}
	return nullptr;
}

bool DraftDocument2D::contains(const ResourceId &id) const
{
	return find(id) != nullptr;
}

DraftDocument2DBuildResult DraftDocument2DBuilder::build(const std::vector<DraftSymbol2D> &symbols) const
{
	DraftDocument2DBuildResult result;
	for (std::size_t index = 0; index < symbols.size(); ++index) {
		const DraftSymbol2D &symbol = symbols[index];
		if (symbol.id.empty()) {
			result.issues.push_back({
				DraftDocument2DIssueCode::EmptySymbolId,
				index,
				symbol,
			});
		}
		if (HasEarlierMatchingId(symbols, index)) {
			result.issues.push_back({
				DraftDocument2DIssueCode::DuplicateSymbolId,
				index,
				symbol,
			});
		}
	}

	if (!result.issues.empty())
		return result;

	result.built = true;
	result.document.symbols = symbols;
	return result;
}

} // namespace iggy
