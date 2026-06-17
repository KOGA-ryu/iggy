#include "runtime/RuntimeGameplayAsciiSourcePlanFinalDebugRows.hpp"

#include <iostream>
#include <string>
#include <vector>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, const char *message)
{
	if (!condition) {
		std::cerr << "FAIL: " << message << '\n';
		++Failures;
	}
}

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId(value);
}

iggy::runtime::RuntimeGameplayAsciiSourcePlan Plan()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan;
	plan.grid.width = 7;
	plan.grid.height = 4;
	plan.grid.backgroundGlyph = '.';
	plan.grid.rows = {
		"#######",
		"#A@.k.#",
		"#.....#",
		"#######",
	};

	iggy::runtime::RuntimeGameplayAsciiSourcePlanGlyphLegendEntry actor;
	actor.glyph = 'A';
	actor.kind = iggy::runtime::RuntimeGameplayAsciiSourcePlanGlyphKind::Actor;
	actor.mapsToScenarioMarker = true;
	actor.scenarioMarkerKind =
		iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::Actor;
	plan.legend.push_back(actor);

	iggy::runtime::RuntimeGameplayAsciiSourcePlanGlyphLegendEntry player;
	player.glyph = '@';
	player.kind =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanGlyphKind::PlayerStart;
	player.mapsToScenarioMarker = true;
	player.scenarioMarkerKind =
		iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::PlayerStart;
	plan.legend.push_back(player);

	iggy::runtime::RuntimeGameplayAsciiSourcePlanAnnotatedCell guard;
	guard.glyph = 'A';
	guard.markerId = Id("npc:guard");
	plan.annotatedCells.push_back(guard);

	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredItemDrop key;
	key.dropId = Id("drop:key");
	key.itemId = Id("item:key");
	key.count = 1;
	key.glyph = 'k';
	plan.authoredItemDrops.push_back(key);

	return plan;
}

iggy::runtime::RuntimeGameplayState StateWithEnabledDrop()
{
	iggy::runtime::RuntimeGameplayState state;
	state.session.hasPlayer = true;
	state.session.player.position = { 3.5F, 1.5F };

	iggy::NpcActorState2D actor;
	actor.npcId = Id("npc:guard");
	actor.position = { 2.5F, 1.5F };
	state.npcActors.actors.push_back(actor);

	iggy::LevelItemDrop2D drop;
	drop.id = Id("drop:key");
	drop.itemId = Id("item:key");
	drop.count = 1;
	drop.position = { 4.5F, 1.5F };
	drop.enabled = true;
	state.inventory.drops.drops.push_back(drop);

	return state;
}

void TestMovedActorPlayerAndEnabledDropRender()
{
	const std::vector<std::string> rows =
		iggy::runtime::finalDebugRowsForAsciiSourcePlan(
			Plan(),
			StateWithEnabledDrop());
	const std::vector<std::string> expected {
		"#######",
		"#.A@k.#",
		"#.....#",
		"#######",
	};

	Expect(rows == expected, "final rows should clear source glyphs and overlay final actor, player, and drop");
}

void TestDisabledDropDoesNotRender()
{
	iggy::runtime::RuntimeGameplayState state = StateWithEnabledDrop();
	state.inventory.drops.drops[0].enabled = false;

	const std::vector<std::string> rows =
		iggy::runtime::finalDebugRowsForAsciiSourcePlan(Plan(), state);
	const std::vector<std::string> expected {
		"#######",
		"#.A@..#",
		"#.....#",
		"#######",
	};

	Expect(rows == expected, "disabled item drop should remain cleared from final rows");
}

} // namespace

int main()
{
	TestMovedActorPlayerAndEnabledDropRender();
	TestDisabledDropDoesNotRender();

	if (Failures != 0)
		return 1;
	return 0;
}
