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

iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterResult Convert(
	const iggy::runtime::RuntimeGameplayScenarioAuthoringPacket &packet,
	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterConfig &config)
{
	return iggy::runtime::RuntimeGameplayScenarioAuthoringAdapter {}.convert(packet, config);
}

iggy::runtime::RuntimeGameplayAsciiSourcePlan SourcePlan()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan;
	plan.formatId = Id("iggy:ascii-source-plan");
	plan.hasSourceId = true;
	plan.sourceId = Id("scenario:authoring-source-plan");
	plan.grid.width = 3;
	plan.grid.height = 3;
	plan.grid.backgroundGlyph = '.';
	plan.grid.rows = {
		"###",
		"#A#",
		"###",
	};
	plan.legend = {
		{
			'A',
			iggy::runtime::RuntimeGameplayAsciiSourcePlanGlyphKind::Actor,
			Id("role:npc"),
			{ Id("tag:guard") },
			true,
			iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::Actor,
			Id("npc:source-plan"),
			Id("profile:authoring"),
		},
	};
	plan.annotatedCells = {
		{
			true,
			Id("cell:source-plan"),
			1,
			1,
			'A',
			{ true, 1, 1 },
			{ true, 1.5F, 1.5F },
			{ false },
			{ Id("tag:guard") },
			Id("npc:source-plan"),
			Id("profile:authoring"),
		},
	};
	return plan;
}

iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterConfig SourcePlanConfig()
{
	iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterConfig config;
	config.hasAsciiSourcePlanProfileScenarioConfig = true;
	config.asciiSourcePlanProfileScenario.hasDefaultFrame = true;
	config.asciiSourcePlanProfileScenario.defaultFrame = Frame();
	config.asciiSourcePlanProfileScenario.profileTraits = Catalog();
	return config;
}

void TestDefaultPacketIsUnsupported()
{
	const iggy::runtime::RuntimeGameplayScenarioAuthoringPacket packet;

	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterResult result = Convert(packet);

	Expect(result.status == iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterStatus::UnsupportedSource, "default authoring packet should be unsupported");
	Expect(!result.ok() && !result.converted, "unsupported authoring packet should not convert");
	Expect(result.packet.source == packet.source, "unsupported authoring result should preserve packet");
	Expect(result.hasIssues() && result.issueCount == 1, "unsupported authoring packet should report one issue");
	if (!result.issues.empty()) {
		Expect(result.issues[0].code == iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterIssueCode::UnsupportedSource, "unsupported authoring issue should use UnsupportedSource");
		Expect(result.issues[0].source == iggy::runtime::RuntimeGameplayScenarioAuthoringSource::Unsupported, "unsupported authoring issue should preserve source");
		Expect(!result.issues[0].hasProfileScenario, "unsupported authoring issue should preserve missing payload flag");
		Expect(!result.issues[0].hasAsciiSourcePlan, "unsupported authoring issue should preserve missing source-plan flag");
	}
}

void TestDeclaredProfileScenarioWithoutPayloadIsInvalid()
{
	iggy::runtime::RuntimeGameplayScenarioAuthoringPacket packet;
	packet.source = iggy::runtime::RuntimeGameplayScenarioAuthoringSource::ProfileScenarioDefinition;

	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterResult result = Convert(packet);

	Expect(result.status == iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterStatus::InvalidPacket, "missing profile payload should be invalid");
	Expect(!result.ok() && !result.converted, "invalid authoring packet should not convert");
	Expect(result.hasIssues() && result.issueCount == 1, "missing profile payload should report one issue");
	if (!result.issues.empty()) {
		Expect(result.issues[0].code == iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterIssueCode::MissingProfileScenarioPayload, "missing profile payload issue should use MissingProfileScenarioPayload");
		Expect(result.issues[0].source == iggy::runtime::RuntimeGameplayScenarioAuthoringSource::ProfileScenarioDefinition, "missing profile payload issue should preserve source");
		Expect(!result.issues[0].hasProfileScenario, "missing profile payload issue should preserve payload flag");
	}
}

