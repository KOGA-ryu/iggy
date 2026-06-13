#include <cstdlib>
#include <string>

#include "runtime/RuntimeAutosaveRotationPolicy.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

bool SameSlot(const iggy::runtime::RuntimeSaveSlotId &slot, iggy::runtime::RuntimeSaveSlotKind kind, const std::string &name)
{
	return slot.kind == kind && slot.name == name;
}

void ExpectAutoSlot(const iggy::runtime::RuntimeSaveSlotId &slot, const std::string &name, const char *message)
{
	Expect(SameSlot(slot, iggy::runtime::RuntimeSaveSlotKind::Auto, name), message);
}

void ExpectRename(
	const iggy::runtime::RuntimeAutosaveRotationRename &rename,
	const std::string &from,
	const std::string &to,
	const char *message)
{
	ExpectAutoSlot(rename.from, from, message);
	ExpectAutoSlot(rename.to, to, message);
}

bool PathPolicyAccepts(const iggy::runtime::RuntimeSaveSlotId &slot)
{
	iggy::runtime::RuntimeSaveSlotPathPolicyConfig config;
	config.baseDirectory = "saves";
	return iggy::runtime::RuntimeSaveSlotPathPolicy {}.pathFor(config, slot).status == iggy::runtime::RuntimeSaveSlotPathStatus::Valid;
}

void TestMaxSlotsZeroInvalid()
{
	const iggy::runtime::RuntimeAutosaveRotationResult result = iggy::runtime::RuntimeAutosaveRotationPolicy {}.plan(0);

	Expect(!result.plan.valid, "maxSlots zero should produce invalid plan");
	Expect(result.plan.deleteSlots.empty(), "invalid autosave plan should not publish delete slots");
	Expect(result.plan.renames.empty(), "invalid autosave plan should not publish renames");
	Expect(result.issues.size() == 1, "invalid autosave plan should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::runtime::RuntimeAutosaveRotationIssueCode::InvalidMaxSlots, "invalid autosave issue should use InvalidMaxSlots");
		Expect(result.issues[0].maxSlots == 0, "invalid autosave issue should preserve maxSlots");
	}
}

void TestMaxSlotsOneWritesAutosaveZeroOnly()
{
	const iggy::runtime::RuntimeAutosaveRotationResult result = iggy::runtime::RuntimeAutosaveRotationPolicy {}.plan(1);

	Expect(result.plan.valid, "maxSlots one should produce valid plan");
	Expect(result.issues.empty(), "maxSlots one should have no issues");
	ExpectAutoSlot(result.plan.writeSlot, "autosave_0", "maxSlots one should write autosave_0");
	Expect(result.plan.deleteSlots.empty(), "maxSlots one should not delete write slot");
	Expect(result.plan.renames.empty(), "maxSlots one should have no renames");
}

void TestMaxSlotsTwoDeletesOldestAndRenamesNewest()
{
	const iggy::runtime::RuntimeAutosaveRotationResult result = iggy::runtime::RuntimeAutosaveRotationPolicy {}.plan(2);

	Expect(result.plan.valid, "maxSlots two should produce valid plan");
	ExpectAutoSlot(result.plan.writeSlot, "autosave_0", "maxSlots two should write autosave_0");
	Expect(result.plan.deleteSlots.size() == 1, "maxSlots two should delete oldest slot");
	if (result.plan.deleteSlots.size() == 1)
		ExpectAutoSlot(result.plan.deleteSlots[0], "autosave_1", "maxSlots two should delete autosave_1");
	Expect(result.plan.renames.size() == 1, "maxSlots two should have one rename");
	if (result.plan.renames.size() == 1)
		ExpectRename(result.plan.renames[0], "autosave_0", "autosave_1", "maxSlots two should rename autosave_0 to autosave_1");
}

