#include <cstdint>
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
	command.hasTargetTileX = true;
	command.hasTargetTileY = true;
	command.targetTile = { x, y };
	return command;
}

iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredPlayerCommand InteractCommand(
	const char *targetId,
	const char *frameId = nullptr)
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredPlayerCommand command;
	if (frameId != nullptr) {
		command.hasFrameId = true;
		command.frameId = Id(frameId);
	}
	command.command =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanPlayerCommandKind::Interact;
	command.targetId = Id(targetId);
	return command;
}

iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredPlayerCommand PickupCommand(
	const char *targetId,
	const char *frameId = nullptr)
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredPlayerCommand command;
	if (frameId != nullptr) {
		command.hasFrameId = true;
		command.frameId = Id(frameId);
	}
	command.command =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanPlayerCommandKind::Pickup;
	command.targetId = Id(targetId);
	return command;
}

iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredInteractionTarget InteractionTarget(
	const char *targetId,
	int x = 2,
	int y = 1)
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredInteractionTarget target;
	target.targetId = Id(targetId);
	target.kind =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanInteractionTargetKind::Usable;
	target.localTile = { true, x, y };
	target.radius = 1.5;
	target.enabled = true;
	return target;
}

iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredItemDrop ItemDrop(
	const char *dropId,
	const char *itemId = "item:key",
	int x = 2,
	int y = 1,
	std::uint32_t count = 1)
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredItemDrop drop;
	drop.dropId = Id(dropId);
	drop.itemId = Id(itemId);
	drop.count = count;
	drop.localTile = { true, x, y };
	drop.localPosition = { true, static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5 };
	drop.pickupRadius = 0.5;
	drop.enabled = true;
	drop.glyph = 'k';
	return drop;
}

iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredProfile Profile(
	const char *profileId,
	int strength = 10,
	int dexterity = 10,
	int constitution = 10,
	int intelligence = 10,
	int wisdom = 10,
	int charisma = 10)
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredProfile profile;
	profile.profileId = Id(profileId);
	profile.traits.strength = strength;
	profile.traits.dexterity = dexterity;
	profile.traits.constitution = constitution;
	profile.traits.intelligence = intelligence;
	profile.traits.wisdom = wisdom;
	profile.traits.charisma = charisma;
	return profile;
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
	Expect(result.authoredProfileCount == 0, "valid source plan should report zero authored profiles");
	Expect(result.plan.sourceId == Id("source:room-a"), "validator should preserve copied source plan");
}

void TestSourcePlanCompatibilityPolicy()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan accepted = ValidPlan();
	accepted.formatId = Id("iggy:ascii-source-plan");
	accepted.version = 1;
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult acceptedResult =
		Validate(accepted);
	Expect(acceptedResult.ok(), "current source-plan format and version should validate");
	Expect(acceptedResult.compatibilityIssueCount == 0, "accepted source-plan compatibility facts should not count issues");

	iggy::runtime::RuntimeGameplayAsciiSourcePlan unsupportedVersion = ValidPlan();
	unsupportedVersion.version = 2;
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult versionResult =
		Validate(unsupportedVersion);
	Expect(!versionResult.ok(), "unsupported source-plan version should fail");
	Expect(HasIssue(versionResult, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::UnsupportedVersion), "unsupported source-plan version should report issue");
	Expect(versionResult.compatibilityIssueCount == 1, "unsupported source-plan version should count one compatibility issue");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanIssue *versionIssue =
		FindIssue(versionResult, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::UnsupportedVersion);
	Expect(versionIssue != nullptr && versionIssue->index == 2 && versionIssue->firstIndex == 1, "unsupported version issue should preserve actual and supported versions");

	iggy::runtime::RuntimeGameplayAsciiSourcePlan unsupportedFormat = ValidPlan();
	unsupportedFormat.formatId = Id("iggy:other-source-plan");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult formatResult =
		Validate(unsupportedFormat);
	Expect(!formatResult.ok(), "unsupported source-plan format id should fail");
	Expect(HasIssue(formatResult, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::UnsupportedFormatId), "unsupported source-plan format id should report issue");
	Expect(formatResult.compatibilityIssueCount == 1, "unsupported source-plan format id should count one compatibility issue");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanIssue *formatIssue =
		FindIssue(formatResult, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::UnsupportedFormatId);
	Expect(formatIssue != nullptr && formatIssue->id == Id("iggy:other-source-plan"), "unsupported format issue should preserve format id");
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
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanIssue *noClaimsIssue =
		FindIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::UnsafeNoClaims);
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanIssue *promotionIssue =
		FindIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::UnsafePromotionPolicy);
	Expect(noClaimsIssue != nullptr && noClaimsIssue->index == 0, "unsafe no-claims issue should identify runtime_truth as first unsafe flag");
	Expect(promotionIssue != nullptr && promotionIssue->index == 3, "unsafe promotion issue should identify profile_scenario_conversion as first unsafe flag");
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

