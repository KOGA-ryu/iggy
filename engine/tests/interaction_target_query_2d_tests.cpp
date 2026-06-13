#include <cstdlib>
#include <vector>

#include "scene/interaction/InteractionTargetQuery2D.hpp"
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

iggy::InteractionTarget2DRegistry Registry(std::vector<iggy::InteractionTarget2D> targets)
{
	const iggy::InteractionTarget2DRegistryBuildResult result = iggy::InteractionTarget2DRegistryBuilder {}.build(targets);
	Expect(result.built, "query test registry setup should build");
	return result.registry;
}

void ExpectDefaultTarget(const iggy::InteractionTarget2D &target, const char *message)
{
	Expect(target.id.empty(), message);
	Expect(target.kind == iggy::InteractionTarget2DKind::Unknown, message);
	Expect(NearVec(target.position, { 0.0F, 0.0F }), message);
	Expect(target.radius == 0.0F, message);
	Expect(target.enabled, message);
}

void ExpectTarget(const iggy::InteractionTarget2D &target, const iggy::InteractionTarget2D &expected, const char *message)
{
	Expect(target.id == expected.id, message);
	Expect(target.kind == expected.kind, message);
	Expect(NearVec(target.position, expected.position), message);
	Expect(target.radius == expected.radius, message);
	Expect(target.enabled == expected.enabled, message);
}

void TestMissingEmptyId()
{
	const iggy::InteractionTarget2DRegistry registry = Registry({});

	const iggy::InteractionTargetQuery2DResult result = iggy::InteractionTargetQuery2D {}.find(registry, {});

	Expect(result.status == iggy::InteractionTargetQuery2DStatus::MissingTargetId, "empty target id query should report MissingTargetId");
	Expect(result.targetId.empty(), "empty target id query should preserve requested id");
	Expect(!result.hasTarget(), "empty target id query should not have target");
	ExpectDefaultTarget(result.target, "empty target id query should preserve default target");
}

void TestMissingNonEmptyId()
{
	const iggy::InteractionTarget2DRegistry registry = Registry({
		Target("target:lever", iggy::InteractionTarget2DKind::Usable),
	});

	const iggy::ResourceId missingId { "target:missing" };
	const iggy::InteractionTargetQuery2DResult result = iggy::InteractionTargetQuery2D {}.find(registry, missingId);

	Expect(result.status == iggy::InteractionTargetQuery2DStatus::NotFound, "missing non-empty target id should report NotFound");
	Expect(result.targetId == missingId, "missing non-empty target id query should preserve requested id");
	Expect(!result.hasTarget(), "missing non-empty target id should not have target");
	ExpectDefaultTarget(result.target, "missing non-empty target id should preserve default target");
}

void TestFoundEnabledTargetPreservesFields()
{
	const iggy::InteractionTarget2D target = Target("target:door", iggy::InteractionTarget2DKind::Door, { 2.5F, -1.0F }, 1.25F, true);
	const iggy::InteractionTarget2DRegistry registry = Registry({
		Target("target:lever", iggy::InteractionTarget2DKind::Usable),
		target,
	});

	const iggy::InteractionTargetQuery2DResult result = iggy::InteractionTargetQuery2D {}.find(registry, target.id);

	Expect(result.status == iggy::InteractionTargetQuery2DStatus::Found, "enabled target query should report Found");
	Expect(result.targetId == target.id, "enabled target query should preserve requested id");
	Expect(result.hasTarget(), "enabled target query should have target");
	ExpectTarget(result.target, target, "enabled target query should copy target fields exactly");
}

void TestFoundDisabledTargetReportsDisabledAndCopiesTarget()
{
	const iggy::InteractionTarget2D target = Target("target:disabled", iggy::InteractionTarget2DKind::Talk, { -4.0F, 9.0F }, 2.0F, false);
	const iggy::InteractionTarget2DRegistry registry = Registry({
		target,
	});

	const iggy::InteractionTargetQuery2DResult result = iggy::InteractionTargetQuery2D {}.find(registry, target.id);

	Expect(result.status == iggy::InteractionTargetQuery2DStatus::Disabled, "disabled target query should report Disabled");
	Expect(result.targetId == target.id, "disabled target query should preserve requested id");
	Expect(result.hasTarget(), "disabled target query should still have copied target");
	ExpectTarget(result.target, target, "disabled target query should copy target fields exactly");
}

