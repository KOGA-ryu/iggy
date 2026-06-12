#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

#include "combat/Combatant.hpp"
#include "combat/CombatEventRecorder.hpp"
#include "enemies/Enemy.hpp"
#include "events/EventRecorder.hpp"
#include "items/Item.hpp"
#include "player/Player.hpp"
#include "save/SaveGameService.hpp"
#include "save/SaveSlotService.hpp"
#include "save/SnapshotByteStream.hpp"
#include "save/SnapshotCodec.hpp"
#include "save/SnapshotChecksum.hpp"
#include "save/SnapshotEntityCodec.hpp"
#include "save/SnapshotEnemyCodec.hpp"
#include "save/SnapshotFileStore.hpp"
#include "save/SnapshotFrameCodec.hpp"
#include "save/SnapshotPlayerCodec.hpp"
#include "save/SnapshotReader.hpp"
#include "save/SnapshotSchemaCodec.hpp"
#include "save/SnapshotVectorCodec.hpp"
#include "save/SnapshotWriter.hpp"
#include "simulation/SimulationWorld.hpp"
#include "targeting/Target.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
}

bool Near(float actual, float expected, float tolerance = 0.0001F)
{
	return std::fabs(actual - expected) <= tolerance;
}

dev::Player MakePlayer(dev::Point tile = { 0, 0 })
{
	dev::Player player;
	player.position.tile = tile;
	player.position.future = tile;
	player.position.previous = tile;
	player.position.precise = tile;
	return player;
}

dev::Enemy MakeEnemy(dev::Point tile)
{
	dev::Enemy enemy;
	enemy.id = 1;
	enemy.position.tile = tile;
	enemy.position.future = tile;
	enemy.position.previous = tile;
	enemy.position.precise = tile;
	return enemy;
}

void TestSimulationSnapshotRestoresDurableState()
{
	dev::SimulationWorld world;
	dev::EventRecorder movementEvents;
	dev::CombatEventRecorder combatEvents;
	world.movementEvents = &movementEvents;
	world.setCombatEventSink(&combatEvents);
	world.players.push_back(MakePlayer({ 2, 2 }));
	world.players[0].combatStats.hitPoints = 18;
	world.players[0].inventory.capacity = 3;
	world.players[0].inventory.items.push_back({ .id = 52, .tile = { 0, 0 }, .combatModifiers = { .defense = 1 } });
	world.players[0].inventory.equipment.weapon = dev::Item { .id = 53, .equipmentSlot = dev::EquipmentSlot::Weapon, .combatModifiers = { .attackPower = 2 } };
	world.enemies.push_back(MakeEnemy({ 4, 4 }));
	world.enemies[0].moveState = dev::EnemyMoveState::Attacking;
	world.enemies[0].stateTimerSeconds = 0.50F;
	world.items.push_back({ .id = 51, .tile = { 3, 2 } });
	dev::Target target { .type = dev::TargetType::Enemy, .id = 50, .tile = { 4, 4 } };
	world.combat.registry().add({
	    .target = target,
	    .stats = { .hitPoints = 7, .attackPower = 3, .defense = 1 },
	});
	world.targets.add(target);
	world.targets.add({ .type = dev::TargetType::Item, .id = 51, .tile = { 3, 2 } });
	movementEvents.emit({ .type = dev::MovementEventType::StepCommitted, .tile = { 2, 2 } });
	combatEvents.emit({ .type = dev::CombatEventType::Hit, .target = target, .damage = 2, .remainingHitPoints = 7 });

	dev::SimulationSnapshot snapshot = dev::SnapshotWriter {}.write(world);

	world.players[0].position.tile = { 9, 9 };
	world.players[0].combatStats.hitPoints = 1;
	world.players[0].inventory.items.clear();
	world.enemies.clear();
	world.items.clear();
	world.combat.registry().replaceAll({});
	world.targets.clear();
	dev::SnapshotReader {}.read(snapshot, world);

	const dev::Combatant *combatant = world.combat.registry().find(target);
	dev::Target restoredTarget = world.targets.resolveAtTile({ 4, 4 });
	dev::Target restoredItem = world.targets.resolveAtTile({ 3, 2 });
	Expect(world.players.size() == 1 && world.players[0].position.tile == dev::Point { 2, 2 }, "snapshot should restore player position");
	Expect(world.players.size() == 1 && world.players[0].combatStats.hitPoints == 18, "snapshot should restore player combat stats");
	Expect(world.players.size() == 1 && world.players[0].inventory.capacity == 3, "snapshot should restore player inventory capacity");
	Expect(world.players.size() == 1 && world.players[0].inventory.items.size() == 1 && world.players[0].inventory.items[0].id == 52, "snapshot should restore player inventory");
	Expect(world.players.size() == 1 && world.players[0].inventory.equipment.weapon.has_value() && world.players[0].inventory.equipment.weapon->id == 53, "snapshot should restore player equipment");
	Expect(world.players.size() == 1 && world.players[0].inventory.equipment.weapon.has_value() && world.players[0].inventory.equipment.weapon->combatModifiers.attackPower == 2, "snapshot should restore equipment combat modifiers");
	Expect(world.enemies.size() == 1 && world.enemies[0].position.tile == dev::Point { 4, 4 }, "snapshot should restore enemy position");
	Expect(world.enemies.size() == 1 && world.enemies[0].moveState == dev::EnemyMoveState::Attacking, "snapshot should restore enemy state");
	Expect(world.items.size() == 1 && world.items[0].tile == dev::Point { 3, 2 }, "snapshot should restore item state");
	Expect(combatant != nullptr && combatant->stats.hitPoints == 7, "snapshot should restore combat registry state");
	Expect(restoredTarget.type == dev::TargetType::Enemy && restoredTarget.id == 50, "snapshot should restore target registry enemy");
	Expect(restoredItem.type == dev::TargetType::Item && restoredItem.id == 51, "snapshot should restore target registry item");
	Expect(snapshot.players.size() == 1 && snapshot.enemies.size() == 1 && snapshot.items.size() == 1 && snapshot.combatants.size() == 1 && snapshot.targets.size() == 2, "snapshot should contain durable state only");
	Expect(!movementEvents.events().empty() && !combatEvents.events().empty(), "snapshot restore should not manage transient event history");
}

