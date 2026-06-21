# Vulkan Texture Format Policy

This document defines the first Vulkan texture format ladder for `iggy3d`: sRGB versus linear data rules, fallback formats, mip policy, sampler compatibility, compressed-format deferral, and platform-specific format evidence for macOS/MoltenVK, Linux, and Windows.

Texture formats are renderer-owned GPU interpretation details. Runtime, content, projection, save, replay, and gameplay code may identify assets and material meaning, but they must not own `VkFormat`, image tiling, format feature flags, sampler compatibility, mip generation rules, or GPU fallback choices.

## Purpose

Define a portable first texture path:

```text
first_texture_format=VK_FORMAT_R8G8B8A8_SRGB
first_data_texture_format=VK_FORMAT_R8G8B8A8_UNORM
first_image_tiling=VK_IMAGE_TILING_OPTIMAL
first_texture_usage=VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
first_mip_policy=mip_levels_1
first_sampler_filter=linear_if_supported_else_nearest
first_wrap_mode=repeat_or_clamp_by_material_policy
compressed_textures=deferred
runtime_format_authority=forbidden
```

This policy keeps the first room and first material portable while leaving room for BC/ASTC/ETC growth after real device evidence exists.

This document narrows:

- [render_assets_and_materials.md](render_assets_and_materials.md)
- [gpu_resource_upload.md](gpu_resource_upload.md)
- [vulkan_image_layouts_and_barriers.md](vulkan_image_layouts_and_barriers.md)
- [descriptor_policy.md](descriptor_policy.md)
- [vulkan_memory_budget_policy.md](vulkan_memory_budget_policy.md)
- [device_selection.md](device_selection.md)
- [platform_matrix.md](platform_matrix.md)
- [debug_validation.md](debug_validation.md)

## Source Priority

Use these sources before implementation:

| Source | Use for |
| --- | --- |
| Vulkan Specification format chapter: https://docs.vulkan.org/spec/latest/chapters/formats.html | `VkFormat`, required support, format feature queries, compressed formats, sRGB definitions |
| Vulkan Specification sampler chapter: https://docs.vulkan.org/spec/latest/chapters/samplers.html | sampler state, filtering, mip LOD behavior, anisotropy limits |
| Vulkan Guide formats: https://docs.vulkan.org/guide/latest/formats.html | practical explanation of format families and format meaning |
| Khronos Vulkan Tutorial image view/sampler and mipmap chapters | first sampled texture, sampler, and runtime mipmap implementation shape |
| Vulkan Samples texture mipmap generation: https://docs.vulkan.org/samples/latest/samples/api/texture_mipmap_generation/README.html | production-ish example after first texture smoke is clean |
| MoltenVK Runtime User Guide: https://github.com/KhronosGroup/MoltenVK/blob/main/Docs/MoltenVK_Runtime_UserGuide.md | Apple portability constraints and extension/format caveats |

Priority rule:

```text
exact_format_behavior=Vulkan Specification and Registry
first_implementation_shape=Khronos Vulkan Tutorial
format_explanation=Vulkan Guide
sample_growth=Vulkan Samples
apple_constraints=MoltenVK notes only, not cross_platform_truth
```

## Scope

In scope:

- first uncompressed color texture format;
- data texture format rules;
- sRGB versus linear material interpretation;
- fallback texture formats and bytes;
- image view format policy;
- sampler compatibility;
- mip level policy;
- runtime mip generation gate;
- compressed texture format deferral;
- platform query receipts for macOS, Linux, and Windows;
- diagnostics and failure reason codes.

Out of scope:

- asset authoring pipeline design;
- KTX/Basis adoption decision;
- texture streaming;
- virtual texturing;
- bindless texture arrays;
- texture arrays and cube maps;
- storage images;
- HDR pipeline;
- color management beyond first sRGB/linear rules;
- renderer-driven runtime mutation.

## Local File Surface

Likely future files:

```text
src/render/vulkan/BuffersImagesMemory.hpp
src/render/vulkan/BuffersImagesMemory.cpp
src/render/vulkan/VulkanTextureFormats.hpp
src/render/vulkan/VulkanTextureFormats.cpp
src/render/vulkan/VulkanRenderAssets.hpp
src/render/vulkan/VulkanRenderAssets.cpp
src/render/vulkan/PipelinesShaders.hpp
src/render/vulkan/PipelinesShaders.cpp
src/render/vulkan/VulkanTypes.hpp
src/render/RenderDiagnostics.hpp
src/render/RenderDiagnostics.cpp
tests/unit/render_texture_format_policy_tests.cpp
tests/unit/render_sampler_policy_tests.cpp
tests/smoke/vulkan_texture_smoke.cpp
tests/smoke/vulkan_material_smoke.cpp
tests/smoke/vulkan_first_room_smoke.cpp
```

