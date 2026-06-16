#include <cstdlib>
#include <string_view>
#include <vector>

#include "runtime/RuntimeGameplayProfileScenarioDefinition.hpp"
#include "runtime/RuntimeGameplayScenarioLedger.hpp"
#include "runtime/RuntimeGameplayScenarioRunner.hpp"
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

iggy::LevelTileMap LevelMap(const char *id, std::vector<std::string_view> rows)
{
	iggy::LevelTileMap map = MapFromRows(rows);
	map.id = Id(id);
	return map;
}

iggy::NpcActorState2D Actor(
	const char *npcId,
	const char *profileId = "profile:ledger",
	iggy::Vec2 position = { 0.5F, 0.5F },
	bool present = true)
{
	return {
		Id(npcId),
		Id(profileId),
		Id("faction:ledger"),
		position,
		Id("goal:ledger"),
		present,
	};
}

iggy::NpcActorControlState2D Control(
	const char *npcId,
	iggy::Vec2 target = { 2.5F, 0.5F },
	iggy::NpcMoveMode moveMode = iggy::NpcMoveMode::Still)
{
	return {
		Id(npcId),
		iggy::moveToNpcObjective(target),
		iggy::seekingNpcBehaviorState(target),
		moveMode,
	};
}

iggy::NpcActorState2DRegistry Actors(std::vector<iggy::NpcActorState2D> actors)
{
	const iggy::NpcActorState2DRegistryBuildResult result =
		iggy::NpcActorState2DRegistryBuilder {}.build(actors);
	Expect(result.built, "ledger actors should build");
	return result.registry;
}

iggy::NpcActorControlState2DRegistry Controls(std::vector<iggy::NpcActorControlState2D> controls)
{
	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);
	Expect(result.built, "ledger controls should build");
	return result.registry;
}

iggy::runtime::RuntimeGameplayState State()
{
	iggy::runtime::RuntimeGameplayState state;
	state.session.level.map = LevelMap("level:ledger", { "..." });
	return state;
}

iggy::runtime::RuntimeGameplayOrchestratedFrameReport FrameReport()
{
	return {};
}

void AddFrameToResult(
	iggy::runtime::RuntimeGameplayScenarioResult &result,
	const iggy::runtime::RuntimeGameplayOrchestratedFrameReport &frame)
{
	result.report.frames.push_back(frame);
	result.report.frameCount = result.report.frames.size();
	result.frameCount = result.report.frameCount;
	if (frame.changed() || frame.refreshedNpcData()) {
		++result.report.changedFrameCount;
		++result.changedFrameCount;
	}
	result.report.inventoryEventCount += frame.inventoryEventCount;
	result.inventoryEventCount = result.report.inventoryEventCount;
	result.report.npcControlPlannedRequestCount += frame.npcControlPlannedRequestCount;
	result.report.npcControlAppliedCount += frame.npcControlAppliedCount;
	result.report.npcControlFailedCount += frame.npcControlFailedCount;
	result.report.npcMovementPlannedRequestCount += frame.npcMovementPlannedRequestCount;
	result.report.npcMovedCount += frame.npcMovedCount;
	result.report.npcBlockedMovementCount += frame.npcBlockedMovementCount;
	result.report.npcRejectedMovementCount += frame.npcRejectedMovementCount;
	result.report.npcMissingActorMovementCount += frame.npcMissingActorMovementCount;
	result.report.npcRefreshDirtyTileCount += frame.npcRefreshDirtyTileCount;
	result.report.npcControlsChanged = result.report.npcControlsChanged || frame.npcControlsChanged;
	result.report.npcActorsChanged = result.report.npcActorsChanged || frame.npcActorsChanged;
	result.report.npcOccupancyRefreshed = result.report.npcOccupancyRefreshed || frame.npcOccupancyRefreshed;
	result.report.npcInteractionRefreshed = result.report.npcInteractionRefreshed || frame.npcInteractionRefreshed;
	result.report.npcAiMapRefreshed = result.report.npcAiMapRefreshed || frame.npcAiMapRefreshed;
	result.report.npcRenderRefreshed = result.report.npcRenderRefreshed || frame.npcRenderRefreshed;
	result.report.npcVisibilityRefreshed = result.report.npcVisibilityRefreshed || frame.npcVisibilityRefreshed;
	result.npcControlPlannedRequestCount = result.report.npcControlPlannedRequestCount;
	result.npcControlAppliedCount = result.report.npcControlAppliedCount;
	result.npcControlFailedCount = result.report.npcControlFailedCount;
	result.npcMovementPlannedRequestCount = result.report.npcMovementPlannedRequestCount;
	result.npcMovedCount = result.report.npcMovedCount;
	result.npcBlockedMovementCount = result.report.npcBlockedMovementCount;
	result.npcRejectedMovementCount = result.report.npcRejectedMovementCount;
	result.npcMissingActorMovementCount = result.report.npcMissingActorMovementCount;
	result.npcRefreshDirtyTileCount = result.report.npcRefreshDirtyTileCount;
	result.npcControlsChanged = result.report.npcControlsChanged;
	result.npcActorsChanged = result.report.npcActorsChanged;
	result.npcOccupancyRefreshed = result.report.npcOccupancyRefreshed;
	result.npcInteractionRefreshed = result.report.npcInteractionRefreshed;
	result.npcAiMapRefreshed = result.report.npcAiMapRefreshed;
	result.npcRenderRefreshed = result.report.npcRenderRefreshed;
	result.npcVisibilityRefreshed = result.report.npcVisibilityRefreshed;
}

