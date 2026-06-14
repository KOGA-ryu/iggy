#include <cstdlib>
#include <vector>

#include "scene/npc/NpcActorState2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::NpcActorState2D Actor(
	const char *npcId,
	const char *aiProfileId = "ai-profile:guard",
	const char *factionId = "faction:town",
	iggy::Vec2 position = { 0.0F, 0.0F },
	const char *currentGoalId = "",
	bool present = true)
{
	return {
		Id(npcId),
		Id(aiProfileId),
		Id(factionId),
		position,
		Id(currentGoalId),
		present,
	};
}

bool SameActor(const iggy::NpcActorState2D &actual, const iggy::NpcActorState2D &expected)
{
	return actual.npcId == expected.npcId
		&& actual.aiProfileId == expected.aiProfileId
		&& actual.factionId == expected.factionId
		&& NearVec(actual.position, expected.position)
		&& actual.currentGoalId == expected.currentGoalId
		&& actual.present == expected.present;
}

bool SameActors(
	const std::vector<iggy::NpcActorState2D> &actual,
	const std::vector<iggy::NpcActorState2D> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (!SameActor(actual[index], expected[index]))
			return false;
	}
	return true;
}

void ExpectActor(
	const iggy::NpcActorState2D &actual,
	const iggy::NpcActorState2D &expected,
	const char *message)
{
	Expect(actual.npcId == expected.npcId, message);
	Expect(actual.aiProfileId == expected.aiProfileId, message);
	Expect(actual.factionId == expected.factionId, message);
	Expect(NearVec(actual.position, expected.position), message);
	Expect(actual.currentGoalId == expected.currentGoalId, message);
	Expect(actual.present == expected.present, message);
}

void TestEmptyInputBuildsValidEmptyRegistry()
{
	const iggy::NpcActorState2DRegistryBuildResult result =
		iggy::NpcActorState2DRegistryBuilder {}.build({});

	Expect(result.built, "empty NPC actor input should build");
	Expect(result.issues.empty(), "empty NPC actor input should have no issues");
	Expect(result.registry.actors.empty(), "empty NPC actor input should publish empty registry");
	Expect(!result.registry.contains(Id("npc:missing")), "empty NPC actor registry should not contain missing id");
	Expect(result.registry.find(Id("npc:missing")) == nullptr, "empty NPC actor registry should return null for missing id");
}

void TestSuccessfulBuildPreservesOrderAndFields()
{
	const std::vector<iggy::NpcActorState2D> actors {
		Actor("npc:guard", "ai-profile:guard", "faction:town", { 1.0F, 2.0F }, "goal:patrol", true),
		Actor("npc:merchant", "ai-profile:merchant", "faction:town", { -2.0F, 4.0F }, "", false),
		Actor("npc:raider", "ai-profile:raider", "faction:bandit", { 5.5F, -3.0F }, "goal:raid", true),
	};

	const iggy::NpcActorState2DRegistryBuildResult result =
		iggy::NpcActorState2DRegistryBuilder {}.build(actors);

	Expect(result.built, "valid NPC actors should build");
	Expect(result.issues.empty(), "valid NPC actors should have no issues");
	Expect(result.registry.actors.size() == actors.size(), "valid NPC actor registry should preserve actor count");
	if (result.registry.actors.size() == actors.size()) {
		ExpectActor(result.registry.actors[0], actors[0], "first NPC actor should preserve fields");
		ExpectActor(result.registry.actors[1], actors[1], "second NPC actor should preserve fields");
		ExpectActor(result.registry.actors[2], actors[2], "third NPC actor should preserve fields");
	}
}

void TestFindAndContainsUseExactNpcIds()
{
	const std::vector<iggy::NpcActorState2D> actors {
		Actor("npc:guard", "ai-profile:guard"),
		Actor("npc:scout", "ai-profile:scout"),
	};
	const iggy::NpcActorState2DRegistryBuildResult result =
		iggy::NpcActorState2DRegistryBuilder {}.build(actors);

	Expect(result.built, "NPC actor lookup setup should build");
	Expect(result.registry.contains(Id("npc:guard")), "NPC actor registry should contain exact id");
	Expect(!result.registry.contains(Id("npc:missing")), "NPC actor registry should not contain missing id");
	const iggy::NpcActorState2D *actor = result.registry.find(Id("npc:scout"));
	Expect(actor != nullptr, "NPC actor registry should find exact id");
	if (actor != nullptr)
		ExpectActor(*actor, actors[1], "NPC actor find should return matching payload");
	Expect(result.registry.find(Id("npc:missing")) == nullptr, "NPC actor registry find should return null for missing id");
}