`VulkanTextureFormats` should exist only when format selection logic becomes too large for `BuffersImagesMemory`. Until then, builders may keep a small private format helper beside resource upload code.

## Ownership

| Item | Owner | Must never own |
| --- | --- | --- |
| `VkFormat` selection | Vulkan texture/resource module | runtime/content/projection |
| format feature queries | device/resource module | gameplay code |
| image view format | Vulkan texture/resource module | material semantic truth |
| sampler object | renderer resource/material layer | runtime command legality |
| fallback GPU texture | renderer resource/material layer | package validation truth |
| texture color-space interpretation | renderer material layer from backend-neutral material metadata | Vulkan handles outside renderer |
| compressed-format choice | renderer after feature evidence | first-room proof |

Rules:

- content may say "base color texture" or "normal texture";
- renderer decides which `VkFormat` represents that data on the current device;
- renderer fallback use must be diagnosed and must not mutate runtime state;
- runtime/content/projection must not include `VkFormat`, `VK_FORMAT_*`, sampler constants, or format feature flags.

## Format Query Rule

Do not assume optional format behavior.

Required query path:

```text
vkGetPhysicalDeviceFormatProperties or vkGetPhysicalDeviceFormatProperties2
vkGetPhysicalDeviceImageFormatProperties or vkGetPhysicalDeviceImageFormatProperties2 when image usage/sample count/tiling/flags matter
```

For every texture candidate, the renderer must verify:

```text
tiling=VK_IMAGE_TILING_OPTIMAL
required_features include VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT
usage includes VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
format supports image creation for selected type/extent/mip/sample_count
sampler filter policy is compatible with format features
```

Runtime mip generation additionally requires blit/filter support. If that support is missing, runtime mip generation is disabled and the texture must use one mip level or precomputed mip data.

## First Format Ladder

Preferred first sampled texture ladder:

| Texture purpose | Preferred format | Fallback | Notes |
| --- | --- | --- | --- |
| base color/albedo | `VK_FORMAT_R8G8B8A8_SRGB` | `VK_FORMAT_B8G8R8A8_SRGB`, then diagnosed fallback texture | color data, sampled as sRGB |
| UI/debug color if needed | `VK_FORMAT_R8G8B8A8_SRGB` | `VK_FORMAT_B8G8R8A8_SRGB` | only if Vulkan UI/debug texture appears |
| normal map | `VK_FORMAT_R8G8B8A8_UNORM` | renderer fallback normal texture | linear data, never sRGB |
| metallic/roughness/AO packed | `VK_FORMAT_R8G8B8A8_UNORM` | `VK_FORMAT_R8_UNORM` per channel later | linear data |
| mask/alpha map | `VK_FORMAT_R8_UNORM` | `VK_FORMAT_R8G8B8A8_UNORM` | linear data |
| height map | `VK_FORMAT_R8_UNORM` | `VK_FORMAT_R16_UNORM` later if needed | linear data |
| HDR/emissive growth | `VK_FORMAT_R16G16B16A16_SFLOAT` | deferred | not first-room scope |
| fallback magenta/checker | same as base color path | generated CPU bytes | must always be available after texture path exists |
| fallback normal | `VK_FORMAT_R8G8B8A8_UNORM` | generated CPU bytes | default normal points out of surface |

Portable baseline:

```text
texture_compression_required=false
uncompressed_rgba8_required_for_first_texture=true
three_channel_rgb_upload=forbidden_initially
loader_expands_to_rgba8=true
```

Rationale:

- four-channel RGBA8 avoids many three-channel support and row-packing surprises;
- uncompressed RGBA8 gives a common first path across macOS/MoltenVK, Linux, and Windows;
- compressed textures are useful later but must not block first material proof.

## sRGB Versus Linear

sRGB textures:

```text
base_color=sRGB
albedo=sRGB
painted_diffuse=sRGB
ui_color=sRGB_if_used
```

Linear textures:

```text
normal=UNORM_or_SNORM_linear_data
metallic=UNORM_linear_data
roughness=UNORM_linear_data
ambient_occlusion=UNORM_linear_data
height=UNORM_or_FLOAT_linear_data
mask=UNORM_linear_data
lookup_tables=linear_unless_documented_otherwise
```

Rules:

- base color is sampled through an sRGB image format so shader input is linearized before lighting/material math;
- data textures must not use sRGB formats;
- alpha is not sRGB-transformed by the color-space intent and should be treated as linear coverage/opacity data;
- shader code must not manually gamma-correct values that the sRGB image format already linearizes;
- if a source asset has ambiguous color-space metadata, renderer import should use the documented default and print a diagnostic.

