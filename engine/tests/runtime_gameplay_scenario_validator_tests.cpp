#include <cstdlib>
#include <string_view>
#include <type_traits>
#include <vector>

#include "runtime/RuntimeGameplayScenarioDefinition.hpp"
#include "runtime/RuntimeGameplayScenarioRunner.hpp"
#include "runtime/RuntimeGameplayScenarioValidator.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::MapFromRows;
using iggy::test::NearVec;

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
	map.id = Id("level:scenario-definition");
	return map;
}

iggy::LevelTileMap InvalidMap()
{
	iggy::LevelTileMap map;
	map.id = Id("level:invalid");
	map.width = 2;
	map.height = 2;
	return map;
}

iggy::NpcActorState2D Actor(
	const char *npcId,
	iggy::Vec2 position = { 0.5F, 0.5F },
	bool present = true)
{
	return {
		Id(npcId),
		Id("profile:scenario-definition"),
		Id("faction:scenario-definition"),
		position,
		Id("goal:scenario-definition"),
		present,
	};
}

iggy::NpcActorControlState2D Control(const char *npcId, iggy::Vec2 target = { 2.5F, 0.5F })
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
	Expect(result.built, "scenario validator actors should build");
	return result.registry;
}

iggy::NpcActorControlState2DRegistry Controls(std::vector<iggy::NpcActorControlState2D> controls)
{
	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);
	Expect(result.built, "scenario validator controls should build");
	return result.registry;
}

iggy::NpcMapPlayControlFramePlanSubject Subject(const char *npcId)
{
	iggy::NpcMapPlayControlFramePlanSubject subject;
	subject.npcId = Id(npcId);
	return subject;
}

iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerFrame Frame(
	iggy::LevelTileMap map,
	std::vector<iggy::NpcMapPlayControlFramePlanSubject> subjects = {})
{
	iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerFrame frame;
	frame.movementMap = map;
	frame.subjects = subjects;
	return frame;
}

iggy::runtime::RuntimeGameplayScenarioFrameDefinition FrameDefinition(
	iggy::LevelTileMap map,
	std::vector<iggy::NpcMapPlayControlFramePlanSubject> subjects = {})
{
	iggy::runtime::RuntimeGameplayScenarioFrameDefinition frame;
	frame.hasFrameId = true;
	frame.frameId = Id("frame:valid");
	frame.frame = Frame(map, subjects);
	return frame;
}

iggy::runtime::RuntimeGameplayScenarioDefinition Definition(
	iggy::NpcActorState2DRegistry actors = {},
	iggy::NpcActorControlState2DRegistry controls = {},
	std::vector<iggy::runtime::RuntimeGameplayScenarioFrameDefinition> frames = {})
{
	iggy::runtime::RuntimeGameplayScenarioDefinition definition;
	definition.hasScenarioId = true;
	definition.scenarioId = Id("scenario:valid");
	definition.initialState.npcActors = actors;
	definition.initialState.npcControls = controls;
	definition.frames = frames;
	return definition;
}

iggy::runtime::RuntimeGameplayScenarioValidationResult Validate(
	const iggy::runtime::RuntimeGameplayScenarioDefinition &definition)
{
	return iggy::runtime::RuntimeGameplayScenarioValidator {}.validate(definition);
}

bool HasIssue(
	const iggy::runtime::RuntimeGameplayScenarioValidationResult &result,
	iggy::runtime::RuntimeGameplayScenarioIssueCode code,
	std::size_t frameIndex)
{
	for (const iggy::runtime::RuntimeGameplayScenarioIssue &issue : result.issues) {
		if (issue.code == code && issue.frameIndex == frameIndex)
			return true;
	}
	return false;
}

void TestEmptyDefaultDefinitionIsValidAndRuns()
{
	const iggy::runtime::RuntimeGameplayScenarioDefinition definition;

	const iggy::runtime::RuntimeGameplayScenarioValidationResult validation = Validate(definition);
	const iggy::runtime::RuntimeGameplayScenarioResult run =
		iggy::runtime::RuntimeGameplayScenarioRunner {}.run(validation.scenario);

	Expect(validation.ok(), "empty/default scenario definition should validate");
	Expect(validation.frameCount == 0, "empty/default scenario definition should report zero frames");
	Expect(run.runner.frameResults.empty(), "empty/default scenario should run as no-op");
}

void TestValidActorControlFrameValidates()
{
	const iggy::runtime::RuntimeGameplayScenarioDefinition definition = Definition(
		Actors({ Actor("npc:valid") }),
		Controls({ Control("npc:valid") }),
		{ FrameDefinition(LevelMap({ "..." }), { Subject("npc:valid") }) });

	const iggy::runtime::RuntimeGameplayScenarioValidationResult result = Validate(definition);

	Expect(result.ok(), "valid actor/control/frame definition should validate");
	Expect(result.frameState.entries.size() == 1, "valid definition should preserve frame-state projection");
}