void TestEmptyNpcIdFails()
{
	const iggy::NpcActorState2D actor = Actor("", "ai-profile:guard");

	const iggy::NpcActorState2DRegistryBuildResult result =
		iggy::NpcActorState2DRegistryBuilder {}.build({ actor });

	Expect(!result.built, "empty NPC actor id should fail build");
	Expect(result.registry.actors.empty(), "failed empty NPC actor id build should publish empty registry");
	Expect(result.issues.size() == 1, "empty NPC actor id should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcActorState2DIssueCode::EmptyNpcId, "empty NPC actor issue should use EmptyNpcId");
		Expect(result.issues[0].actorIndex == 0, "empty NPC actor issue should preserve actor index");
		ExpectActor(result.issues[0].actor, actor, "empty NPC actor issue should preserve actor payload");
	}
}

void TestDuplicateNpcIdFailsForLaterEntry()
{
	const std::vector<iggy::NpcActorState2D> actors {
		Actor("npc:guard", "ai-profile:guard"),
		Actor("npc:guard", "ai-profile:elite"),
	};

	const iggy::NpcActorState2DRegistryBuildResult result =
		iggy::NpcActorState2DRegistryBuilder {}.build(actors);

	Expect(!result.built, "duplicate NPC actor id should fail build");
	Expect(result.registry.actors.empty(), "failed duplicate NPC actor id build should publish empty registry");
	Expect(result.issues.size() == 1, "duplicate NPC actor id should report one issue for later duplicate");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcActorState2DIssueCode::DuplicateNpcId, "duplicate NPC actor issue should use DuplicateNpcId");
		Expect(result.issues[0].actorIndex == 1, "duplicate NPC actor issue should preserve later actor index");
		ExpectActor(result.issues[0].actor, actors[1], "duplicate NPC actor issue should preserve later actor payload");
	}
}

void TestEmptyAiProfileIdFails()
{
	const iggy::NpcActorState2D actor = Actor("npc:guard", "");

	const iggy::NpcActorState2DRegistryBuildResult result =
		iggy::NpcActorState2DRegistryBuilder {}.build({ actor });

	Expect(!result.built, "empty NPC actor AI profile id should fail build");
	Expect(result.registry.actors.empty(), "failed empty AI profile id build should publish empty registry");
	Expect(result.issues.size() == 1, "empty AI profile id should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcActorState2DIssueCode::EmptyAiProfileId, "empty AI profile issue should use EmptyAiProfileId");
		Expect(result.issues[0].actorIndex == 0, "empty AI profile issue should preserve actor index");
		ExpectActor(result.issues[0].actor, actor, "empty AI profile issue should preserve actor payload");
	}
}

void TestNotPresentActorsAndEmptyGoalIdsAreValidAndPreserved()
{
	const iggy::NpcActorState2D actor = Actor("npc:offscreen", "ai-profile:guard", "faction:town", { 8.0F, 9.0F }, "", false);

	const iggy::NpcActorState2DRegistryBuildResult result =
		iggy::NpcActorState2DRegistryBuilder {}.build({ actor });

	Expect(result.built, "not-present NPC actor with empty goal should build");
	Expect(result.issues.empty(), "not-present NPC actor with empty goal should have no issues");
	Expect(result.registry.actors.size() == 1, "not-present NPC actor should be preserved");
	if (result.registry.actors.size() == 1) {
		Expect(!result.registry.actors[0].present, "not-present NPC actor should preserve present flag");
		Expect(result.registry.actors[0].currentGoalId.empty(), "NPC actor should preserve empty goal id as data");
		ExpectActor(result.registry.actors[0], actor, "not-present NPC actor should preserve fields");
	}
}

