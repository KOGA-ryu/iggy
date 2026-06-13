#include <cstdlib>
#include <vector>

#include "scene/draft/DraftDocument2D.hpp"
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
	iggy::DraftSymbol2DKind kind = iggy::DraftSymbol2DKind::Wall,
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

void TestEmptyDocumentBuilds()
{
	const iggy::DraftDocument2DBuildResult result = iggy::DraftDocument2DBuilder {}.build({});

	Expect(result.built, "empty draft document should build");
	Expect(result.issues.empty(), "empty draft document should have no issues");
	Expect(result.document.symbols.empty(), "empty draft document should publish no symbols");
	Expect(!result.document.contains(Id("draft:missing")), "empty draft document should not contain missing id");
	Expect(result.document.find(Id("draft:missing")) == nullptr, "empty draft document find should return null");
}

void TestSuccessfulBuildPreservesOrderAndFields()
{
	const std::vector<iggy::DraftSymbol2D> symbols {
		Symbol("draft:wall", iggy::DraftSymbol2DKind::Wall, { 1.0F, 2.0F }, { 3.0F, 4.0F }, 0.25F, "asset:wall", "definition:wall"),
		Symbol("draft:door", iggy::DraftSymbol2DKind::Door, { 5.0F, 6.0F }, { 1.0F, 2.0F }, 1.5F, "asset:door", "definition:door"),
		Symbol("draft:object", iggy::DraftSymbol2DKind::Object, { -1.0F, 3.0F }, { 0.5F, 0.75F }, 0.0F, "asset:object", "definition:object"),
		Symbol("draft:furniture", iggy::DraftSymbol2DKind::Furniture, { 4.0F, -2.0F }, { 2.0F, 2.0F }, 0.75F, "asset:furniture", "definition:furniture"),
		Symbol("draft:item", iggy::DraftSymbol2DKind::ItemDrop, { 7.0F, 8.0F }, { 1.0F, 1.0F }, 0.0F, "asset:item", "definition:item"),
		Symbol("draft:npc", iggy::DraftSymbol2DKind::Npc, { 9.0F, 1.0F }, { 1.0F, 2.0F }, 0.0F, "asset:npc", "definition:npc"),
		Symbol("draft:region", iggy::DraftSymbol2DKind::Region, { 2.0F, 9.0F }, { 5.0F, 6.0F }, 0.0F, "asset:region", "definition:region"),
		Symbol("draft:story", iggy::DraftSymbol2DKind::StoryMarker, { 3.0F, 9.0F }, { 0.0F, 0.0F }, 0.0F, "asset:story", "definition:story"),
	};

	const iggy::DraftDocument2DBuildResult result = iggy::DraftDocument2DBuilder {}.build(symbols);

	Expect(result.built, "valid draft document should build");
	Expect(result.issues.empty(), "valid draft document should have no issues");
	Expect(result.document.symbols.size() == symbols.size(), "valid draft document should preserve symbol count");
	for (std::size_t index = 0; index < result.document.symbols.size() && index < symbols.size(); ++index)
		ExpectSymbol(result.document.symbols[index], symbols[index], "valid draft document should preserve symbol fields");
}

void TestFindAndContainsUseExactSymbolIds()
{
	const std::vector<iggy::DraftSymbol2D> symbols {
		Symbol("draft:door", iggy::DraftSymbol2DKind::Door, { 1.0F, 0.0F }),
		Symbol("draft:wall", iggy::DraftSymbol2DKind::Wall, { 2.0F, 0.0F }),
	};
	const iggy::DraftDocument2DBuildResult result = iggy::DraftDocument2DBuilder {}.build(symbols);

	Expect(result.built, "draft lookup setup should build");
	Expect(result.document.contains(Id("draft:door")), "draft document should contain exact symbol id");
	Expect(!result.document.contains(Id("draft:missing")), "draft document should not contain missing symbol id");
	const iggy::DraftSymbol2D *symbol = result.document.find(Id("draft:wall"));
	Expect(symbol != nullptr, "draft document should find exact symbol id");
	if (symbol != nullptr)
		ExpectSymbol(*symbol, symbols[1], "draft document find should return matching symbol payload");
	Expect(result.document.find(Id("draft:missing")) == nullptr, "draft document find should return null for missing id");
}

void TestEmptySymbolIdFails()
{
	const iggy::DraftSymbol2D symbol = Symbol("", iggy::DraftSymbol2DKind::Wall);

	const iggy::DraftDocument2DBuildResult result = iggy::DraftDocument2DBuilder {}.build({ symbol });

	Expect(!result.built, "empty draft symbol id should fail build");
	Expect(result.document.symbols.empty(), "failed empty-id draft build should publish no symbols");
	Expect(result.issues.size() == 1, "empty draft symbol id should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::DraftDocument2DIssueCode::EmptySymbolId, "empty draft symbol id should use EmptySymbolId issue");
		Expect(result.issues[0].symbolIndex == 0, "empty draft symbol id issue should preserve index");
		ExpectSymbol(result.issues[0].symbol, symbol, "empty draft symbol id issue should preserve payload");
	}
}