void TestValidAuthoredInteractionTargetPasses()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = ValidPlan();
	plan.authoredInteractionTargets = {
		InteractionTarget("plain-target", 2, 1),
	};

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult result =
		Validate(plan);

	Expect(result.ok(), "valid authored interaction target should validate");
	Expect(result.authoredInteractionTargetCount == 1, "valid authored interaction target should be counted");
	Expect(result.authoredInteractionTargetIssueCount == 0, "valid authored interaction target should have no issues");
	Expect(result.plan.authoredInteractionTargets[0].targetId == Id("plain-target"), "validator should preserve exact interaction target id");
	Expect(result.plan.authoredInteractionTargets[0].localTile.x == 2 && result.plan.authoredInteractionTargets[0].localTile.y == 1, "validator should preserve interaction target tile");
}

void TestAuthoredInteractionTargetIssuesFailInDeclarationOrder()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = ValidPlan();
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredInteractionTarget missingId =
		InteractionTarget("target:missing-id");
	missingId.targetId = {};
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredInteractionTarget duplicate =
		InteractionTarget("target:duplicate");
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredInteractionTarget duplicateAgain =
		InteractionTarget("target:duplicate", 3, 1);
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredInteractionTarget invalidPosition =
		InteractionTarget("target:oob", 8, 1);
	plan.authoredInteractionTargets = {
		missingId,
		duplicate,
		duplicateAgain,
		invalidPosition,
	};

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult result =
		Validate(plan);

	Expect(!result.ok(), "invalid authored interaction targets should fail validation");
	Expect(result.authoredInteractionTargetCount == 4, "authored interaction target count should preserve declarations");
	Expect(result.authoredInteractionTargetIssueCount == 3, "authored interaction target issues should be counted");
	Expect(result.issues[result.issues.size() - 3].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredInteractionTargetMissingId, "first authored interaction target issue should be missing id");
	Expect(result.issues[result.issues.size() - 2].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredInteractionTargetDuplicateId, "second authored interaction target issue should be duplicate id");
	Expect(result.issues[result.issues.size() - 1].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredInteractionTargetPositionOutOfBounds, "third authored interaction target issue should be invalid position");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanIssue *duplicateIssue =
		FindIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredInteractionTargetDuplicateId);
	Expect(duplicateIssue != nullptr && duplicateIssue->index == 2 && duplicateIssue->firstIndex == 1 && duplicateIssue->id == Id("target:duplicate"), "duplicate target issue should preserve target id and indexes");
}

void TestValidAuthoredProfilePasses()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = ValidPlan();
	plan.authoredProfiles = {
		Profile("plain-profile", 12, 11, 10, 9, 8, 7),
	};

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult result =
		Validate(plan);

	Expect(result.ok(), "valid authored profile should validate");
	Expect(result.authoredProfileCount == 1, "valid authored profile should be counted");
	Expect(result.authoredProfileIssueCount == 0, "valid authored profile should have no issues");
	Expect(result.plan.authoredProfiles[0].profileId == Id("plain-profile"), "validator should preserve exact authored profile id");
	Expect(result.plan.authoredProfiles[0].traits.strength == 12, "validator should preserve authored profile strength");
	Expect(result.plan.authoredProfiles[0].traits.charisma == 7, "validator should preserve authored profile charisma");
}

