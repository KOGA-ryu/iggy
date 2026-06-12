#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include "runtime/RuntimeSaveSlotPathPolicy.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::runtime::RuntimeSaveSlotPathPolicyConfig Config(
	std::filesystem::path baseDirectory = "saves",
	std::string extension = ".igsave")
{
	return { baseDirectory, extension };
}

iggy::runtime::RuntimeSaveSlotId Slot(
	iggy::runtime::RuntimeSaveSlotKind kind,
	std::string name)
{
	return { kind, name };
}

std::filesystem::path TempRoot()
{
	return std::filesystem::current_path() / "runtime_save_slot_path_policy_tests_tmp";
}

void RemoveTempRoot()
{
	std::error_code ignored;
	std::filesystem::remove_all(TempRoot(), ignored);
}

void TestValidManualAutoQuickPaths()
{
	const iggy::runtime::RuntimeSaveSlotPathPolicy policy;

	const iggy::runtime::RuntimeSaveSlotPathResult manual = policy.pathFor(Config("base"), Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "slot1"));
	const iggy::runtime::RuntimeSaveSlotPathResult autosave = policy.pathFor(Config("base"), Slot(iggy::runtime::RuntimeSaveSlotKind::Auto, "slot1"));
	const iggy::runtime::RuntimeSaveSlotPathResult quick = policy.pathFor(Config("base"), Slot(iggy::runtime::RuntimeSaveSlotKind::Quick, "slot1"));

	Expect(manual.status == iggy::runtime::RuntimeSaveSlotPathStatus::Valid, "manual slot path should be valid");
	Expect(manual.path == std::filesystem::path("base") / "manual" / "slot1.igsave", "manual slot path should use manual directory");
	Expect(autosave.status == iggy::runtime::RuntimeSaveSlotPathStatus::Valid, "auto slot path should be valid");
	Expect(autosave.path == std::filesystem::path("base") / "auto" / "slot1.igsave", "auto slot path should use auto directory");
	Expect(quick.status == iggy::runtime::RuntimeSaveSlotPathStatus::Valid, "quick slot path should be valid");
	Expect(quick.path == std::filesystem::path("base") / "quick" / "slot1.igsave", "quick slot path should use quick directory");
}

void TestEmptyBaseDirectoryInvalid()
{
	const iggy::runtime::RuntimeSaveSlotPathPolicy policy;

	const iggy::runtime::RuntimeSaveSlotPathResult result = policy.pathFor(Config({}), Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "slot1"));

	Expect(result.status == iggy::runtime::RuntimeSaveSlotPathStatus::EmptyBaseDirectory, "empty base directory should be invalid");
	Expect(result.path.empty(), "invalid empty base directory should not publish a path");
}

void TestEmptySlotNameInvalid()
{
	const iggy::runtime::RuntimeSaveSlotPathPolicy policy;

	const iggy::runtime::RuntimeSaveSlotPathResult result = policy.pathFor(Config("base"), Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, ""));

	Expect(result.status == iggy::runtime::RuntimeSaveSlotPathStatus::EmptySlotName, "empty slot name should be invalid");
	Expect(result.path.empty(), "empty slot name should not publish a path");
}

void TestAllowedSlotNameCharacters()
{
	const iggy::runtime::RuntimeSaveSlotPathPolicy policy;
	const std::vector<std::string> names { "abcXYZ", "slot_01", "slot-01", "A1_b-2" };

	for (const std::string &name : names) {
		const iggy::runtime::RuntimeSaveSlotPathResult result = policy.pathFor(Config("base"), Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, name));
		Expect(result.status == iggy::runtime::RuntimeSaveSlotPathStatus::Valid, "allowed slot name should be valid");
		Expect(result.path == std::filesystem::path("base") / "manual" / (name + ".igsave"), "allowed slot name should be used in path");
	}
}

