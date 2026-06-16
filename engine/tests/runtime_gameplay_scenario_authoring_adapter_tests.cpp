#include <cstdlib>
#include <string_view>
#include <vector>

#include "runtime/RuntimeGameplayScenarioAuthoringAdapter.hpp"
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

iggy::NpcActorState2D Actor(const char *npcId, const char *profileId)
{
	return {
		Id(npcId),
		Id(profileId),
		Id("faction:authoring"),
		{ 0.5F, 0.5F },
		Id("goal:authoring"),
		true,
	};
}

iggy::NpcActorState2DRegistry Actors(std::vector<iggy::NpcActorState2D> actors)
{
	const iggy::NpcActorState2DRegistryBuildResult result =
		iggy::NpcActorState2DRegistryBuilder {}.build(actors);
	Expect(result.built, "authoring actors should build");
	return result.registry;
}

iggy::NpcActorControlState2D Control(const char *npcId)
{
	return {
		Id(npcId),
		iggy::moveToNpcObjective({ 2.5F, 0.5F }),
		iggy::seekingNpcBehaviorState({ 2.5F, 0.5F }),
		iggy::NpcMoveMode::Still,
	};
}

iggy::NpcActorControlState2DRegistry Controls(std::vector<iggy::NpcActorControlState2D> controls)
{
	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);
	Expect(result.built, "authoring controls should build");
	return result.registry;
}

iggy::NpcTraitSet Traits()
{
	iggy::NpcTraitSet traits;
	traits.strength = 12;
	return traits;
}

iggy::NpcAiProfileTraitCatalog Catalog()
{
	const iggy::NpcAiProfileTraitCatalogBuildResult result =
		iggy::NpcAiProfileTraitCatalogBuilder {}.build(
			{ { Id("profile:authoring"), Traits() } });
	Expect(result.built, "authoring catalog should build");
	return result.catalog;
}

iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition Frame()
{
	iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition frame;
	frame.hasFrameId = true;
	frame.frameId = Id("frame:authoring");
	frame.movementMap = LevelMap("level:authoring-frame", { "..." });
	return frame;
}

iggy::runtime::RuntimeGameplayProfileScenarioDefinition ProfileScenario()
{
	iggy::runtime::RuntimeGameplayState state;
	state.session.level.map = LevelMap("level:authoring", { "..." });
	state.npcActors = Actors({ Actor("npc:authoring", "profile:authoring") });
	state.npcControls = Controls({ Control("npc:authoring") });

	iggy::runtime::RuntimeGameplayProfileScenarioDefinition definition;
	definition.hasScenarioId = true;
	definition.scenarioId = Id("scenario:authoring");
	definition.initialState = state;
	definition.profileTraits = Catalog();
	definition.frames = { Frame() };
	return definition;
}

iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterResult Convert(
	const iggy::runtime::RuntimeGameplayScenarioAuthoringPacket &packet)
{
	return iggy::runtime::RuntimeGameplayScenarioAuthoringAdapter {}.convert(packet);
}

void TestDefaultPacketIsUnsupported()
{
	const iggy::runtime::RuntimeGameplayScenarioAuthoringPacket packet;

	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterResult result = Convert(packet);

	Expect(result.status == iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterStatus::UnsupportedSource, "default authoring packet should be unsupported");
	Expect(!result.ok() && !result.converted, "unsupported authoring packet should not convert");
	Expect(result.packet.source == packet.source, "unsupported authoring result should preserve packet");
}

void TestDeclaredProfileScenarioWithoutPayloadIsInvalid()
{
	iggy::runtime::RuntimeGameplayScenarioAuthoringPacket packet;
	packet.source = iggy::runtime::RuntimeGameplayScenarioAuthoringSource::ProfileScenarioDefinition;

	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterResult result = Convert(packet);

	Expect(result.status == iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterStatus::InvalidPacket, "missing profile payload should be invalid");
	Expect(!result.ok() && !result.converted, "invalid authoring packet should not convert");
}

void TestProfileScenarioPacketConvertsByValue()
{
	iggy::runtime::RuntimeGameplayScenarioAuthoringPacket packet;
	packet.source = iggy::runtime::RuntimeGameplayScenarioAuthoringSource::ProfileScenarioDefinition;
	packet.hasProfileScenario = true;
	packet.profileScenario = ProfileScenario();

	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterResult result = Convert(packet);

	Expect(result.ok(), "profile scenario authoring packet should convert");
	Expect(result.status == iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterStatus::Converted, "profile scenario authoring packet should report converted");
	Expect(result.profileScenario.scenarioId == Id("scenario:authoring"), "profile scenario authoring adapter should preserve scenario id");
	Expect(result.profileScenario.frames.size() == 1, "profile scenario authoring adapter should preserve frames");
	Expect(result.profileScenario.initialState.npcActors.actors.size() == 1, "profile scenario authoring adapter should preserve initial actors");
}

void TestConvertedProfileScenarioFeedsExistingBuilder()
{
	iggy::runtime::RuntimeGameplayScenarioAuthoringPacket packet;
	packet.source = iggy::runtime::RuntimeGameplayScenarioAuthoringSource::ProfileScenarioDefinition;
	packet.hasProfileScenario = true;
	packet.profileScenario = ProfileScenario();

	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterResult converted = Convert(packet);
	const iggy::runtime::RuntimeGameplayProfileScenarioDefinitionBuildResult build =
		iggy::runtime::RuntimeGameplayProfileScenarioDefinitionBuilder {}.build(
			converted.profileScenario);

	Expect(converted.ok(), "builder acceptance setup should convert profile scenario");
	Expect(build.built, "converted profile scenario should feed existing builder");
	Expect(build.frameCount == 1 && build.resolvedSubjectCount == 1, "converted profile scenario should preserve build facts");
}

void TestUnsupportedSourceWithPayloadDoesNotConvert()
{
	iggy::runtime::RuntimeGameplayScenarioAuthoringPacket packet;
	packet.source = iggy::runtime::RuntimeGameplayScenarioAuthoringSource::Unsupported;
	packet.hasProfileScenario = true;
	packet.profileScenario = ProfileScenario();

	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterResult result = Convert(packet);

	Expect(result.status == iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterStatus::UnsupportedSource, "unsupported source should not be converted even with payload");
	Expect(result.profileScenario.frames.empty(), "unsupported source should not copy converted profile scenario");
}

void TestAdapterDoesNotMutatePacket()
{
	iggy::runtime::RuntimeGameplayScenarioAuthoringPacket packet;
	packet.source = iggy::runtime::RuntimeGameplayScenarioAuthoringSource::ProfileScenarioDefinition;
	packet.hasProfileScenario = true;
	packet.profileScenario = ProfileScenario();
	const iggy::runtime::RuntimeGameplayScenarioAuthoringPacket before = packet;

	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterResult result = Convert(packet);

	Expect(result.ok(), "immutability setup should convert");
	Expect(packet.source == before.source, "authoring adapter should not mutate source");
	Expect(packet.hasProfileScenario == before.hasProfileScenario, "authoring adapter should not mutate has-profile flag");
	Expect(packet.profileScenario.scenarioId == before.profileScenario.scenarioId, "authoring adapter should not mutate profile scenario");
}

} // namespace

int main()
{
	TestDefaultPacketIsUnsupported();
	TestDeclaredProfileScenarioWithoutPayloadIsInvalid();
	TestProfileScenarioPacketConvertsByValue();
	TestConvertedProfileScenarioFeedsExistingBuilder();
	TestUnsupportedSourceWithPayloadDoesNotConvert();
	TestAdapterDoesNotMutatePacket();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
