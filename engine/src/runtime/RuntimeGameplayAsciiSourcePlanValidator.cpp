#include "runtime/RuntimeGameplayAsciiSourcePlanValidator.hpp"

#include <string>
#include <vector>

namespace iggy::runtime {
namespace {

constexpr std::size_t SupportedSourcePlanVersion = 1;
const ResourceId SupportedSourcePlanFormatId("iggy:ascii-source-plan");

bool IsBuiltInTerrainGlyph(char glyph, char backgroundGlyph)
{
	return glyph == backgroundGlyph || glyph == '.' || glyph == '#' || glyph == ' ';
}

bool HasEarlierMatchingExpectedInventoryStack(
	const std::vector<RuntimeGameplayAsciiSourcePlanExpectedInventoryStack> &stacks,
	std::size_t currentIndex)
{
	for (std::size_t index = 0; index < currentIndex; ++index) {
		if (stacks[index].itemId == stacks[currentIndex].itemId) {
			return true;
		}
	}
	return false;
}

bool HasEarlierMatchingExpectedInteractionTarget(
	const std::vector<RuntimeGameplayAsciiSourcePlanExpectedInteractionTarget>
		&targets,
	std::size_t currentIndex)
{
	for (std::size_t index = 0; index < currentIndex; ++index) {
		if (targets[index].targetId == targets[currentIndex].targetId) {
			return true;
		}
	}
	return false;
}

bool HasEarlierMatchingExpectedActorState(
	const std::vector<RuntimeGameplayAsciiSourcePlanExpectedActorState> &actors,
	std::size_t currentIndex)
{
	for (std::size_t index = 0; index < currentIndex; ++index) {
		if (actors[index].actorId == actors[currentIndex].actorId) {
			return true;
		}
	}
	return false;
}

void AddIssue(
	RuntimeGameplayAsciiSourcePlanValidationResult &result,
	RuntimeGameplayAsciiSourcePlanIssue issue)
{
	switch (issue.code) {
	case RuntimeGameplayAsciiSourcePlanIssueCode::UnsupportedFormatId:
	case RuntimeGameplayAsciiSourcePlanIssueCode::UnsupportedVersion:
		++result.compatibilityIssueCount;
		break;
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
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredControlMissingNpcId:
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredControlUnsupportedBehavior:
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredControlUnsupportedMoveMode:
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredControlMissingTarget:
		++result.authoredControlIssueCount;
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredProfileMissingId:
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredProfileDuplicateId:
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredProfileInvalidTraits:
		++result.authoredProfileIssueCount;
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredInteractionTargetMissingId:
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredInteractionTargetDuplicateId:
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredInteractionTargetUnsupportedKind:
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredInteractionTargetMissingPosition:
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredInteractionTargetPositionOutOfBounds:
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredInteractionTargetInvalidRadius:
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredInteractionTargetUnsupportedEffect:
		++result.authoredInteractionTargetIssueCount;
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredItemDropMissingDropId:
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredItemDropDuplicateDropId:
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredItemDropMissingItemId:
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredItemDropInvalidCount:
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredItemDropMissingPosition:
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredItemDropPositionOutOfBounds:
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredItemDropInvalidPickupRadius:
		++result.authoredItemDropIssueCount;
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredPlayerCommandUnsupportedCommand:
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredPlayerCommandMissingTarget:
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredPlayerCommandUnknownInteractionTarget:
	case RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredPlayerCommandInvalidPickupTarget:
		++result.authoredPlayerCommandIssueCount;
		break;
	case RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedFinalRowsEmpty:
	case RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedFinalRowsDimensionMismatch:
	case RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedTraceFrameRowsEmpty:
	case RuntimeGameplayAsciiSourcePlanIssueCode::
		ExpectedTraceFrameRowsDimensionMismatch:
	case RuntimeGameplayAsciiSourcePlanIssueCode::
		ExpectedInventoryStackMissingItemId:
	case RuntimeGameplayAsciiSourcePlanIssueCode::
		ExpectedInventoryStackInvalidCount:
	case RuntimeGameplayAsciiSourcePlanIssueCode::
		ExpectedInventoryStackDuplicateItemId:
	case RuntimeGameplayAsciiSourcePlanIssueCode::
		ExpectedInteractionTargetMissingId:
	case RuntimeGameplayAsciiSourcePlanIssueCode::
		ExpectedInteractionTargetDuplicateId:
	case RuntimeGameplayAsciiSourcePlanIssueCode::
		ExpectedInteractionTargetUnknownId:
	case RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedActorStateMissingId:
	case RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedActorStateDuplicateId:
	case RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedActorStateUnknownId:
	case RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedActorStateMissingTile:
	case RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedPlayerStateMissingTile:
	case RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedPlayerStateMissingPlayer:
		++result.expectationIssueCount;
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

void ValidateCompatibility(RuntimeGameplayAsciiSourcePlanValidationResult &result)
{
	if (!result.plan.formatId.empty() &&
		result.plan.formatId != SupportedSourcePlanFormatId) {
		RuntimeGameplayAsciiSourcePlanIssue issue;
		issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::UnsupportedFormatId;
		issue.id = result.plan.formatId;
		AddIssue(result, issue);
	}

	if (result.plan.version != SupportedSourcePlanVersion) {
		RuntimeGameplayAsciiSourcePlanIssue issue;
		issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::UnsupportedVersion;
		issue.index = result.plan.version;
		issue.firstIndex = SupportedSourcePlanVersion;
		AddIssue(result, issue);
	}
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

bool BehaviorSupported(RuntimeGameplayAsciiSourcePlanControlBehavior behavior)
{
	return behavior == RuntimeGameplayAsciiSourcePlanControlBehavior::Waiting ||
		behavior == RuntimeGameplayAsciiSourcePlanControlBehavior::Seeking;
}

bool BehaviorNeedsTarget(RuntimeGameplayAsciiSourcePlanControlBehavior behavior)
{
	return behavior == RuntimeGameplayAsciiSourcePlanControlBehavior::Seeking;
}

bool MoveModeSupported(RuntimeGameplayAsciiSourcePlanControlMoveMode moveMode)
{
	return moveMode == RuntimeGameplayAsciiSourcePlanControlMoveMode::Still ||
		moveMode == RuntimeGameplayAsciiSourcePlanControlMoveMode::Walk ||
		moveMode == RuntimeGameplayAsciiSourcePlanControlMoveMode::Jog ||
		moveMode == RuntimeGameplayAsciiSourcePlanControlMoveMode::Run ||
		moveMode == RuntimeGameplayAsciiSourcePlanControlMoveMode::Sprint;
}

void ValidateAuthoredControls(RuntimeGameplayAsciiSourcePlanValidationResult &result)
{
	result.authoredControlCount = result.plan.authoredControls.size();

	for (std::size_t index = 0; index < result.plan.authoredControls.size(); ++index) {
		const RuntimeGameplayAsciiSourcePlanAuthoredControl &control =
			result.plan.authoredControls[index];
		if (control.npcId.empty()) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredControlMissingNpcId;
			issue.index = index;
			AddIssue(result, issue);
		}

		if (!BehaviorSupported(control.behavior)) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code =
				RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredControlUnsupportedBehavior;
			issue.index = index;
			issue.id = control.npcId;
			AddIssue(result, issue);
		}

		if (!MoveModeSupported(control.moveMode)) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code =
				RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredControlUnsupportedMoveMode;
			issue.index = index;
			issue.id = control.npcId;
			AddIssue(result, issue);
		}

		if (BehaviorNeedsTarget(control.behavior) && !control.targetPosition.present) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredControlMissingTarget;
			issue.index = index;
			issue.id = control.npcId;
			AddIssue(result, issue);
		}
	}
}

void ValidateAuthoredProfiles(RuntimeGameplayAsciiSourcePlanValidationResult &result)
{
	result.authoredProfileCount = result.plan.authoredProfiles.size();
	std::vector<ResourceId> profileIds;
	std::vector<std::size_t> profileIdIndexes;

	for (std::size_t index = 0; index < result.plan.authoredProfiles.size();
		++index) {
		const RuntimeGameplayAsciiSourcePlanAuthoredProfile &profile =
			result.plan.authoredProfiles[index];

		if (profile.profileId.empty()) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code =
				RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredProfileMissingId;
			issue.index = index;
			AddIssue(result, issue);
		} else {
			for (std::size_t idIndex = 0; idIndex < profileIds.size(); ++idIndex) {
				if (profileIds[idIndex] == profile.profileId) {
					RuntimeGameplayAsciiSourcePlanIssue issue;
					issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
						AuthoredProfileDuplicateId;
					issue.index = index;
					issue.firstIndex = profileIdIndexes[idIndex];
					issue.id = profile.profileId;
					AddIssue(result, issue);
					break;
				}
			}
			profileIds.push_back(profile.profileId);
			profileIdIndexes.push_back(index);
		}

		const NpcTraitSetValidationResult traitValidation =
			iggy::validate(profile.traits);
		if (!traitValidation.ok()) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code =
				RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredProfileInvalidTraits;
			issue.index = index;
			issue.id = profile.profileId;
			AddIssue(result, issue);
		}
	}
}