void TestInvalidSlotNameCharacters()
{
	const iggy::runtime::RuntimeSaveSlotPathPolicy policy;
	const std::vector<std::string> names { "slot/name", "slot\\name", "slot.name", "slot name", "slot:name", "slot+name", "slot@name" };

	for (const std::string &name : names) {
		const iggy::runtime::RuntimeSaveSlotPathResult result = policy.pathFor(Config("base"), Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, name));
		Expect(result.status == iggy::runtime::RuntimeSaveSlotPathStatus::InvalidSlotName, "invalid slot name character should be rejected");
		Expect(result.path.empty(), "invalid slot name should not publish a path");
	}
}

void TestDefaultAndEmptyExtension()
{
	const iggy::runtime::RuntimeSaveSlotPathPolicy policy;

	const iggy::runtime::RuntimeSaveSlotPathResult defaultExtension = policy.pathFor(Config("base"), Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "slot1"));
	const iggy::runtime::RuntimeSaveSlotPathResult emptyExtension = policy.pathFor(Config("base", ""), Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "slot1"));

	Expect(defaultExtension.status == iggy::runtime::RuntimeSaveSlotPathStatus::Valid, "default extension slot path should be valid");
	Expect(defaultExtension.path == std::filesystem::path("base") / "manual" / "slot1.igsave", "default extension should be applied");
	Expect(emptyExtension.status == iggy::runtime::RuntimeSaveSlotPathStatus::Valid, "empty extension slot path should be valid");
	Expect(emptyExtension.path == std::filesystem::path("base") / "manual" / "slot1", "empty extension should produce path without extension");
}

void TestPolicyDoesNotTouchFilesystem()
{
	RemoveTempRoot();
	const iggy::runtime::RuntimeSaveSlotPathPolicy policy;

	const iggy::runtime::RuntimeSaveSlotPathResult result = policy.pathFor(Config(TempRoot()), Slot(iggy::runtime::RuntimeSaveSlotKind::Auto, "slot1"));

	Expect(result.status == iggy::runtime::RuntimeSaveSlotPathStatus::Valid, "filesystem side-effect setup should create valid path");
	Expect(!std::filesystem::exists(TempRoot()), "slot path policy should not create base directory");
	Expect(!std::filesystem::exists(result.path), "slot path policy should not create save file");
}

void TestPolicyDoesNotMutateInputs()
{
	const iggy::runtime::RuntimeSaveSlotPathPolicy policy;
	iggy::runtime::RuntimeSaveSlotPathPolicyConfig config = Config("base", ".sav");
	const iggy::runtime::RuntimeSaveSlotPathPolicyConfig configBefore = config;
	iggy::runtime::RuntimeSaveSlotId slot = Slot(iggy::runtime::RuntimeSaveSlotKind::Quick, "slot_1");
	const iggy::runtime::RuntimeSaveSlotId slotBefore = slot;

	const iggy::runtime::RuntimeSaveSlotPathResult result = policy.pathFor(config, slot);

	Expect(result.status == iggy::runtime::RuntimeSaveSlotPathStatus::Valid, "slot path immutability setup should be valid");
	Expect(config.baseDirectory == configBefore.baseDirectory, "slot path policy should not mutate base directory");
	Expect(config.extension == configBefore.extension, "slot path policy should not mutate extension");
	Expect(slot.kind == slotBefore.kind, "slot path policy should not mutate slot kind");
	Expect(slot.name == slotBefore.name, "slot path policy should not mutate slot name");
	Expect(result.slot.kind == slot.kind && result.slot.name == slot.name, "slot path result should preserve slot");
}

} // namespace

int main()
{
	TestValidManualAutoQuickPaths();
	TestEmptyBaseDirectoryInvalid();
	TestEmptySlotNameInvalid();
	TestAllowedSlotNameCharacters();
	TestInvalidSlotNameCharacters();
	TestDefaultAndEmptyExtension();
	TestPolicyDoesNotTouchFilesystem();
	TestPolicyDoesNotMutateInputs();

	RemoveTempRoot();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
