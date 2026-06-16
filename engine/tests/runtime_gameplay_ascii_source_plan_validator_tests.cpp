#include <cstdlib>

#include "runtime/RuntimeGameplayAsciiSourcePlanValidator.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::runtime::RuntimeGameplayAsciiSourcePlan ValidPlan()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan;
	plan.formatId = Id("iggy:ascii-source-plan");
	plan.hasSourceId = true;
	plan.sourceId = Id("source:room-a");
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
			{ true, 1.0, 1.0 },
			{ true, 1.0, 1.0, 2.0, 2.0 },
			{ Id("tag:guard") },
			Id("npc:guard"),
			Id("profile:guard"),
		},
	};
	plan.regions = {
		{ true, Id("region:room-a"), 0, 0, 2, 4, { Id("tag:room") } },
	};
	return plan;
}

iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult Validate(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlan &plan)
{
	return iggy::runtime::RuntimeGameplayAsciiSourcePlanValidator {}.validate(plan);
}

bool HasIssue(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult &result,
	iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode code)
{
	for (const iggy::runtime::RuntimeGameplayAsciiSourcePlanIssue &issue : result.issues) {
		if (issue.code == code) {
			return true;
		}
	}
	return false;
}

void TestValidSourcePlanPasses()
{
	const iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = ValidPlan();
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult result =
		Validate(plan);

	Expect(result.ok(), "valid source plan should validate");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationStatus::Valid, "valid source plan should report Valid");
	Expect(result.issueCount == 0, "valid source plan should have zero issues");
	Expect(result.rowCount == 3 && result.width == 5 && result.height == 3, "valid source plan should report grid facts");
	Expect(result.legendCount == 1, "valid source plan should report legend count");
	Expect(result.annotatedCellCount == 1, "valid source plan should report annotated cell count");
	Expect(result.regionCount == 1, "valid source plan should report region count");
	Expect(result.plan.sourceId == Id("source:room-a"), "validator should preserve copied source plan");
}

void TestEmptyAndRaggedRowsFailDeterministically()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan empty;
	empty.grid.width = 3;
	empty.grid.height = 1;
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult emptyResult =
		Validate(empty);
	Expect(!emptyResult.ok(), "empty source plan rows should fail");
	Expect(HasIssue(emptyResult, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::EmptyRows), "empty source plan should report EmptyRows");

	iggy::runtime::RuntimeGameplayAsciiSourcePlan ragged = ValidPlan();
	ragged.grid.width = 5;
	ragged.grid.height = 3;
	ragged.grid.rows = { "#####", "#A#" };
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult raggedResult =
		Validate(ragged);

	Expect(!raggedResult.ok(), "ragged source plan should fail");
	Expect(raggedResult.issues.size() >= 2, "ragged source plan should preserve row shape issues");
	Expect(raggedResult.issues[0].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::GridDimensionMismatch, "first row-shape issue should be dimension mismatch");
	Expect(raggedResult.issues[1].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::RaggedRow, "second row-shape issue should be ragged row");
	Expect(raggedResult.raggedRowCount == 1, "ragged source plan should count ragged rows");
}

void TestLegendGlyphIssuesFail()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = ValidPlan();
	plan.legend.push_back(plan.legend[0]);
	plan.legend.push_back({});

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult result =
		Validate(plan);

	Expect(!result.ok(), "duplicate and empty legend glyphs should fail");
	Expect(result.duplicateGlyphCount == 1, "duplicate legend glyph should be counted");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::DuplicateGlyph), "duplicate legend glyph should be reported");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::EmptyGlyph), "empty legend glyph should be reported");
}

void TestUnknownGridGlyphFails()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = ValidPlan();
	plan.grid.rows[1] = "#Z..#";

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult result =
		Validate(plan);

	Expect(!result.ok(), "unknown grid glyph should fail");
	Expect(result.unknownGridGlyphCount == 1, "unknown grid glyph should be counted");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::UnknownGridGlyph), "unknown grid glyph should be reported");
	if (!result.issues.empty()) {
		const iggy::runtime::RuntimeGameplayAsciiSourcePlanIssue &issue =
			result.issues.back();
		Expect(issue.row == 1 && issue.column == 1 && issue.glyph == 'Z', "unknown grid glyph issue should preserve row/column/glyph");
	}
}

