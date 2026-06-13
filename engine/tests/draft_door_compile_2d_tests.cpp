#include <cstdlib>
#include <vector>

#include "scene/draft/DraftDoorCompile2D.hpp"
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

void ExpectCompiledDoor(const iggy::DraftCompiledDoor2D &actual, const iggy::DraftDoorPlan2D &expected, const char *message)
{
	Expect(actual.sourceSymbolId == expected.symbol.id, message);
	Expect(actual.sourceSymbolIndex == expected.symbolIndex, message);
	Expect(NearVec(actual.center, expected.symbol.position), message);
	Expect(NearVec(actual.size, expected.symbol.size), message);
	Expect(actual.rotationRadians == expected.symbol.rotationRadians, message);
	Expect(actual.assetId == expected.symbol.assetId, message);
	Expect(actual.definitionId == expected.symbol.definitionId, message);
	Expect(actual.swing == iggy::DraftDoorSwing2D::Unknown, message);
}

bool SameDoorPlans(const std::vector<iggy::DraftDoorPlan2D> &actual, const std::vector<iggy::DraftDoorPlan2D> &expected)
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
	const iggy::DraftDoorCompile2DResult result = iggy::DraftDoorCompiler2D {}.compile({});

	Expect(result.doors.empty(), "empty draft door compile should have no doors");
	Expect(result.issues.empty(), "empty draft door compile should have no issues");
	Expect(!result.hasIssues(), "empty draft door compile should report no issues");
}

void TestValidDoorCompilesPreservingPayload()
{
	const iggy::DraftDoorPlan2D door {
		7,
		Symbol("draft:door", iggy::DraftSymbol2DKind::Door, { 1.0F, 2.0F }, { 3.0F, 4.0F }, 0.5F, "asset:door", "definition:door"),
	};
	iggy::DraftCompilePlan2DResult plan;
	plan.doors.push_back(door);

	const iggy::DraftDoorCompile2DResult result = iggy::DraftDoorCompiler2D {}.compile(plan);

	Expect(result.doors.size() == 1, "valid draft door should compile");
	Expect(result.issues.empty(), "valid draft door should not produce issues");
	if (result.doors.size() == 1)
		ExpectCompiledDoor(result.doors[0], door, "compiled draft door should preserve source payload");
}

void TestMultipleDoorsPreserveOrder()
{
	iggy::DraftCompilePlan2DResult plan;
	plan.doors.push_back({ 3, Symbol("draft:door_a", iggy::DraftSymbol2DKind::Door) });
	plan.doors.push_back({ 1, Symbol("draft:door_b", iggy::DraftSymbol2DKind::Door, { 5.0F, 6.0F }, { 2.0F, 3.0F }) });
	plan.doors.push_back({ 9, Symbol("draft:door_c", iggy::DraftSymbol2DKind::Door, { 7.0F, 8.0F }, { 4.0F, 5.0F }) });

	const iggy::DraftDoorCompile2DResult result = iggy::DraftDoorCompiler2D {}.compile(plan);

	Expect(result.doors.size() == 3, "multiple draft doors should compile");
	if (result.doors.size() == 3) {
		Expect(result.doors[0].sourceSymbolId == Id("draft:door_a"), "first compiled door should preserve plan order");
		Expect(result.doors[1].sourceSymbolId == Id("draft:door_b"), "second compiled door should preserve plan order");
		Expect(result.doors[2].sourceSymbolId == Id("draft:door_c"), "third compiled door should preserve plan order");
	}
}

void TestNonDoorPlanCategoriesAreIgnored()
{
	iggy::DraftCompilePlan2DResult plan;
	plan.walls.push_back({ 0, Symbol("draft:wall", iggy::DraftSymbol2DKind::Wall) });
	plan.objects.push_back({ 1, Symbol("draft:object", iggy::DraftSymbol2DKind::Object) });
	plan.itemDrops.push_back({ 2, Symbol("draft:item", iggy::DraftSymbol2DKind::ItemDrop) });
	plan.npcs.push_back({ 3, Symbol("draft:npc", iggy::DraftSymbol2DKind::Npc) });
	plan.markers.push_back({ 4, Symbol("draft:marker", iggy::DraftSymbol2DKind::Region) });
	plan.ignoredDisabledSymbols.push_back({ 5, Symbol("draft:disabled", iggy::DraftSymbol2DKind::Door, { 0.0F, 0.0F }, { 0.0F, 0.0F }, 0.0F, "", "", false) });
	plan.issues.push_back({ iggy::DraftCompilePlan2DIssueCode::UnknownSymbolKind, 6, Symbol("draft:unknown", iggy::DraftSymbol2DKind::Unknown) });

	const iggy::DraftDoorCompile2DResult result = iggy::DraftDoorCompiler2D {}.compile(plan);

	Expect(result.doors.empty(), "draft door compiler should ignore non-door categories");
	Expect(result.issues.empty(), "draft door compiler should not copy non-door plan issues");
}

