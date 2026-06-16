#include <cstdlib>

#include "runtime/RuntimeGameplayAsciiScenarioPacketValidator.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::runtime::RuntimeGameplayAsciiScenarioMarkerDeclaration ActorMarker(
	char marker,
	const char *actorId = "npc:ascii",
	const char *profileId = "profile:ascii")
{
	return {
		marker,
		iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::Actor,
		Id(actorId),
		Id(profileId),
		true,
	};
}

iggy::runtime::RuntimeGameplayAsciiScenarioPacket Packet(
	std::vector<std::string> rows,
	std::vector<iggy::runtime::RuntimeGameplayAsciiScenarioMarkerDeclaration> markers = {})
{
	iggy::runtime::RuntimeGameplayAsciiScenarioPacket packet;
	packet.rows = rows;
	packet.markers = markers;
	return packet;
}

iggy::runtime::RuntimeGameplayAsciiScenarioPacketValidationResult Validate(
	const iggy::runtime::RuntimeGameplayAsciiScenarioPacket &packet)
{
	return iggy::runtime::RuntimeGameplayAsciiScenarioPacketValidator {}.validate(packet);
}

bool HasIssue(
	const iggy::runtime::RuntimeGameplayAsciiScenarioPacketValidationResult &result,
	iggy::runtime::RuntimeGameplayAsciiScenarioPacketIssueCode code)
{
	for (const iggy::runtime::RuntimeGameplayAsciiScenarioPacketIssue &issue : result.issues) {
		if (issue.code == code)
			return true;
	}
	return false;
}

void TestValidPacketPasses()
{
	const iggy::runtime::RuntimeGameplayAsciiScenarioPacket packet =
		Packet({
			"#####",
			"#A.@#",
			"#####",
		},
			{
				ActorMarker('A', "npc:guard", "profile:guard"),
				{
					'@',
					iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::PlayerStart,
					{},
					{},
					true,
				},
			});

	const iggy::runtime::RuntimeGameplayAsciiScenarioPacketValidationResult result =
		Validate(packet);

	Expect(result.ok(), "valid ASCII scenario packet should validate");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiScenarioPacketValidationStatus::Valid, "valid ASCII scenario packet should report Valid");
	Expect(result.issueCount == 0, "valid ASCII scenario packet should have zero issues");
	Expect(result.rowCount == 3 && result.width == 5, "valid ASCII scenario packet should report rows and width");
	Expect(result.markerCount == 2, "valid ASCII scenario packet should report marker count");
}

void TestEmptyRowsAreInvalid()
{
	const iggy::runtime::RuntimeGameplayAsciiScenarioPacket packet;

	const iggy::runtime::RuntimeGameplayAsciiScenarioPacketValidationResult result =
		Validate(packet);

	Expect(!result.ok(), "empty ASCII scenario packet should be invalid");
	Expect(result.emptyRowsCount == 1, "empty ASCII scenario packet should count empty rows issue");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiScenarioPacketIssueCode::EmptyRows), "empty ASCII scenario packet should report EmptyRows");
}

void TestRaggedRowsAreInvalid()
{
	const iggy::runtime::RuntimeGameplayAsciiScenarioPacket packet =
		Packet({ "###", "##", "####" });

	const iggy::runtime::RuntimeGameplayAsciiScenarioPacketValidationResult result =
		Validate(packet);

	Expect(!result.ok(), "ragged ASCII scenario packet should be invalid");
	Expect(result.raggedRowCount == 2, "ragged ASCII scenario packet should count ragged rows");
	Expect(result.issues.size() >= 2, "ragged ASCII scenario packet should preserve issues");
	if (result.issues.size() >= 2) {
		Expect(result.issues[0].code == iggy::runtime::RuntimeGameplayAsciiScenarioPacketIssueCode::RaggedRow && result.issues[0].rowIndex == 1, "first ragged row issue should preserve row index");
		Expect(result.issues[1].code == iggy::runtime::RuntimeGameplayAsciiScenarioPacketIssueCode::RaggedRow && result.issues[1].rowIndex == 2, "second ragged row issue should preserve row index");
	}
}

void TestDuplicateMarkerDeclarationsAreInvalid()
{
	const iggy::runtime::RuntimeGameplayAsciiScenarioPacket packet =
		Packet({ "A." }, { ActorMarker('A'), ActorMarker('A', "npc:other", "profile:other") });

	const iggy::runtime::RuntimeGameplayAsciiScenarioPacketValidationResult result =
		Validate(packet);

	Expect(!result.ok(), "duplicate ASCII marker packet should be invalid");
	Expect(result.duplicateMarkerCount == 1, "duplicate ASCII marker packet should count duplicate marker");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiScenarioPacketIssueCode::DuplicateMarker), "duplicate ASCII marker packet should report DuplicateMarker");
	if (!result.issues.empty()) {
		Expect(result.issues[0].markerIndex == 1, "duplicate ASCII marker issue should preserve later marker index");
		Expect(result.issues[0].firstMarkerIndex == 0, "duplicate ASCII marker issue should preserve first marker index");
		Expect(result.issues[0].marker == 'A', "duplicate ASCII marker issue should preserve marker char");
	}
}

