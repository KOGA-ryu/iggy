#include <cstdlib>
#include <string_view>
#include <vector>

#include "runtime/RuntimeGameplayAsciiSourcePlanProfileScenarioConverter.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::MapFromRows;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::runtime::RuntimeGameplayAsciiSourcePlan SourcePlan()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan;
	plan.formatId = Id("iggy:ascii-source-plan");
	plan.hasSourceId = true;
	plan.sourceId = Id("scenario:ascii-profile");
	plan.grid.width = 5;
	plan.grid.height = 3;
	plan.grid.rows = {
		"#####",
		"#A..#",
		"#####",
	};
	plan.grid.backgroundGlyph = '.';
	plan.legend = {
		{
			'A',
			iggy::runtime::RuntimeGameplayAsciiSourcePlanGlyphKind::Actor,
			Id("role:npc"),
			{ Id("tag:guard") },
			true,
			iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::Actor,
			Id("npc:guard"),
			Id("profile:guard"),
		},
	};
	plan.annotatedCells = {
		{
			true,
			Id("cell:guard"),
			1,
			1,
			'A',
			{ true, 1, 1 },
			{ true, 1.5, 1.5 },
			{ true, 1.0, 1.0, 2.0, 2.0 },
			{ Id("tag:guard") },
			Id("npc:guard"),
			Id("profile:guard"),
		},
	};
	return plan;
}

iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition DefaultFrame()
{
	iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition frame;
	frame.hasFrameId = true;
	frame.frameId = Id("frame:ascii-profile");
	frame.movementMap = MapFromRows({
		"#####",
		"#...#",
		"#####",
	});
	frame.movementMap.id = Id("level:ascii-profile");
	return frame;
}

iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig ConfigWithFrame()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config;
	config.hasDefaultFrame = true;
	config.defaultFrame = DefaultFrame();
	return config;
}

iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult Convert(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlan &plan,
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig &config = {})
{
	return iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConverter {}.convert(plan, config);
}

bool HasIssue(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult &result,
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode code)
{
	for (const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue &issue : result.issues) {
		if (issue.code == code)
			return true;
	}
	return false;
}

void TestInvalidSourcePlanShortCircuitsBeforePublishingDefinition()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.grid.rows[1] = "#Z..#";
	const iggy::runtime::RuntimeGameplayAsciiSourcePlan before = plan;
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config =
		ConfigWithFrame();

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult result =
		Convert(plan, config);

	Expect(!result.ok(), "invalid source plan should not convert");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::SourcePlanInvalid, "invalid source plan should report SourcePlanInvalid");
	Expect(!result.converted, "invalid source plan should not mark converted");
	Expect(!result.sourceValidation.ok(), "converter should preserve nested source validation failure");
	Expect(result.sourcePlanIssueCount == result.sourceValidation.issueCount, "source issues should be mirrored");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::SourcePlanInvalid), "source invalid issue should be present");
	Expect(result.definition.frames.empty(), "invalid source plan should not publish profile scenario frames");
	Expect(plan.grid.rows == before.grid.rows, "converter should not mutate invalid source plan");
}

void TestValidSourcePlanRequiresFrameDefaults()
{
	const iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult result =
		Convert(plan);

	Expect(!result.ok(), "missing frame defaults should not convert");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::MissingFrameDefaults, "missing frame defaults should be deterministic status");
	Expect(result.sourceValidation.ok(), "missing frame defaults should run after source validation passes");
	Expect(result.missingFrameDefaultsCount == 1, "missing frame defaults should be counted once");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::MissingFrameDefaults), "missing frame defaults issue should be present");
	Expect(result.definition.frames.empty(), "missing frame defaults should not publish profile scenario frames");
}

void TestValidSourcePlanAndFrameDefaultsPublishValidatedProfileScenario()
{
	const iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	const iggy::runtime::RuntimeGameplayAsciiSourcePlan before = plan;
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config =
		ConfigWithFrame();

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult result =
		Convert(plan, config);

	Expect(result.ok(), "valid source plan with frame defaults should convert");
	Expect(result.converted, "valid conversion should mark converted");
	Expect(result.issueCount == 0, "valid conversion should have zero issues");
	Expect(result.profileValidation.ok(), "converted profile scenario should validate");
	Expect(result.definition.hasScenarioId, "converted definition should use source id as scenario id");
	Expect(result.definition.scenarioId == Id("scenario:ascii-profile"), "converted definition should preserve source id");
	Expect(result.definition.frames.size() == 1, "converted definition should preserve one default frame");
	Expect(result.definition.frames[0].frameId == Id("frame:ascii-profile"), "converted definition should preserve frame id");
	Expect(result.definition.frames[0].movementMap.id == Id("level:ascii-profile"), "converted definition should preserve frame movement map");
	Expect(plan.grid.rows == before.grid.rows, "converter should not mutate source plan");
}

} // namespace

int main()
{
	TestInvalidSourcePlanShortCircuitsBeforePublishingDefinition();
	TestValidSourcePlanRequiresFrameDefaults();
	TestValidSourcePlanAndFrameDefaultsPublishValidatedProfileScenario();
	return Failures;
}
