#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/document/DocumentMutation.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool sameVec3(cr::CreativeVec3 lhs, cr::CreativeVec3 rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

std::vector<cr::CreativePathPoint> authoredPathPoints() {
  return {
      cr::CreativePathPoint{{1.0, 0.0, 2.0}},
      cr::CreativePathPoint{{4.0, 0.0, 6.0}},
      cr::CreativePathPoint{{7.0, 0.0, 8.0}},
  };
}

std::vector<cr::CreativePathPoint> updatedPathPoints() {
  return {
      cr::CreativePathPoint{{2.0, 0.0, 3.0}},
      cr::CreativePathPoint{{5.0, 0.0, 7.0}},
      cr::CreativePathPoint{{8.0, 0.0, 9.0}},
  };
}

std::vector<cr::CreativePathPoint> authoredLineEndpoints() {
  return {
      cr::CreativePathPoint{{1.0, 0.0, 2.0}},
      cr::CreativePathPoint{{4.0, 0.0, 6.0}},
  };
}

std::vector<cr::CreativePathPoint> updatedLineEndpoints() {
  return {
      cr::CreativePathPoint{{2.0, 0.0, 3.0}},
      cr::CreativePathPoint{{5.0, 0.0, 7.0}},
  };
}

bool samePathPoints(std::span<const cr::CreativePathPoint> lhs,
                    std::span<const cr::CreativePathPoint> rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (std::size_t index = 0; index < lhs.size(); ++index) {
    if (!sameVec3(lhs[index].position, rhs[index].position)) {
      return false;
    }
  }
  return true;
}

cr::CreativeDocumentCreateRequest patrolRouteCreateRequest() {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::PatrolRoute;
  request.name = "Route A";
  request.hasPathOverride = true;
  request.pathPoints = authoredPathPoints();
  return request;
}

cr::CreativeDocumentCreateRequest lineEndpointCreateRequest(
    cr::CreativeObjectKind kind) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = std::string{cr::toString(kind)} + " A";
  request.hasPathOverride = true;
  request.pathPoints = authoredLineEndpoints();
  return request;
}

cr::CreativeObject restoredPatrolRouteObject() {
  cr::CreativeObject object;
  object.id = 7;
  object.kind = cr::CreativeObjectKind::PatrolRoute;
  object.name = "Restored Route";
  object.pathPoints = authoredPathPoints();
  return object;
}

cr::CreativeObject restoredNavLinkObject() {
  cr::CreativeObject object;
  object.id = 9;
  object.kind = cr::CreativeObjectKind::NavLink;
  object.name = "Restored Nav Link";
  object.pathPoints = authoredLineEndpoints();
  return object;
}

cr::CreativeDocumentRestoreRequest restoreRequestWith(
    cr::CreativeObject object) {
  cr::CreativeDocumentRestoreRequest request;
  request.documentId = 9001;
  request.name = "Restored Path Document";
  request.nextObjectId = 42;
  request.objects.push_back(std::move(object));
  return request;
}

cr::CreativeDocumentCreateReceipt createRoom(cr::CreativeDocument& document) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Room;
  return document.createObject(request);
}

bool patrolRouteCreateStoresExactPathPoints() {
  cr::CreativeDocument document;
  const cr::CreativeObjectDescriptor& descriptor =
      cr::describeObject(cr::CreativeObjectKind::PatrolRoute);
  const std::vector<cr::CreativePathPoint> expected = authoredPathPoints();

  cr::CreativeDocumentCreateRequest request = patrolRouteCreateRequest();
  const cr::CreativeDocumentCreateReceipt receipt =
      document.createObject(request);
  const cr::CreativeObject* object = document.findObject(receipt.objectId);

  return expect(receipt.requested, "path create requested") &&
         expect(receipt.accepted, "path create accepted") &&
         expect(receipt.changed, "path create changed") &&
         expect(receipt.objectCreated, "path object created") &&
         expect(receipt.status == cr::CreativeDocumentCreateStatus::Created,
                "path create status") &&
         expect(receipt.reasonCode == "object_created",
                "path create reason") &&
         expect(receipt.revisionBefore == 0U, "path revision before") &&
         expect(receipt.revisionAfter == 1U, "path revision after") &&
         expect(receipt.creationDirtyFlags == descriptor.creationDirtyFlags,
                "path dirty flags") &&
         expect(receipt.creationDirtyFlags != 0U, "path dirty nonzero") &&
         expect(document.objectCount() == 1U, "path object count") &&
         expect(document.revision() == 1U, "path document revision") &&
         expect(document.dirtyFlags() != 0U, "path document dirty") &&
         expect(object != nullptr, "path object findable") &&
         expect(object != nullptr &&
                    object->kind == cr::CreativeObjectKind::PatrolRoute,
                "path object kind") &&
         expect(object != nullptr && object->name == "Route A",
                "path object name") &&
         expect(object != nullptr &&
                    samePathPoints(object->pathPoints, expected),
                "path points exact");
}

