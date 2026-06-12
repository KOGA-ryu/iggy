#include <cstdlib>

#include "core/resource/ResourceId.hpp"
#include "scene/camera/CameraView.hpp"
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

const iggy::ResourceId WalkableMaterial { "material:floor" };
const iggy::ResourceId BlockedMaterial { "material:wall" };

iggy::LevelTileRenderCommandConfig Config(int layer = 3)
{
	return { { WalkableMaterial, BlockedMaterial }, layer };
}

iggy::LevelTileDrawItem DrawItem(iggy::TileCoord tile, bool walkable, std::size_t index)
{
	return {
		tile,
		{ { static_cast<float>(tile.x), static_cast<float>(tile.y) }, { static_cast<float>(tile.x + 1), static_cast<float>(tile.y + 1) } },
		walkable,
		index,
	};
}

void ExpectCommand(const iggy::render::RenderCommand2D &command, iggy::Aabb2 bounds, const iggy::ResourceId &materialId, int layer, std::size_t order, const char *message)
{
	Expect(command.type == iggy::render::RenderCommand2DType::Quad && SameBounds(command.worldBounds, bounds) && command.materialId == materialId && command.layer == layer && command.order == order, message);
}

void TestEmptyDrawListReturnsEmptyCommands()
{
	const iggy::render::RenderCommandList2D commands = iggy::LevelTileRenderCommands {}.build({}, Config());

	Expect(commands.commands.empty(), "empty draw list should return empty render command list");
}

void TestWalkableTileUsesWalkableMaterial()
{
	iggy::LevelTileDrawListResult drawList;
	drawList.items.push_back(DrawItem({ 1, 2 }, true, 9));

	const iggy::render::RenderCommandList2D commands = iggy::LevelTileRenderCommands {}.build(drawList, Config(4));

	Expect(commands.commands.size() == 1, "one walkable draw item should emit one command");
	if (commands.commands.size() == 1)
		ExpectCommand(commands.commands[0], drawList.items[0].worldBounds, WalkableMaterial, 4, 0, "walkable tile should use walkable material");
}

void TestBlockedTileUsesBlockedMaterial()
{
	iggy::LevelTileDrawListResult drawList;
	drawList.items.push_back(DrawItem({ 1, 2 }, false, 9));

	const iggy::render::RenderCommandList2D commands = iggy::LevelTileRenderCommands {}.build(drawList, Config(4));

	Expect(commands.commands.size() == 1, "one blocked draw item should emit one command");
	if (commands.commands.size() == 1)
		ExpectCommand(commands.commands[0], drawList.items[0].worldBounds, BlockedMaterial, 4, 0, "blocked tile should use blocked material");
}

void TestMultipleItemsPreserveOrderAndCommandOrder()
{
	iggy::LevelTileDrawListResult drawList;
	drawList.items.push_back(DrawItem({ 2, 0 }, true, 2));
	drawList.items.push_back(DrawItem({ 0, 0 }, false, 0));
	drawList.items.push_back(DrawItem({ 1, 0 }, true, 1));

	const iggy::render::RenderCommandList2D commands = iggy::LevelTileRenderCommands {}.build(drawList, Config(7));

	Expect(commands.commands.size() == 3, "three draw items should emit three commands");
	if (commands.commands.size() == 3) {
		ExpectCommand(commands.commands[0], drawList.items[0].worldBounds, WalkableMaterial, 7, 0, "first draw item should become first command");
		ExpectCommand(commands.commands[1], drawList.items[1].worldBounds, BlockedMaterial, 7, 1, "second draw item should become second command");
		ExpectCommand(commands.commands[2], drawList.items[2].worldBounds, WalkableMaterial, 7, 2, "third draw item should become third command");
	}
}

void TestDuplicateDrawItemsRemainDuplicateCommands()
{
	const iggy::LevelTileDrawItem item = DrawItem({ 1, 0 }, true, 1);
	iggy::LevelTileDrawListResult drawList;
	drawList.items.push_back(item);
	drawList.items.push_back(item);

	const iggy::render::RenderCommandList2D commands = iggy::LevelTileRenderCommands {}.build(drawList, Config());

	Expect(commands.commands.size() == 2, "duplicate draw items should emit duplicate commands");
	if (commands.commands.size() == 2) {
		ExpectCommand(commands.commands[0], item.worldBounds, WalkableMaterial, 3, 0, "first duplicate should be preserved");
		ExpectCommand(commands.commands[1], item.worldBounds, WalkableMaterial, 3, 1, "second duplicate should be preserved");
	}
}

void TestEmptyMaterialIdsArePreserved()
{
	iggy::LevelTileDrawListResult drawList;
	drawList.items.push_back(DrawItem({ 0, 0 }, true, 0));
	drawList.items.push_back(DrawItem({ 1, 0 }, false, 1));

	const iggy::render::RenderCommandList2D commands = iggy::LevelTileRenderCommands {}.build(drawList, { { {}, {} }, 2 });

	Expect(commands.commands.size() == 2, "empty material setup should still emit commands");
	if (commands.commands.size() == 2) {
		Expect(commands.commands[0].materialId.empty(), "empty walkable material should be preserved");
		Expect(commands.commands[1].materialId.empty(), "empty blocked material should be preserved");
	}
}

void TestCameraVisibleTilesDrawListRenderCommandComposition()
{
	const iggy::LevelTileMap map = iggy::test::MapFromRows({
		"....",
		".#..",
		"....",
	});
	const iggy::CameraViewResult view = iggy::CameraView {}.visibleWorldBounds({ { 1.5F, 1.0F } }, { { 2.0F, 2.0F }, 1.0F });
	const iggy::LevelVisibleTilesResult visible = iggy::LevelVisibleTiles {}.query(map, view.bounds);
	const iggy::LevelTileDrawListResult drawList = iggy::LevelTileDrawList {}.build(map, visible.tiles);
	const iggy::render::RenderCommandList2D commands = iggy::LevelTileRenderCommands {}.build(drawList, Config(6));

	Expect(visible.hasTiles, "composition should produce visible tiles");
	Expect(drawList.items.size() == 6, "composition should produce tile draw items");
	Expect(commands.commands.size() == drawList.items.size(), "composition should emit one render command per draw item");
	if (commands.commands.size() == 6) {
		ExpectCommand(commands.commands[0], drawList.items[0].worldBounds, WalkableMaterial, 6, 0, "composition first command should match first draw item");
		ExpectCommand(commands.commands[4], drawList.items[4].worldBounds, BlockedMaterial, 6, 4, "composition blocked tile should use blocked material");
		ExpectCommand(commands.commands[5], drawList.items[5].worldBounds, WalkableMaterial, 6, 5, "composition should preserve final command order");
	}
}

} // namespace

int main()
{
	TestEmptyDrawListReturnsEmptyCommands();
	TestWalkableTileUsesWalkableMaterial();
	TestBlockedTileUsesBlockedMaterial();
	TestMultipleItemsPreserveOrderAndCommandOrder();
	TestDuplicateDrawItemsRemainDuplicateCommands();
	TestEmptyMaterialIdsArePreserved();
	TestCameraVisibleTilesDrawListRenderCommandComposition();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
