#include <cstdlib>
#include <vector>

#include "scene/interaction/InteractionEffectCatalog2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::InteractionEffectEntry2D Entry(const char *targetId, std::vector<iggy::InteractionEffect2D> effects)
{
	return { iggy::ResourceId { targetId }, effects };
}

bool SameEffect(const iggy::InteractionEffect2D &actual, const iggy::InteractionEffect2D &expected)
{
	return actual.type == expected.type
		&& actual.targetId == expected.targetId
		&& actual.eventId == expected.eventId
		&& actual.text == expected.text
		&& actual.enabledValue == expected.enabledValue;
}

bool SameEntry(const iggy::InteractionEffectEntry2D &actual, const iggy::InteractionEffectEntry2D &expected)
{
	if (actual.targetId != expected.targetId || actual.effects.size() != expected.effects.size())
		return false;
	for (std::size_t index = 0; index < actual.effects.size(); ++index) {
		if (!SameEffect(actual.effects[index], expected.effects[index]))
			return false;
	}
	return true;
}

void ExpectEntry(const iggy::InteractionEffectEntry2D &actual, const iggy::InteractionEffectEntry2D &expected, const char *message)
{
	Expect(SameEntry(actual, expected), message);
}

void TestEmptyInputBuildsValidEmptyCatalog()
{
	const iggy::InteractionEffectCatalog2DBuildResult result = iggy::InteractionEffectCatalog2DBuilder {}.build({});

	Expect(result.built, "empty effect catalog input should build");
	Expect(result.issues.empty(), "empty effect catalog input should have no issues");
	Expect(result.catalog.entries().empty(), "empty effect catalog input should produce empty catalog");
	Expect(!result.catalog.contains(iggy::ResourceId { "target:missing" }), "empty effect catalog should not contain missing id");
	Expect(result.catalog.find(iggy::ResourceId { "target:missing" }) == nullptr, "empty effect catalog should return null for missing id");
}

void TestSuccessfulBuildPreservesEntryAndEffectOrder()
{
	const std::vector<iggy::InteractionEffectEntry2D> entries {
		Entry("target:sign", {
			iggy::inspectTextInteractionEffect(iggy::ResourceId { "target:sign" }, "Read"),
			iggy::emitInteractionEventEffect(iggy::ResourceId { "target:sign" }, iggy::ResourceId { "event:sign_read" }),
		}),
		Entry("target:door", {
			iggy::toggleTargetInteractionEffect(iggy::ResourceId { "target:door" }, false),
			iggy::noneInteractionEffect(),
		}),
	};

	const iggy::InteractionEffectCatalog2DBuildResult result = iggy::InteractionEffectCatalog2DBuilder {}.build(entries);

	Expect(result.built, "valid effect catalog should build");
	Expect(result.issues.empty(), "valid effect catalog should have no issues");
	Expect(result.catalog.entries().size() == entries.size(), "valid effect catalog should preserve entry count");
	if (result.catalog.entries().size() == entries.size()) {
		ExpectEntry(result.catalog.entries()[0], entries[0], "first effect catalog entry should preserve target and effects");
		ExpectEntry(result.catalog.entries()[1], entries[1], "second effect catalog entry should preserve target and effects");
	}
}

void TestFindAndContainsUseExactTargetIds()
{
	const std::vector<iggy::InteractionEffectEntry2D> entries {
		Entry("target:lever", {
			iggy::emitInteractionEventEffect({}, iggy::ResourceId { "event:lever" }),
		}),
		Entry("target:npc", {
			iggy::inspectTextInteractionEffect({}, "Hello"),
		}),
	};
	const iggy::InteractionEffectCatalog2DBuildResult result = iggy::InteractionEffectCatalog2DBuilder {}.build(entries);

	Expect(result.built, "effect catalog lookup setup should build");
	Expect(result.catalog.contains(iggy::ResourceId { "target:lever" }), "effect catalog should contain exact target id");
	Expect(!result.catalog.contains(iggy::ResourceId { "target:missing" }), "effect catalog should not contain missing target id");
	const std::vector<iggy::InteractionEffect2D> *effects = result.catalog.find(iggy::ResourceId { "target:npc" });
	Expect(effects != nullptr, "effect catalog should find exact target id");
	if (effects != nullptr) {
		Expect(effects->size() == 1, "effect catalog lookup should return matching effect list");
		if (effects->size() == 1)
			Expect(SameEffect((*effects)[0], entries[1].effects[0]), "effect catalog lookup should preserve effect payload");
	}
	Expect(result.catalog.find(iggy::ResourceId { "target:missing" }) == nullptr, "effect catalog should return null for missing target id");
}

