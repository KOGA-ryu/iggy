#include <cstdlib>
#include <string_view>
#include <type_traits>
#include <vector>

#include "runtime/RuntimeGameplayOrchestratedFrameRunner.hpp"
#include "runtime/RuntimeGameplayOrchestratedFrameRunnerReport.hpp"
#include "runtime/RuntimeGameplayScenarioRunner.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::MapFromRows;
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

const iggy::ResourceId PlayerId { "player:scenario" };

template <typename T, typename = void>
struct HasNpcActorsField : std::false_type {
};

template <typename T>
struct HasNpcActorsField<T, std::void_t<decltype(&T::npcActors)>> : std::true_type {
};

template <typename T, typename = void>
struct HasNpcControlsField : std::false_type {
};

template <typename T>
struct HasNpcControlsField<T, std::void_t<decltype(&T::npcControls)>> : std::true_type {
};

static_assert(!HasNpcActorsField<iggy::runtime::RuntimeSessionState>::value);
static_assert(!HasNpcControlsField<iggy::runtime::RuntimeSessionState>::value);

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::LevelTileMap LevelMap(std::vector<std::string_view> rows)
{
	iggy::LevelTileMap map = MapFromRows(rows);
	map.id = Id("level:scenario");
	return map;
}

iggy::runtime::RuntimeSessionState SessionWithPlayer(const iggy::LevelTileMap &map)
{
	iggy::runtime::RuntimeSessionState session;
	session.level.map = map;
	session.level.map.playerStart = { 0, 0 };
	session.tickIndex = 91;
	session.hasPlayer = true;
	session.player = PlayerAgent(PlayerId, { 0.0F, 0.0F }, { 0, 0 }, iggy::PlayerMovementStatus::Idle, iggy::PlayerFacing2D::East);
	return session;
}

iggy::runtime::RuntimePlayerCommandExecutionConfig PlayerConfig()
{
	iggy::runtime::RuntimePlayerCommandExecutionConfig config;
	config.movement.maxStep = 1.0F;
	return config;
}

iggy::npc_ai::NpcAgentTickConfig NpcConfig()
{
	iggy::npc_ai::NpcAgentTickConfig config;
	config.maxDistance = 0.25F;
	config.awareness = { 8.0F, 0 };
	return config;
}

iggy::InteractionTarget2D Target(
	const char *id,
	iggy::InteractionTarget2DKind kind = iggy::InteractionTarget2DKind::Usable,
	iggy::Vec2 position = { 0.0F, 0.0F })
{
	return { Id(id), kind, position, 0.0F, true };
}

iggy::InteractionTarget2DRegistry Targets(std::vector<iggy::InteractionTarget2D> targets)
{
	const iggy::InteractionTarget2DRegistryBuildResult result =
		iggy::InteractionTarget2DRegistryBuilder {}.build(targets);
	Expect(result.built, "scenario target registry should build");
	return result.registry;
}

iggy::InteractionEffectEntry2D Entry(const char *targetId, std::vector<iggy::InteractionEffect2D> effects)
{
	return { Id(targetId), effects };
}

iggy::InteractionEffectCatalog2D Catalog(std::vector<iggy::InteractionEffectEntry2D> entries)
{
	const iggy::InteractionEffectCatalog2DBuildResult result =
		iggy::InteractionEffectCatalog2DBuilder {}.build(entries);
	Expect(result.built, "scenario interaction catalog should build");
	return result.catalog;
}

iggy::InventoryState2D Inventory(std::vector<iggy::InventoryItemStack2D> stacks)
{
	const iggy::InventoryState2DBuildResult result = iggy::InventoryState2DBuilder {}.build(stacks);
	Expect(result.built, "scenario inventory should build");
	return result.inventory;
}

iggy::LevelItemDrop2D Drop(const char *id, const char *itemId, std::uint32_t count)
{
	return { Id(id), Id(itemId), count, { 0.0F, 0.0F }, 0.0F, true };
}

iggy::runtime::RuntimeInventoryState InventoryState(
	std::vector<iggy::InventoryItemStack2D> stacks,
	std::vector<iggy::LevelItemDrop2D> drops)
{
	return { Inventory(stacks), iggy::LevelItemDrop2DRegistry { drops } };
}

iggy::NpcActorState2D Actor(const char *npcId, iggy::Vec2 position)
{
	return {
		Id(npcId),
		Id("profile:scenario"),
		Id("faction:scenario"),
		position,
		Id("goal:scenario"),
		true,
	};
}

iggy::NpcActorControlState2D SeekingControl(const char *npcId, iggy::Vec2 target)
{
	return {
		Id(npcId),
		iggy::moveToNpcObjective(target),
		iggy::seekingNpcBehaviorState(target),
		iggy::NpcMoveMode::Walk,
	};
}

iggy::NpcActorState2DRegistry Actors(std::vector<iggy::NpcActorState2D> actors)
{
	const iggy::NpcActorState2DRegistryBuildResult result =
		iggy::NpcActorState2DRegistryBuilder {}.build(actors);
	Expect(result.built, "scenario actors should build");
	return result.registry;
}

