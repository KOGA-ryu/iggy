

#pragma once

#include "app/iggy3d/creative/Document.hpp"
#include "app/iggy3d/creative/Mutation.hpp"
#include "app/iggy3d/creative/MutationApply.hpp"
#include "app/iggy3d/creative/Object.hpp"
#include "app/iggy3d/creative/ObjectDescriptor.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

// DocumentMutation is the document-level mutation bridge.
//
// MutationApply mutates one already-found CreativeObject.
// DocumentMutation finds the object inside CreativeDocument, applies the shared
// mutation executor, owns document revision policy, and returns a document-level
// receipt. This keeps object mutation generic while preserving document truth.
//
// Pipeline:
// - Caller creates CreativeMutationRequest.
// - DocumentMutation finds the target object in CreativeDocument.
// - MutationApply validates object kind, payload, lock state, and descriptor rules.
// - If the object actually changes, DocumentMutation increments document revision.
// - Receipt reports object result, revision before/after, and dirty flags.
//
// This file must not own UI, rendering, filesystem, async jobs, editor tool
// state, command queues, or tests. It is only the authored document mutation
// coordinator.

enum class CreativeDocumentMutationStatus {
    Unknown,
    Applied,
    NoChange,
    Rejected,
    MissingDocument,
    InvalidDocument,
    MissingObject,
    InvalidRequest,
    ApplyFailed,
    BatchPartiallyApplied,
    BatchApplied,
    BatchNoChange
};

struct CreativeDocumentMutationOptions {
    CreativeMutationApplyOptions applyOptions{};

    // When true, a changed object mutation increments the document revision.
    bool incrementRevisionOnChange{true};

    // When true, no-change object mutations are treated as successful receipts.
    bool allowNoChange{true};

    // When true, batch mutation stops on the first failed item.
    bool stopBatchOnFailure{true};
};

struct CreativeDocumentMutationReceipt {
    CreativeDocumentMutationStatus status{CreativeDocumentMutationStatus::Unknown};

    CreativeObjectId objectId{0};
    CreativeObjectKind objectKind{CreativeObjectKind::Unknown};
    CreativeMutationKind mutationKind{CreativeMutationKind::Unknown};

    std::uint64_t revisionBefore{0};
    std::uint64_t revisionAfter{0};

    CreativeObjectDirtyFlags dirtyFlags{0};
    bool changed{false};
    bool allowed{false};

    CreativeMutationApplyReceipt objectReceipt{};
    std::string message{};
};

struct CreativeDocumentBatchMutationReceipt {
    CreativeDocumentMutationStatus status{CreativeDocumentMutationStatus::Unknown};

    std::uint64_t revisionBefore{0};
    std::uint64_t revisionAfter{0};

    CreativeObjectDirtyFlags dirtyFlags{0};
    std::uint64_t attemptedCount{0};
    std::uint64_t appliedCount{0};
    std::uint64_t noChangeCount{0};
    std::uint64_t failedCount{0};

    bool changed{false};
    bool stoppedEarly{false};

    std::vector<CreativeDocumentMutationReceipt> receipts{};
    std::string message{};
};

[[nodiscard]] std::string_view toString(CreativeDocumentMutationStatus status) noexcept;

[[nodiscard]] bool documentMutationSucceeded(CreativeDocumentMutationStatus status) noexcept;
[[nodiscard]] bool documentMutationFailed(CreativeDocumentMutationStatus status) noexcept;
[[nodiscard]] bool documentMutationChanged(CreativeDocumentMutationStatus status) noexcept;

[[nodiscard]] CreativeDocumentMutationReceipt makeDocumentMutationReceipt(
    CreativeDocumentMutationStatus status,
    CreativeObjectId objectId,
    CreativeObjectKind objectKind,
    CreativeMutationKind mutationKind,
    std::uint64_t revisionBefore,
    std::uint64_t revisionAfter,
    CreativeObjectDirtyFlags dirtyFlags,
    bool changed,
    bool allowed,
    CreativeMutationApplyReceipt objectReceipt,
    std::string message);

[[nodiscard]] CreativeDocumentMutationReceipt rejectDocumentMutation(
    CreativeDocument& document,
    CreativeObjectId objectId,
    CreativeMutationKind mutationKind,
    CreativeDocumentMutationStatus status,
    std::string message);

[[nodiscard]] CreativeDocumentMutationReceipt applyDocumentMutation(
    CreativeDocument& document,
    const CreativeMutationRequest& request,
    const CreativeDocumentMutationOptions& options = {});

[[nodiscard]] CreativeDocumentMutationReceipt applyDocumentMutation(
    CreativeDocument& document,
    CreativeObjectId objectId,
    CreativeMutationKind mutationKind,
    const CreativeMutationPayload& payload,
    const CreativeDocumentMutationOptions& options = {});

[[nodiscard]] CreativeDocumentBatchMutationReceipt applyDocumentMutations(
    CreativeDocument& document,
    std::span<const CreativeMutationRequest> requests,
    const CreativeDocumentMutationOptions& options = {});

// Convenience wrappers. These keep caller code readable while still routing all
// object edits through the generic document mutation pipeline.
[[nodiscard]] CreativeDocumentMutationReceipt renameDocumentObject(
    CreativeDocument& document,
    CreativeObjectId objectId,
    std::string name,
    const CreativeDocumentMutationOptions& options = {});

[[nodiscard]] CreativeDocumentMutationReceipt moveDocumentObject(
    CreativeDocument& document,
    CreativeObjectId objectId,
    CreativeVec3 position,
    const CreativeDocumentMutationOptions& options = {});

[[nodiscard]] CreativeDocumentMutationReceipt rotateDocumentObject(
    CreativeDocument& document,
    CreativeObjectId objectId,
    CreativeVec3 rotation,
    const CreativeDocumentMutationOptions& options = {});

[[nodiscard]] CreativeDocumentMutationReceipt resizeDocumentObject(
    CreativeDocument& document,
    CreativeObjectId objectId,
    CreativeVec3 size,
    const CreativeDocumentMutationOptions& options = {});

[[nodiscard]] CreativeDocumentMutationReceipt setDocumentObjectBounds(
    CreativeDocument& document,
    CreativeObjectId objectId,
    CreativeBounds bounds,
    const CreativeDocumentMutationOptions& options = {});

[[nodiscard]] CreativeDocumentMutationReceipt setDocumentObjectVisible(
    CreativeDocument& document,
    CreativeObjectId objectId,
    bool visible,
    const CreativeDocumentMutationOptions& options = {});

[[nodiscard]] CreativeDocumentMutationReceipt setDocumentObjectLocked(
    CreativeDocument& document,
    CreativeObjectId objectId,
    bool locked,
    const CreativeDocumentMutationOptions& options = {});

[[nodiscard]] CreativeDocumentMutationReceipt assignDocumentObjectLayer(
    CreativeDocument& document,
    CreativeObjectId objectId,
    CreativeLayerId layerId,
    const CreativeDocumentMutationOptions& options = {});

[[nodiscard]] CreativeDocumentMutationReceipt addDocumentObjectTag(
    CreativeDocument& document,
    CreativeObjectId objectId,
    std::string tag,
    const CreativeDocumentMutationOptions& options = {});

[[nodiscard]] CreativeDocumentMutationReceipt removeDocumentObjectTag(
    CreativeDocument& document,
    CreativeObjectId objectId,
    std::string tag,
    const CreativeDocumentMutationOptions& options = {});

} // namespace iggy3d::creative