void TestSnapshotCodecRoundTripsVersionedBytes()
{
	dev::SimulationSnapshot snapshot;
	dev::Player player = MakePlayer({ 1, 2 });
	player.moveState = dev::PlayerMoveState::Pathing;
	player.path.pushStep({ 2, 2 });
	player.path.pushStep({ 3, 2 });
	player.destinationAction = {
	    dev::DestinationActionType::Attack,
	    { .type = dev::TargetType::Enemy, .id = 60, .tile = { 3, 2 } },
	    1,
	};
	player.movementModifiers.standGround = true;
	player.animationLock.active = true;
	player.animationLock.elapsedSeconds = 0.25F;
	player.animationLock.cancelAfterSeconds = 0.50F;
	player.combatStats.hitPoints = 11;
	player.inventory.capacity = 2;
	player.inventory.items.push_back({ .id = 62, .tile = { 0, 0 }, .combatModifiers = { .defense = 3 } });
	player.inventory.equipment.weapon = dev::Item { .id = 63, .equipmentSlot = dev::EquipmentSlot::Weapon, .combatModifiers = { .attackPower = 4 } };
	snapshot.players.push_back(player);

	dev::Enemy enemy = MakeEnemy({ 5, 5 });
	enemy.id = 60;
	enemy.moveState = dev::EnemyMoveState::Recovering;
	enemy.tuning.attackWindupSeconds = 0.75F;
	enemy.stateTimerSeconds = 0.25F;
	snapshot.enemies.push_back(enemy);
	snapshot.items.push_back({ .id = 61, .tile = { 6, 5 }, .equipmentSlot = dev::EquipmentSlot::Accessory, .combatModifiers = { .attackPower = 1, .defense = 1 } });
	snapshot.combatants.push_back({
	    .target = { .type = dev::TargetType::Enemy, .id = 60, .tile = { 5, 5 } },
	    .stats = { .hitPoints = 4, .attackPower = 7, .defense = 2 },
	});
	snapshot.targets.push_back({ .type = dev::TargetType::Enemy, .id = 60, .tile = { 5, 5 } });
	snapshot.targets.push_back({ .type = dev::TargetType::Object, .id = 61, .tile = { 6, 5 } });

	dev::SnapshotCodec codec;
	dev::SnapshotBytes bytes = codec.encode(snapshot);
	std::optional<dev::SimulationSnapshot> decoded = codec.decode(bytes);

	Expect(decoded.has_value(), "snapshot codec should decode its own bytes");
	if (!decoded.has_value())
		return;

	Expect(decoded->players.size() == 1, "snapshot codec should preserve player count");
	Expect(decoded->players.size() == 1 && decoded->players[0].position.tile == dev::Point { 1, 2 }, "snapshot codec should preserve player position");
	Expect(decoded->players.size() == 1 && decoded->players[0].path.size() == 2, "snapshot codec should preserve player path length");
	Expect(decoded->players.size() == 1 && decoded->players[0].path.peekNext() == std::optional<dev::Point> { { 2, 2 } }, "snapshot codec should preserve next path step");
	Expect(decoded->players.size() == 1 && decoded->players[0].destinationAction.target.id == 60, "snapshot codec should preserve destination action target");
	Expect(decoded->players.size() == 1 && decoded->players[0].animationLock.active, "snapshot codec should preserve animation lock");
	Expect(decoded->players.size() == 1 && decoded->players[0].inventory.capacity == 2, "snapshot codec should preserve player inventory capacity");
	Expect(decoded->players.size() == 1 && decoded->players[0].inventory.items.size() == 1 && decoded->players[0].inventory.items[0].id == 62, "snapshot codec should preserve player inventory");
	Expect(decoded->players.size() == 1 && decoded->players[0].inventory.items.size() == 1 && decoded->players[0].inventory.items[0].combatModifiers.defense == 3, "snapshot codec should preserve inventory item combat modifiers");
	Expect(decoded->players.size() == 1 && decoded->players[0].inventory.equipment.weapon.has_value() && decoded->players[0].inventory.equipment.weapon->id == 63, "snapshot codec should preserve player equipment");
	Expect(decoded->players.size() == 1 && decoded->players[0].inventory.equipment.weapon.has_value() && decoded->players[0].inventory.equipment.weapon->combatModifiers.attackPower == 4, "snapshot codec should preserve equipment combat modifiers");
	Expect(decoded->enemies.size() == 1 && decoded->enemies[0].moveState == dev::EnemyMoveState::Recovering, "snapshot codec should preserve enemy state");
	Expect(decoded->items.size() == 1 && decoded->items[0].tile == dev::Point { 6, 5 }, "snapshot codec should preserve item state");
	Expect(decoded->items.size() == 1 && decoded->items[0].equipmentSlot == std::optional<dev::EquipmentSlot> { dev::EquipmentSlot::Accessory }, "snapshot codec should preserve floor item equipment slot");
	Expect(decoded->items.size() == 1 && decoded->items[0].combatModifiers.attackPower == 1, "snapshot codec should preserve floor item combat modifiers");
	Expect(decoded->combatants.size() == 1 && decoded->combatants[0].stats.hitPoints == 4, "snapshot codec should preserve combatants");
	Expect(decoded->targets.size() == 2 && decoded->targets[0].type == dev::TargetType::Enemy, "snapshot codec should preserve target registry target type");
	Expect(decoded->targets.size() == 2 && decoded->targets[1].id == 61, "snapshot codec should preserve target registry target id");
}