iggy::runtime::RuntimeGameplayScenarioResult ScenarioResult(
	std::vector<iggy::runtime::RuntimeGameplayOrchestratedFrameReport> frames,
	iggy::runtime::RuntimeGameplayState state = State())
{
	iggy::runtime::RuntimeGameplayScenarioResult result;
	result.state = state;
	for (const iggy::runtime::RuntimeGameplayOrchestratedFrameReport &frame : frames) {
		AddFrameToResult(result, frame);
	}
	return result;
}

iggy::runtime::RuntimeGameplayScenarioLedger Ledger(
	const iggy::runtime::RuntimeGameplayScenarioResult &result)
{
	return iggy::runtime::RuntimeGameplayScenarioLedgerReporter {}.report(result);
}

bool HasEvent(
	const iggy::runtime::RuntimeGameplayScenarioLedger &ledger,
	iggy::runtime::RuntimeGameplayScenarioLedgerEvent event)
{
	for (const iggy::runtime::RuntimeGameplayScenarioLedgerEvent actual : ledger.events) {
		if (actual == event)
			return true;
	}
	return false;
}

iggy::NpcTraitSet Traits(int strength = 10)
{
	iggy::NpcTraitSet traits;
	traits.strength = strength;
	return traits;
}

iggy::NpcAiProfileTraitCatalog Catalog(std::vector<iggy::NpcAiProfileTraitEntry> entries)
{
	const iggy::NpcAiProfileTraitCatalogBuildResult result =
		iggy::NpcAiProfileTraitCatalogBuilder {}.build(entries);
	Expect(result.built, "ledger profile catalog should build");
	return result.catalog;
}

iggy::NpcStrengthEnt StrengthEnt(const char *entryId)
{
	return { Id(entryId), 0, iggy::NpcBehaviorStateType::Seeking, Id("action:seek"), 1.0F, {} };
}