void TestEmptyTargetIdFails()
{
	const iggy::InteractionEffectEntry2D entry = Entry("", {
		iggy::inspectTextInteractionEffect({}, "Text"),
	});

	const iggy::InteractionEffectCatalog2DBuildResult result = iggy::InteractionEffectCatalog2DBuilder {}.build({ entry });

	Expect(!result.built, "empty effect catalog target id should fail build");
	Expect(result.catalog.entries().empty(), "failed empty-target build should not publish catalog entries");
	Expect(result.issues.size() == 1, "empty effect catalog target id should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::InteractionEffectCatalog2DIssueCode::MissingTargetId, "empty target issue should use MissingTargetId");
		Expect(result.issues[0].entryIndex == 0, "empty target issue should preserve entry index");
		Expect(result.issues[0].effectIndex == 0, "empty target issue should use zero effect index");
		ExpectEntry(result.issues[0].entry, entry, "empty target issue should preserve entry payload");
	}
}

void TestEmptyEffectsFails()
{
	const iggy::InteractionEffectEntry2D entry = Entry("target:empty", {});

	const iggy::InteractionEffectCatalog2DBuildResult result = iggy::InteractionEffectCatalog2DBuilder {}.build({ entry });

	Expect(!result.built, "empty effect list should fail build");
	Expect(result.catalog.entries().empty(), "failed empty-effects build should not publish catalog entries");
	Expect(result.issues.size() == 1, "empty effect list should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::InteractionEffectCatalog2DIssueCode::EmptyEffects, "empty effects issue should use EmptyEffects");
		Expect(result.issues[0].entryIndex == 0, "empty effects issue should preserve entry index");
		Expect(result.issues[0].effectIndex == 0, "empty effects issue should use zero effect index");
		ExpectEntry(result.issues[0].entry, entry, "empty effects issue should preserve entry payload");
	}
}

void TestInvalidEffectsFailWithStatus()
{
	const std::vector<iggy::InteractionEffectEntry2D> entries {
		Entry("target:bad_text", {
			iggy::inspectTextInteractionEffect({}, ""),
		}),
		Entry("target:bad_target", {
			iggy::toggleTargetInteractionEffect({}, true),
		}),
		Entry("target:bad_event", {
			iggy::emitInteractionEventEffect({}, {}),
		}),
	};

	const iggy::InteractionEffectCatalog2DBuildResult result = iggy::InteractionEffectCatalog2DBuilder {}.build(entries);

	Expect(!result.built, "invalid effect catalog effects should fail build");
	Expect(result.catalog.entries().empty(), "failed invalid-effect build should not publish catalog entries");
	Expect(result.issues.size() == 3, "invalid effect catalog effects should each report an issue");
	if (result.issues.size() == 3) {
		Expect(result.issues[0].code == iggy::InteractionEffectCatalog2DIssueCode::InvalidEffect && result.issues[0].entryIndex == 0 && result.issues[0].effectIndex == 0, "missing text issue should preserve entry and effect indexes");
		Expect(result.issues[0].effectStatus == iggy::InteractionEffect2DStatus::MissingText, "missing text issue should preserve effect status");
		Expect(result.issues[1].code == iggy::InteractionEffectCatalog2DIssueCode::InvalidEffect && result.issues[1].entryIndex == 1 && result.issues[1].effectIndex == 0, "missing target issue should preserve entry and effect indexes");
		Expect(result.issues[1].effectStatus == iggy::InteractionEffect2DStatus::MissingTarget, "missing target issue should preserve effect status");
		Expect(result.issues[2].code == iggy::InteractionEffectCatalog2DIssueCode::InvalidEffect && result.issues[2].entryIndex == 2 && result.issues[2].effectIndex == 0, "missing event issue should preserve entry and effect indexes");
		Expect(result.issues[2].effectStatus == iggy::InteractionEffect2DStatus::MissingEvent, "missing event issue should preserve effect status");
	}
}