bool patrolRouteCreateRequiresPathOverride() {
  cr::CreativeDocument document;
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::PatrolRoute;

  const cr::CreativeDocumentCreateReceipt receipt =
      document.createObject(request);

  return expect(!receipt.accepted, "missing path not accepted") &&
         expect(receipt.status == cr::CreativeDocumentCreateStatus::Rejected,
                "missing path status") &&
         expect(receipt.reasonCode == "path_override_required",
                "missing path reason") &&
         expect(document.objectCount() == 0U, "missing path object count") &&
         expect(document.revision() == 0U, "missing path revision");
}

bool patrolRouteCreateRejectsShortPath() {
  cr::CreativeDocument document;
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::PatrolRoute;
  request.hasPathOverride = true;
  request.pathPoints = {cr::CreativePathPoint{{1.0, 0.0, 2.0}}};

  const cr::CreativeDocumentCreateReceipt receipt =
      document.createObject(request);

  return expect(!receipt.accepted, "short path not accepted") &&
         expect(receipt.status == cr::CreativeDocumentCreateStatus::Rejected,
                "short path status") &&
         expect(receipt.reasonCode == "invalid_path_points",
                "short path reason") &&
         expect(document.objectCount() == 0U, "short path object count") &&
         expect(document.revision() == 0U, "short path revision");
}

bool patrolRouteCreateRejectsNonFinitePathPoint() {
  cr::CreativeDocument document;
  cr::CreativeDocumentCreateRequest request = patrolRouteCreateRequest();
  request.pathPoints[1].position.x =
      std::numeric_limits<double>::infinity();

  const cr::CreativeDocumentCreateReceipt receipt =
      document.createObject(request);

  return expect(!receipt.accepted, "nonfinite path not accepted") &&
         expect(receipt.status == cr::CreativeDocumentCreateStatus::Rejected,
                "nonfinite path status") &&
         expect(receipt.reasonCode == "invalid_path_points",
                "nonfinite path reason") &&
         expect(document.objectCount() == 0U, "nonfinite path object count") &&
         expect(document.revision() == 0U, "nonfinite path revision");
}

bool nonPathCreateRejectsPathPayload() {
  cr::CreativeDocument document;
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Room;
  request.hasPathOverride = true;
  request.pathPoints = authoredPathPoints();

  const cr::CreativeDocumentCreateReceipt receipt =
      document.createObject(request);

  return expect(!receipt.accepted, "nonpath path not accepted") &&
         expect(receipt.status == cr::CreativeDocumentCreateStatus::Rejected,
                "nonpath path status") &&
         expect(receipt.reasonCode == "path_unsupported",
                "nonpath path reason") &&
         expect(document.objectCount() == 0U, "nonpath path object count") &&
         expect(document.revision() == 0U, "nonpath path revision");
}

bool traversalLinkCreateStoresExactLineEndpoints() {
  bool ok = true;
  for (const cr::CreativeObjectKind kind :
       {cr::CreativeObjectKind::NavLink,
        cr::CreativeObjectKind::JumpLink,
        cr::CreativeObjectKind::ClimbLink}) {
    cr::CreativeDocument document;
    const cr::CreativeObjectDescriptor& descriptor = cr::describeObject(kind);
    const cr::CreativeDocumentCreateReceipt receipt =
        document.createObject(lineEndpointCreateRequest(kind));
    const cr::CreativeObject* object = document.findObject(receipt.objectId);

    ok = expect(receipt.requested, "line endpoint create requested") &&
         expect(receipt.accepted, "line endpoint create accepted") &&
         expect(receipt.changed, "line endpoint create changed") &&
         expect(receipt.objectCreated, "line endpoint object created") &&
         expect(receipt.status == cr::CreativeDocumentCreateStatus::Created,
                "line endpoint create status") &&
         expect(receipt.reasonCode == "object_created",
                "line endpoint create reason") &&
         expect(receipt.creationDirtyFlags == descriptor.creationDirtyFlags,
                "line endpoint dirty flags") &&
         expect(document.objectCount() == 1U,
                "line endpoint object count") &&
         expect(document.revision() == 1U,
                "line endpoint document revision") &&
         expect(object != nullptr, "line endpoint object findable") &&
         expect(object != nullptr && object->kind == kind,
                "line endpoint object kind") &&
         expect(object != nullptr &&
                    samePathPoints(object->pathPoints,
                                   authoredLineEndpoints()),
                "line endpoints exact") &&
         ok;
  }
  return ok;
}