void TestMaxSlotsThreeUsesSafeOldestToNewestRenameOrder()
{
	const iggy::runtime::RuntimeAutosaveRotationResult result = iggy::runtime::RuntimeAutosaveRotationPolicy {}.plan(3);

	Expect(result.plan.valid, "maxSlots three should produce valid plan");
	ExpectAutoSlot(result.plan.writeSlot, "autosave_0", "maxSlots three should write autosave_0");
	Expect(result.plan.deleteSlots.size() == 1, "maxSlots three should delete oldest slot");
	if (result.plan.deleteSlots.size() == 1)
		ExpectAutoSlot(result.plan.deleteSlots[0], "autosave_2", "maxSlots three should delete autosave_2");
	Expect(result.plan.renames.size() == 2, "maxSlots three should have two renames");
	if (result.plan.renames.size() == 2) {
		ExpectRename(result.plan.renames[0], "autosave_1", "autosave_2", "maxSlots three should first rename autosave_1 to autosave_2");
		ExpectRename(result.plan.renames[1], "autosave_0", "autosave_1", "maxSlots three should then rename autosave_0 to autosave_1");
	}
}

void TestGeneratedSlotsAreAutoAndPathPolicyValid()
{
	const iggy::runtime::RuntimeAutosaveRotationResult result = iggy::runtime::RuntimeAutosaveRotationPolicy {}.plan(4);

	Expect(result.plan.valid, "slot validation setup should produce valid plan");
	Expect(PathPolicyAccepts(result.plan.writeSlot), "write slot should be accepted by path policy");
	for (const iggy::runtime::RuntimeSaveSlotId &slot : result.plan.deleteSlots) {
		Expect(slot.kind == iggy::runtime::RuntimeSaveSlotKind::Auto, "delete slot should be auto kind");
		Expect(PathPolicyAccepts(slot), "delete slot should be accepted by path policy");
	}
	for (const iggy::runtime::RuntimeAutosaveRotationRename &rename : result.plan.renames) {
		Expect(rename.from.kind == iggy::runtime::RuntimeSaveSlotKind::Auto, "rename source should be auto kind");
		Expect(rename.to.kind == iggy::runtime::RuntimeSaveSlotKind::Auto, "rename target should be auto kind");
		Expect(PathPolicyAccepts(rename.from), "rename source should be accepted by path policy");
		Expect(PathPolicyAccepts(rename.to), "rename target should be accepted by path policy");
	}
}

void TestPlanIsDeterministic()
{
	const iggy::runtime::RuntimeAutosaveRotationResult first = iggy::runtime::RuntimeAutosaveRotationPolicy {}.plan(3);
	const iggy::runtime::RuntimeAutosaveRotationResult second = iggy::runtime::RuntimeAutosaveRotationPolicy {}.plan(3);

	Expect(first.plan.valid == second.plan.valid, "autosave plan validity should be deterministic");
	Expect(first.plan.writeSlot.name == second.plan.writeSlot.name, "autosave write slot should be deterministic");
	Expect(first.plan.deleteSlots.size() == second.plan.deleteSlots.size(), "autosave delete count should be deterministic");
	for (std::size_t index = 0; index < first.plan.deleteSlots.size() && index < second.plan.deleteSlots.size(); ++index)
		Expect(first.plan.deleteSlots[index].name == second.plan.deleteSlots[index].name, "autosave delete slot should be deterministic");
	Expect(first.plan.renames.size() == second.plan.renames.size(), "autosave rename count should be deterministic");
	for (std::size_t index = 0; index < first.plan.renames.size() && index < second.plan.renames.size(); ++index) {
		Expect(first.plan.renames[index].from.name == second.plan.renames[index].from.name, "autosave rename source should be deterministic");
		Expect(first.plan.renames[index].to.name == second.plan.renames[index].to.name, "autosave rename target should be deterministic");
	}
}

} // namespace

int main()
{
	TestMaxSlotsZeroInvalid();
	TestMaxSlotsOneWritesAutosaveZeroOnly();
	TestMaxSlotsTwoDeletesOldestAndRenamesNewest();
	TestMaxSlotsThreeUsesSafeOldestToNewestRenameOrder();
	TestGeneratedSlotsAreAutoAndPathPolicyValid();
	TestPlanIsDeterministic();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
