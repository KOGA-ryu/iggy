#include "runtime/RuntimeGameplayAsciiSourcePlanRegionAiMapPromoter.hpp"

#include <cmath>

namespace iggy::runtime {
namespace {

void AddIssue(
	RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionResult &result,
	RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionIssue issue)
{
	if (issue.code == RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionIssueCode::AiMapInvalid) {
		++result.aiMapIssueCount;
	}
	result.issues.push_back(issue);
	result.issueCount = result.issues.size();
}

bool RegionHasRoleTag(
	const RuntimeGameplayAsciiSourcePlanRegion &region,
	const ResourceId &roleTag)
{
	for (const ResourceId &tag : region.roleTags) {
		if (tag == roleTag) {
			return true;
		}
	}
	return false;
}

const RuntimeGameplayAsciiSourcePlanRegionAiMapPolicy *FindFirstPolicy(
	const RuntimeGameplayAsciiSourcePlanRegion &region,
	const RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionConfig &config,
	std::size_t &policyIndex)
{
	for (std::size_t index = 0; index < config.policies.size(); ++index) {
		if (RegionHasRoleTag(region, config.policies[index].roleTag)) {
			policyIndex = index;
			return &config.policies[index];
		}
	}
	return nullptr;
}

AiMapNode2D NodeFromRegion(
	const RuntimeGameplayAsciiSourcePlanRegion &region,
	const RuntimeGameplayAsciiSourcePlanRegionAiMapPolicy &policy)
{
	const float width =
		static_cast<float>(region.maxColumn - region.minColumn + 1);
	const float height =
		static_cast<float>(region.maxRow - region.minRow + 1);

	AiMapNode2D node;
	node.id = region.regionId;
	node.position = {
		static_cast<float>(region.minColumn + region.maxColumn + 1) * 0.5F,
		static_cast<float>(region.minRow + region.maxRow + 1) * 0.5F,
	};
	node.radius = 0.5F * std::sqrt(width * width + height * height);
	node.patrolWeight = policy.patrolWeight;
	node.coverWeight = policy.coverWeight;
	node.dangerWeight = policy.dangerWeight;
	node.interestWeight = policy.interestWeight;
	node.tags = policy.nodeTags;
	node.enabled = policy.enabled;
	return node;
}

} // namespace

bool RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionResult::ok() const
{
	return promoted;
}

RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionResult
RuntimeGameplayAsciiSourcePlanRegionAiMapPromoter::promote(
	const RuntimeGameplayAsciiSourcePlan &sourcePlan,
	const RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionConfig &config) const
{
	RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionResult result;
	result.sourcePlan = sourcePlan;
	result.config = config;
	result.regionCount = sourcePlan.regions.size();

	if (sourcePlan.regions.empty()) {
		result.aiMapBuild = AiMap2DBuilder {}.build({});
		result.aiMap = result.aiMapBuild.map;
		result.promoted = result.aiMapBuild.built;
		result.status = result.promoted
			? RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionStatus::Promoted
			: RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionStatus::AiMapInvalid;
		return result;
	}

	if (config.policies.empty()) {
		result.unmappedRegionCount = sourcePlan.regions.size();
		if (config.includeUnmappedRegionDiagnostics) {
			for (std::size_t index = 0; index < sourcePlan.regions.size(); ++index) {
				RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionIssue issue;
				issue.code =
					RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionIssueCode::UnmappedRegion;
				issue.regionIndex = index;
				issue.region = sourcePlan.regions[index];
				AddIssue(result, issue);
			}
		}
		result.aiMapBuild = AiMap2DBuilder {}.build({});
		result.aiMap = result.aiMapBuild.map;
		result.promoted = result.aiMapBuild.built && !config.failOnUnmappedRegions;
		result.status = config.failOnUnmappedRegions
			? RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionStatus::NoPolicies
			: RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionStatus::NoPolicies;
		return result;
	}

	std::vector<AiMapNode2D> nodes;
	bool hasFatalIssue = false;
	for (std::size_t regionIndex = 0; regionIndex < sourcePlan.regions.size(); ++regionIndex) {
		const RuntimeGameplayAsciiSourcePlanRegion &region = sourcePlan.regions[regionIndex];
		std::size_t policyIndex = 0;
		const RuntimeGameplayAsciiSourcePlanRegionAiMapPolicy *policy =
			FindFirstPolicy(region, config, policyIndex);
		if (policy == nullptr) {
			++result.unmappedRegionCount;
			if (config.includeUnmappedRegionDiagnostics) {
				RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionIssue issue;
				issue.code =
					RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionIssueCode::UnmappedRegion;
				issue.regionIndex = regionIndex;
				issue.region = region;
				AddIssue(result, issue);
			}
			if (config.failOnUnmappedRegions) {
				hasFatalIssue = true;
			}
			continue;
		}

		if (!region.hasRegionId || region.regionId.empty()) {
			RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionIssue issue;
			issue.code =
				RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionIssueCode::MappedRegionMissingId;
			issue.regionIndex = regionIndex;
			issue.policyIndex = policyIndex;
			issue.region = region;
			AddIssue(result, issue);
			hasFatalIssue = true;
			continue;
		}

		++result.mappedRegionCount;
		nodes.push_back(NodeFromRegion(region, *policy));
	}

	if (hasFatalIssue) {
		result.status = config.failOnUnmappedRegions && result.mappedRegionCount < result.regionCount
			? RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionStatus::UnmappedRegions
			: RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionStatus::AiMapInvalid;
		result.promoted = false;
		return result;
	}

	result.aiMapBuild = AiMap2DBuilder {}.build(nodes);
	result.aiMap = result.aiMapBuild.map;
	if (!result.aiMapBuild.built) {
		for (const AiMap2DIssue &aiMapIssue : result.aiMapBuild.issues) {
			RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionIssue issue;
			issue.code =
				RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionIssueCode::AiMapInvalid;
			issue.aiMapIssue = aiMapIssue;
			AddIssue(result, issue);
		}
		result.status = RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionStatus::AiMapInvalid;
		result.promoted = false;
		return result;
	}

	result.status = RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionStatus::Promoted;
	result.promoted = true;
	return result;
}

} // namespace iggy::runtime