bool traversalLinkCreateRequiresEndpointOverride() {
  cr::CreativeDocument document;
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::NavLink;

  const cr::CreativeDocumentCreateReceipt receipt =
      document.createObject(request);

  return expect(!receipt.accepted, "missing endpoints not accepted") &&
         expect(receipt.status == cr::CreativeDocumentCreateStatus::Rejected,
                "missing endpoints status") &&
         expect(receipt.reasonCode == "line_endpoint_override_required",
                "missing endpoints reason") &&
         expect(document.objectCount() == 0U,
                "missing endpoints object count") &&
         expect(document.revision() == 0U, "missing endpoints revision");
}

bool traversalLinkCreateRejectsInvalidEndpointCount() {
  cr::CreativeDocument document;
  cr::CreativeDocumentCreateRequest request =
      lineEndpointCreateRequest(cr::CreativeObjectKind::JumpLink);
  request.pathPoints = authoredPathPoints();

  const cr::CreativeDocumentCreateReceipt receipt =
      document.createObject(request);

  return expect(!receipt.accepted, "three endpoints not accepted") &&
         expect(receipt.status == cr::CreativeDocumentCreateStatus::Rejected,
                "three endpoints status") &&
         expect(receipt.reasonCode == "invalid_line_endpoints",
                "three endpoints reason") &&
         expect(document.objectCount() == 0U,
                "three endpoints object count") &&
         expect(document.revision() == 0U, "three endpoints revision");
}

bool traversalLinkCreateRejectsNonFiniteEndpoint() {
  cr::CreativeDocument document;
  cr::CreativeDocumentCreateRequest request =
      lineEndpointCreateRequest(cr::CreativeObjectKind::ClimbLink);
  request.pathPoints[1].position.x =
      std::numeric_limits<double>::infinity();

  const cr::CreativeDocumentCreateReceipt receipt =
      document.createObject(request);

  return expect(!receipt.accepted, "nonfinite endpoint not accepted") &&
         expect(receipt.status == cr::CreativeDocumentCreateStatus::Rejected,
                "nonfinite endpoint status") &&
         expect(receipt.reasonCode == "invalid_line_endpoints",
                "nonfinite endpoint reason") &&
         expect(document.objectCount() == 0U,
                "nonfinite endpoint object count") &&
         expect(document.revision() == 0U, "nonfinite endpoint revision");
}

bool restoreForLoadPreservesPathPayload() {
  cr::CreativeDocument document;
  const cr::CreativeDocumentRestoreRequest request =
      restoreRequestWith(restoredPatrolRouteObject());

  const cr::CreativeDocumentRestoreReceipt receipt =
      document.restoreForLoad(request);
  const cr::CreativeObject* object = document.findObject(7);

  return expect(receipt.accepted, "path restore accepted") &&
         expect(receipt.status == cr::CreativeDocumentRestoreStatus::Restored,
                "path restore status") &&
         expect(receipt.reasonCode == "document_restored",
                "path restore reason") &&
         expect(document.id() == 9001U, "path restore document id") &&
         expect(document.objectCount() == 1U, "path restore object count") &&
         expect(document.nextObjectId() == 42U, "path restore next id") &&
         expect(document.revision() == 0U, "path restore clean revision") &&
         expect(document.dirtyFlags() == 0U, "path restore clean dirty") &&
         expect(object != nullptr, "path restore object findable") &&
         expect(object != nullptr &&
                    object->kind == cr::CreativeObjectKind::PatrolRoute,
                "path restore kind") &&
         expect(object != nullptr &&
                    samePathPoints(object->pathPoints, authoredPathPoints()),
                "path restore points exact");
}

bool restoreForLoadPreservesLineEndpoints() {
  cr::CreativeDocument document;
  const cr::CreativeDocumentRestoreRequest request =
      restoreRequestWith(restoredNavLinkObject());

  const cr::CreativeDocumentRestoreReceipt receipt =
      document.restoreForLoad(request);
  const cr::CreativeObject* object = document.findObject(9);

  return expect(receipt.accepted, "line endpoint restore accepted") &&
         expect(receipt.status == cr::CreativeDocumentRestoreStatus::Restored,
                "line endpoint restore status") &&
         expect(receipt.reasonCode == "document_restored",
                "line endpoint restore reason") &&
         expect(object != nullptr, "line endpoint restore object findable") &&
         expect(object != nullptr &&
                    object->kind == cr::CreativeObjectKind::NavLink,
                "line endpoint restore kind") &&
         expect(object != nullptr &&
                    samePathPoints(object->pathPoints,
                                   authoredLineEndpoints()),
                "line endpoint restore points exact") &&
         expect(document.revision() == 0U,
                "line endpoint restore clean revision") &&
         expect(document.dirtyFlags() == 0U,
                "line endpoint restore clean dirty");
}

