#include <cstdlib>
#include <vector>

#include "scene/interaction/InteractionTarget2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::InteractionTarget2D Target(
	const char *id,
	iggy::InteractionTarget2DKind kind = iggy::InteractionTarget2DKind::Inspectable,
	iggy::Vec2 position = { 0.0F, 0.0F },
	float radius = 0.0F,
	bool enabled = true)
{
	return { iggy::ResourceId { id }, kind, position, radius, enabled };
}

void ExpectTarget(const iggy::InteractionTarget2D &target, const iggy::InteractionTarget2D &expected, const char *message)
{
	Expect(target.id == expected.id, message);
	Expect(target.kind == expected.kind, message);
	Expect(NearVec(target.position, expected.position), message);
	Expect(target.radius == expected.radius, message);
	Expect(target.enabled == expected.enabled, message);
}

void TestEmptyInputBuildsValidEmptyRegistry()
{
	const iggy::InteractionTarget2DRegistryBuildResult result = iggy::InteractionTarget2DRegistryBuilder {}.build({});

	Expect(result.built, "empty interaction target input should build");
	Expect(result.issues.empty(), "empty interaction target input should have no issues");
	Expect(result.registry.targets().empty(), "empty interaction target input should produce empty registry");
	Expect(!result.registry.contains(iggy::ResourceId { "target:missing" }), "empty interaction registry should not contain missing id");
	Expect(result.registry.find(iggy::ResourceId { "target:missing" }) == nullptr, "empty interaction registry should return null for missing id");
}

void TestSuccessfulBuildPreservesOrderAndFields()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:sign", iggy::InteractionTarget2DKind::Inspectable, { 1.0F, 2.0F }, 0.0F, true),
		Target("target:door", iggy::InteractionTarget2DKind::Door, { -3.0F, 4.5F }, 1.5F, false),
		Target("target:coin", iggy::InteractionTarget2DKind::Pickup, { 0.0F, -1.0F }, 0.25F, true),
	};

	const iggy::InteractionTarget2DRegistryBuildResult result = iggy::InteractionTarget2DRegistryBuilder {}.build(targets);

	Expect(result.built, "valid interaction targets should build");
	Expect(result.issues.empty(), "valid interaction targets should have no issues");
	Expect(result.registry.targets().size() == targets.size(), "valid interaction registry should preserve target count");
	if (result.registry.targets().size() == targets.size()) {
		ExpectTarget(result.registry.targets()[0], targets[0], "first interaction target should preserve fields");
		ExpectTarget(result.registry.targets()[1], targets[1], "second interaction target should preserve fields");
		ExpectTarget(result.registry.targets()[2], targets[2], "third interaction target should preserve fields");
	}
}

void TestFindAndContainsUseExactIds()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:lever", iggy::InteractionTarget2DKind::Usable),
		Target("target:npc", iggy::InteractionTarget2DKind::Talk),
	};
	const iggy::InteractionTarget2DRegistryBuildResult result = iggy::InteractionTarget2DRegistryBuilder {}.build(targets);

	Expect(result.built, "lookup setup should build valid registry");
	Expect(result.registry.contains(iggy::ResourceId { "target:lever" }), "registry should contain exact target id");
	Expect(!result.registry.contains(iggy::ResourceId { "target:missing" }), "registry should not contain missing target id");
	const iggy::InteractionTarget2D *target = result.registry.find(iggy::ResourceId { "target:npc" });
	Expect(target != nullptr, "registry should find exact target id");
	if (target != nullptr)
		Expect(target->kind == iggy::InteractionTarget2DKind::Talk, "registry find should return matching target payload");
	Expect(result.registry.find(iggy::ResourceId { "target:missing" }) == nullptr, "registry find should return null for missing id");
}

void TestEmptyIdFails()
{
	const iggy::InteractionTarget2D target = Target("", iggy::InteractionTarget2DKind::Usable, { 2.0F, 3.0F }, 1.0F);

	const iggy::InteractionTarget2DRegistryBuildResult result = iggy::InteractionTarget2DRegistryBuilder {}.build({ target });

	Expect(!result.built, "empty interaction target id should fail build");
	Expect(result.registry.targets().empty(), "failed empty-id build should not publish registry targets");
	Expect(result.issues.size() == 1, "empty interaction target id should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::InteractionTarget2DRegistryIssueCode::EmptyId, "empty id issue should use EmptyId code");
		Expect(result.issues[0].targetIndex == 0, "empty id issue should preserve target index");
		ExpectTarget(result.issues[0].target, target, "empty id issue should preserve target payload");
	}
}

void TestDuplicateIdFailsForLaterTarget()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:lever", iggy::InteractionTarget2DKind::Usable),
		Target("target:lever", iggy::InteractionTarget2DKind::Inspectable, { 4.0F, 5.0F }, 2.0F),
	};

	const iggy::InteractionTarget2DRegistryBuildResult result = iggy::InteractionTarget2DRegistryBuilder {}.build(targets);

	Expect(!result.built, "duplicate interaction target id should fail build");
	Expect(result.registry.targets().empty(), "failed duplicate-id build should not publish registry targets");
	Expect(result.issues.size() == 1, "duplicate interaction target id should report one issue for later duplicate");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::InteractionTarget2DRegistryIssueCode::DuplicateId, "duplicate issue should use DuplicateId code");
		Expect(result.issues[0].targetIndex == 1, "duplicate issue should preserve later target index");
		ExpectTarget(result.issues[0].target, targets[1], "duplicate issue should preserve later target payload");
	}
}

