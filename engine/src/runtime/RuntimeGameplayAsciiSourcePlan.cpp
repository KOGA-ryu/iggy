#include "runtime/RuntimeGameplayAsciiSourcePlan.hpp"

namespace iggy::runtime {

bool RuntimeGameplayAsciiSourcePlanGrid::hasRows() const
{
	return !rows.empty();
}

std::size_t RuntimeGameplayAsciiSourcePlanGrid::rowCount() const
{
	return rows.size();
}

bool RuntimeGameplayAsciiSourcePlanExpectations::hasAny() const
{
	return hasFinalRows ||
		hasFrameCount ||
		hasAcceptedCommandCount ||
		hasPickedUpCount ||
		hasInteractionChanged ||
		hasNpcMovedCount;
}

bool RuntimeGameplayAsciiSourcePlan::hasRows() const
{
	return grid.hasRows();
}

std::size_t RuntimeGameplayAsciiSourcePlan::rowCount() const
{
	return grid.rowCount();
}

std::size_t RuntimeGameplayAsciiSourcePlan::legendCount() const
{
	return legend.size();
}

std::size_t RuntimeGameplayAsciiSourcePlan::annotatedCellCount() const
{
	return annotatedCells.size();
}

std::size_t RuntimeGameplayAsciiSourcePlan::regionCount() const
{
	return regions.size();
}

std::size_t RuntimeGameplayAsciiSourcePlan::authoredControlCount() const
{
	return authoredControls.size();
}

std::size_t RuntimeGameplayAsciiSourcePlan::authoredProfileCount() const
{
	return authoredProfiles.size();
}

std::size_t RuntimeGameplayAsciiSourcePlan::authoredInteractionTargetCount() const
{
	return authoredInteractionTargets.size();
}

std::size_t RuntimeGameplayAsciiSourcePlan::authoredItemDropCount() const
{
	return authoredItemDrops.size();
}

std::size_t RuntimeGameplayAsciiSourcePlan::authoredPlayerCommandCount() const
{
	return authoredPlayerCommands.size();
}

bool RuntimeGameplayAsciiSourcePlan::hasExpectations() const
{
	return expectations.hasAny();
}

bool RuntimeGameplayAsciiSourcePlan::safeForAuthoring() const
{
	return !noClaims.claimsRuntimeTruth &&
		!noClaims.claimsGameplayExecution &&
		!noClaims.claimsFileParsing &&
		!noClaims.claimsProfileScenarioConversion &&
		!promotionPolicy.promotionReady &&
		!promotionPolicy.allowsRuntimeExecution &&
		!promotionPolicy.allowsFileParsing &&
		!promotionPolicy.allowsProfileScenarioConversion;
}

} // namespace iggy::runtime