iggy::NpcActorControlState2DRegistry Controls(std::vector<iggy::NpcActorControlState2D> controls)
{
	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);
	Expect(result.built, "scenario controls should build");
	return result.registry;
}

iggy::NpcActorOccupancy2D Occupancy(const iggy::NpcActorState2DRegistry &actors)
{
	return iggy::NpcActorOccupancyProjector2D {}.project(actors);
}

iggy::AiMap2D AiMap()
{
	const iggy::AiMap2DBuildResult result = iggy::AiMap2DBuilder {}.build({});
	Expect(result.built, "scenario AI map should build");
	return result.map;
}

iggy::runtime::RuntimeGameplayState GameplayState(const iggy::LevelTileMap &map)
{
	iggy::runtime::RuntimeGameplayState state;
	state.session = SessionWithPlayer(map);
	state.inventory.inventory = Inventory({});
	return state;
}

iggy::runtime::RuntimeGameplayFrameInput PlayerFrame(
	iggy::runtime::RuntimeGameplayState state = {},
	std::vector<iggy::PlayerInputIntent2D> intents = {})
{
	iggy::runtime::RuntimeGameplayFrameInput input;
	input.state = state;
	input.actorId = PlayerId;
	input.playerIntents = intents;
	input.fallbackPlayerPosition = { 0.0F, 0.0F };
	input.playerCommandConfig = PlayerConfig();
	input.npcConfig = NpcConfig();
	return input;
}

iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerFrame Frame(
	const iggy::runtime::RuntimeGameplayState &state,
	const iggy::LevelTileMap &map,
	std::vector<iggy::PlayerInputIntent2D> playerIntents = {})
{
	iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerFrame frame;
	frame.playerFrame = PlayerFrame(state, playerIntents);
	frame.movementMap = map;
	frame.previousOccupancy = Occupancy(state.npcActors);
	frame.interactionTargets = Targets({});
	frame.aiMap = AiMap();
	frame.refreshAiMap = AiMap();
	return frame;
}

iggy::runtime::RuntimeGameplayScenarioFrame ScenarioFrame(
	const iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerFrame &frame)
{
	return { frame };
}

const iggy::NpcActorState2D *FindActor(const iggy::NpcActorState2DRegistry &registry, const iggy::ResourceId &npcId)
{
	for (const iggy::NpcActorState2D &actor : registry.actors) {
		if (actor.npcId == npcId)
			return &actor;
	}
	return nullptr;
}

bool SameActors(const iggy::NpcActorState2DRegistry &actual, const iggy::NpcActorState2DRegistry &expected)
{
	if (actual.actors.size() != expected.actors.size())
		return false;
	for (std::size_t index = 0; index < actual.actors.size(); ++index) {
		if (actual.actors[index].npcId != expected.actors[index].npcId
			|| !NearVec(actual.actors[index].position, expected.actors[index].position)
			|| actual.actors[index].present != expected.actors[index].present)
			return false;
	}
	return true;
}

bool SameStacks(const std::vector<iggy::InventoryItemStack2D> &actual, std::vector<iggy::InventoryItemStack2D> expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (actual[index].itemId != expected[index].itemId || actual[index].count != expected[index].count)
			return false;
	}
	return true;
}

void TestEmptyScenarioNoOps()
{
	const iggy::LevelTileMap map = LevelMap({ "..." });
	const iggy::runtime::RuntimeGameplayState state = GameplayState(map);

	const iggy::runtime::RuntimeGameplayScenarioResult result =
		iggy::runtime::RuntimeGameplayScenarioRunner {}.run({ state, {} });

	Expect(!result.hasFrames() && !result.changed(), "empty scenario should report no frames or changes");
	Expect(result.runner.frameResults.empty(), "empty scenario should keep runner ticks empty");
	Expect(result.report.frames.empty(), "empty scenario should keep report frames empty");
	Expect(SameActors(result.state.npcActors, state.npcActors), "empty scenario should preserve actors");
}

void TestSingleFrameNpcMovementMatchesDirectRunnerAndReporter()
{
	const iggy::LevelTileMap map = LevelMap({ "...." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:single", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:single", { 3.5F, 0.5F }) });
	const iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerFrame frame = Frame(state, map);

	const iggy::runtime::RuntimeGameplayScenarioResult scenario =
		iggy::runtime::RuntimeGameplayScenarioRunner {}.run({ state, { ScenarioFrame(frame) } });
	const iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerResult directRunner =
		iggy::runtime::RuntimeGameplayOrchestratedFrameRunner {}.run({ state, { frame } });
	const iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerReport directReport =
		iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerReporter {}.report(directRunner);
	const iggy::NpcActorState2D *actor = FindActor(scenario.state.npcActors, Id("npc:single"));

	Expect(actor != nullptr && NearVec(actor->position, { 1.5F, 0.5F }), "scenario should move seeking NPC through orchestrated runner");
	Expect(SameActors(scenario.state.npcActors, directRunner.state.npcActors), "scenario final actors should match direct runner");
	Expect(scenario.npcMovedCount == directReport.npcMovedCount, "scenario moved count should match direct reporter");
	Expect(scenario.npcRefreshDirtyTileCount == directReport.npcRefreshDirtyTileCount, "scenario dirty tile count should match direct reporter");
	Expect(scenario.report.frames.size() == 1, "scenario report should preserve one compact frame report");
}