void TestSnapshotCodecRejectsInvalidBytes()
{
	dev::SnapshotCodec codec;
	dev::SimulationSnapshot snapshot;
	snapshot.players.push_back(MakePlayer());
	dev::SnapshotBytes bytes = codec.encode(snapshot);

	dev::SnapshotBytes badMagic = bytes;
	badMagic[0] = 'X';
	Expect(!codec.decode(badMagic).has_value(), "snapshot codec should reject bad magic");

	dev::SnapshotBytes badVersion = bytes;
	badVersion[4] = 8;
	Expect(!codec.decode(badVersion).has_value(), "snapshot codec should reject unsupported version");

	dev::SnapshotBytes truncated = bytes;
	truncated.pop_back();
	Expect(!codec.decode(truncated).has_value(), "snapshot codec should reject truncated data");

	dev::SnapshotBytes corruptedPayload = bytes;
	corruptedPayload[12] ^= 0x01U;
	Expect(!codec.decode(corruptedPayload).has_value(), "snapshot codec should reject checksum mismatch");
}

void TestSnapshotChecksumValidatesTrailingChecksum()
{
	dev::SnapshotBytes bytes { 1, 2, 3, 4 };
	dev::SnapshotChecksum checksum;
	const uint32_t expected = checksum.compute(bytes, bytes.size());

	checksum.appendTo(bytes);

	Expect(bytes.size() == 8, "snapshot checksum should append four checksum bytes");
	Expect(checksum.hasValidTrailingChecksum(bytes, 4), "snapshot checksum should validate appended checksum");
	Expect(expected == checksum.compute(bytes, 4), "snapshot checksum should compute payload hash only");

	bytes[0] ^= 0xFFU;
	Expect(!checksum.hasValidTrailingChecksum(bytes, 4), "snapshot checksum should reject mutated payload");
}

void TestSnapshotByteStreamWritesLittleEndianPrimitives()
{
	dev::SnapshotBytes bytes;
	dev::SnapshotByteWriter writer { bytes };
	writer.writeU8(0xABU);
	writer.writeU32(0x12345678U);
	writer.writeI32(-2);
	writer.writeFloat(1.5F);

	Expect(bytes.size() == 13, "snapshot byte writer should append primitive bytes");
	Expect(bytes.size() == 13 && bytes[1] == 0x78U && bytes[2] == 0x56U && bytes[3] == 0x34U && bytes[4] == 0x12U, "snapshot byte writer should write uint32 little-endian");

	dev::SnapshotByteReader reader { bytes };
	uint8_t byte = 0;
	uint32_t unsignedValue = 0;
	int signedValue = 0;
	float floatValue = 0.0F;
	Expect(reader.readU8(byte) && byte == 0xABU, "snapshot byte reader should read u8");
	Expect(reader.readU32(unsignedValue) && unsignedValue == 0x12345678U, "snapshot byte reader should read u32");
	Expect(reader.readI32(signedValue) && signedValue == -2, "snapshot byte reader should read i32");
	Expect(reader.readFloat(floatValue) && Near(floatValue, 1.5F), "snapshot byte reader should read float");
	Expect(reader.consumed(), "snapshot byte reader should report consumed bytes");

	dev::SnapshotByteReader offsetReader { bytes, 1 };
	Expect(offsetReader.readU32(unsignedValue) && unsignedValue == 0x12345678U, "snapshot byte reader should read from a starting offset");
}

void TestSnapshotByteStreamRejectsShortReads()
{
	dev::SnapshotBytes bytes { 1, 2, 3 };
	dev::SnapshotByteReader reader { bytes };
	uint32_t value = 0;
	uint8_t first = 0;

	Expect(!reader.readU32(value), "snapshot byte reader should reject short u32 reads");
	Expect(reader.offset() == 0, "snapshot byte reader should not advance after failed reads");
	Expect(reader.readU8(first) && first == 1, "snapshot byte reader should continue after failed reads");
}

void TestSnapshotEntityCodecRoundTripsItemAndCombatant()
{
	dev::SnapshotBytes bytes;
	dev::SnapshotByteWriter writer { bytes };
	dev::SnapshotEntityCodec codec;
	dev::Item item {
	    .id = 91,
	    .tile = { 3, 4 },
	    .equipmentSlot = dev::EquipmentSlot::Accessory,
	    .combatModifiers = { .attackPower = 2, .defense = 5 },
	};
	dev::Combatant combatant {
	    .target = { .type = dev::TargetType::Enemy, .id = 92, .tile = { 5, 6 } },
	    .stats = { .hitPoints = 7, .attackPower = 8, .defense = 9 },
	};
	codec.writeItem(writer, item);
	codec.writeCombatant(writer, combatant);

	dev::SnapshotByteReader reader { bytes };
	dev::Item decodedItem;
	dev::Combatant decodedCombatant;
	Expect(codec.readItem(reader, decodedItem), "snapshot entity codec should read encoded item");
	Expect(codec.readCombatant(reader, decodedCombatant), "snapshot entity codec should read encoded combatant");
	Expect(decodedItem.id == 91 && decodedItem.tile == dev::Point { 3, 4 }, "snapshot entity codec should preserve item identity and tile");
	Expect(decodedItem.equipmentSlot == std::optional<dev::EquipmentSlot> { dev::EquipmentSlot::Accessory }, "snapshot entity codec should preserve item equipment slot");
	Expect(decodedItem.combatModifiers.attackPower == 2 && decodedItem.combatModifiers.defense == 5, "snapshot entity codec should preserve item combat modifiers");
	Expect(decodedCombatant.target.type == dev::TargetType::Enemy && decodedCombatant.target.id == 92 && decodedCombatant.target.tile == dev::Point { 5, 6 }, "snapshot entity codec should preserve combatant target");
	Expect(decodedCombatant.stats.hitPoints == 7 && decodedCombatant.stats.attackPower == 8 && decodedCombatant.stats.defense == 9, "snapshot entity codec should preserve combatant stats");
	Expect(reader.consumed(), "snapshot entity codec should consume encoded entity bytes");
}