void TestDuplicateSymbolIdFailsForLaterEntry()
{
	const std::vector<iggy::DraftSymbol2D> symbols {
		Symbol("draft:door", iggy::DraftSymbol2DKind::Door),
		Symbol("draft:door", iggy::DraftSymbol2DKind::Object, { 2.0F, 0.0F }),
	};

	const iggy::DraftDocument2DBuildResult result = iggy::DraftDocument2DBuilder {}.build(symbols);

	Expect(!result.built, "duplicate draft symbol id should fail build");
	Expect(result.document.symbols.empty(), "failed duplicate draft build should publish no symbols");
	Expect(result.issues.size() == 1, "duplicate draft symbol id should report later duplicate only");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::DraftDocument2DIssueCode::DuplicateSymbolId, "duplicate draft issue should use DuplicateSymbolId");
		Expect(result.issues[0].symbolIndex == 1, "duplicate draft issue should preserve later index");
		ExpectSymbol(result.issues[0].symbol, symbols[1], "duplicate draft issue should preserve later payload");
	}
}

void TestMultipleIssuesAreReportedInInputOrder()
{
	const std::vector<iggy::DraftSymbol2D> symbols {
		Symbol("draft:door", iggy::DraftSymbol2DKind::Door),
		Symbol("", iggy::DraftSymbol2DKind::Wall),
		Symbol("draft:door", iggy::DraftSymbol2DKind::Object),
		Symbol("draft:door", iggy::DraftSymbol2DKind::Npc),
	};

	const iggy::DraftDocument2DBuildResult result = iggy::DraftDocument2DBuilder {}.build(symbols);

	Expect(!result.built, "multi-issue draft document should fail build");
	Expect(result.document.symbols.empty(), "failed multi-issue draft build should publish no symbols");
	Expect(result.issues.size() == 3, "multi-issue draft document should report all id issues");
	if (result.issues.size() == 3) {
		Expect(result.issues[0].code == iggy::DraftDocument2DIssueCode::EmptySymbolId && result.issues[0].symbolIndex == 1, "empty id should be first draft issue");
		Expect(result.issues[1].code == iggy::DraftDocument2DIssueCode::DuplicateSymbolId && result.issues[1].symbolIndex == 2, "first duplicate should be second draft issue");
		Expect(result.issues[2].code == iggy::DraftDocument2DIssueCode::DuplicateSymbolId && result.issues[2].symbolIndex == 3, "second duplicate should be third draft issue");
	}
}

void TestUnknownKindDisabledAndEmptyReferencesAreValid()
{
	const std::vector<iggy::DraftSymbol2D> symbols {
		Symbol("draft:unknown", iggy::DraftSymbol2DKind::Unknown, { 1.0F, 1.0F }, { 0.0F, 0.0F }, 0.0F, "", "", false),
	};

	const iggy::DraftDocument2DBuildResult result = iggy::DraftDocument2DBuilder {}.build(symbols);

	Expect(result.built, "unknown draft kind and empty references should build");
	Expect(result.issues.empty(), "unknown draft kind and empty references should have no issues");
	Expect(result.document.symbols.size() == 1, "unknown draft kind setup should publish one symbol");
	if (result.document.symbols.size() == 1)
		ExpectSymbol(result.document.symbols[0], symbols[0], "unknown draft kind, disabled state, and empty references should be preserved");
}

void TestNamespacedAndUnqualifiedIdsAreDistinct()
{
	const std::vector<iggy::DraftSymbol2D> symbols {
		Symbol("door", iggy::DraftSymbol2DKind::Door),
		Symbol("draft:door", iggy::DraftSymbol2DKind::Door),
	};

	const iggy::DraftDocument2DBuildResult result = iggy::DraftDocument2DBuilder {}.build(symbols);

	Expect(result.built, "namespaced and unqualified draft ids should build as distinct");
	Expect(result.issues.empty(), "namespaced and unqualified draft ids should not report duplicates");
	Expect(result.document.contains(Id("door")), "draft document should contain unqualified id");
	Expect(result.document.contains(Id("draft:door")), "draft document should contain namespaced id");
	const iggy::DraftSymbol2D *unqualified = result.document.find(Id("door"));
	const iggy::DraftSymbol2D *namespaced = result.document.find(Id("draft:door"));
	Expect(unqualified != nullptr && namespaced != nullptr, "draft document should find both exact ids");
	if (unqualified != nullptr && namespaced != nullptr) {
		Expect(unqualified->id == Id("door"), "draft document should preserve unqualified id");
		Expect(namespaced->id == Id("draft:door"), "draft document should preserve namespaced id");
	}
}

void TestInputVectorIsNotMutated()
{
	const std::vector<iggy::DraftSymbol2D> symbols {
		Symbol("draft:wall", iggy::DraftSymbol2DKind::Wall, { 1.0F, 2.0F }, { 3.0F, 4.0F }, 0.5F, "asset:wall", "definition:wall"),
		Symbol("draft:door", iggy::DraftSymbol2DKind::Door, { 5.0F, 6.0F }, { 1.0F, 1.0F }, 1.25F, "", "", false),
	};
	const std::vector<iggy::DraftSymbol2D> before = symbols;

	const iggy::DraftDocument2DBuildResult result = iggy::DraftDocument2DBuilder {}.build(symbols);

	Expect(result.built, "draft immutability setup should build");
	Expect(SameSymbols(symbols, before), "draft builder should not mutate input vector");
}

} // namespace

int main()
{
	TestEmptyDocumentBuilds();
	TestSuccessfulBuildPreservesOrderAndFields();
	TestFindAndContainsUseExactSymbolIds();
	TestEmptySymbolIdFails();
	TestDuplicateSymbolIdFailsForLaterEntry();
	TestMultipleIssuesAreReportedInInputOrder();
	TestUnknownKindDisabledAndEmptyReferencesAreValid();
	TestNamespacedAndUnqualifiedIdsAreDistinct();
	TestInputVectorIsNotMutated();

	return Failures;
}
