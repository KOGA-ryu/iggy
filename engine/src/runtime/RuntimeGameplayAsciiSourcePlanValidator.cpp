#include "runtime/RuntimeGameplayAsciiSourcePlanValidator.hpp"

#include <string>
#include <vector>

namespace iggy::runtime {
namespace {

bool IsBuiltInTerrainGlyph(char glyph, char backgroundGlyph)
{
	return glyph == backgroundGlyph || glyph == '.' || glyph == '#' || glyph == ' ';
}

void AddIssue(
	RuntimeGameplayAsciiSourcePlanValidationResult &result,
	RuntimeGameplayAsciiSourcePlanIssue issue)
{
	switch (issue.code) {
	case RuntimeGameplayAsciiSourcePlanIssueCode::GridDimensionMismatch:
		++result.dimensionMismatchCount;
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::RaggedRow:
		++result.raggedRowCount;
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::DuplicateGlyph:
		++result.duplicateGlyphCount;
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::UnknownGridGlyph:
		++result.unknownGridGlyphCount;
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::AnnotatedCellOutOfBounds:
	case RuntimeGameplayAsciiSourcePlanIssueCode::AnnotatedCellGlyphMismatch:
	case RuntimeGameplayAsciiSourcePlanIssueCode::DuplicateAnnotatedCellId:
	case RuntimeGameplayAsciiSourcePlanIssueCode::EmptyActorMarkerId:
	case RuntimeGameplayAsciiSourcePlanIssueCode::EmptyProfileMarkerId:
		++result.annotationIssueCount;
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::InvalidRegionBounds:
	case RuntimeGameplayAsciiSourcePlanIssueCode::RegionOutOfBounds:
		++result.regionIssueCount;
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::UnsafeNoClaims:
	case RuntimeGameplayAsciiSourcePlanIssueCode::UnsafePromotionPolicy:
		++result.unsafeBoundaryIssueCount;
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::EmptyRows:
	case RuntimeGameplayAsciiSourcePlanIssueCode::EmptyGlyph:
		break;
	}

	result.issues.push_back(issue);
	result.issueCount = result.issues.size();
}

void ValidateRows(RuntimeGameplayAsciiSourcePlanValidationResult &result)
{
	const RuntimeGameplayAsciiSourcePlanGrid &grid = result.plan.grid;
	result.rowCount = grid.rows.size();
	result.width = grid.width;
	result.height = grid.height;

	if (grid.rows.empty()) {
		RuntimeGameplayAsciiSourcePlanIssue issue;
		issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::EmptyRows;
		AddIssue(result, issue);
		return;
	}

	if (grid.height != grid.rows.size()) {
		RuntimeGameplayAsciiSourcePlanIssue issue;
		issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::GridDimensionMismatch;
		issue.index = grid.height;
		issue.firstIndex = grid.rows.size();
		AddIssue(result, issue);
	}

	for (std::size_t rowIndex = 0; rowIndex < grid.rows.size(); ++rowIndex) {
		if (grid.rows[rowIndex].size() != grid.width) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::RaggedRow;
			issue.row = rowIndex;
			issue.index = grid.rows[rowIndex].size();
			issue.firstIndex = grid.width;
			AddIssue(result, issue);
		}
	}
}

std::vector<char> ValidateLegend(
	RuntimeGameplayAsciiSourcePlanValidationResult &result)
{
	std::vector<char> glyphs;
	result.legendCount = result.plan.legend.size();

	for (std::size_t index = 0; index < result.plan.legend.size(); ++index) {
		const RuntimeGameplayAsciiSourcePlanGlyphLegendEntry &entry =
			result.plan.legend[index];
		if (entry.glyph == '\0') {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::EmptyGlyph;
			issue.index = index;
			AddIssue(result, issue);
			continue;
		}

		for (std::size_t firstIndex = 0; firstIndex < glyphs.size(); ++firstIndex) {
			if (glyphs[firstIndex] == entry.glyph) {
				RuntimeGameplayAsciiSourcePlanIssue issue;
				issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::DuplicateGlyph;
				issue.index = index;
				issue.firstIndex = firstIndex;
				issue.glyph = entry.glyph;
				AddIssue(result, issue);
				break;
			}
		}

		glyphs.push_back(entry.glyph);
	}

	return glyphs;
}

bool HasLegendGlyph(const std::vector<char> &glyphs, char glyph)
{
	for (char known : glyphs) {
		if (known == glyph) {
			return true;
		}
	}
	return false;
}

RuntimeGameplayAsciiSourcePlanGlyphKind GlyphKindFor(
	const RuntimeGameplayAsciiSourcePlan &plan,
	char glyph)
{
	for (const RuntimeGameplayAsciiSourcePlanGlyphLegendEntry &entry : plan.legend) {
		if (entry.glyph == glyph) {
			return entry.kind;
		}
	}
	return RuntimeGameplayAsciiSourcePlanGlyphKind::Unknown;
}

void ValidateGridGlyphs(
	RuntimeGameplayAsciiSourcePlanValidationResult &result,
	const std::vector<char> &legendGlyphs)
{
	const RuntimeGameplayAsciiSourcePlanGrid &grid = result.plan.grid;
	for (std::size_t rowIndex = 0; rowIndex < grid.rows.size(); ++rowIndex) {
		const std::string &row = grid.rows[rowIndex];
		for (std::size_t column = 0; column < row.size(); ++column) {
			const char glyph = row[column];
			if (!IsBuiltInTerrainGlyph(glyph, grid.backgroundGlyph) &&
				!HasLegendGlyph(legendGlyphs, glyph)) {
				RuntimeGameplayAsciiSourcePlanIssue issue;
				issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::UnknownGridGlyph;
				issue.row = rowIndex;
				issue.column = column;
				issue.glyph = glyph;
				AddIssue(result, issue);
			}
		}
	}
}

bool CellInBounds(
	const RuntimeGameplayAsciiSourcePlanGrid &grid,
	std::size_t row,
	std::size_t column)
{
	return row < grid.rows.size() && column < grid.width;
}

void ValidateAnnotatedCells(RuntimeGameplayAsciiSourcePlanValidationResult &result)
{
	const RuntimeGameplayAsciiSourcePlanGrid &grid = result.plan.grid;
	result.annotatedCellCount = result.plan.annotatedCells.size();
	std::vector<ResourceId> cellIds;
	std::vector<std::size_t> cellIdIndexes;

	for (std::size_t index = 0; index < result.plan.annotatedCells.size(); ++index) {
		const RuntimeGameplayAsciiSourcePlanAnnotatedCell &cell =
			result.plan.annotatedCells[index];

		if (cell.hasCellId && !cell.cellId.empty()) {
			for (std::size_t idIndex = 0; idIndex < cellIds.size(); ++idIndex) {
				if (cellIds[idIndex] == cell.cellId) {
					RuntimeGameplayAsciiSourcePlanIssue issue;
					issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::DuplicateAnnotatedCellId;
					issue.index = index;
					issue.firstIndex = cellIdIndexes[idIndex];
					issue.id = cell.cellId;
					AddIssue(result, issue);
					break;
				}
			}
			cellIds.push_back(cell.cellId);
			cellIdIndexes.push_back(index);
		}

		if (!CellInBounds(grid, cell.row, cell.column)) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::AnnotatedCellOutOfBounds;
			issue.index = index;
			issue.row = cell.row;
			issue.column = cell.column;
			issue.glyph = cell.glyph;
			AddIssue(result, issue);
			continue;
		}

		if (grid.rows[cell.row][cell.column] != cell.glyph) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::AnnotatedCellGlyphMismatch;
			issue.index = index;
			issue.row = cell.row;
			issue.column = cell.column;
			issue.glyph = cell.glyph;
			AddIssue(result, issue);
		}

