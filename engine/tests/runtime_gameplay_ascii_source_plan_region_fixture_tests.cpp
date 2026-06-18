#include "runtime/RuntimeGameplayAsciiSourcePlanProfileScenarioConverter.hpp"
#include "runtime/RuntimeGameplayAsciiSourcePlanRegionAiMapPromoter.hpp"
#include "runtime/RuntimeGameplayAsciiSourcePlanTomlFileReader.hpp"
#include "runtime/RuntimeGameplayProfileScenarioRunner.hpp"
#include "support/TestHarness.hpp"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#ifndef IGGY_TEST_FIXTURE_DIR
#error "IGGY_TEST_FIXTURE_DIR must point at engine/tests/fixtures/runtime/ascii_source_plan"
#endif

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::Near;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

std::filesystem::path FixturePath(const char *name)
{
	return std::filesystem::path(IGGY_TEST_FIXTURE_DIR) / name;
}

iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult ReadFixture()
{
	return iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReader {}.read(
		FixturePath("region_ai_map_room.toml"));
}

iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPolicy PatrolPolicy()
{
	return {
		Id("role:patrol-region"),
		{ Id("tag:patrol"), Id("tag:cover") },
		2.0F,
		3.0F,
		4.0F,
		5.0F,
		true,
	};
}

iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionConfig
RegionAiMapConfig()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionConfig config;
	config.policies = { PatrolPolicy() };
	return config;
}

void ExpectPromotedPatrolNode(const iggy::AiMapNode2D &node, const char *context)
{
	Expect(node.id == Id("region:patrol-zone"), std::string(context) + " should preserve region id as node id");
	Expect(NearVec(node.position, { 4.5F, 2.0F }), std::string(context) + " should use inclusive region center");
	Expect(Near(node.radius, 0.5F * std::sqrt(13.0F)), std::string(context) + " should use half diagonal radius");
	Expect(node.tags == std::vector<iggy::ResourceId>({ Id("tag:patrol"), Id("tag:cover") }), std::string(context) + " should preserve configured node tags");
	Expect(node.patrolWeight == 2.0F && node.coverWeight == 3.0F && node.dangerWeight == 4.0F && node.interestWeight == 5.0F, std::string(context) + " should preserve configured weights");
	Expect(node.enabled, std::string(context) + " should preserve enabled flag");
	Expect(node.links.empty(), std::string(context) + " should not invent links");
}

void TestFixtureParsesRegionFactsExactly()
{
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult read =
		ReadFixture();

	Expect(read.ok(), "region AI map fixture should read and parse");
	Expect(read.text.plan.sourceId == Id("scenario:region-ai-map-room"), "fixture should preserve source id");
	Expect(read.text.plan.regions.size() == 1, "fixture should parse one region");
	if (!read.text.plan.regions.empty()) {
		const iggy::runtime::RuntimeGameplayAsciiSourcePlanRegion &region =
			read.text.plan.regions[0];
		Expect(region.hasRegionId, "fixture region should have id");
		Expect(region.regionId == Id("region:patrol-zone"), "fixture region should preserve exact id");
		Expect(region.minRow == 1 && region.minColumn == 3, "fixture region should preserve min bounds");
		Expect(region.maxRow == 2 && region.maxColumn == 5, "fixture region should preserve max bounds");
		Expect(region.roleTags == std::vector<iggy::ResourceId>({ Id("role:patrol-region"), Id("tag:quiet-zone") }), "fixture region should preserve exact role tags");
	}
}

void TestExplicitPolicyPromotesFixtureRegionToAiMap()
{
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult read =
		ReadFixture();
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionResult
		promotion = iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPromoter {}
			.promote(read.text.plan, RegionAiMapConfig());

	Expect(read.ok(), "region AI map fixture should read before promotion");
	Expect(promotion.ok(), "explicit region AI map policy should promote fixture");
	Expect(promotion.regionCount == 1, "region AI map promotion should count one region");
	Expect(promotion.mappedRegionCount == 1 && promotion.unmappedRegionCount == 0, "region AI map promotion should map the fixture region");
	Expect(promotion.issues.empty(), "region AI map promotion should have no diagnostics");
	Expect(promotion.aiMap.nodes.size() == 1, "region AI map promotion should publish one node");
	if (!promotion.aiMap.nodes.empty()) {
		ExpectPromotedPatrolNode(promotion.aiMap.nodes[0], "promoted AI map node");
	}
}

