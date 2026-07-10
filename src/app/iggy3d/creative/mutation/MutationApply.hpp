

#pragma once

#include "app/iggy3d/creative/mutation/Mutation.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"

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

} // namespace iggy3d::creative
