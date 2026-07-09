#include "render/mesh/BeanMesh.hpp"

#include <array>
#include <cmath>

namespace iggy3d {
namespace {

constexpr float kPi = 3.14159265358979323846F;
constexpr std::uint16_t kSegments = 12U;

struct BeanRing {
  float y = 0.0F;
  float radius = 0.0F;
};

void appendTriangle(BeanMesh& mesh,
                    std::uint16_t a,
                    std::uint16_t b,
                    std::uint16_t c) {
  mesh.indices.push_back(a);
  mesh.indices.push_back(b);
  mesh.indices.push_back(c);
}

void appendDoubleSidedTriangle(BeanMesh& mesh,
                               std::uint16_t a,
                               std::uint16_t b,
                               std::uint16_t c) {
  appendTriangle(mesh, a, b, c);
  appendTriangle(mesh, c, b, a);
}

std::uint16_t appendVertex(BeanMesh& mesh, Vec3 position) {
  mesh.vertices.push_back({position});
  return static_cast<std::uint16_t>(mesh.vertices.size() - 1U);
}

void appendCodexProbeCap(BeanMesh& mesh) {
  const std::uint16_t baseA = appendVertex(mesh, {-0.12F, 0.48F, -0.08F});
  const std::uint16_t baseB = appendVertex(mesh, {0.12F, 0.48F, -0.08F});
  const std::uint16_t baseC = appendVertex(mesh, {0.12F, 0.48F, 0.08F});
  const std::uint16_t baseD = appendVertex(mesh, {-0.12F, 0.48F, 0.08F});
  const std::uint16_t tip = appendVertex(mesh, {0.00F, 0.64F, 0.00F});

  appendDoubleSidedTriangle(mesh, baseA, baseB, tip);
  appendDoubleSidedTriangle(mesh, baseB, baseC, tip);
  appendDoubleSidedTriangle(mesh, baseC, baseD, tip);
  appendDoubleSidedTriangle(mesh, baseD, baseA, tip);
  appendDoubleSidedTriangle(mesh, baseA, baseD, baseC);
  appendDoubleSidedTriangle(mesh, baseA, baseC, baseB);
}

}  // namespace

Vec3 defaultBeanModelSize(SceneModelKind kind) {
  switch (kind) {
    case SceneModelKind::PlayerBean:
      return {0.58F, 1.55F, 0.46F};
    case SceneModelKind::NpcBean:
      return {0.52F, 1.35F, 0.42F};
    case SceneModelKind::CodexProbeBean:
      return {0.44F, 1.10F, 0.44F};
  }
  return {0.50F, 1.25F, 0.42F};
}

BeanMesh buildBeanMesh(SceneModelKind kind) {
  BeanMesh mesh;
  mesh.kind = kind;
  constexpr std::array<BeanRing, 6U> kRings{{
      {-0.48F, 0.08F},
      {-0.36F, 0.27F},
      {-0.16F, 0.41F},
      {0.10F, 0.45F},
      {0.32F, 0.34F},
      {0.47F, 0.15F},
  }};

  mesh.vertices.reserve(kRings.size() * kSegments + 8U);
  mesh.indices.reserve((kRings.size() - 1U) * kSegments * 12U + kSegments * 12U + 36U);

  for (std::size_t ringIndex = 0; ringIndex < kRings.size(); ++ringIndex) {
    const BeanRing& ring = kRings[ringIndex];
    const float normalized = (ring.y + 0.50F);
    const float centerX = std::sin(normalized * kPi) * 0.035F;
    const float xRadius = ring.radius * (1.0F + 0.06F * std::sin(normalized * kPi * 2.0F));
    const float zRadius = ring.radius * 0.78F;
    for (std::uint16_t segment = 0U; segment < kSegments; ++segment) {
      const float angle = (static_cast<float>(segment) / static_cast<float>(kSegments)) *
                          kPi * 2.0F;
      const float x = centerX + std::cos(angle) * xRadius;
      const float z = std::sin(angle) * zRadius;
      appendVertex(mesh, {x, ring.y, z});
    }
  }

  for (std::uint16_t ring = 0U; ring + 1U < static_cast<std::uint16_t>(kRings.size());
       ++ring) {
    for (std::uint16_t segment = 0U; segment < kSegments; ++segment) {
      const std::uint16_t nextSegment = static_cast<std::uint16_t>((segment + 1U) % kSegments);
      const std::uint16_t a = static_cast<std::uint16_t>(ring * kSegments + segment);
      const std::uint16_t b = static_cast<std::uint16_t>(ring * kSegments + nextSegment);
      const std::uint16_t c = static_cast<std::uint16_t>((ring + 1U) * kSegments + segment);
      const std::uint16_t d = static_cast<std::uint16_t>((ring + 1U) * kSegments + nextSegment);
      appendDoubleSidedTriangle(mesh, a, c, b);
      appendDoubleSidedTriangle(mesh, b, c, d);
    }
  }

  const std::uint16_t bottomCenter = appendVertex(mesh, {0.0F, -0.52F, 0.0F});
  const std::uint16_t topCenter = appendVertex(mesh, {0.0F, 0.52F, 0.0F});
  const std::uint16_t topRingStart =
      static_cast<std::uint16_t>((kRings.size() - 1U) * kSegments);
  for (std::uint16_t segment = 0U; segment < kSegments; ++segment) {
    const std::uint16_t nextSegment = static_cast<std::uint16_t>((segment + 1U) % kSegments);
    appendDoubleSidedTriangle(mesh, bottomCenter, nextSegment, segment);
    appendDoubleSidedTriangle(mesh, topCenter,
                              static_cast<std::uint16_t>(topRingStart + segment),
                              static_cast<std::uint16_t>(topRingStart + nextSegment));
  }

  if (kind == SceneModelKind::CodexProbeBean) {
    appendCodexProbeCap(mesh);
  }
  return mesh;
}

}  // namespace iggy3d
