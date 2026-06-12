#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

#include "session/GameSession.hpp"
#include "session/SessionCommandApplier.hpp"
#include "session/SessionCommandByteStream.hpp"
#include "session/SessionCommandCodec.hpp"
#include "session/SessionCommandDispatcher.hpp"
#include "session/SessionCommandLog.hpp"
#include "session/SessionCommandLogChecksum.hpp"
#include "session/SessionCommandLogCodec.hpp"
#include "session/SessionCommandLogFileStore.hpp"
#include "session/SessionCommandLogFrameCodec.hpp"
#include "session/SessionCommandPacketByteCodec.hpp"
#include "session/SessionCommandPacketListCodec.hpp"
#include "session/SessionCommandPacketValidator.hpp"
#include "session/SessionCommandReplayer.hpp"
#include "session/SessionEventEmitter.hpp"
#include "session/SessionEventRecorder.hpp"
#include "session/SessionScriptRunner.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
}

void TestSessionCommandDispatcherAppliesLifecycleCommands()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_command_dispatch_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root };
	dev::SessionCommandDispatcher dispatcher { session };

	dev::SessionCommandResult start = dispatcher.dispatch({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 2, 6 }, .playerHitPoints = 15 },
	});
	Expect(start.type == dev::SessionCommandResultType::Applied, "session command should start new game");
	Expect(session.world().players.size() == 1 && session.world().players[0].position.tile == dev::Point { 2, 6 }, "new game command should apply settings");

	dev::SessionCommandResult save = dispatcher.dispatch({
	    .type = dev::SessionCommandType::SaveSlot,
	    .slotId = 1,
	});
	Expect(save.type == dev::SessionCommandResultType::Applied, "session command should save active slot");

	session.world().players[0].position.tile = { 9, 9 };
	dev::SessionCommandResult load = dispatcher.dispatch({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 1,
	});
	Expect(load.type == dev::SessionCommandResultType::Applied, "session command should load existing slot");
	Expect(session.world().players[0].position.tile == dev::Point { 2, 6 }, "load command should restore saved world");

	dev::SessionCommandResult pause = dispatcher.dispatch({
	    .type = dev::SessionCommandType::SetMode,
	    .mode = dev::GameSessionMode::Paused,
	});
	Expect(pause.type == dev::SessionCommandResultType::Applied, "session command should apply mode change");
	Expect(session.mode() == dev::GameSessionMode::Paused, "mode command should change session mode");

	std::filesystem::remove_all(root);
}

void TestSessionCommandDispatcherRejectsInvalidLifecycleCommands()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_command_reject_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root };
	dev::SessionCommandDispatcher dispatcher { session };

	dev::SessionCommandResult saveEmpty = dispatcher.dispatch({
	    .type = dev::SessionCommandType::SaveSlot,
	    .slotId = 1,
	});
	Expect(saveEmpty.type == dev::SessionCommandResultType::Rejected, "session command should reject saving empty session");

	dev::SessionCommandResult missingLoad = dispatcher.dispatch({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 99,
	});
	Expect(missingLoad.type == dev::SessionCommandResultType::Rejected, "session command should reject missing load slot");
	Expect(!session.hasActiveWorld(), "rejected missing load should leave empty session empty");

	dev::SessionCommandResult missingMode = dispatcher.dispatch({
	    .type = dev::SessionCommandType::SetMode,
	});
	Expect(missingMode.type == dev::SessionCommandResultType::Rejected, "session command should reject missing mode payload");

	std::filesystem::remove_all(root);
}

void TestSessionCommandApplierMapsLifecycleOutcomes()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_command_applier_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root };
	dev::SessionCommandApplier applier { session };

	dev::SessionCommandApplication start = applier.apply({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 3, 7 } },
	});
	Expect(start.result.type == dev::SessionCommandResultType::Applied, "session command applier should apply start command");
	Expect(start.eventType == dev::SessionEventType::GameStarted, "session command applier should map start to GameStarted");
	Expect(session.world().players.size() == 1 && session.world().players[0].position.tile == dev::Point { 3, 7 }, "session command applier should mutate session for start command");

	dev::SessionCommandApplication save = applier.apply({
	    .type = dev::SessionCommandType::SaveSlot,
	    .slotId = 1,
	});
	Expect(save.result.type == dev::SessionCommandResultType::Applied, "session command applier should apply save command");
	Expect(save.eventType == dev::SessionEventType::SaveCompleted, "session command applier should map successful save to SaveCompleted");

	session.world().players[0].position.tile = { 9, 9 };
	dev::SessionCommandApplication load = applier.apply({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 1,
	});
	Expect(load.result.type == dev::SessionCommandResultType::Applied, "session command applier should apply load command");
	Expect(load.eventType == dev::SessionEventType::LoadCompleted, "session command applier should map successful load to LoadCompleted");
	Expect(session.world().players[0].position.tile == dev::Point { 3, 7 }, "session command applier should restore saved world");

	dev::SessionCommandApplication mode = applier.apply({
	    .type = dev::SessionCommandType::SetMode,
	    .mode = dev::GameSessionMode::Inventory,
	});
	Expect(mode.result.type == dev::SessionCommandResultType::Applied, "session command applier should apply mode command");
	Expect(mode.eventType == dev::SessionEventType::ModeChanged, "session command applier should map successful mode change to ModeChanged");

	std::filesystem::remove_all(root);
}