void TestNonPositiveSizesProduceIssues()
{
	iggy::DraftCompilePlan2DResult plan;
	plan.doors.push_back({ 0, Symbol("draft:zero_width", iggy::DraftSymbol2DKind::Door, { 0.0F, 0.0F }, { 0.0F, 1.0F }) });
	plan.doors.push_back({ 1, Symbol("draft:negative_width", iggy::DraftSymbol2DKind::Door, { 0.0F, 0.0F }, { -1.0F, 1.0F }) });
	plan.doors.push_back({ 2, Symbol("draft:zero_height", iggy::DraftSymbol2DKind::Door, { 0.0F, 0.0F }, { 1.0F, 0.0F }) });
	plan.doors.push_back({ 3, Symbol("draft:negative_height", iggy::DraftSymbol2DKind::Door, { 0.0F, 0.0F }, { 1.0F, -1.0F }) });

	const iggy::DraftDoorCompile2DResult result = iggy::DraftDoorCompiler2D {}.compile(plan);

	Expect(result.doors.empty(), "non-positive draft door sizes should not compile");
	Expect(result.issues.size() == 4, "non-positive draft door sizes should produce issues");
	Expect(result.hasIssues(), "non-positive draft door sizes should report issues");
	if (result.issues.size() == 4) {
		for (std::size_t index = 0; index < result.issues.size(); ++index) {
			Expect(result.issues[index].code == iggy::DraftDoorCompile2DIssueCode::NonPositiveSize, "draft door size issue should use NonPositiveSize");
			Expect(result.issues[index].symbolIndex == index, "draft door size issues should preserve door order");
			ExpectSymbol(result.issues[index].symbol, plan.doors[index].symbol, "draft door size issue should copy door payload");
		}
	}
}

void TestMixedValidAndInvalidDoorsPreserveOutputsAndIssues()
{
	iggy::DraftCompilePlan2DResult plan;
	plan.doors.push_back({ 0, Symbol("draft:door_a", iggy::DraftSymbol2DKind::Door, { 1.0F, 0.0F }, { 1.0F, 2.0F }) });
	plan.doors.push_back({ 1, Symbol("draft:invalid", iggy::DraftSymbol2DKind::Door, { 2.0F, 0.0F }, { 0.0F, 2.0F }) });
	plan.doors.push_back({ 2, Symbol("draft:door_b", iggy::DraftSymbol2DKind::Door, { 3.0F, 0.0F }, { 3.0F, 4.0F }) });

	const iggy::DraftDoorCompile2DResult result = iggy::DraftDoorCompiler2D {}.compile(plan);

	Expect(result.doors.size() == 2, "mixed draft door compile should keep valid doors");
	Expect(result.issues.size() == 1, "mixed draft door compile should report invalid door");
	if (result.doors.size() == 2) {
		Expect(result.doors[0].sourceSymbolId == Id("draft:door_a"), "first valid draft door should compile first");
		Expect(result.doors[1].sourceSymbolId == Id("draft:door_b"), "second valid draft door should compile second");
	}
	if (result.issues.size() == 1) {
		Expect(result.issues[0].symbolIndex == 1, "mixed draft door compile issue should preserve invalid source index");
		ExpectSymbol(result.issues[0].symbol, plan.doors[1].symbol, "mixed draft door compile issue should copy invalid door payload");
	}
}

void TestDefaultSwingIsUnknown()
{
	iggy::DraftCompilePlan2DResult plan;
	plan.doors.push_back({ 0, Symbol("draft:door", iggy::DraftSymbol2DKind::Door, { 1.0F, 2.0F }, { 3.0F, 4.0F }, 0.25F, "asset:door", "definition:door") });

	const iggy::DraftDoorCompile2DResult result = iggy::DraftDoorCompiler2D {}.compile(plan);

	Expect(result.doors.size() == 1, "default swing setup should compile one door");
	if (result.doors.size() == 1)
		Expect(result.doors[0].swing == iggy::DraftDoorSwing2D::Unknown, "compiled draft door swing should default to Unknown");
}

void TestInputPlanIsNotMutated()
{
	iggy::DraftCompilePlan2DResult plan;
	plan.doors.push_back({ 0, Symbol("draft:door", iggy::DraftSymbol2DKind::Door, { 1.0F, 2.0F }, { 3.0F, 4.0F }) });
	plan.doors.push_back({ 1, Symbol("draft:invalid", iggy::DraftSymbol2DKind::Door, { 5.0F, 6.0F }, { -1.0F, 4.0F }) });
	const std::vector<iggy::DraftDoorPlan2D> beforeDoors = plan.doors;

	const iggy::DraftDoorCompile2DResult result = iggy::DraftDoorCompiler2D {}.compile(plan);

	Expect(result.doors.size() == 1 && result.issues.size() == 1, "draft door compile immutability setup should exercise success and issue paths");
	Expect(SameDoorPlans(plan.doors, beforeDoors), "draft door compiler should not mutate input plan");
}

} // namespace

int main()
{
	TestEmptyPlanProducesEmptyCompileResult();
	TestValidDoorCompilesPreservingPayload();
	TestMultipleDoorsPreserveOrder();
	TestNonDoorPlanCategoriesAreIgnored();
	TestNonPositiveSizesProduceIssues();
	TestMixedValidAndInvalidDoorsPreserveOutputsAndIssues();
	TestDefaultSwingIsUnknown();
	TestInputPlanIsNotMutated();

	return Failures;
}
