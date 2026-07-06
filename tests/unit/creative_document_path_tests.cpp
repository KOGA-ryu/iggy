#include "app/iggy3d/creative/document/Document.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <span>
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

cr::CreativeObject restoredPatrolRouteObject() {
  cr::CreativeObject object;
  object.id = 7;
  object.kind = cr::CreativeObjectKind::PatrolRoute;
  object.name = "Restored Route";
  object.pathPoints = authoredPathPoints();
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
                  restoreForLoadPreservesPathPayload() &&
                  restoreForLoadRejectsInvalidPathWithoutMutation() &&
                  restoreForLoadRejectsNonPathPathPayload() &&
                  documentCopyPreservesPathPayload();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
