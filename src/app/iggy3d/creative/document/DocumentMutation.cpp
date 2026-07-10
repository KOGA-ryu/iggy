

#include "app/iggy3d/creative/document/DocumentMutation.hpp"

#include <optional>
#include <utility>

namespace iggy3d::creative {
namespace {

[[nodiscard]] CreativeObjectDirtyFlags mergeDirtyFlags(CreativeObjectDirtyFlags lhs, CreativeObjectDirtyFlags rhs) noexcept {
    return lhs | rhs;
}

[[nodiscard]] CreativeDocumentMutationStatus statusFromApplyReceipt(const CreativeMutationApplyReceipt& receipt) noexcept {
    switch (receipt.status) {
    case CreativeMutationApplyStatus::Applied:
        return CreativeDocumentMutationStatus::Applied;
    case CreativeMutationApplyStatus::NoChange:
        return CreativeDocumentMutationStatus::NoChange;
    case CreativeMutationApplyStatus::Rejected:
    case CreativeMutationApplyStatus::MissingPayload:
    case CreativeMutationApplyStatus::WrongPayload:
    case CreativeMutationApplyStatus::UnsupportedMutation:
    case CreativeMutationApplyStatus::LockedObject:
    case CreativeMutationApplyStatus::InvalidObject:
    case CreativeMutationApplyStatus::InvalidMutation:
    case CreativeMutationApplyStatus::Unknown:
        return CreativeDocumentMutationStatus::ApplyFailed;
    }

    return CreativeDocumentMutationStatus::ApplyFailed;
}

[[nodiscard]] CreativeDocumentMutationStatus statusFromBatchCounts(
    std::uint64_t appliedCount,
    std::uint64_t noChangeCount,
    std::uint64_t failedCount) noexcept {
    if (failedCount > 0 && appliedCount > 0) {
        return CreativeDocumentMutationStatus::BatchPartiallyApplied;
    }

    if (failedCount > 0) {
        return CreativeDocumentMutationStatus::ApplyFailed;
    }

    if (appliedCount > 0) {
        return CreativeDocumentMutationStatus::BatchApplied;
    }

    if (noChangeCount > 0) {
        return CreativeDocumentMutationStatus::BatchNoChange;
    }

    return CreativeDocumentMutationStatus::BatchNoChange;
}

void incrementDocumentRevisionForMutation(CreativeDocument& document,
                                          CreativeObjectDirtyFlags dirtyFlags) {
    // This function intentionally exists as the only revision bridge for object
    // mutation. Keep revision ownership in CreativeDocument rather than
    // incrementing revisions in random call sites.
    document.markObjectMutationChanged(dirtyFlags);
}

[[nodiscard]] std::optional<CreativeObjectId> requestedRelationshipParentId(
    const CreativeMutationRequest& request) {
    const auto& value = request.payload.value;
    if (request.kind == CreativeMutationKind::SetParent &&
        std::holds_alternative<SetParentMutation>(value)) {
        return std::get<SetParentMutation>(value).parentId;
    }

    if (request.kind == CreativeMutationKind::AttachTo &&
        std::holds_alternative<AttachToMutation>(value)) {
        return std::get<AttachToMutation>(value).targetId;
    }

    return std::nullopt;
}

[[nodiscard]] CreativeDocumentMutationReceipt rejectDocumentRelationshipMutation(
    CreativeDocument& document,
    const CreativeObject& object,
    CreativeMutationKind mutationKind,
    std::string message) {
    const auto revision = document.revision();
    return makeDocumentMutationReceipt(
        CreativeDocumentMutationStatus::ApplyFailed,
        object.id,
        object.kind,
        mutationKind,
        revision,
        revision,
        0,
        false,
        false,
        rejectMutation(object, mutationKind, CreativeMutationApplyStatus::Rejected, message),
        std::move(message));
}

[[nodiscard]] std::string_view validateRelationshipParentAssignment(
    const CreativeDocument& document,
    CreativeObjectId objectId,
    CreativeObjectId parentId) {
    std::vector<CreativeObject> proposedObjects{
        document.objects().begin(), document.objects().end()};
    for (CreativeObject& object : proposedObjects) {
        if (object.id == objectId) {
            object.parentId = parentId;
            break;
        }
    }

    return validateCreativeObjectParentGraph(proposedObjects);
}

} // namespace

std::string_view toString(CreativeDocumentMutationStatus status) noexcept {
    switch (status) {
    case CreativeDocumentMutationStatus::Unknown: return "Unknown";
    case CreativeDocumentMutationStatus::Applied: return "Applied";
    case CreativeDocumentMutationStatus::NoChange: return "NoChange";
    case CreativeDocumentMutationStatus::Rejected: return "Rejected";
    case CreativeDocumentMutationStatus::MissingDocument: return "MissingDocument";
    case CreativeDocumentMutationStatus::InvalidDocument: return "InvalidDocument";
    case CreativeDocumentMutationStatus::MissingObject: return "MissingObject";
    case CreativeDocumentMutationStatus::InvalidRequest: return "InvalidRequest";
    case CreativeDocumentMutationStatus::ApplyFailed: return "ApplyFailed";
    case CreativeDocumentMutationStatus::BatchPartiallyApplied: return "BatchPartiallyApplied";
    case CreativeDocumentMutationStatus::BatchApplied: return "BatchApplied";
    case CreativeDocumentMutationStatus::BatchNoChange: return "BatchNoChange";
    }

    return "Unknown";
}

bool documentMutationSucceeded(CreativeDocumentMutationStatus status) noexcept {
    switch (status) {
    case CreativeDocumentMutationStatus::Applied:
    case CreativeDocumentMutationStatus::NoChange:
    case CreativeDocumentMutationStatus::BatchApplied:
    case CreativeDocumentMutationStatus::BatchNoChange:
        return true;
    default:
        return false;
    }
}

bool documentMutationFailed(CreativeDocumentMutationStatus status) noexcept {
    return !documentMutationSucceeded(status);
}

bool documentMutationChanged(CreativeDocumentMutationStatus status) noexcept {
    return status == CreativeDocumentMutationStatus::Applied ||
           status == CreativeDocumentMutationStatus::BatchApplied ||
           status == CreativeDocumentMutationStatus::BatchPartiallyApplied;
}

CreativeDocumentMutationReceipt makeDocumentMutationReceipt(
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
    std::string message) {
    return CreativeDocumentMutationReceipt{
        status,
        objectId,
        objectKind,
        mutationKind,
        revisionBefore,
        revisionAfter,
        dirtyFlags,
        changed,
        allowed,
        std::move(objectReceipt),
        std::move(message),
    };
}

CreativeDocumentMutationReceipt rejectDocumentMutation(
    CreativeDocument& document,
    CreativeObjectId objectId,
    CreativeMutationKind mutationKind,
    CreativeDocumentMutationStatus status,
    std::string message) {
    const auto revision = document.revision();
    CreativeObject placeholder{};
    placeholder.id = objectId;
    const std::string objectMessage = message;

    return makeDocumentMutationReceipt(
        status,
        objectId,
        CreativeObjectKind::Unknown,
        mutationKind,
        revision,
        revision,
        0,
        false,
        false,
        rejectMutation(placeholder, mutationKind, CreativeMutationApplyStatus::Rejected, objectMessage),
        std::move(message));
}

CreativeDocumentMutationReceipt applyDocumentMutation(
    CreativeDocument& document,
    const CreativeMutationRequest& request,
    const CreativeDocumentMutationOptions& options) {
    if (!document.isValid()) {
        return rejectDocumentMutation(document, request.objectId, request.kind, CreativeDocumentMutationStatus::InvalidDocument, "cannot mutate object in invalid document");
    }

    if (request.objectId == 0 || request.kind == CreativeMutationKind::Unknown) {
        return rejectDocumentMutation(document, request.objectId, request.kind, CreativeDocumentMutationStatus::InvalidRequest, "document mutation request is invalid");
    }

    auto* object = document.findObject(request.objectId);
    if (object == nullptr) {
        return rejectDocumentMutation(document, request.objectId, request.kind, CreativeDocumentMutationStatus::MissingObject, "document does not contain requested object");
    }

    const std::optional<CreativeObjectId> relationshipParentId =
        requestedRelationshipParentId(request);
    if (relationshipParentId.has_value() && canMutate(object->kind, request.kind)) {
        const std::string_view relationshipValidation =
            validateRelationshipParentAssignment(
                document, object->id, *relationshipParentId);
        if (!relationshipValidation.empty()) {
            return rejectDocumentRelationshipMutation(
                document, *object, request.kind, std::string{relationshipValidation});
        }
    }

    const auto revisionBefore = document.revision();
    auto objectReceipt = applyMutation(*object, request, options.applyOptions);
    const auto documentStatus = statusFromApplyReceipt(objectReceipt);

    if (documentStatus == CreativeDocumentMutationStatus::Applied && objectReceipt.changed && options.incrementRevisionOnChange) {
        incrementDocumentRevisionForMutation(document, objectReceipt.dirtyFlags);
    }

    const auto revisionAfter = document.revision();
    return makeDocumentMutationReceipt(
        documentStatus,
        object->id,
        object->kind,
        request.kind,
        revisionBefore,
        revisionAfter,
        objectReceipt.dirtyFlags,
        objectReceipt.changed,
        objectReceipt.allowed,
        std::move(objectReceipt),
        "document mutation applied through object mutation pipeline");
}

CreativeDocumentMutationReceipt applyDocumentMutation(
    CreativeDocument& document,
    CreativeObjectId objectId,
    CreativeMutationKind mutationKind,
    const CreativeMutationPayload& payload,
    const CreativeDocumentMutationOptions& options) {
    return applyDocumentMutation(
        document,
        CreativeMutationRequest{0, objectId, mutationKind, payload},
        options);
}

CreativeDocumentBatchMutationReceipt applyDocumentMutations(
    CreativeDocument& document,
    std::span<const CreativeMutationRequest> requests,
    const CreativeDocumentMutationOptions& options) {
    CreativeDocumentBatchMutationReceipt batch{};
    batch.revisionBefore = document.revision();
    batch.revisionAfter = batch.revisionBefore;
    batch.attemptedCount = static_cast<std::uint64_t>(requests.size());

    for (const auto& request : requests) {
        auto receipt = applyDocumentMutation(document, request, options);
        batch.dirtyFlags = mergeDirtyFlags(batch.dirtyFlags, receipt.dirtyFlags);

        if (receipt.status == CreativeDocumentMutationStatus::Applied) {
            ++batch.appliedCount;
            batch.changed = true;
        } else if (receipt.status == CreativeDocumentMutationStatus::NoChange) {
            ++batch.noChangeCount;
        } else {
            ++batch.failedCount;
            if (options.stopBatchOnFailure) {
                batch.stoppedEarly = true;
                batch.receipts.push_back(std::move(receipt));
                break;
            }
        }

        batch.receipts.push_back(std::move(receipt));
    }

    batch.revisionAfter = document.revision();
    batch.status = statusFromBatchCounts(batch.appliedCount, batch.noChangeCount, batch.failedCount);
    batch.message = "document mutation batch completed";
    return batch;
}

CreativeDocumentMutationReceipt renameDocumentObject(
    CreativeDocument& document,
    CreativeObjectId objectId,
    std::string name,
    const CreativeDocumentMutationOptions& options) {
    return applyDocumentMutation(document, objectId, CreativeMutationKind::Rename, makeRenamePayload(std::move(name)), options);
}

CreativeDocumentMutationReceipt moveDocumentObject(
    CreativeDocument& document,
    CreativeObjectId objectId,
    CreativeVec3 position,
    const CreativeDocumentMutationOptions& options) {
    return applyDocumentMutation(document, objectId, CreativeMutationKind::Move, makeMovePayload(position), options);
}

CreativeDocumentMutationReceipt setDocumentObjectVisible(
    CreativeDocument& document,
    CreativeObjectId objectId,
    bool visible,
    const CreativeDocumentMutationOptions& options) {
    return applyDocumentMutation(document, objectId, CreativeMutationKind::SetVisible, makeVisibilityPayload(visible), options);
}

CreativeDocumentMutationReceipt setDocumentObjectLocked(
    CreativeDocument& document,
    CreativeObjectId objectId,
    bool locked,
    const CreativeDocumentMutationOptions& options) {
    return applyDocumentMutation(document, objectId, CreativeMutationKind::SetLocked, makeLockPayload(locked), options);
}

} // namespace iggy3d::creative
