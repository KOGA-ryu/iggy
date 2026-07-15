#include "render/vulkan/FrameCapture.hpp"

#include <array>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace iggy3d::vulkan {
namespace {

constexpr std::uint32_t kSha256Initial[8] = {
    0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
    0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U};

constexpr std::uint32_t kSha256Round[64] = {
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U,
    0x923f82a4U, 0xab1c5ed5U, 0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
    0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U, 0xe49b69c1U, 0xefbe4786U,
    0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U, 0xc6e00bf3U, 0xd5a79147U,
    0x06ca6351U, 0x14292967U, 0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
    0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U, 0xa2bfe8a1U, 0xa81a664bU,
    0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU,
    0x5b9cca4fU, 0x682e6ff3U, 0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
    0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U};

std::uint32_t rotr(std::uint32_t value, std::uint32_t bits) {
  return (value >> bits) | (value << (32U - bits));
}

void appendBigEndian32(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
  bytes.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
  bytes.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
  bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
  bytes.push_back(static_cast<std::uint8_t>(value & 0xFFU));
}

std::string hexDigest(const std::array<std::uint32_t, 8>& state) {
  std::ostringstream stream;
  stream << std::hex << std::setfill('0');
  for (std::uint32_t word : state) {
    stream << std::setw(8) << word;
  }
  return stream.str();
}

std::vector<std::uint8_t> normalizedRgbBytes(const NormalizedCapture& capture) {
  std::vector<std::uint8_t> rgb;
  rgb.reserve(static_cast<std::size_t>(capture.width) * capture.height * 3U);
  for (std::size_t index = 0; index + 3U < capture.rgba.size(); index += 4U) {
    rgb.push_back(capture.rgba[index]);
    rgb.push_back(capture.rgba[index + 1U]);
    rgb.push_back(capture.rgba[index + 2U]);
  }
  return rgb;
}

std::uint32_t crc32(const std::vector<std::uint8_t>& bytes,
                    std::size_t begin,
                    std::size_t end) {
  std::uint32_t crc = 0xffffffffU;
  for (std::size_t i = begin; i < end; ++i) {
    crc ^= bytes[i];
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc & 1U) != 0U ? (crc >> 1U) ^ 0xedb88320U : (crc >> 1U);
    }
  }
  return crc ^ 0xffffffffU;
}

std::uint32_t adler32(const std::vector<std::uint8_t>& bytes) {
  std::uint32_t a = 1U;
  std::uint32_t b = 0U;
  for (std::uint8_t byte : bytes) {
    a = (a + byte) % 65521U;
    b = (b + a) % 65521U;
  }
  return (b << 16U) | a;
}

void appendPngChunk(std::vector<std::uint8_t>& png,
                    const char type[4],
                    const std::vector<std::uint8_t>& data) {
  appendBigEndian32(png, static_cast<std::uint32_t>(data.size()));
  const std::size_t chunkStart = png.size();
  png.insert(png.end(), type, type + 4);
  png.insert(png.end(), data.begin(), data.end());
  appendBigEndian32(png, crc32(png, chunkStart, png.size()));
}

std::vector<std::uint8_t> pngBytes(const NormalizedCapture& capture) {
  std::vector<std::uint8_t> scanlines;
  scanlines.reserve((static_cast<std::size_t>(capture.width) * 4U + 1U) * capture.height);
  for (std::uint32_t y = 0; y < capture.height; ++y) {
    scanlines.push_back(0U);
    const std::size_t rowStart = static_cast<std::size_t>(y) * capture.width * 4U;
    scanlines.insert(scanlines.end(), capture.rgba.begin() + static_cast<std::ptrdiff_t>(rowStart),
                     capture.rgba.begin() +
                         static_cast<std::ptrdiff_t>(rowStart + capture.width * 4U));
  }

  std::vector<std::uint8_t> zlib;
  zlib.push_back(0x78U);
  zlib.push_back(0x01U);
  std::size_t offset = 0;
  while (offset < scanlines.size()) {
    const std::size_t remaining = scanlines.size() - offset;
    const std::uint16_t block =
        static_cast<std::uint16_t>(remaining > 65535U ? 65535U : remaining);
    const bool finalBlock = offset + block == scanlines.size();
    zlib.push_back(finalBlock ? 0x01U : 0x00U);
    zlib.push_back(static_cast<std::uint8_t>(block & 0xFFU));
    zlib.push_back(static_cast<std::uint8_t>((block >> 8U) & 0xFFU));
    const std::uint16_t nlen = static_cast<std::uint16_t>(~block);
    zlib.push_back(static_cast<std::uint8_t>(nlen & 0xFFU));
    zlib.push_back(static_cast<std::uint8_t>((nlen >> 8U) & 0xFFU));
    zlib.insert(zlib.end(), scanlines.begin() + static_cast<std::ptrdiff_t>(offset),
                scanlines.begin() + static_cast<std::ptrdiff_t>(offset + block));
    offset += block;
  }
  appendBigEndian32(zlib, adler32(scanlines));

  std::vector<std::uint8_t> png{0x89U, 'P', 'N', 'G', '\r', '\n', 0x1aU, '\n'};
  std::vector<std::uint8_t> ihdr;
  appendBigEndian32(ihdr, capture.width);
  appendBigEndian32(ihdr, capture.height);
  ihdr.push_back(8U);
  ihdr.push_back(6U);
  ihdr.push_back(0U);
  ihdr.push_back(0U);
  ihdr.push_back(0U);
  appendPngChunk(png, "IHDR", ihdr);
  appendPngChunk(png, "IDAT", zlib);
  appendPngChunk(png, "IEND", {});
  return png;
}