iggy::NpcMapPlayControlFramePlanPools Pools()
{
	iggy::NpcMapPlayControlFramePlanPools pools;
	const iggy::NpcStrengthPoolBuildResult strength = iggy::NpcStrengthPoolBuilder {}.build({ StrengthEnt("strength:ledger") });
	const iggy::NpcDexterityPoolBuildResult dexterity = iggy::NpcDexterityPoolBuilder {}.build({});
	const iggy::NpcConstitutionPoolBuildResult constitution = iggy::NpcConstitutionPoolBuilder {}.build({});
	const iggy::NpcIntelligencePoolBuildResult intelligence = iggy::NpcIntelligencePoolBuilder {}.build({});
	const iggy::NpcWisdomPoolBuildResult wisdom = iggy::NpcWisdomPoolBuilder {}.build({});
	const iggy::NpcCharismaPoolBuildResult charisma = iggy::NpcCharismaPoolBuilder {}.build({});
	Expect(strength.built && dexterity.built && constitution.built && intelligence.built && wisdom.built && charisma.built, "ledger pools should build");
	pools.strength = strength.pool;
	pools.dexterity = dexterity.pool;
	pools.constitution = constitution.pool;
	pools.intelligence = intelligence.pool;
	pools.wisdom = wisdom.pool;
	pools.charisma = charisma.pool;
	return pools;
}

iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition ProfileFrame()
{
	iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition frame;
	frame.hasFrameId = true;
	frame.frameId = Id("frame:ledger-profile");
	frame.pools = Pools();
	frame.movementMap = LevelMap("level:ledger-profile", { "..." });
	return frame;
}

void TestEmptyScenarioResult()
{
	const iggy::runtime::RuntimeGameplayScenarioResult result = ScenarioResult({});
	const iggy::runtime::RuntimeGameplayScenarioLedger ledger = Ledger(result);

	Expect(ledger.empty(), "empty scenario ledger should use empty status");
	Expect(ledger.frameCount == 0 && ledger.frames.empty(), "empty scenario ledger should report no frames");
	Expect(ledger.inventoryEventCount == 0 && ledger.npcMovedCount == 0, "empty scenario ledger should report zero counts");
	Expect(ledger.events.size() == 2, "empty scenario ledger should still expose start/final events");
	Expect(ledger.events[0] == iggy::runtime::RuntimeGameplayScenarioLedgerEvent::ScenarioStarted, "empty scenario ledger should start with ScenarioStarted");
	Expect(ledger.events[1] == iggy::runtime::RuntimeGameplayScenarioLedgerEvent::ScenarioUnchanged, "empty scenario ledger should end unchanged");
}

void TestSingleFrameMovedNpcScenario()
{
	iggy::runtime::RuntimeGameplayOrchestratedFrameReport frame = FrameReport();
	frame.npcControlPlannedRequestCount = 1;
	frame.npcControlAppliedCount = 1;
	frame.npcControlsChanged = true;
	frame.npcMovementPlannedRequestCount = 1;
	frame.npcMovedCount = 1;
	frame.npcActorsChanged = true;
	frame.npcRefreshDirtyTileCount = 2;
	frame.npcOccupancyRefreshed = true;
	frame.npcInteractionRefreshed = true;
	frame.npcAiMapRefreshed = true;
	frame.npcRenderRefreshed = true;
	frame.npcVisibilityRefreshed = true;
	iggy::runtime::RuntimeGameplayState state = State();
	state.npcActors = Actors({ Actor("npc:moved"), Actor("npc:absent", "profile:ledger", { 2.5F, 0.5F }, false) });
	state.npcControls = Controls({ Control("npc:moved") });
	const iggy::runtime::RuntimeGameplayScenarioLedger ledger =
		Ledger(ScenarioResult({ frame }, state));

	Expect(ledger.status == iggy::runtime::RuntimeGameplayScenarioLedgerStatus::Reported, "moved scenario ledger should be reported");
	Expect(ledger.frameCount == 1 && ledger.changedFrameCount == 1, "moved scenario ledger should count changed frame");
	Expect(ledger.npcControlPlannedRequestCount == 1 && ledger.npcControlAppliedCount == 1, "moved scenario ledger should report control counts");
	Expect(ledger.npcMovedCount == 1 && ledger.npcMovementPlannedRequestCount == 1, "moved scenario ledger should report movement counts");
	Expect(ledger.npcRefreshDirtyTileCount == 2, "moved scenario ledger should report dirty tile count");
	Expect(ledger.finalNpcActorCount == 2 && ledger.finalPresentNpcCount == 1, "moved scenario ledger should report final actor/present counts");
	Expect(ledger.finalNpcControlCount == 1, "moved scenario ledger should report final control count");
	Expect(ledger.frames.size() == 1 && ledger.frames[0].frameIndex == 0 && ledger.frames[0].npcMovedCount == 1, "moved scenario ledger should preserve per-frame row");
	Expect(HasEvent(ledger, iggy::runtime::RuntimeGameplayScenarioLedgerEvent::NpcActorMoved), "moved scenario ledger should emit moved event");
	Expect(HasEvent(ledger, iggy::runtime::RuntimeGameplayScenarioLedgerEvent::RefreshVisibility), "moved scenario ledger should emit visibility refresh event");
}