void TestAuthoredProfileIssuesFailInDeclarationOrder()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = ValidPlan();
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredProfile missingId =
		Profile("profile:missing-id");
	missingId.profileId = {};
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredProfile duplicate =
		Profile("profile:duplicate");
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredProfile duplicateAgain =
		Profile("profile:duplicate", 11);
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredProfile invalidTraits =
		Profile("profile:invalid", 21);
	plan.authoredProfiles = {
		missingId,
		duplicate,
		duplicateAgain,
		invalidTraits,
	};

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult result =
		Validate(plan);

	Expect(!result.ok(), "invalid authored profiles should fail validation");
	Expect(result.authoredProfileCount == 4, "authored profile count should preserve declarations");
	Expect(result.authoredProfileIssueCount == 3, "authored profile issues should be counted");
	Expect(result.issues[result.issues.size() - 3].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredProfileMissingId, "first authored profile issue should be missing id");
	Expect(result.issues[result.issues.size() - 2].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredProfileDuplicateId, "second authored profile issue should be duplicate id");
	Expect(result.issues[result.issues.size() - 1].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredProfileInvalidTraits, "third authored profile issue should be invalid traits");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanIssue *duplicateIssue =
		FindIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredProfileDuplicateId);
	Expect(duplicateIssue != nullptr && duplicateIssue->index == 2 && duplicateIssue->firstIndex == 1 && duplicateIssue->id == Id("profile:duplicate"), "duplicate profile issue should preserve profile id and indexes");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanIssue *traitIssue =
		FindIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredProfileInvalidTraits);
	Expect(traitIssue != nullptr && traitIssue->index == 3 && traitIssue->id == Id("profile:invalid"), "invalid trait issue should preserve profile id and index");
}

void TestValidAuthoredPlayerCommandPasses()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = ValidPlan();
	plan.authoredPlayerCommands = {
		MoveToTileCommand(3, 1, "frame:player-move"),
		InteractCommand("target:door", "frame:interact"),
		PickupCommand("target:pickup", "frame:pickup"),
	};

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult result =
		Validate(plan);

	Expect(result.ok(), "valid authored player command should validate");
	Expect(result.authoredPlayerCommandCount == 3, "valid authored player commands should be counted");
	Expect(result.authoredPlayerCommandIssueCount == 0, "valid authored player command should have no issues");
	Expect(result.plan.authoredPlayerCommands[0].frameId == Id("frame:player-move"), "validator should preserve authored player command frame id");
	Expect(result.plan.authoredPlayerCommands[0].targetTile.x == 3 && result.plan.authoredPlayerCommands[0].targetTile.y == 1, "validator should preserve authored player command target tile");
	Expect(result.plan.authoredPlayerCommands[1].command == iggy::runtime::RuntimeGameplayAsciiSourcePlanPlayerCommandKind::Interact, "validator should preserve authored interact command kind");
	Expect(result.plan.authoredPlayerCommands[1].targetId == Id("target:door"), "validator should preserve authored interact command target id");
	Expect(result.plan.authoredPlayerCommands[2].command == iggy::runtime::RuntimeGameplayAsciiSourcePlanPlayerCommandKind::Pickup, "validator should preserve authored pickup command kind");
	Expect(result.plan.authoredPlayerCommands[2].targetId == Id("target:pickup"), "validator should preserve authored pickup target id");
}

void TestValidAuthoredItemDropPasses()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = ValidPlan();
	plan.authoredItemDrops = {
		ItemDrop("plain-drop", "item:key", 3, 1, 2),
	};

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult result =
		Validate(plan);

	Expect(result.ok(), "valid authored item drop should validate");
	Expect(result.authoredItemDropCount == 1, "valid authored item drop should be counted");
	Expect(result.authoredItemDropIssueCount == 0, "valid authored item drop should have no issues");
	Expect(result.plan.authoredItemDrops[0].dropId == Id("plain-drop"), "validator should preserve exact drop id");
	Expect(result.plan.authoredItemDrops[0].itemId == Id("item:key"), "validator should preserve exact item id");
	Expect(result.plan.authoredItemDrops[0].localTile.x == 3 && result.plan.authoredItemDrops[0].localTile.y == 1, "validator should preserve item drop tile");
	Expect(result.plan.authoredItemDrops[0].count == 2, "validator should preserve item drop count");
}