		const RuntimeGameplayAsciiSourcePlanGlyphKind kind =
			GlyphKindFor(result.plan, cell.glyph);
		if (kind == RuntimeGameplayAsciiSourcePlanGlyphKind::Actor) {
			if (cell.markerId.empty()) {
				RuntimeGameplayAsciiSourcePlanIssue issue;
				issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::EmptyActorMarkerId;
				issue.index = index;
				issue.row = cell.row;
				issue.column = cell.column;
				issue.glyph = cell.glyph;
				AddIssue(result, issue);
			}
			if (cell.profileId.empty()) {
				RuntimeGameplayAsciiSourcePlanIssue issue;
				issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::EmptyProfileMarkerId;
				issue.index = index;
				issue.row = cell.row;
				issue.column = cell.column;
				issue.glyph = cell.glyph;
				AddIssue(result, issue);
			}
		}
	}
}

void ValidateRegions(RuntimeGameplayAsciiSourcePlanValidationResult &result)
{
	const RuntimeGameplayAsciiSourcePlanGrid &grid = result.plan.grid;
	result.regionCount = result.plan.regions.size();

	for (std::size_t index = 0; index < result.plan.regions.size(); ++index) {
		const RuntimeGameplayAsciiSourcePlanRegion &region = result.plan.regions[index];
		if (region.minRow > region.maxRow || region.minColumn > region.maxColumn) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::InvalidRegionBounds;
			issue.index = index;
			issue.id = region.regionId;
			AddIssue(result, issue);
		}

		if (region.maxRow >= grid.rows.size() || region.maxColumn >= grid.width) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::RegionOutOfBounds;
			issue.index = index;
			issue.row = region.maxRow;
			issue.column = region.maxColumn;
			issue.id = region.regionId;
			AddIssue(result, issue);
		}
	}
}