void TestMultipleIssuesAreReportedDeterministically()
{
	const std::vector<iggy::InteractionEffectEntry2D> entries {
		Entry("", {}),
		Entry("target:mixed", {
			iggy::noneInteractionEffect(),
			iggy::inspectTextInteractionEffect({}, ""),
			iggy::toggleTargetInteractionEffect({}, true),
		}),
		Entry("target:event", {
			iggy::emitInteractionEventEffect({}, {}),
		}),
	};

	const iggy::InteractionEffectCatalog2DBuildResult result = iggy::InteractionEffectCatalog2DBuilder {}.build(entries);

	Expect(!result.built, "multi-issue effect catalog should fail build");
	Expect(result.catalog.entries().empty(), "failed multi-issue effect catalog should not publish entries");
	Expect(result.issues.size() == 5, "multi-issue effect catalog should report all issues");
	if (result.issues.size() == 5) {
		Expect(result.issues[0].code == iggy::InteractionEffectCatalog2DIssueCode::MissingTargetId && result.issues[0].entryIndex == 0, "missing target id should be first issue for first entry");
		Expect(result.issues[1].code == iggy::InteractionEffectCatalog2DIssueCode::EmptyEffects && result.issues[1].entryIndex == 0, "empty effects should follow missing target id for first entry");
		Expect(result.issues[2].code == iggy::InteractionEffectCatalog2DIssueCode::InvalidEffect && result.issues[2].entryIndex == 1 && result.issues[2].effectIndex == 1, "missing text should be reported at second entry second effect");
		Expect(result.issues[2].effectStatus == iggy::InteractionEffect2DStatus::MissingText, "missing text issue should preserve status");
		Expect(result.issues[3].code == iggy::InteractionEffectCatalog2DIssueCode::InvalidEffect && result.issues[3].entryIndex == 1 && result.issues[3].effectIndex == 2, "missing target should be reported at second entry third effect");
		Expect(result.issues[3].effectStatus == iggy::InteractionEffect2DStatus::MissingTarget, "missing target issue should preserve status");
		Expect(result.issues[4].code == iggy::InteractionEffectCatalog2DIssueCode::InvalidEffect && result.issues[4].entryIndex == 2 && result.issues[4].effectIndex == 0, "missing event should be reported at third entry first effect");
		Expect(result.issues[4].effectStatus == iggy::InteractionEffect2DStatus::MissingEvent, "missing event issue should preserve status");
	}
}

void TestDuplicateTargetIdsAreAllowedAndFindReturnsFirst()
{
	const iggy::InteractionEffectEntry2D first = Entry("target:duplicate", {
		iggy::inspectTextInteractionEffect({}, "First"),
	});
	const iggy::InteractionEffectEntry2D second = Entry("target:duplicate", {
		iggy::inspectTextInteractionEffect({}, "Second"),
	});
	const iggy::InteractionEffectCatalog2DBuildResult result = iggy::InteractionEffectCatalog2DBuilder {}.build({ first, second });

	Expect(result.built, "duplicate effect catalog target ids should be allowed");
	Expect(result.issues.empty(), "duplicate effect catalog target ids should not produce issues");
	Expect(result.catalog.entries().size() == 2, "duplicate effect catalog entries should be preserved");
	const std::vector<iggy::InteractionEffect2D> *effects = result.catalog.find(iggy::ResourceId { "target:duplicate" });
	Expect(effects != nullptr, "duplicate effect catalog id should be found");
	if (effects != nullptr) {
		Expect(effects->size() == first.effects.size(), "duplicate effect catalog find should return first effect list");
		if (effects->size() == first.effects.size())
			Expect(SameEffect((*effects)[0], first.effects[0]), "duplicate effect catalog find should return first matching entry");
	}
}

void TestNamespacedAndUnqualifiedIdsAreDistinct()
{
	const iggy::InteractionEffectEntry2D unqualified = Entry("door", {
		iggy::inspectTextInteractionEffect({}, "Door"),
	});
	const iggy::InteractionEffectEntry2D namespaced = Entry("target:door", {
		iggy::inspectTextInteractionEffect({}, "Target door"),
	});
	const iggy::InteractionEffectCatalog2DBuildResult result = iggy::InteractionEffectCatalog2DBuilder {}.build({ unqualified, namespaced });

	Expect(result.built, "namespaced and unqualified effect catalog ids should build");
	Expect(result.catalog.contains(iggy::ResourceId { "door" }), "effect catalog should contain unqualified id");
	Expect(result.catalog.contains(iggy::ResourceId { "target:door" }), "effect catalog should contain namespaced id");
	const std::vector<iggy::InteractionEffect2D> *door = result.catalog.find(iggy::ResourceId { "door" });
	const std::vector<iggy::InteractionEffect2D> *targetDoor = result.catalog.find(iggy::ResourceId { "target:door" });
	Expect(door != nullptr && targetDoor != nullptr && door != targetDoor, "namespaced and unqualified ids should resolve to distinct effect lists");
	if (door != nullptr && targetDoor != nullptr) {
		Expect((*door)[0].text == "Door", "unqualified id should resolve unqualified effect");
		Expect((*targetDoor)[0].text == "Target door", "namespaced id should resolve namespaced effect");
	}
}

} // namespace

int main()
{
	TestEmptyInputBuildsValidEmptyCatalog();
	TestSuccessfulBuildPreservesEntryAndEffectOrder();
	TestFindAndContainsUseExactTargetIds();
	TestEmptyTargetIdFails();
	TestEmptyEffectsFails();
	TestInvalidEffectsFailWithStatus();
	TestMultipleIssuesAreReportedDeterministically();
	TestDuplicateTargetIdsAreAllowedAndFindReturnsFirst();
	TestNamespacedAndUnqualifiedIdsAreDistinct();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