void TestAuthoredItemDropIssuesFailInDeclarationOrder()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = ValidPlan();
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredItemDrop missingDropId =
		ItemDrop("drop:missing-id");
	missingDropId.dropId = {};
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredItemDrop missingItemId =
		ItemDrop("drop:missing-item");
	missingItemId.itemId = {};
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredItemDrop duplicate =
		ItemDrop("drop:duplicate");
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredItemDrop duplicateAgain =
		ItemDrop("drop:duplicate", "item:other", 3, 1);
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredItemDrop invalidCount =
		ItemDrop("drop:zero-count", "item:key", 2, 1, 0);
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredItemDrop invalidPosition =
		ItemDrop("drop:oob", "item:key", 8, 1);
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredItemDrop missingPosition =
		ItemDrop("drop:missing-position");
	missingPosition.localTile = {};
	missingPosition.localPosition = {};
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredItemDrop invalidRadius =
		ItemDrop("drop:bad-radius");
	invalidRadius.pickupRadius = -0.25;
	plan.authoredItemDrops = {
		missingDropId,
		missingItemId,
		duplicate,
		duplicateAgain,
		invalidCount,
		invalidPosition,
		missingPosition,
		invalidRadius,
	};

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult result =
		Validate(plan);

	Expect(!result.ok(), "invalid authored item drops should fail validation");
	Expect(result.authoredItemDropCount == 8, "authored item drop count should preserve declarations");
	Expect(result.authoredItemDropIssueCount == 7, "authored item drop issues should be counted");
	Expect(result.issues[result.issues.size() - 7].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredItemDropMissingDropId, "first authored item drop issue should be missing drop id");
	Expect(result.issues[result.issues.size() - 6].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredItemDropMissingItemId, "second authored item drop issue should be missing item id");
	Expect(result.issues[result.issues.size() - 5].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredItemDropDuplicateDropId, "third authored item drop issue should be duplicate drop id");
	Expect(result.issues[result.issues.size() - 4].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredItemDropInvalidCount, "fourth authored item drop issue should be invalid count");
	Expect(result.issues[result.issues.size() - 3].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredItemDropPositionOutOfBounds, "fifth authored item drop issue should be invalid position");
	Expect(result.issues[result.issues.size() - 2].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredItemDropMissingPosition, "sixth authored item drop issue should be missing position");
	Expect(result.issues[result.issues.size() - 1].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredItemDropInvalidPickupRadius, "seventh authored item drop issue should be invalid pickup radius");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanIssue *duplicateIssue =
		FindIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredItemDropDuplicateDropId);
	Expect(duplicateIssue != nullptr && duplicateIssue->index == 3 && duplicateIssue->firstIndex == 2 && duplicateIssue->id == Id("drop:duplicate"), "duplicate drop issue should preserve drop id and indexes");
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
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredPlayerCommand missingInteractionTarget;
	missingInteractionTarget.command =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanPlayerCommandKind::Interact;
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredPlayerCommand missingPickupTarget;
	missingPickupTarget.command =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanPlayerCommandKind::Pickup;
	plan.authoredPlayerCommands = {
		unknown,
		missingTarget,
		missingInteractionTarget,
		missingPickupTarget,
	};

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult result =
		Validate(plan);

	Expect(!result.ok(), "invalid authored player commands should fail validation");
	Expect(result.authoredPlayerCommandCount == 4, "authored player command count should preserve declarations");
	Expect(result.authoredPlayerCommandIssueCount == 4, "authored player command issues should be counted");
	Expect(result.issues[result.issues.size() - 4].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredPlayerCommandUnsupportedCommand, "first authored player command issue should be unsupported command");
	Expect(result.issues[result.issues.size() - 3].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredPlayerCommandMissingTarget, "second authored player command issue should be missing move target");
	Expect(result.issues[result.issues.size() - 2].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredPlayerCommandMissingTarget, "third authored player command issue should be missing interact target");
	Expect(result.issues[result.issues.size() - 1].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredPlayerCommandMissingTarget, "fourth authored player command issue should be missing pickup target");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanIssue *targetIssue =
		FindIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredPlayerCommandMissingTarget);
	Expect(targetIssue != nullptr && targetIssue->index == 1, "missing player command target issue should preserve command index");
}

void TestAuthoredPlayerCommandTargetReferencesValidateWhenAuthoredTargetsExist()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = ValidPlan();
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredInteractionTarget pickup =
		InteractionTarget("target:key");
	pickup.kind =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanInteractionTargetKind::Pickup;
	plan.authoredInteractionTargets = {
		InteractionTarget("target:door"),
		pickup,
	};
	plan.authoredItemDrops = {
		ItemDrop("drop:key"),
	};
	plan.authoredPlayerCommands = {
		InteractCommand("target:door"),
		PickupCommand("target:key"),
		PickupCommand("drop:key"),
	};

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult result =
		Validate(plan);

	Expect(result.ok(), "player command authored target references should validate when ids exist");
	Expect(result.authoredPlayerCommandCount == 3, "valid target references should preserve command count");
	Expect(result.authoredPlayerCommandIssueCount == 0, "valid target references should not add player command issues");
}

