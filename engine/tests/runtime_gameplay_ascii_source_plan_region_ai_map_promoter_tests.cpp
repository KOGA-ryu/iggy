#include <cstdlib>
#include <cmath>
#include <vector>

#include "runtime/RuntimeGameplayAsciiSourcePlanRegionAiMapPromoter.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::runtime::RuntimeGameplayAsciiSourcePlan SourcePlan()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan;
	plan.hasSourceId = true;
	plan.sourceId = Id("source:region-ai");
	plan.grid.width = 4;
	plan.grid.height = 4;
	plan.grid.rows = {
		"....",
		"....",
		"....",
		"....",
	};
	plan.grid.backgroundGlyph = '.';
	return plan;
}

iggy::runtime::RuntimeGameplayAsciiSourcePlanRegion Region(
	const char *id,
	std::size_t minRow,
	std::size_t minColumn,
	std::size_t maxRow,
	std::size_t maxColumn,
	std::vector<iggy::ResourceId> roleTags)
{
	return {
		true,
		Id(id),
		minRow,
		minColumn,
		maxRow,
		maxColumn,
		roleTags,
	};
}

iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPolicy Policy(
	const char *roleTag,
	std::vector<iggy::ResourceId> nodeTags = { Id("ai:patrol") },
	float patrolWeight = 1.0F,
	float coverWeight = 0.0F,
	float dangerWeight = 0.0F,
	float interestWeight = 0.0F,
	bool enabled = true)
{
	return {
		Id(roleTag),
		nodeTags,
		patrolWeight,
		coverWeight,
		dangerWeight,
		interestWeight,
		enabled,
	};
}

iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionResult Promote(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlan &plan,
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionConfig &config = {})
{
	return iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromoter {}.promote(plan, config);
}

void TestNoRegionsPromotesEmptyAiMap()
{
	const iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionResult result =
		Promote(plan);

	Expect(result.ok(), "no-region source plan should promote empty ai map");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionStatus::Promoted, "no-region source plan should report Promoted");
	Expect(result.regionCount == 0 && result.mappedRegionCount == 0 && result.unmappedRegionCount == 0, "no-region source plan should report zero counts");
	Expect(result.aiMapBuild.built, "no-region source plan should build empty ai map");
	Expect(result.aiMap.nodes.empty(), "no-region source plan should publish empty ai map");
	Expect(result.issues.empty(), "no-region source plan should have no diagnostics");
}

void TestNoPoliciesReportsUnmappedDiagnosticsNonFatal()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.regions = { Region("region:patrol", 1, 1, 1, 2, { Id("role:patrol") }) };

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionResult result =
		Promote(plan);

	Expect(result.ok(), "no-policy region promotion should stay non-fatal by default");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionStatus::NoPolicies, "no-policy region promotion should report NoPolicies");
	Expect(result.unmappedRegionCount == 1 && result.mappedRegionCount == 0, "no-policy region should be counted unmapped");
	Expect(result.issues.size() == 1, "no-policy region should report one unmapped diagnostic");
	Expect(result.issues[0].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionIssueCode::UnmappedRegion, "no-policy issue should be unmapped");
	Expect(result.aiMap.nodes.empty(), "no-policy region should publish empty ai map");
}

