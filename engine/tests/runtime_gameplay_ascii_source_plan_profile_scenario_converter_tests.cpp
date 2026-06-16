#include <cstdlib>
#include <string_view>
#include <vector>

#include "runtime/RuntimeGameplayAsciiSourcePlanProfileScenarioConverter.hpp"
#include "runtime/RuntimeGameplayProfileScenarioRunner.hpp"
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

iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPolicy AiMapPolicy(
	const char *roleTag,
	std::vector<iggy::ResourceId> nodeTags = { Id("tag:patrol") })
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlanRegionAiMapPolicy policy;
	policy.roleTag = Id(roleTag);
	policy.nodeTags = nodeTags;
	policy.patrolWeight = 1.0F;
	return policy;
}

iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredControl SeekingControl(
	const char *npcId = "npc:guard",
	iggy::Vec2 target = { 2.5F, 1.5F },
	const char *frameId = nullptr)
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredControl control;
	control.hasFrameId = frameId != nullptr;
	if (frameId != nullptr)
		control.frameId = Id(frameId);
	control.npcId = Id(npcId);
	control.behavior = iggy::runtime::RuntimeGameplayAsciiSourcePlanControlBehavior::Seeking;
	control.moveMode = iggy::runtime::RuntimeGameplayAsciiSourcePlanControlMoveMode::Walk;
	control.targetPosition = { true, target.x, target.y };
	return control;
}

iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredPlayerCommand MovePlayerToTile(
	int x,
	int y,
	const char *frameId = nullptr,
	bool hasDeclarationIndex = false,
	std::size_t declarationIndex = 0)
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredPlayerCommand command;
	command.hasFrameId = frameId != nullptr;
	if (frameId != nullptr)
		command.frameId = Id(frameId);
	command.command =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanPlayerCommandKind::MoveToTile;
	command.hasTargetTile = true;
	command.hasTargetTileX = true;
	command.hasTargetTileY = true;
	command.targetTile = { x, y };
	command.hasDeclarationIndex = hasDeclarationIndex;
	command.declarationIndex = declarationIndex;
	return command;
}

iggy::NpcTraitSet Traits()
{
	iggy::NpcTraitSet traits;
	traits.strength = 10;
	return traits;
}

iggy::NpcAiProfileTraitCatalog Catalog(std::vector<iggy::NpcAiProfileTraitEntry> entries)
{
	const iggy::NpcAiProfileTraitCatalogBuildResult result =
		iggy::NpcAiProfileTraitCatalogBuilder {}.build(entries);
	Expect(result.built, "converter profile catalog should build");
	return result.catalog;
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
	Expect(result.definition.initialState.npcActors.actors.empty(), "invalid source plan should not publish actors to run");
	Expect(result.definition.initialState.npcControls.entries.empty(), "invalid source plan should not publish controls to run");
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
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config =
		ConfigWithFrame();
	config.profileTraits = Catalog({ { Id("profile:guard"), Traits() } });

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
	Expect(result.definition.frames[0].movementMap.id == Id("scenario:ascii-profile"), "converted definition should use promoted map as frame movement map");
	Expect(plan.grid.rows == before.grid.rows, "converter should not mutate source plan");
}

void TestTerrainActorAndDefaultControlPromotion()
{
	const iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config =
		ConfigWithFrame();
	config.profileTraits = Catalog({ { Id("profile:guard"), Traits() } });
	config.defaultFactionId = Id("faction:ascii");
	config.defaultGoalId = Id("goal:ascii");

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult result =
		Convert(plan, config);

	Expect(result.ok(), "valid source plan with profile catalog should convert");
	Expect(result.promotedMap.width == 5 && result.promotedMap.height == 3, "promoted map should preserve dimensions");
	Expect(result.promotedMap.tileAt(0, 0) != nullptr && !result.promotedMap.tileAt(0, 0)->walkable, "hash terrain should promote as blocked");
	Expect(result.promotedMap.tileAt(2, 1) != nullptr && result.promotedMap.tileAt(2, 1)->walkable, "floor terrain should promote as walkable");
	Expect(result.promotedMap.tileAt(1, 1) != nullptr && result.promotedMap.tileAt(1, 1)->walkable, "actor glyph should promote as walkable floor");
	Expect(result.promotedActorCount == 1 && result.defaultControlCount == 1, "one actor should create one default control");
	Expect(result.definition.initialState.session.level.map.width == 5, "initial state should receive promoted map");
	Expect(result.definition.frames.size() == 1 && result.definition.frames[0].movementMap.width == 5, "frame should receive promoted movement map");
	Expect(result.definition.initialState.npcActors.actors.size() == 1, "initial state should receive promoted actor registry");
	Expect(result.definition.initialState.npcControls.entries.size() == 1, "initial state should receive promoted control registry");
	const iggy::NpcActorState2D &actor = result.definition.initialState.npcActors.actors[0];
	Expect(actor.npcId == Id("npc:guard"), "actor marker id should become npc id");
	Expect(actor.aiProfileId == Id("profile:guard"), "actor profile id should become ai profile id");
	Expect(actor.factionId == Id("faction:ascii"), "actor faction should come from config default");
	Expect(actor.currentGoalId == Id("goal:ascii"), "actor current goal should come from config default");
	Expect(actor.position.x == 1.5F && actor.position.y == 1.5F, "actor should preserve local position when present");
	const iggy::NpcActorControlState2D &control = result.definition.initialState.npcControls.entries[0];
	Expect(control.npcId == Id("npc:guard"), "default control should use promoted npc id");
	Expect(control.objective.type == iggy::NpcObjectiveType::Wait, "default control should wait");
	Expect(control.behavior.type == iggy::NpcBehaviorStateType::Waiting, "default control should use waiting behavior");
	Expect(control.moveMode == iggy::NpcMoveMode::Still, "default control should use still move mode");
}