void TestSessionCommandApplierMapsRejectedOutcomes()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_command_applier_reject_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root };
	dev::SessionCommandApplier applier { session };

	dev::SessionCommandApplication save = applier.apply({
	    .type = dev::SessionCommandType::SaveSlot,
	    .slotId = 1,
	});
	Expect(save.result.type == dev::SessionCommandResultType::Rejected, "session command applier should reject saving empty sessions");
	Expect(save.eventType == dev::SessionEventType::SaveFailed, "session command applier should map rejected save to SaveFailed");

	dev::SessionCommandApplication load = applier.apply({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 99,
	});
	Expect(load.result.type == dev::SessionCommandResultType::Rejected, "session command applier should reject missing load slots");
	Expect(load.eventType == dev::SessionEventType::LoadFailed, "session command applier should map rejected load to LoadFailed");

	dev::SessionCommandApplication mode = applier.apply({
	    .type = dev::SessionCommandType::SetMode,
	});
	Expect(mode.result.type == dev::SessionCommandResultType::Rejected, "session command applier should reject mode commands without payload");
	Expect(mode.eventType == dev::SessionEventType::ModeChangeRejected, "session command applier should map rejected mode to ModeChangeRejected");

	std::filesystem::remove_all(root);
}

void TestSessionEventEmitterBuildsLifecycleEvents()
{
	dev::SessionEventRecorder events;
	dev::SessionCommand command {
		.type = dev::SessionCommandType::LoadSlot,
		.slotId = 4,
		.mode = dev::GameSessionMode::Inventory,
	};

	dev::SessionEventEmitter { &events }.emit(command, dev::SessionEventType::LoadCompleted);
	dev::SessionEventEmitter {}.emit(command, dev::SessionEventType::LoadFailed);

	const std::vector<dev::SessionEvent> &recorded = events.events();
	Expect(recorded.size() == 1, "session event emitter should ignore missing event sinks");
	Expect(recorded.size() == 1 && recorded[0].type == dev::SessionEventType::LoadCompleted, "session event emitter should preserve event type");
	Expect(recorded.size() == 1 && recorded[0].commandType == dev::SessionCommandType::LoadSlot, "session event emitter should preserve command type");
	Expect(recorded.size() == 1 && recorded[0].slotId == std::optional<dev::SaveSlotId> { 4 }, "session event emitter should preserve slot id");
	Expect(recorded.size() == 1 && recorded[0].mode == std::optional<dev::GameSessionMode> { dev::GameSessionMode::Inventory }, "session event emitter should preserve mode payload");
}

void TestSessionCommandDispatcherEmitsSuccessEvents()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_event_success_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root };
	dev::SessionEventRecorder events;
	dev::SessionCommandDispatcher dispatcher { session, &events };

	(void)dispatcher.dispatch({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 1, 1 } },
	});
	(void)dispatcher.dispatch({
	    .type = dev::SessionCommandType::SaveSlot,
	    .slotId = 2,
	});
	(void)dispatcher.dispatch({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 2,
	});
	(void)dispatcher.dispatch({
	    .type = dev::SessionCommandType::SetMode,
	    .mode = dev::GameSessionMode::Inventory,
	});

	const std::vector<dev::SessionEvent> &recorded = events.events();
	Expect(recorded.size() == 4, "session dispatcher should emit success lifecycle events");
	Expect(recorded.size() == 4 && recorded[0].type == dev::SessionEventType::GameStarted, "start command should emit GameStarted");
	Expect(recorded.size() == 4 && recorded[1].type == dev::SessionEventType::SaveCompleted, "save command should emit SaveCompleted");
	Expect(recorded.size() == 4 && recorded[1].slotId == std::optional<dev::SaveSlotId> { 2 }, "save event should include slot id");
	Expect(recorded.size() == 4 && recorded[2].type == dev::SessionEventType::LoadCompleted, "load command should emit LoadCompleted");
	Expect(recorded.size() == 4 && recorded[3].type == dev::SessionEventType::ModeChanged, "mode command should emit ModeChanged");
	Expect(recorded.size() == 4 && recorded[3].mode == std::optional<dev::GameSessionMode> { dev::GameSessionMode::Inventory }, "mode event should include target mode");

	std::filesystem::remove_all(root);
}