void TestExplicitConverterConfigWiresPromotedAiMapIntoProfileFrame()
{
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult read =
		ReadFixture();
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig
		config;
	config.promoteRegionAiMap = true;
	config.regionAiMap = RegionAiMapConfig();

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult
		conversion =
			iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConverter {}
				.convert(read.text.plan, config);

	Expect(read.ok(), "region AI map fixture should read before conversion");
	Expect(conversion.ok(), "explicit region AI map conversion should succeed");
	Expect(conversion.config.promoteRegionAiMap, "conversion should preserve explicit region AI map opt-in");
	Expect(conversion.config.regionAiMap.policies.size() == 1, "conversion should preserve explicit region AI map policy");
	Expect(conversion.profileTraitCatalog.built, "conversion should build profile traits from fixture facts");
	Expect(conversion.promotedAiMapRegionCount == 1 && conversion.unmappedAiMapRegionCount == 0, "conversion should mirror region AI map promotion counts");
	Expect(conversion.definition.frames.size() == 1, "conversion should produce one fallback frame");
	if (!conversion.definition.frames.empty()) {
		const iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition &frame =
			conversion.definition.frames[0];
		Expect(frame.aiMap.nodes.size() == 1, "conversion should wire promoted AI map into frame");
		Expect(frame.refreshAiMap.nodes.size() == 1, "conversion should wire promoted refresh AI map into frame");
		if (!frame.aiMap.nodes.empty()) {
			ExpectPromotedPatrolNode(frame.aiMap.nodes[0], "profile frame AI map node");
		}
		if (!frame.refreshAiMap.nodes.empty()) {
			ExpectPromotedPatrolNode(frame.refreshAiMap.nodes[0], "profile frame refresh AI map node");
		}
	}
}

void TestProfileScenarioRunnerPreservesPromotedAiMap()
{
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult read =
		ReadFixture();
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig
		config;
	config.promoteRegionAiMap = true;
	config.regionAiMap = RegionAiMapConfig();
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult
		conversion =
			iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConverter {}
				.convert(read.text.plan, config);

	const iggy::runtime::RuntimeGameplayProfileScenarioRunResult run =
		iggy::runtime::RuntimeGameplayProfileScenarioRunner {}.run(
			conversion.definition);

	Expect(read.ok(), "region AI map fixture should read before runner preservation");
	Expect(conversion.ok(), "runner preservation setup should convert");
	Expect(run.ran(), "runner should execute region AI map fixture");
	Expect(run.frameCount == 1, "runner should preserve one-frame scenario shape");
	Expect(run.definition.frames.size() == 1, "runner result should preserve profile frame");
	if (!run.definition.frames.empty()) {
		Expect(run.definition.frames[0].aiMap.nodes.size() == 1, "runner should preserve promoted AI map");
		Expect(run.definition.frames[0].refreshAiMap.nodes.size() == 1, "runner should preserve promoted refresh AI map");
	}
	Expect(run.build.scenarioDefinition.frames.size() == 1, "runner build should lower one scenario frame");
	if (!run.build.scenarioDefinition.frames.empty()) {
		Expect(run.build.scenarioDefinition.frames[0].frame.aiMap.nodes.size() == 1, "runner build should lower promoted AI map");
		Expect(run.build.scenarioDefinition.frames[0].frame.refreshAiMap.nodes.size() == 1, "runner build should lower promoted refresh AI map");
	}
}

} // namespace

int main()
{
	TestFixtureParsesRegionFactsExactly();
	TestExplicitPolicyPromotesFixtureRegionToAiMap();
	TestExplicitConverterConfigWiresPromotedAiMapIntoProfileFrame();
	TestProfileScenarioRunnerPreservesPromotedAiMap();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
