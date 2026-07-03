

#pragma once

#include "app/iggy3d/creative/Mutation.hpp"
#include "app/iggy3d/creative/Object.hpp"
#include "app/iggy3d/creative/ObjectDescriptor.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace iggy3d::creative {

// MutationApply is the creative-mode verb executor.
//
// Mutation.hpp defines the reusable mutation vocabulary and payloads.
// ObjectDescriptor.hpp defines which object kinds allow which mutations and
// which systems become dirty when mutations land.
// MutationApply.hpp defines the generic application result and the shared
// mutation execution entry point.
//
// This file is the anti-copy-paste layer:
// - Move is implemented once.
// - Rotate is implemented once.
// - Resize is implemented once.
// - Rename is implemented once.
// - Object kinds opt into these verbs through descriptors/rules.
//
// This file must not own document storage, UI state, rendering resources,
// filesystem paths, async jobs, editor tool state, command queues, or tests.
// It mutates one CreativeObject that was already found by its owner.

enum class CreativeMutationApplyStatus {
    Unknown,
    Applied,
    NoChange,
    Rejected,
    MissingPayload,
    WrongPayload,
    UnsupportedMutation,
    LockedObject,
    InvalidObject,
    InvalidMutation
};

struct CreativeMutationApplyReceipt {
    CreativeMutationApplyStatus status{CreativeMutationApplyStatus::Unknown};

    CreativeObjectId objectId{0};
    CreativeObjectKind objectKind{CreativeObjectKind::Unknown};
    CreativeMutationKind mutationKind{CreativeMutationKind::Unknown};

    CreativeObjectDirtyFlags dirtyFlags{0};
    bool changed{false};
    bool allowed{false};

    std::string message{};
};

struct CreativeMutationApplyOptions {
    bool rejectLockedObjects{true};
    bool allowNoChange{true};
    bool validateDescriptorRules{true};
    bool validatePayloadShape{true};
};

[[nodiscard]] std::string_view toString(CreativeMutationApplyStatus status) noexcept;

[[nodiscard]] bool mutationApplySucceeded(CreativeMutationApplyStatus status) noexcept;
[[nodiscard]] bool mutationApplyFailed(CreativeMutationApplyStatus status) noexcept;
[[nodiscard]] bool mutationApplyChanged(CreativeMutationApplyStatus status) noexcept;

[[nodiscard]] CreativeMutationApplyReceipt makeMutationApplyReceipt(
    CreativeMutationApplyStatus status,
    const CreativeObject& object,
    CreativeMutationKind mutationKind,
    CreativeObjectDirtyFlags dirtyFlags,
    bool changed,
    bool allowed,
    std::string message);

[[nodiscard]] CreativeMutationApplyReceipt rejectMutation(
    const CreativeObject& object,
    CreativeMutationKind mutationKind,
    CreativeMutationApplyStatus status,
    std::string message);

[[nodiscard]] CreativeMutationApplyReceipt applyMutation(
    CreativeObject& object,
    const CreativeMutationRequest& request,
    const CreativeMutationApplyOptions& options = {});

[[nodiscard]] CreativeMutationApplyReceipt applyMutation(
    CreativeObject& object,
    CreativeMutationKind mutationKind,
    const CreativeMutationPayload& payload,
    const CreativeMutationApplyOptions& options = {});

