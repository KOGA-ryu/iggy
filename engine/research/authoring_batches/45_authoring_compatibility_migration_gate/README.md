# Batch 45: Authoring Compatibility/Migration Gate

## Goal
Decide whether v1 needs compatibility/migration helpers after source-plan version policy is explicit.

## Current State
Batch 21 tightens format/version compatibility. No migration framework should exist until needed.

## Slices
1. Review source-plan version policy and fixture history.
2. Identify whether any existing fixture/content would require migration.
3. Decide no-go, docs-only guidance, or a tiny one-version migration helper.
4. If approved, write an implementation packet with strict scope.

## Verification
Read-only unless docs are updated.

## Hard Stops
No broad migration framework, save/load migration, or TOML rewrite tool unless explicitly approved.

## Expected Result
Version compatibility policy has a follow-up path without prebuilding unnecessary machinery.
