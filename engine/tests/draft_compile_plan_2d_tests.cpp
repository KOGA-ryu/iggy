#include <cstdlib>
#include <vector>

#include "scene/draft/DraftCompilePlan2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::DraftSymbol2D Symbol(
	const char *id,
	iggy::DraftSymbol2DKind kind,
	iggy::Vec2 position = { 0.0F, 0.0F },
	iggy::Vec2 size = { 1.0F, 1.0F },
	float rotationRadians = 0.0F,
	const char *assetId = "asset:symbol",
	const char *definitionId = "definition:symbol",
	bool enabled = true)
{
	return {
		Id(id),
		kind,
		position,
		size,
		rotationRadians,
		Id(assetId),
		Id(definitionId),
		enabled,
	};
}

iggy::DraftDocument2D Document(std::vector<iggy::DraftSymbol2D> symbols)
{
	return { symbols };
}

void ExpectSymbol(const iggy::DraftSymbol2D &actual, const iggy::DraftSymbol2D &expected, const char *message)
{
	Expect(actual.id == expected.id, message);
	Expect(actual.kind == expected.kind, message);
	Expect(NearVec(actual.position, expected.position), message);
	Expect(NearVec(actual.size, expected.size), message);
	Expect(actual.rotationRadians == expected.rotationRadians, message);
	Expect(actual.assetId == expected.assetId, message);
	Expect(actual.definitionId == expected.definitionId, message);
	Expect(actual.enabled == expected.enabled, message);
}

bool SameSymbols(const std::vector<iggy::DraftSymbol2D> &actual, const std::vector<iggy::DraftSymbol2D> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (actual[index].id != expected[index].id
			|| actual[index].kind != expected[index].kind
			|| !NearVec(actual[index].position, expected[index].position)
			|| !NearVec(actual[index].size, expected[index].size)
			|| actual[index].rotationRadians != expected[index].rotationRadians
			|| actual[index].assetId != expected[index].assetId
			|| actual[index].definitionId != expected[index].definitionId
			|| actual[index].enabled != expected[index].enabled) {
			return false;
		}
	}
	return true;
}

void TestEmptyDocumentProducesEmptyPlan()
{
	const iggy::DraftCompilePlan2DResult result = iggy::DraftCompilePlanner2D {}.plan({});

	Expect(result.walls.empty(), "empty draft compile plan should have no walls");
	Expect(result.doors.empty(), "empty draft compile plan should have no doors");
	Expect(result.objects.empty(), "empty draft compile plan should have no objects");
	Expect(result.itemDrops.empty(), "empty draft compile plan should have no item drops");
	Expect(result.npcs.empty(), "empty draft compile plan should have no npcs");
	Expect(result.markers.empty(), "empty draft compile plan should have no markers");
	Expect(result.collisionBlockers.empty(), "empty draft compile plan should have no collision blockers");
	Expect(result.ignoredDisabledSymbols.empty(), "empty draft compile plan should have no ignored symbols");
	Expect(result.issues.empty(), "empty draft compile plan should have no issues");
	Expect(!result.hasIssues(), "empty draft compile plan should report no issues");
}