bool writeBytes(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream output(path, std::ios::binary);
  if (!output) {
    return false;
  }
  output.write(reinterpret_cast<const char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
  output.close();
  return output.good() && std::filesystem::exists(path) && std::filesystem::file_size(path) > 0U;
}

bool writeText(const std::filesystem::path& path, const std::string& text) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream output(path);
  if (!output) {
    return false;
  }
  output << text;
  output.close();
  return output.good() && std::filesystem::exists(path) && std::filesystem::file_size(path) > 0U;
}

RenderReceipt captureReceipt(std::string_view result, std::string_view reasonCode) {
  RenderReceipt receipt;
  appendReceiptField(receipt, "receipt_version", "1");
  appendReceiptField(receipt, "repo", "iggy3d");
  appendReceiptField(receipt, "file_plan", "src/render/vulkan/FrameCapture.cpp");
  appendReceiptField(receipt, "packet_order", "7");
  appendReceiptField(receipt, "backend", "vulkan");
  appendReceiptField(receipt, "normalized_format", "RGBA8");
  appendReceiptField(receipt, "frame_hash_algorithm", "sha256_normalized_rgb");
  appendReceiptField(receipt, "result", result);
  appendReceiptField(receipt, "reason_code", reasonCode);
  return receipt;
}

}  // namespace

std::string_view captureFormatName(VkFormat format) {
  switch (format) {
    case VK_FORMAT_B8G8R8A8_SRGB:
      return "VK_FORMAT_B8G8R8A8_SRGB";
    case VK_FORMAT_R8G8B8A8_SRGB:
      return "VK_FORMAT_R8G8B8A8_SRGB";
    case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
      return "VK_FORMAT_A8B8G8R8_SRGB_PACK32";
    default:
      return "unsupported";
  }
}

NormalizedCapture normalizeCapturePixels(const std::uint8_t* source,
                                         std::size_t sourceSize,
                                         std::uint32_t width,
                                         std::uint32_t height,
                                         VkFormat sourceFormat) {
  NormalizedCapture capture;
  capture.width = width;
  capture.height = height;
  capture.sourceFormat = std::string{captureFormatName(sourceFormat)};
  const std::size_t expected = static_cast<std::size_t>(width) * height * 4U;
  if (source == nullptr || width == 0U || height == 0U || sourceSize < expected) {
    capture.width = 0;
    capture.height = 0;
    return capture;
  }
  if (sourceFormat != VK_FORMAT_B8G8R8A8_SRGB &&
      sourceFormat != VK_FORMAT_R8G8B8A8_SRGB) {
    capture.width = 0;
    capture.height = 0;
    return capture;
  }
  capture.rgba.resize(expected);
  for (std::size_t index = 0; index + 3U < expected; index += 4U) {
    if (sourceFormat == VK_FORMAT_B8G8R8A8_SRGB) {
      capture.rgba[index] = source[index + 2U];
      capture.rgba[index + 1U] = source[index + 1U];
      capture.rgba[index + 2U] = source[index];
      capture.rgba[index + 3U] = source[index + 3U];
    } else {
      capture.rgba[index] = source[index];
      capture.rgba[index + 1U] = source[index + 1U];
      capture.rgba[index + 2U] = source[index + 2U];
      capture.rgba[index + 3U] = source[index + 3U];
    }
  }
  return capture;
}

FrameCapture::~FrameCapture() {
  destroy();
}

