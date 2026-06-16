#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeGameplayAsciiSourcePlan.hpp"
#include "scene/ai/AiMap2D.hpp"

namespace iggy::runtime {

struct RuntimeGameplayAsciiSourcePlanRegionAiMapPolicy {
	ResourceId roleTag;
	std::vector<ResourceId> nodeTags;
	float patrolWeight = 0.0F;
	float coverWeight = 0.0F;
	float dangerWeight = 0.0F;
	float interestWeight = 0.0F;
	bool enabled = true;
};

struct RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionConfig {
	std::vector<RuntimeGameplayAsciiSourcePlanRegionAiMapPolicy> policies;
	bool includeUnmappedRegionDiagnostics = true;
	bool failOnUnmappedRegions = false;
};

enum class RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionStatus {
	Promoted,
	NoPolicies,
	UnmappedRegions,
	AiMapInvalid,
};

enum class RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionIssueCode {
	UnmappedRegion,
	MappedRegionMissingId,
	AiMapInvalid,
};

struct RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionIssue {
	RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionIssueCode code =
		RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionIssueCode::UnmappedRegion;
	std::size_t regionIndex = 0;
	std::size_t policyIndex = 0;
	RuntimeGameplayAsciiSourcePlanRegion region;
	AiMap2DIssue aiMapIssue;
};

struct RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionResult {
	RuntimeGameplayAsciiSourcePlan sourcePlan;
	RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionConfig config;
	AiMap2DBuildResult aiMapBuild;
	AiMap2D aiMap;
	std::vector<RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionIssue> issues;
	RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionStatus status =
		RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionStatus::Promoted;
	bool promoted = false;
	std::size_t regionCount = 0;
	std::size_t mappedRegionCount = 0;
	std::size_t unmappedRegionCount = 0;
	std::size_t aiMapIssueCount = 0;
	std::size_t issueCount = 0;

	[[nodiscard]] bool ok() const;
};

class RuntimeGameplayAsciiSourcePlanRegionAiMapPromoter {
public:
	[[nodiscard]] RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionResult promote(
		const RuntimeGameplayAsciiSourcePlan &sourcePlan,
		const RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionConfig &config = {}) const;
};

} // namespace iggy::runtime
