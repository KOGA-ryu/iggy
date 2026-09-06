# Dear ImGui (docking branch) — vendored

- Upstream: https://github.com/ocornut/imgui
- Tag: `v1.92.8-docking` (IMGUI_VERSION "1.92.8", IMGUI_VERSION_NUM 19280)
- Upstream commit: `b61e56346a92cfcaf1f43a545ca37b0b32239654`
- Vendored: 2026-07-14 (clone of the tag, file subset below; no local modifications)
- License: MIT (see LICENSE.txt)
- Build target: `iggy3d_imgui` (cmake/iggy3d_imgui.cmake). Third-party code: no
  iggy3d warning flags, SYSTEM includes (see docs/creative_desktop_ui_plan.md DL-8).
- imconfig.h is upstream-default. It is the only file that may ever be customized;
  record any future change here.

Update procedure: clone the new `-docking` tag, verify the tag's commit SHA,
re-copy exactly this file set, refresh the hashes below, update
cmake/iggy3d_imgui.cmake if upstream adds/renames sources.

## File set (SHA-256)

```
1353bfa59da16eec481723fd4e5d910356f0406b79e3c4142e0c9867bafffab5  imconfig.h
da6d77803fc8963ca6a6826d6d6d08e3be74fdd7f527841b90858e3e545bb478  imgui.h
f72ec91731b9114a6cee203f575582b9859825a4f66836cf32cc7e70488c7aa6  imgui_internal.h
889b396795202d1457560a797a7242e96f6f132d4b88ca2d69be58bf05e1771f  imstb_rectpack.h
a985f5fa0ed97353d493b497961e9eef52082edcd045cf6954b69990ec9d0741  imstb_textedit.h
c51a0f7e7ea760f2366bd3752635ec58e21fccfec4a832501639990ba6ce0528  imstb_truetype.h
49cca8475198925b69b128985148f29eb37db689a49c7c6f0dba6d2fac5415bd  imgui.cpp
6814299d0618749a6fc73b0259a42bd6c2eabf5b55c0bfef293bfdcda68155af  imgui_draw.cpp
33b5146f5eb362605a166e532b336d1955579743eab719d96241a163a21222eb  imgui_tables.cpp
7b46ca08ca95e046aa57a88d2838933843851348a42f3a738d57ebdaa2f5c682  imgui_widgets.cpp
173506a2d6f7fb67990d257fb2507f188690eca39060c39469ae7bef43aae2a3  LICENSE.txt
a7716305e9312d32d7ffb7806555d3ba518c6df6dbf26077d5f6a6e022c6efe5  backends/imgui_impl_sdl3.h
4a22edbd5589a7e6db7ba4df452ce7264cbff1f1467568ea848a75fd8738d3b4  backends/imgui_impl_vulkan.h
cc4c25e1fd31a6e50af0cdc41be636c084cbe115a9ee97e1f85d4839ef54b0f9  backends/imgui_impl_sdl3.cpp
ff4a9b986f3e5cddaa6b5b1c288d02fa2fddc4b8d98942c07195d71e4878145a  backends/imgui_impl_vulkan.cpp
```

Deliberately NOT vendored: imgui_demo.cpp (nothing may call ShowDemoWindow in
committed code), misc/, examples/, docs/, other backends.
