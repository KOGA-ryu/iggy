# 01. Input To Intent

This layer answers one question: how does a device event become a gameplay
movement request?

The important design choice is that raw input stays at the app edge. Mouse
clicks, touch taps, and hotkeys are translated into semantic commands before the
simulation sees them.

```text
RawInputEvent
  -> InputEventMatcher
  -> RuntimeInputRouter
  -> RuntimeSessionInputRouter
  -> RuntimeMovementInputRouter
  -> RuntimeMovementInputContextBuilder
  -> RuntimeTargetInputRouter
  -> RuntimeMovementIntentInputStep
  -> InputMapper
  -> PlayerIntent
  -> IntentCommandBuilder
  -> MovementCommandSource
```

## What Each Boundary Owns

`InputEventMatcher` recognizes boring device shapes: pressed key, pressed mouse
button, pressed touch point. It does not decide gameplay meaning.

`RuntimeSessionInputRouter` gets first chance at lifecycle hotkeys. Pause and
inventory mode changes are session commands, not movement commands.

`RuntimeMovementInputContextBuilder` builds the shared per-event movement
context. It selects the runtime player, resolves session mode into effective
focus, preserves action context, and computes the current movement block reason.

`RuntimeMovementInputRouter` orders movement input routes. Stop hotkeys and
target-aware pointer interactions get first chance before generic walk intent.

`RuntimeMovementIntentInputStep` handles generic movement-shaped input. It maps
the raw event to `PlayerIntent`, asks the action gate whether movement is
allowed, then queues a movement command.

`RuntimeRawInputDrainer` owns source consumption. It drains raw input sources
once, routes each event through the current context, and keeps blocked movement
reasons separate from handled command routes.

## Test Boundary

`input_routing_tests` covers this layer before the simulation is involved. The
tests assert that mouse clicks and touch taps become movement commands, that
session mode resolves into the effective input focus, that target-aware clicks
become interaction movement commands, and that blocked pointer input reports a
reason instead of silently disappearing.

`input_drain_tests` covers the frame-facing edge around those route decisions:
queued raw events are consumed once, nullable source lists are skipped safely,
and route outcomes become frame/run input report counts and block reasons.

## Why Focus Is Early

Inventory, menus, text entry, stun, and animation locks can all make the same
physical input mean "do not move." That decision belongs before command
creation. The simulation should receive either a valid command or a clear report
that movement input was blocked.

This is why the context builder computes the block reason once and later input
steps reuse it. It keeps stop, target interaction, generic walking, and blocked
pointer reporting aligned.

## Lesson

Controls feel simple when device input is boring and gameplay meaning is
explicit. The player presses a button; the app edge decides whether that button
means pause, inventory, stop, interact, walk, or blocked movement. After that,
the rest of the engine sees commands, not hardware.