void TestAnnotatedCellIssuesFail()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = ValidPlan();
	plan.annotatedCells.push_back(plan.annotatedCells[0]);
	plan.annotatedCells[1].row = 7;
	plan.annotatedCells[1].column = 8;
	plan.annotatedCells.push_back(plan.annotatedCells[0]);
	plan.annotatedCells[2].cellId = Id("cell:mismatch");
	plan.annotatedCells[2].column = 2;
	plan.annotatedCells[2].glyph = 'A';
	plan.annotatedCells[2].markerId = {};
	plan.annotatedCells[2].profileId = {};

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult result =
		Validate(plan);

	Expect(!result.ok(), "invalid annotated cells should fail");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::DuplicateAnnotatedCellId), "duplicate annotated cell id should be reported");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AnnotatedCellOutOfBounds), "out-of-bounds annotated cell should be reported");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AnnotatedCellGlyphMismatch), "annotated cell glyph mismatch should be reported");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::EmptyActorMarkerId), "actor marker id issue should be reported");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::EmptyProfileMarkerId), "profile marker id issue should be reported");
	Expect(result.annotationIssueCount >= 5, "annotation issue count should include duplicate, out-of-bounds, mismatch, and ids");
}

void TestRegionIssuesFail()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = ValidPlan();
	plan.regions = {
		{ true, Id("region:bad-order"), 2, 0, 1, 4, {} },
		{ true, Id("region:oob"), 0, 0, 7, 9, {} },
	};

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult result =
		Validate(plan);

	Expect(!result.ok(), "invalid regions should fail");
	Expect(result.regionIssueCount == 2, "region issue count should include invalid bounds and out of bounds");
	Expect(result.issues[0].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::InvalidRegionBounds, "region issues should preserve region order when earlier checks pass");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::InvalidRegionBounds), "invalid region bounds should be reported");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::RegionOutOfBounds), "out-of-bounds region should be reported");
}

void TestUnsafeNoClaimsAndPromotionPolicyFail()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = ValidPlan();
	plan.noClaims.claimsRuntimeTruth = true;
	plan.promotionPolicy.allowsProfileScenarioConversion = true;

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult result =
		Validate(plan);

	Expect(!result.ok(), "unsafe no-claims and promotion flags should fail");
	Expect(result.unsafeBoundaryIssueCount == 2, "unsafe boundary flags should be counted");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::UnsafeNoClaims), "unsafe no-claim should be reported");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::UnsafePromotionPolicy), "unsafe promotion policy should be reported");
}

void TestExactIdsAndInputImmutability()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = ValidPlan();
	plan.sourceId = Id("source:namespaced");
	plan.annotatedCells[0].markerId = Id("plain-actor");
	plan.annotatedCells[0].profileId = Id("profile:namespaced");
	plan.regions[0].regionId = Id("plain-region");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlan before = plan;

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult result =
		Validate(plan);

	Expect(result.ok(), "exact id source plan should validate");
	Expect(result.plan.sourceId == Id("source:namespaced"), "validator should preserve exact namespaced source id");
	Expect(result.plan.annotatedCells[0].markerId == Id("plain-actor"), "validator should preserve exact unqualified actor id");
	Expect(result.plan.annotatedCells[0].profileId == Id("profile:namespaced"), "validator should preserve exact namespaced profile id");
	Expect(result.plan.regions[0].regionId == Id("plain-region"), "validator should preserve exact unqualified region id");
	Expect(plan.sourceId == before.sourceId, "validator should not mutate source id");
	Expect(plan.grid.rows == before.grid.rows, "validator should not mutate rows");
	Expect(plan.annotatedCells[0].markerId == before.annotatedCells[0].markerId, "validator should not mutate annotated cells");
	Expect(plan.regions[0].regionId == before.regions[0].regionId, "validator should not mutate regions");
}

} // namespace

int main()
{
	TestValidSourcePlanPasses();
	TestEmptyAndRaggedRowsFailDeterministically();
	TestLegendGlyphIssuesFail();
	TestUnknownGridGlyphFails();
	TestAnnotatedCellIssuesFail();
	TestRegionIssuesFail();
	TestUnsafeNoClaimsAndPromotionPolicyFail();
	TestExactIdsAndInputImmutability();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
