#include <cstdlib>

#include "runtime/RuntimeGameplayProfileScenarioRunner.hpp"
#include "runtime/RuntimeGameplayScenarioAuthoringAdapter.hpp"
#include "support/RuntimeScenarioFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
namespace scenario = iggy::test::scenario;

void TestMinimalProfileScenarioRunsThroughExistingRunner()
{
	const iggy::runtime::RuntimeGameplayProfileScenarioDefinition definition =
		scenario::MinimalMovingProfileScenario("npc:fixture-runner", "profile:fixture-runner");

	const iggy::runtime::RuntimeGameplayProfileScenarioRunResult result =
		iggy::runtime::RuntimeGameplayProfileScenarioRunner {}.run(definition);

	Expect(result.ran(), "fixture profile scenario should run");
	Expect(result.validation.ok(), "fixture profile scenario should validate");
	Expect(result.build.resolvedSubjectCount == 1, "fixture profile scenario should resolve one subject");
	Expect(result.npcControlAppliedCount == 1, "fixture profile scenario should apply one control");
	Expect(result.npcMovedCount == 1, "fixture profile scenario should move one actor");
	Expect(result.state.npcActors.actors.size() == 1, "fixture profile scenario should preserve actor registry");
	if (!result.state.npcActors.actors.empty())
		Expect(result.state.npcActors.actors[0].npcId == scenario::Id("npc:fixture-runner"), "fixture profile scenario should preserve exact npc id");
}

void TestFixtureProfileScenarioFeedsAuthoringAdapterByValue()
{
	iggy::runtime::RuntimeGameplayScenarioAuthoringPacket packet;
	packet.source = iggy::runtime::RuntimeGameplayScenarioAuthoringSource::ProfileScenarioDefinition;
	packet.hasProfileScenario = true;
	packet.profileScenario =
		scenario::MinimalMovingProfileScenario("npc:fixture-authoring", "profile:fixture-authoring");
	const iggy::runtime::RuntimeGameplayScenarioAuthoringPacket before = packet;

	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterResult result =
		iggy::runtime::RuntimeGameplayScenarioAuthoringAdapter {}.convert(packet);

	Expect(result.ok(), "fixture profile scenario authoring packet should convert");
	Expect(result.profileScenario.scenarioId == scenario::Id("scenario:scenario-fixture"), "fixture authoring adapter should preserve scenario id");
	Expect(result.profileScenario.initialState.npcActors.actors.size() == 1, "fixture authoring adapter should preserve actor registry");
	Expect(packet.profileScenario.scenarioId == before.profileScenario.scenarioId, "fixture authoring adapter should not mutate packet scenario");
	Expect(packet.profileScenario.initialState.npcActors.actors[0].aiProfileId == before.profileScenario.initialState.npcActors.actors[0].aiProfileId, "fixture authoring adapter should not mutate profile ids");
}

void TestFixtureHelpersPreserveNamespacedAndUnqualifiedIds()
{
	const iggy::runtime::RuntimeGameplayProfileScenarioDefinition definition =
		scenario::ProfileScenario(
			scenario::GameplayState(
				scenario::Actors({
					scenario::Actor("npc:namespaced", "profile:namespaced"),
					scenario::Actor("unqualified", "plain-profile", { 1.5F, 0.5F }),
				}),
				scenario::Controls({
					scenario::Control("npc:namespaced"),
					scenario::Control("unqualified", { 3.5F, 0.5F }),
				})),
			scenario::ProfileCatalog({
				scenario::Profile("profile:namespaced"),
				scenario::Profile("plain-profile"),
			}),
			{ scenario::ProfileFrame("frame:ids", scenario::LevelMap("level:ids", { "...." })) });

	Expect(definition.initialState.npcActors.actors[0].npcId == scenario::Id("npc:namespaced"), "fixture should preserve namespaced npc id");
	Expect(definition.initialState.npcActors.actors[1].npcId == scenario::Id("unqualified"), "fixture should preserve unqualified npc id");
	Expect(definition.profileTraits.entries[0].profileId == scenario::Id("profile:namespaced"), "fixture should preserve namespaced profile id");
	Expect(definition.profileTraits.entries[1].profileId == scenario::Id("plain-profile"), "fixture should preserve unqualified profile id");
}

} // namespace

int main()
{
	TestMinimalProfileScenarioRunsThroughExistingRunner();
	TestFixtureProfileScenarioFeedsAuthoringAdapterByValue();
	TestFixtureHelpersPreserveNamespacedAndUnqualifiedIds();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