void TestAuthoredControlReplacesDefaultControlForPromotedActor()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.authoredControls = { SeekingControl() };
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config =
		ConfigWithFrame();
	config.profileTraits = Catalog({ { Id("profile:guard"), Traits() } });

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult result =
		Convert(plan, config);

	Expect(result.ok(), "authored control for promoted actor should convert");
	Expect(result.authoredControlCount == 1, "authored control should be counted");
	Expect(result.authoredControlIssueCount == 0, "valid authored control should have no conversion issues");
	Expect(result.definition.initialState.npcControls.entries.size() == 1, "authored control conversion should publish one control");
	const iggy::NpcActorControlState2D &control =
		result.definition.initialState.npcControls.entries[0];
	Expect(control.npcId == Id("npc:guard"), "authored control should preserve npc id");
	Expect(control.objective.type == iggy::NpcObjectiveType::MoveTo, "authored seeking control should promote move-to objective");
	Expect(control.objective.targetPosition.x == 2.5F && control.objective.targetPosition.y == 1.5F, "authored seeking objective should preserve target");
	Expect(control.behavior.type == iggy::NpcBehaviorStateType::Seeking, "authored seeking control should promote seeking behavior");
	Expect(control.behavior.targetPosition.x == 2.5F && control.behavior.targetPosition.y == 1.5F, "authored seeking behavior should preserve target");
	Expect(control.moveMode == iggy::NpcMoveMode::Walk, "authored control should promote walk move mode");
	Expect(result.definition.frames.size() == 1 && result.definition.frames[0].controlOverrides.empty(), "no-frame authored control should not create frame overrides");
}

void TestFrameIdAuthoredControlsCreateScenarioFramesInFirstSeenOrder()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.authoredControls = {
		SeekingControl("npc:guard", { 2.5F, 1.5F }, "frame:move-one"),
		SeekingControl("npc:guard", { 3.5F, 1.5F }, "frame:move-two"),
	};
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config =
		ConfigWithFrame();
	config.profileTraits = Catalog({ { Id("profile:guard"), Traits() } });

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult result =
		Convert(plan, config);

	Expect(result.ok(), "frame-id authored controls should convert");
	Expect(result.definition.frames.size() == 2, "frame-id authored controls should create one frame per first-seen frame id");
	if (result.definition.frames.size() == 2) {
		Expect(result.definition.frames[0].hasFrameId && result.definition.frames[0].frameId == Id("frame:move-one"), "first frame-id group should preserve first frame id");
		Expect(result.definition.frames[1].hasFrameId && result.definition.frames[1].frameId == Id("frame:move-two"), "second frame-id group should preserve second frame id");
		Expect(result.definition.frames[0].controlOverrides.size() == 1, "first frame-id group should publish one override");
		Expect(result.definition.frames[1].controlOverrides.size() == 1, "second frame-id group should publish one override");
		Expect(result.definition.frames[0].controlOverrides[0].objective.targetPosition.x == 2.5F, "first frame-id group should preserve first target");
		Expect(result.definition.frames[1].controlOverrides[0].objective.targetPosition.x == 3.5F, "second frame-id group should preserve second target");
		Expect(result.definition.frames[0].movementMap.width == result.promotedMap.width, "first frame-id frame should use promoted movement map");
		Expect(result.definition.frames[1].movementMap.width == result.promotedMap.width, "second frame-id frame should use promoted movement map");
	}
	Expect(result.definition.initialState.npcControls.entries.size() == 1, "frame-id authored controls should keep initial controls valid");
	Expect(result.definition.initialState.npcControls.entries[0].objective.type == iggy::NpcObjectiveType::Wait, "frame-id authored controls should not replace initial controls");
}