void TestMappedRegionCreatesAiMapNode()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.regions = { Region("region:patrol", 1, 1, 2, 3, { Id("role:patrol") }) };
	iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionConfig config;
	config.policies = {
		Policy(
			"role:patrol",
			{ Id("tag:patrol"), Id("plain") },
			2.0F,
			3.0F,
			4.0F,
			5.0F,
			false),
	};
	const iggy::runtime::RuntimeGameplayAsciiSourcePlan before = plan;

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionResult result =
		Promote(plan, config);

	Expect(result.ok(), "mapped region should promote");
	Expect(result.mappedRegionCount == 1 && result.unmappedRegionCount == 0, "mapped region should report mapped count");
	Expect(result.aiMap.nodes.size() == 1, "mapped region should create one ai map node");
	if (result.aiMap.nodes.size() == 1) {
		const iggy::AiMapNode2D &node = result.aiMap.nodes[0];
		Expect(node.id == Id("region:patrol"), "mapped region should use exact region id as node id");
		Expect(NearVec(node.position, { 2.5F, 2.0F }), "mapped region should use inclusive center");
		Expect(node.radius == 0.5F * std::sqrt(13.0F), "mapped region should use half diagonal radius");
		Expect(node.tags == std::vector<iggy::ResourceId>({ Id("tag:patrol"), Id("plain") }), "mapped region should preserve configured node tags");
		Expect(node.patrolWeight == 2.0F && node.coverWeight == 3.0F && node.dangerWeight == 4.0F && node.interestWeight == 5.0F, "mapped region should preserve configured weights");
		Expect(!node.enabled, "mapped region should preserve enabled flag");
		Expect(node.links.empty(), "mapped region should not invent links");
	}
	Expect(plan.regions.size() == before.regions.size(), "region ai map promoter should not mutate region count");
	Expect(plan.regions[0].regionId == before.regions[0].regionId, "region ai map promoter should not mutate region id");
	Expect(plan.regions[0].roleTags == before.regions[0].roleTags, "region ai map promoter should not mutate region tags");
}

void TestFirstMatchingPolicyWins()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.regions = {
		Region("region:multi", 0, 0, 0, 0, { Id("role:first"), Id("role:second") }),
	};
	iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionConfig config;
	config.policies = {
		Policy("role:first", { Id("tag:first") }, 1.0F),
		Policy("role:second", { Id("tag:second") }, 9.0F),
	};

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionResult result =
		Promote(plan, config);

	Expect(result.ok(), "multi-role region should promote");
	Expect(result.aiMap.nodes.size() == 1, "multi-role region should create one node");
	if (!result.aiMap.nodes.empty()) {
		Expect(result.aiMap.nodes[0].tags == std::vector<iggy::ResourceId>({ Id("tag:first") }), "first matching policy should provide tags");
		Expect(result.aiMap.nodes[0].patrolWeight == 1.0F, "first matching policy should provide weights");
	}
}

void TestUnmappedRegionNonFatalByDefault()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.regions = {
		Region("region:patrol", 0, 0, 0, 0, { Id("role:patrol") }),
		Region("region:ignored", 1, 1, 1, 1, { Id("role:ignored") }),
	};
	iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionConfig config;
	config.policies = { Policy("role:patrol") };

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionResult result =
		Promote(plan, config);

	Expect(result.ok(), "unmapped region should be non-fatal by default");
	Expect(result.mappedRegionCount == 1 && result.unmappedRegionCount == 1, "mixed mapping should count mapped and unmapped");
	Expect(result.issues.size() == 1, "unmapped region should report diagnostic by default");
	Expect(result.aiMap.nodes.size() == 1 && result.aiMap.nodes[0].id == Id("region:patrol"), "unmapped region should not create node");
}

void TestFailOnUnmappedRegionsBlocksPromotion()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.regions = {
		Region("region:patrol", 0, 0, 0, 0, { Id("role:patrol") }),
		Region("region:ignored", 1, 1, 1, 1, { Id("role:ignored") }),
	};
	iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionConfig config;
	config.policies = { Policy("role:patrol") };
	config.failOnUnmappedRegions = true;

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionResult result =
		Promote(plan, config);

	Expect(!result.ok(), "fail-on-unmapped should block promotion");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionStatus::UnmappedRegions, "fail-on-unmapped should report UnmappedRegions");
	Expect(result.mappedRegionCount == 1 && result.unmappedRegionCount == 1, "fail-on-unmapped should preserve counts");
	Expect(result.aiMap.nodes.empty(), "fail-on-unmapped should not publish ai map nodes");
}