void TestMissingControlIssue()
{
	const iggy::runtime::RuntimeGameplayScenarioDefinition definition = Definition(
		Actors({ Actor("npc:missing-control") }),
		Controls({}),
		{ FrameDefinition(LevelMap({ "..." }), { Subject("npc:missing-control") }) });

	const iggy::runtime::RuntimeGameplayScenarioValidationResult result = Validate(definition);

	Expect(!result.ok(), "missing control definition should be invalid");
	Expect(result.missingControlCount == 1, "missing control should increment count");
	Expect(result.issues[0].code == iggy::runtime::RuntimeGameplayScenarioIssueCode::MissingControl, "missing control should be first issue");
	Expect(result.issues[0].npcId == Id("npc:missing-control"), "missing control should preserve npc id");
}

void TestOrphanControlIssue()
{
	const iggy::runtime::RuntimeGameplayScenarioDefinition definition = Definition(
		Actors({}),
		Controls({ Control("npc:orphan-control") }),
		{ FrameDefinition(LevelMap({ "..." })) });

	const iggy::runtime::RuntimeGameplayScenarioValidationResult result = Validate(definition);

	Expect(!result.ok(), "orphan control definition should be invalid");
	Expect(result.orphanControlCount == 1, "orphan control should increment count");
	Expect(result.issues[0].code == iggy::runtime::RuntimeGameplayScenarioIssueCode::OrphanControl, "orphan control should be first issue");
	Expect(result.issues[0].npcId == Id("npc:orphan-control"), "orphan control should preserve npc id");
}

void TestDuplicateTraitSubjectIssue()
{
	const iggy::runtime::RuntimeGameplayScenarioDefinition definition = Definition(
		Actors({ Actor("npc:duplicate") }),
		Controls({ Control("npc:duplicate") }),
		{ FrameDefinition(LevelMap({ "..." }), { Subject("npc:duplicate"), Subject("npc:duplicate") }) });

	const iggy::runtime::RuntimeGameplayScenarioValidationResult result = Validate(definition);

	Expect(!result.ok(), "duplicate trait subject should be invalid");
	Expect(result.duplicateTraitSubjectCount == 1, "duplicate trait subject should increment count");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayScenarioIssueCode::DuplicateTraitSubject, 0), "duplicate trait subject issue should preserve frame index");
}

void TestMissingTraitSubjectForPresentActorIssue()
{
	const iggy::runtime::RuntimeGameplayScenarioDefinition definition = Definition(
		Actors({ Actor("npc:missing-trait") }),
		Controls({ Control("npc:missing-trait") }),
		{ FrameDefinition(LevelMap({ "..." })) });

	const iggy::runtime::RuntimeGameplayScenarioValidationResult result = Validate(definition);

	Expect(!result.ok(), "missing trait subject should be invalid");
	Expect(result.missingTraitSubjectCount == 1, "missing trait subject should increment count");
	Expect(result.issues.back().code == iggy::runtime::RuntimeGameplayScenarioIssueCode::MissingTraitSubject, "missing trait subject should be reported");
	Expect(result.issues.back().npcId == Id("npc:missing-trait"), "missing trait subject should preserve npc id");
}

void TestAbsentActorDoesNotRequireTraitSubject()
{
	const iggy::runtime::RuntimeGameplayScenarioDefinition definition = Definition(
		Actors({ Actor("npc:absent", { 0.5F, 0.5F }, false) }),
		Controls({ Control("npc:absent") }),
		{ FrameDefinition(LevelMap({ "..." })) });

	const iggy::runtime::RuntimeGameplayScenarioValidationResult result = Validate(definition);

	Expect(result.ok(), "absent actor should not require trait subject by default");
	Expect(result.missingTraitSubjectCount == 0, "absent actor should not increment missing trait subject count");
}

void TestInvalidMovementMapIssue()
{
	const iggy::runtime::RuntimeGameplayScenarioDefinition definition = Definition(
		Actors({ Actor("npc:map") }),
		Controls({ Control("npc:map") }),
		{ FrameDefinition(InvalidMap(), { Subject("npc:map") }) });

	const iggy::runtime::RuntimeGameplayScenarioValidationResult result = Validate(definition);

	Expect(!result.ok(), "invalid movement map should invalidate definition");
	Expect(result.invalidMovementMapCount == 1, "invalid movement map should increment count");
	Expect(result.issues[0].code == iggy::runtime::RuntimeGameplayScenarioIssueCode::InvalidMovementMap, "invalid movement map should be frame issue");
	Expect(result.issues[0].movementMap.id == Id("level:invalid"), "invalid movement map issue should preserve map");
}