void TestNoFrameAuthoredPlayerCommandCreatesDefaultFrameIntent()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.authoredPlayerCommands = {
		MovePlayerToTile(3, 1),
	};
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config =
		ConfigWithFrame();
	config.profileTraits = Catalog({ { Id("profile:guard"), Traits() } });

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult result =
		Convert(plan, config);

	Expect(result.ok(), "no-frame authored player command should convert into default frame");
	Expect(result.authoredPlayerCommandCount == 1, "authored player command should be counted");
	Expect(result.definition.frames.size() == 1, "no-frame authored player command should keep default one-frame path");
	Expect(result.definition.frames[0].playerFrame.playerIntents.size() == 1, "default frame should receive one player intent");
	const iggy::PlayerInputIntent2D &intent =
		result.definition.frames[0].playerFrame.playerIntents[0];
	Expect(intent.type == iggy::PlayerInputIntent2DType::MoveToTile, "authored player command should become move-to-tile intent");
	Expect(intent.tile.x == 3 && intent.tile.y == 1, "authored player command should preserve target tile");
	Expect(result.definition.frames[0].controlOverrides.empty(), "no-frame player command should not invent NPC overrides");
}

void TestSharedFrameIdAuthoredPlayerCommandAndNpcControlShareFrame()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.authoredControls = {
		SeekingControl("npc:guard", { 2.5F, 1.5F }, "frame:shared"),
	};
	plan.authoredControls[0].hasDeclarationIndex = true;
	plan.authoredControls[0].declarationIndex = 0;
	plan.authoredPlayerCommands = {
		MovePlayerToTile(3, 1, "frame:shared", true, 1),
	};
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config =
		ConfigWithFrame();
	config.profileTraits = Catalog({ { Id("profile:guard"), Traits() } });

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult result =
		Convert(plan, config);

	Expect(result.ok(), "shared-frame authored player command and NPC control should convert");
	Expect(result.definition.frames.size() == 1, "shared frame id should produce one frame");
	if (!result.definition.frames.empty()) {
		Expect(result.definition.frames[0].frameId == Id("frame:shared"), "shared frame should preserve frame id");
		Expect(result.definition.frames[0].controlOverrides.size() == 1, "shared frame should carry NPC control override");
		Expect(result.definition.frames[0].playerFrame.playerIntents.size() == 1, "shared frame should carry player intent");
		Expect(result.definition.frames[0].playerFrame.playerIntents[0].tile.x == 3, "shared frame player intent should preserve tile");
	}
}

void TestPlayerOnlyFrameIdCreatesScenarioFrame()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.authoredPlayerCommands = {
		MovePlayerToTile(3, 1, "frame:player-only"),
	};
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config =
		ConfigWithFrame();
	config.profileTraits = Catalog({ { Id("profile:guard"), Traits() } });

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult result =
		Convert(plan, config);

	Expect(result.ok(), "player-only frame id should convert");
	Expect(result.definition.frames.size() == 1, "player-only frame id should produce one frame");
	if (!result.definition.frames.empty()) {
		Expect(result.definition.frames[0].hasFrameId && result.definition.frames[0].frameId == Id("frame:player-only"), "player-only frame should preserve frame id");
		Expect(result.definition.frames[0].controlOverrides.empty(), "player-only frame should not invent NPC overrides");
		Expect(result.definition.frames[0].playerFrame.playerIntents.size() == 1, "player-only frame should publish player intent");
	}
}

void TestAuthoredFrameGroupsPreserveCrossFamilyDeclarationOrder()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.authoredPlayerCommands = {
		MovePlayerToTile(3, 1, "frame:player-first", true, 0),
	};
	plan.authoredControls = {
		SeekingControl("npc:guard", { 2.5F, 1.5F }, "frame:npc-second"),
	};
	plan.authoredControls[0].hasDeclarationIndex = true;
	plan.authoredControls[0].declarationIndex = 1;
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config =
		ConfigWithFrame();
	config.profileTraits = Catalog({ { Id("profile:guard"), Traits() } });

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult result =
		Convert(plan, config);

	Expect(result.ok(), "cross-family authored frame groups should convert");
	Expect(result.definition.frames.size() == 2, "cross-family authored frame groups should produce two frames");
	if (result.definition.frames.size() == 2) {
		Expect(result.definition.frames[0].frameId == Id("frame:player-first"), "player command declaration should be first frame");
		Expect(result.definition.frames[1].frameId == Id("frame:npc-second"), "NPC control declaration should be second frame");
	}
}