void TestDeclaredAsciiSourcePlanWithoutPayloadIsInvalid()
{
	iggy::runtime::RuntimeGameplayScenarioAuthoringPacket packet;
	packet.source = iggy::runtime::RuntimeGameplayScenarioAuthoringSource::AsciiSourcePlan;
	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterConfig config =
		SourcePlanConfig();
	const iggy::runtime::RuntimeGameplayScenarioAuthoringPacket before = packet;
	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterConfig configBefore = config;

	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterResult result =
		Convert(packet, config);

	Expect(result.status == iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterStatus::InvalidPacket, "missing source-plan payload should be invalid");
	Expect(!result.ok() && !result.converted, "missing source-plan payload should not convert");
	Expect(result.hasIssues() && result.issueCount == 1, "missing source-plan payload should report one issue");
	if (!result.issues.empty()) {
		Expect(result.issues[0].code == iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterIssueCode::MissingAsciiSourcePlanPayload, "missing source-plan payload issue should use MissingAsciiSourcePlanPayload");
		Expect(result.issues[0].source == iggy::runtime::RuntimeGameplayScenarioAuthoringSource::AsciiSourcePlan, "missing source-plan payload issue should preserve source");
		Expect(!result.issues[0].hasAsciiSourcePlan, "missing source-plan payload issue should preserve payload flag");
		Expect(result.issues[0].hasAsciiSourcePlanConversionConfig, "missing source-plan payload issue should preserve config flag");
	}
	Expect(packet.source == before.source && packet.hasAsciiSourcePlan == before.hasAsciiSourcePlan, "missing source-plan payload path should not mutate packet");
	Expect(config.hasAsciiSourcePlanProfileScenarioConfig == configBefore.hasAsciiSourcePlanProfileScenarioConfig, "missing source-plan payload path should not mutate config");
}

void TestAsciiSourcePlanWithoutConversionConfigIsInvalid()
{
	iggy::runtime::RuntimeGameplayScenarioAuthoringPacket packet;
	packet.source = iggy::runtime::RuntimeGameplayScenarioAuthoringSource::AsciiSourcePlan;
	packet.hasAsciiSourcePlan = true;
	packet.asciiSourcePlan = SourcePlan();
	const iggy::runtime::RuntimeGameplayScenarioAuthoringPacket before = packet;

	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterResult result =
		Convert(packet);

	Expect(result.status == iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterStatus::InvalidPacket, "source-plan without conversion config should be invalid");
	Expect(!result.ok() && !result.converted, "source-plan without conversion config should not convert");
	Expect(result.hasIssues() && result.issueCount == 1, "source-plan without conversion config should report one issue");
	if (!result.issues.empty()) {
		Expect(result.issues[0].code == iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterIssueCode::MissingAsciiSourcePlanConversionConfig, "source-plan without config issue should use MissingAsciiSourcePlanConversionConfig");
		Expect(result.issues[0].source == iggy::runtime::RuntimeGameplayScenarioAuthoringSource::AsciiSourcePlan, "source-plan without config issue should preserve source");
		Expect(result.issues[0].hasAsciiSourcePlan, "source-plan without config issue should preserve payload flag");
		Expect(!result.issues[0].hasAsciiSourcePlanConversionConfig, "source-plan without config issue should preserve missing config flag");
	}
	Expect(result.packet.asciiSourcePlan.sourceId == Id("scenario:authoring-source-plan"), "source-plan without config result should preserve packet");
	Expect(packet.asciiSourcePlan.sourceId == before.asciiSourcePlan.sourceId, "source-plan without config path should not mutate packet");
}

void TestAsciiSourcePlanDelegatesToConverter()
{
	iggy::runtime::RuntimeGameplayScenarioAuthoringPacket packet;
	packet.source = iggy::runtime::RuntimeGameplayScenarioAuthoringSource::AsciiSourcePlan;
	packet.hasAsciiSourcePlan = true;
	packet.asciiSourcePlan = SourcePlan();
	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterConfig config =
		SourcePlanConfig();
	const iggy::runtime::RuntimeGameplayScenarioAuthoringPacket before = packet;
	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterConfig configBefore = config;

	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterResult result =
		Convert(packet, config);

	Expect(result.ok(), "valid source-plan packet should convert through adapter");
	Expect(result.status == iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterStatus::Converted, "valid source-plan packet should report Converted");
	Expect(result.converted, "valid source-plan packet should mark converted");
	Expect(!result.hasIssues() && result.issueCount == 0, "valid source-plan conversion should not add adapter issues");
	Expect(result.asciiSourcePlanConversion.ok(), "adapter should preserve successful nested conversion result");
	Expect(result.asciiSourcePlanConversion.definition.scenarioId == Id("scenario:authoring-source-plan"), "nested conversion should preserve source scenario id");
	Expect(result.profileScenario.scenarioId == result.asciiSourcePlanConversion.definition.scenarioId, "adapter should publish converted profile scenario definition");
	Expect(result.profileScenario.initialState.npcActors.actors.size() == 1, "published profile scenario should include converted actor");
	Expect(result.profileScenario.initialState.npcControls.entries.size() == 1, "published profile scenario should include converted control");
	Expect(result.profileScenario.frames.size() == 1, "published profile scenario should include default frame");
	Expect(packet.asciiSourcePlan.sourceId == before.asciiSourcePlan.sourceId, "source-plan delegation should not mutate packet");
	Expect(config.hasAsciiSourcePlanProfileScenarioConfig == configBefore.hasAsciiSourcePlanProfileScenarioConfig, "source-plan delegation should not mutate config");
}

