#include <cstdlib>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "modules/npc_ai/NpcAgentRenderCommands.hpp"
#include "scene/camera/CameraView.hpp"
#include "scene/level/LevelTileDrawList.hpp"
#include "scene/level/LevelTileRenderCommands.hpp"
#include "scene/level/LevelVisibleTiles.hpp"
#include "servers/render/RenderCommand2D.hpp"
#include "servers/render/RenderCommandList2DComposer.hpp"
#include "support/GeometryAssertions.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;
using iggy::test::SameBounds;

const iggy::ResourceId FirstMaterial { "material:first" };
const iggy::ResourceId SecondMaterial { "material:second" };
const iggy::ResourceId FloorMaterial { "material:floor" };
const iggy::ResourceId WallMaterial { "material:wall" };
const iggy::ResourceId NpcMaterial { "material:npc" };

bool SameRect(iggy::Rect2 actual, iggy::Rect2 expected)
{
	return NearVec(actual.position, expected.position) && NearVec(actual.size, expected.size);
}

iggy::render::RenderCommand2D Command(iggy::Aabb2 bounds, const iggy::ResourceId &materialId, int layer, std::size_t order)
{
	return { iggy::render::RenderCommand2DType::Quad, bounds, materialId, layer, order };
}

iggy::render::RenderCommand2D TexturedCommand(iggy::Aabb2 bounds, const iggy::ResourceId &materialId, const iggy::ResourceId &textureId, iggy::Rect2 sourceRect, int layer, std::size_t order)
{
	return { iggy::render::RenderCommand2DType::Quad, bounds, materialId, layer, order, { textureId, sourceRect, true } };
}

iggy::render::RenderCommandList2D List(std::vector<iggy::render::RenderCommand2D> commands)
{
	return { commands };
}

void ExpectCommand(const iggy::render::RenderCommand2D &command, iggy::Aabb2 bounds, const iggy::ResourceId &materialId, int layer, std::size_t order, const char *message)
{
	Expect(command.type == iggy::render::RenderCommand2DType::Quad && SameBounds(command.worldBounds, bounds) && command.materialId == materialId && command.layer == layer && command.order == order, message);
}

void ExpectNoTexturePayload(const iggy::render::RenderCommand2D &command, const char *message)
{
	Expect(command.texture.textureId.empty() && !command.texture.hasSourceRect, message);
}

void ExpectTexturePayload(const iggy::render::RenderCommand2D &command, const iggy::ResourceId &textureId, iggy::Rect2 sourceRect, const char *message)
{
	Expect(command.texture.textureId == textureId && SameRect(command.texture.sourceRect, sourceRect) && command.texture.hasSourceRect, message);
}

iggy::npc_ai::NpcAgentEntry Agent(const char *id, iggy::Vec2 position)
{
	iggy::npc_ai::NpcAgentEntry agent;
	agent.id = iggy::ResourceId { id };
	agent.state.position = position;
	return agent;
}

void TestEmptyTargetAndEmptySourceRemainEmpty()
{
	iggy::render::RenderCommandList2D target;
	const iggy::render::RenderCommandList2D source;

	iggy::render::RenderCommandList2DComposer {}.append(target, source);

	Expect(target.commands.empty(), "empty target plus empty source should remain empty");
}

void TestEmptySourceDoesNotChangeTarget()
{
	iggy::render::RenderCommandList2D target = List({
		Command({ { 0.0F, 0.0F }, { 1.0F, 1.0F } }, FirstMaterial, 3, 7),
	});
	const iggy::render::RenderCommandList2D source;

	iggy::render::RenderCommandList2DComposer {}.append(target, source);

	Expect(target.commands.size() == 1, "empty source should not add commands");
	if (target.commands.size() == 1)
		ExpectCommand(target.commands[0], { { 0.0F, 0.0F }, { 1.0F, 1.0F } }, FirstMaterial, 3, 7, "empty source should preserve existing target command exactly");
}

