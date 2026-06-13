#include <cstdlib>
#include <vector>

#include "scene/draft/DraftWallCompile2D.hpp"
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

void ExpectCompiledWall(const iggy::DraftCompiledWall2D &actual, const iggy::DraftWallPlan2D &expected, const char *message)
{
	Expect(actual.sourceSymbolId == expected.symbol.id, message);
	Expect(actual.sourceSymbolIndex == expected.symbolIndex, message);
	Expect(NearVec(actual.center, expected.symbol.position), message);
	Expect(NearVec(actual.size, expected.symbol.size), message);
	Expect(actual.rotationRadians == expected.symbol.rotationRadians, message);
	Expect(actual.assetId == expected.symbol.assetId, message);
	Expect(actual.definitionId == expected.symbol.definitionId, message);
}

bool SameWallPlans(const std::vector<iggy::DraftWallPlan2D> &actual, const std::vector<iggy::DraftWallPlan2D> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (actual[index].symbolIndex != expected[index].symbolIndex)
			return false;
		const iggy::DraftSymbol2D &actualSymbol = actual[index].symbol;
		const iggy::DraftSymbol2D &expectedSymbol = expected[index].symbol;
		if (actualSymbol.id != expectedSymbol.id
			|| actualSymbol.kind != expectedSymbol.kind
			|| !NearVec(actualSymbol.position, expectedSymbol.position)
			|| !NearVec(actualSymbol.size, expectedSymbol.size)
			|| actualSymbol.rotationRadians != expectedSymbol.rotationRadians
			|| actualSymbol.assetId != expectedSymbol.assetId
			|| actualSymbol.definitionId != expectedSymbol.definitionId
			|| actualSymbol.enabled != expectedSymbol.enabled) {
			return false;
		}
	}
	return true;
}

void TestEmptyPlanProducesEmptyCompileResult()
{
	const iggy::DraftWallCompile2DResult result = iggy::DraftWallCompiler2D {}.compile({});

	Expect(result.walls.empty(), "empty draft wall compile should have no walls");
	Expect(result.issues.empty(), "empty draft wall compile should have no issues");
	Expect(!result.hasIssues(), "empty draft wall compile should report no issues");
}

void TestValidWallCompilesPreservingPayload()
{
	const iggy::DraftWallPlan2D wall {
		7,
		Symbol("draft:wall", iggy::DraftSymbol2DKind::Wall, { 1.0F, 2.0F }, { 3.0F, 4.0F }, 0.5F, "asset:wall", "definition:wall"),
	};
	iggy::DraftCompilePlan2DResult plan;
	plan.walls.push_back(wall);

	const iggy::DraftWallCompile2DResult result = iggy::DraftWallCompiler2D {}.compile(plan);

	Expect(result.walls.size() == 1, "valid draft wall should compile");
	Expect(result.issues.empty(), "valid draft wall should not produce issues");
	if (result.walls.size() == 1)
		ExpectCompiledWall(result.walls[0], wall, "compiled draft wall should preserve source payload");
}

void TestMultipleWallsPreserveOrder()
{
	iggy::DraftCompilePlan2DResult plan;
	plan.walls.push_back({ 3, Symbol("draft:wall_a", iggy::DraftSymbol2DKind::Wall) });
	plan.walls.push_back({ 1, Symbol("draft:wall_b", iggy::DraftSymbol2DKind::Wall, { 5.0F, 6.0F }, { 2.0F, 3.0F }) });
	plan.walls.push_back({ 9, Symbol("draft:wall_c", iggy::DraftSymbol2DKind::Wall, { 7.0F, 8.0F }, { 4.0F, 5.0F }) });

	const iggy::DraftWallCompile2DResult result = iggy::DraftWallCompiler2D {}.compile(plan);

	Expect(result.walls.size() == 3, "multiple draft walls should compile");
	if (result.walls.size() == 3) {
		Expect(result.walls[0].sourceSymbolId == Id("draft:wall_a"), "first compiled wall should preserve plan order");
		Expect(result.walls[1].sourceSymbolId == Id("draft:wall_b"), "second compiled wall should preserve plan order");
		Expect(result.walls[2].sourceSymbolId == Id("draft:wall_c"), "third compiled wall should preserve plan order");
	}
}

void TestNonWallPlanCategoriesAreIgnored()
{
	iggy::DraftCompilePlan2DResult plan;
	plan.doors.push_back({ 0, Symbol("draft:door", iggy::DraftSymbol2DKind::Door) });
	plan.objects.push_back({ 1, Symbol("draft:object", iggy::DraftSymbol2DKind::Object) });
	plan.itemDrops.push_back({ 2, Symbol("draft:item", iggy::DraftSymbol2DKind::ItemDrop) });
	plan.npcs.push_back({ 3, Symbol("draft:npc", iggy::DraftSymbol2DKind::Npc) });
	plan.markers.push_back({ 4, Symbol("draft:marker", iggy::DraftSymbol2DKind::Region) });
	plan.ignoredDisabledSymbols.push_back({ 5, Symbol("draft:disabled", iggy::DraftSymbol2DKind::Wall, { 0.0F, 0.0F }, { 0.0F, 0.0F }, 0.0F, "", "", false) });
	plan.issues.push_back({ iggy::DraftCompilePlan2DIssueCode::UnknownSymbolKind, 6, Symbol("draft:unknown", iggy::DraftSymbol2DKind::Unknown) });

	const iggy::DraftWallCompile2DResult result = iggy::DraftWallCompiler2D {}.compile(plan);

	Expect(result.walls.empty(), "draft wall compiler should ignore non-wall categories");
	Expect(result.issues.empty(), "draft wall compiler should not copy non-wall plan issues");
}