void TestNoFramePlayerCommandMixedWithFrameIdGroupFails()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.authoredControls = {
		SeekingControl("npc:guard", { 2.5F, 1.5F }, "frame:npc"),
	};
	plan.authoredPlayerCommands = {
		MovePlayerToTile(3, 1),
	};
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config =
		ConfigWithFrame();
	config.profileTraits = Catalog({ { Id("profile:guard"), Traits() } });

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult result =
		Convert(plan, config);

	Expect(!result.ok(), "no-frame player command mixed with frame-id groups should fail deterministically");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::AuthoredPlayerCommandInvalid, "ambiguous player command should report authored player command invalid");
	Expect(result.authoredPlayerCommandIssueCount == 1, "ambiguous player command should be counted");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::AmbiguousAuthoredPlayerCommandFrame), "ambiguous player command issue should be present");
	Expect(result.definition.frames.empty(), "ambiguous player command should not publish runnable definition");
}

void TestDuplicateAuthoredControlActorWithinFrameBlocksConversion()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.authoredControls = {
		SeekingControl("npc:guard", { 2.5F, 1.5F }, "frame:move-one"),
		SeekingControl("npc:guard", { 3.5F, 1.5F }, "frame:move-one"),
	};
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config =
		ConfigWithFrame();
	config.profileTraits = Catalog({ { Id("profile:guard"), Traits() } });

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult result =
		Convert(plan, config);

	Expect(!result.ok(), "duplicate frame authored control actor should fail conversion");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::AuthoredControlInvalid, "duplicate frame authored control actor should report authored control invalid");
	Expect(result.authoredControlIssueCount == 1, "duplicate frame authored control actor should be counted once");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::DuplicateAuthoredControlActor), "duplicate frame authored control actor issue should be present");
	Expect(result.issues[0].authoredControl.frameId == Id("frame:move-one"), "duplicate frame authored control issue should preserve frame id");
	Expect(result.definition.frames.empty(), "duplicate frame authored control actor should not publish runnable definition");
}

void TestSameActorAcrossDifferentFrameIdsIsAllowed()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.authoredControls = {
		SeekingControl("npc:guard", { 2.5F, 1.5F }, "frame:move-one"),
		SeekingControl("npc:guard", { 3.5F, 1.5F }, "frame:move-two"),
	};
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config =
		ConfigWithFrame();
	config.profileTraits = Catalog({ { Id("profile:guard"), Traits() } });

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult result =
		Convert(plan, config);

	Expect(result.ok(), "same actor in different frame-id groups should convert");
	Expect(result.authoredControlIssueCount == 0, "same actor in different frame-id groups should not be duplicate");
	Expect(result.definition.frames.size() == 2, "same actor in different frame-id groups should publish both frames");
}

void TestUnknownAuthoredControlActorBlocksConversion()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.authoredControls = { SeekingControl("npc:missing") };
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config =
		ConfigWithFrame();
	config.profileTraits = Catalog({ { Id("profile:guard"), Traits() } });

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult result =
		Convert(plan, config);

	Expect(!result.ok(), "authored control for unknown actor should fail conversion");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::AuthoredControlInvalid, "unknown authored control actor should report authored control invalid status");
	Expect(result.authoredControlIssueCount == 1, "unknown authored control actor should be counted");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::UnknownAuthoredControlActor), "unknown authored control actor issue should be present");
	Expect(result.issues[0].authoredControl.npcId == Id("npc:missing"), "unknown authored control actor issue should preserve npc id");
	Expect(result.definition.frames.empty(), "unknown authored control actor should not publish runnable definition");
}

void TestDuplicateAuthoredControlActorBlocksConversion()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.authoredControls = {
		SeekingControl("npc:guard", { 2.5F, 1.5F }),
		SeekingControl("npc:guard", { 3.5F, 1.5F }),
	};
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config =
		ConfigWithFrame();
	config.profileTraits = Catalog({ { Id("profile:guard"), Traits() } });

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult result =
		Convert(plan, config);

	Expect(!result.ok(), "duplicate authored control actor should fail conversion");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::AuthoredControlInvalid, "duplicate authored control actor should report authored control invalid status");
	Expect(result.authoredControlCount == 2, "duplicate authored controls should preserve count");
	Expect(result.authoredControlIssueCount == 1, "duplicate authored control actor should be counted once");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::DuplicateAuthoredControlActor), "duplicate authored control actor issue should be present");
	Expect(result.issues[0].authoredControl.targetPosition.x == 3.5, "duplicate authored control issue should preserve duplicate declaration");
	Expect(result.definition.frames.empty(), "duplicate authored control actor should not publish runnable definition");
}