bool restoreForLoadRejectsInvalidPathWithoutMutation() {
  cr::CreativeDocument document;
  const cr::CreativeDocumentCreateReceipt existing = createRoom(document);
  const std::uint64_t revisionBefore = document.revision();
  const cr::CreativeObjectDirtyFlags dirtyBefore = document.dirtyFlags();

  cr::CreativeObject missingPath = restoredPatrolRouteObject();
  missingPath.pathPoints.clear();
  const cr::CreativeDocumentRestoreReceipt missingReceipt =
      document.restoreForLoad(restoreRequestWith(missingPath));

  cr::CreativeObject invalidPath = restoredPatrolRouteObject();
  invalidPath.pathPoints[0].position.y =
      std::numeric_limits<double>::quiet_NaN();
  const cr::CreativeDocumentRestoreReceipt invalidReceipt =
      document.restoreForLoad(restoreRequestWith(invalidPath));

  return expect(existing.accepted, "path reject setup accepted") &&
         expect(!missingReceipt.accepted, "missing restore rejected") &&
         expect(missingReceipt.status ==
                    cr::CreativeDocumentRestoreStatus::InvalidObject,
                "missing restore status") &&
         expect(missingReceipt.reasonCode == "invalid_path_points",
                "missing restore reason") &&
         expect(!invalidReceipt.accepted, "invalid restore rejected") &&
         expect(invalidReceipt.status ==
                    cr::CreativeDocumentRestoreStatus::InvalidObject,
                "invalid restore status") &&
         expect(invalidReceipt.reasonCode == "invalid_path_points",
                "invalid restore reason") &&
         expect(document.objectCount() == 1U, "invalid restore object count") &&
         expect(document.containsObject(existing.objectId),
                "invalid restore existing object remains") &&
         expect(document.revision() == revisionBefore,
                "invalid restore revision unchanged") &&
         expect(document.dirtyFlags() == dirtyBefore,
                "invalid restore dirty unchanged");
}

bool restoreForLoadRejectsInvalidLineEndpointsWithoutMutation() {
  cr::CreativeDocument document;
  const cr::CreativeDocumentCreateReceipt existing = createRoom(document);
  const std::uint64_t revisionBefore = document.revision();
  const cr::CreativeObjectDirtyFlags dirtyBefore = document.dirtyFlags();

  cr::CreativeObject invalidLink = restoredNavLinkObject();
  invalidLink.pathPoints = authoredPathPoints();
  const cr::CreativeDocumentRestoreReceipt receipt =
      document.restoreForLoad(restoreRequestWith(invalidLink));

  return expect(existing.accepted, "line endpoint reject setup accepted") &&
         expect(!receipt.accepted, "invalid line endpoint restore rejected") &&
         expect(receipt.status ==
                    cr::CreativeDocumentRestoreStatus::InvalidObject,
                "invalid line endpoint restore status") &&
         expect(receipt.reasonCode == "invalid_line_endpoints",
                "invalid line endpoint restore reason") &&
         expect(document.objectCount() == 1U,
                "invalid line endpoint restore object count") &&
         expect(document.containsObject(existing.objectId),
                "invalid line endpoint existing remains") &&
         expect(document.revision() == revisionBefore,
                "invalid line endpoint revision unchanged") &&
         expect(document.dirtyFlags() == dirtyBefore,
                "invalid line endpoint dirty unchanged");
}

bool restoreForLoadRejectsNonPathPathPayload() {
  cr::CreativeDocument document;
  const cr::CreativeDocumentCreateReceipt existing = createRoom(document);
  const std::uint64_t revisionBefore = document.revision();

  cr::CreativeObject object;
  object.id = 8;
  object.kind = cr::CreativeObjectKind::Crate;
  object.name = "Bad Crate";
  object.bounds = {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}};
  object.pathPoints = authoredPathPoints();

  const cr::CreativeDocumentRestoreReceipt receipt =
      document.restoreForLoad(restoreRequestWith(object));

  return expect(existing.accepted, "nonpath restore setup accepted") &&
         expect(!receipt.accepted, "nonpath restore rejected") &&
         expect(receipt.status == cr::CreativeDocumentRestoreStatus::InvalidObject,
                "nonpath restore status") &&
         expect(receipt.reasonCode == "path_unsupported",
                "nonpath restore reason") &&
         expect(document.objectCount() == 1U, "nonpath restore object count") &&
         expect(document.containsObject(existing.objectId),
                "nonpath restore existing remains") &&
         expect(document.revision() == revisionBefore,
                "nonpath restore revision unchanged");
}