void TestNonPositiveSizesProduceIssues()
{
	iggy::DraftCompilePlan2DResult plan;
	plan.walls.push_back({ 0, Symbol("draft:zero_width", iggy::DraftSymbol2DKind::Wall, { 0.0F, 0.0F }, { 0.0F, 1.0F }) });
	plan.walls.push_back({ 1, Symbol("draft:negative_width", iggy::DraftSymbol2DKind::Wall, { 0.0F, 0.0F }, { -1.0F, 1.0F }) });
	plan.walls.push_back({ 2, Symbol("draft:zero_height", iggy::DraftSymbol2DKind::Wall, { 0.0F, 0.0F }, { 1.0F, 0.0F }) });
	plan.walls.push_back({ 3, Symbol("draft:negative_height", iggy::DraftSymbol2DKind::Wall, { 0.0F, 0.0F }, { 1.0F, -1.0F }) });

	const iggy::DraftWallCompile2DResult result = iggy::DraftWallCompiler2D {}.compile(plan);

	Expect(result.walls.empty(), "non-positive draft wall sizes should not compile");
	Expect(result.issues.size() == 4, "non-positive draft wall sizes should produce issues");
	Expect(result.hasIssues(), "non-positive draft wall sizes should report issues");
	if (result.issues.size() == 4) {
		for (std::size_t index = 0; index < result.issues.size(); ++index) {
			Expect(result.issues[index].code == iggy::DraftWallCompile2DIssueCode::NonPositiveSize, "draft wall size issue should use NonPositiveSize");
			Expect(result.issues[index].symbolIndex == index, "draft wall size issues should preserve wall order");
			ExpectSymbol(result.issues[index].symbol, plan.walls[index].symbol, "draft wall size issue should copy wall payload");
		}
	}
}

void TestMixedValidAndInvalidWallsPreserveOutputsAndIssues()
{
	iggy::DraftCompilePlan2DResult plan;
	plan.walls.push_back({ 0, Symbol("draft:wall_a", iggy::DraftSymbol2DKind::Wall, { 1.0F, 0.0F }, { 1.0F, 2.0F }) });
	plan.walls.push_back({ 1, Symbol("draft:invalid", iggy::DraftSymbol2DKind::Wall, { 2.0F, 0.0F }, { 0.0F, 2.0F }) });
	plan.walls.push_back({ 2, Symbol("draft:wall_b", iggy::DraftSymbol2DKind::Wall, { 3.0F, 0.0F }, { 3.0F, 4.0F }) });

	const iggy::DraftWallCompile2DResult result = iggy::DraftWallCompiler2D {}.compile(plan);

	Expect(result.walls.size() == 2, "mixed draft wall compile should keep valid walls");
	Expect(result.issues.size() == 1, "mixed draft wall compile should report invalid wall");
	if (result.walls.size() == 2) {
		Expect(result.walls[0].sourceSymbolId == Id("draft:wall_a"), "first valid draft wall should compile first");
		Expect(result.walls[1].sourceSymbolId == Id("draft:wall_b"), "second valid draft wall should compile second");
	}
	if (result.issues.size() == 1) {
		Expect(result.issues[0].symbolIndex == 1, "mixed draft wall compile issue should preserve invalid source index");
		ExpectSymbol(result.issues[0].symbol, plan.walls[1].symbol, "mixed draft wall compile issue should copy invalid wall payload");
	}
}

void TestInputPlanIsNotMutated()
{
	iggy::DraftCompilePlan2DResult plan;
	plan.walls.push_back({ 0, Symbol("draft:wall", iggy::DraftSymbol2DKind::Wall, { 1.0F, 2.0F }, { 3.0F, 4.0F }) });
	plan.walls.push_back({ 1, Symbol("draft:invalid", iggy::DraftSymbol2DKind::Wall, { 5.0F, 6.0F }, { -1.0F, 4.0F }) });
	const std::vector<iggy::DraftWallPlan2D> beforeWalls = plan.walls;

	const iggy::DraftWallCompile2DResult result = iggy::DraftWallCompiler2D {}.compile(plan);

	Expect(result.walls.size() == 1 && result.issues.size() == 1, "draft wall compile immutability setup should exercise success and issue paths");
	Expect(SameWallPlans(plan.walls, beforeWalls), "draft wall compiler should not mutate input plan");
}

} // namespace

int main()
{
	TestEmptyPlanProducesEmptyCompileResult();
	TestValidWallCompilesPreservingPayload();
	TestMultipleWallsPreserveOrder();
	TestNonWallPlanCategoriesAreIgnored();
	TestNonPositiveSizesProduceIssues();
	TestMixedValidAndInvalidWallsPreserveOutputsAndIssues();
	TestInputPlanIsNotMutated();

	return Failures;
}
