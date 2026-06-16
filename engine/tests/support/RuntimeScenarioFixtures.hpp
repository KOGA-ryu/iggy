#pragma once

#include <string_view>
#include <vector>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "runtime/RuntimeGameplayProfileScenarioDefinition.hpp"
#include "scene/ai/AiMap2D.hpp"
#include "scene/ai/NpcCharismaPool.hpp"
#include "scene/ai/NpcConstitutionPool.hpp"
#include "scene/ai/NpcDexterityPool.hpp"
#include "scene/ai/NpcIntelligencePool.hpp"
#include "scene/ai/NpcStrengthPool.hpp"
#include "scene/ai/NpcWisdomPool.hpp"
#include "scene/npc/NpcActorControlState2D.hpp"
#include "scene/npc/NpcActorState2D.hpp"
#include "LevelMapFixtures.hpp"
#include "TestHarness.hpp"

namespace iggy::test::scenario {

inline ResourceId Id(const char *value)
{
	return ResourceId { value };
}

inline LevelTileMap LevelMap(
	const char *id,
	std::vector<std::string_view> rows = { "...." })
{
	LevelTileMap map = iggy::test::MapFromRows(rows);
	map.id = Id(id);
	return map;
}

inline NpcTraitSet Traits(int strength = 12)
{
	NpcTraitSet traits;
	traits.strength = strength;
	return traits;
}

inline NpcAiProfileTraitEntry Profile(
	const char *profileId,
	NpcTraitSet traits = Traits())
{
	return { Id(profileId), traits };
}

inline NpcAiProfileTraitCatalog ProfileCatalog(
	std::vector<NpcAiProfileTraitEntry> entries)
{
	const NpcAiProfileTraitCatalogBuildResult result =
		NpcAiProfileTraitCatalogBuilder {}.build(entries);
	iggy::test::Expect(result.built, "scenario profile catalog should build");
	return result.catalog;
}

inline NpcActorState2D Actor(
	const char *npcId,
	const char *profileId,
	Vec2 position = { 0.5F, 0.5F },
	bool present = true)
{
	return {
		Id(npcId),
		Id(profileId),
		Id("faction:scenario-fixture"),
		position,
		Id("goal:scenario-fixture"),
		present,
	};
}

inline NpcActorState2DRegistry Actors(std::vector<NpcActorState2D> actors)
{
	const NpcActorState2DRegistryBuildResult result =
		NpcActorState2DRegistryBuilder {}.build(actors);
	iggy::test::Expect(result.built, "scenario actors should build");
	return result.registry;
}

inline NpcActorControlState2D Control(
	const char *npcId,
	Vec2 target = { 2.5F, 0.5F },
	NpcMoveMode moveMode = NpcMoveMode::Still)
{
	return {
		Id(npcId),
		moveToNpcObjective(target),
		seekingNpcBehaviorState(target),
		moveMode,
	};
}

inline NpcActorControlState2DRegistry Controls(
	std::vector<NpcActorControlState2D> controls)
{
	const NpcActorControlState2DRegistryBuildResult result =
		NpcActorControlState2DRegistryBuilder {}.build(controls);
	iggy::test::Expect(result.built, "scenario controls should build");
	return result.registry;
}

inline AiMap2D AiMap(std::vector<AiMapNode2D> nodes = {})
{
	const AiMap2DBuildResult result = AiMap2DBuilder {}.build(nodes);
	iggy::test::Expect(result.built, "scenario AI map should build");
	return result.map;
}

inline NpcStrengthEnt StrengthEnt(
	const char *entryId = "strength:scenario-fixture",
	const char *actionTag = "action:scenario-fixture",
	NpcBehaviorStateType behavior = NpcBehaviorStateType::Seeking,
	float weight = 1.0F)
{
	return { Id(entryId), 0, behavior, Id(actionTag), weight, {} };
}

inline NpcMapPlayControlFramePlanPools Pools(
	std::vector<NpcStrengthEnt> strength = { StrengthEnt() })
{
	NpcMapPlayControlFramePlanPools pools;
	const NpcStrengthPoolBuildResult strengthBuild =
		NpcStrengthPoolBuilder {}.build(strength);
	const NpcDexterityPoolBuildResult dexterityBuild =
		NpcDexterityPoolBuilder {}.build({});
	const NpcConstitutionPoolBuildResult constitutionBuild =
		NpcConstitutionPoolBuilder {}.build({});
	const NpcIntelligencePoolBuildResult intelligenceBuild =
		NpcIntelligencePoolBuilder {}.build({});
	const NpcWisdomPoolBuildResult wisdomBuild =
		NpcWisdomPoolBuilder {}.build({});
	const NpcCharismaPoolBuildResult charismaBuild =
		NpcCharismaPoolBuilder {}.build({});

	iggy::test::Expect(strengthBuild.built, "scenario strength pool should build");
	iggy::test::Expect(dexterityBuild.built, "scenario dexterity pool should build");
	iggy::test::Expect(constitutionBuild.built, "scenario constitution pool should build");
	iggy::test::Expect(intelligenceBuild.built, "scenario intelligence pool should build");
	iggy::test::Expect(wisdomBuild.built, "scenario wisdom pool should build");
	iggy::test::Expect(charismaBuild.built, "scenario charisma pool should build");

	pools.strength = strengthBuild.pool;
	pools.dexterity = dexterityBuild.pool;
	pools.constitution = constitutionBuild.pool;
	pools.intelligence = intelligenceBuild.pool;
	pools.wisdom = wisdomBuild.pool;
	pools.charisma = charismaBuild.pool;
	return pools;
}

inline runtime::RuntimeGameplayState GameplayState(
	NpcActorState2DRegistry actors = {},
	NpcActorControlState2DRegistry controls = {},
	LevelTileMap map = LevelMap("level:scenario-fixture-state"))
{
	runtime::RuntimeGameplayState state;
	state.session.level.map = map;
	state.npcActors = actors;
	state.npcControls = controls;
	return state;
}

inline runtime::RuntimeGameplayProfileScenarioFrameDefinition ProfileFrame(
	const char *frameId = "frame:scenario-fixture",
	LevelTileMap movementMap = LevelMap("level:scenario-fixture-frame"))
{
	runtime::RuntimeGameplayProfileScenarioFrameDefinition frame;
	frame.hasFrameId = true;
	frame.frameId = Id(frameId);
	frame.pools = Pools();
	frame.aiMap = AiMap();
	frame.movementMap = movementMap;
	frame.refreshAiMap = AiMap();
	return frame;
}

inline runtime::RuntimeGameplayProfileScenarioDefinition ProfileScenario(
	runtime::RuntimeGameplayState state,
	NpcAiProfileTraitCatalog catalog,
	std::vector<runtime::RuntimeGameplayProfileScenarioFrameDefinition> frames,
	const char *scenarioId = "scenario:scenario-fixture")
{
	runtime::RuntimeGameplayProfileScenarioDefinition definition;
	definition.hasScenarioId = true;
	definition.scenarioId = Id(scenarioId);
	definition.initialState = state;
	definition.profileTraits = catalog;
	definition.frames = frames;
	return definition;
}

inline runtime::RuntimeGameplayProfileScenarioDefinition MinimalMovingProfileScenario(
	const char *npcId = "npc:scenario-fixture",
	const char *profileId = "profile:scenario-fixture")
{
	return ProfileScenario(
		GameplayState(
			Actors({ Actor(npcId, profileId, { 0.5F, 0.5F }) }),
			Controls({ Control(npcId, { 2.5F, 0.5F }) })),
		ProfileCatalog({ Profile(profileId) }),
		{ ProfileFrame() });
}

} // namespace iggy::test::scenario