bool patrolRoutePathMutationAppliesStoredPoints() {
  cr::CreativeDocument document;
  const cr::CreativeDocumentCreateReceipt created =
      document.createObject(patrolRouteCreateRequest());
  const std::vector<cr::CreativePathPoint> expected = updatedPathPoints();
  const cr::CreativeObjectDirtyFlags expectedDirty =
      cr::dirtyFlagsForMutation(cr::CreativeObjectKind::PatrolRoute,
                                cr::CreativeMutationKind::SetPatrolRoute);
  (void)document.drainDirtyFlags();
  const std::uint64_t revisionBefore = document.revision();

  const cr::CreativeDocumentMutationReceipt receipt =
      cr::applyDocumentMutation(
          document, created.objectId, cr::CreativeMutationKind::SetPatrolRoute,
          cr::makePathPointsPayload(expected));
  const cr::CreativeObject* object = document.findObject(created.objectId);

  return expect(created.accepted, "path mutation setup accepted") &&
         expect(receipt.status == cr::CreativeDocumentMutationStatus::Applied,
                "path mutation document applied") &&
         expect(receipt.objectReceipt.status ==
                    cr::CreativeMutationApplyStatus::Applied,
                "path mutation object applied") &&
         expect(receipt.objectKind == cr::CreativeObjectKind::PatrolRoute,
                "path mutation object kind") &&
         expect(receipt.mutationKind ==
                    cr::CreativeMutationKind::SetPatrolRoute,
                "path mutation kind") &&
         expect(receipt.changed, "path mutation changed") &&
         expect(receipt.allowed, "path mutation allowed") &&
         expect(receipt.revisionBefore == revisionBefore,
                "path mutation revision before") &&
         expect(receipt.revisionAfter == revisionBefore + 1U,
                "path mutation revision after") &&
         expect(document.revision() == revisionBefore + 1U,
                "path mutation document revision advanced") &&
         expect(receipt.dirtyFlags == expectedDirty,
                "path mutation dirty flags") &&
         expect(expectedDirty != 0U, "path mutation dirty nonzero") &&
         expect(cr::hasDirtyFlag(expectedDirty,
                                 cr::CreativeObjectDirtyFlag::Gameplay),
                "path mutation dirty gameplay") &&
         expect(cr::hasDirtyFlag(expectedDirty,
                                 cr::CreativeObjectDirtyFlag::Logic),
                "path mutation dirty logic") &&
         expect(document.dirtyFlags() == expectedDirty,
                "path mutation document dirty accumulated") &&
         expect(receipt.objectReceipt.message == "object path points changed",
                "path mutation message") &&
         expect(object != nullptr, "path mutation object findable") &&
         expect(object != nullptr &&
                    samePathPoints(object->pathPoints, expected),
                "path mutation points exact");
}

bool patrolRoutePathMutationSamePointsNoChange() {
  cr::CreativeDocument document;
  const cr::CreativeDocumentCreateReceipt created =
      document.createObject(patrolRouteCreateRequest());
  (void)document.drainDirtyFlags();
  const std::uint64_t revisionBefore = document.revision();

  const cr::CreativeDocumentMutationReceipt receipt =
      cr::applyDocumentMutation(
          document, created.objectId, cr::CreativeMutationKind::SetPatrolRoute,
          cr::makePathPointsPayload(authoredPathPoints()));
  const cr::CreativeObject* object = document.findObject(created.objectId);

  return expect(created.accepted, "path no-change setup accepted") &&
         expect(receipt.status == cr::CreativeDocumentMutationStatus::NoChange,
                "path no-change document status") &&
         expect(receipt.objectReceipt.status ==
                    cr::CreativeMutationApplyStatus::NoChange,
                "path no-change object status") &&
         expect(!receipt.changed, "path no-change changed false") &&
         expect(receipt.allowed, "path no-change allowed") &&
         expect(receipt.dirtyFlags == 0U, "path no-change dirty zero") &&
         expect(receipt.revisionBefore == revisionBefore,
                "path no-change revision before") &&
         expect(receipt.revisionAfter == revisionBefore,
                "path no-change revision after") &&
         expect(document.revision() == revisionBefore,
                "path no-change document revision unchanged") &&
         expect(document.dirtyFlags() == 0U,
                "path no-change document dirty zero") &&
         expect(receipt.objectReceipt.message ==
                    "object path points already match requested path",
                "path no-change message") &&
         expect(object != nullptr, "path no-change object findable") &&
         expect(object != nullptr &&
                    samePathPoints(object->pathPoints, authoredPathPoints()),
                "path no-change points preserved");
}

