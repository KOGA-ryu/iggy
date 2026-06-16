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

const iggy::runtime::RuntimeGameplayAsciiSourcePlanIssue *FindIssue(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult &result,
	iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode code)
{
	for (const iggy::runtime::RuntimeGameplayAsciiSourcePlanIssue &issue : result.issues) {
		if (issue.code == code) {
			return &issue;
		}
	}
	return nullptr;
}

iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredPlayerCommand MoveToTileCommand(
	int x,
	int y,
	const char *frameId = nullptr)
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredPlayerCommand command;
	if (frameId != nullptr) {
		command.hasFrameId = true;
		command.frameId = Id(frameId);
	}
	command.command =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanPlayerCommandKind::MoveToTile;
	command.hasTargetTile = true;
	command.targetTile = { x, y };
	return command;
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

void TestValidAuthoredControlPasses()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = ValidPlan();
	plan.authoredControls = {
		{ true, Id("frame:one"), Id("npc:guard"), iggy::runtime::RuntimeGameplayAsciiSourcePlanControlBehavior::Seeking, iggy::runtime::RuntimeGameplayAsciiSourcePlanControlMoveMode::Walk, { true, 2.5, 1.5 } },
	};

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult result =
		Validate(plan);

	Expect(result.ok(), "valid authored control should validate");
	Expect(result.authoredControlCount == 1, "valid authored control should be counted");
	Expect(result.authoredPlayerCommandCount == 0, "valid authored control test should report zero authored player commands");
	Expect(result.authoredControlIssueCount == 0, "valid authored control should have no control issues");
	Expect(result.plan.authoredControls[0].npcId == Id("npc:guard"), "validator should preserve exact authored control npc id");
	Expect(result.plan.authoredControls[0].targetPosition.x == 2.5, "validator should preserve authored control target position");
}

void TestAuthoredControlIssuesFailInDeclarationOrder()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = ValidPlan();
	plan.authoredControls = {
		{ false, {}, {}, iggy::runtime::RuntimeGameplayAsciiSourcePlanControlBehavior::Seeking, iggy::runtime::RuntimeGameplayAsciiSourcePlanControlMoveMode::Walk, { true, 2.5, 1.5 } },
		{ false, {}, Id("npc:bad-behavior"), iggy::runtime::RuntimeGameplayAsciiSourcePlanControlBehavior::Unknown, iggy::runtime::RuntimeGameplayAsciiSourcePlanControlMoveMode::Walk, { true, 2.5, 1.5 } },
		{ false, {}, Id("npc:bad-mode"), iggy::runtime::RuntimeGameplayAsciiSourcePlanControlBehavior::Waiting, iggy::runtime::RuntimeGameplayAsciiSourcePlanControlMoveMode::Unknown, {} },
		{ false, {}, Id("npc:missing-target"), iggy::runtime::RuntimeGameplayAsciiSourcePlanControlBehavior::Seeking, iggy::runtime::RuntimeGameplayAsciiSourcePlanControlMoveMode::Walk, {} },
	};

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult result =
		Validate(plan);

	Expect(!result.ok(), "invalid authored controls should fail validation");
	Expect(result.authoredControlCount == 4, "authored control count should preserve declarations");
	Expect(result.authoredControlIssueCount == 4, "authored control issue count should include all control issues");
	Expect(result.issues[result.issues.size() - 4].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredControlMissingNpcId, "first authored control issue should be missing npc id");
	Expect(result.issues[result.issues.size() - 3].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredControlUnsupportedBehavior, "second authored control issue should be unsupported behavior");
	Expect(result.issues[result.issues.size() - 2].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredControlUnsupportedMoveMode, "third authored control issue should be unsupported move mode");
	Expect(result.issues[result.issues.size() - 1].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredControlMissingTarget, "fourth authored control issue should be missing target");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanIssue *targetIssue =
		FindIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredControlMissingTarget);
	Expect(targetIssue != nullptr && targetIssue->index == 3 && targetIssue->id == Id("npc:missing-target"), "missing target issue should preserve control index and npc id");
}