void TestAppendOneCommandToEmptyTargetNormalizesOrder()
{
	iggy::render::RenderCommandList2D target;
	const iggy::render::RenderCommandList2D source = List({
		Command({ { 2.0F, 3.0F }, { 4.0F, 5.0F } }, FirstMaterial, 2, 99),
	});

	iggy::render::RenderCommandList2DComposer {}.append(target, source);

	Expect(target.commands.size() == 1, "one source command should append one target command");
	if (target.commands.size() == 1)
		ExpectCommand(target.commands[0], { { 2.0F, 3.0F }, { 4.0F, 5.0F } }, FirstMaterial, 2, 0, "append should preserve fields and normalize order to zero");
}

void TestAppendMultipleCommandsToNonEmptyTarget()
{
	iggy::render::RenderCommandList2D target = List({
		Command({ { 0.0F, 0.0F }, { 1.0F, 1.0F } }, FirstMaterial, 0, 0),
	});
	const iggy::render::RenderCommandList2D source = List({
		Command({ { 1.0F, 0.0F }, { 2.0F, 1.0F } }, SecondMaterial, 1, 42),
		Command({ { 2.0F, 0.0F }, { 3.0F, 1.0F } }, FirstMaterial, 2, 12),
	});

	iggy::render::RenderCommandList2DComposer {}.append(target, source);

	Expect(target.commands.size() == 3, "two source commands should append after existing target command");
	if (target.commands.size() == 3) {
		ExpectCommand(target.commands[0], { { 0.0F, 0.0F }, { 1.0F, 1.0F } }, FirstMaterial, 0, 0, "existing target command should remain first");
		ExpectCommand(target.commands[1], { { 1.0F, 0.0F }, { 2.0F, 1.0F } }, SecondMaterial, 1, 1, "first appended command should get fresh order");
		ExpectCommand(target.commands[2], { { 2.0F, 0.0F }, { 3.0F, 1.0F } }, FirstMaterial, 2, 2, "second appended command should get fresh order");
	}
}

void TestArbitrarySourceOrdersNormalizeToFinalOrder()
{
	iggy::render::RenderCommandList2D target = List({
		Command({ { -1.0F, 0.0F }, { 0.0F, 1.0F } }, FirstMaterial, 0, 88),
	});
	const iggy::render::RenderCommandList2D source = List({
		Command({ { 0.0F, 0.0F }, { 1.0F, 1.0F } }, SecondMaterial, 0, 10),
		Command({ { 1.0F, 0.0F }, { 2.0F, 1.0F } }, SecondMaterial, 0, 0),
		Command({ { 2.0F, 0.0F }, { 3.0F, 1.0F } }, SecondMaterial, 0, 999),
	});

	iggy::render::RenderCommandList2DComposer {}.append(target, source);

	Expect(target.commands.size() == 4, "source commands with arbitrary order should append");
	if (target.commands.size() == 4) {
		Expect(target.commands[1].order == 1, "first appended arbitrary-order command should normalize to order 1");
		Expect(target.commands[2].order == 2, "second appended arbitrary-order command should normalize to order 2");
		Expect(target.commands[3].order == 3, "third appended arbitrary-order command should normalize to order 3");
	}
}

void TestLayersArePreservedButNotSorted()
{
	iggy::render::RenderCommandList2D target;
	const iggy::render::RenderCommandList2D source = List({
		Command({ { 0.0F, 0.0F }, { 1.0F, 1.0F } }, FirstMaterial, 10, 0),
		Command({ { 1.0F, 0.0F }, { 2.0F, 1.0F } }, SecondMaterial, -5, 1),
	});

	iggy::render::RenderCommandList2DComposer {}.append(target, source);

	Expect(target.commands.size() == 2, "two layered commands should append");
	if (target.commands.size() == 2) {
		Expect(target.commands[0].layer == 10 && target.commands[0].order == 0, "first layer should remain first without sorting");
		Expect(target.commands[1].layer == -5 && target.commands[1].order == 1, "second layer should remain second without sorting");
	}
}

