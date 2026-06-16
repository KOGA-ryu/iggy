#include <cstdlib>

#include "core/resource/ResourceId.hpp"
#include "scene/camera/CameraView.hpp"
#include "scene/level/LevelNpcAgentRenderCommands.hpp"
#include "scene/level/LevelTileDrawList.hpp"
#include "scene/level/LevelTileRenderCommands.hpp"
#include "scene/level/LevelVisibleTiles.hpp"
#include "servers/render/RenderCommand2D.hpp"
#include "support/GeometryAssertions.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::SameBounds;

const iggy::ResourceId NpcMaterial { "material:npc" };
const iggy::ResourceId WalkableMaterial { "material:floor" };
const iggy::ResourceId BlockedMaterial { "material:wall" };

iggy::npc_ai::NpcAgentEntry Agent(const char *id, iggy::Vec2 position)
{
	iggy::npc_ai::NpcAgentEntry agent;
	agent.id = iggy::ResourceId { id };
	agent.state.position = position;
	return agent;
}

iggy::LevelNpcAgentRenderCommandConfig Config(int layer = 9)
{
	return { NpcMaterial, { 1.0F, 1.0F }, { 0.5F, 0.5F }, layer };
}

void ExpectCommand(const iggy::render::RenderCommand2D &command, iggy::Aabb2 bounds, const iggy::ResourceId &materialId, int layer, std::size_t order, const char *message)
{
	Expect(command.type == iggy::render::RenderCommand2DType::Quad && SameBounds(command.worldBounds, bounds) && command.materialId == materialId && command.layer == layer && command.order == order, message);
}

void TestEmptyAgentListReturnsEmptyCommands()
{
	const iggy::render::RenderCommandList2D commands = iggy::LevelNpcAgentRenderCommands {}.build({}, Config());

	Expect(commands.commands.empty(), "empty agent list should return empty render command list");
}

void TestOneAgentEmitsDefaultCenteredQuad()
{
	const std::vector<iggy::npc_ai::NpcAgentEntry> agents {
		Agent("npc:one", { 2.0F, 3.0F }),
	};

	const iggy::render::RenderCommandList2D commands = iggy::LevelNpcAgentRenderCommands {}.build(agents, Config(4));

	Expect(commands.commands.size() == 1, "one agent should emit one render command");
	if (commands.commands.size() == 1)
		ExpectCommand(commands.commands[0], { { 1.5F, 2.5F }, { 2.5F, 3.5F } }, NpcMaterial, 4, 0, "default size and anchor should center bounds on agent position");
}

void TestCustomSizeAndAnchorProduceExpectedBounds()
{
	const std::vector<iggy::npc_ai::NpcAgentEntry> agents {
		Agent("npc:one", { 10.0F, 20.0F }),
	};
	const iggy::LevelNpcAgentRenderCommandConfig config { NpcMaterial, { 2.0F, 4.0F }, { 0.0F, 1.0F }, 6 };

	const iggy::render::RenderCommandList2D commands = iggy::LevelNpcAgentRenderCommands {}.build(agents, config);

	Expect(commands.commands.size() == 1, "one custom-sized agent should emit one command");
	if (commands.commands.size() == 1)
		ExpectCommand(commands.commands[0], { { 10.0F, 16.0F }, { 12.0F, 20.0F } }, NpcMaterial, 6, 0, "custom size and anchor should determine command bounds");
}

void TestMultipleAgentsPreserveOrderAndCommandOrder()
{
	const std::vector<iggy::npc_ai::NpcAgentEntry> agents {
		Agent("npc:first", { 0.0F, 0.0F }),
		Agent("npc:second", { 4.0F, 0.0F }),
		Agent("npc:third", { 8.0F, 0.0F }),
	};

	const iggy::render::RenderCommandList2D commands = iggy::LevelNpcAgentRenderCommands {}.build(agents, Config(5));

	Expect(commands.commands.size() == 3, "three agents should emit three commands");
	if (commands.commands.size() == 3) {
		ExpectCommand(commands.commands[0], { { -0.5F, -0.5F }, { 0.5F, 0.5F } }, NpcMaterial, 5, 0, "first agent should become first command");
		ExpectCommand(commands.commands[1], { { 3.5F, -0.5F }, { 4.5F, 0.5F } }, NpcMaterial, 5, 1, "second agent should become second command");
		ExpectCommand(commands.commands[2], { { 7.5F, -0.5F }, { 8.5F, 0.5F } }, NpcMaterial, 5, 2, "third agent should become third command");
	}
}