void TestSnapshotEntityCodecRejectsInvalidEnums()
{
	dev::SnapshotEntityCodec codec;

	dev::SnapshotBytes badTargetBytes;
	dev::SnapshotByteWriter badTargetWriter { badTargetBytes };
	badTargetWriter.writeU8(static_cast<uint8_t>(dev::TargetType::Object) + 1U);
	badTargetWriter.writeU32(1);
	badTargetWriter.writeI32(0);
	badTargetWriter.writeI32(0);
	dev::SnapshotByteReader badTargetReader { badTargetBytes };
	dev::Target target;
	Expect(!codec.readTarget(badTargetReader, target), "snapshot entity codec should reject invalid target type");

	dev::SnapshotBytes badSlotBytes;
	dev::SnapshotByteWriter badSlotWriter { badSlotBytes };
	badSlotWriter.writeU32(2);
	badSlotWriter.writeI32(1);
	badSlotWriter.writeI32(1);
	badSlotWriter.writeU8(1);
	badSlotWriter.writeU8(static_cast<uint8_t>(dev::EquipmentSlot::Accessory) + 1U);
	badSlotWriter.writeI32(0);
	badSlotWriter.writeI32(0);
	dev::SnapshotByteReader badSlotReader { badSlotBytes };
	dev::Item item;
	Expect(!codec.readItem(badSlotReader, item), "snapshot entity codec should reject invalid equipment slot");

	dev::SnapshotBytes badActionBytes;
	dev::SnapshotByteWriter badActionWriter { badActionBytes };
	badActionWriter.writeU8(static_cast<uint8_t>(dev::DestinationActionType::Interact) + 1U);
	dev::SnapshotByteReader badActionReader { badActionBytes };
	dev::DestinationAction action;
	Expect(!codec.readDestinationAction(badActionReader, action), "snapshot entity codec should reject invalid destination action type");
}

void TestSnapshotPlayerCodecRoundTripsDurablePlayerState()
{
	dev::Player player = MakePlayer({ 4, 5 });
	player.position.future = { 5, 5 };
	player.position.previous = { 3, 5 };
	player.position.precise = { 4, 5 };
	player.moveState = dev::PlayerMoveState::Pathing;
	player.path.pushStep({ 5, 5 });
	player.path.pushStep({ 6, 5 });
	player.destinationAction = {
	    dev::DestinationActionType::Attack,
	    { .type = dev::TargetType::Enemy, .id = 72, .tile = { 6, 5 } },
	    1,
	};
	player.movementModifiers.standGround = true;
	player.animationLock.active = true;
	player.animationLock.elapsedSeconds = 0.25F;
	player.animationLock.cancelAfterSeconds = 0.75F;
	player.combatStats = { .hitPoints = 13, .attackPower = 8, .defense = 4 };
	player.inventory.capacity = 4;
	player.inventory.items.push_back({ .id = 73, .tile = { 2, 2 }, .combatModifiers = { .defense = 2 } });
	player.inventory.equipment.weapon = dev::Item { .id = 74, .equipmentSlot = dev::EquipmentSlot::Weapon, .combatModifiers = { .attackPower = 5 } };
	player.inventory.equipment.armor = dev::Item { .id = 75, .equipmentSlot = dev::EquipmentSlot::Armor, .combatModifiers = { .defense = 6 } };
	player.moveSpeedTilesPerSecond = 6.5F;

	dev::SnapshotBytes bytes;
	dev::SnapshotByteWriter writer { bytes };
	dev::SnapshotPlayerCodec codec;
	codec.writePlayer(writer, player);

	dev::SnapshotByteReader reader { bytes };
	dev::Player decoded;
	Expect(codec.readPlayer(reader, decoded), "snapshot player codec should read encoded player");
	Expect(decoded.position.tile == dev::Point { 4, 5 } && decoded.position.future == dev::Point { 5, 5 }, "snapshot player codec should preserve actor position");
	Expect(decoded.moveState == dev::PlayerMoveState::Pathing, "snapshot player codec should preserve move state");
	Expect(decoded.path.size() == 2 && decoded.path.peekNext() == std::optional<dev::Point> { { 5, 5 } }, "snapshot player codec should preserve path steps");
	Expect(decoded.destinationAction.type == dev::DestinationActionType::Attack && decoded.destinationAction.target.id == 72, "snapshot player codec should preserve destination action");
	Expect(decoded.movementModifiers.standGround, "snapshot player codec should preserve movement modifiers");
	Expect(decoded.animationLock.active && Near(decoded.animationLock.elapsedSeconds, 0.25F) && Near(decoded.animationLock.cancelAfterSeconds, 0.75F), "snapshot player codec should preserve animation lock");
	Expect(decoded.combatStats.hitPoints == 13 && decoded.combatStats.attackPower == 8 && decoded.combatStats.defense == 4, "snapshot player codec should preserve combat stats");
	Expect(decoded.inventory.capacity == 4 && decoded.inventory.items.size() == 1 && decoded.inventory.items[0].id == 73, "snapshot player codec should preserve inventory contents");
	Expect(decoded.inventory.equipment.weapon.has_value() && decoded.inventory.equipment.weapon->combatModifiers.attackPower == 5, "snapshot player codec should preserve weapon equipment");
	Expect(decoded.inventory.equipment.armor.has_value() && decoded.inventory.equipment.armor->combatModifiers.defense == 6, "snapshot player codec should preserve armor equipment");
	Expect(Near(decoded.moveSpeedTilesPerSecond, 6.5F), "snapshot player codec should preserve movement speed");
	Expect(reader.consumed(), "snapshot player codec should consume encoded player bytes");
}

