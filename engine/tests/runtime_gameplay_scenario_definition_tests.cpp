#include <cstdlib>
#include <string_view>
#include <vector>

#include "runtime/RuntimeGameplayScenarioDefinition.hpp"
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

iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerFrame Frame(const char *levelId)
{
	iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerFrame frame;
	frame.movementMap = LevelMap(levelId, { ".." });
	return frame;
}

void TestEmptyDefinitionBuildsEmptyScenario()
{
	const iggy::runtime::RuntimeGameplayScenarioDefinition definition;

	const iggy::runtime::RuntimeGameplayScenarioDefinitionBuildResult result =
		iggy::runtime::RuntimeGameplayScenarioDefinitionBuilder {}.build(definition);

	Expect(result.built, "empty scenario definition should build");
	Expect(result.frameCount == 0, "empty scenario definition should report zero frames");
	Expect(result.scenario.frames.empty(), "empty scenario definition should produce empty scenario");
}

void TestDefinitionConversionPreservesStateAndFrameOrder()
{
	iggy::runtime::RuntimeGameplayScenarioDefinition definition;
	definition.hasScenarioId = true;
	definition.scenarioId = Id("scenario:ordered");
	definition.initialState.session.level.map = LevelMap("level:initial", { "..." });
	definition.frames = {
		{ true, Id("frame:first"), Frame("level:first") },
		{ true, Id("frame:second"), Frame("level:second") },
	};

	const iggy::runtime::RuntimeGameplayScenarioDefinitionBuildResult result =
		iggy::runtime::RuntimeGameplayScenarioDefinitionBuilder {}.build(definition);

	Expect(result.built, "scenario definition should build");
	Expect(result.definition.scenarioId == Id("scenario:ordered"), "build result should copy scenario id");
	Expect(result.scenario.initialState.session.level.map.id == Id("level:initial"), "scenario should preserve initial state");
	Expect(result.scenario.frames.size() == 2, "scenario should preserve frame count");
	if (result.scenario.frames.size() == 2) {
		Expect(result.scenario.frames[0].frame.movementMap.id == Id("level:first"), "scenario should preserve first frame");
		Expect(result.scenario.frames[1].frame.movementMap.id == Id("level:second"), "scenario should preserve second frame");
	}
}

void TestBuildDoesNotMutateDefinition()
{
	iggy::runtime::RuntimeGameplayScenarioDefinition definition;
	definition.frames = { { true, Id("frame:copy"), Frame("level:copy") } };
	const iggy::runtime::RuntimeGameplayScenarioDefinition before = definition;

	const iggy::runtime::RuntimeGameplayScenarioDefinitionBuildResult result =
		iggy::runtime::RuntimeGameplayScenarioDefinitionBuilder {}.build(definition);

	Expect(result.built, "scenario definition copy setup should build");
	Expect(definition.frames.size() == before.frames.size(), "builder should not mutate frame count");
	if (!definition.frames.empty() && !before.frames.empty()) {
		Expect(definition.frames[0].frameId == before.frames[0].frameId, "builder should not mutate frame id");
		Expect(definition.frames[0].frame.movementMap.id == before.frames[0].frame.movementMap.id, "builder should not mutate frame payload");
	}
}

} // namespace

int main()
{
	TestEmptyDefinitionBuildsEmptyScenario();
	TestDefinitionConversionPreservesStateAndFrameOrder();
	TestBuildDoesNotMutateDefinition();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