Initial defaults:

```text
unknown_base_color_space=sRGB
unknown_data_texture_space=linear
ambiguous_texture_space_policy=diagnose_and_use_material_slot_default
```

## Image View Policy

First texture image view:

```text
viewType=VK_IMAGE_VIEW_TYPE_2D
aspectMask=VK_IMAGE_ASPECT_COLOR_BIT
baseMipLevel=0
levelCount=texture_mip_levels
baseArrayLayer=0
layerCount=1
components=identity_swizzle
```

Rules:

- image view format normally equals image format;
- mutable-format images are deferred;
- image view reinterpretation is forbidden until a separate file plan proves compatibility;
- component swizzle is identity for first texture path;
- YCbCr/multi-plane textures are deferred.

## Sampler Compatibility

First sampler family:

```text
sampler.texture.default
magFilter=VK_FILTER_LINEAR if supported else VK_FILTER_NEAREST
minFilter=VK_FILTER_LINEAR if supported else VK_FILTER_NEAREST
mipmapMode=VK_SAMPLER_MIPMAP_MODE_LINEAR only when mip_levels>1 and compatible
addressModeU=VK_SAMPLER_ADDRESS_MODE_REPEAT for tiling materials
addressModeV=VK_SAMPLER_ADDRESS_MODE_REPEAT for tiling materials
addressModeW=VK_SAMPLER_ADDRESS_MODE_REPEAT
anisotropyEnable=false_initially
unnormalizedCoordinates=VK_FALSE
```

Sampler rules:

- sampler choice must match format feature support;
- if linear filtering is unsupported for the selected format, use nearest filtering and print a diagnostic;
- anisotropy is deferred until device feature and limits are queried and recorded;
- compare samplers are depth/shadow-map scope and are deferred;
- sampler objects are cached by stable sampler key, not created per draw.

First sampler keys:

```text
sampler.nearest.repeat
sampler.linear.repeat
sampler.nearest.clamp
sampler.linear.clamp
```

## Mip Policy

First texture smoke:

```text
mip_levels=1
runtime_mip_generation=false
sampler_mip_mode=nearest_or_linear_but_effectively_single_level
```

Growth policy:

```text
preferred_mips=offline_precomputed
runtime_mip_generation=allowed_after_format_blit_support_query
compressed_mips=precomputed_only
```

Runtime mip generation gate:

- source/destination image usage includes transfer source and transfer destination when blitting is used;
- format supports required blit/filter features for the selected tiling;
- every mip level transition is covered by image layout policy;
- sync validation is clean;
- diagnostics print mip count and generation path.

Rules:

- no runtime mip generation in the first texture smoke;
- no compressed runtime mip generation initially;
- no per-frame mip generation;
- missing mip chains fall back to one mip level with a diagnostic;
- mip count is part of the texture GPU record.

## Compressed Format Policy

Compressed textures are deferred until after uncompressed texture/material smoke passes.

Initial compressed families:

| Family | Likely platform role | Status |
| --- | --- | --- |
| BC1/BC3/BC5/BC7 | Windows/Linux desktop GPUs, some Apple paths through MoltenVK/Metal depending on device | deferred, query required |
| ASTC LDR | Apple/mobile-oriented portability candidate, may be useful on Apple Silicon | deferred, query required |
| ETC2/EAC | portability candidate, especially mobile-class hardware | deferred, query required |
| PVRTC | Apple legacy/mobile-specific | rejected for first cross-platform desktop plan |

Rules:

- compressed format support is never assumed from OS name alone;
- enable compressed texture asset use only after physical-device features and format properties prove support;
- compressed base color still has sRGB versus UNORM variants;
- normal/data compressed textures must use linear variants;
- fallback uncompressed RGBA8 path remains required.

Feature evidence to record:

```text
textureCompressionBC=true|false
textureCompressionASTC_LDR=true|false
textureCompressionETC2=true|false
format_bc7_srgb_sampled=true|false
format_bc5_unorm_sampled=true|false
format_astc_4x4_srgb_sampled=true|false
```

## Platform Notes

macOS/MoltenVK:

- MoltenVK is a portability target, not the cross-platform source of truth;
- Vulkan format behavior still follows Vulkan-visible queried support;
- Metal-backed support can vary by macOS version, GPU family, and MoltenVK configuration;
- compressed format use must be proven by query on the target machine;
- prefer uncompressed RGBA8 first so Apple portability does not block first material proof.

Linux:

- native Vulkan drivers vary across AMD, NVIDIA, Intel, Mesa, proprietary drivers, and software paths;
- do not assume BC/ASTC/ETC support without query;
- lavapipe/headless smoke may support different features than real GPUs and must be labeled as software evidence.