void TestSnapshotPlayerCodecRejectsInvalidMoveState()
{
	dev::SnapshotBytes bytes;
	dev::SnapshotByteWriter writer { bytes };
	dev::SnapshotEntityCodec entityCodec;
	entityCodec.writeActorPosition(writer, dev::ActorPosition {});
	writer.writeU8(static_cast<uint8_t>(dev::PlayerMoveState::Acting) + 1U);

	dev::SnapshotByteReader reader { bytes };
	dev::Player player;
	Expect(!dev::SnapshotPlayerCodec {}.readPlayer(reader, player), "snapshot player codec should reject invalid move state");
}

void TestSnapshotEnemyCodecRoundTripsDurableEnemyState()
{
	dev::Enemy enemy = MakeEnemy({ 7, 8 });
	enemy.id = 81;
	enemy.position.future = { 8, 8 };
	enemy.position.previous = { 6, 8 };
	enemy.position.precise = { 7, 8 };
	enemy.moveState = dev::EnemyMoveState::Recovering;
	enemy.tuning.maxStepsPerTick = 2;
	enemy.tuning.attackRangeTiles = 3;
	enemy.tuning.attackWindupSeconds = 0.60F;
	enemy.tuning.attackRecoverySeconds = 0.90F;
	enemy.combatStats = { .hitPoints = 10, .attackPower = 11, .defense = 12 };
	enemy.stateTimerSeconds = 1.25F;

	dev::SnapshotBytes bytes;
	dev::SnapshotByteWriter writer { bytes };
	dev::SnapshotEnemyCodec codec;
	codec.writeEnemy(writer, enemy);

	dev::SnapshotByteReader reader { bytes };
	dev::Enemy decoded;
	Expect(codec.readEnemy(reader, decoded), "snapshot enemy codec should read encoded enemy");
	Expect(decoded.id == 81, "snapshot enemy codec should preserve enemy id");
	Expect(decoded.position.tile == dev::Point { 7, 8 } && decoded.position.future == dev::Point { 8, 8 }, "snapshot enemy codec should preserve actor position");
	Expect(decoded.moveState == dev::EnemyMoveState::Recovering, "snapshot enemy codec should preserve move state");
	Expect(decoded.tuning.maxStepsPerTick == 2 && decoded.tuning.attackRangeTiles == 3, "snapshot enemy codec should preserve integer tuning");
	Expect(Near(decoded.tuning.attackWindupSeconds, 0.60F) && Near(decoded.tuning.attackRecoverySeconds, 0.90F), "snapshot enemy codec should preserve timing tuning");
	Expect(decoded.combatStats.hitPoints == 10 && decoded.combatStats.attackPower == 11 && decoded.combatStats.defense == 12, "snapshot enemy codec should preserve combat stats");
	Expect(Near(decoded.stateTimerSeconds, 1.25F), "snapshot enemy codec should preserve state timer");
	Expect(reader.consumed(), "snapshot enemy codec should consume encoded enemy bytes");
}

void TestSnapshotEnemyCodecRejectsInvalidMoveState()
{
	dev::SnapshotBytes bytes;
	dev::SnapshotByteWriter writer { bytes };
	dev::SnapshotEntityCodec entityCodec;
	writer.writeU32(1);
	entityCodec.writeActorPosition(writer, dev::ActorPosition {});
	writer.writeU8(static_cast<uint8_t>(dev::EnemyMoveState::Recovering) + 1U);

	dev::SnapshotByteReader reader { bytes };
	dev::Enemy enemy;
	Expect(!dev::SnapshotEnemyCodec {}.readEnemy(reader, enemy), "snapshot enemy codec should reject invalid move state");
}

void TestSnapshotVectorCodecFramesCountedVectors()
{
	std::vector<int> values { 3, 4, 5 };
	dev::SnapshotBytes bytes;
	dev::SnapshotByteWriter writer { bytes };
	dev::SnapshotVectorCodec codec;
	codec.writeVector(writer, values, [](dev::SnapshotByteWriter &itemWriter, int value) {
		itemWriter.writeI32(value);
	});

	Expect(bytes.size() == 16, "snapshot vector codec should write count and item bytes");
	Expect(bytes.size() == 16 && bytes[0] == 3 && bytes[1] == 0 && bytes[2] == 0 && bytes[3] == 0, "snapshot vector codec should write count little-endian");

	std::vector<int> decoded;
	dev::SnapshotByteReader reader { bytes };
	Expect(codec.readVector(reader, decoded, [](dev::SnapshotByteReader &itemReader, int &value) {
		return itemReader.readI32(value);
	}), "snapshot vector codec should read encoded vectors");
	Expect(decoded == values, "snapshot vector codec should preserve vector items");
	Expect(reader.consumed(), "snapshot vector codec should consume encoded vector bytes");
}

