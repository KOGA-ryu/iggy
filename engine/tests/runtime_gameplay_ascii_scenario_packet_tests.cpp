#include <cstdlib>

#include "runtime/RuntimeGameplayAsciiScenarioPacket.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

void TestDefaultPacketIsEmpty()
{
	const iggy::runtime::RuntimeGameplayAsciiScenarioPacket packet;

	Expect(!packet.hasSourceId, "default ASCII scenario packet should not have source id");
	Expect(packet.sourceId.empty(), "default ASCII scenario packet source id should be empty");
	Expect(!packet.hasRows(), "default ASCII scenario packet should not have rows");
	Expect(packet.rowCount() == 0, "default ASCII scenario packet should report zero rows");
	Expect(packet.markerCount() == 0, "default ASCII scenario packet should report zero markers");
	Expect(packet.frameCount() == 0, "default ASCII scenario packet should report zero frames");
}

void TestPacketPreservesRowsAndSource()
{
	iggy::runtime::RuntimeGameplayAsciiScenarioPacket packet;
	packet.hasSourceId = true;
	packet.sourceId = Id("source:ascii");
	packet.rows = {
		"#####",
		"#A..#",
		"#####",
	};

	Expect(packet.hasSourceId, "ASCII scenario packet should preserve source flag");
	Expect(packet.sourceId == Id("source:ascii"), "ASCII scenario packet should preserve source id");
	Expect(packet.hasRows(), "ASCII scenario packet should report rows");
	Expect(packet.rowCount() == 3, "ASCII scenario packet should preserve row count");
	Expect(packet.rows[1] == "#A..#", "ASCII scenario packet should preserve row text exactly");
}

void TestMarkerDeclarationsPreserveExactIdsAndFlags()
{
	iggy::runtime::RuntimeGameplayAsciiScenarioPacket packet;
	packet.markers = {
		{
			'A',
			iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::Actor,
			Id("npc:guard"),
			Id("profile:guard"),
			true,
		},
		{
			'b',
			iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::Actor,
			Id("bandit"),
			Id("plain-profile"),
			false,
		},
		{
			'@',
			iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::PlayerStart,
			{},
			{},
			true,
		},
	};

	Expect(packet.markerCount() == 3, "ASCII scenario packet should preserve marker count");
	Expect(packet.markers[0].marker == 'A', "ASCII scenario marker should preserve character");
	Expect(packet.markers[0].actorId == Id("npc:guard"), "ASCII scenario marker should preserve namespaced actor id");
	Expect(packet.markers[0].profileId == Id("profile:guard"), "ASCII scenario marker should preserve namespaced profile id");
	Expect(packet.markers[1].actorId == Id("bandit"), "ASCII scenario marker should preserve unqualified actor id");
	Expect(packet.markers[1].profileId == Id("plain-profile"), "ASCII scenario marker should preserve unqualified profile id");
	Expect(!packet.markers[1].present, "ASCII scenario marker should preserve present flag");
	Expect(packet.markers[2].kind == iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::PlayerStart, "ASCII scenario marker should preserve player-start kind");
}

void TestFrameDeclarationsPreserveOrder()
{
	iggy::runtime::RuntimeGameplayAsciiScenarioPacket packet;
	packet.frames = {
		{ true, Id("frame:first") },
		{ false, {} },
		{ true, Id("frame:third") },
	};

	Expect(packet.frameCount() == 3, "ASCII scenario packet should preserve frame count");
	Expect(packet.frames[0].hasFrameId && packet.frames[0].frameId == Id("frame:first"), "ASCII scenario packet should preserve first frame id");
	Expect(!packet.frames[1].hasFrameId && packet.frames[1].frameId.empty(), "ASCII scenario packet should preserve anonymous frame declaration");
	Expect(packet.frames[2].hasFrameId && packet.frames[2].frameId == Id("frame:third"), "ASCII scenario packet should preserve frame order");
}

void TestPacketCopiesAreIndependent()
{
	iggy::runtime::RuntimeGameplayAsciiScenarioPacket packet;
	packet.hasSourceId = true;
	packet.sourceId = Id("source:copy");
	packet.rows = { "A." };
	packet.markers = {
		{ 'A', iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::Actor, Id("npc:copy"), Id("profile:copy"), true },
	};

	iggy::runtime::RuntimeGameplayAsciiScenarioPacket copy = packet;
	copy.rows[0] = "..";
	copy.markers[0].actorId = Id("npc:changed");

	Expect(packet.rows[0] == "A.", "ASCII scenario packet copy should not mutate source rows");
	Expect(packet.markers[0].actorId == Id("npc:copy"), "ASCII scenario packet copy should not mutate source marker ids");
}

} // namespace

int main()
{
	TestDefaultPacketIsEmpty();
	TestPacketPreservesRowsAndSource();
	TestMarkerDeclarationsPreserveExactIdsAndFlags();
	TestFrameDeclarationsPreserveOrder();
	TestPacketCopiesAreIndependent();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