// Direct helpers for callers that already know they are applying simple shared
// mutations. These helpers still return receipts and are intended to be used by
// the generic applyMutation path, not duplicated per object kind.
[[nodiscard]] CreativeMutationApplyReceipt applyRenameMutation(CreativeObject& object, const RenameMutation& mutation);
[[nodiscard]] CreativeMutationApplyReceipt applyVisibilityMutation(CreativeObject& object, const VisibilityMutation& mutation);
[[nodiscard]] CreativeMutationApplyReceipt applyLockMutation(CreativeObject& object, const LockMutation& mutation);
[[nodiscard]] CreativeMutationApplyReceipt applyMoveMutation(CreativeObject& object, const MoveMutation& mutation);
[[nodiscard]] CreativeMutationApplyReceipt applyRotateMutation(CreativeObject& object, const RotateMutation& mutation);
[[nodiscard]] CreativeMutationApplyReceipt applyScaleMutation(CreativeObject& object, const ScaleMutation& mutation);
[[nodiscard]] CreativeMutationApplyReceipt applySetTransformMutation(CreativeObject& object, const SetTransformMutation& mutation);
[[nodiscard]] CreativeMutationApplyReceipt applyResizeMutation(CreativeObject& object, const ResizeMutation& mutation);
[[nodiscard]] CreativeMutationApplyReceipt applyStretchMutation(CreativeObject& object, const StretchMutation& mutation);
[[nodiscard]] CreativeMutationApplyReceipt applySetBoundsMutation(CreativeObject& object, const SetBoundsMutation& mutation);
[[nodiscard]] CreativeMutationApplyReceipt applyScalarMutation(CreativeObject& object, CreativeMutationKind mutationKind, const ScalarMutation& mutation);
[[nodiscard]] CreativeMutationApplyReceipt applySetParentMutation(CreativeObject& object, const SetParentMutation& mutation);
[[nodiscard]] CreativeMutationApplyReceipt applyClearParentMutation(CreativeObject& object, CreativeMutationKind mutationKind);
[[nodiscard]] CreativeMutationApplyReceipt applyAssignLayerMutation(CreativeObject& object, const AssignLayerMutation& mutation);
[[nodiscard]] CreativeMutationApplyReceipt applyAddTagMutation(CreativeObject& object, const TagMutation& mutation);
[[nodiscard]] CreativeMutationApplyReceipt applyRemoveTagMutation(CreativeObject& object, const TagMutation& mutation);
[[nodiscard]] CreativeMutationApplyReceipt applyClearTagsMutation(CreativeObject& object, CreativeMutationKind mutationKind);

// Detailed future executor hooks. They are declared now so the mutation engine
// has a complete landing map, but complex payload storage is intentionally not
// forced into CreativeObject yet. Until specific payload fields exist on objects,
// these return NoChange with no dirty flags rather than pretending that future
// storage changed.
[[nodiscard]] CreativeMutationApplyReceipt applyAttachMutation(CreativeObject& object, CreativeMutationKind mutationKind, const AttachToMutation& mutation);
[[nodiscard]] CreativeMutationApplyReceipt applyLinkMutation(CreativeObject& object, CreativeMutationKind mutationKind, const LinkTargetMutation& mutation);
[[nodiscard]] CreativeMutationApplyReceipt applySocketMutation(CreativeObject& object, CreativeMutationKind mutationKind, const SetSocketMutation& mutation);
[[nodiscard]] CreativeMutationApplyReceipt applyTextMutation(CreativeObject& object, CreativeMutationKind mutationKind, const TextMutation& mutation);
[[nodiscard]] CreativeMutationApplyReceipt applyReferenceSourceMutation(CreativeObject& object, CreativeMutationKind mutationKind, const ReferenceSourceMutation& mutation);
[[nodiscard]] CreativeMutationApplyReceipt applyColorMutation(CreativeObject& object, CreativeMutationKind mutationKind, const ColorMutation& mutation);
[[nodiscard]] CreativeMutationApplyReceipt applyAudioSourceMutation(CreativeObject& object, CreativeMutationKind mutationKind, const AudioSourceMutation& mutation);
[[nodiscard]] CreativeMutationApplyReceipt applyStringIdMutation(CreativeObject& object, CreativeMutationKind mutationKind, const StringIdMutation& mutation);
[[nodiscard]] CreativeMutationApplyReceipt applyObjectKindMutation(CreativeObject& object, CreativeMutationKind mutationKind, const ObjectKindMutation& mutation);

} // namespace iggy3d::creative
