#include <cstdlib>
#include <vector>

#include "runtime/RuntimeBinaryCodec.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::Near;

bool SameBytes(const std::vector<std::uint8_t> &actual, const std::vector<std::uint8_t> &expected)
{
	return actual == expected;
}

void ExpectBytes(const std::vector<std::uint8_t> &actual, const std::vector<std::uint8_t> &expected, const char *message)
{
	Expect(SameBytes(actual, expected), message);
}

void TestIntegerWritesUseLittleEndianOrder()
{
	iggy::runtime::RuntimeBinaryWriter writer;
	writer.writeU16LE(0x1234U);
	writer.writeU32LE(0x12345678U);
	writer.writeU64LE(0x0123456789ABCDEFULL);

	ExpectBytes(
		writer.bytes(),
		{
			0x34, 0x12,
			0x78, 0x56, 0x34, 0x12,
			0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01,
		},
		"integer writer should emit exact little-endian bytes");
}

void TestNegativeI32WritesTwosComplementLittleEndian()
{
	iggy::runtime::RuntimeBinaryWriter writer;
	writer.writeI32LE(-2);

	ExpectBytes(writer.bytes(), { 0xFE, 0xFF, 0xFF, 0xFF }, "negative int32 should write two's-complement little-endian bytes");
}

void TestF32WritesKnownIeeeBytes()
{
	iggy::runtime::RuntimeBinaryWriter writer;
	writer.writeF32LE(1.0F);
	writer.writeF32LE(-2.5F);

	ExpectBytes(writer.bytes(), { 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x20, 0xC0 }, "float writer should emit known IEEE-754 little-endian bytes");
}

void TestBoolWritesAndReads()
{
	iggy::runtime::RuntimeBinaryWriter writer;
	writer.writeBool(false);
	writer.writeBool(true);
	writer.writeU8(7);

	iggy::runtime::RuntimeBinaryReader reader(writer.bytes());
	bool first = true;
	bool second = false;
	bool third = false;

	Expect(reader.readBool(first) == iggy::runtime::RuntimeBinaryReadStatus::Ok, "reader should read false bool");
	Expect(reader.readBool(second) == iggy::runtime::RuntimeBinaryReadStatus::Ok, "reader should read true bool");
	Expect(reader.readBool(third) == iggy::runtime::RuntimeBinaryReadStatus::Ok, "reader should read nonzero bool as true");
	Expect(!first, "false bool should read as false");
	Expect(second, "true bool should read as true");
	Expect(third, "nonzero bool byte should read as true");
}

void TestReaderReadsValuesAndTracksOffset()
{
	iggy::runtime::RuntimeBinaryWriter writer;
	writer.writeU8(0xAAU);
	writer.writeU16LE(0x1234U);
	writer.writeU32LE(0x89ABCDEFU);
	writer.writeU64LE(0x0123456789ABCDEFULL);
	writer.writeI32LE(-123456);
	writer.writeF32LE(-2.5F);

	iggy::runtime::RuntimeBinaryReader reader(writer.bytes());
	std::uint8_t u8 = 0;
	std::uint16_t u16 = 0;
	std::uint32_t u32 = 0;
	std::uint64_t u64 = 0;
	std::int32_t i32 = 0;
	float f32 = 0.0F;

	Expect(reader.offset() == 0 && reader.remaining() == writer.bytes().size(), "reader should start at offset zero");
	Expect(reader.readU8(u8) == iggy::runtime::RuntimeBinaryReadStatus::Ok, "reader should read u8");
	Expect(reader.offset() == 1, "reader offset should advance after u8");
	Expect(reader.readU16LE(u16) == iggy::runtime::RuntimeBinaryReadStatus::Ok, "reader should read u16");
	Expect(reader.readU32LE(u32) == iggy::runtime::RuntimeBinaryReadStatus::Ok, "reader should read u32");
	Expect(reader.readU64LE(u64) == iggy::runtime::RuntimeBinaryReadStatus::Ok, "reader should read u64");
	Expect(reader.readI32LE(i32) == iggy::runtime::RuntimeBinaryReadStatus::Ok, "reader should read i32");
	Expect(reader.readF32LE(f32) == iggy::runtime::RuntimeBinaryReadStatus::Ok, "reader should read f32");

	Expect(u8 == 0xAAU, "reader should preserve u8 value");
	Expect(u16 == 0x1234U, "reader should preserve u16 value");
	Expect(u32 == 0x89ABCDEFU, "reader should preserve u32 value");
	Expect(u64 == 0x0123456789ABCDEFULL, "reader should preserve u64 value");
	Expect(i32 == -123456, "reader should preserve i32 value");
	Expect(Near(f32, -2.5F), "reader should preserve f32 value");
	Expect(reader.remaining() == 0, "reader should consume all bytes");
}