void TestSessionCommandDispatcherEmitsFailureEvents()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_event_failure_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root };
	dev::SessionEventRecorder events;
	dev::SessionCommandDispatcher dispatcher { session, &events };

	(void)dispatcher.dispatch({
	    .type = dev::SessionCommandType::SaveSlot,
	    .slotId = 3,
	});
	(void)dispatcher.dispatch({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 99,
	});
	(void)dispatcher.dispatch({
	    .type = dev::SessionCommandType::SetMode,
	});

	const std::vector<dev::SessionEvent> &recorded = events.events();
	Expect(recorded.size() == 3, "session dispatcher should emit failure lifecycle events");
	Expect(recorded.size() == 3 && recorded[0].type == dev::SessionEventType::SaveFailed, "empty save command should emit SaveFailed");
	Expect(recorded.size() == 3 && recorded[0].slotId == std::optional<dev::SaveSlotId> { 3 }, "save failure should include slot id");
	Expect(recorded.size() == 3 && recorded[1].type == dev::SessionEventType::LoadFailed, "missing load command should emit LoadFailed");
	Expect(recorded.size() == 3 && recorded[1].slotId == std::optional<dev::SaveSlotId> { 99 }, "load failure should include slot id");
	Expect(recorded.size() == 3 && recorded[2].type == dev::SessionEventType::ModeChangeRejected, "missing mode command should emit ModeChangeRejected");

	std::filesystem::remove_all(root);
}

void TestSessionCommandReplayAppliesLifecycleSequence()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_replay_sequence_test";
	std::filesystem::remove_all(root);

	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 3, 4 }, .playerHitPoints = 12 },
	});
	log.record({
	    .type = dev::SessionCommandType::SaveSlot,
	    .slotId = 1,
	});
	log.record({
	    .type = dev::SessionCommandType::SetMode,
	    .mode = dev::GameSessionMode::Paused,
	});
	log.record({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 1,
	});

	dev::GameSession session { root };
	dev::SessionEventRecorder events;
	dev::SessionCommandDispatcher dispatcher { session, &events };
	dev::SessionCommandReplayer replayer { dispatcher };
	std::vector<dev::SessionCommandResult> results = replayer.replay(log);

	Expect(!log.empty(), "session command log should record commands");
	Expect(results.size() == 4, "session replay should return one result per command");
	Expect(results.size() == 4 && results[0].type == dev::SessionCommandResultType::Applied, "replay should apply start command");
	Expect(results.size() == 4 && results[1].type == dev::SessionCommandResultType::Applied, "replay should apply save command");
	Expect(results.size() == 4 && results[2].type == dev::SessionCommandResultType::Applied, "replay should apply mode command");
	Expect(results.size() == 4 && results[3].type == dev::SessionCommandResultType::Applied, "replay should apply load command");
	Expect(session.world().players.size() == 1 && session.world().players[0].position.tile == dev::Point { 3, 4 }, "session replay should restore final player position");
	Expect(session.world().players.size() == 1 && session.world().players[0].combatStats.hitPoints == 12, "session replay should restore final player hp");
	Expect(session.mode() == dev::GameSessionMode::Gameplay, "load command in replay should return session to gameplay");
	Expect(events.events().size() == 4, "session replay should emit lifecycle events");
	Expect(events.events().size() == 4 && events.events()[0].type == dev::SessionEventType::GameStarted, "session replay should emit start event");
	Expect(events.events().size() == 4 && events.events()[3].type == dev::SessionEventType::LoadCompleted, "session replay should emit load event");

	std::filesystem::remove_all(root);
}

void TestSessionCommandReplayReportsRejectedCommands()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_replay_reject_test";
	std::filesystem::remove_all(root);

	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 99,
	});
	log.record({
	    .type = dev::SessionCommandType::SetMode,
	});

	dev::GameSession session { root };
	dev::SessionEventRecorder events;
	dev::SessionCommandDispatcher dispatcher { session, &events };
	dev::SessionCommandReplayer replayer { dispatcher };
	std::vector<dev::SessionCommandResult> results = replayer.replay(log);

	Expect(results.size() == 2, "session replay should report rejected command results");
	Expect(results.size() == 2 && results[0].type == dev::SessionCommandResultType::Rejected, "session replay should reject missing load");
	Expect(results.size() == 2 && results[1].type == dev::SessionCommandResultType::Rejected, "session replay should reject malformed mode command");
	Expect(!session.hasActiveWorld(), "replayed rejected lifecycle commands should not create active world");
	Expect(events.events().size() == 2, "replayed rejected lifecycle commands should emit failure events");
	Expect(events.events().size() == 2 && events.events()[0].type == dev::SessionEventType::LoadFailed, "replayed missing load should emit LoadFailed");
	Expect(events.events().size() == 2 && events.events()[1].type == dev::SessionEventType::ModeChangeRejected, "replayed malformed mode should emit ModeChangeRejected");

	std::filesystem::remove_all(root);
}