void TestSnapshotVectorCodecRejectsTruncatedVectors()
{
	dev::SnapshotBytes bytes;
	dev::SnapshotByteWriter writer { bytes };
	writer.writeU32(2);
	writer.writeI32(10);

	dev::SnapshotVectorCodec codec;
	dev::SnapshotByteReader reader { bytes };
	std::vector<int> decoded;
	Expect(!codec.readVector(reader, decoded, [](dev::SnapshotByteReader &itemReader, int &value) {
		return itemReader.readI32(value);
	}), "snapshot vector codec should reject truncated item data");
}

void TestSnapshotSchemaCodecRoundTripsOrderedSections()
{
	dev::SimulationSnapshot snapshot;
	dev::Player player = MakePlayer({ 1, 2 });
	player.path.pushStep({ 2, 2 });
	player.inventory.capacity = 2;
	player.inventory.items.push_back({ .id = 101, .tile = { 3, 3 } });
	snapshot.players.push_back(player);
	dev::Enemy enemy = MakeEnemy({ 4, 4 });
	enemy.id = 102;
	enemy.tuning.attackRangeTiles = 2;
	snapshot.enemies.push_back(enemy);
	snapshot.items.push_back({ .id = 103, .tile = { 5, 5 }, .equipmentSlot = dev::EquipmentSlot::Armor, .combatModifiers = { .defense = 2 } });
	snapshot.combatants.push_back({
	    .target = { .type = dev::TargetType::Enemy, .id = 102, .tile = { 4, 4 } },
	    .stats = { .hitPoints = 6, .attackPower = 7, .defense = 8 },
	});
	snapshot.targets.push_back({ .type = dev::TargetType::Item, .id = 103, .tile = { 5, 5 } });

	dev::SnapshotBytes bytes;
	dev::SnapshotByteWriter writer { bytes };
	dev::SnapshotSchemaCodec codec;
	codec.writeSnapshot(writer, snapshot);

	dev::SnapshotByteReader reader { bytes };
	dev::SimulationSnapshot decoded;
	Expect(codec.readSnapshot(reader, decoded), "snapshot schema codec should read encoded snapshot sections");
	Expect(decoded.players.size() == 1 && decoded.players[0].position.tile == dev::Point { 1, 2 }, "snapshot schema codec should preserve player section");
	Expect(decoded.players.size() == 1 && decoded.players[0].path.size() == 1, "snapshot schema codec should preserve player path inside section");
	Expect(decoded.enemies.size() == 1 && decoded.enemies[0].id == 102 && decoded.enemies[0].tuning.attackRangeTiles == 2, "snapshot schema codec should preserve enemy section");
	Expect(decoded.items.size() == 1 && decoded.items[0].id == 103 && decoded.items[0].combatModifiers.defense == 2, "snapshot schema codec should preserve item section");
	Expect(decoded.combatants.size() == 1 && decoded.combatants[0].stats.attackPower == 7, "snapshot schema codec should preserve combatant section");
	Expect(decoded.targets.size() == 1 && decoded.targets[0].type == dev::TargetType::Item, "snapshot schema codec should preserve target section");
	Expect(reader.consumed(), "snapshot schema codec should consume all schema bytes");
}

void TestSnapshotSchemaCodecRejectsIncompleteOrTrailingPayload()
{
	dev::SnapshotSchemaCodec codec;

	dev::SnapshotBytes incompleteBytes;
	dev::SnapshotByteWriter incompleteWriter { incompleteBytes };
	incompleteWriter.writeU32(0);
	dev::SnapshotByteReader incompleteReader { incompleteBytes };
	dev::SimulationSnapshot incomplete;
	Expect(!codec.readSnapshot(incompleteReader, incomplete), "snapshot schema codec should reject missing sections");

	dev::SimulationSnapshot emptySnapshot;
	dev::SnapshotBytes trailingBytes;
	dev::SnapshotByteWriter trailingWriter { trailingBytes };
	codec.writeSnapshot(trailingWriter, emptySnapshot);
	trailingWriter.writeU8(0xFFU);
	dev::SnapshotByteReader trailingReader { trailingBytes };
	dev::SimulationSnapshot trailing;
	Expect(!codec.readSnapshot(trailingReader, trailing), "snapshot schema codec should reject trailing payload bytes");
}

void TestSnapshotFrameCodecFramesPayloadBytes()
{
	dev::SnapshotBytes payload { 10, 20, 30 };
	dev::SnapshotFrameCodec frameCodec;
	dev::SnapshotBytes bytes = frameCodec.encode(payload);
	std::optional<dev::SnapshotBytes> decoded = frameCodec.decode(bytes);

	Expect(bytes.size() == 15, "snapshot frame codec should write header, payload, and checksum");
	Expect(bytes.size() == 15 && bytes[0] == 'I' && bytes[1] == 'G' && bytes[2] == 'G' && bytes[3] == 'Y', "snapshot frame codec should write magic");
	Expect(bytes.size() == 15 && bytes[4] == 7 && bytes[5] == 0 && bytes[6] == 0 && bytes[7] == 0, "snapshot frame codec should write version little-endian");
	Expect(decoded.has_value() && *decoded == payload, "snapshot frame codec should restore payload bytes");
}