void TestInvalidAsciiSourcePlanPreservesNestedConversionFailure()
{
	iggy::runtime::RuntimeGameplayScenarioAuthoringPacket packet;
	packet.source = iggy::runtime::RuntimeGameplayScenarioAuthoringSource::AsciiSourcePlan;
	packet.hasAsciiSourcePlan = true;
	packet.asciiSourcePlan = SourcePlan();
	packet.asciiSourcePlan.grid.rows[1] = "#Z#";
	const iggy::runtime::RuntimeGameplayScenarioAuthoringPacket before = packet;
	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterConfig config =
		SourcePlanConfig();

	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterResult result =
		Convert(packet, config);

	Expect(result.status == iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterStatus::ConversionFailed, "invalid source-plan conversion should report ConversionFailed");
	Expect(!result.ok() && !result.converted, "invalid source-plan conversion should not publish as converted");
	Expect(!result.asciiSourcePlanConversion.ok(), "adapter should preserve failed nested conversion result");
	Expect(result.asciiSourcePlanConversion.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::SourcePlanInvalid, "adapter should preserve source-plan invalid nested status");
	Expect(result.profileScenario.frames.empty(), "failed source-plan conversion should not publish profile scenario frames");
	Expect(result.hasIssues() && result.issueCount == 1, "failed source-plan conversion should report one adapter issue");
	if (!result.issues.empty()) {
		Expect(result.issues[0].code == iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterIssueCode::AsciiSourcePlanConversionFailed, "failed source-plan conversion issue should use AsciiSourcePlanConversionFailed");
		Expect(result.issues[0].asciiSourcePlanConversionStatus == iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::SourcePlanInvalid, "failed source-plan conversion issue should preserve nested status");
	}
	Expect(packet.asciiSourcePlan.grid.rows == before.asciiSourcePlan.grid.rows, "invalid source-plan conversion should not mutate packet");
}

void TestMissingFrameDefaultsPreservesNestedConversionFailure()
{
	iggy::runtime::RuntimeGameplayScenarioAuthoringPacket packet;
	packet.source = iggy::runtime::RuntimeGameplayScenarioAuthoringSource::AsciiSourcePlan;
	packet.hasAsciiSourcePlan = true;
	packet.asciiSourcePlan = SourcePlan();
	iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterConfig config;
	config.hasAsciiSourcePlanProfileScenarioConfig = true;
	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterConfig configBefore = config;

	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterResult result =
		Convert(packet, config);

	Expect(result.status == iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterStatus::ConversionFailed, "missing frame defaults should map to ConversionFailed");
	Expect(result.asciiSourcePlanConversion.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::MissingFrameDefaults, "missing frame defaults should preserve nested converter status");
	Expect(result.profileScenario.frames.empty(), "missing frame defaults should not publish profile scenario");
	Expect(result.hasIssues() && result.issueCount == 1, "missing frame defaults should report one adapter issue");
	if (!result.issues.empty())
		Expect(result.issues[0].asciiSourcePlanConversionStatus == iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::MissingFrameDefaults, "missing frame defaults issue should preserve nested status");
	Expect(config.hasAsciiSourcePlanProfileScenarioConfig == configBefore.hasAsciiSourcePlanProfileScenarioConfig, "missing frame defaults path should not mutate config");
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
	Expect(!result.hasIssues() && result.issueCount == 0, "converted profile scenario should have no adapter issues");
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
	Expect(result.hasIssues() && result.issueCount == 1, "unsupported source with payload should report one issue");
	if (!result.issues.empty())
		Expect(result.issues[0].hasProfileScenario, "unsupported source issue should preserve payload-present flag");
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
	TestDeclaredAsciiSourcePlanWithoutPayloadIsInvalid();
	TestAsciiSourcePlanWithoutConversionConfigIsInvalid();
	TestAsciiSourcePlanDelegatesToConverter();
	TestInvalidAsciiSourcePlanPreservesNestedConversionFailure();
	TestMissingFrameDefaultsPreservesNestedConversionFailure();
	TestProfileScenarioPacketConvertsByValue();
	TestConvertedProfileScenarioFeedsExistingBuilder();
	TestUnsupportedSourceWithPayloadDoesNotConvert();
	TestAdapterDoesNotMutatePacket();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