void TestActorMarkersRequireIds()
{
	const iggy::runtime::RuntimeGameplayAsciiScenarioPacket packet =
		Packet({ "A" },
			{ {
				'A',
				iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::Actor,
				{},
				{},
				true,
			} });

	const iggy::runtime::RuntimeGameplayAsciiScenarioPacketValidationResult result =
		Validate(packet);

	Expect(!result.ok(), "actor marker without ids should be invalid");
	Expect(result.emptyActorIdCount == 1, "actor marker without actor id should count issue");
	Expect(result.emptyProfileIdCount == 1, "actor marker without profile id should count issue");
	Expect(result.issues.size() >= 2, "actor marker id issues should be preserved");
	if (result.issues.size() >= 2) {
		Expect(result.issues[0].code == iggy::runtime::RuntimeGameplayAsciiScenarioPacketIssueCode::EmptyActorId, "actor id issue should come before profile id issue");
		Expect(result.issues[1].code == iggy::runtime::RuntimeGameplayAsciiScenarioPacketIssueCode::EmptyProfileId, "profile id issue should follow actor id issue");
	}
}

void TestUnknownMarkerKindIsInvalid()
{
	const iggy::runtime::RuntimeGameplayAsciiScenarioPacket packet =
		Packet({ "?" },
			{ {
				'?',
				iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::Unknown,
				{},
				{},
				true,
			} });

	const iggy::runtime::RuntimeGameplayAsciiScenarioPacketValidationResult result =
		Validate(packet);

	Expect(!result.ok(), "unknown marker kind should be invalid");
	Expect(result.unknownMarkerKindCount == 1, "unknown marker kind should count issue");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiScenarioPacketIssueCode::UnknownMarkerKind), "unknown marker kind should report issue");
}

void TestUnknownGridMarkersAreInvalid()
{
	const iggy::runtime::RuntimeGameplayAsciiScenarioPacket packet =
		Packet({ "A?." }, { ActorMarker('A') });

	const iggy::runtime::RuntimeGameplayAsciiScenarioPacketValidationResult result =
		Validate(packet);

	Expect(!result.ok(), "unknown grid marker should be invalid");
	Expect(result.unknownGridMarkerCount == 1, "unknown grid marker should count issue");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiScenarioPacketIssueCode::UnknownGridMarker), "unknown grid marker should report issue");
	if (!result.issues.empty()) {
		const iggy::runtime::RuntimeGameplayAsciiScenarioPacketIssue &issue =
			result.issues.back();
		Expect(issue.rowIndex == 0 && issue.columnIndex == 1, "unknown grid marker should preserve cell coordinate");
		Expect(issue.marker == '?', "unknown grid marker should preserve marker character");
	}
}

void TestIssueOrderingIsDeterministic()
{
	const iggy::runtime::RuntimeGameplayAsciiScenarioPacket packet =
		Packet({ "A?", "A" },
			{
				ActorMarker('A'),
				ActorMarker('A', "npc:duplicate", "profile:duplicate"),
			});

	const iggy::runtime::RuntimeGameplayAsciiScenarioPacketValidationResult result =
		Validate(packet);

	Expect(result.issues.size() == 3, "mixed ASCII packet should report three issues");
	if (result.issues.size() == 3) {
		Expect(result.issues[0].code == iggy::runtime::RuntimeGameplayAsciiScenarioPacketIssueCode::RaggedRow, "mixed issue order should start with packet row shape");
		Expect(result.issues[1].code == iggy::runtime::RuntimeGameplayAsciiScenarioPacketIssueCode::DuplicateMarker, "mixed issue order should then report marker declaration issues");
		Expect(result.issues[2].code == iggy::runtime::RuntimeGameplayAsciiScenarioPacketIssueCode::UnknownGridMarker, "mixed issue order should then scan grid markers");
	}
}

void TestNamespacedAndUnqualifiedIdsRemainExact()
{
	const iggy::runtime::RuntimeGameplayAsciiScenarioPacket packet =
		Packet({ "Ab" },
			{
				ActorMarker('A', "npc:namespaced", "profile:namespaced"),
				ActorMarker('b', "plain", "plain-profile"),
			});

	const iggy::runtime::RuntimeGameplayAsciiScenarioPacketValidationResult result =
		Validate(packet);

	Expect(result.ok(), "exact id ASCII packet should validate");
	Expect(result.packet.markers[0].actorId == Id("npc:namespaced"), "validator should preserve namespaced actor id");
	Expect(result.packet.markers[0].profileId == Id("profile:namespaced"), "validator should preserve namespaced profile id");
	Expect(result.packet.markers[1].actorId == Id("plain"), "validator should preserve unqualified actor id");
	Expect(result.packet.markers[1].profileId == Id("plain-profile"), "validator should preserve unqualified profile id");
}

void TestValidatorDoesNotMutatePacket()
{
	iggy::runtime::RuntimeGameplayAsciiScenarioPacket packet =
		Packet({ "A." }, { ActorMarker('A', "npc:immutable", "profile:immutable") });
	const iggy::runtime::RuntimeGameplayAsciiScenarioPacket before = packet;

	const iggy::runtime::RuntimeGameplayAsciiScenarioPacketValidationResult result =
		Validate(packet);

	Expect(result.ok(), "immutability setup should validate");
	Expect(packet.rows == before.rows, "ASCII validator should not mutate rows");
	Expect(packet.markers[0].actorId == before.markers[0].actorId, "ASCII validator should not mutate actor id");
	Expect(packet.markers[0].profileId == before.markers[0].profileId, "ASCII validator should not mutate profile id");
}

} // namespace

int main()
{
	TestValidPacketPasses();
	TestEmptyRowsAreInvalid();
	TestRaggedRowsAreInvalid();
	TestDuplicateMarkerDeclarationsAreInvalid();
	TestActorMarkersRequireIds();
	TestUnknownMarkerKindIsInvalid();
	TestUnknownGridMarkersAreInvalid();
	TestIssueOrderingIsDeterministic();
	TestNamespacedAndUnqualifiedIdsRemainExact();
	TestValidatorDoesNotMutatePacket();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