void TestMixedSymbolsAreClassified()
{
	const std::vector<iggy::DraftSymbol2D> symbols {
		Symbol("draft:wall", iggy::DraftSymbol2DKind::Wall, { 1.0F, 0.0F }, { 4.0F, 1.0F }, 0.1F, "asset:wall", "definition:wall"),
		Symbol("draft:door", iggy::DraftSymbol2DKind::Door, { 2.0F, 0.0F }, { 1.0F, 2.0F }, 0.2F, "asset:door", "definition:door"),
		Symbol("draft:object", iggy::DraftSymbol2DKind::Object, { 3.0F, 0.0F }, { 1.0F, 1.0F }, 0.3F, "asset:object", "definition:object"),
		Symbol("draft:furniture", iggy::DraftSymbol2DKind::Furniture, { 4.0F, 0.0F }, { 2.0F, 2.0F }, 0.4F, "asset:furniture", "definition:furniture"),
		Symbol("draft:item", iggy::DraftSymbol2DKind::ItemDrop, { 5.0F, 0.0F }, { 1.0F, 1.0F }, 0.5F, "asset:item", "definition:item"),
		Symbol("draft:npc", iggy::DraftSymbol2DKind::Npc, { 6.0F, 0.0F }, { 1.0F, 2.0F }, 0.6F, "asset:npc", "definition:npc"),
		Symbol("draft:region", iggy::DraftSymbol2DKind::Region, { 7.0F, 0.0F }, { 5.0F, 5.0F }, 0.7F, "asset:region", "definition:region"),
		Symbol("draft:story", iggy::DraftSymbol2DKind::StoryMarker, { 8.0F, 0.0F }, { 0.0F, 0.0F }, 0.8F, "asset:story", "definition:story"),
		Symbol("draft:blocker", iggy::DraftSymbol2DKind::CollisionBlocker, { 9.0F, 0.0F }, { 3.0F, 2.0F }, 0.0F, "asset:blocker", "definition:blocker"),
	};

	const iggy::DraftCompilePlan2DResult result =
		iggy::DraftCompilePlanner2D {}.plan(Document(symbols));

	Expect(result.walls.size() == 1, "mixed draft compile plan should classify wall");
	Expect(result.doors.size() == 1, "mixed draft compile plan should classify door");
	Expect(result.objects.size() == 2, "mixed draft compile plan should classify object and furniture together");
	Expect(result.itemDrops.size() == 1, "mixed draft compile plan should classify item drop");
	Expect(result.npcs.size() == 1, "mixed draft compile plan should classify npc");
	Expect(result.markers.size() == 2, "mixed draft compile plan should classify region and story markers together");
	Expect(result.collisionBlockers.size() == 1, "mixed draft compile plan should classify collision blocker");
	if (result.walls.size() == 1) {
		Expect(result.walls[0].symbolIndex == 0, "wall plan should preserve original index");
		ExpectSymbol(result.walls[0].symbol, symbols[0], "wall plan should copy symbol payload");
	}
	if (result.doors.size() == 1) {
		Expect(result.doors[0].symbolIndex == 1, "door plan should preserve original index");
		ExpectSymbol(result.doors[0].symbol, symbols[1], "door plan should copy symbol payload");
	}
	if (result.objects.size() == 2) {
		Expect(result.objects[0].symbolIndex == 2 && result.objects[1].symbolIndex == 3, "object plans should preserve original indexes");
		ExpectSymbol(result.objects[0].symbol, symbols[2], "object plan should copy object payload");
		ExpectSymbol(result.objects[1].symbol, symbols[3], "object plan should copy furniture payload");
	}
	if (result.itemDrops.size() == 1) {
		Expect(result.itemDrops[0].symbolIndex == 4, "item drop plan should preserve original index");
		ExpectSymbol(result.itemDrops[0].symbol, symbols[4], "item drop plan should copy symbol payload");
	}
	if (result.npcs.size() == 1) {
		Expect(result.npcs[0].symbolIndex == 5, "npc plan should preserve original index");
		ExpectSymbol(result.npcs[0].symbol, symbols[5], "npc plan should copy symbol payload");
	}
	if (result.markers.size() == 2) {
		Expect(result.markers[0].symbolIndex == 6 && result.markers[1].symbolIndex == 7, "marker plans should preserve original indexes");
		ExpectSymbol(result.markers[0].symbol, symbols[6], "marker plan should copy region payload");
		ExpectSymbol(result.markers[1].symbol, symbols[7], "marker plan should copy story payload");
	}
	if (result.collisionBlockers.size() == 1) {
		Expect(result.collisionBlockers[0].symbolIndex == 8, "collision blocker plan should preserve original index");
		ExpectSymbol(result.collisionBlockers[0].symbol, symbols[8], "collision blocker plan should copy symbol payload");
	}
	Expect(!result.hasIssues(), "mixed known draft symbols should have no compile issues");
}