void TestHasTargetOnlyForFoundAndDisabled()
{
	iggy::InteractionTargetQuery2DResult result;

	result.status = iggy::InteractionTargetQuery2DStatus::Found;
	Expect(result.hasTarget(), "Found query result should have target");
	result.status = iggy::InteractionTargetQuery2DStatus::Disabled;
	Expect(result.hasTarget(), "Disabled query result should have target");
	result.status = iggy::InteractionTargetQuery2DStatus::MissingTargetId;
	Expect(!result.hasTarget(), "MissingTargetId query result should not have target");
	result.status = iggy::InteractionTargetQuery2DStatus::NotFound;
	Expect(!result.hasTarget(), "NotFound query result should not have target");
}

void TestNamespacedAndUnqualifiedIdsRemainDistinct()
{
	const iggy::InteractionTarget2D unqualified = Target("door", iggy::InteractionTarget2DKind::Door, { 1.0F, 1.0F }, 0.5F);
	const iggy::InteractionTarget2D namespaced = Target("target:door", iggy::InteractionTarget2DKind::Inspectable, { 2.0F, 2.0F }, 0.75F);
	const iggy::InteractionTarget2DRegistry registry = Registry({
		unqualified,
		namespaced,
	});

	const iggy::InteractionTargetQuery2DResult unqualifiedResult = iggy::InteractionTargetQuery2D {}.find(registry, iggy::ResourceId { "door" });
	const iggy::InteractionTargetQuery2DResult namespacedResult = iggy::InteractionTargetQuery2D {}.find(registry, iggy::ResourceId { "target:door" });

	Expect(unqualifiedResult.status == iggy::InteractionTargetQuery2DStatus::Found, "unqualified interaction target id should be found");
	Expect(namespacedResult.status == iggy::InteractionTargetQuery2DStatus::Found, "namespaced interaction target id should be found");
	ExpectTarget(unqualifiedResult.target, unqualified, "unqualified interaction target query should return unqualified target");
	ExpectTarget(namespacedResult.target, namespaced, "namespaced interaction target query should return namespaced target");
}

void TestEmptyRegistryReturnsNotFoundForNonEmptyId()
{
	const iggy::InteractionTarget2DRegistry registry = Registry({});
	const iggy::ResourceId targetId { "target:missing" };

	const iggy::InteractionTargetQuery2DResult result = iggy::InteractionTargetQuery2D {}.find(registry, targetId);

	Expect(result.status == iggy::InteractionTargetQuery2DStatus::NotFound, "empty registry should return NotFound for non-empty id");
	Expect(result.targetId == targetId, "empty registry query should preserve requested non-empty id");
	Expect(!result.hasTarget(), "empty registry query should not have target");
	ExpectDefaultTarget(result.target, "empty registry query should preserve default target");
}

void TestQueryDoesNotMutateRegistryOrRequestedId()
{
	const iggy::InteractionTarget2D target = Target("target:immutable", iggy::InteractionTarget2DKind::Pickup, { 3.0F, 4.0F }, 1.0F);
	const iggy::InteractionTarget2DRegistry registry = Registry({
		target,
	});
	const iggy::ResourceId targetId { "target:immutable" };
	const iggy::ResourceId targetIdBefore = targetId;
	const std::vector<iggy::InteractionTarget2D> registryTargetsBefore = registry.targets();

	const iggy::InteractionTargetQuery2DResult result = iggy::InteractionTargetQuery2D {}.find(registry, targetId);

	Expect(result.status == iggy::InteractionTargetQuery2DStatus::Found, "immutability setup should find target");
	Expect(targetId == targetIdBefore, "interaction target query should not mutate requested id");
	Expect(registry.targets().size() == registryTargetsBefore.size(), "interaction target query should not mutate registry target count");
	if (registry.targets().size() == registryTargetsBefore.size())
		ExpectTarget(registry.targets()[0], registryTargetsBefore[0], "interaction target query should not mutate registry target payload");
}

} // namespace

int main()
{
	TestMissingEmptyId();
	TestMissingNonEmptyId();
	TestFoundEnabledTargetPreservesFields();
	TestFoundDisabledTargetReportsDisabledAndCopiesTarget();
	TestHasTargetOnlyForFoundAndDisabled();
	TestNamespacedAndUnqualifiedIdsRemainDistinct();
	TestEmptyRegistryReturnsNotFoundForNonEmptyId();
	TestQueryDoesNotMutateRegistryOrRequestedId();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