void TestNamespacedAndUnqualifiedNpcIdsAreDistinct()
{
	const std::vector<iggy::NpcActorState2D> actors {
		Actor("guard", "ai-profile:guard"),
		Actor("npc:guard", "ai-profile:guard"),
	};

	const iggy::NpcActorState2DRegistryBuildResult result =
		iggy::NpcActorState2DRegistryBuilder {}.build(actors);

	Expect(result.built, "namespaced and unqualified NPC actor ids should build as distinct ids");
	Expect(result.issues.empty(), "namespaced and unqualified NPC actor ids should not produce duplicate issues");
	Expect(result.registry.contains(Id("guard")), "NPC actor registry should contain unqualified id");
	Expect(result.registry.contains(Id("npc:guard")), "NPC actor registry should contain namespaced id");
	const iggy::NpcActorState2D *unqualified = result.registry.find(Id("guard"));
	const iggy::NpcActorState2D *namespaced = result.registry.find(Id("npc:guard"));
	Expect(unqualified != nullptr && namespaced != nullptr, "NPC actor registry should find both exact ids");
	if (unqualified != nullptr && namespaced != nullptr) {
		Expect(unqualified->npcId != namespaced->npcId, "namespaced and unqualified NPC actor ids should remain distinct");
	}
}

void TestMultipleIssuesPreserveDeterministicInputOrder()
{
	const std::vector<iggy::NpcActorState2D> actors {
		Actor("npc:guard", "ai-profile:guard"),
		Actor("", ""),
		Actor("npc:guard", ""),
	};

	const iggy::NpcActorState2DRegistryBuildResult result =
		iggy::NpcActorState2DRegistryBuilder {}.build(actors);

	Expect(!result.built, "multiple NPC actor issues should fail build");
	Expect(result.registry.actors.empty(), "failed multi-issue NPC actor build should publish empty registry");
	Expect(result.issues.size() == 4, "multiple NPC actor issues should all be reported");
	if (result.issues.size() == 4) {
		Expect(result.issues[0].code == iggy::NpcActorState2DIssueCode::EmptyNpcId && result.issues[0].actorIndex == 1, "empty NPC id should be first issue for second actor");
		Expect(result.issues[1].code == iggy::NpcActorState2DIssueCode::EmptyAiProfileId && result.issues[1].actorIndex == 1, "empty AI profile should follow empty NPC id for second actor");
		Expect(result.issues[2].code == iggy::NpcActorState2DIssueCode::DuplicateNpcId && result.issues[2].actorIndex == 2, "duplicate NPC id should be first issue for third actor");
		Expect(result.issues[3].code == iggy::NpcActorState2DIssueCode::EmptyAiProfileId && result.issues[3].actorIndex == 2, "empty AI profile should follow duplicate NPC id for third actor");
	}
}

void TestBuildDoesNotMutateInputs()
{
	std::vector<iggy::NpcActorState2D> actors {
		Actor("npc:guard", "ai-profile:guard", "faction:town", { 1.0F, 2.0F }, "goal:patrol", true),
		Actor("npc:offscreen", "ai-profile:idle", "faction:town", { -3.0F, 4.0F }, "", false),
	};
	const std::vector<iggy::NpcActorState2D> actorsBefore = actors;

	const iggy::NpcActorState2DRegistryBuildResult result =
		iggy::NpcActorState2DRegistryBuilder {}.build(actors);

	Expect(result.built, "NPC actor immutability setup should build");
	Expect(SameActors(actors, actorsBefore), "NPC actor registry builder should not mutate inputs");
}

} // namespace

int main()
{
	TestEmptyInputBuildsValidEmptyRegistry();
	TestSuccessfulBuildPreservesOrderAndFields();
	TestFindAndContainsUseExactNpcIds();
	TestEmptyNpcIdFails();
	TestDuplicateNpcIdFailsForLaterEntry();
	TestEmptyAiProfileIdFails();
	TestNotPresentActorsAndEmptyGoalIdsAreValidAndPreserved();
	TestNamespacedAndUnqualifiedNpcIdsAreDistinct();
	TestMultipleIssuesPreserveDeterministicInputOrder();
	TestBuildDoesNotMutateInputs();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