bool patrolRoutePathMutationRejectsInvalidPoints() {
  cr::CreativeDocument document;
  const cr::CreativeDocumentCreateReceipt created =
      document.createObject(patrolRouteCreateRequest());
  const std::vector<cr::CreativePathPoint> original = authoredPathPoints();
  (void)document.drainDirtyFlags();
  const std::uint64_t revisionBefore = document.revision();

  const cr::CreativeDocumentMutationReceipt shortReceipt =
      cr::applyDocumentMutation(
          document, created.objectId, cr::CreativeMutationKind::SetPatrolRoute,
          cr::makePathPointsPayload({cr::CreativePathPoint{{1.0, 0.0, 2.0}}}));

  std::vector<cr::CreativePathPoint> nonFinite = updatedPathPoints();
  nonFinite[1].position.z = std::numeric_limits<double>::quiet_NaN();
  const cr::CreativeDocumentMutationReceipt nonFiniteReceipt =
      cr::applyDocumentMutation(
          document, created.objectId, cr::CreativeMutationKind::SetPatrolRoute,
          cr::makePathPointsPayload(nonFinite));
  const cr::CreativeObject* object = document.findObject(created.objectId);

  return expect(created.accepted, "path invalid setup accepted") &&
         expect(shortReceipt.status ==
                    cr::CreativeDocumentMutationStatus::ApplyFailed,
                "path short document failed") &&
         expect(shortReceipt.objectReceipt.status ==
                    cr::CreativeMutationApplyStatus::Rejected,
                "path short object rejected") &&
         expect(shortReceipt.objectReceipt.message == "path points are invalid",
                "path short message") &&
         expect(nonFiniteReceipt.status ==
                    cr::CreativeDocumentMutationStatus::ApplyFailed,
                "path nonfinite document failed") &&
         expect(nonFiniteReceipt.objectReceipt.status ==
                    cr::CreativeMutationApplyStatus::Rejected,
                "path nonfinite object rejected") &&
         expect(nonFiniteReceipt.objectReceipt.message ==
                    "path points are invalid",
                "path nonfinite message") &&
         expect(document.revision() == revisionBefore,
                "path invalid revision unchanged") &&
         expect(document.dirtyFlags() == 0U, "path invalid dirty zero") &&
         expect(object != nullptr, "path invalid object findable") &&
         expect(object != nullptr && samePathPoints(object->pathPoints, original),
                "path invalid points preserved");
}

bool nonPathPathMutationRejectsDeterministically() {
  cr::CreativeDocument document;
  const cr::CreativeDocumentCreateReceipt room = createRoom(document);
  (void)document.drainDirtyFlags();
  const std::uint64_t revisionBefore = document.revision();

  const cr::CreativeDocumentMutationReceipt receipt =
      cr::applyDocumentMutation(
          document, room.objectId, cr::CreativeMutationKind::SetPatrolRoute,
          cr::makePathPointsPayload(updatedPathPoints()));

  return expect(room.accepted, "nonpath path mutation setup accepted") &&
         expect(receipt.status ==
                    cr::CreativeDocumentMutationStatus::ApplyFailed,
                "nonpath path mutation document failed") &&
         expect(receipt.objectKind == cr::CreativeObjectKind::Room,
                "nonpath path mutation object kind") &&
         expect(receipt.objectReceipt.status ==
                    cr::CreativeMutationApplyStatus::UnsupportedMutation,
                "nonpath path mutation unsupported") &&
         expect(receipt.objectReceipt.message ==
                    "object kind does not allow this mutation",
                "nonpath path mutation message") &&
         expect(!receipt.changed, "nonpath path mutation changed false") &&
         expect(!receipt.allowed, "nonpath path mutation allowed false") &&
         expect(receipt.dirtyFlags == 0U, "nonpath path mutation dirty zero") &&
         expect(document.revision() == revisionBefore,
                "nonpath path mutation revision unchanged") &&
         expect(document.dirtyFlags() == 0U,
                "nonpath path mutation document dirty zero");
}