bool InteractionTargetKindSupported(
	RuntimeGameplayAsciiSourcePlanInteractionTargetKind kind)
{
	return kind == RuntimeGameplayAsciiSourcePlanInteractionTargetKind::Inspectable ||
		kind == RuntimeGameplayAsciiSourcePlanInteractionTargetKind::Usable ||
		kind == RuntimeGameplayAsciiSourcePlanInteractionTargetKind::Pickup ||
		kind == RuntimeGameplayAsciiSourcePlanInteractionTargetKind::Talk ||
		kind == RuntimeGameplayAsciiSourcePlanInteractionTargetKind::Door;
}

bool InteractionEffectKindSupported(
	RuntimeGameplayAsciiSourcePlanInteractionEffectKind effect)
{
	return effect == RuntimeGameplayAsciiSourcePlanInteractionEffectKind::None ||
		effect == RuntimeGameplayAsciiSourcePlanInteractionEffectKind::InspectText ||
		effect == RuntimeGameplayAsciiSourcePlanInteractionEffectKind::ToggleTarget ||
		effect == RuntimeGameplayAsciiSourcePlanInteractionEffectKind::EmitEvent ||
		effect == RuntimeGameplayAsciiSourcePlanInteractionEffectKind::PickupItem;
}

bool TileInBounds(
	const RuntimeGameplayAsciiSourcePlanGrid &grid,
	const RuntimeGameplayAsciiSourcePlanLocalTile &tile)
{
	return tile.x >= 0 && tile.y >= 0 &&
		static_cast<std::size_t>(tile.x) < grid.width &&
		static_cast<std::size_t>(tile.y) < grid.rows.size();
}