void TestSessionCommandCodecRoundTripsCommands()
{
	dev::SessionCommandCodec codec;
	std::vector<dev::SessionCommand> commands {
		{
		    .type = dev::SessionCommandType::StartNewGame,
		    .newGameSettings = dev::NewGameSettings { .playerStart = { 6, 8 }, .playerHitPoints = 17 },
		},
		{
		    .type = dev::SessionCommandType::SaveSlot,
		    .slotId = 3,
		},
		{
		    .type = dev::SessionCommandType::LoadSlot,
		    .slotId = 4,
		},
		{
		    .type = dev::SessionCommandType::SetMode,
		    .mode = dev::GameSessionMode::Inventory,
		},
	};

	for (const dev::SessionCommand &command : commands) {
		dev::SessionCommandPacket packet = codec.toPacket(command);
		dev::SessionCommandBytes bytes = codec.encode(packet);
		std::optional<dev::SessionCommandPacket> decodedPacket = codec.decode(bytes);
		Expect(decodedPacket.has_value(), "session command packet should decode");
		std::optional<dev::SessionCommand> decoded = decodedPacket.has_value()
		    ? codec.fromPacket(*decodedPacket)
		    : std::nullopt;
		Expect(decoded.has_value(), "session command packet should become command");
		if (!decoded.has_value())
			continue;
		Expect(decoded->type == command.type, "session command codec should preserve command type");
		Expect(decoded->newGameSettings.has_value() == command.newGameSettings.has_value(), "session command codec should preserve new-game payload presence");
		Expect(decoded->slotId == command.slotId, "session command codec should preserve slot id");
		Expect(decoded->mode == command.mode, "session command codec should preserve mode");
		if (command.newGameSettings.has_value()) {
			Expect(decoded->newGameSettings->playerStart == command.newGameSettings->playerStart, "session command codec should preserve player start");
			Expect(decoded->newGameSettings->playerHitPoints == command.newGameSettings->playerHitPoints, "session command codec should preserve player hp");
		}
	}
}

void TestSessionCommandCodecRejectsInvalidPackets()
{
	dev::SessionCommandCodec codec;

	dev::SessionCommandPacket invalidType {
		.commandType = 99,
	};
	Expect(!codec.fromPacket(invalidType).has_value(), "session command codec should reject invalid command type");

	dev::SessionCommandPacket saveWithoutSlot {
		.commandType = static_cast<uint8_t>(dev::SessionCommandType::SaveSlot),
	};
	Expect(!codec.fromPacket(saveWithoutSlot).has_value(), "session command codec should reject save command without slot");

	dev::SessionCommandPacket modeWithoutPayload {
		.commandType = static_cast<uint8_t>(dev::SessionCommandType::SetMode),
	};
	Expect(!codec.fromPacket(modeWithoutPayload).has_value(), "session command codec should reject mode command without mode payload");

	dev::SessionCommandPacket invalidMode {
		.commandType = static_cast<uint8_t>(dev::SessionCommandType::SetMode),
		.hasMode = 1,
		.mode = 99,
	};
	Expect(!codec.fromPacket(invalidMode).has_value(), "session command codec should reject invalid mode");

	dev::SessionCommandPacket extraPayload {
		.commandType = static_cast<uint8_t>(dev::SessionCommandType::LoadSlot),
		.hasNewGameSettings = 1,
		.hasSlotId = 1,
		.slotId = 5,
	};
	Expect(!codec.fromPacket(extraPayload).has_value(), "session command codec should reject unexpected payload fields");

	dev::SessionCommandBytes shortBytes = codec.encode(codec.toPacket({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 1,
	}));
	shortBytes.pop_back();
	Expect(!codec.decode(shortBytes).has_value(), "session command codec should reject wrong byte size");
}

void TestSessionCommandPacketValidatorRejectsMalformedPayloads()
{
	dev::SessionCommandPacketValidator validator;

	Expect(validator.isValid({
	           .commandType = static_cast<uint8_t>(dev::SessionCommandType::StartNewGame),
	           .hasNewGameSettings = 1,
	           .playerStartX = 2,
	           .playerStartY = 3,
	           .playerHitPoints = 14,
	       }),
	    "session command packet validator should accept valid new-game packets");
	Expect(validator.isValid({
	           .commandType = static_cast<uint8_t>(dev::SessionCommandType::SaveSlot),
	           .hasSlotId = 1,
	           .slotId = 7,
	       }),
	    "session command packet validator should accept valid slot packets");
	Expect(!validator.isValid({
	           .commandType = static_cast<uint8_t>(dev::SessionCommandType::SaveSlot),
	           .hasSlotId = 2,
	           .slotId = 7,
	       }),
	    "session command packet validator should reject non-boolean payload flags");
	Expect(!validator.isValid({
	           .commandType = static_cast<uint8_t>(dev::SessionCommandType::SetMode),
	           .hasMode = 1,
	           .mode = 99,
	       }),
	    "session command packet validator should reject invalid modes");
	Expect(!validator.isValid({
	           .commandType = static_cast<uint8_t>(dev::SessionCommandType::LoadSlot),
	           .hasNewGameSettings = 1,
	           .hasSlotId = 1,
	           .slotId = 3,
	       }),
	    "session command packet validator should reject unexpected payload fields");
}