void TestSnapshotFrameCodecRejectsInvalidFrames()
{
	dev::SnapshotBytes payload { 10, 20, 30 };
	dev::SnapshotFrameCodec frameCodec;
	dev::SnapshotBytes bytes = frameCodec.encode(payload);

	dev::SnapshotBytes badMagic = bytes;
	badMagic.resize(badMagic.size() - 4U);
	badMagic[0] = 'X';
	dev::SnapshotChecksum {}.appendTo(badMagic);
	Expect(!frameCodec.decode(badMagic).has_value(), "snapshot frame codec should reject bad magic");

	dev::SnapshotBytes badVersion = bytes;
	badVersion.resize(badVersion.size() - 4U);
	badVersion[4] = 8;
	dev::SnapshotChecksum {}.appendTo(badVersion);
	Expect(!frameCodec.decode(badVersion).has_value(), "snapshot frame codec should reject unsupported version");

	dev::SnapshotBytes truncated = bytes;
	truncated.pop_back();
	Expect(!frameCodec.decode(truncated).has_value(), "snapshot frame codec should reject truncated frames");
}

void TestSnapshotFileStoreSavesAndLoadsVersionedBytes()
{
	const std::filesystem::path path = std::filesystem::temp_directory_path() / "iggy_snapshot_store_test.bin";
	std::filesystem::remove(path);
	std::filesystem::remove(path.string() + ".tmp");

	dev::SimulationSnapshot snapshot;
	dev::Player player = MakePlayer({ 6, 7 });
	player.combatStats.hitPoints = 13;
	snapshot.players.push_back(player);

	dev::SnapshotFileStore store;
	Expect(store.save(path, snapshot), "snapshot file store should save snapshot bytes");
	std::optional<dev::SimulationSnapshot> loaded = store.load(path);
	Expect(loaded.has_value(), "snapshot file store should load saved bytes");
	Expect(loaded.has_value() && loaded->players.size() == 1 && loaded->players[0].position.tile == dev::Point { 6, 7 }, "loaded snapshot should preserve player position");
	Expect(loaded.has_value() && loaded->players[0].combatStats.hitPoints == 13, "loaded snapshot should preserve player hp");

	std::filesystem::remove(path);
}

void TestSnapshotFileStoreRejectsCorruptFile()
{
	const std::filesystem::path path = std::filesystem::temp_directory_path() / "iggy_snapshot_store_corrupt_test.bin";
	std::filesystem::remove(path);

	{
		std::ofstream output { path, std::ios::binary | std::ios::trunc };
		output << "not a snapshot";
	}

	dev::SnapshotFileStore store;
	Expect(!store.load(path).has_value(), "snapshot file store should reject corrupt files");

	std::filesystem::remove(path);
}

void TestSaveGameServiceSavesAndLoadsWorld()
{
	const std::filesystem::path path = std::filesystem::temp_directory_path() / "iggy_save_game_service_test.bin";
	std::filesystem::remove(path);
	std::filesystem::remove(path.string() + ".tmp");

	dev::SimulationWorld world;
	dev::CombatEventRecorder combatEvents;
	world.setCombatEventSink(&combatEvents);
	world.players.push_back(MakePlayer({ 8, 2 }));
	world.players[0].combatStats.hitPoints = 17;
	world.players[0].inventory.items.push_back({ .id = 73, .tile = { 0, 0 } });
	dev::Target target { .type = dev::TargetType::Enemy, .id = 70, .tile = { 9, 2 } };
	world.combat.registry().add({
	    .target = target,
	    .stats = { .hitPoints = 6, .attackPower = 4, .defense = 1 },
	});
	world.items.push_back({ .id = 72, .tile = { 11, 2 } });
	world.targets.add(target);
	world.targets.add({ .type = dev::TargetType::Object, .id = 71, .tile = { 10, 2 } });
	world.targets.add({ .type = dev::TargetType::Item, .id = 72, .tile = { 11, 2 } });

	dev::SaveGameService saves;
	Expect(saves.saveWorld(path, world), "save game service should save world");

	world.players[0].position.tile = { 0, 0 };
	world.players[0].combatStats.hitPoints = 1;
	world.players[0].inventory.items.clear();
	world.items.clear();
	world.combat.registry().replaceAll({});
	world.targets.clear();
	Expect(saves.loadWorld(path, world), "save game service should load world");

	const dev::Combatant *combatant = world.combat.registry().find(target);
	dev::Target restoredEnemy = world.targets.resolveAtTile({ 9, 2 });
	dev::Target restoredObject = world.targets.resolveAtTile({ 10, 2 });
	dev::Target restoredItem = world.targets.resolveAtTile({ 11, 2 });
	Expect(world.players.size() == 1 && world.players[0].position.tile == dev::Point { 8, 2 }, "save game service should restore player position");
	Expect(world.players.size() == 1 && world.players[0].combatStats.hitPoints == 17, "save game service should restore player hp");
	Expect(world.players.size() == 1 && world.players[0].inventory.items.size() == 1 && world.players[0].inventory.items[0].id == 73, "save game service should restore player inventory");
	Expect(world.items.size() == 1 && world.items[0].id == 72, "save game service should restore item state");
	Expect(combatant != nullptr && combatant->stats.hitPoints == 6, "save game service should restore combat state");
	Expect(restoredEnemy.type == dev::TargetType::Enemy && restoredEnemy.id == 70, "save game service should restore enemy target");
	Expect(restoredObject.type == dev::TargetType::Object && restoredObject.id == 71, "save game service should restore object target");
	Expect(restoredItem.type == dev::TargetType::Item && restoredItem.id == 72, "save game service should restore item target");
	Expect(world.combatEvents == &combatEvents, "save game service should preserve world event sinks");

	std::filesystem::remove(path);
}