void TestAuthoredPlayerCommandUnknownReferencesFailWhenAuthoredTargetsExist()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = ValidPlan();
	plan.authoredInteractionTargets = {
		InteractionTarget("target:door"),
	};
	plan.authoredItemDrops = {
		ItemDrop("drop:key"),
	};
	plan.authoredPlayerCommands = {
		InteractCommand("target:missing"),
		PickupCommand("target:door"),
		PickupCommand("drop:missing"),
	};

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult result =
		Validate(plan);

	Expect(!result.ok(), "unknown player command target references should fail validation");
	Expect(result.authoredPlayerCommandIssueCount == 3, "unknown target references should count player command issues");
	Expect(result.issues[result.issues.size() - 3].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredPlayerCommandUnknownInteractionTarget, "first cross-reference issue should be unknown interaction target");
	Expect(result.issues[result.issues.size() - 2].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredPlayerCommandInvalidPickupTarget, "second cross-reference issue should be invalid pickup target type");
	Expect(result.issues[result.issues.size() - 1].code == iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredPlayerCommandInvalidPickupTarget, "third cross-reference issue should be unknown pickup target");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanIssue *interactIssue =
		FindIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredPlayerCommandUnknownInteractionTarget);
	Expect(interactIssue != nullptr && interactIssue->index == 0 && interactIssue->id == Id("target:missing"), "unknown interact target issue should preserve command index and target id");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanIssue *pickupIssue =
		FindIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredPlayerCommandInvalidPickupTarget);
	Expect(pickupIssue != nullptr && pickupIssue->index == 1 && pickupIssue->id == Id("target:door"), "invalid pickup target issue should preserve command index and target id");
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

void TestExpectationShapeValidates()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = ValidPlan();
	plan.expectations.hasFinalRows = true;
	plan.expectations.finalRows = {
		"#####",
		"#.A.#",
		"#####",
	};
	plan.expectations.hasFrameCount = true;
	plan.expectations.frameCount = 1;
	plan.expectations.hasAcceptedCommandCount = true;
	plan.expectations.acceptedCommandCount = 0;
	plan.expectations.hasPickedUpCount = true;
	plan.expectations.pickedUpCount = 0;
	plan.expectations.hasInteractionChanged = true;
	plan.expectations.interactionChanged = false;
	plan.expectations.hasNpcMovedCount = true;
	plan.expectations.npcMovedCount = 1;
	iggy::runtime::RuntimeGameplayAsciiSourcePlanExpectedTraceFrame frame;
	frame.hasFrameId = true;
	frame.frameId = Id("frame:move");
	frame.hasRows = true;
	frame.rows = {
		"#####",
		"#A..#",
		"#####",
	};
	frame.hasNpcMovedCount = true;
	frame.npcMovedCount = 1;
	plan.expectations.traceFrames.push_back(frame);
	plan.expectations.inventoryStacks.push_back({ Id("item:key"), 1 });
	plan.authoredInteractionTargets = {
		InteractionTarget("target:door"),
	};
	plan.expectations.interactionTargets.push_back({ Id("target:door"), false });

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult result =
		Validate(plan);

	Expect(result.ok(), "well-shaped expectations should validate");
	Expect(result.expectationIssueCount == 0, "well-shaped expectations should have no expectation issues");
	Expect(result.plan.expectations.hasFinalRows, "validator should preserve expectation final rows");
	Expect(result.plan.expectations.finalRows[1] == "#.A.#", "validator should preserve expectation row text");
	Expect(result.plan.expectations.hasNpcMovedCount && result.plan.expectations.npcMovedCount == 1, "validator should preserve expectation counts");
	Expect(result.plan.expectations.traceFrames.size() == 1, "validator should preserve trace expectations");
	Expect(result.plan.expectations.traceFrames[0].rows[1] == "#A..#", "validator should preserve trace expectation rows");
	Expect(result.plan.expectations.inventoryStacks.size() == 1, "validator should preserve inventory expectations");
	Expect(result.plan.expectations.inventoryStacks[0].itemId == Id("item:key"), "validator should preserve inventory expected item id");
	Expect(result.plan.expectations.interactionTargets.size() == 1, "validator should preserve interaction expectations");
	Expect(result.plan.expectations.interactionTargets[0].targetId == Id("target:door"), "validator should preserve interaction expected target id");
}