void TestSessionCommandByteStreamWritesLittleEndianPrimitives()
{
	std::vector<uint8_t> bytes;
	dev::SessionCommandByteWriter writer { bytes };
	writer.writeU8(0xABU);
	writer.writeU16(0x1234U);
	writer.writeU32(0xABCDEF12U);

	Expect(bytes.size() == 7, "session command byte writer should append primitive bytes");
	Expect(bytes.size() == 7 && bytes[1] == 0x34U && bytes[2] == 0x12U, "session command byte writer should write u16 little-endian");
	Expect(bytes.size() == 7 && bytes[3] == 0x12U && bytes[4] == 0xEFU && bytes[5] == 0xCDU && bytes[6] == 0xABU, "session command byte writer should write u32 little-endian");

	dev::SessionCommandByteReader reader { bytes };
	uint8_t byte = 0;
	uint16_t shortValue = 0;
	uint32_t wordValue = 0;
	Expect(reader.readU8(byte) && byte == 0xABU, "session command byte reader should read u8");
	Expect(reader.readU16(shortValue) && shortValue == 0x1234U, "session command byte reader should read u16");
	Expect(reader.readU32(wordValue) && wordValue == 0xABCDEF12U, "session command byte reader should read u32");
	Expect(reader.consumed(), "session command byte reader should report consumed bytes");

	dev::SessionCommandByteReader offsetReader { bytes, 3 };
	Expect(offsetReader.readU32(wordValue) && wordValue == 0xABCDEF12U, "session command byte reader should read from a starting offset");
}

void TestSessionCommandByteStreamRejectsShortReads()
{
	std::vector<uint8_t> bytes { 1, 2, 3 };
	dev::SessionCommandByteReader reader { bytes };
	uint32_t wordValue = 0;
	uint8_t first = 0;

	Expect(!reader.readU32(wordValue), "session command byte reader should reject short u32 reads");
	Expect(reader.offset() == 0, "session command byte reader should not advance after failed u32 reads");
	Expect(reader.readU8(first) && first == 1, "session command byte reader should continue after failed reads");
}

void TestSessionCommandPacketByteCodecRoundTripsPackets()
{
	dev::SessionCommandPacketByteCodec codec;
	dev::SessionCommandPacket packet {
		.commandType = static_cast<uint8_t>(dev::SessionCommandType::SaveSlot),
		.hasSlotId = 1,
		.slotId = 0x01020304U,
	};

	dev::SessionCommandBytes bytes = codec.encode(packet);
	std::optional<dev::SessionCommandPacket> decoded = codec.decode(bytes);

	Expect(bytes.size() == 25, "session command packet byte codec should write fixed packet size");
	Expect(bytes.size() == 25 && bytes[9] == 0x04 && bytes[10] == 0x03 && bytes[11] == 0x02 && bytes[12] == 0x01, "session command packet byte codec should write slot id little-endian");
	Expect(decoded.has_value(), "session command packet byte codec should decode valid bytes");
	Expect(decoded.has_value() && decoded->commandType == packet.commandType, "session command packet byte codec should preserve command type");
	Expect(decoded.has_value() && decoded->hasSlotId == 1 && decoded->slotId == packet.slotId, "session command packet byte codec should preserve slot payload");
}

void TestSessionCommandPacketByteCodecRejectsInvalidBytes()
{
	dev::SessionCommandPacketByteCodec codec;
	dev::SessionCommandPacket packet {
		.commandType = static_cast<uint8_t>(dev::SessionCommandType::SetMode),
		.hasMode = 1,
		.mode = static_cast<uint8_t>(dev::GameSessionMode::Inventory),
	};

	dev::SessionCommandBytes shortBytes = codec.encode(packet);
	shortBytes.pop_back();
	Expect(!codec.decode(shortBytes).has_value(), "session command packet byte codec should reject wrong byte size");

	dev::SessionCommandBytes invalidPacketBytes = codec.encode(packet);
	invalidPacketBytes[14] = 99;
	Expect(!codec.decode(invalidPacketBytes).has_value(), "session command packet byte codec should reject invalid decoded packets");
}

void TestSessionCommandLogCodecRoundTripsAndReplays()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_log_codec_replay_test";
	std::filesystem::remove_all(root);

	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 10, 4 }, .playerHitPoints = 14 },
	});
	log.record({
	    .type = dev::SessionCommandType::SaveSlot,
	    .slotId = 5,
	});
	log.record({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 5,
	});

	dev::SessionCommandLogCodec codec;
	dev::SessionCommandLogBytes bytes = codec.encode(log);
	std::optional<dev::SessionCommandLog> decoded = codec.decode(bytes);
	Expect(decoded.has_value(), "session command log codec should decode its own bytes");
	Expect(decoded.has_value() && decoded->commands().size() == 3, "session command log codec should preserve command count");
	Expect(decoded.has_value() && decoded->commands()[0].newGameSettings->playerStart == dev::Point { 10, 4 }, "session command log codec should preserve new-game settings");
	Expect(decoded.has_value() && decoded->commands()[1].slotId == std::optional<dev::SaveSlotId> { 5 }, "session command log codec should preserve save slot");

	dev::GameSession session { root };
	dev::SessionCommandDispatcher dispatcher { session };
	dev::SessionCommandReplayer replayer { dispatcher };
	std::vector<dev::SessionCommandResult> results = decoded.has_value()
	    ? replayer.replay(*decoded)
	    : std::vector<dev::SessionCommandResult> {};

	Expect(results.size() == 3, "decoded session command log should replay");
	Expect(session.world().players.size() == 1 && session.world().players[0].position.tile == dev::Point { 10, 4 }, "decoded session command log should reproduce session state");
	Expect(session.world().players.size() == 1 && session.world().players[0].combatStats.hitPoints == 14, "decoded session command log should reproduce player hp");

	std::filesystem::remove_all(root);
}