void TestNegativeRadiusFails()
{
	const iggy::InteractionTarget2D target = Target("target:bad_radius", iggy::InteractionTarget2DKind::Door, { 1.0F, 1.0F }, -0.01F);

	const iggy::InteractionTarget2DRegistryBuildResult result = iggy::InteractionTarget2DRegistryBuilder {}.build({ target });

	Expect(!result.built, "negative interaction target radius should fail build");
	Expect(result.registry.targets().empty(), "failed negative-radius build should not publish registry targets");
	Expect(result.issues.size() == 1, "negative interaction target radius should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::InteractionTarget2DRegistryIssueCode::InvalidRadius, "negative radius issue should use InvalidRadius code");
		Expect(result.issues[0].targetIndex == 0, "negative radius issue should preserve target index");
		ExpectTarget(result.issues[0].target, target, "negative radius issue should preserve target payload");
	}
}

void TestMultipleIssuesAreReportedInDeterministicInputOrder()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:lever", iggy::InteractionTarget2DKind::Usable),
		Target("", iggy::InteractionTarget2DKind::Inspectable, { 1.0F, 1.0F }, -1.0F),
		Target("target:lever", iggy::InteractionTarget2DKind::Door, { 2.0F, 2.0F }, -2.0F),
		Target("target:lever", iggy::InteractionTarget2DKind::Pickup, { 3.0F, 3.0F }, 0.0F),
	};

	const iggy::InteractionTarget2DRegistryBuildResult result = iggy::InteractionTarget2DRegistryBuilder {}.build(targets);

	Expect(!result.built, "multiple interaction target issues should fail build");
	Expect(result.registry.targets().empty(), "failed multi-issue build should not publish registry targets");
	Expect(result.issues.size() == 5, "multiple interaction target issues should all be reported");
	if (result.issues.size() == 5) {
		Expect(result.issues[0].code == iggy::InteractionTarget2DRegistryIssueCode::EmptyId && result.issues[0].targetIndex == 1, "empty id should be first issue for second target");
		Expect(result.issues[1].code == iggy::InteractionTarget2DRegistryIssueCode::InvalidRadius && result.issues[1].targetIndex == 1, "invalid radius should follow empty id for second target");
		Expect(result.issues[2].code == iggy::InteractionTarget2DRegistryIssueCode::DuplicateId && result.issues[2].targetIndex == 2, "duplicate id should be first issue for third target");
		Expect(result.issues[3].code == iggy::InteractionTarget2DRegistryIssueCode::InvalidRadius && result.issues[3].targetIndex == 2, "invalid radius should follow duplicate id for third target");
		Expect(result.issues[4].code == iggy::InteractionTarget2DRegistryIssueCode::DuplicateId && result.issues[4].targetIndex == 3, "duplicate id should be reported for fourth target");
	}
}

void TestDisabledTargetsAndUnknownKindAreValid()
{
	const iggy::InteractionTarget2D target = Target("target:unknown", iggy::InteractionTarget2DKind::Unknown, { -2.0F, 8.0F }, 0.0F, false);

	const iggy::InteractionTarget2DRegistryBuildResult result = iggy::InteractionTarget2DRegistryBuilder {}.build({ target });

	Expect(result.built, "disabled unknown interaction target should be valid data");
	Expect(result.issues.empty(), "disabled unknown interaction target should not report issues");
	Expect(result.registry.targets().size() == 1, "disabled unknown interaction target should be preserved");
	if (result.registry.targets().size() == 1)
		ExpectTarget(result.registry.targets()[0], target, "disabled unknown interaction target should preserve payload");
}

void TestNamespacedAndUnqualifiedIdsAreDistinct()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("door", iggy::InteractionTarget2DKind::Door),
		Target("target:door", iggy::InteractionTarget2DKind::Door),
	};

	const iggy::InteractionTarget2DRegistryBuildResult result = iggy::InteractionTarget2DRegistryBuilder {}.build(targets);

	Expect(result.built, "namespaced and unqualified interaction target ids should build as distinct ids");
	Expect(result.issues.empty(), "namespaced and unqualified interaction target ids should not produce duplicate issues");
	Expect(result.registry.contains(iggy::ResourceId { "door" }), "registry should contain unqualified interaction target id");
	Expect(result.registry.contains(iggy::ResourceId { "target:door" }), "registry should contain namespaced interaction target id");
	const iggy::InteractionTarget2D *unqualified = result.registry.find(iggy::ResourceId { "door" });
	const iggy::InteractionTarget2D *namespaced = result.registry.find(iggy::ResourceId { "target:door" });
	Expect(unqualified != nullptr && namespaced != nullptr && unqualified != namespaced, "distinct interaction target ids should resolve to distinct targets");
}

} // namespace

int main()
{
	TestEmptyInputBuildsValidEmptyRegistry();
	TestSuccessfulBuildPreservesOrderAndFields();
	TestFindAndContainsUseExactIds();
	TestEmptyIdFails();
	TestDuplicateIdFailsForLaterTarget();
	TestNegativeRadiusFails();
	TestMultipleIssuesAreReportedInDeterministicInputOrder();
	TestDisabledTargetsAndUnknownKindAreValid();
	TestNamespacedAndUnqualifiedIdsAreDistinct();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
