#include <cstdlib>

#include "runtime/RuntimeGameplayAsciiScenarioPacketValidator.hpp"
#include "runtime/RuntimeGameplayAsciiSourcePlanAdapter.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::runtime::RuntimeGameplayAsciiSourcePlan SourcePlan()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan;
	plan.formatId = Id("iggy:ascii-source-plan");
	plan.hasSourceId = true;
	plan.sourceId = Id("source:adapter");
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
			{},
			{},
		},
		{
			'.',
			iggy::runtime::RuntimeGameplayAsciiSourcePlanGlyphKind::Terrain,
			Id("role:floor"),
			{},
			true,
			iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::Floor,
			{},
			{},
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
			{ true, 1.0, 1.0 },
			{ true, 1.0, 1.0, 2.0, 2.0 },
			{ Id("tag:guard") },
			Id("npc:guard"),
			Id("profile:guard"),
		},
	};
	plan.regions = {
		{ true, Id("region:room"), 0, 0, 2, 4, { Id("tag:room") } },
	};
	return plan;
}

iggy::runtime::RuntimeGameplayAsciiSourcePlanAdapterResult Convert(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlan &plan)
{
	return iggy::runtime::RuntimeGameplayAsciiSourcePlanToPacketAdapter {}.convert(plan);
}

bool HasIssue(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanAdapterResult &result,
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAdapterIssueCode code)
{
	for (const iggy::runtime::RuntimeGameplayAsciiSourcePlanAdapterIssue &issue :
		result.issues) {
		if (issue.code == code) {
			return true;
		}
	}
	return false;
}

void TestValidSourcePlanConvertsToTypedPacket()
{
	const iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanAdapterResult result =
		Convert(plan);

	Expect(result.converted(), "valid source plan should convert");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanAdapterStatus::Converted, "valid source plan should report Converted");
	Expect(result.validation.ok(), "adapter should preserve valid nested validation");
	Expect(result.issueCount == 0, "converted source plan should have no adapter issues");
	Expect(result.packet.hasSourceId && result.packet.sourceId == Id("source:adapter"), "adapter should preserve source id");
	Expect(result.packet.rows == plan.grid.rows, "adapter should preserve source rows");
	Expect(result.rowCount == 3, "adapter should report row count");
	Expect(result.markerCount == 2, "adapter should produce floor and actor marker declarations");
	Expect(result.packet.markers[0].marker == '.', "adapter should map non-actor legend markers first");
	Expect(result.packet.markers[0].kind == iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::Floor, "adapter should preserve floor marker kind");
	Expect(result.packet.markers[1].marker == 'A', "adapter should map annotated actor cell marker");
	Expect(result.packet.markers[1].actorId == Id("npc:guard"), "adapter should preserve actor id");
	Expect(result.packet.markers[1].profileId == Id("profile:guard"), "adapter should preserve profile id");
}

void TestConvertedPacketValidatesWithExistingPacketValidator()
{
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanAdapterResult result =
		Convert(SourcePlan());
	const iggy::runtime::RuntimeGameplayAsciiScenarioPacketValidationResult packet =
		iggy::runtime::RuntimeGameplayAsciiScenarioPacketValidator {}.validate(result.packet);

	Expect(result.converted(), "source plan should convert before packet validation");
	Expect(packet.ok(), "converted source plan packet should validate with existing typed packet validator");
	Expect(packet.rowCount == 3 && packet.width == 5, "typed packet validator should preserve converted row facts");
	Expect(packet.markerCount == 2, "typed packet validator should preserve converted marker facts");
}

void TestInvalidSourcePlanStopsBeforeConversion()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.grid.rows.clear();

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanAdapterResult result =
		Convert(plan);

	Expect(!result.converted(), "invalid source plan should not convert");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanAdapterStatus::SourcePlanInvalid, "invalid source plan should report SourcePlanInvalid");
	Expect(!result.validation.ok(), "invalid source plan should preserve failed nested validation");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanAdapterIssueCode::SourcePlanInvalid), "invalid source plan should report adapter issue");
	Expect(result.packet.rows.empty(), "invalid source plan should not publish converted rows");
}

void TestDuplicateScenarioMarkerGlyphIsUnmappable()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.grid.rows = {
		"#####",
		"#A.A#",
		"#####",
	};
	plan.annotatedCells.push_back(plan.annotatedCells[0]);
	plan.annotatedCells[1].cellId = Id("cell:guard-2");
	plan.annotatedCells[1].row = 1;
	plan.annotatedCells[1].column = 3;
	plan.annotatedCells[1].markerId = Id("npc:guard-2");
	plan.annotatedCells[1].profileId = Id("profile:guard-2");

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanAdapterResult result =
		Convert(plan);

	Expect(!result.converted(), "duplicate scenario marker glyph should be unmappable");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanAdapterStatus::Unmappable, "duplicate marker glyph should report Unmappable");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanAdapterIssueCode::DuplicateScenarioMarkerGlyph), "duplicate marker glyph should report issue");
	Expect(result.markerCount == 2, "adapter should keep already representable floor and first actor markers");
}

void TestUnsupportedAnnotatedCellMappingIsReported()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.legend[0].mapsToScenarioMarker = false;

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanAdapterResult result =
		Convert(plan);

	Expect(!result.converted(), "annotated actor ids without scenario mapping should be unmappable");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanAdapterStatus::Unmappable, "unsupported annotated mapping should report Unmappable");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanAdapterIssueCode::UnsupportedAnnotatedCellMapping), "unsupported annotated mapping should report issue");
}

void TestAdapterDoesNotMutateInput()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	const iggy::runtime::RuntimeGameplayAsciiSourcePlan before = plan;

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanAdapterResult result =
		Convert(plan);

	Expect(result.converted(), "valid source plan should convert");
	Expect(plan.grid.rows == before.grid.rows, "adapter should not mutate rows");
	Expect(plan.legend[0].targetMarkerId == before.legend[0].targetMarkerId, "adapter should not mutate legend");
	Expect(plan.annotatedCells[0].markerId == before.annotatedCells[0].markerId, "adapter should not mutate annotated cells");
	Expect(result.sourcePlan.annotatedCells[0].markerId == before.annotatedCells[0].markerId, "adapter result should preserve copied source plan");
}

} // namespace

int main()
{
	TestValidSourcePlanConvertsToTypedPacket();
	TestConvertedPacketValidatesWithExistingPacketValidator();
	TestInvalidSourcePlanStopsBeforeConversion();
	TestDuplicateScenarioMarkerGlyphIsUnmappable();
	TestUnsupportedAnnotatedCellMappingIsReported();
	TestAdapterDoesNotMutateInput();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
