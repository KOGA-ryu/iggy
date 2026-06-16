#include <cstdlib>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "runtime/RuntimeGameplayProfileScenarioRunner.hpp"
#include "runtime/RuntimeGameplaySaveSlotStore.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::MapFromRows;
using iggy::test::NearVec;

std::filesystem::path TempRoot()
{
	return std::filesystem::temp_directory_path() / "iggy_runtime_orchestrated_scenario_save_load_acceptance_tests";
}

void ResetTempRoot()
{
	std::error_code ignored;
	std::filesystem::remove_all(TempRoot(), ignored);
	std::filesystem::create_directories(TempRoot() / "manual", ignored);
}

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::runtime::RuntimeSaveSlotId Slot(const char *name)
{
	return { iggy::runtime::RuntimeSaveSlotKind::Manual, std::string(name) };
}

iggy::runtime::RuntimeGameplaySaveSlotStoreConfig SlotConfig()
{
	iggy::runtime::RuntimeGameplaySaveSlotStoreConfig config;
	config.baseDirectory = TempRoot();
	config.restore.session.buildConfig.buildRenderCache = false;
	return config;
}

iggy::LevelTileMap LevelMap(const char *id, std::vector<std::string_view> rows)
{
	iggy::LevelTileMap map = MapFromRows(rows);
	map.id = Id(id);
	map.playerStart = { 0, 0 };
	return map;
}

iggy::NpcActorState2D Actor(const char *npcId, iggy::Vec2 position)
{
	return {
		Id(npcId),
		Id("profile:acceptance"),
		Id("faction:acceptance"),
		position,
		Id("goal:acceptance"),
		true,
	};
}

iggy::NpcActorState2DRegistry Actors(std::vector<iggy::NpcActorState2D> actors)
{
	const iggy::NpcActorState2DRegistryBuildResult result =
		iggy::NpcActorState2DRegistryBuilder {}.build(actors);
	Expect(result.built, "acceptance actors should build");
	return result.registry;
}

iggy::NpcActorControlState2D Control(const char *npcId, iggy::Vec2 target)
{
	return {
		Id(npcId),
		iggy::moveToNpcObjective(target),
		iggy::seekingNpcBehaviorState(target),
		iggy::NpcMoveMode::Still,
	};
}

iggy::NpcActorControlState2DRegistry Controls(std::vector<iggy::NpcActorControlState2D> controls)
{
	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);
	Expect(result.built, "acceptance controls should build");
	return result.registry;
}

iggy::NpcTraitSet Traits()
{
	iggy::NpcTraitSet traits;
	traits.strength = 12;
	return traits;
}

iggy::NpcAiProfileTraitCatalog ProfileCatalog()
{
	const iggy::NpcAiProfileTraitCatalogBuildResult result =
		iggy::NpcAiProfileTraitCatalogBuilder {}.build(
			{ { Id("profile:acceptance"), Traits() } });
	Expect(result.built, "acceptance profile catalog should build");
	return result.catalog;
}

iggy::NpcMapPlayControlFramePlanPools Pools()
{
	iggy::NpcMapPlayControlFramePlanPools pools;
	const iggy::NpcStrengthPoolBuildResult strength =
		iggy::NpcStrengthPoolBuilder {}.build({
			{ Id("strength:acceptance-seek"), 0, iggy::NpcBehaviorStateType::Seeking, Id("action:seek"), 1.0F, {} },
		});
	const iggy::NpcDexterityPoolBuildResult dexterity =
		iggy::NpcDexterityPoolBuilder {}.build({});
	const iggy::NpcConstitutionPoolBuildResult constitution =
		iggy::NpcConstitutionPoolBuilder {}.build({});
	const iggy::NpcIntelligencePoolBuildResult intelligence =
		iggy::NpcIntelligencePoolBuilder {}.build({});
	const iggy::NpcWisdomPoolBuildResult wisdom =
		iggy::NpcWisdomPoolBuilder {}.build({});
	const iggy::NpcCharismaPoolBuildResult charisma =
		iggy::NpcCharismaPoolBuilder {}.build({});
	Expect(strength.built && dexterity.built && constitution.built && intelligence.built && wisdom.built && charisma.built, "acceptance pools should build");
	pools.strength = strength.pool;
	pools.dexterity = dexterity.pool;
	pools.constitution = constitution.pool;
	pools.intelligence = intelligence.pool;
	pools.wisdom = wisdom.pool;
	pools.charisma = charisma.pool;
	return pools;
}