void TestValidAuthoredPlayerCommandPasses()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = ValidPlan();
	plan.authoredPlayerCommands = {
		MoveToTileCommand(3, 1, "frame:player-move"),
	};

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult result =
		Validate(plan);

	Expect(result.ok(), "valid authored player command should validate");
	Expect(result.authoredPlayerCommandCount == 1, "valid authored player command should be counted");
	Expect(result.authoredPlayerCommandIssueCount == 0, "valid authored player command should have no issues");
	Expect(result.plan.authoredPlayerCommands[0].frameId == Id("frame:player-move"), "validator should preserve authored player command frame id");
	Expect(result.plan.authoredPlayerCommands[0].targetTile.x == 3 && result.plan.authoredPlayerCommands[0].targetTile.y == 1, "validator should preserve authored player command target tile");
}

void TestAuthoredPlayerCommandIssuesFailInDeclarationOrder()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = ValidPlan();
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredPlayerCommand unknown;
	unknown.command =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanPlayerCommandKind::Unknown;
	unknown.hasTargetTile = true;
	unknown.targetTile = { 3, 1 };
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredPlayerCommand missingTarget;
	missingTarget.command =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanPlayerCommandKind::MoveToTile;
	plan.authoredPlayerCommands = {
		unknown,
		missingTarget,
	};

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult result =
		Validate(plan);

	Expect(!result.ok(), "invalid authored player commands should fail validation");
	Expect(result.authoredPlayerCommandCount == 2, "authored player command count should preserve declarations");
	Expect(result.authoredPlayerCommandIssueCount == 2, "authored player command issues should be counted");
	Expect(result.issues[result.issues.size() - 2].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredPlayerCommandUnsupportedCommand, "first authored player command issue should be unsupported command");
	Expect(result.issues[result.issues.size() - 1].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredPlayerCommandMissingTarget, "second authored player command issue should be missing target");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanIssue *targetIssue =
		FindIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredPlayerCommandMissingTarget);
	Expect(targetIssue != nullptr && targetIssue->index == 1, "missing player command target issue should preserve command index");
}

void TestMultiplePlayerCommandsPerFrameArePreserved()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = ValidPlan();
	plan.authoredPlayerCommands = {
		MoveToTileCommand(2, 1, "frame:shared"),
		MoveToTileCommand(3, 1, "frame:shared"),
		MoveToTileCommand(4, 1, "frame:next"),
	};

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult result =
		Validate(plan);

	Expect(result.ok(), "multiple authored player commands should validate because runtime frame input accepts multiple intents");
	Expect(result.authoredPlayerCommandCount == 3, "validator should preserve player command count");
	Expect(result.plan.authoredPlayerCommands[0].frameId == Id("frame:shared"), "first same-frame command should preserve frame id");
	Expect(result.plan.authoredPlayerCommands[1].targetTile.x == 3, "second same-frame command should preserve order");
	Expect(result.plan.authoredPlayerCommands[2].frameId == Id("frame:next"), "different-frame command should preserve frame id");
}

void TestExactIdsAndInputImmutability()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = ValidPlan();
	plan.sourceId = Id("source:namespaced");
	plan.annotatedCells[0].markerId = Id("plain-actor");
	plan.annotatedCells[0].profileId = Id("profile:namespaced");
	plan.regions[0].regionId = Id("plain-region");
	plan.authoredControls = {
		{ false, {}, Id("plain-actor"), iggy::runtime::RuntimeGameplayAsciiSourcePlanControlBehavior::Waiting, iggy::runtime::RuntimeGameplayAsciiSourcePlanControlMoveMode::Still, {} },
	};
	plan.authoredPlayerCommands = {
		MoveToTileCommand(3, 1, "plain-frame"),
	};
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
	Expect(plan.authoredControls[0].npcId == before.authoredControls[0].npcId, "validator should not mutate authored controls");
	Expect(plan.authoredPlayerCommands[0].frameId == before.authoredPlayerCommands[0].frameId, "validator should not mutate authored player commands");
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
	TestValidAuthoredControlPasses();
	TestAuthoredControlIssuesFailInDeclarationOrder();
	TestValidAuthoredPlayerCommandPasses();
	TestAuthoredPlayerCommandIssuesFailInDeclarationOrder();
	TestMultiplePlayerCommandsPerFrameArePreserved();
	TestExactIdsAndInputImmutability();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