Windows:

- desktop GPUs commonly support BC formats, but the renderer must still query;
- do not make BC the only asset path;
- WARP or remote/virtual GPUs may not match production device support and must be labeled.

Cross-platform rule:

```text
portable_first_path=uncompressed_RGBA8_sRGB_base_color_plus_RGBA8_UNORM_data
compressed_path=optional_after_query
```

## Fallback Textures

Required fallback resources once texture path exists:

| Fallback | Format | Bytes | Use |
| --- | --- | --- | --- |
| missing base color | selected base color sRGB format | checker or magenta/black RGBA8 | visible missing texture |
| missing normal | `VK_FORMAT_R8G8B8A8_UNORM` | flat normal RGBA | normal-map fallback |
| missing data map | `VK_FORMAT_R8G8B8A8_UNORM` or `VK_FORMAT_R8_UNORM` | neutral roughness/metallic/AO bytes | material fallback |
| failed compressed texture | uncompressed fallback format | generated CPU bytes | compressed-format unsupported path |

Rules:

- fallback textures are renderer-owned;
- fallback use is visible in diagnostics;
- fallback use does not mutate content/runtime truth;
- fallback resources must be named and included in memory budget diagnostics.

## Diagnostics Receipt

Texture smoke should print:

```text
texture_format_policy_version=
texture_format_query_api=vkGetPhysicalDeviceFormatProperties|vkGetPhysicalDeviceFormatProperties2
base_color_preferred_format=VK_FORMAT_R8G8B8A8_SRGB
base_color_selected_format=
base_color_format_sampled_supported=true|false
base_color_format_linear_filter_supported=true|false
data_texture_selected_format=
normal_texture_selected_format=
fallback_base_color_format=
fallback_normal_format=
mip_policy=mip_levels_1|offline_mips|runtime_generated
mip_levels=
runtime_mip_generation_enabled=true|false
sampler_selected=
sampler_linear_filter_enabled=true|false
sampler_anisotropy_enabled=false
texture_compression_bc_supported=true|false|unqueried
texture_compression_astc_ldr_supported=true|false|unqueried
texture_compression_etc2_supported=true|false|unqueried
platform_texture_path=macos_moltenvk|linux_native|windows_native|software
fallback_texture_used=true|false
reason=
runtime_state_touched=false
```

## Validation And Tests

Firewall scan:

```sh
rg -n "VkFormat|VK_FORMAT_|VkSampler|VK_FILTER_|VK_SAMPLER_|textureCompression|VK_FORMAT_FEATURE" src/runtime src/content src/projection src/runtime/save src/render/FrameInput.hpp src/render/RendererApi.hpp
```

Expected result:

```text
no runtime/content/projection/save/renderer-api texture format ownership leak
```

Unit tests should cover:

- base color uses sRGB format candidates;
- data textures reject sRGB formats;
- ambiguous material slot uses documented default;
- unsupported preferred format falls through to next candidate;
- unsupported sampled format rejects texture creation;
- missing linear-filter support falls back to nearest sampler;
- mip generation is disabled without required format/blit support;
- compressed formats remain disabled until query evidence exists;
- fallback texture selection does not alter runtime state.

Smoke tests should cover:

- create/upload/sample one sRGB base color texture;
- create/upload/sample one linear fallback/data texture once material path exists;
- print selected format and feature support;
- print sampler key;
- print mip count;
- print fallback use;
- run on macOS/MoltenVK first, then Linux native Vulkan, then Windows native Vulkan.

## Failure Reason Codes

Use stable reason codes in diagnostics:

```text
texture_format_scope_blocked
texture_format_query_failed
texture_format_candidate_unsupported
texture_format_sampled_bit_missing
texture_format_image_create_unsupported
texture_format_srgb_required_but_unavailable
texture_format_data_used_srgb
texture_format_linear_filter_unavailable
texture_format_sampler_fallback_nearest
texture_format_mip_generation_unavailable
texture_format_runtime_mips_disabled
texture_format_compression_unqueried
texture_format_compression_unsupported
texture_format_fallback_created
texture_format_fallback_used
texture_format_platform_quirk
texture_format_runtime_leak
```

## Acceptance Gate

This policy is ready for implementation planning when:

- first base-color format ladder is accepted;
- data texture linear rules are accepted;
- fallback texture bytes are specified in a file plan;
- sampler fallback behavior is accepted;
- mip policy is accepted as one mip first, offline mips preferred later;
- compressed formats remain optional and query-gated;
- diagnostics receipt fields are accepted;
- macOS, Linux, and Windows lanes all require format query receipts;
- firewall scan has no Vulkan texture format leaks outside renderer-owned files.