void TestExpectationShapeIssuesFail()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan emptyRows = ValidPlan();
	emptyRows.expectations.hasFinalRows = true;
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult emptyResult =
		Validate(emptyRows);

	Expect(!emptyResult.ok(), "empty expected final rows should fail");
	Expect(emptyResult.expectationIssueCount == 1, "empty expected final rows should count one expectation issue");
	Expect(HasIssue(emptyResult, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedFinalRowsEmpty), "empty expected final rows should report issue");

	iggy::runtime::RuntimeGameplayAsciiSourcePlan wrongWidth = ValidPlan();
	wrongWidth.expectations.hasFinalRows = true;
	wrongWidth.expectations.finalRows = {
		"#####",
		"#A#",
		"#####",
	};
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult widthResult =
		Validate(wrongWidth);

	Expect(!widthResult.ok(), "wrong-width expected final rows should fail");
	Expect(widthResult.expectationIssueCount == 1, "wrong-width expected final rows should count one expectation issue");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanIssue *issue =
		FindIssue(widthResult, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedFinalRowsDimensionMismatch);
	Expect(issue != nullptr && issue->row == 1 && issue->index == 3 && issue->firstIndex == 5, "wrong-width expectation issue should preserve row and widths");

	iggy::runtime::RuntimeGameplayAsciiSourcePlan wrongTraceWidth = ValidPlan();
	iggy::runtime::RuntimeGameplayAsciiSourcePlanExpectedTraceFrame frame;
	frame.hasRows = true;
	frame.rows = {
		"#####",
		"#A#",
		"#####",
	};
	wrongTraceWidth.expectations.traceFrames.push_back(frame);
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult traceResult =
		Validate(wrongTraceWidth);

	Expect(!traceResult.ok(), "wrong-width expected trace rows should fail");
	Expect(traceResult.expectationIssueCount == 1, "wrong-width expected trace rows should count one expectation issue");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanIssue *traceIssue =
		FindIssue(traceResult, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedTraceFrameRowsDimensionMismatch);
	Expect(traceIssue != nullptr && traceIssue->index == 0 && traceIssue->row == 1 && traceIssue->column == 3 && traceIssue->firstIndex == 5, "wrong-width trace expectation issue should preserve frame, row, and widths");

	iggy::runtime::RuntimeGameplayAsciiSourcePlan missingItem = ValidPlan();
	missingItem.expectations.inventoryStacks.push_back({});
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult missingResult =
		Validate(missingItem);
	Expect(!missingResult.ok(), "missing expected inventory item id should fail");
	Expect(HasIssue(missingResult, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedInventoryStackMissingItemId), "missing expected inventory item id should report issue");
	Expect(HasIssue(missingResult, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedInventoryStackInvalidCount), "missing expected inventory count should report invalid count");

	iggy::runtime::RuntimeGameplayAsciiSourcePlan duplicateItem = ValidPlan();
	duplicateItem.expectations.inventoryStacks.push_back({ Id("item:key"), 1 });
	duplicateItem.expectations.inventoryStacks.push_back({ Id("item:key"), 2 });
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult duplicateResult =
		Validate(duplicateItem);
	Expect(!duplicateResult.ok(), "duplicate expected inventory item ids should fail");
	Expect(HasIssue(duplicateResult, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedInventoryStackDuplicateItemId), "duplicate expected inventory item id should report issue");

	iggy::runtime::RuntimeGameplayAsciiSourcePlan unknownTarget = ValidPlan();
	unknownTarget.authoredInteractionTargets = {
		InteractionTarget("target:known"),
	};
	unknownTarget.expectations.interactionTargets.push_back({ Id("target:missing"), false });
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult unknownResult =
		Validate(unknownTarget);
	Expect(!unknownResult.ok(), "unknown expected interaction target should fail");
	Expect(HasIssue(unknownResult, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedInteractionTargetUnknownId), "unknown expected interaction target should report issue");

	iggy::runtime::RuntimeGameplayAsciiSourcePlan duplicateTarget = ValidPlan();
	duplicateTarget.authoredInteractionTargets = {
		InteractionTarget("target:door"),
	};
	duplicateTarget.expectations.interactionTargets.push_back({ Id("target:door"), false });
	duplicateTarget.expectations.interactionTargets.push_back({ Id("target:door"), true });
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult duplicateTargetResult =
		Validate(duplicateTarget);
	Expect(!duplicateTargetResult.ok(), "duplicate expected interaction targets should fail");
	Expect(HasIssue(duplicateTargetResult, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedInteractionTargetDuplicateId), "duplicate expected interaction target should report issue");

	iggy::runtime::RuntimeGameplayAsciiSourcePlan validActorState = ValidPlan();
	validActorState.expectations.actorStates.push_back(
		{ Id("npc:guard"), { 2, 1 }, true });
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult validActorStateResult =
		Validate(validActorState);
	Expect(validActorStateResult.ok(), "known expected actor state with tile should validate");

	iggy::runtime::RuntimeGameplayAsciiSourcePlan missingActorId = ValidPlan();
	missingActorId.expectations.actorStates.push_back(
		{ {}, { 2, 1 }, true });
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult missingActorIdResult =
		Validate(missingActorId);
	Expect(!missingActorIdResult.ok(), "expected actor state without actor id should fail");
	Expect(HasIssue(missingActorIdResult, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedActorStateMissingId), "missing expected actor id should report issue");

	iggy::runtime::RuntimeGameplayAsciiSourcePlan unknownActorState = ValidPlan();
	unknownActorState.expectations.actorStates.push_back(
		{ Id("npc:missing"), { 2, 1 }, true });
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult unknownActorStateResult =
		Validate(unknownActorState);
	Expect(!unknownActorStateResult.ok(), "unknown expected actor state should fail");
	Expect(HasIssue(unknownActorStateResult, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedActorStateUnknownId), "unknown expected actor state should report issue");

	iggy::runtime::RuntimeGameplayAsciiSourcePlan duplicateActorState = ValidPlan();
	duplicateActorState.expectations.actorStates.push_back(
		{ Id("npc:guard"), { 2, 1 }, true });
	duplicateActorState.expectations.actorStates.push_back(
		{ Id("npc:guard"), { 3, 1 }, true });
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult duplicateActorStateResult =
		Validate(duplicateActorState);
	Expect(!duplicateActorStateResult.ok(), "duplicate expected actor states should fail");
	Expect(HasIssue(duplicateActorStateResult, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedActorStateDuplicateId), "duplicate expected actor state should report issue");

	iggy::runtime::RuntimeGameplayAsciiSourcePlan missingActorTile = ValidPlan();
	missingActorTile.expectations.actorStates.push_back(
		{ Id("npc:guard"), {}, false });
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult missingActorTileResult =
		Validate(missingActorTile);
	Expect(!missingActorTileResult.ok(), "expected actor state without tile should fail");
	Expect(HasIssue(missingActorTileResult, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedActorStateMissingTile), "missing expected actor tile should report issue");

	iggy::runtime::RuntimeGameplayAsciiSourcePlan missingPlayer = ValidPlan();
	missingPlayer.expectations.hasPlayerState = true;
	missingPlayer.expectations.playerState = { { 2, 1 }, true };
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult missingPlayerResult =
		Validate(missingPlayer);
	Expect(!missingPlayerResult.ok(), "expected player state without player start should fail");
	Expect(HasIssue(missingPlayerResult, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedPlayerStateMissingPlayer), "missing expected player should report issue");

	iggy::runtime::RuntimeGameplayAsciiSourcePlan validPlayerState = ValidPlan();
	validPlayerState.grid.rows[1] = "#A.@#";
	validPlayerState.legend.push_back(
		{
			'@',
			iggy::runtime::RuntimeGameplayAsciiSourcePlanGlyphKind::PlayerStart,
			Id("role:player"),
			{},
			true,
			iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::PlayerStart,
			Id("player:one"),
			{},
		});
	validPlayerState.expectations.hasPlayerState = true;
	validPlayerState.expectations.playerState = { { 2, 1 }, true };
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult validPlayerStateResult =
		Validate(validPlayerState);
	Expect(validPlayerStateResult.ok(), "expected player state with player start should validate");

	iggy::runtime::RuntimeGameplayAsciiSourcePlan missingPlayerTile = validPlayerState;
	missingPlayerTile.expectations.playerState = {};
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult missingPlayerTileResult =
		Validate(missingPlayerTile);
	Expect(!missingPlayerTileResult.ok(), "expected player state without tile should fail");
	Expect(HasIssue(missingPlayerTileResult, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedPlayerStateMissingTile), "missing expected player tile should report issue");
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
	plan.authoredProfiles = {
		Profile("plain-profile"),
	};
	plan.authoredInteractionTargets = {
		InteractionTarget("plain-target"),
	};
	plan.authoredItemDrops = {
		ItemDrop("plain-drop", "item:namespaced"),
	};
	plan.authoredPlayerCommands = {
		MoveToTileCommand(3, 1, "plain-frame"),
	};
	plan.expectations.hasFinalRows = true;
	plan.expectations.finalRows = {
		"#####",
		"#A..#",
		"#####",
	};
	const iggy::runtime::RuntimeGameplayAsciiSourcePlan before = plan;

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult result =
		Validate(plan);

	Expect(result.ok(), "exact id source plan should validate");
	Expect(result.plan.sourceId == Id("source:namespaced"), "validator should preserve exact namespaced source id");
	Expect(result.plan.annotatedCells[0].markerId == Id("plain-actor"), "validator should preserve exact unqualified actor id");
	Expect(result.plan.annotatedCells[0].profileId == Id("profile:namespaced"), "validator should preserve exact namespaced profile id");
	Expect(result.plan.regions[0].regionId == Id("plain-region"), "validator should preserve exact unqualified region id");
	Expect(result.plan.authoredProfiles[0].profileId == Id("plain-profile"), "validator should preserve exact authored profile id");
	Expect(result.plan.authoredInteractionTargets[0].targetId == Id("plain-target"), "validator should preserve exact authored interaction target id");
	Expect(result.plan.authoredItemDrops[0].dropId == Id("plain-drop"), "validator should preserve exact authored item drop id");
	Expect(result.plan.authoredItemDrops[0].itemId == Id("item:namespaced"), "validator should preserve exact authored item id");
	Expect(plan.sourceId == before.sourceId, "validator should not mutate source id");
	Expect(plan.grid.rows == before.grid.rows, "validator should not mutate rows");
	Expect(plan.annotatedCells[0].markerId == before.annotatedCells[0].markerId, "validator should not mutate annotated cells");
	Expect(plan.regions[0].regionId == before.regions[0].regionId, "validator should not mutate regions");
	Expect(plan.authoredControls[0].npcId == before.authoredControls[0].npcId, "validator should not mutate authored controls");
	Expect(plan.authoredProfiles[0].profileId == before.authoredProfiles[0].profileId, "validator should not mutate authored profiles");
	Expect(plan.authoredInteractionTargets[0].targetId == before.authoredInteractionTargets[0].targetId, "validator should not mutate authored interaction targets");
	Expect(plan.authoredItemDrops[0].dropId == before.authoredItemDrops[0].dropId, "validator should not mutate authored item drops");
	Expect(plan.authoredPlayerCommands[0].frameId == before.authoredPlayerCommands[0].frameId, "validator should not mutate authored player commands");
	Expect(plan.expectations.finalRows == before.expectations.finalRows, "validator should not mutate expectations");
}

} // namespace