bool PositionInBounds(
	const RuntimeGameplayAsciiSourcePlanGrid &grid,
	const RuntimeGameplayAsciiSourcePlanLocalPosition &position)
{
	return position.x >= 0.0 && position.y >= 0.0 &&
		position.x < static_cast<double>(grid.width) &&
		position.y < static_cast<double>(grid.rows.size());
}

void ValidateAuthoredInteractionTargets(
	RuntimeGameplayAsciiSourcePlanValidationResult &result)
{
	const RuntimeGameplayAsciiSourcePlanGrid &grid = result.plan.grid;
	result.authoredInteractionTargetCount =
		result.plan.authoredInteractionTargets.size();
	std::vector<ResourceId> targetIds;
	std::vector<std::size_t> targetIdIndexes;

	for (std::size_t index = 0;
		index < result.plan.authoredInteractionTargets.size();
		++index) {
		const RuntimeGameplayAsciiSourcePlanAuthoredInteractionTarget &target =
			result.plan.authoredInteractionTargets[index];

		if (target.targetId.empty()) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
				AuthoredInteractionTargetMissingId;
			issue.index = index;
			AddIssue(result, issue);
		} else {
			for (std::size_t idIndex = 0; idIndex < targetIds.size(); ++idIndex) {
				if (targetIds[idIndex] == target.targetId) {
					RuntimeGameplayAsciiSourcePlanIssue issue;
					issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
						AuthoredInteractionTargetDuplicateId;
					issue.index = index;
					issue.firstIndex = targetIdIndexes[idIndex];
					issue.id = target.targetId;
					AddIssue(result, issue);
					break;
				}
			}
			targetIds.push_back(target.targetId);
			targetIdIndexes.push_back(index);
		}

		if (!InteractionTargetKindSupported(target.kind)) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
				AuthoredInteractionTargetUnsupportedKind;
			issue.index = index;
			issue.id = target.targetId;
			AddIssue(result, issue);
		}

		if (!target.localTile.present && !target.localPosition.present) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
				AuthoredInteractionTargetMissingPosition;
			issue.index = index;
			issue.id = target.targetId;
			AddIssue(result, issue);
		} else if (
			(target.localTile.present && !TileInBounds(grid, target.localTile)) ||
			(target.localPosition.present &&
				!PositionInBounds(grid, target.localPosition))) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
				AuthoredInteractionTargetPositionOutOfBounds;
			issue.index = index;
			issue.id = target.targetId;
			if (target.localTile.present) {
				issue.row = static_cast<std::size_t>(target.localTile.y);
				issue.column = static_cast<std::size_t>(target.localTile.x);
			}
			AddIssue(result, issue);
		}

		if (target.radius < 0.0) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
				AuthoredInteractionTargetInvalidRadius;
			issue.index = index;
			issue.id = target.targetId;
			AddIssue(result, issue);
		}

		if (!InteractionEffectKindSupported(target.effect)) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
				AuthoredInteractionTargetUnsupportedEffect;
			issue.index = index;
			issue.id = target.targetId;
			AddIssue(result, issue);
		}
	}
}