void TestMappedMissingIdFails()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	iggy::runtime::RuntimeGameplayAsciiSourcePlanRegion region =
		Region("region:missing", 0, 0, 0, 0, { Id("role:patrol") });
	region.hasRegionId = false;
	region.regionId = {};
	plan.regions = { region };
	iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionConfig config;
	config.policies = { Policy("role:patrol") };

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionResult result =
		Promote(plan, config);

	Expect(!result.ok(), "mapped region without id should fail");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionStatus::AiMapInvalid, "mapped missing id should report AiMapInvalid");
	Expect(result.issues.size() == 1, "mapped missing id should report one issue");
	Expect(result.issues[0].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionIssueCode::MappedRegionMissingId, "mapped missing id issue should be preserved");
}

void TestDuplicateMappedRegionIdsFailThroughAiMapBuilder()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.regions = {
		Region("region:dupe", 0, 0, 0, 0, { Id("role:patrol") }),
		Region("region:dupe", 1, 1, 1, 1, { Id("role:patrol") }),
	};
	iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionConfig config;
	config.policies = { Policy("role:patrol") };

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionResult result =
		Promote(plan, config);

	Expect(!result.ok(), "duplicate mapped region ids should fail");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionStatus::AiMapInvalid, "duplicate mapped region ids should report AiMapInvalid");
	Expect(result.aiMapBuild.issues.size() == 1, "duplicate mapped region ids should preserve ai map builder issue");
	Expect(result.aiMapBuild.issues[0].code == iggy::AiMap2DIssueCode::DuplicateNodeId, "duplicate mapped region ids should preserve duplicate node issue");
	Expect(result.aiMapIssueCount == 1, "duplicate mapped region ids should mirror ai map issue count");
}

void TestNamespacedAndUnqualifiedIdsAreExact()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.regions = {
		Region("room", 0, 0, 0, 0, { Id("role:patrol") }),
		Region("region:room", 1, 1, 1, 1, { Id("role:namespaced") }),
	};
	iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionConfig config;
	config.policies = {
		Policy("role:patrol", { Id("tag:plain") }),
		Policy("role:namespaced", { Id("tag:namespaced") }),
	};

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionResult result =
		Promote(plan, config);

	Expect(result.ok(), "namespaced and unqualified ids should promote");
	Expect(result.aiMap.nodes.size() == 2, "namespaced and unqualified ids should produce two nodes");
	Expect(result.aiMap.contains(Id("room")), "unqualified region id should be exact");
	Expect(result.aiMap.contains(Id("region:room")), "namespaced region id should be exact");
}

void TestConfigIsCopiedAndNotMutated()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.regions = { Region("region:patrol", 0, 0, 0, 0, { Id("role:patrol") }) };
	iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionConfig config;
	config.policies = { Policy("role:patrol", { Id("tag:patrol") }) };
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionConfig before = config;

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionResult result =
		Promote(plan, config);

	Expect(result.ok(), "config immutability setup should promote");
	Expect(config.policies[0].nodeTags == before.policies[0].nodeTags, "promoter should not mutate config");
	Expect(result.config.policies[0].nodeTags == before.policies[0].nodeTags, "promoter result should copy config");
}

} // namespace

int main()
{
	TestNoRegionsPromotesEmptyAiMap();
	TestNoPoliciesReportsUnmappedDiagnosticsNonFatal();
	TestMappedRegionCreatesAiMapNode();
	TestFirstMatchingPolicyWins();
	TestUnmappedRegionNonFatalByDefault();
	TestFailOnUnmappedRegionsBlocksPromotion();
	TestMappedMissingIdFails();
	TestDuplicateMappedRegionIdsFailThroughAiMapBuilder();
	TestNamespacedAndUnqualifiedIdsAreExact();
	TestConfigIsCopiedAndNotMutated();
	return Failures;
}