void TestActorWithoutLocalPositionUsesTileCenter()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.annotatedCells[0].localPosition.present = false;
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config =
		ConfigWithFrame();
	config.profileTraits = Catalog({ { Id("profile:guard"), Traits() } });

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult result =
		Convert(plan, config);

	Expect(result.ok(), "actor without local position should convert");
	const iggy::NpcActorState2D &actor = result.definition.initialState.npcActors.actors[0];
	Expect(actor.position.x == 1.5F && actor.position.y == 1.5F, "actor without local position should use tile center");
}

void TestUnsupportedCustomTerrainFailsBeforeDefinition()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.grid.rows[1] = "#A~.#";
	plan.legend.push_back({
		'~',
		iggy::runtime::RuntimeGameplayAsciiSourcePlanGlyphKind::Terrain,
		Id("role:water"),
		{},
		false,
		iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::Unknown,
		{},
		{},
	});
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config =
		ConfigWithFrame();
	config.profileTraits = Catalog({ { Id("profile:guard"), Traits() } });

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult result =
		Convert(plan, config);

	Expect(!result.ok(), "unsupported custom terrain should fail conversion");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::UnsupportedTerrainPromotion, "unsupported custom terrain should report terrain status");
	Expect(result.unsupportedTerrainPromotionCount == 1, "unsupported custom terrain should be counted once");
	Expect(result.issues[0].glyph == '~' && result.issues[0].row == 1 && result.issues[0].column == 2, "unsupported terrain issue should preserve glyph location");
	Expect(result.definition.frames.empty(), "unsupported terrain should not publish profile scenario definition");
	Expect(result.definition.initialState.npcActors.actors.empty(), "unsupported terrain should not publish actors to run");
	Expect(result.definition.initialState.npcControls.entries.empty(), "unsupported terrain should not publish controls to run");
}

void TestMappedCustomTerrainPromotes()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.grid.rows[1] = "#A~.#";
	plan.legend.push_back({
		'~',
		iggy::runtime::RuntimeGameplayAsciiSourcePlanGlyphKind::Terrain,
		Id("role:water"),
		{},
		false,
		iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::Unknown,
		{},
		{},
	});
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config =
		ConfigWithFrame();
	config.profileTraits = Catalog({ { Id("profile:guard"), Traits() } });
	config.terrain = { { '~', false } };

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult result =
		Convert(plan, config);

	Expect(result.ok(), "mapped custom terrain should convert");
	Expect(result.promotedMap.tileAt(2, 1) != nullptr && !result.promotedMap.tileAt(2, 1)->walkable, "mapped custom terrain should preserve configured walkability");
}

void TestDuplicatePromotedActorReportsActorRegistryInvalid()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.grid.rows[1] = "#AA.#";
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAnnotatedCell duplicate = plan.annotatedCells[0];
	duplicate.cellId = Id("cell:guard-copy");
	duplicate.column = 2;
	duplicate.localPosition = { true, 2.5, 1.5 };
	plan.annotatedCells.push_back(duplicate);
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config =
		ConfigWithFrame();
	config.profileTraits = Catalog({ { Id("profile:guard"), Traits() } });

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult result =
		Convert(plan, config);

	Expect(!result.ok(), "duplicate promoted actor id should fail conversion");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::ActorRegistryInvalid, "duplicate actor should report actor registry invalid");
	Expect(result.actorRegistryIssueCount == 1, "duplicate actor should be counted once");
	Expect(result.actorRegistry.issues[0].code == iggy::NpcActorState2DIssueCode::DuplicateNpcId, "actor registry issue should preserve duplicate id code");
	Expect(result.definition.frames.empty(), "invalid actor registry should not publish profile scenario definition");
}

void TestInvalidDefaultControlReportsControlRegistryInvalid()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config =
		ConfigWithFrame();
	config.profileTraits = Catalog({ { Id("profile:guard"), Traits() } });
	config.hasDefaultControl = true;
	config.defaultControl.objective = iggy::followNpcObjective({});
	config.defaultControl.behavior = iggy::waitingNpcBehaviorState();
	config.defaultControl.moveMode = iggy::NpcMoveMode::Still;

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult result =
		Convert(plan, config);

	Expect(!result.ok(), "invalid default control should fail conversion");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::ControlRegistryInvalid, "invalid default control should report control registry invalid");
	Expect(result.controlRegistryIssueCount == 1, "invalid default control should be counted once");
	Expect(result.controlRegistry.issues[0].code == iggy::NpcActorControlState2DIssueCode::InvalidObjective, "control registry issue should preserve invalid objective code");
	Expect(result.definition.frames.empty(), "invalid control registry should not publish profile scenario definition");
}

