#include <cstdlib>
#include <iostream>
#include <string_view>

#include "modules/npc_ai/NpcSpawnBuilder.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
}

bool SameTile(iggy::TileCoord tile, int x, int y)
{
	return tile.x == x && tile.y == y;
}

iggy::LevelBlueprint BlueprintWithBounds()
{
	iggy::LevelBlueprint blueprint;
	blueprint.bounds = { 5, 4 };
	blueprint.playerStarts.push_back({ 0, 0 });
	return blueprint;
}

bool HasIssue(const iggy::npc_ai::NpcSpawnBuildResult &result, iggy::npc_ai::NpcSpawnBuildIssueCode code)
{
	for (const iggy::npc_ai::NpcSpawnBuildIssue &issue : result.issues) {
		if (issue.code == code)
			return true;
	}
	return false;
}

void TestEmptyBlueprintProducesEmptyBatch()
{
	const iggy::LevelBlueprint blueprint = BlueprintWithBounds();
	const iggy::npc_ai::NpcSpawnBuildResult result = iggy::npc_ai::NpcSpawnBuilder {}.build(blueprint);

	Expect(result.agents.empty(), "empty blueprint should produce no NPC agents");
	Expect(result.issues.empty(), "empty blueprint should produce no spawn issues");
}

void TestOneNpcSpawnProducesAgent()
{
	iggy::LevelBlueprint blueprint = BlueprintWithBounds();
	blueprint.entitySpawns.push_back({ iggy::ResourceId { "enemy:skeleton" }, 2, 1, iggy::ResourceId { "spawn:skeleton_01" } });

	const iggy::npc_ai::NpcSpawnBuildResult result = iggy::npc_ai::NpcSpawnBuilder {}.build(blueprint);

	Expect(result.agents.size() == 1, "one enemy spawn should produce one NPC agent");
	Expect(result.agents[0].id == iggy::ResourceId { "spawn:skeleton_01" }, "NPC spawn should preserve explicit spawn id");
	Expect(result.agents[0].state.position == iggy::Vec2 { 2.5F, 1.5F }, "NPC spawn should initialize position at tile center");
	Expect(SameTile(result.agents[0].state.homeTile, 2, 1), "NPC spawn should initialize home tile");
	Expect(!result.agents[0].state.awareness.alerted, "NPC spawn should initialize empty awareness");
	Expect(!result.agents[0].state.followState.completed && result.agents[0].state.followState.waypointIndex == 0, "NPC spawn should initialize empty follow state");
}

void TestMultipleNpcSpawnsPreserveOrder()
{
	iggy::LevelBlueprint blueprint = BlueprintWithBounds();
	blueprint.entitySpawns.push_back({ iggy::ResourceId { "enemy:skeleton" }, 1, 1, iggy::ResourceId { "spawn:first" } });
	blueprint.entitySpawns.push_back({ iggy::ResourceId { "npc:villager" }, 3, 2, iggy::ResourceId { "spawn:second" } });

	const iggy::npc_ai::NpcSpawnBuildResult result = iggy::npc_ai::NpcSpawnBuilder {}.build(blueprint);

	Expect(result.agents.size() == 2, "two NPC spawns should produce two agents");
	Expect(result.agents[0].id == iggy::ResourceId { "spawn:first" }, "first NPC spawn should preserve order");
	Expect(result.agents[1].id == iggy::ResourceId { "spawn:second" }, "second NPC spawn should preserve order");
	Expect(result.agents[1].state.position == iggy::Vec2 { 3.5F, 2.5F }, "second NPC spawn should preserve its tile center");
}

void TestNonNpcSpawnSkipped()
{
	iggy::LevelBlueprint blueprint = BlueprintWithBounds();
	blueprint.entitySpawns.push_back({ iggy::ResourceId { "item:potion" }, 1, 1, iggy::ResourceId { "spawn:potion_01" } });
	blueprint.entitySpawns.push_back({ iggy::ResourceId { "enemy:skeleton" }, 2, 1, iggy::ResourceId { "spawn:skeleton_01" } });

	const iggy::npc_ai::NpcSpawnBuildResult result = iggy::npc_ai::NpcSpawnBuilder {}.build(blueprint);

	Expect(result.agents.size() == 1, "non-NPC item spawn should be skipped");
	Expect(result.agents[0].id == iggy::ResourceId { "spawn:skeleton_01" }, "NPC agent after skipped item should still be built");
	Expect(HasIssue(result, iggy::npc_ai::NpcSpawnBuildIssueCode::NonNpcSpawnSkipped), "skipped item should be reported");
}

void TestOutOfBoundsNpcSpawnSkipped()
{
	iggy::LevelBlueprint blueprint = BlueprintWithBounds();
	blueprint.entitySpawns.push_back({ iggy::ResourceId { "enemy:skeleton" }, 5, 1, iggy::ResourceId { "spawn:oob" } });
	blueprint.entitySpawns.push_back({ iggy::ResourceId { "enemy:bat" }, 2, 2, iggy::ResourceId { "spawn:bat_01" } });

	const iggy::npc_ai::NpcSpawnBuildResult result = iggy::npc_ai::NpcSpawnBuilder {}.build(blueprint);

	Expect(result.agents.size() == 1, "out-of-bounds NPC spawn should be skipped");
	Expect(result.agents[0].id == iggy::ResourceId { "spawn:bat_01" }, "valid NPC spawn after out-of-bounds spawn should still be built");
	Expect(HasIssue(result, iggy::npc_ai::NpcSpawnBuildIssueCode::OutOfBoundsSpawnSkipped), "out-of-bounds NPC spawn should be reported");
}

void TestSpawnWithoutIdFallsBackToType()
{
	iggy::LevelBlueprint blueprint = BlueprintWithBounds();
	blueprint.entitySpawns.push_back({ iggy::ResourceId { "enemy:skeleton" }, 2, 1 });

	const iggy::npc_ai::NpcSpawnBuildResult result = iggy::npc_ai::NpcSpawnBuilder {}.build(blueprint);

	Expect(result.agents.size() == 1, "NPC spawn without id should still produce an agent");
	Expect(result.agents[0].id == iggy::ResourceId { "enemy:skeleton" }, "NPC spawn without id should fall back to type as agent id");
}

} // namespace

int main()
{
	TestEmptyBlueprintProducesEmptyBatch();
	TestOneNpcSpawnProducesAgent();
	TestMultipleNpcSpawnsPreserveOrder();
	TestNonNpcSpawnSkipped();
	TestOutOfBoundsNpcSpawnSkipped();
	TestSpawnWithoutIdFallsBackToType();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