void TestTruncatedReadsDoNotModifyOutputOrOffset()
{
	const std::vector<std::uint8_t> bytes { 0x34, 0x12, 0xAA };
	iggy::runtime::RuntimeBinaryReader reader(bytes);
	std::uint16_t u16 = 0;
	std::uint32_t u32 = 0xDEADBEEFU;
	const std::size_t offsetBefore = 0;

	Expect(reader.readU16LE(u16) == iggy::runtime::RuntimeBinaryReadStatus::Ok, "setup should read u16");
	Expect(u16 == 0x1234U, "setup should read expected u16");
	const std::size_t failingOffset = reader.offset();

	Expect(reader.readU32LE(u32) == iggy::runtime::RuntimeBinaryReadStatus::OutOfBytes, "truncated u32 should fail");
	Expect(u32 == 0xDEADBEEFU, "truncated u32 should not modify output");
	Expect(reader.offset() == failingOffset, "truncated u32 should not advance offset");
	Expect(failingOffset != offsetBefore, "setup should have advanced offset before failure");
}

void TestTruncatedBoolAndF32DoNotModifyOutputOrOffset()
{
	const std::vector<std::uint8_t> empty;
	iggy::runtime::RuntimeBinaryReader boolReader(empty);
	bool boolValue = true;
	Expect(boolReader.readBool(boolValue) == iggy::runtime::RuntimeBinaryReadStatus::OutOfBytes, "truncated bool should fail");
	Expect(boolValue, "truncated bool should not modify output");
	Expect(boolReader.offset() == 0, "truncated bool should not advance offset");

	const std::vector<std::uint8_t> shortFloat { 0x00, 0x00, 0x80 };
	iggy::runtime::RuntimeBinaryReader floatReader(shortFloat);
	float floatValue = 7.0F;
	Expect(floatReader.readF32LE(floatValue) == iggy::runtime::RuntimeBinaryReadStatus::OutOfBytes, "truncated f32 should fail");
	Expect(Near(floatValue, 7.0F), "truncated f32 should not modify output");
	Expect(floatReader.offset() == 0, "truncated f32 should not advance offset");
}

void TestReadBytesBehavior()
{
	const std::vector<std::uint8_t> bytes { 1, 2, 3, 4 };
	iggy::runtime::RuntimeBinaryReader reader(bytes);
	std::vector<std::uint8_t> out { 9 };

	Expect(reader.readBytes(2, out) == iggy::runtime::RuntimeBinaryReadStatus::Ok, "readBytes should read requested bytes");
	ExpectBytes(out, { 1, 2 }, "readBytes should replace output with requested bytes");
	Expect(reader.offset() == 2 && reader.remaining() == 2, "readBytes should advance offset");

	Expect(reader.readBytes(3, out) == iggy::runtime::RuntimeBinaryReadStatus::OutOfBytes, "truncated readBytes should fail");
	ExpectBytes(out, { 1, 2 }, "truncated readBytes should not modify output");
	Expect(reader.offset() == 2, "truncated readBytes should not advance offset");

	Expect(reader.readBytes(0, out) == iggy::runtime::RuntimeBinaryReadStatus::Ok, "readBytes zero count should succeed");
	Expect(out.empty(), "readBytes zero count should replace output with empty bytes");
	Expect(reader.offset() == 2, "readBytes zero count should not advance offset");
}