bool traversalLinkEndpointMutationAppliesStoredPoints() {
  cr::CreativeDocument document;
  const cr::CreativeDocumentCreateReceipt created =
      document.createObject(
          lineEndpointCreateRequest(cr::CreativeObjectKind::NavLink));
  (void)document.drainDirtyFlags();
  const std::uint64_t revisionBefore = document.revision();

  const cr::CreativeDocumentMutationReceipt receipt =
      cr::applyDocumentMutation(
          document, created.objectId, cr::CreativeMutationKind::SetPatrolRoute,
          cr::makePathPointsPayload(updatedLineEndpoints()));
  const cr::CreativeObject* object = document.findObject(created.objectId);

  return expect(created.accepted, "line endpoint mutation setup accepted") &&
         expect(receipt.status == cr::CreativeDocumentMutationStatus::Applied,
                "line endpoint mutation document applied") &&
         expect(receipt.objectReceipt.status ==
                    cr::CreativeMutationApplyStatus::Applied,
                "line endpoint mutation object applied") &&
         expect(receipt.objectKind == cr::CreativeObjectKind::NavLink,
                "line endpoint mutation object kind") &&
         expect(receipt.changed, "line endpoint mutation changed") &&
         expect(receipt.allowed, "line endpoint mutation allowed") &&
         expect(receipt.revisionBefore == revisionBefore,
                "line endpoint mutation revision before") &&
         expect(receipt.revisionAfter == revisionBefore + 1U,
                "line endpoint mutation revision after") &&
         expect(receipt.objectReceipt.message ==
                    "object line endpoints changed",
                "line endpoint mutation message") &&
         expect(object != nullptr, "line endpoint mutation object findable") &&
         expect(object != nullptr &&
                    samePathPoints(object->pathPoints,
                                   updatedLineEndpoints()),
                "line endpoint mutation points exact");
}

bool traversalLinkEndpointMutationRejectsInvalidCount() {
  cr::CreativeDocument document;
  const cr::CreativeDocumentCreateReceipt created =
      document.createObject(
          lineEndpointCreateRequest(cr::CreativeObjectKind::JumpLink));
  (void)document.drainDirtyFlags();
  const std::uint64_t revisionBefore = document.revision();

  const cr::CreativeDocumentMutationReceipt receipt =
      cr::applyDocumentMutation(
          document, created.objectId, cr::CreativeMutationKind::SetPatrolRoute,
          cr::makePathPointsPayload(authoredPathPoints()));
  const cr::CreativeObject* object = document.findObject(created.objectId);

  return expect(created.accepted, "line endpoint invalid setup accepted") &&
         expect(receipt.status ==
                    cr::CreativeDocumentMutationStatus::ApplyFailed,
                "line endpoint invalid document failed") &&
         expect(receipt.objectReceipt.status ==
                    cr::CreativeMutationApplyStatus::Rejected,
                "line endpoint invalid object rejected") &&
         expect(receipt.objectReceipt.message == "line endpoints are invalid",
                "line endpoint invalid message") &&
         expect(document.revision() == revisionBefore,
                "line endpoint invalid revision unchanged") &&
         expect(document.dirtyFlags() == 0U,
                "line endpoint invalid dirty zero") &&
         expect(object != nullptr, "line endpoint invalid object findable") &&
         expect(object != nullptr &&
                    samePathPoints(object->pathPoints,
                                   authoredLineEndpoints()),
                "line endpoint invalid preserved");
}