void TestDisabledSymbolsAreIgnored()
{
	const std::vector<iggy::DraftSymbol2D> symbols {
		Symbol("draft:disabled_wall", iggy::DraftSymbol2DKind::Wall, { 1.0F, 0.0F }, { 1.0F, 1.0F }, 0.0F, "asset:wall", "definition:wall", false),
		Symbol("draft:disabled_unknown", iggy::DraftSymbol2DKind::Unknown, { 2.0F, 0.0F }, { 1.0F, 1.0F }, 0.0F, "", "", false),
		Symbol("draft:disabled_blocker", iggy::DraftSymbol2DKind::CollisionBlocker, { 3.0F, 0.0F }, { 2.0F, 2.0F }, 0.0F, "asset:blocker", "definition:blocker", false),
	};

	const iggy::DraftCompilePlan2DResult result =
		iggy::DraftCompilePlanner2D {}.plan(Document(symbols));

	Expect(result.walls.empty(), "disabled draft wall should not compile");
	Expect(result.collisionBlockers.empty(), "disabled collision blocker should not compile");
	Expect(result.issues.empty(), "disabled unknown draft symbol should not produce issue");
	Expect(result.ignoredDisabledSymbols.size() == 3, "disabled draft symbols should be recorded as ignored");
	if (result.ignoredDisabledSymbols.size() == 3) {
		Expect(result.ignoredDisabledSymbols[0].symbolIndex == 0, "first ignored symbol should preserve index");
		ExpectSymbol(result.ignoredDisabledSymbols[0].symbol, symbols[0], "first ignored symbol should copy payload");
		Expect(result.ignoredDisabledSymbols[1].symbolIndex == 1, "second ignored symbol should preserve index");
		ExpectSymbol(result.ignoredDisabledSymbols[1].symbol, symbols[1], "second ignored symbol should copy payload");
		Expect(result.ignoredDisabledSymbols[2].symbolIndex == 2, "third ignored symbol should preserve index");
		ExpectSymbol(result.ignoredDisabledSymbols[2].symbol, symbols[2], "third ignored symbol should copy payload");
	}
}

void TestEnabledUnknownSymbolsProduceIssues()
{
	const std::vector<iggy::DraftSymbol2D> symbols {
		Symbol("draft:unknown", iggy::DraftSymbol2DKind::Unknown, { 1.0F, 1.0F }, { 0.0F, 0.0F }, 0.0F, "", ""),
	};

	const iggy::DraftCompilePlan2DResult result =
		iggy::DraftCompilePlanner2D {}.plan(Document(symbols));

	Expect(result.walls.empty() && result.doors.empty() && result.objects.empty(), "unknown draft symbol should produce no compile output");
	Expect(result.issues.size() == 1, "unknown draft symbol should produce one issue");
	Expect(result.hasIssues(), "unknown draft symbol should make result report issues");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::DraftCompilePlan2DIssueCode::UnknownSymbolKind, "unknown draft issue should use UnknownSymbolKind");
		Expect(result.issues[0].symbolIndex == 0, "unknown draft issue should preserve original index");
		ExpectSymbol(result.issues[0].symbol, symbols[0], "unknown draft issue should copy symbol payload");
	}
}

void TestMultipleUnknownIssuesPreserveInputOrder()
{
	const std::vector<iggy::DraftSymbol2D> symbols {
		Symbol("draft:unknown_a", iggy::DraftSymbol2DKind::Unknown),
		Symbol("draft:wall", iggy::DraftSymbol2DKind::Wall),
		Symbol("draft:unknown_b", iggy::DraftSymbol2DKind::Unknown),
	};

	const iggy::DraftCompilePlan2DResult result =
		iggy::DraftCompilePlanner2D {}.plan(Document(symbols));

	Expect(result.issues.size() == 2, "multiple unknown draft symbols should produce two issues");
	if (result.issues.size() == 2) {
		Expect(result.issues[0].symbolIndex == 0, "first unknown issue should preserve input order");
		Expect(result.issues[1].symbolIndex == 2, "second unknown issue should preserve input order");
		ExpectSymbol(result.issues[0].symbol, symbols[0], "first unknown issue should copy payload");
		ExpectSymbol(result.issues[1].symbol, symbols[2], "second unknown issue should copy payload");
	}
	Expect(result.walls.size() == 1 && result.walls[0].symbolIndex == 1, "known symbol between unknowns should still compile");
}