void TestWriterAppendOrderAcrossMixedWrites()
{
	iggy::runtime::RuntimeBinaryWriter writer;
	writer.writeU8(0xABU);
	writer.writeBool(true);
	writer.writeU16LE(0x1234U);
	writer.writeBytes({ 0x55, 0x66 });
	writer.writeBool(false);

	ExpectBytes(writer.bytes(), { 0xAB, 0x01, 0x34, 0x12, 0x55, 0x66, 0x00 }, "writer should preserve mixed append order");
}

void TestTakeBytesReturnsCopy()
{
	iggy::runtime::RuntimeBinaryWriter writer;
	writer.writeBytes({ 1, 2, 3 });
	std::vector<std::uint8_t> copy = writer.takeBytes();
	copy[0] = 9;

	ExpectBytes(writer.bytes(), { 1, 2, 3 }, "takeBytes should not mutate writer bytes");
	ExpectBytes(copy, { 9, 2, 3 }, "takeBytes caller copy should be independently mutable");
}

void TestWriterReaderRoundTrip()
{
	iggy::runtime::RuntimeBinaryWriter writer;
	writer.writeU8(7);
	writer.writeBool(true);
	writer.writeU16LE(0xABCDU);
	writer.writeU32LE(0x12345678U);
	writer.writeU64LE(0x0123456789ABCDEFULL);
	writer.writeI32LE(-77);
	writer.writeF32LE(3.5F);
	writer.writeBytes({ 4, 5, 6 });

	iggy::runtime::RuntimeBinaryReader reader(writer.bytes());
	std::uint8_t u8 = 0;
	bool flag = false;
	std::uint16_t u16 = 0;
	std::uint32_t u32 = 0;
	std::uint64_t u64 = 0;
	std::int32_t i32 = 0;
	float f32 = 0.0F;
	std::vector<std::uint8_t> bytes;

	Expect(reader.readU8(u8) == iggy::runtime::RuntimeBinaryReadStatus::Ok, "round trip should read u8");
	Expect(reader.readBool(flag) == iggy::runtime::RuntimeBinaryReadStatus::Ok, "round trip should read bool");
	Expect(reader.readU16LE(u16) == iggy::runtime::RuntimeBinaryReadStatus::Ok, "round trip should read u16");
	Expect(reader.readU32LE(u32) == iggy::runtime::RuntimeBinaryReadStatus::Ok, "round trip should read u32");
	Expect(reader.readU64LE(u64) == iggy::runtime::RuntimeBinaryReadStatus::Ok, "round trip should read u64");
	Expect(reader.readI32LE(i32) == iggy::runtime::RuntimeBinaryReadStatus::Ok, "round trip should read i32");
	Expect(reader.readF32LE(f32) == iggy::runtime::RuntimeBinaryReadStatus::Ok, "round trip should read f32");
	Expect(reader.readBytes(3, bytes) == iggy::runtime::RuntimeBinaryReadStatus::Ok, "round trip should read bytes");

	Expect(u8 == 7, "round trip should preserve u8");
	Expect(flag, "round trip should preserve bool");
	Expect(u16 == 0xABCDU, "round trip should preserve u16");
	Expect(u32 == 0x12345678U, "round trip should preserve u32");
	Expect(u64 == 0x0123456789ABCDEFULL, "round trip should preserve u64");
	Expect(i32 == -77, "round trip should preserve i32");
	Expect(Near(f32, 3.5F), "round trip should preserve f32");
	ExpectBytes(bytes, { 4, 5, 6 }, "round trip should preserve byte payload");
	Expect(reader.remaining() == 0, "round trip should consume all bytes");
}

} // namespace

int main()
{
	TestIntegerWritesUseLittleEndianOrder();
	TestNegativeI32WritesTwosComplementLittleEndian();
	TestF32WritesKnownIeeeBytes();
	TestBoolWritesAndReads();
	TestReaderReadsValuesAndTracksOffset();
	TestTruncatedReadsDoNotModifyOutputOrOffset();
	TestTruncatedBoolAndF32DoNotModifyOutputOrOffset();
	TestReadBytesBehavior();
	TestWriterAppendOrderAcrossMixedWrites();
	TestTakeBytesReturnsCopy();
	TestWriterReaderRoundTrip();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