RenderReceipt FrameCapture::create(const FrameCaptureCreateInfo& createInfo) {
  destroy();
  device_ = createInfo.device;
  extent_ = createInfo.extent;
  colorFormat_ = createInfo.colorFormat;
  if (createInfo.physicalDevice == VK_NULL_HANDLE || createInfo.device == VK_NULL_HANDLE ||
      extent_.width == 0U || extent_.height == 0U) {
    return captureReceipt("skip", "screenshot_capture_unavailable");
  }
  allocator_.create({createInfo.physicalDevice, createInfo.device});
  const VkDeviceSize byteSize =
      static_cast<VkDeviceSize>(extent_.width) * extent_.height * 4ULL;
  VulkanAllocationResult allocation =
      allocator_.createBuffer("buffer.packet7.frame_capture.readback", byteSize,
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  if (allocation.outcome != RenderOutcome::Ok) {
    return captureReceipt("fail", "memory_allocation_failed");
  }
  readback_ = allocation.buffer;
  ready_ = true;
  RenderReceipt receipt = captureReceipt("pass", "packet7_capture_buffer_ready");
  appendReceiptField(receipt, "capture_width", static_cast<std::uint64_t>(extent_.width));
  appendReceiptField(receipt, "capture_height", static_cast<std::uint64_t>(extent_.height));
  appendReceiptField(receipt, "capture_source_format", captureFormatName(colorFormat_));
  return receipt;
}

void FrameCapture::destroy() {
  allocator_.destroyBuffer(readback_);
  allocator_.destroy();
  readback_ = {};
  extent_ = {};
  device_ = {};
  colorFormat_ = {};
  ready_ = false;
}

VkBuffer FrameCapture::buffer() const {
  return readback_.buffer;
}

VkDeviceSize FrameCapture::bufferSizeBytes() const {
  return readback_.sizeBytes;
}

bool FrameCapture::ready() const {
  return ready_ && readback_.buffer != VK_NULL_HANDLE && readback_.allocation.memory != VK_NULL_HANDLE;
}

NormalizedCapture FrameCapture::readMappedRgba() const {
  NormalizedCapture capture;
  if (!ready() || device_ == VK_NULL_HANDLE) {
    return capture;
  }
  void* mapped = nullptr;
  if (vkMapMemory(device_, readback_.allocation.memory, 0, readback_.sizeBytes, 0, &mapped) !=
      VK_SUCCESS) {
    return capture;
  }
  capture.width = extent_.width;
  capture.height = extent_.height;
  capture.sourceFormat = std::string{captureFormatName(colorFormat_)};
  capture = normalizeCapturePixels(static_cast<const std::uint8_t*>(mapped),
                                   static_cast<std::size_t>(readback_.sizeBytes),
                                   extent_.width, extent_.height, colorFormat_);
  vkUnmapMemory(device_, readback_.allocation.memory);
  return capture;
}

std::string sha256NormalizedRgb(const NormalizedCapture& capture) {
  std::vector<std::uint8_t> bytes = normalizedRgbBytes(capture);
  const std::uint64_t bitLength = static_cast<std::uint64_t>(bytes.size()) * 8ULL;
  bytes.push_back(0x80U);
  while ((bytes.size() % 64U) != 56U) {
    bytes.push_back(0U);
  }
  for (int shift = 56; shift >= 0; shift -= 8) {
    bytes.push_back(static_cast<std::uint8_t>((bitLength >> shift) & 0xFFULL));
  }

  std::array<std::uint32_t, 8> state{};
  for (std::size_t i = 0; i < state.size(); ++i) {
    state[i] = kSha256Initial[i];
  }
  for (std::size_t chunk = 0; chunk < bytes.size(); chunk += 64U) {
    std::array<std::uint32_t, 64> w{};
    for (std::size_t i = 0; i < 16U; ++i) {
      const std::size_t base = chunk + i * 4U;
      w[i] = (static_cast<std::uint32_t>(bytes[base]) << 24U) |
             (static_cast<std::uint32_t>(bytes[base + 1U]) << 16U) |
             (static_cast<std::uint32_t>(bytes[base + 2U]) << 8U) |
             static_cast<std::uint32_t>(bytes[base + 3U]);
    }
    for (std::size_t i = 16U; i < 64U; ++i) {
      const std::uint32_t s0 = rotr(w[i - 15U], 7U) ^ rotr(w[i - 15U], 18U) ^ (w[i - 15U] >> 3U);
      const std::uint32_t s1 = rotr(w[i - 2U], 17U) ^ rotr(w[i - 2U], 19U) ^ (w[i - 2U] >> 10U);
      w[i] = w[i - 16U] + s0 + w[i - 7U] + s1;
    }
    std::uint32_t a = state[0];
    std::uint32_t b = state[1];
    std::uint32_t c = state[2];
    std::uint32_t d = state[3];
    std::uint32_t e = state[4];
    std::uint32_t f = state[5];
    std::uint32_t g = state[6];
    std::uint32_t h = state[7];
    for (std::size_t i = 0; i < 64U; ++i) {
      const std::uint32_t s1 = rotr(e, 6U) ^ rotr(e, 11U) ^ rotr(e, 25U);
      const std::uint32_t ch = (e & f) ^ ((~e) & g);
      const std::uint32_t temp1 = h + s1 + ch + kSha256Round[i] + w[i];
      const std::uint32_t s0 = rotr(a, 2U) ^ rotr(a, 13U) ^ rotr(a, 22U);
      const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
      const std::uint32_t temp2 = s0 + maj;
      h = g;
      g = f;
      f = e;
      e = d + temp1;
      d = c;
      c = b;
      b = a;
      a = temp1 + temp2;
    }
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
  }
  return hexDigest(state);
}

double nonBackgroundPixelCoverage(const NormalizedCapture& capture,
                                  std::uint8_t backgroundR,
                                  std::uint8_t backgroundG,
                                  std::uint8_t backgroundB) {
  if (capture.width == 0U || capture.height == 0U || capture.rgba.size() < 4U) {
    return 0.0;
  }
  std::uint64_t visible = 0;
  const std::uint64_t pixels = static_cast<std::uint64_t>(capture.width) * capture.height;
  for (std::size_t index = 0; index + 3U < capture.rgba.size(); index += 4U) {
    if (capture.rgba[index] != backgroundR || capture.rgba[index + 1U] != backgroundG ||
        capture.rgba[index + 2U] != backgroundB) {
      ++visible;
    }
  }
  return pixels == 0U ? 0.0 : static_cast<double>(visible) / static_cast<double>(pixels);
}

FrameCaptureResult writePacket7CaptureArtifacts(const NormalizedCapture& capture,
                                                const FrameCaptureArtifacts& artifacts) {
  FrameCaptureResult result;
  if (capture.width == 0U || capture.height == 0U ||
      capture.rgba.size() != static_cast<std::size_t>(capture.width) * capture.height * 4U) {
    result.receipt = captureReceipt("skip", "screenshot_capture_unavailable");
    return result;
  }

  result.hash = sha256NormalizedRgb(capture);
  result.nonBackgroundPixelCoverage = nonBackgroundPixelCoverage(capture, 9U, 14U, 20U);
  const std::vector<std::uint8_t> png = pngBytes(capture);
  const bool wrotePng = !artifacts.screenshotPath.empty() &&
                        writeBytes(artifacts.screenshotPath, png);
  const bool wroteRaw = !artifacts.rawPath.empty() && writeBytes(artifacts.rawPath, capture.rgba);
  const std::string meta = "capture_width=" + std::to_string(capture.width) +
                           "\ncapture_height=" + std::to_string(capture.height) +
                           "\ncapture_source_format=" + capture.sourceFormat +
                           "\nnormalized_format=RGBA8\nframe_hash_algorithm=sha256_normalized_rgb\n"
                           "frame_hash_exact=" +
                           result.hash + "\nexternal_ui_recorded=" +
                           (artifacts.externalUiRecorded ? "1" : "0") + "\n";
  const bool wroteMeta = !artifacts.metaPath.empty() && writeText(artifacts.metaPath, meta);
  const bool wroteHash =
      !artifacts.hashPath.empty() && writeText(artifacts.hashPath, result.hash + "\n");

  result.written = wrotePng && wroteRaw && wroteMeta && wroteHash;
  result.receipt =
      captureReceipt(result.written ? "pass" : "fail",
                     result.written ? "packet7_screenshot_written" : "screenshot_write_failed");
  appendReceiptField(result.receipt, "screenshot_written", result.written);
  appendReceiptField(result.receipt, "screenshot_path", artifacts.screenshotPath.string());
  appendReceiptField(result.receipt, "screenshot_raw_path", artifacts.rawPath.string());
  appendReceiptField(result.receipt, "screenshot_meta_path", artifacts.metaPath.string());
  appendReceiptField(result.receipt, "frame_hash_path", artifacts.hashPath.string());
  appendReceiptField(result.receipt, "capture_width", static_cast<std::uint64_t>(capture.width));
  appendReceiptField(result.receipt, "capture_height", static_cast<std::uint64_t>(capture.height));
  appendReceiptField(result.receipt, "capture_source_format", capture.sourceFormat);
  appendReceiptField(result.receipt, "frame_hash_exact", result.hash);
  return result;
}

}  // namespace iggy3d::vulkan