void ValidateBoundaryFlags(RuntimeGameplayAsciiSourcePlanValidationResult &result)
{
	const RuntimeGameplayAsciiSourcePlanNoClaims &noClaims = result.plan.noClaims;
	if (noClaims.claimsRuntimeTruth || noClaims.claimsGameplayExecution ||
		noClaims.claimsFileParsing || noClaims.claimsProfileScenarioConversion) {
		RuntimeGameplayAsciiSourcePlanIssue issue;
		issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::UnsafeNoClaims;
		AddIssue(result, issue);
	}

	const RuntimeGameplayAsciiSourcePlanPromotionPolicy &promotion =
		result.plan.promotionPolicy;
	if (promotion.promotionReady || promotion.allowsRuntimeExecution ||
		promotion.allowsFileParsing || promotion.allowsProfileScenarioConversion) {
		RuntimeGameplayAsciiSourcePlanIssue issue;
		issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::UnsafePromotionPolicy;
		AddIssue(result, issue);
	}
}

} // namespace

bool RuntimeGameplayAsciiSourcePlanValidationResult::ok() const
{
	return status == RuntimeGameplayAsciiSourcePlanValidationStatus::Valid;
}

RuntimeGameplayAsciiSourcePlanValidationResult
RuntimeGameplayAsciiSourcePlanValidator::validate(
	const RuntimeGameplayAsciiSourcePlan &plan) const
{
	RuntimeGameplayAsciiSourcePlanValidationResult result;
	result.plan = plan;

	ValidateRows(result);
	const std::vector<char> legendGlyphs = ValidateLegend(result);
	ValidateAnnotatedCells(result);
	ValidateRegions(result);
	ValidateBoundaryFlags(result);
	ValidateGridGlyphs(result, legendGlyphs);

	result.status = result.issues.empty()
		? RuntimeGameplayAsciiSourcePlanValidationStatus::Valid
		: RuntimeGameplayAsciiSourcePlanValidationStatus::Invalid;
	result.issueCount = result.issues.size();
	return result;
}

} // namespace iggy::runtime