void TestSessionCommandLogCodecRejectsInvalidBytes()
{
	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 1, 1 } },
	});

	dev::SessionCommandLogCodec codec;
	dev::SessionCommandLogBytes bytes = codec.encode(log);

	dev::SessionCommandLogBytes badMagic = bytes;
	badMagic[0] = 'X';
	Expect(!codec.decode(badMagic).has_value(), "session command log codec should reject bad magic");

	dev::SessionCommandLogBytes badVersion = bytes;
	badVersion[4] = 2;
	Expect(!codec.decode(badVersion).has_value(), "session command log codec should reject bad version");

	dev::SessionCommandLogBytes truncated = bytes;
	truncated.pop_back();
	Expect(!codec.decode(truncated).has_value(), "session command log codec should reject truncated bytes");

	dev::SessionCommandLogBytes corrupted = bytes;
	corrupted[12] ^= 0x01U;
	Expect(!codec.decode(corrupted).has_value(), "session command log codec should reject checksum mismatch");
}

void TestSessionCommandLogChecksumValidatesTrailingChecksum()
{
	dev::SessionCommandLogBytes bytes { 1, 2, 3, 4 };
	dev::SessionCommandLogChecksum checksum;
	const uint32_t expected = checksum.compute(bytes, bytes.size());

	checksum.appendTo(bytes);

	Expect(bytes.size() == 8, "session command log checksum should append four checksum bytes");
	Expect(checksum.hasValidTrailingChecksum(bytes, 4), "session command log checksum should validate appended checksum");
	Expect(expected == checksum.compute(bytes, 4), "session command log checksum should compute payload hash only");

	bytes[0] ^= 0xFFU;
	Expect(!checksum.hasValidTrailingChecksum(bytes, 4), "session command log checksum should reject mutated payload");
}

void TestSessionCommandPacketListCodecFramesPacketBytes()
{
	dev::SessionCommandPacketByteCodec packetCodec;
	std::vector<dev::SessionCommandBytes> packets {
		packetCodec.encode({
		    .commandType = static_cast<uint8_t>(dev::SessionCommandType::SaveSlot),
		    .hasSlotId = 1,
		    .slotId = 5,
		}),
		packetCodec.encode({
		    .commandType = static_cast<uint8_t>(dev::SessionCommandType::SetMode),
		    .hasMode = 1,
		    .mode = static_cast<uint8_t>(dev::GameSessionMode::Inventory),
		}),
	};

	dev::SessionCommandPacketListCodec codec;
	dev::SessionCommandLogBytes bytes = codec.encode(packets);
	std::optional<std::vector<dev::SessionCommandBytes>> decoded = codec.decode(bytes);

	Expect(bytes.size() == 54, "session command packet list codec should write count and packet bytes");
	Expect(bytes.size() == 54 && bytes[0] == 2 && bytes[1] == 0 && bytes[2] == 0 && bytes[3] == 0, "session command packet list codec should write count little-endian");
	Expect(decoded.has_value() && *decoded == packets, "session command packet list codec should restore packet bytes");
}

void TestSessionCommandPacketListCodecRejectsInvalidSizes()
{
	dev::SessionCommandPacketByteCodec packetCodec;
	std::vector<dev::SessionCommandBytes> packets {
		packetCodec.encode({
		    .commandType = static_cast<uint8_t>(dev::SessionCommandType::SaveSlot),
		    .hasSlotId = 1,
		    .slotId = 5,
		}),
	};

	dev::SessionCommandPacketListCodec codec;
	dev::SessionCommandLogBytes truncated = codec.encode(packets);
	truncated.pop_back();
	Expect(!codec.decode(truncated).has_value(), "session command packet list codec should reject truncated packet lists");

	dev::SessionCommandLogBytes wrongCount = codec.encode(packets);
	wrongCount[0] = 2;
	Expect(!codec.decode(wrongCount).has_value(), "session command packet list codec should reject mismatched packet counts");
}