int main()
{
	TestValidSourcePlanPasses();
	TestSourcePlanCompatibilityPolicy();
	TestEmptyAndRaggedRowsFailDeterministically();
	TestLegendGlyphIssuesFail();
	TestUnknownGridGlyphFails();
	TestAnnotatedCellIssuesFail();
	TestRegionIssuesFail();
	TestUnsafeNoClaimsAndPromotionPolicyFail();
	TestValidAuthoredControlPasses();
	TestAuthoredControlIssuesFailInDeclarationOrder();
	TestValidAuthoredProfilePasses();
	TestAuthoredProfileIssuesFailInDeclarationOrder();
	TestValidAuthoredInteractionTargetPasses();
	TestAuthoredInteractionTargetIssuesFailInDeclarationOrder();
	TestValidAuthoredPlayerCommandPasses();
	TestValidAuthoredItemDropPasses();
	TestAuthoredItemDropIssuesFailInDeclarationOrder();
	TestAuthoredPlayerCommandIssuesFailInDeclarationOrder();
	TestAuthoredPlayerCommandTargetReferencesValidateWhenAuthoredTargetsExist();
	TestAuthoredPlayerCommandUnknownReferencesFailWhenAuthoredTargetsExist();
	TestMultiplePlayerCommandsPerFrameArePreserved();
	TestExpectationShapeValidates();
	TestExpectationShapeIssuesFail();
	TestExactIdsAndInputImmutability();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