void TestMissingProfileCatalogSurfacesProfileScenarioValidationFailure()
{
	const iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config =
		ConfigWithFrame();

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult result =
		Convert(plan, config);

	Expect(!result.ok(), "missing profile catalog should fail through profile scenario validation");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::ProfileScenarioInvalid, "missing profile should report profile scenario invalid");
	Expect(result.profileScenarioIssueCount > 0, "missing profile should be mirrored into conversion issues");
	Expect(!result.profileValidation.ok(), "nested profile validation should fail");
	Expect(result.profileValidation.missingProfileTraitCount == 1, "nested profile validation should preserve missing profile diagnostics");
	Expect(result.definition.initialState.npcActors.actors.size() == 1, "profile validation failure should preserve promoted actor facts for audit");
	Expect(result.definition.frames.size() == 1, "profile validation failure should preserve generated frame for audit");
}

void TestFrameUsesPromotedMapAndDoesNotInventScripts()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.hasSourceId = true;
	plan.sourceId = Id("scenario:plain");
	plan.annotatedCells[0].markerId = Id("plain-actor");
	plan.annotatedCells[0].profileId = Id("profile:namespaced");
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config =
		ConfigWithFrame();
	config.profileTraits = Catalog({ { Id("profile:namespaced"), Traits() } });

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult result =
		Convert(plan, config);

	Expect(result.ok(), "exact id profile scenario should convert");
	Expect(result.definition.scenarioId == Id("scenario:plain"), "converter should preserve exact source id");
	Expect(result.definition.frames.size() == 1, "converter should produce exactly one default frame");
	Expect(result.definition.frames[0].movementMap.id == Id("scenario:plain"), "default frame should use promoted map id");
	Expect(result.definition.frames[0].movementMap.tiles.size() == result.definition.initialState.session.level.map.tiles.size(), "frame movement map should match promoted map tile count");
	Expect(result.definition.initialState.npcActors.actors[0].npcId == Id("plain-actor"), "converter should preserve unqualified npc id");
	Expect(result.definition.initialState.npcActors.actors[0].aiProfileId == Id("profile:namespaced"), "converter should preserve namespaced profile id");
	Expect(result.definition.frames[0].playerFrame.npcMovementRequests.empty(), "converter should not invent prepared movement script semantics");
}

void TestConvertedDefaultProfileScenarioRunsThroughProfileRunner()
{
	const iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config =
		ConfigWithFrame();
	config.profileTraits = Catalog({ { Id("profile:guard"), Traits() } });

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult converted =
		Convert(plan, config);
	const iggy::runtime::RuntimeGameplayProfileScenarioRunResult run =
		iggy::runtime::RuntimeGameplayProfileScenarioRunner {}.run(converted.definition);

	Expect(converted.ok(), "runner acceptance requires converted profile scenario");
	Expect(run.ran(), "converted default profile scenario should run");
	Expect(run.validation.ok(), "converted default profile scenario should validate in runner");
	Expect(run.frameCount == 1, "converted default profile scenario should run one frame");
	Expect(run.ledger.hasProfileValidation, "converted default profile scenario ledger should preserve profile validation");
	Expect(run.state.npcActors.actors.size() == 1, "converted default profile scenario should preserve promoted actor through runner");
	Expect(run.state.npcControls.entries.size() == 1, "converted default profile scenario should preserve promoted control through runner");
	Expect(run.state.npcActors.actors[0].npcId == Id("npc:guard"), "converted default profile scenario should preserve npc id after runner");
	Expect(run.state.npcActors.actors[0].position.x == 1.5F && run.state.npcActors.actors[0].position.y == 1.5F, "converted default profile scenario should not invent movement");
	Expect(run.npcMovedCount == 0 && !run.npcActorsChanged, "converted default profile scenario should remain no-op for movement");
	Expect(converted.definition.initialState.session.level.map.width == run.state.session.level.map.width, "runner should preserve promoted session map");
}