void TestNegativeSizeIsNormalized()
{
	const std::vector<iggy::npc_ai::NpcAgentEntry> agents {
		Agent("npc:one", { 0.0F, 0.0F }),
	};
	const iggy::LevelNpcAgentRenderCommandConfig config { NpcMaterial, { -2.0F, -4.0F }, { 0.5F, 0.5F }, 2 };

	const iggy::render::RenderCommandList2D commands = iggy::LevelNpcAgentRenderCommands {}.build(agents, config);

	Expect(commands.commands.size() == 1, "negative-size agent should still emit one command");
	if (commands.commands.size() == 1)
		ExpectCommand(commands.commands[0], { { -1.0F, -2.0F }, { 1.0F, 2.0F } }, NpcMaterial, 2, 0, "negative size should be normalized by absolute value");
}

void TestZeroSizeProducesDegenerateBounds()
{
	const std::vector<iggy::npc_ai::NpcAgentEntry> agents {
		Agent("npc:one", { 1.0F, 2.0F }),
	};
	const iggy::LevelNpcAgentRenderCommandConfig config { NpcMaterial, { 0.0F, 0.0F }, { 0.5F, 0.5F }, 2 };

	const iggy::render::RenderCommandList2D commands = iggy::LevelNpcAgentRenderCommands {}.build(agents, config);

	Expect(commands.commands.size() == 1, "zero-size agent should still emit one command");
	if (commands.commands.size() == 1)
		ExpectCommand(commands.commands[0], { { 1.0F, 2.0F }, { 1.0F, 2.0F } }, NpcMaterial, 2, 0, "zero size should produce degenerate bounds at the agent position");
}

void TestEmptyMaterialIdIsPreserved()
{
	const std::vector<iggy::npc_ai::NpcAgentEntry> agents {
		Agent("npc:one", { 1.0F, 2.0F }),
	};
	const iggy::LevelNpcAgentRenderCommandConfig config { {}, { 1.0F, 1.0F }, { 0.5F, 0.5F }, 2 };

	const iggy::render::RenderCommandList2D commands = iggy::LevelNpcAgentRenderCommands {}.build(agents, config);

	Expect(commands.commands.size() == 1, "empty material id should still emit a command");
	if (commands.commands.size() == 1)
		Expect(commands.commands[0].materialId.empty(), "empty NPC material id should be preserved without validation");
}

void TestTileAndNpcCommandsShareRenderCommandContract()
{
	const iggy::LevelTileMap map = iggy::test::MapFromRows({
		"..",
		".#",
	});
	const iggy::CameraViewResult view = iggy::CameraView {}.visibleWorldBounds({ { 1.0F, 1.0F } }, { { 2.0F, 2.0F }, 1.0F });
	const iggy::LevelVisibleTilesResult visible = iggy::LevelVisibleTiles {}.query(map, view.bounds);
	const iggy::LevelTileDrawListResult drawList = iggy::LevelTileDrawList {}.build(map, visible.tiles);
	const iggy::render::RenderCommandList2D tileCommands = iggy::LevelTileRenderCommands {}.build(drawList, { { WalkableMaterial, BlockedMaterial }, 1 });

	const std::vector<iggy::npc_ai::NpcAgentEntry> agents {
		Agent("npc:one", { 0.5F, 0.5F }),
	};
	const iggy::render::RenderCommandList2D npcCommands = iggy::LevelNpcAgentRenderCommands {}.build(agents, Config(7));

	iggy::render::RenderCommandList2D merged;
	merged.commands.insert(merged.commands.end(), tileCommands.commands.begin(), tileCommands.commands.end());
	merged.commands.insert(merged.commands.end(), npcCommands.commands.begin(), npcCommands.commands.end());

	Expect(tileCommands.commands.size() == 4, "composition should produce tile render commands");
	Expect(npcCommands.commands.size() == 1, "composition should produce NPC render commands");
	Expect(merged.commands.size() == tileCommands.commands.size() + npcCommands.commands.size(), "tile and NPC commands should merge through the same command list contract");
	if (merged.commands.size() == 5) {
		Expect(merged.commands[0].materialId == WalkableMaterial, "merged list should keep first tile material");
		Expect(merged.commands[3].materialId == BlockedMaterial, "merged list should keep blocked tile material");
		ExpectCommand(merged.commands[4], { { 0.0F, 0.0F }, { 1.0F, 1.0F } }, NpcMaterial, 7, 0, "merged list should keep NPC command data");
	}
}

} // namespace

int main()
{
	TestEmptyAgentListReturnsEmptyCommands();
	TestOneAgentEmitsDefaultCenteredQuad();
	TestCustomSizeAndAnchorProduceExpectedBounds();
	TestMultipleAgentsPreserveOrderAndCommandOrder();
	TestNegativeSizeIsNormalized();
	TestZeroSizeProducesDegenerateBounds();
	TestEmptyMaterialIdIsPreserved();
	TestTileAndNpcCommandsShareRenderCommandContract();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