void ValidateAuthoredItemDrops(
	RuntimeGameplayAsciiSourcePlanValidationResult &result)
{
	const RuntimeGameplayAsciiSourcePlanGrid &grid = result.plan.grid;
	result.authoredItemDropCount = result.plan.authoredItemDrops.size();
	std::vector<ResourceId> dropIds;
	std::vector<std::size_t> dropIdIndexes;

	for (std::size_t index = 0; index < result.plan.authoredItemDrops.size();
		++index) {
		const RuntimeGameplayAsciiSourcePlanAuthoredItemDrop &drop =
			result.plan.authoredItemDrops[index];

		if (drop.dropId.empty()) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
				AuthoredItemDropMissingDropId;
			issue.index = index;
			AddIssue(result, issue);
		} else {
			for (std::size_t idIndex = 0; idIndex < dropIds.size(); ++idIndex) {
				if (dropIds[idIndex] == drop.dropId) {
					RuntimeGameplayAsciiSourcePlanIssue issue;
					issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
						AuthoredItemDropDuplicateDropId;
					issue.index = index;
					issue.firstIndex = dropIdIndexes[idIndex];
					issue.id = drop.dropId;
					AddIssue(result, issue);
					break;
				}
			}
			dropIds.push_back(drop.dropId);
			dropIdIndexes.push_back(index);
		}

		if (drop.itemId.empty()) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
				AuthoredItemDropMissingItemId;
			issue.index = index;
			issue.id = drop.dropId;
			AddIssue(result, issue);
		}

		if (drop.count == 0) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
				AuthoredItemDropInvalidCount;
			issue.index = index;
			issue.id = drop.dropId;
			AddIssue(result, issue);
		}

		if (!drop.localTile.present && !drop.localPosition.present) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
				AuthoredItemDropMissingPosition;
			issue.index = index;
			issue.id = drop.dropId;
			AddIssue(result, issue);
		} else if (
			(drop.localTile.present && !TileInBounds(grid, drop.localTile)) ||
			(drop.localPosition.present &&
				!PositionInBounds(grid, drop.localPosition))) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
				AuthoredItemDropPositionOutOfBounds;
			issue.index = index;
			issue.id = drop.dropId;
			if (drop.localTile.present) {
				issue.row = static_cast<std::size_t>(drop.localTile.y);
				issue.column = static_cast<std::size_t>(drop.localTile.x);
			}
			AddIssue(result, issue);
		}

		if (drop.pickupRadius < 0.0) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
				AuthoredItemDropInvalidPickupRadius;
			issue.index = index;
			issue.id = drop.dropId;
			AddIssue(result, issue);
		}
	}
}

bool PlayerCommandSupported(
	RuntimeGameplayAsciiSourcePlanPlayerCommandKind command)
{
	return command ==
			RuntimeGameplayAsciiSourcePlanPlayerCommandKind::MoveToTile ||
		command == RuntimeGameplayAsciiSourcePlanPlayerCommandKind::Interact ||
		command == RuntimeGameplayAsciiSourcePlanPlayerCommandKind::Pickup;
}

bool PlayerCommandNeedsTargetTile(
	RuntimeGameplayAsciiSourcePlanPlayerCommandKind command)
{
	return command ==
		RuntimeGameplayAsciiSourcePlanPlayerCommandKind::MoveToTile;
}