void TestAppendPreservesTexturePayloadAndNormalizesOrder()
{
	iggy::render::RenderCommandList2D target = List({
		Command({ { -1.0F, -1.0F }, { 0.0F, 0.0F } }, FirstMaterial, 0, 0),
	});
	const iggy::Rect2 sourceRect { { 8.0F, 16.0F }, { 24.0F, 32.0F } };
	const iggy::render::RenderCommandList2D source = List({
		TexturedCommand({ { 0.0F, 0.0F }, { 1.0F, 1.0F } }, SecondMaterial, iggy::ResourceId { "texture:hero" }, sourceRect, 4, 99),
	});

	iggy::render::RenderCommandList2DComposer {}.append(target, source);

	Expect(target.commands.size() == 2, "textured command should append after existing target command");
	if (target.commands.size() == 2) {
		ExpectCommand(target.commands[1], { { 0.0F, 0.0F }, { 1.0F, 1.0F } }, SecondMaterial, 4, 1, "appended textured command should normalize order");
		ExpectTexturePayload(target.commands[1], iggy::ResourceId { "texture:hero" }, sourceRect, "append should preserve texture id, source rect, and presence flag");
	}
}

void TestMergedPreservesMixedTexturePayloads()
{
	const iggy::Rect2 sourceRect { { 0.0F, 0.0F }, { 16.0F, 16.0F } };
	const iggy::render::RenderCommandList2D first = List({
		Command({ { 0.0F, 0.0F }, { 1.0F, 1.0F } }, FirstMaterial, 0, 42),
	});
	const iggy::render::RenderCommandList2D second = List({
		TexturedCommand({ { 1.0F, 0.0F }, { 2.0F, 1.0F } }, SecondMaterial, iggy::ResourceId { "texture:hero" }, sourceRect, 1, 99),
	});

	const iggy::render::RenderCommandList2D merged = iggy::render::RenderCommandList2DComposer {}.merged(first, second);

	Expect(merged.commands.size() == 2, "mixed material/textured merge should produce two commands");
	if (merged.commands.size() == 2) {
		ExpectNoTexturePayload(merged.commands[0], "merged material-only command should keep empty texture payload");
		ExpectTexturePayload(merged.commands[1], iggy::ResourceId { "texture:hero" }, sourceRect, "merged textured command should preserve payload");
		Expect(merged.commands[0].order == 0 && merged.commands[1].order == 1, "mixed merge should normalize final order");
	}
}

void TestSelfAppendUsesSnapshotAndFreshOrders()
{
	iggy::render::RenderCommandList2D target = List({
		Command({ { 0.0F, 0.0F }, { 1.0F, 1.0F } }, FirstMaterial, 0, 0),
		TexturedCommand({ { 1.0F, 0.0F }, { 2.0F, 1.0F } }, SecondMaterial, iggy::ResourceId { "texture:self" }, { { 2.0F, 3.0F }, { 4.0F, 5.0F } }, 1, 1),
	});

	iggy::render::RenderCommandList2DComposer {}.append(target, target);

	Expect(target.commands.size() == 4, "self append should duplicate the original snapshot once");
	if (target.commands.size() == 4) {
		ExpectCommand(target.commands[0], { { 0.0F, 0.0F }, { 1.0F, 1.0F } }, FirstMaterial, 0, 0, "self append should keep original first command");
		ExpectCommand(target.commands[1], { { 1.0F, 0.0F }, { 2.0F, 1.0F } }, SecondMaterial, 1, 1, "self append should keep original second command");
		ExpectCommand(target.commands[2], { { 0.0F, 0.0F }, { 1.0F, 1.0F } }, FirstMaterial, 0, 2, "self append should append copied first command with fresh order");
		ExpectCommand(target.commands[3], { { 1.0F, 0.0F }, { 2.0F, 1.0F } }, SecondMaterial, 1, 3, "self append should append copied second command with fresh order");
		ExpectTexturePayload(target.commands[1], iggy::ResourceId { "texture:self" }, { { 2.0F, 3.0F }, { 4.0F, 5.0F } }, "self append should preserve original textured payload");
		ExpectTexturePayload(target.commands[3], iggy::ResourceId { "texture:self" }, { { 2.0F, 3.0F }, { 4.0F, 5.0F } }, "self append should preserve copied textured payload");
	}
}