void TestSaveSlotServiceListsMetadata()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_save_slot_metadata_test";
	std::filesystem::remove_all(root);

	dev::SimulationWorld world;
	world.players.push_back(MakePlayer({ 3, 9 }));
	world.players[0].combatStats.hitPoints = 14;
	world.enemies.push_back(MakeEnemy({ 4, 9 }));
	world.items.push_back({ .id = 90, .tile = { 5, 9 } });
	world.targets.add({ .type = dev::TargetType::Item, .id = 90, .tile = { 5, 9 } });

	dev::SaveSlotService slots { root };
	Expect(slots.saveSlot(1, world), "save slot service should save occupied slot");

	{
		std::ofstream corrupt { slots.pathForSlot(2), std::ios::binary | std::ios::trunc };
		corrupt << "corrupt";
	}

	std::vector<dev::SaveSlotMetadata> listed = slots.listSlots(1, 3);
	Expect(listed.size() == 3, "save slot service should list requested slot count");
	Expect(listed.size() == 3 && listed[0].slotId == 1 && listed[0].occupied && listed[0].valid, "saved slot should be occupied and valid");
	Expect(listed.size() == 3 && listed[0].playerTile == dev::Point { 3, 9 }, "slot metadata should include player tile");
	Expect(listed.size() == 3 && listed[0].playerHitPoints == 14, "slot metadata should include player hp");
	Expect(listed.size() == 3 && listed[0].enemyCount == 1, "slot metadata should include enemy count");
	Expect(listed.size() == 3 && listed[1].slotId == 2 && listed[1].occupied && !listed[1].valid, "corrupt slot should be occupied but invalid");
	Expect(listed.size() == 3 && listed[2].slotId == 3 && !listed[2].occupied && !listed[2].valid, "missing slot should be empty and invalid");

	std::filesystem::remove_all(root);
}

void TestSaveSlotServiceLoadsWorld()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_save_slot_load_test";
	std::filesystem::remove_all(root);

	dev::SimulationWorld source;
	source.players.push_back(MakePlayer({ 7, 1 }));
	source.players[0].combatStats.hitPoints = 19;
	source.players[0].inventory.items.push_back({ .id = 82, .tile = { 0, 0 } });
	dev::Target target { .type = dev::TargetType::Enemy, .id = 80, .tile = { 8, 1 } };
	source.combat.registry().add({
	    .target = target,
	    .stats = { .hitPoints = 8, .attackPower = 5, .defense = 1 },
	});
	source.items.push_back({ .id = 81, .tile = { 9, 1 } });
	source.targets.add(target);
	source.targets.add({ .type = dev::TargetType::Item, .id = 81, .tile = { 9, 1 } });

	dev::SaveSlotService slots { root };
	Expect(slots.saveSlot(4, source), "save slot service should save loadable slot");

	dev::SimulationWorld loaded;
	dev::CombatEventRecorder combatEvents;
	loaded.setCombatEventSink(&combatEvents);
	Expect(slots.loadSlot(4, loaded), "save slot service should load saved slot");

	const dev::Combatant *combatant = loaded.combat.registry().find(target);
	dev::Target loadedTarget = loaded.targets.resolveAtTile({ 8, 1 });
	dev::Target loadedItemTarget = loaded.targets.resolveAtTile({ 9, 1 });
	Expect(loaded.players.size() == 1 && loaded.players[0].position.tile == dev::Point { 7, 1 }, "slot load should restore player tile");
	Expect(loaded.players.size() == 1 && loaded.players[0].combatStats.hitPoints == 19, "slot load should restore player hp");
	Expect(loaded.players.size() == 1 && loaded.players[0].inventory.items.size() == 1 && loaded.players[0].inventory.items[0].id == 82, "slot load should restore player inventory");
	Expect(loaded.items.size() == 1 && loaded.items[0].id == 81, "slot load should restore item state");
	Expect(combatant != nullptr && combatant->stats.hitPoints == 8, "slot load should restore combat state");
	Expect(loadedTarget.type == dev::TargetType::Enemy && loadedTarget.id == 80, "slot load should restore target registry");
	Expect(loadedItemTarget.type == dev::TargetType::Item && loadedItemTarget.id == 81, "slot load should restore item target");
	Expect(loaded.combatEvents == &combatEvents, "slot load should preserve world event sinks");

	std::filesystem::remove_all(root);
}


} // namespace

int main()
{
	TestSimulationSnapshotRestoresDurableState();
	TestSnapshotCodecRoundTripsVersionedBytes();
	TestSnapshotCodecRejectsInvalidBytes();
	TestSnapshotChecksumValidatesTrailingChecksum();
	TestSnapshotByteStreamWritesLittleEndianPrimitives();
	TestSnapshotByteStreamRejectsShortReads();
	TestSnapshotEntityCodecRoundTripsItemAndCombatant();
	TestSnapshotEntityCodecRejectsInvalidEnums();
	TestSnapshotPlayerCodecRoundTripsDurablePlayerState();
	TestSnapshotPlayerCodecRejectsInvalidMoveState();
	TestSnapshotEnemyCodecRoundTripsDurableEnemyState();
	TestSnapshotEnemyCodecRejectsInvalidMoveState();
	TestSnapshotVectorCodecFramesCountedVectors();
	TestSnapshotVectorCodecRejectsTruncatedVectors();
	TestSnapshotSchemaCodecRoundTripsOrderedSections();
	TestSnapshotSchemaCodecRejectsIncompleteOrTrailingPayload();
	TestSnapshotFrameCodecFramesPayloadBytes();
	TestSnapshotFrameCodecRejectsInvalidFrames();
	TestSnapshotFileStoreSavesAndLoadsVersionedBytes();
	TestSnapshotFileStoreRejectsCorruptFile();
	TestSaveGameServiceSavesAndLoadsWorld();
	TestSaveSlotServiceListsMetadata();
	TestSaveSlotServiceLoadsWorld();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