bool PlayerCommandNeedsTargetId(
	RuntimeGameplayAsciiSourcePlanPlayerCommandKind command)
{
	return command == RuntimeGameplayAsciiSourcePlanPlayerCommandKind::Interact ||
		command == RuntimeGameplayAsciiSourcePlanPlayerCommandKind::Pickup;
}

bool HasAuthoredInteractionTarget(
	const RuntimeGameplayAsciiSourcePlan &plan,
	const ResourceId &targetId)
{
	for (const RuntimeGameplayAsciiSourcePlanAuthoredInteractionTarget &target :
		plan.authoredInteractionTargets) {
		if (target.targetId == targetId) {
			return true;
		}
	}
	return false;
}

bool HasAuthoredActor(
	const RuntimeGameplayAsciiSourcePlan &plan,
	const ResourceId &actorId)
{
	for (const RuntimeGameplayAsciiSourcePlanAnnotatedCell &cell :
		plan.annotatedCells) {
		if (cell.markerId == actorId) {
			for (const RuntimeGameplayAsciiSourcePlanGlyphLegendEntry &entry :
				plan.legend) {
				if (entry.glyph == cell.glyph &&
					(entry.kind ==
							RuntimeGameplayAsciiSourcePlanGlyphKind::Actor ||
						entry.scenarioMarkerKind ==
							RuntimeGameplayAsciiScenarioMarkerKind::Actor)) {
					return true;
				}
			}
		}
	}
	return false;
}

bool IsPlayerStartLegend(const RuntimeGameplayAsciiSourcePlanGlyphLegendEntry &entry)
{
	return entry.kind == RuntimeGameplayAsciiSourcePlanGlyphKind::PlayerStart ||
		entry.scenarioMarkerKind ==
			RuntimeGameplayAsciiScenarioMarkerKind::PlayerStart;
}

bool IsPlayerStartGlyph(const RuntimeGameplayAsciiSourcePlan &plan, char glyph)
{
	for (const RuntimeGameplayAsciiSourcePlanGlyphLegendEntry &entry :
		plan.legend) {
		if (entry.glyph == glyph && IsPlayerStartLegend(entry)) {
			return true;
		}
	}
	return false;
}

bool HasPlayerStart(const RuntimeGameplayAsciiSourcePlan &plan)
{
	for (const RuntimeGameplayAsciiSourcePlanAnnotatedCell &cell :
		plan.annotatedCells) {
		if (IsPlayerStartGlyph(plan, cell.glyph)) {
			return true;
		}
	}
	for (const std::string &row : plan.grid.rows) {
		for (char glyph : row) {
			if (IsPlayerStartGlyph(plan, glyph)) {
				return true;
			}
		}
	}
	return false;
}

bool HasAuthoredPickupTarget(
	const RuntimeGameplayAsciiSourcePlan &plan,
	const ResourceId &targetId)
{
	for (const RuntimeGameplayAsciiSourcePlanAuthoredInteractionTarget &target :
		plan.authoredInteractionTargets) {
		if (target.targetId == targetId &&
			target.kind == RuntimeGameplayAsciiSourcePlanInteractionTargetKind::Pickup) {
			return true;
		}
	}
	return false;
}

bool HasAuthoredItemDrop(
	const RuntimeGameplayAsciiSourcePlan &plan,
	const ResourceId &dropId)
{
	for (const RuntimeGameplayAsciiSourcePlanAuthoredItemDrop &drop :
		plan.authoredItemDrops) {
		if (drop.dropId == dropId) {
			return true;
		}
	}
	return false;
}