void TestConvertedRegionAiMapProfileScenarioRunsThroughProfileRunner()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.regions = {
		Region("region:patrol", 1, 2, 1, 3, { Id("role:patrol") }),
	};
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config =
		ConfigWithFrame();
	config.profileTraits = Catalog({ { Id("profile:guard"), Traits() } });
	config.promoteRegionAiMap = true;
	config.regionAiMap.policies = {
		AiMapPolicy("role:patrol", { Id("tag:patrol") }),
	};

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult converted =
		Convert(plan, config);
	const iggy::runtime::RuntimeGameplayProfileScenarioRunResult run =
		iggy::runtime::RuntimeGameplayProfileScenarioRunner {}.run(converted.definition);

	Expect(converted.ok(), "region-promoted profile scenario should convert");
	Expect(run.ran(), "region-promoted profile scenario should run");
	Expect(run.validation.ok(), "region-promoted profile scenario should validate in runner");
	Expect(run.build.built, "region-promoted profile scenario should build before runner execution");
	Expect(run.definition.frames[0].aiMap.nodes.size() == 1, "runner should preserve promoted profile frame ai map");
	Expect(run.definition.frames[0].refreshAiMap.nodes.size() == 1, "runner should preserve promoted profile frame refresh ai map");
	Expect(run.build.scenarioDefinition.frames[0].frame.aiMap.nodes.size() == 1, "runner build should lower promoted ai map into ordinary scenario frame");
	Expect(run.build.scenarioDefinition.frames[0].frame.refreshAiMap.nodes.size() == 1, "runner build should lower promoted refresh ai map into ordinary scenario frame");
	Expect(run.build.scenarioDefinition.frames[0].frame.aiMap.nodes[0].id == Id("region:patrol"), "runner build should preserve exact promoted ai map region id");
	Expect(run.build.scenarioDefinition.frames[0].frame.refreshAiMap.nodes[0].id == Id("region:patrol"), "runner build should preserve exact promoted refresh ai map region id");
	Expect(run.frameCount == 1, "region-promoted profile scenario should run one frame");
	Expect(run.state.npcActors.actors.size() == 1, "region-promoted profile scenario should preserve promoted actor through runner");
}

void TestDefaultConfigLeavesRegionAiMapPromotionDisabled()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.regions = {
		Region("region:patrol", 1, 2, 1, 3, { Id("role:patrol") }),
	};
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config =
		ConfigWithFrame();
	config.profileTraits = Catalog({ { Id("profile:guard"), Traits() } });

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult result =
		Convert(plan, config);

	Expect(result.ok(), "default config with regions should still convert");
	Expect(!result.config.promoteRegionAiMap, "default config should keep region ai map promotion disabled");
	Expect(result.regionAiMapPromotion.regionCount == 0, "disabled region ai map promotion should not run promoter");
	Expect(result.promotedAiMapRegionCount == 0 && result.unmappedAiMapRegionCount == 0, "disabled region ai map promotion should not mirror counts");
	Expect(result.definition.frames.size() == 1, "disabled region ai map promotion should preserve default frame");
	Expect(result.definition.frames[0].aiMap.nodes.empty(), "disabled region ai map promotion should not alter frame ai map");
}

void TestEnabledRegionAiMapPromotionPreservesNestedResult()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.regions = {
		Region("region:patrol", 1, 2, 1, 3, { Id("role:patrol") }),
	};
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config =
		ConfigWithFrame();
	config.profileTraits = Catalog({ { Id("profile:guard"), Traits() } });
	config.promoteRegionAiMap = true;
	config.regionAiMap.policies = {
		AiMapPolicy("role:patrol", { Id("tag:patrol"), Id("plain") }),
	};

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult result =
		Convert(plan, config);

	Expect(result.ok(), "enabled region ai map promotion should convert");
	Expect(result.regionAiMapPromotion.ok(), "enabled region ai map promotion should preserve nested success");
	Expect(result.promotedAiMapRegionCount == 1 && result.unmappedAiMapRegionCount == 0, "enabled region ai map promotion should mirror counts");
	Expect(result.regionAiMapPromotion.aiMap.nodes.size() == 1, "enabled region ai map promotion should preserve ai map node");
	Expect(result.regionAiMapPromotion.aiMap.nodes[0].id == Id("region:patrol"), "enabled region ai map promotion should preserve exact region id");
	Expect(result.regionAiMapPromotion.aiMap.nodes[0].tags == std::vector<iggy::ResourceId>({ Id("tag:patrol"), Id("plain") }), "enabled region ai map promotion should preserve configured node tags");
	Expect(result.profileValidation.ok(), "region ai map converted definition should validate");
	Expect(result.definition.frames[0].aiMap.nodes.size() == 1, "enabled region ai map promotion should wire frame ai map");
	Expect(result.definition.frames[0].refreshAiMap.nodes.size() == 1, "enabled region ai map promotion should wire frame refresh ai map");
	Expect(result.definition.frames[0].aiMap.nodes[0].id == Id("region:patrol"), "frame ai map should preserve exact region id");
	Expect(result.definition.frames[0].refreshAiMap.nodes[0].id == Id("region:patrol"), "frame refresh ai map should preserve exact region id");
	Expect(result.definition.frames[0].aiMap.nodes[0].tags == std::vector<iggy::ResourceId>({ Id("tag:patrol"), Id("plain") }), "frame ai map should preserve configured node tags");
	Expect(result.definition.frames[0].refreshAiMap.nodes[0].tags == result.definition.frames[0].aiMap.nodes[0].tags, "frame refresh ai map should match frame ai map tags");
	Expect(result.definition.frames[0].movementMap.id == Id("scenario:ascii-profile"), "region ai map promotion should still use promoted terrain map for movement map");
}

