#include <cstdlib>

#include "core/resource/ResourceId.hpp"
#include "scene/camera/CameraView.hpp"
#include "scene/level/LevelTileDrawList.hpp"
#include "scene/level/LevelVisibleTiles.hpp"
#include "servers/render/RenderCommand2D.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

bool SameBounds(iggy::Aabb2 actual, iggy::Aabb2 expected)
{
	return NearVec(actual.min, expected.min) && NearVec(actual.max, expected.max);
}

void ExpectCommand(const iggy::render::RenderCommand2D &command, iggy::Aabb2 bounds, const iggy::ResourceId &materialId, int layer, std::size_t order, const char *message)
{
	Expect(command.type == iggy::render::RenderCommand2DType::Quad && SameBounds(command.worldBounds, bounds) && command.materialId == materialId && command.layer == layer && command.order == order, message);
}

void TestEmptyCommandListIsValid()
{
	const iggy::render::RenderCommandList2D list;

	Expect(list.commands.empty(), "empty render command list should be valid and empty");
}

void TestAddOneQuadPreservesFields()
{
	iggy::render::RenderCommandList2D list;
	const iggy::Aabb2 bounds { { 1.0F, 2.0F }, { 3.0F, 4.0F } };
	const iggy::ResourceId materialId { "material:floor" };

	iggy::render::RenderCommandListBuilder2D {}.addQuad(list, bounds, materialId, 2);

	Expect(list.commands.size() == 1, "one added quad should append one command");
	if (list.commands.size() == 1)
		ExpectCommand(list.commands[0], bounds, materialId, 2, 0, "quad command should preserve supplied fields and order zero");
}

void TestMultipleCommandsPreserveInsertionOrder()
{
	iggy::render::RenderCommandList2D list;
	const iggy::render::RenderCommandListBuilder2D builder;

	builder.addQuad(list, { { 0.0F, 0.0F }, { 1.0F, 1.0F } }, iggy::ResourceId { "material:first" }, 0);
	builder.addQuad(list, { { 1.0F, 0.0F }, { 2.0F, 1.0F } }, iggy::ResourceId { "material:second" }, 0);
	builder.addQuad(list, { { 2.0F, 0.0F }, { 3.0F, 1.0F } }, iggy::ResourceId { "material:third" }, 0);

	Expect(list.commands.size() == 3, "three added quads should append three commands");
	if (list.commands.size() == 3) {
		Expect(list.commands[0].materialId == iggy::ResourceId { "material:first" } && list.commands[0].order == 0, "first command should remain first with order 0");
		Expect(list.commands[1].materialId == iggy::ResourceId { "material:second" } && list.commands[1].order == 1, "second command should remain second with order 1");
		Expect(list.commands[2].materialId == iggy::ResourceId { "material:third" } && list.commands[2].order == 2, "third command should remain third with order 2");
	}
}

void TestDifferentLayersAreStoredButNotSorted()
{
	iggy::render::RenderCommandList2D list;
	const iggy::render::RenderCommandListBuilder2D builder;

	builder.addQuad(list, { { 0.0F, 0.0F }, { 1.0F, 1.0F } }, iggy::ResourceId { "material:front" }, 10);
	builder.addQuad(list, { { 1.0F, 0.0F }, { 2.0F, 1.0F } }, iggy::ResourceId { "material:back" }, -5);

	Expect(list.commands.size() == 2, "different layer setup should append two commands");
	if (list.commands.size() == 2) {
		Expect(list.commands[0].layer == 10 && list.commands[0].order == 0, "first layer should be stored without sorting");
		Expect(list.commands[1].layer == -5 && list.commands[1].order == 1, "second layer should be stored without sorting");
	}
}

void TestEmptyMaterialIdIsPreserved()
{
	iggy::render::RenderCommandList2D list;
	const iggy::ResourceId emptyMaterial;

	iggy::render::RenderCommandListBuilder2D {}.addQuad(list, { { 0.0F, 0.0F }, { 1.0F, 1.0F } }, emptyMaterial, 0);

	Expect(list.commands.size() == 1, "empty material command should still append");
	if (list.commands.size() == 1)
		Expect(list.commands[0].materialId.empty(), "empty material id should be preserved without validation");
}

void TestLevelTileDrawItemsCanFeedRenderCommands()
{
	const iggy::ResourceId floorMaterial { "material:floor" };
	const iggy::ResourceId wallMaterial { "material:wall" };
	const iggy::LevelTileMap map = iggy::test::MapFromRows({
		"....",
		".#..",
		"....",
	});
	const iggy::CameraViewResult view = iggy::CameraView {}.visibleWorldBounds({ { 1.5F, 1.0F } }, { { 2.0F, 2.0F }, 1.0F });
	const iggy::LevelVisibleTilesResult visible = iggy::LevelVisibleTiles {}.query(map, view.bounds);
	const iggy::LevelTileDrawListResult drawList = iggy::LevelTileDrawList {}.build(map, visible.tiles);

	iggy::render::RenderCommandList2D commands;
	const iggy::render::RenderCommandListBuilder2D builder;
	for (const iggy::LevelTileDrawItem &item : drawList.items)
		builder.addQuad(commands, item.worldBounds, item.walkable ? floorMaterial : wallMaterial, 1);

	Expect(drawList.items.size() == 6, "composition setup should produce tile draw items");
	Expect(commands.commands.size() == drawList.items.size(), "composition should produce one command per draw item");
	if (commands.commands.size() == 6) {
		ExpectCommand(commands.commands[0], drawList.items[0].worldBounds, floorMaterial, 1, 0, "first tile draw item should feed first render command");
		ExpectCommand(commands.commands[4], drawList.items[4].worldBounds, wallMaterial, 1, 4, "blocked tile draw item should select wall material in caller code");
		ExpectCommand(commands.commands[5], drawList.items[5].worldBounds, floorMaterial, 1, 5, "last tile draw item should preserve order in render command list");
	}
}

} // namespace

int main()
{
	TestEmptyCommandListIsValid();
	TestAddOneQuadPreservesFields();
	TestMultipleCommandsPreserveInsertionOrder();
	TestDifferentLayersAreStoredButNotSorted();
	TestEmptyMaterialIdIsPreserved();
	TestLevelTileDrawItemsCanFeedRenderCommands();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
