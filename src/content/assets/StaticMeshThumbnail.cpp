#include "content/assets/StaticMeshThumbnail.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include "content/assets/StaticMeshAsset.hpp"

namespace iggy3d {
namespace {

struct ThumbnailProjection {
  Vec3 center;
  float minimumU = 0.0F;
  float minimumV = 0.0F;
  float scale = 0.0F;
  float offsetX = 0.0F;
  float offsetY = 0.0F;
  bool valid = false;
};

struct ProjectedVertex {
  float x = 0.0F;
  float y = 0.0F;
  float depth = 0.0F;
};

constexpr Vec3 kThumbnailRight{0.70710678F, 0.0F, -0.70710678F};
constexpr Vec3 kThumbnailUp{-0.40824829F, 0.81649658F, -0.40824829F};
constexpr Vec3 kThumbnailTowardCamera{0.57735027F, 0.57735027F, 0.57735027F};

[[nodiscard]] bool finite(float value) noexcept {
  return std::isfinite(value);
}

[[nodiscard]] ProjectedVertex project(Vec3 point,
                                      const ThumbnailProjection& projection) {
  const Vec3 local = point - projection.center;
  const float u = dot(local, kThumbnailRight);
  const float v = dot(local, kThumbnailUp);
  return {
      projection.offsetX + (u - projection.minimumU) * projection.scale,
      projection.offsetY - (v - projection.minimumV) * projection.scale,
      dot(local, kThumbnailTowardCamera),
  };
}

[[nodiscard]] ThumbnailProjection makeProjection(
    const StaticMeshAsset& asset) noexcept {
  ThumbnailProjection output;
  if (!asset.hasBounds || asset.vertices.empty()) {
    return output;
  }
  output.center = (asset.boundsMin + asset.boundsMax) * 0.5F;
  float maximumU = -std::numeric_limits<float>::infinity();
  float maximumV = -std::numeric_limits<float>::infinity();
  output.minimumU = std::numeric_limits<float>::infinity();
  output.minimumV = std::numeric_limits<float>::infinity();
  for (const StaticMeshVertex& vertex : asset.vertices) {
    const Vec3 local = vertex.position - output.center;
    const float u = dot(local, kThumbnailRight);
    const float v = dot(local, kThumbnailUp);
    if (!finite(u) || !finite(v)) {
      return {};
    }
    output.minimumU = std::min(output.minimumU, u);
    output.minimumV = std::min(output.minimumV, v);
    maximumU = std::max(maximumU, u);
    maximumV = std::max(maximumV, v);
  }
  const float width = maximumU - output.minimumU;
  const float height = maximumV - output.minimumV;
  const float longest = std::max(width, height);
  if (!finite(longest) || longest <= 1.0e-6F) {
    return {};
  }
  constexpr float kInset = 2.0F;
  constexpr float kDrawable =
      static_cast<float>(kStaticMeshThumbnailExtent) - 2.0F * kInset - 1.0F;
  output.scale = kDrawable / longest;
  output.offsetX = kInset + (kDrawable - width * output.scale) * 0.5F;
  // Screen Y grows downward, so the projected V maximum starts at the inset.
  output.offsetY = kInset + height * output.scale;
  output.valid = finite(output.scale) && output.scale > 0.0F;
  return output;
}

[[nodiscard]] float edge(float ax,
                         float ay,
                         float bx,
                         float by,
                         float px,
                         float py) noexcept {
  return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
}

[[nodiscard]] std::uint8_t channel(float value) noexcept {
  return static_cast<std::uint8_t>(
      std::clamp(std::lround(value * 255.0F), 0L, 255L));
}

[[nodiscard]] StaticMeshThumbnailPixel shadedMaterial(
    const StaticMeshAsset& asset,
    const StaticMeshPrimitive& primitive,
    Vec3 first,
    Vec3 second,
    Vec3 third) noexcept {
  constexpr Vec3 kLight{-0.32444284F, 0.81110711F, 0.48666426F};
  Vec3 face = normalized(cross(second - first, third - first));
  const float light = 0.48F + 0.52F * std::fabs(dot(face, kLight));
  std::array<float, 3U> base{0.62F, 0.68F, 0.74F};
  if (primitive.materialIndex < asset.materials.size()) {
    const StaticMeshMaterial& material = asset.materials[primitive.materialIndex];
    base = {material.baseColorFactor[0], material.baseColorFactor[1],
            material.baseColorFactor[2]};
  }
  return {channel(std::clamp(base[0], 0.0F, 1.0F) * light),
          channel(std::clamp(base[1], 0.0F, 1.0F) * light),
          channel(std::clamp(base[2], 0.0F, 1.0F) * light), 255U};
}

void rasterTriangle(const ProjectedVertex& first,
                    const ProjectedVertex& second,
                    const ProjectedVertex& third,
                    StaticMeshThumbnailPixel color,
                    std::array<float, kStaticMeshThumbnailPixelCount>& depth,
                    StaticMeshAssetThumbnail& thumbnail) noexcept {
  const float area = edge(first.x, first.y, second.x, second.y, third.x, third.y);
  if (!finite(area) || std::fabs(area) <= 1.0e-6F) {
    return;
  }
  const int minimumX = std::clamp(
      static_cast<int>(std::floor(std::min({first.x, second.x, third.x}))), 0,
      static_cast<int>(kStaticMeshThumbnailExtent) - 1);
  const int maximumX = std::clamp(
      static_cast<int>(std::ceil(std::max({first.x, second.x, third.x}))), 0,
      static_cast<int>(kStaticMeshThumbnailExtent) - 1);
  const int minimumY = std::clamp(
      static_cast<int>(std::floor(std::min({first.y, second.y, third.y}))), 0,
      static_cast<int>(kStaticMeshThumbnailExtent) - 1);
  const int maximumY = std::clamp(
      static_cast<int>(std::ceil(std::max({first.y, second.y, third.y}))), 0,
      static_cast<int>(kStaticMeshThumbnailExtent) - 1);
  for (int y = minimumY; y <= maximumY; ++y) {
    for (int x = minimumX; x <= maximumX; ++x) {
      const float px = static_cast<float>(x) + 0.5F;
      const float py = static_cast<float>(y) + 0.5F;
      const float firstWeight =
          edge(second.x, second.y, third.x, third.y, px, py);
      const float secondWeight =
          edge(third.x, third.y, first.x, first.y, px, py);
      const float thirdWeight =
          edge(first.x, first.y, second.x, second.y, px, py);
      const bool inside = area > 0.0F
                              ? firstWeight >= 0.0F && secondWeight >= 0.0F &&
                                    thirdWeight >= 0.0F
                              : firstWeight <= 0.0F && secondWeight <= 0.0F &&
                                    thirdWeight <= 0.0F;
      if (!inside) {
        continue;
      }
      const float inverseArea = 1.0F / area;
      const float pixelDepth =
          (firstWeight * first.depth + secondWeight * second.depth +
           thirdWeight * third.depth) *
          inverseArea;
      const std::size_t index = static_cast<std::size_t>(y) *
                                    kStaticMeshThumbnailExtent +
                                static_cast<std::size_t>(x);
      if (pixelDepth < depth[index]) {
        continue;
      }
      depth[index] = pixelDepth;
      thumbnail.pixels[index] = color;
    }
  }
}

void setPixel(StaticMeshAssetThumbnail& thumbnail,
              int x,
              int y,
              StaticMeshThumbnailPixel color) noexcept {
  if (x < 0 || y < 0 || x >= static_cast<int>(kStaticMeshThumbnailExtent) ||
      y >= static_cast<int>(kStaticMeshThumbnailExtent)) {
    return;
  }
  thumbnail.pixels[static_cast<std::size_t>(y) *
                       kStaticMeshThumbnailExtent +
                   static_cast<std::size_t>(x)] = color;
}

void drawLine(StaticMeshAssetThumbnail& thumbnail,
              ProjectedVertex first,
              ProjectedVertex second,
              StaticMeshThumbnailPixel color) noexcept {
  int x0 = static_cast<int>(std::lround(first.x));
  int y0 = static_cast<int>(std::lround(first.y));
  const int x1 = static_cast<int>(std::lround(second.x));
  const int y1 = static_cast<int>(std::lround(second.y));
  const int dx = std::abs(x1 - x0);
  const int sx = x0 < x1 ? 1 : -1;
  const int dy = -std::abs(y1 - y0);
  const int sy = y0 < y1 ? 1 : -1;
  int error = dx + dy;
  for (;;) {
    setPixel(thumbnail, x0, y0, color);
    if (x0 == x1 && y0 == y1) {
      return;
    }
    const int twiceError = 2 * error;
    if (twiceError >= dy) {
      error += dy;
      x0 += sx;
    }
    if (twiceError <= dx) {
      error += dx;
      y0 += sy;
    }
  }
}

[[nodiscard]] std::array<Vec3, 8U> boundsCorners(Vec3 minimum,
                                                 Vec3 maximum) noexcept {
  return {{{minimum.x, minimum.y, minimum.z},
           {maximum.x, minimum.y, minimum.z},
           {maximum.x, maximum.y, minimum.z},
           {minimum.x, maximum.y, minimum.z},
           {minimum.x, minimum.y, maximum.z},
           {maximum.x, minimum.y, maximum.z},
           {maximum.x, maximum.y, maximum.z},
           {minimum.x, maximum.y, maximum.z}}};
}

void drawBounds(StaticMeshAssetThumbnail& thumbnail,
                Vec3 minimum,
                Vec3 maximum,
                const ThumbnailProjection& projection,
                StaticMeshThumbnailPixel color) noexcept {
  constexpr std::array<std::array<std::uint8_t, 2U>, 12U> kEdges{{
      {{0U, 1U}}, {{1U, 2U}}, {{2U, 3U}}, {{3U, 0U}},
      {{4U, 5U}}, {{5U, 6U}}, {{6U, 7U}}, {{7U, 4U}},
      {{0U, 4U}}, {{1U, 5U}}, {{2U, 6U}}, {{3U, 7U}},
  }};
  const std::array<Vec3, 8U> corners = boundsCorners(minimum, maximum);
  for (const auto& edgeIndices : kEdges) {
    drawLine(thumbnail, project(corners[edgeIndices[0]], projection),
             project(corners[edgeIndices[1]], projection), color);
  }
}

void drawSocket(StaticMeshAssetThumbnail& thumbnail,
                const StaticMeshAttachmentSocket& socket,
                const ThumbnailProjection& projection) noexcept {
  const ProjectedVertex point = project(socket.position, projection);
  const StaticMeshThumbnailPixel color =
      socket.role == StaticMeshAttachmentSocketRole::Receiver
          ? StaticMeshThumbnailPixel{80U, 255U, 132U, 255U}
          : StaticMeshThumbnailPixel{255U, 92U, 218U, 255U};
  const int x = static_cast<int>(std::lround(point.x));
  const int y = static_cast<int>(std::lround(point.y));
  setPixel(thumbnail, x, y, color);
  setPixel(thumbnail, x - 1, y, color);
  setPixel(thumbnail, x + 1, y, color);
  setPixel(thumbnail, x, y - 1, color);
  setPixel(thumbnail, x, y + 1, color);
}

}  // namespace

StaticMeshAssetThumbnail buildStaticMeshAssetThumbnail(
    const StaticMeshAsset& asset) noexcept {
  StaticMeshAssetThumbnail thumbnail;
  const ThumbnailProjection projection = makeProjection(asset);
  if (!projection.valid || asset.indices.empty() || asset.primitives.empty()) {
    return thumbnail;
  }

  std::array<float, kStaticMeshThumbnailPixelCount> depth;
  depth.fill(-std::numeric_limits<float>::infinity());
  for (const StaticMeshPrimitive& primitive : asset.primitives) {
    const std::size_t firstIndex = primitive.firstIndex;
    const std::size_t endIndex =
        std::min(asset.indices.size(),
                 firstIndex + static_cast<std::size_t>(primitive.indexCount));
    for (std::size_t index = firstIndex; index + 2U < endIndex; index += 3U) {
      const std::uint32_t firstIndexValue = asset.indices[index];
      const std::uint32_t secondIndexValue = asset.indices[index + 1U];
      const std::uint32_t thirdIndexValue = asset.indices[index + 2U];
      if (firstIndexValue >= asset.vertices.size() ||
          secondIndexValue >= asset.vertices.size() ||
          thirdIndexValue >= asset.vertices.size()) {
        continue;
      }
      const Vec3 first = asset.vertices[firstIndexValue].position;
      const Vec3 second = asset.vertices[secondIndexValue].position;
      const Vec3 third = asset.vertices[thirdIndexValue].position;
      rasterTriangle(project(first, projection), project(second, projection),
                     project(third, projection),
                     shadedMaterial(asset, primitive, first, second, third),
                     depth, thumbnail);
    }
  }

  drawBounds(thumbnail, asset.boundsMin, asset.boundsMax, projection,
             {94U, 220U, 255U, 255U});
  for (const StaticMeshCollisionPart& part : asset.collisionParts) {
    drawBounds(thumbnail, part.boundsMin, part.boundsMax, projection,
               {255U, 176U, 56U, 255U});
  }
  for (const StaticMeshAttachmentSocket& socket : asset.attachmentSockets) {
    drawSocket(thumbnail, socket, projection);
  }
  thumbnail.coveredPixelCount = static_cast<std::uint16_t>(std::count_if(
      thumbnail.pixels.begin(), thumbnail.pixels.end(),
      [](StaticMeshThumbnailPixel pixel) { return pixel.a != 0U; }));
  thumbnail.valid = thumbnail.coveredPixelCount != 0U;
  return thumbnail;
}

}  // namespace iggy3d
