# Lifecycle Replay

Lifecycle replay records session commands, not UI clicks or hotkeys. A replay
script says "start a new game", "save slot 2", "load slot 2", or "enter
inventory mode". It does not say which key, menu item, or controller button
created that intent.

That keeps live input, scripts, tests, and future networking on the same path:

```text
SessionCommand
  -> SessionCommandDispatcher
  -> SessionCommandApplier
  -> GameSession
  -> SessionEvent
```

## Runtime Path

`SessionCommandSource` is the runtime boundary for lifecycle intent. Menus,
debug tools, startup scripts, and live input can all produce `SessionCommand`
values.

`SessionCommandDispatcher` emits lifecycle events for the result. It delegates
the actual mutation to `SessionCommandApplier`.

`SessionCommandApplier` owns the command outcome rules:

```text
StartNewGame -> Applied + GameStarted
SaveSlot     -> Applied + SaveCompleted / Rejected + SaveFailed
LoadSlot     -> Applied + LoadCompleted / Rejected + LoadFailed
SetMode      -> Applied + ModeChanged / Rejected + ModeChangeRejected
```

This is the same lesson as movement commands: once intent becomes semantic, the
game should not care where it came from.

## Replay Path

Replay stores command history, then sends it through the same dispatcher:

```text
SessionCommandLog
  -> SessionCommandReplayer
  -> SessionCommandDispatcher
  -> SessionCommandApplier
  -> GameSession
  -> SessionEvent
```

That means replay can reproduce both success and failure. A script that tries to
load a missing slot is still a valid script; it produces a rejected command
result and a `LoadFailed` event.

## Byte Format Layers

Lifecycle command logs use a small byte stack:

```text
SessionCommandLog
  -> SessionCommandLogCodec
  -> SessionCommandLogFrameCodec
  -> SessionCommandPacketListCodec
  -> SessionCommandPacketByteCodec
  -> SessionCommandByteStream
```

`SessionCommandCodec` turns semantic commands into packet structs and back. It
also exposes packet byte encode/decode for lower layers.

`SessionCommandPacketValidator` rejects malformed packet shapes before they can
become semantic commands. For example, `SaveSlot` and `LoadSlot` must carry a
slot id, and `SetMode` must carry a valid session mode.

`SessionCommandPacketByteCodec` owns the fixed-width byte layout for one
lifecycle command packet. It uses little-endian primitive fields and rejects the
wrong byte size or invalid decoded packet.

`SessionCommandPacketListCodec` owns the counted packet-list payload. It writes
the packet count, appends fixed-size packet bytes, and rejects mismatched counts
or truncated packet lists.

`SessionCommandLogFrameCodec` owns the replay-file frame: magic bytes, format
version, packet-list payload, and checksum-protected validation.

`SessionCommandLogChecksum` owns replay-file integrity checks outside the full
log codec.

`SessionCommandByteStream` is the shared primitive byte boundary used by packet
bytes, packet lists, frames, and checksums.

## File And Script Layers

`SessionCommandLogFileStore` persists valid lifecycle command logs. It owns file
I/O, temp-file writes, missing-file handling, and corrupt-file rejection. It
does not apply commands.

`SessionScriptRunner` is the use-case layer over files and replay:

```text
script file
  -> SessionCommandLogFileStore
  -> SessionCommandLog
  -> SessionCommandReplayer
  -> SessionCommandDispatcher
  -> SessionCommandApplier
  -> GameSession
```

It distinguishes file/format failure from command rejection:

```text
LoadFailed  the script file was missing or invalid, so no command dispatched
Completed   the script loaded and every command produced an Applied/Rejected result
```

## Validation

Lifecycle replay validation happens at several boundaries:

```text
missing file                 -> SessionCommandLogFileStore rejects
bad magic/version/checksum   -> SessionCommandLogFrameCodec rejects
bad packet-list count/size   -> SessionCommandPacketListCodec rejects
wrong packet byte size       -> SessionCommandPacketByteCodec rejects
invalid packet shape         -> SessionCommandPacketValidator rejects
valid command, bad state     -> SessionCommandApplier rejects
```

The distinction matters. A corrupt startup script is a file or format problem.
A valid startup script that tries to save before starting a game is a lifecycle
rule problem.
