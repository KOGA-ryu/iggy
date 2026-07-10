#pragma once

#include <cstddef>
#include <string_view>

namespace iggy3d::save_codec_detail {

inline constexpr std::string_view kEnvelopeHeader = "iggy3d.save_envelope.v1";
inline constexpr std::size_t kMaxSaveBytes = 1024U * 1024U;

}  // namespace iggy3d::save_codec_detail