void TestSessionCommandLogFrameCodecFramesPacketBytes()
{
	dev::SessionCommandPacketByteCodec packetCodec;
	std::vector<dev::SessionCommandBytes> packets {
		packetCodec.encode({
		    .commandType = static_cast<uint8_t>(dev::SessionCommandType::SaveSlot),
		    .hasSlotId = 1,
		    .slotId = 5,
		}),
		packetCodec.encode({
		    .commandType = static_cast<uint8_t>(dev::SessionCommandType::SetMode),
		    .hasMode = 1,
		    .mode = static_cast<uint8_t>(dev::GameSessionMode::Inventory),
		}),
	};

	dev::SessionCommandLogFrameCodec frameCodec;
	dev::SessionCommandLogBytes bytes = frameCodec.encode(packets);
	std::optional<std::vector<dev::SessionCommandBytes>> decoded = frameCodec.decode(bytes);

	Expect(bytes.size() == 66, "session command log frame codec should write header, packets, and checksum");
	Expect(bytes.size() == 66 && bytes[0] == 'I' && bytes[1] == 'S' && bytes[2] == 'C' && bytes[3] == 'L', "session command log frame codec should write magic");
	Expect(bytes.size() == 66 && bytes[4] == 1 && bytes[8] == 2, "session command log frame codec should write version and packet list count");
	Expect(decoded.has_value() && decoded->size() == 2, "session command log frame codec should restore packet count");
	Expect(decoded.has_value() && (*decoded)[0] == packets[0], "session command log frame codec should preserve first packet");
	Expect(decoded.has_value() && (*decoded)[1] == packets[1], "session command log frame codec should preserve second packet");
}

void TestSessionCommandLogFrameCodecRejectsInvalidFrames()
{
	dev::SessionCommandPacketByteCodec packetCodec;
	std::vector<dev::SessionCommandBytes> packets {
		packetCodec.encode({
		    .commandType = static_cast<uint8_t>(dev::SessionCommandType::SaveSlot),
		    .hasSlotId = 1,
		    .slotId = 5,
		}),
	};
	dev::SessionCommandLogFrameCodec frameCodec;
	dev::SessionCommandLogBytes bytes = frameCodec.encode(packets);

	dev::SessionCommandLogBytes badMagic = bytes;
	badMagic[0] = 'X';
	Expect(!frameCodec.decode(badMagic).has_value(), "session command log frame codec should reject checksum-protected bad magic");

	dev::SessionCommandLogBytes badVersion = bytes;
	badVersion.resize(badVersion.size() - 4U);
	badVersion[4] = 2;
	dev::SessionCommandLogChecksum {}.appendTo(badVersion);
	Expect(!frameCodec.decode(badVersion).has_value(), "session command log frame codec should reject unsupported version");

	dev::SessionCommandLogBytes wrongCount = bytes;
	wrongCount.resize(wrongCount.size() - 4U);
	wrongCount[8] = 2;
	dev::SessionCommandLogChecksum {}.appendTo(wrongCount);
	Expect(!frameCodec.decode(wrongCount).has_value(), "session command log frame codec should reject payload size mismatch");
}

void TestSessionCommandLogFileStoreSavesLoadsAndReplays()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_log_file_store_replay_test";
	const std::filesystem::path path = root / "boot.iscl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);
	std::filesystem::remove(path.string() + ".tmp");

	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 4, 11 }, .playerHitPoints = 16 },
	});
	log.record({
	    .type = dev::SessionCommandType::SetMode,
	    .mode = dev::GameSessionMode::Paused,
	});
	log.record({
	    .type = dev::SessionCommandType::SetMode,
	    .mode = dev::GameSessionMode::Gameplay,
	});

	dev::SessionCommandLogFileStore store;
	Expect(store.save(path, log), "session command log file store should save log bytes");
	std::optional<dev::SessionCommandLog> loaded = store.load(path);
	Expect(loaded.has_value(), "session command log file store should load saved log");
	Expect(loaded.has_value() && loaded->commands().size() == 3, "loaded session command log should preserve command count");

	dev::GameSession session { root / "saves" };
	dev::SessionCommandDispatcher dispatcher { session };
	dev::SessionCommandReplayer replayer { dispatcher };
	std::vector<dev::SessionCommandResult> results = loaded.has_value()
	    ? replayer.replay(*loaded)
	    : std::vector<dev::SessionCommandResult> {};

	Expect(results.size() == 3, "loaded session command log should replay");
	Expect(session.mode() == dev::GameSessionMode::Gameplay, "loaded session command log should reproduce final session mode");
	Expect(session.world().players.size() == 1 && session.world().players[0].position.tile == dev::Point { 4, 11 }, "loaded session command log should reproduce player start");
	Expect(session.world().players.size() == 1 && session.world().players[0].combatStats.hitPoints == 16, "loaded session command log should reproduce player hp");

	std::filesystem::remove_all(root);
}

void TestSessionCommandLogFileStoreRejectsCorruptAndMissingFiles()
{
	const std::filesystem::path path = std::filesystem::temp_directory_path() / "iggy_session_log_file_store_corrupt_test.iscl";
	const std::filesystem::path missingPath = std::filesystem::temp_directory_path() / "iggy_session_log_file_store_missing_test.iscl";
	std::filesystem::remove(path);
	std::filesystem::remove(missingPath);

	{
		std::ofstream output { path, std::ios::binary | std::ios::trunc };
		output << "not a session command log";
	}

	dev::SessionCommandLogFileStore store;
	Expect(!store.load(path).has_value(), "session command log file store should reject corrupt files");
	Expect(!store.load(missingPath).has_value(), "session command log file store should return empty for missing files");

	std::filesystem::remove(path);
}