void TestRelativeOrderWithinCategoriesIsPreserved()
{
	const std::vector<iggy::DraftSymbol2D> symbols {
		Symbol("draft:wall_a", iggy::DraftSymbol2DKind::Wall),
		Symbol("draft:object_a", iggy::DraftSymbol2DKind::Object),
		Symbol("draft:wall_b", iggy::DraftSymbol2DKind::Wall),
		Symbol("draft:furniture_b", iggy::DraftSymbol2DKind::Furniture),
		Symbol("draft:marker_a", iggy::DraftSymbol2DKind::Region),
		Symbol("draft:marker_b", iggy::DraftSymbol2DKind::StoryMarker),
	};

	const iggy::DraftCompilePlan2DResult result =
		iggy::DraftCompilePlanner2D {}.plan(Document(symbols));

	Expect(result.walls.size() == 2, "ordered draft compile plan should include two walls");
	Expect(result.objects.size() == 2, "ordered draft compile plan should include two object-like symbols");
	Expect(result.markers.size() == 2, "ordered draft compile plan should include two markers");
	if (result.walls.size() == 2) {
		Expect(result.walls[0].symbol.id == Id("draft:wall_a"), "wall category should preserve first wall order");
		Expect(result.walls[1].symbol.id == Id("draft:wall_b"), "wall category should preserve second wall order");
	}
	if (result.objects.size() == 2) {
		Expect(result.objects[0].symbol.id == Id("draft:object_a"), "object category should preserve first object order");
		Expect(result.objects[1].symbol.id == Id("draft:furniture_b"), "object category should preserve furniture order");
	}
	if (result.markers.size() == 2) {
		Expect(result.markers[0].symbol.id == Id("draft:marker_a"), "marker category should preserve region order");
		Expect(result.markers[1].symbol.id == Id("draft:marker_b"), "marker category should preserve story order");
	}
}

void TestInputDocumentIsNotMutated()
{
	const std::vector<iggy::DraftSymbol2D> symbols {
		Symbol("draft:wall", iggy::DraftSymbol2DKind::Wall, { 1.0F, 2.0F }, { 3.0F, 4.0F }, 0.5F, "asset:wall", "definition:wall"),
		Symbol("draft:disabled", iggy::DraftSymbol2DKind::Door, { 5.0F, 6.0F }, { 1.0F, 2.0F }, 1.0F, "asset:door", "definition:door", false),
		Symbol("draft:unknown", iggy::DraftSymbol2DKind::Unknown, { 7.0F, 8.0F }, { 0.0F, 0.0F }, 0.0F, "", ""),
	};
	const iggy::DraftDocument2D document = Document(symbols);
	const std::vector<iggy::DraftSymbol2D> before = document.symbols;

	const iggy::DraftCompilePlan2DResult result = iggy::DraftCompilePlanner2D {}.plan(document);

	Expect(result.walls.size() == 1 && result.ignoredDisabledSymbols.size() == 1 && result.issues.size() == 1, "draft compile immutability setup should exercise all result paths");
	Expect(SameSymbols(document.symbols, before), "draft compile planner should not mutate input document");
}

} // namespace

int main()
{
	TestEmptyDocumentProducesEmptyPlan();
	TestMixedSymbolsAreClassified();
	TestDisabledSymbolsAreIgnored();
	TestEnabledUnknownSymbolsProduceIssues();
	TestMultipleUnknownIssuesPreserveInputOrder();
	TestRelativeOrderWithinCategoriesIsPreserved();
	TestInputDocumentIsNotMutated();

	return Failures;
}
