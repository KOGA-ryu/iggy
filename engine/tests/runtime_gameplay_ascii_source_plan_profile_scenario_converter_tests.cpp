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

} // namespace

int main()
{
	TestInvalidSourcePlanShortCircuitsBeforePublishingDefinition();
	TestValidSourcePlanRequiresFrameDefaults();
	TestValidSourcePlanAndFrameDefaultsPublishValidatedProfileScenario();
	TestTerrainActorAndDefaultControlPromotion();
	TestActorWithoutLocalPositionUsesTileCenter();
	TestUnsupportedCustomTerrainFailsBeforeDefinition();
	TestMappedCustomTerrainPromotes();
	TestDuplicatePromotedActorReportsActorRegistryInvalid();
	TestInvalidDefaultControlReportsControlRegistryInvalid();
	TestMissingProfileCatalogSurfacesProfileScenarioValidationFailure();
	TestFrameUsesPromotedMapAndDoesNotInventScripts();
	TestConvertedDefaultProfileScenarioRunsThroughProfileRunner();
	return Failures;
}