void TestMergedReturnsNewListWithoutMutatingInputs()
{
	const iggy::render::RenderCommandList2D first = List({
		Command({ { 0.0F, 0.0F }, { 1.0F, 1.0F } }, FirstMaterial, 0, 99),
	});
	const iggy::render::RenderCommandList2D second = List({
		Command({ { 1.0F, 0.0F }, { 2.0F, 1.0F } }, SecondMaterial, 1, 77),
	});

	const iggy::render::RenderCommandList2D merged = iggy::render::RenderCommandList2DComposer {}.merged(first, second);

	Expect(first.commands.size() == 1 && first.commands[0].order == 99, "merged should not mutate first input");
	Expect(second.commands.size() == 1 && second.commands[0].order == 77, "merged should not mutate second input");
	Expect(merged.commands.size() == 2, "merged should produce a two-command list");
	if (merged.commands.size() == 2) {
		ExpectCommand(merged.commands[0], { { 0.0F, 0.0F }, { 1.0F, 1.0F } }, FirstMaterial, 0, 0, "merged first command should normalize to order 0");
		ExpectCommand(merged.commands[1], { { 1.0F, 0.0F }, { 2.0F, 1.0F } }, SecondMaterial, 1, 1, "merged second command should normalize to order 1");
	}
}

void TestTileAndNpcRenderCommandsComposeWithNormalizedOrder()
{
	const iggy::LevelTileMap map = iggy::test::MapFromRows({
		"..",
		".#",
	});
	const iggy::CameraViewResult view = iggy::CameraView {}.visibleWorldBounds({ { 1.0F, 1.0F } }, { { 2.0F, 2.0F }, 1.0F });
	const iggy::LevelVisibleTilesResult visible = iggy::LevelVisibleTiles {}.query(map, view.bounds);
	const iggy::LevelTileDrawListResult drawList = iggy::LevelTileDrawList {}.build(map, visible.tiles);
	const iggy::render::RenderCommandList2D tileCommands = iggy::LevelTileRenderCommands {}.build(drawList, { { FloorMaterial, WallMaterial }, 1 });

	const std::vector<iggy::npc_ai::NpcAgentEntry> agents {
		Agent("npc:one", { 0.5F, 0.5F }),
	};
	const iggy::npc_ai::NpcAgentRenderCommandConfig npcConfig { NpcMaterial, { 1.0F, 1.0F }, { 0.5F, 0.5F }, 8 };
	const iggy::render::RenderCommandList2D npcCommands = iggy::npc_ai::NpcAgentRenderCommands {}.build(agents, npcConfig);

	const iggy::render::RenderCommandList2D merged = iggy::render::RenderCommandList2DComposer {}.merged(tileCommands, npcCommands);

	Expect(tileCommands.commands.size() == 4, "composition setup should produce tile commands");
	Expect(npcCommands.commands.size() == 1, "composition setup should produce NPC commands");
	Expect(merged.commands.size() == 5, "composer should merge tile and NPC command lists");
	if (merged.commands.size() == 5) {
		Expect(merged.commands[0].materialId == FloorMaterial && merged.commands[0].order == 0, "merged list should start with first tile command");
		Expect(merged.commands[3].materialId == WallMaterial && merged.commands[3].order == 3, "merged list should preserve blocked tile command position");
		ExpectCommand(merged.commands[4], { { 0.0F, 0.0F }, { 1.0F, 1.0F } }, NpcMaterial, 8, 4, "merged list should append NPC command with normalized final order");
	}
}

} // namespace

int main()
{
	TestEmptyTargetAndEmptySourceRemainEmpty();
	TestEmptySourceDoesNotChangeTarget();
	TestAppendOneCommandToEmptyTargetNormalizesOrder();
	TestAppendMultipleCommandsToNonEmptyTarget();
	TestArbitrarySourceOrdersNormalizeToFinalOrder();
	TestLayersArePreservedButNotSorted();
	TestAppendPreservesTexturePayloadAndNormalizesOrder();
	TestMergedPreservesMixedTexturePayloads();
	TestSelfAppendUsesSnapshotAndFreshOrders();
	TestMergedReturnsNewListWithoutMutatingInputs();
	TestTileAndNpcRenderCommandsComposeWithNormalizedOrder();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