void TestInventoryOnlyScenario()
{
	iggy::runtime::RuntimeGameplayOrchestratedFrameReport frame = FrameReport();
	frame.inventoryEventCount = 1;
	frame.inventoryChanged = true;
	frame.playerChanged = true;
	iggy::runtime::RuntimeGameplayState state = State();
	state.inventory.inventory.stacks = { { Id("item:ledger"), 2 } };
	state.inventory.drops.drops = { { Id("drop:ledger"), Id("item:ledger"), 1, { 0.0F, 0.0F }, 0.0F, true } };
	const iggy::runtime::RuntimeGameplayScenarioLedger ledger =
		Ledger(ScenarioResult({ frame }, state));

	Expect(ledger.inventoryEventCount == 1, "inventory scenario ledger should report inventory event");
	Expect(ledger.npcMovedCount == 0 && ledger.npcControlAppliedCount == 0, "inventory scenario ledger should not invent NPC facts");
	Expect(ledger.finalInventoryStackCount == 1 && ledger.finalInventoryDropCount == 1, "inventory scenario ledger should report final inventory facts");
	Expect(HasEvent(ledger, iggy::runtime::RuntimeGameplayScenarioLedgerEvent::InventoryChanged), "inventory scenario ledger should emit inventory event");
	Expect(!HasEvent(ledger, iggy::runtime::RuntimeGameplayScenarioLedgerEvent::NpcActorMoved), "inventory scenario ledger should not emit moved event");
}

void TestBlockedNpcScenario()
{
	iggy::runtime::RuntimeGameplayOrchestratedFrameReport frame = FrameReport();
	frame.npcMovementPlannedRequestCount = 1;
	frame.npcBlockedMovementCount = 1;
	const iggy::runtime::RuntimeGameplayScenarioLedger ledger =
		Ledger(ScenarioResult({ frame }));

	Expect(ledger.npcBlockedMovementCount == 1, "blocked scenario ledger should report blocked movement");
	Expect(ledger.npcMovedCount == 0 && ledger.npcRefreshDirtyTileCount == 0, "blocked scenario ledger should not report movement refresh");
	Expect(HasEvent(ledger, iggy::runtime::RuntimeGameplayScenarioLedgerEvent::NpcActorBlocked), "blocked scenario ledger should emit blocked event");
	Expect(!HasEvent(ledger, iggy::runtime::RuntimeGameplayScenarioLedgerEvent::RefreshOccupancy), "blocked scenario ledger should not emit refresh event");
}