bool legacyPatrolRoutePayloadsRemainFutureStorageNoChange() {
  cr::CreativeDocument document;
  const cr::CreativeDocumentCreateReceipt created =
      document.createObject(patrolRouteCreateRequest());
  const std::vector<cr::CreativePathPoint> original = authoredPathPoints();
  (void)document.drainDirtyFlags();
  const std::uint64_t revisionBefore = document.revision();

  const cr::CreativeDocumentMutationReceipt textReceipt =
      cr::applyDocumentMutation(
          document, created.objectId, cr::CreativeMutationKind::SetPatrolRoute,
          cr::makeTextPayload("route-a"));
  const cr::CreativeDocumentMutationReceipt stringIdReceipt =
      cr::applyDocumentMutation(
          document, created.objectId, cr::CreativeMutationKind::SetPatrolRoute,
          cr::makeStringIdPayload("route-b"));
  const cr::CreativeMutationPayload pathPayload =
      cr::makePathPointsPayload(updatedPathPoints());
  const cr::CreativeMutationPayload textPayload = cr::makeTextPayload("route-a");
  const cr::CreativeMutationPayload stringIdPayload =
      cr::makeStringIdPayload("route-b");
  const cr::CreativeObject* object = document.findObject(created.objectId);

  return expect(created.accepted, "path legacy setup accepted") &&
         expect(cr::mutationStoragePolicy(
                    cr::CreativeMutationKind::SetPatrolRoute) ==
                    cr::CreativeMutationStoragePolicy::PayloadDependent,
                "path legacy mutation is payload-dependent") &&
         expect(cr::mutationPayloadStoragePolicy(
                    cr::CreativeMutationKind::SetPatrolRoute, pathPayload) ==
                    cr::CreativeMutationStoragePolicy::StoredObject,
                "path points payload stored policy") &&
         expect(cr::mutationPayloadStoragePolicy(
                    cr::CreativeMutationKind::SetPatrolRoute, textPayload) ==
                    cr::CreativeMutationStoragePolicy::FutureStoragePlaceholder,
                "path legacy text future policy") &&
         expect(cr::mutationPayloadStoragePolicy(
                    cr::CreativeMutationKind::SetPatrolRoute,
                    stringIdPayload) ==
                    cr::CreativeMutationStoragePolicy::FutureStoragePlaceholder,
                "path legacy string id future policy") &&
         expect(cr::mutationPayloadHasStoredObjectEffect(
                    cr::CreativeMutationKind::SetPatrolRoute, pathPayload),
                "path points payload has stored effect") &&
         expect(!cr::mutationPayloadHasStoredObjectEffect(
                    cr::CreativeMutationKind::SetPatrolRoute, textPayload),
                "path legacy text has no stored effect") &&
         expect(!cr::mutationPayloadHasStoredObjectEffect(
                    cr::CreativeMutationKind::SetPatrolRoute, stringIdPayload),
                "path legacy string id has no stored effect") &&
         expect(textReceipt.status ==
                    cr::CreativeDocumentMutationStatus::NoChange,
                "path legacy text no change") &&
         expect(textReceipt.objectReceipt.status ==
                    cr::CreativeMutationApplyStatus::NoChange,
                "path legacy text object no change") &&
         expect(textReceipt.objectReceipt.message ==
                    "mutation has no stored object field yet",
                "path legacy text message") &&
         expect(stringIdReceipt.status ==
                    cr::CreativeDocumentMutationStatus::NoChange,
                "path legacy string id no change") &&
         expect(stringIdReceipt.objectReceipt.status ==
                    cr::CreativeMutationApplyStatus::NoChange,
                "path legacy string id object no change") &&
         expect(stringIdReceipt.objectReceipt.message ==
                    "mutation has no stored object field yet",
                "path legacy string id message") &&
         expect(textReceipt.dirtyFlags == 0U, "path legacy text dirty zero") &&
         expect(stringIdReceipt.dirtyFlags == 0U,
                "path legacy string id dirty zero") &&
         expect(document.revision() == revisionBefore,
                "path legacy revision unchanged") &&
         expect(document.dirtyFlags() == 0U, "path legacy dirty zero") &&
         expect(object != nullptr, "path legacy object findable") &&
         expect(object != nullptr && samePathPoints(object->pathPoints, original),
                "path legacy points preserved");
}

bool documentCopyPreservesPathPayload() {
  cr::CreativeDocument document;
  const cr::CreativeDocumentCreateReceipt created =
      document.createObject(patrolRouteCreateRequest());
  const cr::CreativeDocument copy = document;

  const cr::CreativeObject* original = document.findObject(created.objectId);
  const cr::CreativeObject* copied = copy.findObject(created.objectId);

  return expect(created.accepted, "copy setup accepted") &&
         expect(original != nullptr, "copy original findable") &&
         expect(copied != nullptr, "copy copied findable") &&
         expect(original != nullptr && copied != nullptr &&
                    samePathPoints(copied->pathPoints, original->pathPoints),
                "copy path points exact") &&
         expect(copy.objectCount() == document.objectCount(),
                "copy object count") &&
         expect(copy.revision() == document.revision(), "copy revision");
}

}  // namespace

int main() {
  const bool ok = patrolRouteCreateStoresExactPathPoints() &&
                  patrolRouteCreateRequiresPathOverride() &&
                  patrolRouteCreateRejectsShortPath() &&
                  patrolRouteCreateRejectsNonFinitePathPoint() &&
                  nonPathCreateRejectsPathPayload() &&
                  traversalLinkCreateStoresExactLineEndpoints() &&
                  traversalLinkCreateRequiresEndpointOverride() &&
                  traversalLinkCreateRejectsInvalidEndpointCount() &&
                  traversalLinkCreateRejectsNonFiniteEndpoint() &&
                  restoreForLoadPreservesPathPayload() &&
                  restoreForLoadPreservesLineEndpoints() &&
                  restoreForLoadRejectsInvalidPathWithoutMutation() &&
                  restoreForLoadRejectsInvalidLineEndpointsWithoutMutation() &&
                  restoreForLoadRejectsNonPathPathPayload() &&
                  patrolRoutePathMutationAppliesStoredPoints() &&
                  patrolRoutePathMutationSamePointsNoChange() &&
                  patrolRoutePathMutationRejectsInvalidPoints() &&
                  nonPathPathMutationRejectsDeterministically() &&
                  traversalLinkEndpointMutationAppliesStoredPoints() &&
                  traversalLinkEndpointMutationRejectsInvalidCount() &&
                  legacyPatrolRoutePayloadsRemainFutureStorageNoChange() &&
                  documentCopyPreservesPathPayload();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