void ValidateAuthoredPlayerCommands(
	RuntimeGameplayAsciiSourcePlanValidationResult &result)
{
	result.authoredPlayerCommandCount =
		result.plan.authoredPlayerCommands.size();

	for (std::size_t index = 0;
		index < result.plan.authoredPlayerCommands.size();
		++index) {
		const RuntimeGameplayAsciiSourcePlanAuthoredPlayerCommand &command =
			result.plan.authoredPlayerCommands[index];

		if (!PlayerCommandSupported(command.command)) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
				AuthoredPlayerCommandUnsupportedCommand;
			issue.index = index;
			AddIssue(result, issue);
		}

		if (PlayerCommandNeedsTargetTile(command.command) &&
			!(command.hasTargetTile ||
				(command.hasTargetTileX && command.hasTargetTileY))) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
				AuthoredPlayerCommandMissingTarget;
			issue.index = index;
			AddIssue(result, issue);
		}

		if (PlayerCommandNeedsTargetId(command.command) &&
			command.targetId.empty()) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
				AuthoredPlayerCommandMissingTarget;
			issue.index = index;
			AddIssue(result, issue);
		}

		if (command.command ==
				RuntimeGameplayAsciiSourcePlanPlayerCommandKind::Interact &&
			!command.targetId.empty() &&
			!result.plan.authoredInteractionTargets.empty() &&
			!HasAuthoredInteractionTarget(result.plan, command.targetId)) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
				AuthoredPlayerCommandUnknownInteractionTarget;
			issue.index = index;
			issue.id = command.targetId;
			AddIssue(result, issue);
		}

		if (command.command ==
				RuntimeGameplayAsciiSourcePlanPlayerCommandKind::Pickup &&
			!command.targetId.empty() &&
			(!result.plan.authoredInteractionTargets.empty() ||
				!result.plan.authoredItemDrops.empty()) &&
			!HasAuthoredPickupTarget(result.plan, command.targetId) &&
			!HasAuthoredItemDrop(result.plan, command.targetId)) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
				AuthoredPlayerCommandInvalidPickupTarget;
			issue.index = index;
			issue.id = command.targetId;
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
		if (noClaims.claimsRuntimeTruth)
			issue.index = 0;
		else if (noClaims.claimsGameplayExecution)
			issue.index = 1;
		else if (noClaims.claimsFileParsing)
			issue.index = 2;
		else
			issue.index = 3;
		AddIssue(result, issue);
	}

	const RuntimeGameplayAsciiSourcePlanPromotionPolicy &promotion =
		result.plan.promotionPolicy;
	if (promotion.promotionReady || promotion.allowsRuntimeExecution ||
		promotion.allowsFileParsing || promotion.allowsProfileScenarioConversion) {
		RuntimeGameplayAsciiSourcePlanIssue issue;
		issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::UnsafePromotionPolicy;
		if (promotion.promotionReady)
			issue.index = 0;
		else if (promotion.allowsRuntimeExecution)
			issue.index = 1;
		else if (promotion.allowsFileParsing)
			issue.index = 2;
		else
			issue.index = 3;
		AddIssue(result, issue);
	}
}