void TestSessionScriptRunnerRunsSavedLifecycleScript()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_script_runner_test";
	const std::filesystem::path path = root / "script.iscl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::StartNewGame,
	    .newGameSettings = dev::NewGameSettings { .playerStart = { 12, 6 }, .playerHitPoints = 21 },
	});
	log.record({
	    .type = dev::SessionCommandType::SaveSlot,
	    .slotId = 2,
	});
	log.record({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 2,
	});

	dev::SessionCommandLogFileStore store;
	Expect(store.save(path, log), "session script runner test should create script file");

	dev::GameSession session { root / "saves" };
	dev::SessionEventRecorder events;
	dev::SessionCommandDispatcher dispatcher { session, &events };
	dev::SessionScriptRunner runner { dispatcher };
	dev::SessionScriptRunResult result = runner.run(path);

	Expect(result.status == dev::SessionScriptRunStatus::Completed, "session script runner should complete valid script files");
	Expect(result.commandResults.size() == 3, "session script runner should return per-command results");
	Expect(result.commandResults.size() == 3 && result.commandResults[0].type == dev::SessionCommandResultType::Applied, "session script runner should apply new-game command");
	Expect(result.commandResults.size() == 3 && result.commandResults[1].type == dev::SessionCommandResultType::Applied, "session script runner should apply save command");
	Expect(result.commandResults.size() == 3 && result.commandResults[2].type == dev::SessionCommandResultType::Applied, "session script runner should apply load command");
	Expect(session.world().players.size() == 1 && session.world().players[0].position.tile == dev::Point { 12, 6 }, "session script runner should reproduce player position");
	Expect(session.world().players.size() == 1 && session.world().players[0].combatStats.hitPoints == 21, "session script runner should reproduce player hp");
	Expect(events.events().size() == 3, "session script runner should still emit dispatcher events");

	std::filesystem::remove_all(root);
}

void TestSessionScriptRunnerReportsLoadFailureAndCommandRejectionSeparately()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_session_script_runner_failure_test";
	const std::filesystem::path path = root / "script.iscl";
	const std::filesystem::path missingPath = root / "missing.iscl";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::GameSession session { root / "saves" };
	dev::SessionCommandDispatcher dispatcher { session };
	dev::SessionScriptRunner runner { dispatcher };

	dev::SessionScriptRunResult missing = runner.run(missingPath);
	Expect(missing.status == dev::SessionScriptRunStatus::LoadFailed, "session script runner should report missing file load failure");
	Expect(missing.commandResults.empty(), "missing session script should not dispatch commands");

	dev::SessionCommandLog log;
	log.record({
	    .type = dev::SessionCommandType::LoadSlot,
	    .slotId = 99,
	});
	dev::SessionCommandLogFileStore store;
	Expect(store.save(path, log), "session script runner rejection test should create script file");

	dev::SessionScriptRunResult rejected = runner.run(path);
	Expect(rejected.status == dev::SessionScriptRunStatus::Completed, "session script runner should complete loadable scripts even when commands reject");
	Expect(rejected.commandResults.size() == 1, "session script runner should return rejected command result");
	Expect(rejected.commandResults.size() == 1 && rejected.commandResults[0].type == dev::SessionCommandResultType::Rejected, "session script runner should preserve command-level rejection");

	std::filesystem::remove_all(root);
}

} // namespace

int main()
{
	TestSessionCommandDispatcherAppliesLifecycleCommands();
	TestSessionCommandDispatcherRejectsInvalidLifecycleCommands();
	TestSessionCommandApplierMapsLifecycleOutcomes();
	TestSessionCommandApplierMapsRejectedOutcomes();
	TestSessionEventEmitterBuildsLifecycleEvents();
	TestSessionCommandDispatcherEmitsSuccessEvents();
	TestSessionCommandDispatcherEmitsFailureEvents();
	TestSessionCommandReplayAppliesLifecycleSequence();
	TestSessionCommandReplayReportsRejectedCommands();
	TestSessionCommandCodecRoundTripsCommands();
	TestSessionCommandCodecRejectsInvalidPackets();
	TestSessionCommandPacketValidatorRejectsMalformedPayloads();
	TestSessionCommandByteStreamWritesLittleEndianPrimitives();
	TestSessionCommandByteStreamRejectsShortReads();
	TestSessionCommandPacketByteCodecRoundTripsPackets();
	TestSessionCommandPacketByteCodecRejectsInvalidBytes();
	TestSessionCommandLogCodecRoundTripsAndReplays();
	TestSessionCommandLogCodecRejectsInvalidBytes();
	TestSessionCommandLogChecksumValidatesTrailingChecksum();
	TestSessionCommandPacketListCodecFramesPacketBytes();
	TestSessionCommandPacketListCodecRejectsInvalidSizes();
	TestSessionCommandLogFrameCodecFramesPacketBytes();
	TestSessionCommandLogFrameCodecRejectsInvalidFrames();
	TestSessionCommandLogFileStoreSavesLoadsAndReplays();
	TestSessionCommandLogFileStoreRejectsCorruptAndMissingFiles();
	TestSessionScriptRunnerRunsSavedLifecycleScript();
	TestSessionScriptRunnerReportsLoadFailureAndCommandRejectionSeparately();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