void TestMultiFrameScenarioCarriesStateAndReportOrder()
{
	const iggy::LevelTileMap map = LevelMap({ "....." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:multi", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:multi", { 4.5F, 0.5F }) });

	iggy::runtime::RuntimeGameplayState expectedAfterFirst = state;
	expectedAfterFirst.npcActors = Actors({ Actor("npc:multi", { 1.5F, 0.5F }) });
	const iggy::runtime::RuntimeGameplayScenario scenario {
		state,
		{
			ScenarioFrame(Frame(state, map)),
			ScenarioFrame(Frame(expectedAfterFirst, map)),
		},
	};

	const iggy::runtime::RuntimeGameplayScenarioResult result =
		iggy::runtime::RuntimeGameplayScenarioRunner {}.run(scenario);
	const iggy::NpcActorState2D *actor = FindActor(result.state.npcActors, Id("npc:multi"));

	Expect(actor != nullptr && NearVec(actor->position, { 2.5F, 0.5F }), "multi-frame scenario should carry actor position");
	Expect(result.frameCount == 2 && result.report.frames.size() == 2, "scenario should preserve report frame order");
	Expect(result.npcMovedCount == 2, "multi-frame scenario should aggregate moved count");
}

void TestPlayerPickupAndNpcMovementFacts()
{
	const iggy::LevelTileMap map = LevelMap({ "...." });
	const iggy::InteractionTarget2D target = Target("target:pickup", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:pickup", "item:pickup", 2);
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.interaction = {
		Targets({ target }),
		Catalog({ Entry("target:pickup", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
	};
	state.inventory = InventoryState({}, { drop });
	state.npcActors = Actors({ Actor("npc:pickup", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:pickup", { 3.5F, 0.5F }) });
	const iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerFrame frame =
		Frame(state, map, { iggy::playerInteractIntent(target.id) });

	const iggy::runtime::RuntimeGameplayScenarioResult result =
		iggy::runtime::RuntimeGameplayScenarioRunner {}.run({ state, { ScenarioFrame(frame) } });

	Expect(result.inventoryEventCount > 0, "scenario should report player inventory event facts");
	Expect(result.npcMovedCount == 1, "scenario should report NPC movement facts");
	Expect(SameStacks(result.state.inventory.inventory.stacks, { { Id("item:pickup"), 2 } }), "scenario should preserve player pickup result");
}

void TestBlockedNpcMovementReportsAndPreservesActor()
{
	const iggy::LevelTileMap map = LevelMap({ "...." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({
		Actor("npc:mover", { 0.5F, 0.5F }),
		Actor("npc:blocker", { 1.5F, 0.5F }),
	});
	state.npcControls = Controls({ SeekingControl("npc:mover", { 3.5F, 0.5F }) });
	const iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerFrame frame = Frame(state, map);

	const iggy::runtime::RuntimeGameplayScenarioResult result =
		iggy::runtime::RuntimeGameplayScenarioRunner {}.run({ state, { ScenarioFrame(frame) } });
	const iggy::NpcActorState2D *mover = FindActor(result.state.npcActors, Id("npc:mover"));

	Expect(mover != nullptr && NearVec(mover->position, { 0.5F, 0.5F }), "blocked scenario should preserve mover position");
	Expect(result.npcBlockedMovementCount == 1, "blocked scenario should report blocked movement");
	Expect(result.npcMovedCount == 0, "blocked scenario should not report moved actor");
}

void TestScenarioCopiesInputAndDoesNotMutate()
{
	const iggy::LevelTileMap map = LevelMap({ "...." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:copy", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:copy", { 3.5F, 0.5F }) });
	iggy::runtime::RuntimeGameplayScenario scenario {
		state,
		{ ScenarioFrame(Frame(state, map)) },
	};
	const iggy::runtime::RuntimeGameplayScenario before = scenario;

	const iggy::runtime::RuntimeGameplayScenarioResult result =
		iggy::runtime::RuntimeGameplayScenarioRunner {}.run(scenario);

	Expect(SameActors(scenario.initialState.npcActors, before.initialState.npcActors), "scenario runner should not mutate input state");
	Expect(result.scenario.frames.size() == before.frames.size(), "scenario result should copy source scenario");
	Expect(SameActors(result.scenario.initialState.npcActors, before.initialState.npcActors), "scenario copy should preserve source actors");
}

} // namespace

int main()
{
	TestEmptyScenarioNoOps();
	TestSingleFrameNpcMovementMatchesDirectRunnerAndReporter();
	TestMultiFrameScenarioCarriesStateAndReportOrder();
	TestPlayerPickupAndNpcMovementFacts();
	TestBlockedNpcMovementReportsAndPreservesActor();
	TestScenarioCopiesInputAndDoesNotMutate();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