void ValidateExpectations(RuntimeGameplayAsciiSourcePlanValidationResult &result)
{
	const RuntimeGameplayAsciiSourcePlanExpectations &expectations =
		result.plan.expectations;

	if (expectations.hasFinalRows) {
		if (expectations.finalRows.empty()) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code =
				RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedFinalRowsEmpty;
			AddIssue(result, issue);
			return;
		}

		if (expectations.finalRows.size() != result.plan.grid.rows.size()) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
				ExpectedFinalRowsDimensionMismatch;
			issue.index = expectations.finalRows.size();
			issue.firstIndex = result.plan.grid.rows.size();
			AddIssue(result, issue);
		}

		for (std::size_t row = 0; row < expectations.finalRows.size(); ++row) {
			if (expectations.finalRows[row].size() != result.plan.grid.width) {
				RuntimeGameplayAsciiSourcePlanIssue issue;
				issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
					ExpectedFinalRowsDimensionMismatch;
				issue.row = row;
				issue.index = expectations.finalRows[row].size();
				issue.firstIndex = result.plan.grid.width;
				AddIssue(result, issue);
			}
		}
	}

	for (std::size_t frameIndex = 0;
		frameIndex < expectations.traceFrames.size();
		++frameIndex) {
		const RuntimeGameplayAsciiSourcePlanExpectedTraceFrame &frame =
			expectations.traceFrames[frameIndex];
		if (!frame.hasRows) {
			continue;
		}
		if (frame.rows.empty()) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code =
				RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedTraceFrameRowsEmpty;
			issue.index = frameIndex;
			AddIssue(result, issue);
			continue;
		}
		if (frame.rows.size() != result.plan.grid.rows.size()) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
				ExpectedTraceFrameRowsDimensionMismatch;
			issue.index = frameIndex;
			issue.firstIndex = frame.rows.size();
			issue.row = result.plan.grid.rows.size();
			AddIssue(result, issue);
		}
		for (std::size_t row = 0; row < frame.rows.size(); ++row) {
			if (frame.rows[row].size() != result.plan.grid.width) {
				RuntimeGameplayAsciiSourcePlanIssue issue;
				issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
					ExpectedTraceFrameRowsDimensionMismatch;
				issue.index = frameIndex;
				issue.row = row;
				issue.firstIndex = result.plan.grid.width;
				issue.column = frame.rows[row].size();
				AddIssue(result, issue);
			}
		}
	}

	for (std::size_t index = 0; index < expectations.inventoryStacks.size();
		++index) {
		const RuntimeGameplayAsciiSourcePlanExpectedInventoryStack &stack =
			expectations.inventoryStacks[index];
		if (stack.itemId.empty()) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
				ExpectedInventoryStackMissingItemId;
			issue.index = index;
			AddIssue(result, issue);
		}
		if (stack.count == 0) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
				ExpectedInventoryStackInvalidCount;
			issue.index = index;
			issue.id = stack.itemId;
			AddIssue(result, issue);
		}
		if (!stack.itemId.empty() &&
			HasEarlierMatchingExpectedInventoryStack(
				expectations.inventoryStacks,
				index)) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
				ExpectedInventoryStackDuplicateItemId;
			issue.index = index;
			issue.id = stack.itemId;
			AddIssue(result, issue);
		}
	}

	for (std::size_t index = 0; index < expectations.interactionTargets.size();
		++index) {
		const RuntimeGameplayAsciiSourcePlanExpectedInteractionTarget &target =
			expectations.interactionTargets[index];
		if (target.targetId.empty()) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
				ExpectedInteractionTargetMissingId;
			issue.index = index;
			AddIssue(result, issue);
			continue;
		}
		if (HasEarlierMatchingExpectedInteractionTarget(
				expectations.interactionTargets,
				index)) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
				ExpectedInteractionTargetDuplicateId;
			issue.index = index;
			issue.id = target.targetId;
			AddIssue(result, issue);
		}
		if (!HasAuthoredInteractionTarget(result.plan, target.targetId)) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
				ExpectedInteractionTargetUnknownId;
			issue.index = index;
			issue.id = target.targetId;
			AddIssue(result, issue);
		}
	}

	for (std::size_t index = 0; index < expectations.actorStates.size();
		++index) {
		const RuntimeGameplayAsciiSourcePlanExpectedActorState &actor =
			expectations.actorStates[index];
		if (actor.actorId.empty()) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code =
				RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedActorStateMissingId;
			issue.index = index;
			AddIssue(result, issue);
			continue;
		}
		if (HasEarlierMatchingExpectedActorState(
				expectations.actorStates,
				index)) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code =
				RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedActorStateDuplicateId;
			issue.index = index;
			issue.id = actor.actorId;
			AddIssue(result, issue);
		}
		if (!HasAuthoredActor(result.plan, actor.actorId)) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code =
				RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedActorStateUnknownId;
			issue.index = index;
			issue.id = actor.actorId;
			AddIssue(result, issue);
		}
		if (!actor.hasTile) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code =
				RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedActorStateMissingTile;
			issue.index = index;
			issue.id = actor.actorId;
			AddIssue(result, issue);
		}
	}

	if (expectations.hasPlayerState) {
		if (!HasPlayerStart(result.plan)) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
				ExpectedPlayerStateMissingPlayer;
			AddIssue(result, issue);
		}
		if (!expectations.playerState.hasTile) {
			RuntimeGameplayAsciiSourcePlanIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanIssueCode::
				ExpectedPlayerStateMissingTile;
			AddIssue(result, issue);
		}
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

	ValidateCompatibility(result);
	ValidateRows(result);
	const std::vector<char> legendGlyphs = ValidateLegend(result);
	ValidateAnnotatedCells(result);
	ValidateRegions(result);
	ValidateAuthoredControls(result);
	ValidateAuthoredProfiles(result);
	ValidateAuthoredInteractionTargets(result);
	ValidateAuthoredItemDrops(result);
	ValidateAuthoredPlayerCommands(result);
	ValidateExpectations(result);
	ValidateBoundaryFlags(result);
	ValidateGridGlyphs(result, legendGlyphs);

	result.status = result.issues.empty()
		? RuntimeGameplayAsciiSourcePlanValidationStatus::Valid
		: RuntimeGameplayAsciiSourcePlanValidationStatus::Invalid;
	result.issueCount = result.issues.size();
	return result;
}

} // namespace iggy::runtime