void TestInvalidRegionAiMapPromotionBlocksConversion()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.regions = {
		Region("region:dupe", 1, 2, 1, 2, { Id("role:patrol") }),
		Region("region:dupe", 1, 3, 1, 3, { Id("role:patrol") }),
	};
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config =
		ConfigWithFrame();
	config.profileTraits = Catalog({ { Id("profile:guard"), Traits() } });
	config.promoteRegionAiMap = true;
	config.regionAiMap.policies = { AiMapPolicy("role:patrol") };

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult result =
		Convert(plan, config);

	Expect(!result.ok(), "invalid promoted ai map should block conversion");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::AiMapPromotionInvalid, "invalid promoted ai map should report ai map promotion status");
	Expect(!result.regionAiMapPromotion.ok(), "invalid promoted ai map should preserve nested failure");
	Expect(result.aiMapPromotionIssueCount == 1, "invalid promoted ai map should mirror issue count");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::AiMapPromotionInvalid), "invalid promoted ai map should add conversion issue");
	Expect(!result.converted, "invalid promoted ai map should not mark conversion complete");
	Expect(result.definition.frames.empty(), "invalid promoted ai map should not publish profile scenario definition");
}

void TestUnmappedRegionDoesNotBlockConversionByDefault()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan = SourcePlan();
	plan.regions = {
		Region("region:patrol", 1, 2, 1, 2, { Id("role:patrol") }),
		Region("region:ignored", 1, 3, 1, 3, { Id("role:ignored") }),
	};
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config =
		ConfigWithFrame();
	config.profileTraits = Catalog({ { Id("profile:guard"), Traits() } });
	config.promoteRegionAiMap = true;
	config.regionAiMap.policies = { AiMapPolicy("role:patrol") };

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult result =
		Convert(plan, config);

	Expect(result.ok(), "unmapped ai map region should not block conversion by default");
	Expect(result.regionAiMapPromotion.ok(), "unmapped ai map region should preserve nested non-fatal success");
	Expect(result.promotedAiMapRegionCount == 1 && result.unmappedAiMapRegionCount == 1, "unmapped ai map region should mirror mapped/unmapped counts");
	Expect(result.issueCount == 0, "unmapped ai map region should not become conversion issue by default");
	Expect(result.regionAiMapPromotion.issues.size() == 1, "unmapped ai map region should remain inspectable in nested diagnostics");
}

} // namespace

int main()
{
	TestInvalidSourcePlanShortCircuitsBeforePublishingDefinition();
	TestValidSourcePlanRequiresFrameDefaults();
	TestValidSourcePlanAndFrameDefaultsPublishValidatedProfileScenario();
	TestTerrainActorAndDefaultControlPromotion();
	TestAuthoredControlReplacesDefaultControlForPromotedActor();
	TestFrameIdAuthoredControlsCreateScenarioFramesInFirstSeenOrder();
	TestNoFrameAuthoredPlayerCommandCreatesDefaultFrameIntent();
	TestSharedFrameIdAuthoredPlayerCommandAndNpcControlShareFrame();
	TestPlayerOnlyFrameIdCreatesScenarioFrame();
	TestAuthoredFrameGroupsPreserveCrossFamilyDeclarationOrder();
	TestNoFramePlayerCommandMixedWithFrameIdGroupFails();
	TestDuplicateAuthoredControlActorWithinFrameBlocksConversion();
	TestSameActorAcrossDifferentFrameIdsIsAllowed();
	TestUnknownAuthoredControlActorBlocksConversion();
	TestDuplicateAuthoredControlActorBlocksConversion();
	TestActorWithoutLocalPositionUsesTileCenter();
	TestUnsupportedCustomTerrainFailsBeforeDefinition();
	TestMappedCustomTerrainPromotes();
	TestDuplicatePromotedActorReportsActorRegistryInvalid();
	TestInvalidDefaultControlReportsControlRegistryInvalid();
	TestMissingProfileCatalogSurfacesProfileScenarioValidationFailure();
	TestFrameUsesPromotedMapAndDoesNotInventScripts();
	TestConvertedDefaultProfileScenarioRunsThroughProfileRunner();
	TestConvertedRegionAiMapProfileScenarioRunsThroughProfileRunner();
	TestDefaultConfigLeavesRegionAiMapPromotionDisabled();
	TestEnabledRegionAiMapPromotionPreservesNestedResult();
	TestInvalidRegionAiMapPromotionBlocksConversion();
	TestUnmappedRegionDoesNotBlockConversionByDefault();
	return Failures;
}