void TestIssueOrderingAcrossFrames()
{
	const iggy::runtime::RuntimeGameplayScenarioDefinition definition = Definition(
		Actors({ Actor("npc:ordered") }),
		Controls({ Control("npc:ordered") }),
		{
			FrameDefinition(InvalidMap(), { Subject("npc:ordered") }),
			FrameDefinition(LevelMap({ "..." }), { Subject("npc:ordered"), Subject("npc:ordered") }),
		});

	const iggy::runtime::RuntimeGameplayScenarioValidationResult result = Validate(definition);

	Expect(result.issues.size() == 2, "ordered issue setup should produce two issues");
	if (result.issues.size() == 2) {
		Expect(result.issues[0].code == iggy::runtime::RuntimeGameplayScenarioIssueCode::InvalidMovementMap && result.issues[0].frameIndex == 0, "first frame issue should come first");
		Expect(result.issues[1].code == iggy::runtime::RuntimeGameplayScenarioIssueCode::DuplicateTraitSubject && result.issues[1].frameIndex == 1, "second frame issue should come second");
	}
}

void TestNamespacedAndUnqualifiedIdsRemainDistinct()
{
	const iggy::runtime::RuntimeGameplayScenarioDefinition definition = Definition(
		Actors({ Actor("npc:guard") }),
		Controls({ Control("npc:guard") }),
		{ FrameDefinition(LevelMap({ "..." }), { Subject("guard") }) });

	const iggy::runtime::RuntimeGameplayScenarioValidationResult result = Validate(definition);

	Expect(!result.ok(), "namespaced actor and unqualified subject should remain distinct");
	Expect(result.orphanTraitSubjectCount == 1, "unqualified subject should be orphan");
	Expect(result.missingTraitSubjectCount == 1, "namespaced actor should still miss trait subject");
}

void TestValidationDoesNotMutateDefinition()
{
	iggy::runtime::RuntimeGameplayScenarioDefinition definition = Definition(
		Actors({ Actor("npc:copy") }),
		Controls({ Control("npc:copy") }),
		{ FrameDefinition(LevelMap({ "..." }), { Subject("npc:copy") }) });
	const iggy::runtime::RuntimeGameplayScenarioDefinition before = definition;

	const iggy::runtime::RuntimeGameplayScenarioValidationResult result = Validate(definition);

	Expect(result.ok(), "immutability setup should validate");
	Expect(definition.scenarioId == before.scenarioId, "validator should not mutate scenario id");
	Expect(definition.frames.size() == before.frames.size(), "validator should not mutate frame count");
	Expect(definition.frames[0].frame.subjects[0].npcId == before.frames[0].frame.subjects[0].npcId, "validator should not mutate subject id");
}

void TestValidDefinitionRunsThroughScenarioRunner()
{
	const iggy::runtime::RuntimeGameplayScenarioDefinition definition = Definition(
		Actors({ Actor("npc:acceptance", { 0.5F, 0.5F }) }),
		Controls({ Control("npc:acceptance", { 2.5F, 0.5F }) }),
		{ FrameDefinition(LevelMap({ "..." }), { Subject("npc:acceptance") }) });

	const iggy::runtime::RuntimeGameplayScenarioValidationResult validation = Validate(definition);
	const iggy::runtime::RuntimeGameplayScenarioResult run =
		iggy::runtime::RuntimeGameplayScenarioRunner {}.run(validation.scenario);

	Expect(validation.ok(), "acceptance definition should validate");
	Expect(run.npcMovedCount == 1, "acceptance definition should run through scenario movement");
	Expect(run.state.npcActors.actors.size() == 1 && NearVec(run.state.npcActors.actors[0].position, { 1.5F, 0.5F }), "acceptance scenario should update actor position");
}

} // namespace

int main()
{
	TestEmptyDefaultDefinitionIsValidAndRuns();
	TestValidActorControlFrameValidates();
	TestMissingControlIssue();
	TestOrphanControlIssue();
	TestDuplicateTraitSubjectIssue();
	TestMissingTraitSubjectForPresentActorIssue();
	TestAbsentActorDoesNotRequireTraitSubject();
	TestInvalidMovementMapIssue();
	TestIssueOrderingAcrossFrames();
	TestNamespacedAndUnqualifiedIdsRemainDistinct();
	TestValidationDoesNotMutateDefinition();
	TestValidDefinitionRunsThroughScenarioRunner();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