iggy::runtime::RuntimeGameplayState InitialState()
{
	iggy::runtime::RuntimeGameplayState state;
	state.session.level.map = LevelMap("level:acceptance-initial", { "...." });
	state.session.tickIndex = 77;
	state.inventory.inventory.stacks = {
		{ Id("item:acceptance-key"), 1 },
	};
	state.inventory.drops.drops = {
		{ Id("drop:acceptance-coin"), Id("item:coin"), 3, { 3.5F, 0.5F }, 0.25F, true },
	};
	state.npcActors = Actors({ Actor("npc:acceptance", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ Control("npc:acceptance", { 2.5F, 0.5F }) });
	return state;
}

iggy::runtime::RuntimeGameplayProfileScenarioDefinition Scenario(
	iggy::runtime::RuntimeGameplayState state)
{
	iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition frame;
	frame.hasFrameId = true;
	frame.frameId = Id("frame:acceptance");
	frame.pools = Pools();
	frame.movementMap = LevelMap("level:acceptance-movement", { "...." });

	iggy::runtime::RuntimeGameplayProfileScenarioDefinition definition;
	definition.hasScenarioId = true;
	definition.scenarioId = Id("scenario:save-load-acceptance");
	definition.initialState = state;
	definition.profileTraits = ProfileCatalog();
	definition.frames = { frame };
	return definition;
}

const iggy::NpcActorState2D *FindActor(
	const iggy::NpcActorState2DRegistry &registry,
	const iggy::ResourceId &npcId)
{
	for (const iggy::NpcActorState2D &actor : registry.actors) {
		if (actor.npcId == npcId)
			return &actor;
	}
	return nullptr;
}

const iggy::NpcActorControlState2D *FindControl(
	const iggy::NpcActorControlState2DRegistry &registry,
	const iggy::ResourceId &npcId)
{
	for (const iggy::NpcActorControlState2D &control : registry.entries) {
		if (control.npcId == npcId)
			return &control;
	}
	return nullptr;
}

void ExpectSharedFacts(
	const iggy::runtime::RuntimeGameplayState &actual,
	const iggy::runtime::RuntimeGameplayState &expected,
	const char *message)
{
	Expect(actual.session.level.map.id == expected.session.level.map.id, message);
	Expect(actual.session.tickIndex == expected.session.tickIndex, message);
	Expect(actual.inventory.inventory.stacks.size() == expected.inventory.inventory.stacks.size(), message);
	Expect(actual.inventory.drops.drops.size() == expected.inventory.drops.drops.size(), message);
	if (!actual.inventory.inventory.stacks.empty() && !expected.inventory.inventory.stacks.empty()) {
		Expect(actual.inventory.inventory.stacks[0].itemId == expected.inventory.inventory.stacks[0].itemId, message);
		Expect(actual.inventory.inventory.stacks[0].count == expected.inventory.inventory.stacks[0].count, message);
	}
}

void TestGameplaySaveLoadSurroundsProfileScenarioRun()
{
	ResetTempRoot();
	const iggy::runtime::RuntimeGameplaySaveSlotStoreConfig config = SlotConfig();
	const iggy::runtime::RuntimeGameplayState initial = InitialState();
	const iggy::runtime::RuntimeGameplaySaveSlotStore store;

	const iggy::runtime::RuntimeGameplaySaveSlotSaveResult initialSave =
		store.saveState(initial, config, Slot("initial"));
	const iggy::runtime::RuntimeGameplaySaveSlotLoadResult initialLoad =
		store.load(config, Slot("initial"));

	Expect(initialSave.status == iggy::runtime::RuntimeGameplaySaveSlotStatus::Saved, "initial gameplay state should save");
	Expect(initialLoad.status == iggy::runtime::RuntimeGameplaySaveSlotStatus::Loaded, "initial gameplay state should load");
	ExpectSharedFacts(initialLoad.state, initial, "initial load should preserve session/inventory facts");
	const iggy::NpcActorState2D *initialActor =
		FindActor(initialLoad.state.npcActors, Id("npc:acceptance"));
	const iggy::NpcActorControlState2D *initialControl =
		FindControl(initialLoad.state.npcControls, Id("npc:acceptance"));
	Expect(initialActor != nullptr && NearVec(initialActor->position, { 0.5F, 0.5F }), "initial load should preserve NPC actor position");
	Expect(initialControl != nullptr && initialControl->moveMode == iggy::NpcMoveMode::Still, "initial load should preserve NPC control state");

	const iggy::runtime::RuntimeGameplayProfileScenarioRunResult scenario =
		iggy::runtime::RuntimeGameplayProfileScenarioRunner {}.run(
			Scenario(initialLoad.state));
	Expect(scenario.ran(), "loaded state should run a valid profile scenario");
	Expect(scenario.npcControlAppliedCount == 1 && scenario.npcMovedCount == 1, "profile scenario should update control and move actor");

	const iggy::runtime::RuntimeGameplaySaveSlotSaveResult finalSave =
		store.saveState(scenario.state, config, Slot("final"));
	const iggy::runtime::RuntimeGameplaySaveSlotLoadResult finalLoad =
		store.load(config, Slot("final"));

	Expect(finalSave.status == iggy::runtime::RuntimeGameplaySaveSlotStatus::Saved, "final gameplay state should save");
	Expect(finalLoad.status == iggy::runtime::RuntimeGameplaySaveSlotStatus::Loaded, "final gameplay state should load");
	ExpectSharedFacts(finalLoad.state, scenario.state, "final load should preserve final session/inventory facts");
	const iggy::NpcActorState2D *finalActor =
		FindActor(finalLoad.state.npcActors, Id("npc:acceptance"));
	const iggy::NpcActorControlState2D *finalControl =
		FindControl(finalLoad.state.npcControls, Id("npc:acceptance"));
	Expect(finalActor != nullptr && NearVec(finalActor->position, { 1.5F, 0.5F }), "final load should preserve moved NPC actor position");
	Expect(finalControl != nullptr && finalControl->moveMode == iggy::NpcMoveMode::Walk, "final load should preserve updated NPC control state");
}

} // namespace

int main()
{
	TestGameplaySaveLoadSurroundsProfileScenarioRun();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