void TestMultiFrameOrderAndAggregateParity()
{
	iggy::runtime::RuntimeGameplayOrchestratedFrameReport unchanged = FrameReport();
	iggy::runtime::RuntimeGameplayOrchestratedFrameReport changed = FrameReport();
	changed.npcMovedCount = 1;
	changed.npcActorsChanged = true;
	changed.npcRefreshDirtyTileCount = 2;
	changed.npcRenderRefreshed = true;
	changed.npcVisibilityRefreshed = true;
	const iggy::runtime::RuntimeGameplayScenarioResult result =
		ScenarioResult({ unchanged, changed });
	const iggy::runtime::RuntimeGameplayScenarioLedger ledger = Ledger(result);

	Expect(ledger.frames.size() == 2, "multi-frame ledger should preserve row count");
	Expect(!ledger.frames[0].changed && ledger.frames[1].changed, "multi-frame ledger should preserve row order and changed status");
	Expect(ledger.frameCount == result.frameCount && ledger.changedFrameCount == result.changedFrameCount, "ledger should match scenario frame counts");
	Expect(ledger.npcMovedCount == result.npcMovedCount, "ledger should match scenario moved count");
	Expect(ledger.npcRefreshDirtyTileCount == result.npcRefreshDirtyTileCount, "ledger should match scenario dirty count");
	if (ledger.events.size() >= 4) {
		Expect(ledger.events[0] == iggy::runtime::RuntimeGameplayScenarioLedgerEvent::ScenarioStarted, "ledger event order should start with scenario start");
		Expect(ledger.events[1] == iggy::runtime::RuntimeGameplayScenarioLedgerEvent::FrameUnchanged, "first frame event should be unchanged");
		Expect(ledger.events[2] == iggy::runtime::RuntimeGameplayScenarioLedgerEvent::FrameChanged, "second frame event should be changed");
	}
}

void TestReporterDoesNotMutateInput()
{
	iggy::runtime::RuntimeGameplayOrchestratedFrameReport frame = FrameReport();
	frame.npcMovedCount = 1;
	frame.npcActorsChanged = true;
	iggy::runtime::RuntimeGameplayScenarioResult result = ScenarioResult({ frame });
	const iggy::runtime::RuntimeGameplayScenarioResult before = result;

	const iggy::runtime::RuntimeGameplayScenarioLedger ledger = Ledger(result);

	Expect(ledger.npcMovedCount == 1, "immutability setup should produce movement fact");
	Expect(result.report.frames.size() == before.report.frames.size(), "ledger reporter should not mutate report frames");
	Expect(result.npcMovedCount == before.npcMovedCount, "ledger reporter should not mutate source summary counts");
}

void TestProfileScenarioFeedsLedger()
{
	iggy::runtime::RuntimeGameplayState state = State();
	state.session.level.map = LevelMap("level:ledger-profile", { "..." });
	state.npcActors = Actors({ Actor("npc:profile-ledger", "profile:ledger-profile") });
	state.npcControls = Controls({ Control("npc:profile-ledger", { 2.5F, 0.5F }, iggy::NpcMoveMode::Still) });
	iggy::runtime::RuntimeGameplayProfileScenarioDefinition definition;
	definition.hasScenarioId = true;
	definition.scenarioId = Id("scenario:ledger-profile");
	definition.initialState = state;
	definition.profileTraits = Catalog({ { Id("profile:ledger-profile"), Traits(12) } });
	definition.frames = { ProfileFrame() };

	const iggy::runtime::RuntimeGameplayProfileScenarioDefinitionBuildResult build =
		iggy::runtime::RuntimeGameplayProfileScenarioDefinitionBuilder {}.build(definition);
	const iggy::runtime::RuntimeGameplayScenarioResult run =
		iggy::runtime::RuntimeGameplayScenarioRunner {}.run(build.scenario);
	const iggy::runtime::RuntimeGameplayScenarioLedger ledger =
		iggy::runtime::RuntimeGameplayScenarioLedgerReporter {}.report(build, run);

	Expect(build.resolvedSubjectCount == 1, "profile ledger setup should resolve one subject");
	Expect(ledger.hasProfileBuild, "profile ledger should preserve profile build facts");
	Expect(ledger.profileBuild.resolvedSubjectCount == 1, "profile ledger should copy profile build result");
	Expect(ledger.npcControlAppliedCount == 1 && ledger.npcMovedCount == 1, "profile ledger should report normal scenario execution facts");
}

} // namespace

int main()
{
	TestEmptyScenarioResult();
	TestSingleFrameMovedNpcScenario();
	TestInventoryOnlyScenario();
	TestBlockedNpcScenario();
	TestMultiFrameOrderAndAggregateParity();
	TestReporterDoesNotMutateInput();
	TestProfileScenarioFeedsLedger();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
