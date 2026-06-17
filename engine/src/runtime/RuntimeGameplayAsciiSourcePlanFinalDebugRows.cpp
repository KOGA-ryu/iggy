#include "runtime/RuntimeGameplayAsciiSourcePlanFinalDebugRows.hpp"

#include "scene/debug/SceneAsciiCanvas2D.hpp"
#include "scene/level/TileCoord.hpp"
#include "scene/player/PlayerAgentState.hpp"

namespace iggy::runtime {
namespace {

char GlyphForActor(
	const RuntimeGameplayAsciiSourcePlan &plan,
	const ResourceId &npcId)
{
	for (const RuntimeGameplayAsciiSourcePlanAnnotatedCell &cell :
		plan.annotatedCells) {
		if (cell.markerId == npcId)
			return cell.glyph;
	}
	return 'N';
}

char GlyphForItemDrop(
	const RuntimeGameplayAsciiSourcePlan &plan,
	const ResourceId &dropId)
{
	for (const RuntimeGameplayAsciiSourcePlanAuthoredItemDrop &drop :
		plan.authoredItemDrops) {
		if (drop.dropId == dropId && drop.glyph != '\0')
			return drop.glyph;
	}
	return 'i';
}

bool IsAuthoredItemDropGlyph(
	const RuntimeGameplayAsciiSourcePlan &plan,
	char glyph)
{
	for (const RuntimeGameplayAsciiSourcePlanAuthoredItemDrop &drop :
		plan.authoredItemDrops) {
		if (drop.glyph == glyph && glyph != '\0')
			return true;
	}
	return false;
}

char BaseDebugGlyphForSource(
	const RuntimeGameplayAsciiSourcePlan &plan,
	char glyph)
{
	if (IsAuthoredItemDropGlyph(plan, glyph))
		return '.';

	for (const RuntimeGameplayAsciiSourcePlanGlyphLegendEntry &entry :
		plan.legend) {
		if (entry.glyph != glyph)
			continue;
		if (entry.kind == RuntimeGameplayAsciiSourcePlanGlyphKind::Actor
			|| entry.scenarioMarkerKind == RuntimeGameplayAsciiScenarioMarkerKind::Actor) {
			return '.';
		}
		if (entry.kind == RuntimeGameplayAsciiSourcePlanGlyphKind::PlayerStart
			|| entry.scenarioMarkerKind == RuntimeGameplayAsciiScenarioMarkerKind::PlayerStart) {
			return '.';
		}
	}
	return glyph;
}

} // namespace

std::vector<std::string> finalDebugRowsForAsciiSourcePlan(
	const RuntimeGameplayAsciiSourcePlan &plan,
	const RuntimeGameplayState &state)
{
	SceneAsciiCanvas2D canvas =
		makeSceneAsciiCanvas2D(plan.grid.width, plan.grid.height, ' ');

	for (std::size_t row = 0; row < plan.grid.rows.size(); ++row) {
		const std::string &text = plan.grid.rows[row];
		for (std::size_t column = 0; column < text.size(); ++column) {
			canvas = setSceneAsciiCanvas2DPoint(
				canvas,
				column,
				row,
				BaseDebugGlyphForSource(plan, text[column])).canvas;
		}
	}

	for (const NpcActorState2D &actor : state.npcActors.actors) {
		if (!actor.present)
			continue;
		const TileCoord tile = tileForPoint(actor.position);
		if (tile.x < 0 || tile.y < 0)
			continue;
		canvas = setSceneAsciiCanvas2DPoint(
			canvas,
			static_cast<std::size_t>(tile.x),
			static_cast<std::size_t>(tile.y),
			GlyphForActor(plan, actor.npcId)).canvas;
	}

	for (const LevelItemDrop2D &drop : state.inventory.drops.drops) {
		if (!drop.enabled)
			continue;
		const TileCoord tile = tileForPoint(drop.position);
		if (tile.x < 0 || tile.y < 0)
			continue;
		canvas = setSceneAsciiCanvas2DPoint(
			canvas,
			static_cast<std::size_t>(tile.x),
			static_cast<std::size_t>(tile.y),
			GlyphForItemDrop(plan, drop.id)).canvas;
	}

	if (state.session.hasPlayer) {
		const TileCoord tile = playerTile(state.session.player);
		if (tile.x >= 0 && tile.y >= 0) {
			canvas = setSceneAsciiCanvas2DPoint(
				canvas,
				static_cast<std::size_t>(tile.x),
				static_cast<std::size_t>(tile.y),
				'@').canvas;
		}
	}

	return renderSceneAsciiCanvas2DRows(canvas);
}

} // namespace iggy::runtime
